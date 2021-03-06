//
//  ImageManager.cpp
//  MiRay/resources
//
//  Created by Damir Sagidullin on 08.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "ImageManager.h"
#include "../ThirdParty/FreeImage/FreeImage.h"

using namespace mr;

// ------------------------------------------------------------------------ //

ImageManager::ImageManager()
{
}

ImageManager::~ImageManager()
{
}

// ------------------------------------------------------------------------ //

void ImageManager::Release(const std::string & name)
{
	auto it = m_resources.find(name);
	if (it != m_resources.end())
		m_resources.erase(it);
}

ImagePtr ImageManager::Create(int w, int h, Image::eType t, const char * name)
{
	ImagePtr spImage;
	switch (t)
	{
		case Image::TYPE_1B: spImage.reset(new Image1B(*this, name)); break;
		case Image::TYPE_3B: spImage.reset(new Image3B(*this, name)); break;
		case Image::TYPE_4B: spImage.reset(new Image4B(*this, name)); break;
		case Image::TYPE_1W: spImage.reset(new Image1W(*this, name)); break;
		case Image::TYPE_3W: spImage.reset(new Image3W(*this, name)); break;
		case Image::TYPE_4W: spImage.reset(new Image4W(*this, name)); break;
		case Image::TYPE_1F: spImage.reset(new Image1F(*this, name)); break;
		case Image::TYPE_3F: spImage.reset(new Image3F(*this, name)); break;
		case Image::TYPE_4F: spImage.reset(new Image4F(*this, name)); break;
		default: break;
	}
	if (!spImage)
		return nullptr;

	m_resources[spImage->Name()] = spImage;

	spImage->Create(w, h);

	return std::move(spImage);
}

ImagePtr ImageManager::CreateCopy(const Image * pSrcImage, Image::eType t, const char * name)
{
	if (!pSrcImage)
		return nullptr;

	const int w = pSrcImage->Width();
	const int h = pSrcImage->Height();
	ImagePtr spImage = Create(w, h, t, name);
	if (!spImage || !spImage->Data())
		return nullptr;

	if (pSrcImage->Type() == spImage->Type())
	{
		memcpy(spImage->DataB(), pSrcImage->DataB(), w * h * spImage->PixelSize());
	}
	else
	{
		for (int y = 0; y < h; y++)
		{
			for (int x = 0; x < w; x++)
				spImage->SetPixel(x, y, pSrcImage->GetPixel(x, y));
		}
	}
	
	return std::move(spImage);
}

// ------------------------------------------------------------------------ //

//bool ReadFile(std::vector<byte> & data, const char * strFilename)
//{
//	FILE * f = fopen(strFilename, "rb");
//	if (!f)
//		return false;
//
//	fseek(f, 0, SEEK_END);
//	size_t fileSize = ftell(f);
//	fseek(f, 0, SEEK_SET);
//
//	data.resize(fileSize);
//	size_t readSize = fread(data.data(), 1, fileSize, f);
//
//	fclose(f);
//
//	return !data.empty() && (readSize == fileSize);
//}
//
//bool SaveFile(const char * strFilename, const std::vector<byte> & buf)
//{
//	FILE * f = fopen(strFilename, "wb");
//	if (!f)
//		return false;
//	
//	size_t writeSize = fwrite(buf.data(), buf.size(), 1, f);
//	
//	fclose(f);
//	
//	return writeSize == buf.size();
//}

// ------------------------------------------------------------------------ //

ImagePtr ImageManager::Load(const char * strFilename)
{
	if (!strFilename || !*strFilename)
		return nullptr;

	auto it = m_resources.find(strFilename);
	if (it != m_resources.end())
	{
		ImagePtr spImage = it->second.lock();
		if (spImage)
			return spImage;
	}

	printf("Loading image '%s'...\n", strFilename);
	
	//check the file signature and deduce its format
	FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(strFilename, 0);
	if (fif == FIF_UNKNOWN) // if still unknown, try to guess the file format from the file extension
		fif = FreeImage_GetFIFFromFilename(strFilename);

	if (fif == FIF_UNKNOWN)
		return nullptr; // if still unkown, return failure

	//check that the plugin has reading capabilities and load the file
	FIBITMAP * dib = nullptr;
	if (FreeImage_FIFSupportsReading(fif))
		dib = FreeImage_Load(fif, strFilename);

	if (!dib)
		return nullptr; // if the image failed to load, return failure

	uint32 width = FreeImage_GetWidth(dib);
	uint32 height = FreeImage_GetHeight(dib);
	uint32 bpp = FreeImage_GetBPP(dib);
	if ((FreeImage_GetBits(dib) == nullptr) || (width == 0) || (height == 0) || (bpp == 0))
	{// if this somehow one of these failed (they shouldn't), return failure
		FreeImage_Unload(dib);
		return nullptr;
	}

	ImagePtr spImage;

//	unsigned dpmX = FreeImage_GetDotsPerMeterX(dib);
//	unsigned dpmY = FreeImage_GetDotsPerMeterY(dib);
	FREE_IMAGE_TYPE imageType = FreeImage_GetImageType(dib);
	FREE_IMAGE_COLOR_TYPE colorType = FreeImage_GetColorType(dib);
	if ((colorType == FIC_RGB) || (colorType == FIC_RGBALPHA) || (colorType == FIC_PALETTE))
	{
		Image::eType it = Image::TYPE_NONE;
		if (colorType != FIC_PALETTE)
		{
			switch (imageType)
			{
				case FIT_BITMAP:
					if (bpp == 24)
						it = Image::TYPE_3B;
					else if (bpp == 32)
						it = Image::TYPE_4B;
					break;
				case FIT_RGB16:	 it = Image::TYPE_3W; break;
				case FIT_RGBA16: it = Image::TYPE_4W; break;
				case FIT_RGBF:	 it = Image::TYPE_3F; break;
				case FIT_RGBAF:	 it = Image::TYPE_4F; break;
				default:		 it = Image::TYPE_NONE; break;
			}
		}

		if (it == Image::TYPE_NONE)
		{
			it = Image::TYPE_3B;
			FIBITMAP * dibNew = FreeImage_ConvertTo24Bits(dib);
			FreeImage_Unload(dib);
			dib = dibNew;
		}

		byte * bits = FreeImage_GetBits(dib);
		if (bits != nullptr)
		{
			spImage = Create(width, height, it, strFilename);
			if (spImage && spImage->Data())
			{
				uint32 srcPitch = FreeImage_GetPitch(dib);
				uint32 destPixelSize = spImage->PixelSize();
				uint32 destPitch = static_cast<uint32>(spImage->Width() * destPixelSize);
				uint32 pitch = std::min(srcPitch, destPitch);
				for (uint32 y = 0 ; y < height; y++)
					memcpy(spImage->DataB() + y * destPitch, bits + (height - y - 1) * srcPitch, pitch);

				if ((FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR) && (bpp == 24 || bpp == 32))
				{
					for (uint32 y = 0 ; y < height; y++)
					{
						byte * pDest = spImage->DataB() + y * destPitch;
						for (uint32 x = 0; x < width; x++)
						{
							std::swap(pDest[0], pDest[2]);
							pDest += destPixelSize;
						}
					}
				}
			}
		}
	}
	else if (colorType == FIC_MINISBLACK) // greyscale
	{
		Image::eType it;
		switch (imageType)
		{
			case FIT_BITMAP: it = (bpp == 8) ? Image::TYPE_1B : Image::TYPE_NONE; break;
			case FIT_UINT16: it = Image::TYPE_1W; break;
			case FIT_FLOAT:	 it = Image::TYPE_1F; break;
			default:		 it = Image::TYPE_NONE; break;
		}

		if (it == Image::TYPE_NONE)
		{
			it = Image::TYPE_1B;
			FIBITMAP * dibNew = FreeImage_ConvertToGreyscale(dib);
			FreeImage_Unload(dib);
			dib = dibNew;
		}

		byte * bits = FreeImage_GetBits(dib);
		if (bits != nullptr)
		{
			spImage = Create(width, height, it, strFilename);
			if (spImage && spImage->Data())
			{
				uint32 srcPitch = FreeImage_GetPitch(dib);
				uint32 destPixelSize = spImage->PixelSize();
				uint32 destPitch = static_cast<uint32>(spImage->Width() * destPixelSize);
				uint32 pitch = std::min(srcPitch, destPitch);
				for (uint32 y = 0 ; y < height; y++)
					memcpy(spImage->DataB() + y * destPitch, bits + (height - y - 1) * srcPitch, pitch);
			}
		}
	}

	FreeImage_Unload(dib);

	return std::move(spImage);
}

// ------------------------------------------------------------------------ //

bool ImageManager::Save(const char * strFilename, const Image & image, bool saveAlpha, eFileFormat ff)
{
	FREE_IMAGE_FORMAT fif;
	switch (ff)
	{
		default:
		case FILE_FORMAT_AUTO:	fif = FreeImage_GetFIFFromFilename(strFilename); break;
		case FILE_FORMAT_PNG:	fif = FIF_PNG; break;
		case FILE_FORMAT_JPEG:	fif = FIF_JPEG; break;
		case FILE_FORMAT_TIFF:	fif = FIF_TIFF; break;
		case FILE_FORMAT_TARGA:	fif = FIF_TARGA; break;
		case FILE_FORMAT_DDS:	fif = FIF_DDS; break;
	}

	FREE_IMAGE_TYPE type;
	switch (image.Type())
	{
		default: type = FIT_BITMAP; break;
		case Image::TYPE_1W: type = FIT_UINT16; break;
		case Image::TYPE_3W: type = FIT_RGB16; break;
		case Image::TYPE_4W: type = saveAlpha ? FIT_RGBA16 : FIT_RGB16; break;
		case Image::TYPE_1F: type = FIT_FLOAT; break;
		case Image::TYPE_3F: type = FIT_RGBF; break;
		case Image::TYPE_4F: type = saveAlpha ? FIT_RGBAF : FIT_RGBF; break;
	}

	if (!FreeImage_FIFSupportsExportType(fif, type))
	{
		switch (type)
		{
			default:		type = FIT_BITMAP; break;
			case FIT_FLOAT:	type = FIT_INT16; break;
			case FIT_RGBF:	type = FIT_RGB16; break;
			case FIT_RGBAF:	type = FIT_RGBA16; break;
		}
		if (!FreeImage_FIFSupportsExportType(fif, type))
			type = FIT_BITMAP;
	}

	Image::eType imageType;
	if (type == FIT_BITMAP)
	{
		int bpp = image.NumChannels();
		if (!saveAlpha && bpp == 4)
			bpp = 3;

		if (!FreeImage_FIFSupportsExportBPP(fif, bpp * 8))
		{
			if (bpp == 3 || !FreeImage_FIFSupportsExportBPP(fif, 24))
				return false;

			bpp = 3;
		}

		switch (bpp)
		{
			case 1: imageType = Image::TYPE_1B; break;
			case 3: imageType = Image::TYPE_3B; break;
			case 4: imageType = Image::TYPE_4B; break;
			default: return false;
		}
	}
	else
	{
		switch (type)
		{
			case FIT_UINT16: imageType = Image::TYPE_1W; break;
			case FIT_RGB16:	 imageType = Image::TYPE_3W; break;
			case FIT_RGBA16: imageType = Image::TYPE_4W; break;
			case FIT_FLOAT:	 imageType = Image::TYPE_1F; break;
			case FIT_RGBF:	 imageType = Image::TYPE_3F; break;
			case FIT_RGBAF:	 imageType = Image::TYPE_4F; break;
			default: return false;
		}
	}
	
	int width = image.Width();
	int height = image.Height();
	int bpp = image.PixelSize();
	const byte * pSrcData = image.DataB();

	ImagePtr pTmpImage;
	if (image.Type() != imageType)
	{
		pTmpImage = CreateCopy(&image, imageType);
		if (!pTmpImage)
			return false;

		pSrcData = pTmpImage->DataB();
		bpp = pTmpImage->PixelSize();
	}

	bool res = false;

	FIBITMAP * dib = FreeImage_AllocateT(type, width, height, bpp * 8);
	if (dib != nullptr)
	{
		byte * bits = FreeImage_GetBits(dib);
		if (bits != nullptr)
		{
			uint32 destPitch = FreeImage_GetPitch(dib);
			uint32 srcPitch = static_cast<uint32>(width * bpp);
			uint32 pitch = std::min(srcPitch, destPitch);
			for (int y = 0 ; y < height; y++)
				memcpy(bits + (height - y - 1) * destPitch, pSrcData + y * srcPitch, pitch);

			if ((FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR) && (imageType == Image::TYPE_3B || bpp == Image::TYPE_4B))
			{
				for (int y = 0 ; y < height; y++)
				{
					byte * pDest = bits + y * destPitch;
					for (int x = 0; x < width; x++)
					{
						std::swap(pDest[0], pDest[2]);
						pDest += bpp;
					}
				}
			}

			res = FreeImage_Save(fif, dib, strFilename) != FALSE;
		}

		FreeImage_Unload(dib);
	}

	return res;
}

ImagePtr ImageManager::LoadNormalmap(const char * strFilename)
{
	if (!strFilename || !*strFilename)
		return nullptr;

	std::string strNormalmapName = std::string("normalmap@") + strFilename;

	auto it = m_resources.find(strNormalmapName);
	if (it != m_resources.end())
	{
		ImagePtr res = it->second.lock();
		if (res)
			return res;
	}

	ImagePtr pImage = Load(strFilename);
	if (!pImage)
		return nullptr;

	ImagePtr spNormalmapImage(new Image4F(*this, strNormalmapName.c_str()));
	if (!spNormalmapImage)
		return nullptr;

	m_resources[spNormalmapImage->Name()] = spNormalmapImage;

	const int w = pImage->Width();
	const int h = pImage->Height();
	const Vec2 scale((float)w / 256.f, (float)h / 256.f);
	spNormalmapImage->Create(w, h);

	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			Vec3 normal;
			normal.x = (pImage->GetPixel(x > 0 ? x - 1 : w - 1, y).r -
						pImage->GetPixel(x + 1 < w ? x + 1 : 0, y).r) * scale.x;
			normal.y = (pImage->GetPixel(x, y > 0 ? y - 1 : h - 1).r -
						pImage->GetPixel(x, y + 1 < h ? y + 1 : 0).r) * scale.y;
			normal.z = 1.f / 128.f;
			normal.Normalize();
			normal = normal * 0.5f + Vec3(0.5f, 0.5f, 0.5f);
			spNormalmapImage->SetPixel(x, y, ColorF(normal.x, normal.y, normal.z, pImage->GetPixel(x, y).r));
		}
	}

	return std::move(spNormalmapImage);
}
