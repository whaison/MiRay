//
//  node.cpp
//  MiRay/resources
//
//  Created by Damir Sagidullin on 21.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "MaterialResource.h"

namespace mr
{

class MaterialTexture
{
	MaterialParameter	&m_param;
	std::string			m_filename;
	Image				*m_pTexture;

public:
	MaterialTexture(MaterialParameter & param) : m_param(param), m_pTexture(nullptr)
	{
	}

	~MaterialTexture()
	{
		SAFE_RELEASE(m_pTexture);
	}

	MaterialParameter & Parameter() { return m_param; }
	std::string & Filename() { return m_filename; }
	Image * & Texture() { return m_pTexture; }
};

class MaterialLayerImpl : public MaterialLayer
{
	friend MaterialManager;
		
public:
	MaterialLayerImpl();
	~MaterialLayerImpl();

	MaterialTexture	m_ambientTexture;
	MaterialTexture	m_emissiveTexture;
	MaterialTexture	m_diffuseTexture;
	MaterialTexture	m_opacityTexture;

	MaterialTexture	m_iorTexture;
	
	MaterialTexture	m_reflectionTexture;
	MaterialTexture	m_reflectionTintTexture;
	MaterialTexture	m_reflectionGlossinessTexture;
	MaterialTexture	m_reflectionExitColorTexture;

	MaterialTexture	m_refractionTintTexture;
	MaterialTexture	m_refractionGlossinessTexture;
	MaterialTexture	m_refractionExitColorTexture;
	
	std::string		m_bumpMapName;
	Image			*m_pNormalmap;

	MaterialTexture	m_bumpLevelTexture;

	void LoadTextures(ImageManager * pImageManager, const char * strLocalPath);

	void Load(pugi::xml_node node);
	void Save(pugi::xml_node node);
};

}

using namespace mr;

MaterialLayerImpl::MaterialLayerImpl()
	: m_ambientTexture(m_ambient)
	, m_emissiveTexture(m_emissive)
	, m_diffuseTexture(m_diffuse)
	, m_opacityTexture(m_opacity)
	, m_iorTexture(m_indexOfRefraction)
	, m_reflectionTexture(m_reflection)
	, m_reflectionTintTexture(m_reflectionTint)
	, m_reflectionGlossinessTexture(m_reflectionGlossiness)
	, m_reflectionExitColorTexture(m_reflectionExitColor)
	, m_refractionTintTexture(m_refractionTint)
	, m_refractionGlossinessTexture(m_refractionGlossiness)
	, m_refractionExitColorTexture(m_refractionExitColor)
	, m_pNormalmap(nullptr)
	, m_bumpLevelTexture(m_bumpLevel)
{
}

MaterialLayerImpl::~MaterialLayerImpl()
{
	SAFE_RELEASE(m_pNormalmap);
}

// ------------------------------------------------------------------------ //

static ColorF ColorFromString(const char * str, const ColorF & def = ColorF::Null)
{
	if (!str || !*str)
		return def;
	
	ColorF out;
	int n = sscanf(str, "%f %f %f", &out.r, &out.g, &out.b);
	if (n == 1)
		out.g = out.b = out.r;
	else if (n != 3)
		return def;
	return out;
}

static float FloatFromString(const char * str, float def = 0.f)
{
	float out;
	if (!str || !*str || sscanf(str, "%f", &out) != 1)
		return def;
	return out;
}

static void BoolFromString(bool & out, const char * str, bool def = false)
{
	int i = 0;
	if (!str || !*str || sscanf(str, "%d", &i) != 1)
		out = def;
	
	out = (i != 0);
}

bool ReadMaterialParam(MaterialParameter & param, std::string & strTextureName, pugi::xml_node node, const char * name)
{
	pugi::xml_node value = node.child(name);
	if (!value.empty())
		param.SetColor(ColorFromString(value.text().get(), param.GetColor()));

	char map_name[64];
	strcpy(map_name, name);
	strcat(map_name, "-map");
	pugi::xml_node map = node.child(map_name);
	if (!map.empty())
		strTextureName = map.text().get();

	return true;
}

// ------------------------------------------------------------------------ //

static const char * ColorToString(const ColorF & c, char * buf, size_t sz)
{
	if (c.r == c.g && c.r == c.b)
		snprintf(buf, sz, "%g", c.r);
	else
		snprintf(buf, sz, "%g %g %g", c.r, c.g, c.b);
	return buf;
}

static const char * FloatToString(float f, char * buf, size_t sz)
{
	snprintf(buf, sz, "%g", f);
	return buf;
}

static const char * BoolToString(bool b, char * buf, size_t sz)
{
	snprintf(buf, sz, "%d", b ? 1 : 0);
	return buf;
}

void SaveMaterialParam(pugi::xml_node node, const char * name, const MaterialParameter & param, const std::string & strTextureName)
{
	char buf[256];
	node.append_child(name).text().set(ColorToString(param.GetColor(), buf, sizeof(buf)));
	if (!strTextureName.empty())
	{
		strcpy(buf, name);
		strcat(buf, "-map");
		node.append_child(buf).text().set(strTextureName.c_str());
	}
}

// ------------------------------------------------------------------------ //

void MaterialLayerImpl::Load(pugi::xml_node node)
{
	ReadMaterialParam(m_ambient, m_ambientTexture.Filename(), node, "ambient");
	ReadMaterialParam(m_emissive, m_emissiveTexture.Filename(), node, "emissive");
	ReadMaterialParam(m_diffuse, m_diffuseTexture.Filename(), node, "diffuse");
	ReadMaterialParam(m_opacity, m_opacityTexture.Filename(), node, "opacity");
	
	ReadMaterialParam(m_indexOfRefraction, m_iorTexture.Filename(), node, "ior");
	
	ReadMaterialParam(m_reflection, m_reflectionTexture.Filename(), node, "reflection");
	ReadMaterialParam(m_reflectionTint, m_reflectionTintTexture.Filename(), node, "reflection-tint");
	ReadMaterialParam(m_reflectionGlossiness, m_reflectionGlossinessTexture.Filename(), node, "reflection-glossiness");
	
	ReadMaterialParam(m_refractionTint, m_refractionTintTexture.Filename(), node, "refraction-tint");
	ReadMaterialParam(m_refractionGlossiness, m_refractionGlossinessTexture.Filename(), node, "refraction-glossiness");
	
	BoolFromString(m_fresnelReflection, node.child("fresnel-reflection").text().get(), m_fresnelReflection);

	pugi::xml_node bump = node.child("bump-map");
	if (!bump.empty())
		m_bumpMapName = bump.text().get();

	ReadMaterialParam(m_bumpLevel, m_bumpLevelTexture.Filename(), node, "bump-level");
}

void MaterialLayerImpl::Save(pugi::xml_node node)
{
	if (!m_ambientTexture.Filename().empty() || (m_ambient.GetColor() != ColorF::Null))
		SaveMaterialParam(node, "ambient", m_ambient, m_ambientTexture.Filename());

	if (!m_emissiveTexture.Filename().empty() || (m_emissive.GetColor() != ColorF::Null))
		SaveMaterialParam(node, "emissive", m_emissive, m_emissiveTexture.Filename());

	SaveMaterialParam(node, "diffuse", m_diffuse, m_diffuseTexture.Filename());
	
	SaveMaterialParam(node, "opacity", m_opacity, m_opacityTexture.Filename());
	
	SaveMaterialParam(node, "ior", m_indexOfRefraction, m_iorTexture.Filename());
	
	char buf[256];
	node.append_child("fresnel-reflection").text().set(BoolToString(FresnelReflection(), buf, sizeof(buf)));

	if (!m_reflectionTexture.Filename().empty() || (m_reflection.GetColor() != ColorF::Null))
		SaveMaterialParam(node, "reflection", m_reflection, m_reflectionTexture.Filename());

	if (!m_reflectionTintTexture.Filename().empty() || (m_reflectionTint.GetColor() != ColorF::White))
		SaveMaterialParam(node, "reflection-tint", m_reflectionTint, m_reflectionTintTexture.Filename());

	if (!m_reflectionGlossinessTexture.Filename().empty() || (m_reflectionGlossiness.GetColor() != ColorF::White))
		SaveMaterialParam(node, "reflection-glossiness", m_reflectionGlossiness, m_reflectionGlossinessTexture.Filename());

	if (!m_reflectionExitColorTexture.Filename().empty() || (m_reflectionExitColor.GetColor() != ColorF::Null))
		SaveMaterialParam(node, "reflection-exit-color", m_reflectionExitColor, m_reflectionExitColorTexture.Filename());

	if (!m_refractionTintTexture.Filename().empty() || (m_refractionTint.GetColor() != ColorF::White))
		SaveMaterialParam(node, "refraction-tint", m_refractionTint, m_refractionTintTexture.Filename());

	if (!m_refractionGlossinessTexture.Filename().empty() || (m_refractionGlossiness.GetColor() != ColorF::White))
		SaveMaterialParam(node, "refraction-glossiness", m_refractionGlossiness, m_refractionGlossinessTexture.Filename());

	if (!m_refractionExitColorTexture.Filename().empty() || (m_refractionExitColor.GetColor() != ColorF::Null))
		SaveMaterialParam(node, "refraction-exit-color", m_refractionExitColor, m_refractionExitColorTexture.Filename());

	if (!m_bumpMapName.empty())
	{
		node.append_child("bump-map").text().set(m_bumpMapName.c_str());

		if (!m_bumpLevelTexture.Filename().empty() || (m_bumpLevel.GetColor() != ColorF::Null))
			SaveMaterialParam(node, "bump-level", m_bumpLevel, m_bumpLevelTexture.Filename());
	}
}

// ------------------------------------------------------------------------ //

ePixelFormat PixelFormat(Image::eType type)
{
	switch (type)
	{
		case Image::TYPE_1B: return PIXEL_FORMAT_1B;
		case Image::TYPE_3B: return PIXEL_FORMAT_3B;
		case Image::TYPE_4B: return PIXEL_FORMAT_4B;
		case Image::TYPE_1F: return PIXEL_FORMAT_1F;
		case Image::TYPE_3F: return PIXEL_FORMAT_3F;
		case Image::TYPE_4F: return PIXEL_FORMAT_4F;
		default: return PIXEL_FORMAT_NONE;
	}
}

void LoadTexture(MaterialTexture & texture, ImageManager * pImageManager, const char * strLocalPath)
{
	if (texture.Filename().empty())
		return;

	Image * pTexture = pImageManager->Load((strLocalPath + texture.Filename()).c_str());
	if (!pTexture)
		return;

	texture.Texture() = pTexture;
	texture.Parameter().SetTexture(pTexture->Width(), pTexture->Height(), PixelFormat(pTexture->Type()),
								   pTexture->Width() * pTexture->PixelSize(), pTexture->Data());
}

void MaterialLayerImpl::LoadTextures(ImageManager * pImageManager, const char * strLocalPath)
{
	LoadTexture(m_ambientTexture, pImageManager, strLocalPath);
	LoadTexture(m_emissiveTexture, pImageManager, strLocalPath);
	LoadTexture(m_diffuseTexture, pImageManager, strLocalPath);
	LoadTexture(m_opacityTexture, pImageManager, strLocalPath);
	LoadTexture(m_iorTexture, pImageManager, strLocalPath);
	LoadTexture(m_reflectionTexture, pImageManager, strLocalPath);
	LoadTexture(m_reflectionTintTexture, pImageManager, strLocalPath);
	LoadTexture(m_reflectionGlossinessTexture, pImageManager, strLocalPath);
	LoadTexture(m_reflectionExitColorTexture, pImageManager, strLocalPath);
	LoadTexture(m_refractionTintTexture, pImageManager, strLocalPath);
	LoadTexture(m_refractionGlossinessTexture, pImageManager, strLocalPath);
	LoadTexture(m_refractionExitColorTexture, pImageManager, strLocalPath);
	LoadTexture(m_bumpLevelTexture, pImageManager, strLocalPath);

	if (!m_bumpMapName.empty())
		m_pNormalmap = pImageManager->LoadNormalmap((strLocalPath + m_bumpMapName).c_str());
}

// ------------------------------------------------------------------------ //

MaterialResource::MaterialResource(MaterialManager & owner, const char * strName)
	: IResource(owner, strName)
{
}

MaterialResource::~MaterialResource()
{
	while (m_layers.empty())
	{
		delete m_layers.back();
		m_layers.pop_back();
	}
}

void MaterialResource::Create()
{
	m_layers.push_back(new MaterialLayerImpl());
}

void MaterialResource::Load(pugi::xml_node node)
{
	for (pugi::xml_node layer = node.child("layer"); layer; layer = layer.next_sibling("layer"))
	{
		m_layers.push_back(new MaterialLayerImpl());
		m_layers.back()->Load(layer);
	}

	if (m_layers.empty())
		m_layers.push_back(new MaterialLayerImpl());
}

void MaterialResource::Save(pugi::xml_node node)
{
	for (auto it = m_layers.begin(); it != m_layers.end(); ++it)
		(*it)->Save(node.append_child("layer"));
}

void MaterialResource::LoadTextures(ImageManager * pImageManager, const char * strLocalPath)
{
	for (auto it = m_layers.begin(); it != m_layers.end(); ++it)
		(*it)->LoadTextures(pImageManager, strLocalPath);
}

size_t MaterialResource::NumLayers() const
{
	return m_layers.size();
}

const MaterialLayer * MaterialResource::Layer(size_t i) const
{
	return i < m_layers.size() ? m_layers[i] : nullptr;
}

// ------------------------------------------------------------------------ //
