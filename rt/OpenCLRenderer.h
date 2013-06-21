//
//  OpenCLRenderer.h
//  MiRay/rt
//
//  Created by Damir Sagidullin on 12.06.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

#ifdef _WIN32
#include <CL/opencl.h>
#else
#include <OpenCL/opencl.h>
#endif

namespace mr
{
	
class Image;

class OpenCLRenderer
{	
	BVH & m_scene;
	cl_device_type		m_deviceType;
	cl_device_id		m_deviceId;
	cl_context			m_context;
	cl_command_queue	m_commands;
	cl_kernel			m_kernel;
	cl_program			m_program;
	cl_mem				m_triangles;
	cl_mem				m_materials;
	cl_mem				m_textures;
	cl_mem				m_lights;
	cl_mem				m_nodes;
	cl_mem				m_nodeTriangles;
	cl_mem				m_result;
	size_t				m_localWorkSize[2];
	bool				m_bInitialized;

	cl_uint	m_size[4];
	Image *	m_pImage;
	RectI	m_rcRenderArea;
	Vec3	m_vEyePos;
	ColorF	m_bgColor;
	float	m_fFrameBlend;
	Vec3	m_vCamDelta[3];
	Vec2	m_dp;
	const Image * m_pEnvironmentMap;
	
	typedef TVec2<int>	Vec2I;
	Vec2I	m_pos;
	Vec2I	m_delta;
	
	float	m_fDistEpsilon;
	int		m_nMaxDepth;

	bool SetupComputeDevices();
	bool SetupComputeKernel(const char * pKernelFilename, const char * pKernelMethodName);
	bool SetupSceneBuffers();
	void UpdateResultBuffer(cl_uint width, cl_uint height);

public:
	
	OpenCLRenderer(BVH & scene, const char * pKernelFilename, const char * pKernelMethodName);
	~OpenCLRenderer();
	
	void Render(Image & image, const RectI * pViewportRect,
				const Matrix & matCamera, const Matrix & matViewProj,
				const ColorF & bgColor, const Image * pEnvironmentMap,
				int numThreads, int nFrameNumber);
	
	void Join();
	void Interrupt();
};
	
}