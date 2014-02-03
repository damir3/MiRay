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
	
class IImage;

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
	cl_float3			m_camPosition;
	cl_float3			m_camDirectionX;
	cl_float3			m_camDirectionY;
	cl_float3			m_camDirectionZ;
	cl_float2			m_dp;
	cl_float			m_fFrameBlend;
	size_t				m_localWorkSize[2];
	cl_uint				m_size[4];
	bool				m_bInitialized;
	
	bool SetupComputeDevices();
	bool SetupComputeKernel(const char * pKernelFilename, const char * pKernelMethodName);
	bool SetupSceneBuffers();
	void UpdateResultBuffer(cl_uint width, cl_uint height);

public:
	
	OpenCLRenderer(BVH & scene, const char * pKernelFilename, const char * pKernelMethodName);
	~OpenCLRenderer();
	
	void Render(IImage & image, const RectI * pViewportRect,
				const Matrix & matCamera, const Matrix & matViewProj,
				const ColorF & bgColor, const IImage * pEnvironmentMap,
				int numThreads, int nFrameNumber);
	
	void Join();
	void Interrupt();
};
	
}
