//
//  SceneView.cpp
//  MiRay/ui
//
//  Created by Damir Sagidullin on 01.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#ifndef _WIN32
#include <unistd.h>
#endif

#include "SceneView.h"
#include "RenderModel.h"
#include "RenderThread.h"
#include "../rt/SoftwareRenderer.h"
#include "../rt/OpenCLRenderer.h"

using namespace mr;

SceneView::SceneView(const char * pResourcesPath)
	: m_resourcesPath(pResourcesPath)
	, m_fWidth(0.f)
	, m_fHeight(0.f)
	, m_fRWidth(0.f)
	, m_fRHeight(0.f)
	, m_fFOV(60.f)
	, m_bGesture(false)
	, m_vGestureTarget(Vec3::Null)
	, m_vGestureTargetNormal(Vec3::Null)
	, m_matCamera(Matrix::Identity)
	, m_matView(Matrix::Identity)
	, m_matViewProj(Matrix::Identity)
	, m_bgColor(1.f, 1.f, 1.f, 1.f)
	, m_bShowGrid(true)
	, m_bShowWireframe(false)
	, m_bShowNormals(false)
	, m_bShowBVH(false)
	, m_width(0), m_height(0)
	, m_texture(0)
	, m_nFrameCount(0)
	, m_renderMode(RM_SOFTWARE)
	, m_pImageManager(NULL)
	, m_pModelManager(NULL)
	, m_pModel(NULL)
	, m_pRenderModel(NULL)
	, m_pEnvironmentMap(NULL)
	, m_pCS(NULL)
	, m_rcRenderMap(0, 0, 0, 0)
	, m_pRenderMap(NULL)
	, m_pBuffer(NULL)
	, m_pRenderThread(new RenderThread())
{
	m_vCollisionTriangle[0] = m_vCollisionTriangle[1] = m_vCollisionTriangle[2] = Vec3::Null;
	m_pImageManager = new ImageManager();
	m_pModelManager = new ModelManager(m_pImageManager);
	ResetCamera();
}

SceneView::~SceneView()
{
	SAFE_DELETE(m_pRenderThread);
	delete m_pCS;
	SAFE_RELEASE(m_pBuffer);
	SAFE_RELEASE(m_pRenderMap);
	SAFE_DELETE(m_pRenderModel);
	SAFE_RELEASE(m_pEnvironmentMap);
	SAFE_RELEASE(m_pModel);
	delete m_pModelManager;
	delete m_pImageManager;
}

bool SceneView::Init()
{
	return true;
}

void SceneView::Done()
{
}

void SceneView::ResetCamera(int i)
{
	m_nFrameCount = 0;
	if (m_pModel)
		CalculateCameraMatrix(m_matCamera, m_pModel->BoundingBox().Center(), m_pModel->BoundingBox().Size().Length() / powf(2.f, (float)i), -90.f, 10.f);
	else
		CalculateCameraMatrix(m_matCamera, Vec3::Null, 30.f, -90.f, 10.f);

	UpdateMatrices();
}

void SceneView::SetRenderMode(eRenderMode rm)
{
	m_renderMode = rm;
	StopRenderThread();
	ResumeRenderThread();
}

bool SceneView::LoadScene(const char * pFilename)
{
	StopRenderThread();

	printf("Loading scene '%s'...\n", pFilename);
	SAFE_RELEASE(m_pModel);
	
	double tm1 = Timer::GetSeconds();

	m_pModel = m_pModelManager->LoadModel(pFilename);
	if (!m_pModel)
		return false;

	double tm2 = Timer::GetSeconds();

	printf("Scene loading time: %f ms\n", (tm2 - tm1) * 1000.0);
	Vec3 vCenter = m_pModel->BoundingBox().Center();
	Vec3 vSize = m_pModel->BoundingBox().Size();
	printf("center(%g %g %g) size(%g %g %g)\n", vCenter.x, vCenter.y, vCenter.z, vSize.x, vSize.y, vSize.z);

	SAFE_DELETE(m_pRenderModel);
	m_pRenderModel = new RenderModel(*m_pModel);
	
	CreateBVH();

	ResetCamera();

	return true;
}

bool SceneView::SetEnvironmentImage(const char * pFilename)
{
	StopRenderThread();
	SAFE_RELEASE(m_pEnvironmentMap);
	m_pEnvironmentMap = m_pImageManager->Load(pFilename);
	ResumeRenderThread();
	return m_pEnvironmentMap != NULL;
}

bool SceneView::SaveImage(const char * pFilename) const
{
	return m_pRenderMap ? m_pImageManager->Save(pFilename, *m_pRenderMap) : false;
}

void SceneView::CreateBVH()
{
	if (m_pCS)
	{
		delete m_pCS;
		m_pCS = NULL;
	}

	double tm1 = Timer::GetSeconds();

	size_t numTriangles = 0;
	for (Model::MeshArray::const_iterator itMesh = m_pModel->Meshes().begin(), itMeshEnd = m_pModel->Meshes().end(); itMesh != itMeshEnd; ++itMesh)
	{
		for (Model::GeometryArray::const_iterator itGeom = (*itMesh)->m_geometries.begin(), itGeomEnd = (*itMesh)->m_geometries.end(); itGeom != itGeomEnd; ++itGeom)
			numTriangles += (*itGeom)->m_indices.size() / 3;
	}

	m_pCS = new BVH(numTriangles);

	for (Model::MeshArray::const_iterator itMesh = m_pModel->Meshes().begin(), itMeshEnd = m_pModel->Meshes().end(); itMesh != itMeshEnd; ++itMesh)
	{
		for (Model::GeometryArray::const_iterator itGeom = (*itMesh)->m_geometries.begin(), itGeomEnd = (*itMesh)->m_geometries.end(); itGeom != itGeomEnd; ++itGeom)
		{
			Model::Geometry & geom = *(*itGeom);
			for (const uint32 *pInd = &geom.m_indices[0], *pIndEnd = pInd + geom.m_indices.size(); pInd < pIndEnd; pInd += 3)
			{
				CollisionTriangle t(geom.m_vertices[pInd[0]], geom.m_vertices[pInd[1]], geom.m_vertices[pInd[2]],
									geom.m_pMaterial);
				m_pCS->AddTriangle(t);
			}
		}
	}

	m_pCS->Build(30);
	double tm2 = Timer::GetSeconds();

	m_pRenderThread->SetOpenCLRenderer(new OpenCLRenderer(*m_pCS, (m_resourcesPath + "/kernel.cl").c_str(), "MainKernel"));
	m_pRenderThread->SetRenderer(new SoftwareRenderer(*m_pCS));

	printf("Collision scene creating time: %ld triangles, %d nodes, %d, %f ms\n", m_pCS->Triangles().size(),
		   static_cast<int>(m_pCS->Root()->GetNodeCount()),
		   static_cast<int>(m_pCS->Root()->GetChildrenDepth()),
		   (tm2 - tm1) * 1000.0);
}

Vec3 SceneView::GetTarget(float x, float y) const
{
	if (m_pCS)
	{
		Matrix matViewProjInv;
		matViewProjInv.Inverse(m_matViewProj);
		Vec4 v(x / m_fRWidth * 2.f - 1.f, -(y / m_fRHeight * 2.f - 1.f), 1.f, 1.f);
		v.Transform(matViewProjInv);
		TraceResult tr;
		if (m_pCS->TraceRay(m_matCamera.Pos(), Vec3(v.x, v.y, v.z) / v.w, tr))
			return tr.pos;
	}

	if (m_pModel)
		return m_pModel->BoundingBox().Center();

	return Vec3::Null;
}

void SceneView::BeginGesture(float x, float y)
{
	m_bGesture = true;
	m_vGestureTarget = GetTarget(x, y);
	m_vGestureTargetNormal = Vec3::Null;
}

void SceneView::EndGesture()
{
	m_bGesture = false;
	m_vGestureTarget = Vec3::Null;
}

void SceneView::Rotate(float dx, float dy)
{
	m_nFrameCount = 0;
	Matrix matRotation;
	matRotation.RotationAxis(Vec3::Z, DEG2RAD(-dx * 0.5f), m_vGestureTarget);
	m_matCamera = m_matCamera * matRotation;
	float pitch = RAD2DEG(-m_matCamera.AxisZ().Pitch());
	float dPitch = clamp(pitch - dy * 0.5f, -89.99f, 89.99f) - pitch;
	matRotation.RotationAxis(m_matCamera.AxisX(), DEG2RAD(dPitch), m_vGestureTarget);
	m_matCamera = m_matCamera * matRotation;

	// normalize camera matrix
	CalculateCameraMatrix(m_matCamera, m_matCamera.Pos(), 0.f,
						  RAD2DEG(m_matCamera.AxisZ().Yaw()), RAD2DEG(m_matCamera.AxisZ().Pitch()));

	UpdateMatrices();
}

void SceneView::Pan(float dx, float dy)
{
	m_nFrameCount = 0;
	float fDistance = Vec3::Dot(m_matCamera.AxisZ(), (m_matCamera.Pos() - m_vGestureTarget));
	m_matCamera.Pos() -= m_matCamera.Axis(0) * ((dx * 2.f / m_fRWidth) * fDistance / m_matProj.m11);
	m_matCamera.Pos() += m_matCamera.Axis(1) * ((dy * 2.f / m_fRHeight) * fDistance / m_matProj.m22);

	UpdateMatrices();
}

void SceneView::Zoom(float x, float y, float d)
{
	m_nFrameCount = 0;
	float fMul = powf(1.03f, fabsf(d));
	if (d < 0.f) fMul = 1.f / fMul;

	Vec3 vTarget = GetTarget(x, y);
	Vec3 vDir = m_matCamera.Pos() - vTarget;
	float fDist = vDir.Normalize();
	fDist = clamp(fDist * fMul, 0.3f, 900.f);
	m_matCamera.Pos() = vTarget + vDir * fDist;

	UpdateMatrices();
}

void SceneView::OtherMouseDown(float x, float y)
{
	Matrix matViewProjInv;
	matViewProjInv.Inverse(m_matViewProj);
	Vec4 v(x / m_fRWidth * 2.f - 1.f, -(y / m_fRHeight * 2.f - 1.f), 1.f, 1.f);
	v.Transform(matViewProjInv);

	TraceResult tr;
	if (m_pCS->TraceRay(m_matCamera.Pos(), Vec3(v.x, v.y, v.z) / v.w, tr))
	{
		m_vGestureTarget = tr.pos;
		m_vGestureTargetNormal = tr.pTriangle->GetNormal(tr.pc);
		m_vCollisionTriangle[0] = tr.pTriangle->Vertex(0).pos;
		m_vCollisionTriangle[1] = tr.pTriangle->Vertex(1).pos;
		m_vCollisionTriangle[2] = tr.pTriangle->Vertex(2).pos;
	}
}

//void SceneView::RenderScene(int width, int height, int samples)
//{
//	if (!m_pCS)
//		return;
//
//	Image * pImage = m_pImageManager->Create(width, height, Image::TYPE_4B);
//	
//#ifdef _WIN32
//	SYSTEM_INFO sysinfo;
//	::GetSystemInfo(&sysinfo);
//	int numCPU = static_cast<int>(sysinfo.dwNumberOfProcessors);
//#else
//	int numCPU = static_cast<int>(::sysconf(_SC_NPROCESSORS_ONLN));
//#endif
//
//	double tm1 = Timer::GetSeconds();
//
//	SoftwareRenderer renderer(*m_pCS);
//	renderer.Render(*pImage, NULL, m_matCamera, m_matViewProj, m_bgColor, m_pEnvironmentMap, numCPU, 0);
//	renderer.Join();
//
//	double tm2 = Timer::GetSeconds();
//
//	m_pImageManager->Save("/Users/damir/Documents/test.png", *pImage, ImageManager::FILE_FORMAT_PNG);
//
//	SAFE_RELEASE(pImage);
//
//	printf("Render scene time: %dx%d (%d threads) %f ms\n", width, height, numCPU, (tm2 - tm1) * 1000.0);
//}

void SceneView::Resize(float w, float h, float rw, float rh)
{
	m_fWidth = w;
	m_fHeight = h;
	m_fRWidth = rw;
	m_fRHeight = rh;

	StopRenderThread();

	int width = (int)m_fRWidth;
	int height = (int)m_fRHeight;

	SAFE_RELEASE(m_pRenderMap);
	m_pRenderMap = m_pImageManager->Create(width, height, Image::TYPE_4F);
	
	SAFE_RELEASE(m_pBuffer);
	m_pBuffer = m_pImageManager->Create(width, height, Image::TYPE_4F);
	if (m_pBuffer)
		std::fill(m_pBuffer->DataF(), m_pBuffer->DataF() + m_pBuffer->Width() * m_pBuffer->Height() * m_pBuffer->NumChannels(), 0.f);

	UpdateMatrices();
}

void SceneView::DrawGrid()
{
	glDisable(GL_BLEND);
	glDisable(GL_LINE_SMOOTH);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_LINE_SMOOTH);
	//glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glLineWidth(1.f);
	
//	glDepthMask(GL_FALSE);
	
	glBegin(GL_LINES);
	glColor4f(0.75f, 0.75f, 0.75f, 1.f);
	for (int x = -9; x < 10; x++)
	{
		if (x == 0) continue;
		glVertex3fv(Vec3((float)x, -10.f, 0.f));
		glVertex3fv(Vec3((float)x, 10.f, 0.f));
		glVertex3fv(Vec3(-10.f, (float)x, 0.f));
		glVertex3fv(Vec3(10.f, (float)x, 0.f));
	}
	glColor4f(0.5f, 0.5f, 0.5f, 1.f);
	for (int x = -9; x < 10; x++)
	{
		glVertex3fv(Vec3(x * 10.f, -100.f, 0.f));
		glVertex3fv(Vec3(x * 10.f, 100.f, 0.f));
		glVertex3fv(Vec3(-100.f, x * 10.f, 0.f));
		glVertex3fv(Vec3(100.f, x * 10.f, 0.f));
	}
	
	glVertex3fv(Vec3(100.f, 100.f, 0.f));
	glVertex3fv(Vec3(100.f, -100.f, 0.f));
	
	glVertex3fv(Vec3(100.f, 100.f, 0.f));
	glVertex3fv(Vec3(-100.f, 100.f, 0.f));
	
	glVertex3fv(Vec3(-100.f, -100.f, 0.f));
	glVertex3fv(Vec3(100.f, -100.f, 0.f));
	
	glVertex3fv(Vec3(-100.f, -100.f, 0.f));
	glVertex3fv(Vec3(-100.f, 100.f, 0.f));
	
	glColor3f(1.f, 0.f, 0.f);
	glVertex3fv(Vec3::Null);
	glVertex3fv(Vec3(100.f, 0.f, 0.f));

	glColor3f(0.f, 1.f, 0.f);
	glVertex3fv(Vec3::Null);
	glVertex3fv(Vec3(0.f, 100.f, 0.f));

	glColor3f(0.f, 0.f, 1.f);
	glVertex3fv(Vec3::Null);
	glVertex3fv(Vec3(0.f, 0.f, 100.f));

	glEnd();
	
//	glDepthMask(GL_TRUE);
	//glDisable(GL_LINE_SMOOTH);
}

void SceneView::DrawWireframeBox(const BBox & box, const Color & c) const
{
	glDisable(GL_BLEND);
	glDisable(GL_LINE_SMOOTH);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_LINE_SMOOTH);
	//glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glLineWidth(1.f);
		
	glBegin(GL_LINES);
	
	glColor4ubv(c);

	glVertex3fv(Vec3(box.vMins.x, box.vMins.y, box.vMins.z));
	glVertex3fv(Vec3(box.vMaxs.x, box.vMins.y, box.vMins.z));
	
	glVertex3fv(Vec3(box.vMins.x, box.vMaxs.y, box.vMins.z));
	glVertex3fv(Vec3(box.vMaxs.x, box.vMaxs.y, box.vMins.z));
	
	glVertex3fv(Vec3(box.vMins.x, box.vMins.y, box.vMaxs.z));
	glVertex3fv(Vec3(box.vMaxs.x, box.vMins.y, box.vMaxs.z));
	
	glVertex3fv(Vec3(box.vMins.x, box.vMaxs.y, box.vMaxs.z));
	glVertex3fv(Vec3(box.vMaxs.x, box.vMaxs.y, box.vMaxs.z));
	
	glVertex3fv(Vec3(box.vMins.x, box.vMins.y, box.vMins.z));
	glVertex3fv(Vec3(box.vMins.x, box.vMaxs.y, box.vMins.z));

	glVertex3fv(Vec3(box.vMaxs.x, box.vMins.y, box.vMins.z));
	glVertex3fv(Vec3(box.vMaxs.x, box.vMaxs.y, box.vMins.z));

	glVertex3fv(Vec3(box.vMins.x, box.vMins.y, box.vMaxs.z));
	glVertex3fv(Vec3(box.vMins.x, box.vMaxs.y, box.vMaxs.z));
	
	glVertex3fv(Vec3(box.vMaxs.x, box.vMins.y, box.vMaxs.z));
	glVertex3fv(Vec3(box.vMaxs.x, box.vMaxs.y, box.vMaxs.z));

	glVertex3fv(Vec3(box.vMins.x, box.vMins.y, box.vMins.z));
	glVertex3fv(Vec3(box.vMins.x, box.vMins.y, box.vMaxs.z));

	glVertex3fv(Vec3(box.vMaxs.x, box.vMins.y, box.vMins.z));
	glVertex3fv(Vec3(box.vMaxs.x, box.vMins.y, box.vMaxs.z));

	glVertex3fv(Vec3(box.vMins.x, box.vMaxs.y, box.vMins.z));
	glVertex3fv(Vec3(box.vMins.x, box.vMaxs.y, box.vMaxs.z));
	
	glVertex3fv(Vec3(box.vMaxs.x, box.vMaxs.y, box.vMins.z));
	glVertex3fv(Vec3(box.vMaxs.x, box.vMaxs.y, box.vMaxs.z));

	glEnd();
}

void SceneView::DrawBox(const BBox & bbox, const Color & c) const
{
	glBegin(GL_QUADS);
	glColor4ubv(VertexColor(c, Vec3::X));
	glVertex3fv(Vec3(bbox.vMins.x, bbox.vMaxs.y, bbox.vMaxs.z));
	glVertex3fv(Vec3(bbox.vMins.x, bbox.vMins.y, bbox.vMaxs.z));
	glVertex3fv(Vec3(bbox.vMins.x, bbox.vMins.y, bbox.vMins.z));
	glVertex3fv(Vec3(bbox.vMins.x, bbox.vMaxs.y, bbox.vMins.z));
	
	glColor4ubv(VertexColor(c, -Vec3::X));
	glVertex3fv(Vec3(bbox.vMaxs.x, bbox.vMins.y, bbox.vMaxs.z));
	glVertex3fv(Vec3(bbox.vMaxs.x, bbox.vMaxs.y, bbox.vMaxs.z));
	glVertex3fv(Vec3(bbox.vMaxs.x, bbox.vMaxs.y, bbox.vMins.z));
	glVertex3fv(Vec3(bbox.vMaxs.x, bbox.vMins.y, bbox.vMins.z));
	
	glColor4ubv(VertexColor(c, Vec3::Y));
	glVertex3fv(Vec3(bbox.vMins.x, bbox.vMins.y, bbox.vMaxs.z));
	glVertex3fv(Vec3(bbox.vMaxs.x, bbox.vMins.y, bbox.vMaxs.z));
	glVertex3fv(Vec3(bbox.vMaxs.x, bbox.vMins.y, bbox.vMins.z));
	glVertex3fv(Vec3(bbox.vMins.x, bbox.vMins.y, bbox.vMins.z));
	
	glColor4ubv(VertexColor(c, -Vec3::Y));
	glVertex3fv(Vec3(bbox.vMaxs.x, bbox.vMaxs.y, bbox.vMaxs.z));
	glVertex3fv(Vec3(bbox.vMins.x, bbox.vMaxs.y, bbox.vMaxs.z));
	glVertex3fv(Vec3(bbox.vMins.x, bbox.vMaxs.y, bbox.vMins.z));
	glVertex3fv(Vec3(bbox.vMaxs.x, bbox.vMaxs.y, bbox.vMins.z));
	
	glColor4ubv(VertexColor(c, Vec3::Z));
	glVertex3fv(Vec3(bbox.vMins.x, bbox.vMins.y, bbox.vMins.z));
	glVertex3fv(Vec3(bbox.vMaxs.x, bbox.vMins.y, bbox.vMins.z));
	glVertex3fv(Vec3(bbox.vMaxs.x, bbox.vMaxs.y, bbox.vMins.z));
	glVertex3fv(Vec3(bbox.vMins.x, bbox.vMaxs.y, bbox.vMins.z));
	
	glColor4ubv(VertexColor(c, -Vec3::Z));
	glVertex3fv(Vec3(bbox.vMaxs.x, bbox.vMins.y, bbox.vMaxs.z));
	glVertex3fv(Vec3(bbox.vMins.x, bbox.vMins.y, bbox.vMaxs.z));
	glVertex3fv(Vec3(bbox.vMins.x, bbox.vMaxs.y, bbox.vMaxs.z));
	glVertex3fv(Vec3(bbox.vMaxs.x, bbox.vMaxs.y, bbox.vMaxs.z));
	glEnd();
}

void SceneView::DrawBVHNode(const BVHNode * pCN, byte level) const
{
	if (!pCN)
		return;
	
	static Color colors[] = {Color(255, 0, 0), Color(0, 255, 0), Color(0, 0, 255), Color(255, 255, 0), Color(255, 0, 255), Color(0, 255, 255), Color(255, 128, 0)};
	DrawWireframeBox(pCN->BoundingBox(), colors[level % 7]);

//	const sTriangleCutArray & triangles = pCN->Triangles();
//	if (!triangles.empty())
//	{
//		Color c(255, 255, 255);
//		glBegin(GL_TRIANGLES);
//		for (sTriangleCutArray::const_iterator it = triangles.begin(), itEnd = triangles.end(); it != itEnd; ++it)
//		{
//			glColor3ubv(VertexColor(c, it->v[1]));
//			glVertex3fv(it->v[0]);
//			glColor3ubv(VertexColor(c, it->v[1]));
//			glVertex3fv(it->v[1]);
//			glColor3ubv(VertexColor(c, it->v[2]));
//			glVertex3fv(it->v[2]);
//		}
//		glEnd();
//		
//		glPolygonOffset(-4.f, -4.f);
//		glPolygonMode(GL_FRONT, GL_LINE);
//		glEnable(GL_POLYGON_OFFSET_LINE);
//		glDepthMask(GL_FALSE);
//		
//		glColor4ub(0, 128, 255, 255);
//		glBegin(GL_TRIANGLES);
//		for (sTriangleCutArray::const_iterator it = triangles.begin(), itEnd = triangles.end(); it != itEnd; ++it)
//		{
//			glVertex3fv(it->v[0]);
//			glVertex3fv(it->v[1]);
//			glVertex3fv(it->v[2]);
//		}
//		glEnd();
//		
//		glDepthMask(GL_TRUE);
//		glDisable(GL_POLYGON_OFFSET_LINE);
//		glPolygonMode(GL_FRONT, GL_FILL);
//	}

	DrawBVHNode(pCN->Child(0), level + 1);
	DrawBVHNode(pCN->Child(1), level + 1);
}

void SceneView::UpdateMatrices()
{
	CalculateProjectionMatrix(m_matProj, m_fFOV, m_fWidth / m_fHeight);
	m_matView.Inverse(m_matCamera);
	m_matViewProj = m_matView * m_matProj;

	StopRenderThread();
	ResumeRenderThread();
}

void SceneView::Draw()
{
//	if (m_pRenderMap && m_renderMode != RM_OPENGL)
//	{// render frame to render map
//#ifdef _WIN32
//		SYSTEM_INFO sysinfo;
//		::GetSystemInfo(&sysinfo);
//		int numCPU = static_cast<int>(sysinfo.dwNumberOfProcessors);
//#else
//		int numCPU = static_cast<int>(::sysconf(_SC_NPROCESSORS_ONLN));
//#endif
//
//		double tm1 = Timer::GetSeconds();
//
//		SoftwareRenderer renderer(*m_pCS);
//		int nScale = m_nFrameCount < 2 ? (2 << (1 - m_nFrameCount)) : 1;
//		m_rcRenderMap = RectI(0, 0, m_pRenderMap->Width() / nScale, m_pRenderMap->Height() / nScale);
//		renderer.Render(*m_pRenderMap, &m_rcRenderMap, m_matCamera, m_matViewProj,
//						m_bgColor, m_pEnvironmentMap, 1, numCPU, std::max(m_nFrameCount - 2, 0));
//		renderer.Join();
//
//		if (!m_bGesture)
//			m_nFrameCount++;
//
//		double tm2 = Timer::GetSeconds();
//
//		printf("Render scene: %d %dx%d (%d threads) %f ms\n", m_nFrameCount, m_width, m_height, numCPU, (tm2 - tm1) * 1000.0);
//	}

    glViewport(0, 0, (int)m_fWidth, (int)m_fHeight);
	glClearColor(m_bgColor.r, m_bgColor.g, m_bgColor.b, m_bgColor.a);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (m_renderMode == RM_OPENGL/* || m_renderMode == RM_MIXED*/)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(m_matProj);
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(m_matView);

	//	glEnable(GL_CULL_FACE);
	//	glFrontFace(GL_CW);
		glShadeModel(GL_SMOOTH);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		if (m_bShowGrid)
			DrawGrid();

		if (m_pRenderModel)
		{
			if (m_bShowWireframe)
				m_pRenderModel->DrawWireframe();
			else
				m_pRenderModel->Draw();

			if (m_bShowNormals)
				m_pRenderModel->DrawNormals(m_pModel->BoundingBox().Size().Length() * 0.001f);
		}

	//	for (Model::MeshArray::const_iterator itMesh = m_pModel->Meshes().begin(), itMeshEnd = m_pModel->Meshes().end(); itMesh != itMeshEnd; ++itMesh)
	//		DrawMesh(*(*itMesh));

		if (m_bShowBVH)
			DrawBVHNode(m_pCS->Root(), 0);
	}

	DrawRenderMap();

	if (m_pModel)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(m_matProj);
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(m_matView);

		if (m_bGesture)
		{
			Vec3 vBoxExt = Vec3( Vec3::Dot(m_vGestureTarget - m_matCamera.Pos(), m_matCamera.AxisZ()) * 0.005f );
			DrawBox(BBox(m_vGestureTarget - vBoxExt, m_vGestureTarget + vBoxExt), Color(0, 128, 255));
		}

		glLineWidth(1.f);
		glDepthMask(GL_FALSE);
		glBegin(GL_LINES);
		
		glColor3ub(255, 0, 0);
		glVertex3fv(m_vCollisionTriangle[0]);
		glVertex3fv(m_vCollisionTriangle[1]);
		glVertex3fv(m_vCollisionTriangle[1]);
		glVertex3fv(m_vCollisionTriangle[2]);
		glVertex3fv(m_vCollisionTriangle[2]);
		glVertex3fv(m_vCollisionTriangle[0]);
		
		glColor3ub(0, 255, 0);
		glVertex3fv(m_vGestureTarget);
		glVertex3fv(m_vGestureTarget + m_vGestureTargetNormal * (m_pModel->BoundingBox().Size().Length() * 0.01f));
		
		glEnd();
		glDepthMask(GL_TRUE);
//		glLineWidth(1.f);
	}
}

void SceneView::UpdateRenderMapTexture()
{
	if (!m_pRenderMap)
		return;

	if (!m_texture || m_width != m_pRenderMap->Width() || m_height != m_pRenderMap->Height())
	{
		m_width = m_pRenderMap->Width();
		m_height = m_pRenderMap->Height();
		
		if (m_texture)
		{
			glDeleteTextures(1, &m_texture);
			m_texture = 0;
		}

		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		assert(GL_NO_ERROR == glGetError());
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		assert(GL_NO_ERROR == glGetError());
	}

	if (m_texture)
	{
		m_rcRenderMap = m_pRenderThread->LockRenderMap();
		glBindTexture(GL_TEXTURE_2D, m_texture);
		assert(GL_NO_ERROR == glGetError());
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_pRenderMap->Width(), m_rcRenderMap.Height(),
						GL_RGBA, GL_FLOAT, m_pRenderMap->Data());
		assert(GL_NO_ERROR == glGetError());
		m_pRenderThread->UnlockRenderMap();
	}
}

void SceneView::DrawRenderMap()
{
	if (!m_pRenderMap || m_renderMode == RM_OPENGL)
		return;

	if (m_pRenderThread->IsRenderMapUpdated())
		UpdateRenderMapTexture();

	if (!m_texture)
		return;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, m_texture);

	float u = (float)m_rcRenderMap.Width() / (float)m_pRenderMap->Width();
	float v = (float)m_rcRenderMap.Height() / (float)m_pRenderMap->Height();
//	if (m_renderMode == RM_MIXED)
//	{
//		glEnable(GL_BLEND);
//		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
//		glColor4ub(255, 255, 255, 128);
//	}
//	else
	{
		glDisable(GL_BLEND);
		glColor4ub(255, 255, 255, 255);
	}

	glBegin(GL_QUADS);
	glTexCoord2f(0.f, v);
	glVertex2f(-1.f, -1.f);
	glTexCoord2f(u, v);
	glVertex2f(+1.f, -1.f);
	glTexCoord2f(u, 0.f);
	glVertex2f(+1.f, +1.f);
	glTexCoord2f(0.f, 0.f);
	glVertex2f(-1.f, +1.f);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}

void SceneView::StopRenderThread()
{
	m_pRenderThread->Stop();
}

void SceneView::ResumeRenderThread()
{
	if (m_renderMode != RM_OPENGL)
	{
		m_pRenderThread->Start(m_renderMode == RM_SOFTWARE ? 0 : 1,
							   *m_pRenderMap, *m_pBuffer, m_bgColor, m_pEnvironmentMap, m_matCamera, m_matViewProj);
	}
}
