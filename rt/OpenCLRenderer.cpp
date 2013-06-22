//
//  OpenCLRenderer.cpp
//  MiRay/rt
//
//  Created by Damir Sagidullin on 12.06.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "OpenCLRenderer.h"

using namespace mr;

#define SAFE_RELEASE_MEM_OBJECT(a)			if (a) { clReleaseMemObject(a); (a) = NULL; }

OpenCLRenderer::OpenCLRenderer(BVH & scene, const char * pKernelFilename, const char * pKernelMethodName)
	: m_scene(scene)
	, m_deviceType(CL_DEVICE_TYPE_GPU)
	, m_deviceId(NULL)
	, m_context(NULL)
	, m_commands(NULL)
	, m_kernel(NULL)
	, m_program(NULL)
	, m_triangles(NULL)
	, m_materials(NULL)
	, m_textures(NULL)
	, m_lights(NULL)
	, m_nodes(NULL)
	, m_nodeTriangles(NULL)
	, m_result(NULL)
	, m_bInitialized(false)
{
	std::fill(m_size, m_size + 4, (cl_uint)0);

	if (!SetupComputeDevices())
		return;

	if (!SetupComputeKernel(pKernelFilename, pKernelMethodName))
		return;

	if (!SetupSceneBuffers())
		return;

	m_bInitialized = true;
}

OpenCLRenderer::~OpenCLRenderer()
{
	clFinish(m_commands);
	clReleaseKernel(m_kernel);
	clReleaseProgram(m_program);
	clReleaseCommandQueue(m_commands);
	m_kernel = NULL;
	m_program = NULL;
	m_commands = NULL;

	SAFE_RELEASE_MEM_OBJECT(m_result);
	SAFE_RELEASE_MEM_OBJECT(m_nodes);
	SAFE_RELEASE_MEM_OBJECT(m_nodeTriangles);
	SAFE_RELEASE_MEM_OBJECT(m_lights);
	SAFE_RELEASE_MEM_OBJECT(m_textures);
	SAFE_RELEASE_MEM_OBJECT(m_materials);
	SAFE_RELEASE_MEM_OBJECT(m_triangles);

	clReleaseContext(m_context);
	m_context = NULL;
}

#ifndef __APPLE__
void clLogMessagesToStdout(const char * str, const void *, size_t, void *)
{
	printf(str);
};
#endif

bool OpenCLRenderer::SetupComputeDevices()
{
	cl_uint num_platforms;
	cl_int err = clGetPlatformIDs(0, NULL, &num_platforms); // get OpenCL platform count
	if (err != CL_SUCCESS)
	{
		printf("Error %i in clGetPlatformIDs Call!!!\n\n", err);
		return false;
	}

	if (!num_platforms)
	{
		printf("Error: No OpenCL platform found!\n\n");
		return false;
	}

	cl_platform_id platform;
	{
		std::vector<cl_platform_id> platforms(num_platforms);
		err = clGetPlatformIDs(num_platforms, platforms.data(), NULL);
		printf("Available OpenCL platforms:\n");
		for (cl_uint pi = 0; pi < num_platforms; pi++)
		{
			char platform_name[1024] = {0};
			err = clGetPlatformInfo(platforms[pi], CL_PLATFORM_NAME, 1024, &platform_name, NULL);
			if (err != CL_SUCCESS) continue;

			printf("%d: '%s'\n", pi, platform_name);

			cl_uint num_devices;
			err = clGetDeviceIDs(platforms[pi], CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices);
			if (err != CL_SUCCESS) continue;

			std::vector<cl_device_id> devices(num_devices);
			err = clGetDeviceIDs(platforms[pi], CL_DEVICE_TYPE_ALL, num_devices, devices.data(), NULL);
			if (err != CL_SUCCESS) continue;

			printf("Available OpenCL devices:\n");
			for (cl_uint di = 0; di < num_devices; di++)
			{
				cl_device_type device_type;
				size_t returned_size;
				cl_char vendor_name[1024] = {0};
				cl_char device_name[1024] = {0};
				err = clGetDeviceInfo(devices[di], CL_DEVICE_TYPE, sizeof(cl_device_type), &device_type, NULL);
				err |= clGetDeviceInfo(devices[di], CL_DEVICE_VENDOR, sizeof(vendor_name), vendor_name, &returned_size);
				err |= clGetDeviceInfo(devices[di], CL_DEVICE_NAME, sizeof(device_name), device_name, &returned_size);
				if (err != CL_SUCCESS) continue;
				printf("%d: [%d] '%s' '%s'...\n", di, (int)device_type, vendor_name, device_name);
			}
		}

		platform = platforms[0];
	}

	// Locate a compute device
	err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_DEFAULT, 1, &m_deviceId, NULL);
	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to locate compute device!\n");
		return false;
	}

	// Create a context containing the compute device(s)
#ifdef __APPLE__
	m_context = clCreateContext(0, 1, &m_deviceId, clLogMessagesToStdoutAPPLE, NULL, &err);
#else
	m_context = clCreateContext(0, 1, &m_deviceId, clLogMessagesToStdout, NULL, &err);
#endif
	if (!m_context)
	{
		printf("Error: Failed to create a compute context!\n");
		return false;
	}

	cl_device_id device_ids[16];
	size_t returned_size;

	err = clGetContextInfo(m_context, CL_CONTEXT_DEVICES, sizeof(device_ids), device_ids, &returned_size);
	if (err)
	{
		printf("Error: Failed to retrieve compute devices for context!\n");
		return false;
	}

	size_t device_count = returned_size / sizeof(cl_device_id);

	bool bDeviceFound = false;
	for (size_t i = 0; i < device_count; i++)
	{
		cl_device_type device_type;
		cl_char vendor_name[1024] = {0};
		cl_char device_name[1024] = {0};
		clGetDeviceInfo(device_ids[i], CL_DEVICE_TYPE, sizeof(cl_device_type), &device_type, NULL);
		clGetDeviceInfo(device_ids[i], CL_DEVICE_VENDOR, sizeof(vendor_name), vendor_name, &returned_size);
		clGetDeviceInfo(device_ids[i], CL_DEVICE_NAME, sizeof(device_name), device_name, &returned_size);
		printf("%d: [%d] '%s' '%s'...\n", (int)i, (int)device_type, vendor_name, device_name);
//		if (device_type == m_deviceType)
		{
			m_deviceId = device_ids[i];
			bDeviceFound = true;
		}
	}

	if (!bDeviceFound)
	{
		printf("Error: Failed to locate compute device!\n");
		return false;
	}

	cl_ulong memSize = 0;
	clGetDeviceInfo(m_deviceId, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), &memSize, NULL);
	printf("OpenCL global memory size: %lld\n", memSize);

	// Create a command queue
	m_commands = clCreateCommandQueue(m_context, m_deviceId, 0, &err);
	if (!m_commands)
	{
		printf("Error: Failed to create a command queue!\n");
		return false;
	}

	// Report the device vendor and device name
	cl_char vendor_name[1024] = {0};
	cl_char device_name[1024] = {0};
	err = clGetDeviceInfo(m_deviceId, CL_DEVICE_VENDOR, sizeof(vendor_name), vendor_name, &returned_size);
	err|= clGetDeviceInfo(m_deviceId, CL_DEVICE_NAME, sizeof(device_name), device_name, &returned_size);
	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to retrieve device info!\n");
		return false;
	}

	printf("--------------------------------\n");
	printf("Connecting to %s %s...\n", vendor_name, device_name);
	return true;
}

static bool ReadFile(std::vector<char> & data, const char * strFilename)
{
	FILE * f = fopen(strFilename, "rb");
	if (!f)
		return false;

	fseek(f, 0, SEEK_END);
	size_t fileSize = ftell(f);
	fseek(f, 0, SEEK_SET);

	data.resize(fileSize + 1);
	size_t readSize = fread(data.data(), 1, fileSize, f);

	fclose(f);

	data[readSize] = '\0';
	return (readSize == fileSize);
}

bool OpenCLRenderer::SetupComputeKernel(const char * pKernelFilename, const char * pKernelMethodName)
{
	if (m_kernel)
	{
		clReleaseKernel(m_kernel);
		m_kernel = NULL;
	}

	if (m_program)
	{
		clReleaseProgram(m_program);
		m_program = NULL;
	}

	printf("--------------------------------\n");
	printf("Loading kernel source from file '%s'...\n", pKernelFilename);
	std::vector<char> source;
	if (!ReadFile(source, pKernelFilename))
	{
		printf("Error: Failed to load kernel source!\n");
		return false;
	}

	// Create the compute program from the source buffer
	cl_int err = 0;
	const char * data = source.data();
	m_program = clCreateProgramWithSource(m_context, 1, (const char **)&data, NULL, &err);
	if (!m_program || err != CL_SUCCESS)
	{
		printf("Error: Failed to create compute program!\n");
		return false;
	}

	// Build the program executable
	err = clBuildProgram(m_program, 0, NULL, NULL, NULL, NULL);
	if (err != CL_SUCCESS)
	{
		size_t len;
		char buffer[4096];
		printf("Error: Failed to build program executable!\n");
		clGetProgramBuildInfo(m_program, m_deviceId, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
		printf("%s\n", buffer);
		return false;
	}

	// Create the compute kernel from within the program
	printf("Creating kernel '%s'...\n", pKernelMethodName);
	m_kernel = clCreateKernel(m_program, pKernelMethodName, &err);
	if (!m_kernel || err != CL_SUCCESS)
	{
		printf("Error: Failed to create compute kernel!\n");
		return false;
	}

	// Get the maximum work group size for executing the kernel on the device
	size_t maxWorkGroupSize;
	err = clGetKernelWorkGroupInfo(m_kernel, m_deviceId, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &maxWorkGroupSize, NULL);
	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to retrieve kernel work group info! %d\n", err);
		return false;
	}

	size_t returned_size;
	size_t maxWorkItemSize[3];
	err = clGetDeviceInfo(m_deviceId, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(maxWorkItemSize), maxWorkItemSize, &returned_size);
	if (err != CL_SUCCESS)
	{
		printf("Error: Failed to retrieve device max work item sizes! %d\n", err);
		return false;
	}

	m_localWorkSize[0] = std::max((size_t)(sqrtf((float)maxWorkGroupSize)), maxWorkGroupSize / maxWorkItemSize[1]);
	m_localWorkSize[1] = maxWorkGroupSize / m_localWorkSize[0];

	m_localWorkSize[0] = std::min(m_localWorkSize[0], maxWorkItemSize[0]);
	m_localWorkSize[1] = std::min(m_localWorkSize[1], maxWorkItemSize[1]);

	m_localWorkSize[0] = std::min<size_t>(m_localWorkSize[0], 16);
	m_localWorkSize[1] = std::min<size_t>(m_localWorkSize[1], 16);

	printf("--------------------------------\n");

	return true;
}

#pragma pack(push, 16)
struct KernelTriangle
{
	cl_float3	pos[3];
	cl_float3	normal[3];
	cl_float2	tc[3];
	cl_uint		material;
	cl_uint		tmp; // #pragma pack(push, 16) doesn't work in Visual Studio
};

struct KernelNode
{
	cl_float3	center;
	cl_float3	extents;
	cl_float4	plane;
	cl_uint		childs[2];
	cl_uint		beginTriangle;
	cl_uint		endTriangle;
};
#pragma pack(pop)

inline void copy_float3(cl_float3 & dest, const Vec3 & src)
{
	dest.s[0] = src.x;
	dest.s[1] = src.y;
	dest.s[2] = src.z;
	dest.s[3] = 1.0;
}

inline void copy_float2(cl_float2 & dest, const Vec2 & src)
{
	dest.s[0] = src.x;
	dest.s[1] = src.y;
}

bool OpenCLRenderer::SetupSceneBuffers()
{
	SAFE_RELEASE_MEM_OBJECT(m_nodes);
	SAFE_RELEASE_MEM_OBJECT(m_nodeTriangles);
	SAFE_RELEASE_MEM_OBJECT(m_lights);
	SAFE_RELEASE_MEM_OBJECT(m_textures);
	SAFE_RELEASE_MEM_OBJECT(m_materials);
	SAFE_RELEASE_MEM_OBJECT(m_triangles);

	std::vector<const void *>	materials;

	{// fill triangles buffer
		enum { MAX_WRITE_TRIANGLES = 0x20000 }; // 16MB
		const size_t sizeOfTriangle = sizeof(KernelTriangle);
		const size_t numTriangles = m_scene.Triangles().size();

		printf("Creating OpenCL %d triangles...\n", (int)numTriangles);
		m_triangles = clCreateBuffer(m_context, CL_MEM_READ_ONLY, numTriangles * sizeOfTriangle, NULL, NULL);
		if (!m_triangles)
		{
			printf("Failed to create OpenCL triangles array!\n");
			return false;
		}

		std::vector<KernelTriangle>		triangles(std::min<size_t>(numTriangles, MAX_WRITE_TRIANGLES));
		std::map<const void *, uint32>	mapMaterials;
		size_t offset = 0, count = 0;

		for (size_t i = 0; i < numTriangles; i++)
		{
			const CollisionTriangle & tri = m_scene.Triangles()[i];
			KernelTriangle & kernelTri = triangles[count++];
			copy_float3(kernelTri.pos[0], tri.Vertex(0).pos);
			copy_float3(kernelTri.pos[1], tri.Vertex(1).pos);
			copy_float3(kernelTri.pos[2], tri.Vertex(2).pos);
			copy_float3(kernelTri.normal[0], tri.Vertex(0).normal);
			copy_float3(kernelTri.normal[1], tri.Vertex(1).normal);
			copy_float3(kernelTri.normal[2], tri.Vertex(2).normal);
			copy_float2(kernelTri.tc[0], tri.Vertex(0).tc);
			copy_float2(kernelTri.tc[1], tri.Vertex(1).tc);
			copy_float2(kernelTri.tc[2], tri.Vertex(2).tc);

			std::map<const void *, uint32>::const_iterator itMaterial = mapMaterials.find(tri.UserData());
			if (itMaterial == mapMaterials.end())
			{
				kernelTri.material = static_cast<cl_uint>(materials.size());
				mapMaterials[tri.UserData()] = kernelTri.material;
				materials.push_back(tri.UserData());
			}
			else
				kernelTri.material = itMaterial->second;

			if (count == MAX_WRITE_TRIANGLES || (offset + count) == numTriangles)
			{
				cl_int err = clEnqueueWriteBuffer(m_commands, m_triangles, CL_TRUE, offset * sizeOfTriangle, count * sizeOfTriangle, triangles.data(), 0, NULL, NULL);
				if (err != CL_SUCCESS)
				{
					printf("Failed to write buffer! %d\n", err);
					return false;
				}

				offset += count;
				count = 0;
			}
		}
	}

	{// fill nodes buffer
		enum { MAX_WRITE_NODES = 0x20000 };
		const size_t sizeOfNode = sizeof(KernelNode);
		const size_t numNodes = m_scene.Root()->GetNodeCount();

		printf("Creating OpenCL %d nodes...\n", (int)numNodes);
		m_nodes = clCreateBuffer(m_context, CL_MEM_READ_ONLY, numNodes * sizeOfNode, NULL, NULL);
		if (!m_nodes)
		{
			printf("Failed to create OpenCL nodes array!\n");
			return false;
		}

		std::vector<cl_uint>		nodeTriangles;
		std::vector<KernelNode>		nodes(std::min<size_t>(numNodes, MAX_WRITE_NODES));
		size_t offset = 0, count = 0;

		const BVHNode * pStackNodes[64]; // BVH tree depth must be less than 64
		const BVHNode ** ppTopNode = pStackNodes;
		*ppTopNode++ = m_scene.Root();

		while (ppTopNode > pStackNodes)
		{
			const BVHNode * pNode = *(--ppTopNode);
			KernelNode & kernelNode = nodes[count++];
			kernelNode.plane.s[0] = pNode->Axis() == 0 ? 1.f : 0.f;
			kernelNode.plane.s[1] = pNode->Axis() == 1 ? 1.f : 0.f;
			kernelNode.plane.s[2] = pNode->Axis() == 2 ? 1.f : 0.f;
			kernelNode.plane.s[3] = pNode->Dist();
			copy_float3(kernelNode.center, pNode->BoundingBox().Center());
			copy_float3(kernelNode.extents, pNode->BoundingBox().Size() * 0.5f);

			if (pNode->Triangles().empty())
			{
				assert(pNode->Child(0) && pNode->Child(1));
				*ppTopNode++ = pNode->Child(1);
				*ppTopNode++ = pNode->Child(0);
				kernelNode.childs[0] = (cl_uint)(offset + count);
				kernelNode.childs[1] = (cl_uint)(offset + count + pNode->Child(0)->GetNodeCount());
				kernelNode.beginTriangle = kernelNode.endTriangle = 0;
			}
			else
			{
				kernelNode.childs[0] = 0;
				kernelNode.childs[1] = 0;

				kernelNode.beginTriangle = (cl_uint)nodeTriangles.size();
				for (std::vector<CollisionTriangle *>::const_iterator it = pNode->Triangles().begin(), itEnd = pNode->Triangles().end(); it != itEnd; ++it)
					nodeTriangles.push_back(static_cast<cl_uint>((*it) - m_scene.Triangles().data()));

				kernelNode.endTriangle = (cl_uint)nodeTriangles.size();
			}

			if (count == MAX_WRITE_NODES || (offset + count) == numNodes)
			{
				cl_int err = clEnqueueWriteBuffer(m_commands, m_nodes, CL_TRUE, offset * sizeOfNode, count * sizeOfNode, nodes.data(), 0, NULL, NULL);
				if (err != CL_SUCCESS)
				{
					printf("Failed to write buffer! %d\n", err);
					return false;
				}

				offset += count;
				count = 0;
			}
		}

		printf("Creating OpenCL %ld node triangles...\n", nodeTriangles.size());
		m_nodeTriangles = clCreateBuffer(m_context, CL_MEM_READ_ONLY, nodeTriangles.size() * sizeof(cl_uint), NULL, NULL);
		if (!m_nodeTriangles)
		{
			printf("Failed to create OpenCL node triangles array!\n");
			return false;
		}

		cl_int err = clEnqueueWriteBuffer(m_commands, m_nodeTriangles, CL_TRUE, 0, nodeTriangles.size() * sizeof(cl_uint), nodeTriangles.data(), 0, NULL, NULL);
		if (err != CL_SUCCESS)
		{
			printf("Failed to write buffer! %d\n", err);
			return false;
		}
	}

	printf("Creating OpenCL %d materials...\n", (int)materials.size());

	return true;
}

void OpenCLRenderer::UpdateResultBuffer(cl_uint width, cl_uint height)
{
	if (m_size[0] == width && m_size[1] == height)
		return;

	SAFE_RELEASE_MEM_OBJECT(m_result);

	// create result buffer
	m_size[0] = width;
	m_size[1] = height;
	m_size[2] = (cl_uint)m_scene.Triangles().size();
	m_size[3] = 6; // trace depth
	m_result = clCreateBuffer(m_context, CL_MEM_READ_WRITE, 4 * 4 * width * height, NULL, NULL);
	std::vector<float> data(m_size[0] * m_size[1] * 4);
	std::fill(data.begin(), data.end(), 0.f);
	//cl_int err = clEnqueueWriteBuffer(m_commands, m_result, CL_TRUE, 0, m_size[0] * m_size[1] * 4 * 4, data.data(), 0, NULL, NULL);
	//err = clEnqueueReadBuffer(m_commands, m_result, CL_TRUE, 0, m_size[0] * m_size[1] * 4 * 4, data.data(), 0, NULL, NULL);

	if (!m_result)
		printf("Failed to create OpenCL array!\n");
}

void OpenCLRenderer::Render(Image & image, const RectI * pViewportRect,
							const Matrix & matCamera, const Matrix & matViewProj,
							const ColorF & bgColor, const Image * pEnvironmentMap,
							int numThreads, int nFrameNumber)
{
	if (!m_bInitialized)
		return;

	UpdateResultBuffer(image.Width(), image.Height());

	if (!m_result)
		return;

	RectI rcRenderArea = pViewportRect ? *pViewportRect : RectI(0, 0, image.Width(), image.Height());
	m_fFrameBlend = 1.f / (nFrameNumber + 1);

	//float fDistEpsilon = m_scene.Root()->BoundingBox().Size().Length() * 0.0001f;
	m_dp.s[0] = 2.f / rcRenderArea.Width();
	m_dp.s[1] = 2.f / rcRenderArea.Height();

	Matrix matViewProjInv;
	matViewProjInv.Inverse(matViewProj);
	Vec4 p = Vec4(0.f, 0.f, 1.f, 1.f);
	p.Transform(matViewProjInv);
	Vec3 vCamDeltaZ = Vec3(p.x, p.y, p.z) / p.w;
	p = Vec4(1.f, 0.f, 1.f, 1.f);
	p.Transform(matViewProjInv);
	Vec3 vCamDeltaX = Vec3(p.x, p.y, p.z) / p.w - vCamDeltaZ;
	p = Vec4(0.f, 1.f, 1.f, 1.f);
	p.Transform(matViewProjInv);
	Vec3 vCamDeltaY = Vec3(p.x, p.y, p.z) / p.w - vCamDeltaZ;
	vCamDeltaZ -= matCamera.Pos();

	Vec2 vPixelOffset = nFrameNumber > 0 ? Vec2(frand(), frand()) : Vec2(0.5f, 0.5f);
	vCamDeltaZ += vCamDeltaX * (vPixelOffset.y * m_dp.s[0]);
	vCamDeltaZ += vCamDeltaY * -(vPixelOffset.y * m_dp.s[1]);

	copy_float3(m_camPosition, matCamera.Pos());
	copy_float3(m_camDirectionX, vCamDeltaX);
	copy_float3(m_camDirectionY, vCamDeltaY);
	copy_float3(m_camDirectionZ, vCamDeltaZ);

	cl_int err = 0;
	cl_uint numArgs = 0;
	err |= clSetKernelArg(m_kernel, numArgs++, sizeof(cl_mem), &m_result);
	err |= clSetKernelArg(m_kernel, numArgs++, sizeof(cl_mem), &m_triangles);
	err |= clSetKernelArg(m_kernel, numArgs++, sizeof(cl_mem), &m_nodes);
	err |= clSetKernelArg(m_kernel, numArgs++, sizeof(cl_mem), &m_nodeTriangles);
	err |= clSetKernelArg(m_kernel, numArgs++, sizeof(m_size), &m_size);
	err |= clSetKernelArg(m_kernel, numArgs++, sizeof(cl_float3), &m_camPosition);
	err |= clSetKernelArg(m_kernel, numArgs++, sizeof(cl_float3), &m_camDirectionX);
	err |= clSetKernelArg(m_kernel, numArgs++, sizeof(cl_float3), &m_camDirectionY);
	err |= clSetKernelArg(m_kernel, numArgs++, sizeof(cl_float3), &m_camDirectionZ);
	err |= clSetKernelArg(m_kernel, numArgs++, sizeof(cl_float2), &m_dp);
	err |= clSetKernelArg(m_kernel, numArgs++, sizeof(cl_float), &m_fFrameBlend);
	if (err != CL_SUCCESS)
		return;

	size_t globalWorkSize[2];
	globalWorkSize[0] = ((m_size[0] + m_localWorkSize[0] - 1) / m_localWorkSize[0]) * m_localWorkSize[0];
	globalWorkSize[1] = ((m_size[1] + m_localWorkSize[1] - 1) / m_localWorkSize[1]) * m_localWorkSize[1];

	err = clEnqueueNDRangeKernel(m_commands, m_kernel, 2, NULL, globalWorkSize, m_localWorkSize, 0, NULL, NULL);
	if (err)
	{
		printf("Failed to enqueue kernel! %d\n", err);
		return;
	}

	err = clEnqueueReadBuffer(m_commands, m_result, CL_TRUE, 0, m_size[0] * m_size[1] * 4 * 4, image.Data(), 0, NULL, NULL);
	if (err != CL_SUCCESS)
	{
		printf("Failed to read buffer! %d\n", err);
		return;
	}
}
