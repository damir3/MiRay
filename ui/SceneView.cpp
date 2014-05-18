//
//  SceneView.cpp
//  MiRay/ui
//
//  Created by Damir Sagidullin on 01.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#ifndef _WIN32
#include <unistd.h>
#else
#include <direct.h>
#endif

#include "SceneView.h"
#include "SceneModel.h"
#include "OmniLight.h"
#include "RenderThread.h"
#include "SceneUtils.h"
#include "../rt/SoftwareRenderer.h"
#include "../rt/OpenCLRenderer.h"
#include "../resources/MaterialResource.h"

using namespace mr;

const float GIZMO_ARROW_LENGTH = 150.f;
const float GIZMO_CIRCLE_RADIUS = 300.f;
const float GIZMO_CIRCLE_THICKNESS = 20.f;

// ------------------------------------------------------------------------ //

SceneView::SceneView(const char * pResourcesPath)
	: m_resourcesPath(pResourcesPath)
	, m_fWidth(0.f)
	, m_fHeight(0.f)
	, m_fRWidth(0.f)
	, m_fRHeight(0.f)
	, m_fFOV(60.f)
	, m_fFocalDistance(10.f)
	, m_fDepthOfField(0.f)
	, m_fNearZ(0.1f)
	, m_fFarZ(1000.f)
	, m_matCamera(Matrix::Identity)
	, m_matView(Matrix::Identity)
	, m_matViewProj(Matrix::Identity)
	, m_bgColor(1.f, 1.f, 1.f)
	, m_bShowGrid(true)
	, m_bShowWireframe(false)
	, m_bShowNormals(false)
	, m_bShowBVH(false)
	, m_bShouldRedraw(false)
	, m_width(0), m_height(0)
	, m_texture(0)
	, m_nFrameCount(0)
	, m_renderMode(RM_SOFTWARE)
	, m_pBVH(new BVH())
	, m_pImageManager(new ImageManager())
	, m_pModelManager(NULL)
	, m_pEnvironmentMap(NULL)
	, m_rcRenderMap(0, 0, 0, 0)
	, m_pRenderMap(NULL)
	, m_pBuffer(NULL)
	, m_pRenderThread(new RenderThread())
	, m_iMouseDown(0)
	, m_vMousePos(0.f, 0.f)
	, m_gizmo(GIZMO_MOVE)
	, m_pGizmoObject(NULL)
	, m_pSelectedMaterial(NULL)
	, m_vGizmoStartDelta(0.f)
	, m_fGizmoScale(0.f)
	, m_iGizmoAxis(-1)
	, m_bGizmoTransformation(false)
	, m_vTargetPos(Vec3::Null)
	, m_vTargetNormal(Vec3::Null)
{
	m_pModelManager.reset(new ModelManager(m_pImageManager.get()));
	m_pRenderThread->SetRenderer(new SoftwareRenderer(*m_pBVH));

	ResetCamera();
}

SceneView::~SceneView()
{
	m_pRenderThread.reset();
	RemoveAllModels();
	RemoveAllLights();
	m_pBuffer.reset();
	m_pRenderMap.reset();
	m_pEnvironmentMap.reset();
	m_pModelManager.reset();
	m_pImageManager.reset();
	m_pBVH.reset();
}

bool SceneView::Init()
{
	return true;
}

void SceneView::Done()
{
}

// ------------------------------------------------------------------------ //

int SceneView::FramesCount() const { return m_pRenderThread ? m_pRenderThread->FramesCount() : 0; }
double SceneView::FramesRenderTime() const { return m_pRenderThread ? m_pRenderThread->FramesRenderTime() : 0.0; }

void SceneView::Resize(float w, float h, float rw, float rh)
{
	m_fWidth = w;
	m_fHeight = h;
	m_fRWidth = rw;
	m_fRHeight = rh;
	
	StopRenderThread();
	
	int width = (int)m_fRWidth;
	int height = (int)m_fRHeight;
	
	m_pRenderMap = m_pImageManager->Create(width, height, Image::TYPE_4F);
	
	m_pBuffer = m_pImageManager->Create(width, height, Image::TYPE_4F);
	if (m_pBuffer)
		std::fill(m_pBuffer->DataF(), m_pBuffer->DataF() + m_pBuffer->Width() * m_pBuffer->Height() * m_pBuffer->NumChannels(), 0.f);
	
	UpdateMatrices();
}

void SceneView::SetRenderMode(eRenderMode rm)
{
	m_renderMode = rm;
	StopRenderThread();
	ResumeRenderThread();
}

void SceneView::SetGizmo(eGizmo gizmo)
{
	if (m_gizmo == gizmo)
		return;

	m_gizmo = gizmo;
	m_bGizmoTransformation = false;
	m_iGizmoAxis = -1;
	m_bShouldRedraw = true;
	OnMouseMove(m_vMousePos.x, m_vMousePos.y, 0.f, 0.f, MOUSE_NONE);
}

void SceneView::ResetScene()
{
	StopRenderThread();

	RemoveAllModels();
	RemoveAllLights();
	ResetCamera();
}

bool SceneView::LoadScene(const char * pFilename)
{
	pugi::xml_document doc;
	if (doc.load_file(pFilename).status != pugi::status_ok)
		return false;

	pugi::xml_node node = doc.child("scene");
	if (node.empty())
		return false;

	StopRenderThread();

	RemoveAllModels();
	RemoveAllLights();

	std::string strPrevDirectory =  PushDirectory(pFilename);
	for (pugi::xml_node object = node.first_child(); object; object = object.next_sibling())
	{
		if (!strcmp(object.name(), "model"))
		{
			SceneModel *pSceneModel = new SceneModel(*m_pBVH);
			if (pSceneModel->Load(object, m_pModelManager.get()))
				m_models.push_back(pSceneModel);
			else
				SAFE_DELETE(pSceneModel);
		}
		else if (!strcmp(object.name(), "omni-light"))
		{
			OmniLight *pLight = new OmniLight();
			if (pLight->Load(object))
				m_lights.push_back(pLight);
			else
				SAFE_DELETE(pLight);
		}
		else if (!strcmp(object.name(), "camera"))
		{
			Vec3 position(0.f);
			float yaw = 0.f, pitch = 0.f;
			ReadVec3(position, "position", object);
			ReadFloat(yaw, "yaw", object);
			ReadFloat(pitch, "pitch", object);
			ReadFloat(m_fFOV, "fov", object);
			CalculateCameraMatrix(m_matCamera, position, 0.f, yaw, pitch);
			if (m_pRenderThread->Renderer())
			{
				ReadFloat(m_fFocalDistance, "focal-distance", object);
				ReadFloat(m_fDepthOfField, "depth-of-field", object);
				m_pRenderThread->Renderer()->SetFocalDistance(m_fFocalDistance);
				m_pRenderThread->Renderer()->SetDepthOfField(m_fDepthOfField);
			}
		}
		else if (!strcmp(object.name(), "environment"))
		{
			ReadVec3((Vec3 &)m_bgColor, "color", object);

			pugi::xml_node filename = object.child("map");
			if (!filename.empty())
				m_pEnvironmentMap = m_pImageManager->Load(filename.text().get());
		}
	}
	PopDirectory(strPrevDirectory.c_str());

	BBox bbox = m_pBVH->BoundingBox();
	float l = 30.f;
	if (!bbox.IsEmpty())
		l = bbox.Size().Length();
	m_fNearZ = l * 0.001f;
	m_fFarZ = l * 10.f;
	CalculateProjectionMatrix(m_matProj, m_fFOV, m_fWidth / m_fHeight, m_fNearZ, m_fFarZ);
	m_matView.Inverse(m_matCamera);
	m_matViewProj = m_matView * m_matProj;
	if (m_pRenderThread->Renderer())
		m_pRenderThread->Renderer()->SetLights(m_lights.size(), (ILight **)m_lights.data());

	m_pRenderThread->SetOpenCLRenderer(new OpenCLRenderer(*m_pBVH, (m_resourcesPath + "/kernel.cl").c_str(), "MainKernel"));

	ResumeRenderThread();

	return true;
}

bool SceneView::SaveScene(const char * pFilename) const
{
	pugi::xml_document doc;

	pugi::xml_node scene = doc.append_child("scene");

	{// save environment map
		pugi::xml_node node = scene.append_child("environment");
		node.append_child("map").text().set(m_pEnvironmentMap->Name().c_str());
		SaveVec3((Vec3 &)m_bgColor, "color", node);
	}

	{// save camera
		pugi::xml_node camera = scene.append_child("camera");
		SaveVec3(m_matCamera.Pos(), "position", camera);
		SaveFloat(RAD2DEG(m_matCamera.AxisZ().Yaw()), "yaw", camera);
		SaveFloat(RAD2DEG(m_matCamera.AxisZ().Pitch()), "pitch", camera);
		SaveFloat(m_fFOV, "fov", camera);
		SaveFloat(m_fFocalDistance, "focal-distance", camera);
		SaveFloat(m_fDepthOfField, "depth-of-field", camera);
	}

	for (auto it = m_lights.begin(); it != m_lights.end(); ++it)
		(*it)->Save(scene);

	for (auto it = m_models.begin(); it != m_models.end(); ++it)
		(*it)->Save(scene);

	return doc.save_file(pFilename, "\t", pugi::format_default, pugi::encoding_utf8);
}

void SceneView::AppendModel(const char * pFilename)
{
	StopRenderThread();

	SceneModel *pSceneModel = new SceneModel(*m_pBVH);
	if (pSceneModel->Init(pFilename, Matrix::Identity, m_pModelManager.get(), pugi::xml_node()))
	{
		RemoveAllModels();
		RemoveAllLights();

		m_models.push_back(pSceneModel);

		m_pRenderThread->SetOpenCLRenderer(new OpenCLRenderer(*m_pBVH, (m_resourcesPath + "/kernel.cl").c_str(), "MainKernel"));
		
		//if (m_pRenderThread->Renderer())
		//{// update lights
		//	BBox bbox = m_pBVH->BoundingBox();
		//	OmniLight * pLight = new OmniLight();
		//	pLight->SetOrigin(Vec3::Lerp3(bbox.vMins, bbox.vMaxs, Vec3(0.f, 0.f, 2.f)));
		//	pLight->SetRadius(bbox.Size().Length() * 0.04f);
		//	pLight->SetIntensity(Vec3(0.3f) * bbox.Size().LengthSquared());
		//	m_lights.push_back(pLight);
		//	m_pRenderThread->Renderer()->SetLights(m_lights.size(), (ILight **)m_lights.data());
		//}
		
		ResetCamera();
	}
	else
	{
		SAFE_DELETE(pSceneModel);

		ResumeRenderThread();
	}
}

void SceneView::DeleteObject(ITransformable *pObject)
{
	StopRenderThread();

	if (m_pGizmoObject == pObject)
		m_pGizmoObject = NULL;

	auto itModel = std::find(m_models.begin(), m_models.end(), pObject);
	if (itModel != m_models.end())
		m_models.erase(itModel);
	else
	{
		auto itLight = std::find(m_lights.begin(), m_lights.end(), pObject);
		if (itLight != m_lights.end())
		{
			m_lights.erase(itLight);
			if (m_pRenderThread->Renderer())
				m_pRenderThread->Renderer()->SetLights(m_lights.size(), (ILight **)m_lights.data());
		}
	}

	delete pObject;

	ResumeRenderThread();
}

void SceneView::DeleteSelection()
{
	DeleteObject(m_pGizmoObject);
}

bool SceneView::RemoveModel(ITransformable * pObject)
{
	auto it = std::find(m_models.begin(), m_models.end(), pObject);
	if (it == m_models.end())
		return false;

	delete (*it);
	m_models.erase(it);
	return true;
}

void SceneView::RemoveAllModels()
{
	m_pGizmoObject = NULL;

	for (auto it = m_models.begin(); it != m_models.end(); ++it)
		delete (*it);
	
	m_models.clear();
}

void SceneView::RemoveAllLights()
{
	m_pGizmoObject = NULL;
	if (m_pRenderThread && m_pRenderThread->Renderer())
		m_pRenderThread->Renderer()->SetLights(0, NULL);

	for (auto it = m_lights.begin(); it != m_lights.end(); ++it)
		delete (*it);

	m_lights.clear();
}

bool SceneView::SetEnvironmentImage(const char * pFilename)
{
	StopRenderThread();
	m_pEnvironmentMap = m_pImageManager->Load(pFilename);
	ResumeRenderThread();
	return m_pEnvironmentMap != NULL;
}

bool SceneView::SaveImage(const char * pFilename) const
{
	return m_pRenderMap ? m_pImageManager->Save(pFilename, *m_pRenderMap) : false;
}

// ------------------------------------------------------------------------ //

Vec3 SceneView::GetFrustumPosition(float x, float y, float z) const
{
	Matrix matViewProjInv;
	matViewProjInv.Inverse(m_matViewProj);
	Vec4 v(x / m_fRWidth * 2.f - 1.f, -(y / m_fRHeight * 2.f - 1.f), z, 1.f);
	v.Transform(matViewProjInv);
	return Vec3(v.x, v.y, v.z) / v.w;
}

Vec2 TraceBumpMap(Vec2 & tc, const Vec3 & dir,
				  const IMaterialLayer * pMaterial, const sMaterialContext & mc,
				  int numLinearSearchSteps, int numBinarySearchSteps);

bool SceneView::GetTarget(float x, float y, Vec3 & vPos, Vec3 & vNormal)
{
	TraceResult tr;
	if (m_pBVH->TraceRay(m_matCamera.Pos(), GetFrustumPosition(x, y, 1.f), tr))
	{
		if (tr.pTriangle && tr.pTriangle->Material() && tr.pTriangle->Material()->Layer(0)->HasBumpMap())
		{// bump mapping
			const IMaterialLayer *pMaterial = tr.pTriangle->Material()->Layer(0);

			sMaterialContext mc;
			mc.tc = tr.pTriangle->GetTexCoord(tr.pc);
			mc.dir = Vec3::Normalize(tr.pos - m_matCamera.Pos());
			mc.normal = tr.pTriangle->GetNormal(tr.pc);
			tr.pTriangle->GetTangents(mc.tangent, mc.binormal, mc.normal, pMaterial->BumpDepth());

			Vec2 res = TraceBumpMap(mc.tc, mc.dir, pMaterial, mc, 32, 5);
			vPos = tr.pos + mc.dir * (pMaterial->BumpDepth() * (res.x - 1.f) / res.y);
			vNormal = pMaterial->BumpMapNormal(mc);
		}
		else
		{
			vPos = tr.pos;
			vNormal = tr.pTriangle->GetNormal(tr.pc);
		}
		
		if (tr.pVolume)
			vNormal.TTransformNormal(tr.pVolume->InverseTransformation());
		vNormal.Normalize();

		return true;
	}

	BBox bbox = m_pBVH->BoundingBox();
	vPos = bbox.IsEmpty() ? Vec3::Null : bbox.Center();
	vNormal = Vec3::Null;
	return false;
}

bool SceneView::GetRayAxisIntersectionDelta(const Vec3 & rayTarget, const Vec3 & pos, const Vec3 & axis, bool checkIntersection, Vec3 & delta)
{
	Vec3 rayDir = rayTarget - m_matCamera.Pos();
	Vec3 normal = Vec3::Cross(rayDir, axis);
	if (normal.Normalize() == 0.f)
		return false;
	
	if (checkIntersection)
	{
		float d = Vec3::Dot(normal, pos - m_matCamera.Pos());
		if (fabsf(d) > GIZMO_ARROW_LENGTH * m_fGizmoScale * 0.05f)
			return false;
	}

	Vec3 dir = Vec3::Cross(normal, rayDir);
	float dist = Vec3::Dot(dir, m_matCamera.Pos() - pos) / Vec3::Dot(dir, axis);
	if (checkIntersection && (dist < 0.f || dist > GIZMO_ARROW_LENGTH * m_fGizmoScale))
		return false;

	delta = axis * dist;
	return true;
}

bool SceneView::GetRayPlaneIntersectionDelta(Vec3 & rayTarget, const Vec3 & pos, const Vec3 & axis, Vec3 & delta)
{
	Vec3 rayDir = rayTarget - m_matCamera.Pos();
	float d1 = Vec3::Dot(pos - m_matCamera.Pos(), axis);
	float d2 = Vec3::Dot(pos - rayTarget, axis);
	if ((d1 > 0.f) ^ (d2 <= 0.f))
		return false;

	Vec3 intersection = m_matCamera.Pos() + rayDir * (d1 / Vec3::Dot(rayDir, axis));
	delta = intersection - pos;
	return true;
}

bool SceneView::CheckRayCircleIntersectionDelta(Vec3 & rayTarget, const Vec3 & pos, const Vec3 & axis, Vec3 & delta)
{
	Vec3 dir;
	if (!GetRayPlaneIntersectionDelta(rayTarget, pos, axis, dir))
		return false;

	const float r1 = GIZMO_CIRCLE_RADIUS * m_fGizmoScale;
	const float r2 = r1 - GIZMO_CIRCLE_THICKNESS * m_fGizmoScale;

	float d = dir.Length();
	if (d < r2 || d > r1)
		return false;

	rayTarget = pos + dir;
	delta = dir;

	return true;
}


// ------------------------------------------------------------------------ //

void SceneView::ResetCamera(int i)
{
	BBox bbox = m_pBVH->BoundingBox();
	Vec3 vCenter = Vec3::Null;
	float l = 30.f;
	if (!bbox.IsEmpty())
	{
		vCenter = bbox.Center();
		l = bbox.Size().Length();
	}
	m_fNearZ = l * 0.001f;
	m_fFarZ = l * 10.f;

	float dist = l / powf(2.f, (float)i);
	if (m_pRenderThread->Renderer())
		m_pRenderThread->Renderer()->SetFocalDistance(dist);

	m_nFrameCount = 0;
	CalculateCameraMatrix(m_matCamera, vCenter, dist, -90.f, 10.f);
	
	UpdateMatrices();
}

void SceneView::RotateCamera(float dx, float dy)
{
	Matrix matRotation;
	matRotation.RotationAxis(Vec3::Z, DEG2RAD(-dx * 0.5f), m_vTargetPos);
	m_matCamera = m_matCamera * matRotation;
	float pitch = RAD2DEG(-m_matCamera.AxisZ().Pitch());
	float dPitch = clamp(pitch - dy * 0.5f, -89.99f, 89.99f) - pitch;
	matRotation.RotationAxis(m_matCamera.AxisX(), DEG2RAD(dPitch), m_vTargetPos);
	m_matCamera = m_matCamera * matRotation;

	// normalize camera matrix
	CalculateCameraMatrix(m_matCamera, m_matCamera.Pos(), 0.f,
						  RAD2DEG(m_matCamera.AxisZ().Yaw()), RAD2DEG(m_matCamera.AxisZ().Pitch()));
	UpdateMatrices();
}

void SceneView::MoveCamera(float dx, float dy)
{
	float fDistance = Vec3::Dot(m_matCamera.AxisZ(), (m_matCamera.Pos() - m_vTargetPos));
	m_matCamera.Pos() -= m_matCamera.Axis(0) * ((dx * 2.f / m_fRWidth) * fDistance / m_matProj.m11);
	m_matCamera.Pos() += m_matCamera.Axis(1) * ((dy * 2.f / m_fRHeight) * fDistance / m_matProj.m22);
	UpdateMatrices();
}

void SceneView::Zoom(float x, float y, float d)
{
	m_nFrameCount = 0;
	float fMul = powf(1.03f, fabsf(d));
	if (d < 0.f) fMul = 1.f / fMul;
	
	Vec3 vTarget, vNormal;
	GetTarget(x, y, vTarget, vNormal);
	Vec3 vDir = m_matCamera.Pos() - vTarget;
	float fDist = vDir.Normalize() * fMul;
	fDist = clamp(fDist, m_fNearZ, m_fFarZ * 0.9f);
	m_matCamera.Pos() = vTarget + vDir * fDist;
	
	UpdateMatrices();
}

void SceneView::UpdateMatrices()
{
	CalculateProjectionMatrix(m_matProj, m_fFOV, m_fWidth / m_fHeight, m_fNearZ, m_fFarZ);
	m_matView.Inverse(m_matCamera);
	m_matViewProj = m_matView * m_matProj;
	m_fGizmoScale = m_pGizmoObject ? PosScale(m_matGizmo.Pos()) : 0.f;

	StopRenderThread();
	ResumeRenderThread();
}

// ------------------------------------------------------------------------ //

void SceneView::OnMouseDown(float x, float y, eMouseButton button)
{
	m_iMouseDown |= button;
	m_bGizmoTransformation = (m_iGizmoAxis >= 0);
	if (m_bGizmoTransformation)
	{
		m_matGizmoStart = m_matGizmo;
		m_vGizmoStartDelta = m_vGizmoDelta;
	}
	else
		GetTarget(x, y, m_vTargetPos, m_vTargetNormal);
}

void SceneView::OnMouseUp(eMouseButton button)
{
	m_iMouseDown &= ~button;
	m_vTargetPos = Vec3::Null;
	m_bShouldRedraw = true;
}

void SceneView::OnMouseMove(float x, float y, float dx, float dy, eMouseButton button)
{
	m_nFrameCount = 0;
	m_vMousePos = Vec2(x, y);
	switch (button)
	{
		case MOUSE_LEFT:
		{
			if (m_bGizmoTransformation)
			{
				m_matGizmo = m_matGizmoStart;
				Vec3 vTargetPos = GetFrustumPosition(x, y, 1.f);

				switch (m_gizmo)
				{
					case GIZMO_MOVE:
						if (GetRayAxisIntersectionDelta(vTargetPos, m_matGizmoStart.Pos(), Matrix::Identity.Axis(m_iGizmoAxis), false, m_vGizmoDelta))
							m_matGizmo.Pos() += m_vGizmoDelta - m_vGizmoStartDelta;
						break;
						
					case GIZMO_ROTATE:
					{
						Vec3 vGizmoAxis(Vec3::Normalize(m_matGizmoStart.Axis(m_iGizmoAxis)));
						if (GetRayPlaneIntersectionDelta(vTargetPos, m_matGizmoStart.Pos(), vGizmoAxis, m_vGizmoDelta))
						{
							m_vGizmoDelta.Normalize();
							float dAngle = acosf(Vec3::Dot(m_vGizmoStartDelta, m_vGizmoDelta));
							if (Vec3::Dot(m_vGizmoDelta, Vec3::Cross(vGizmoAxis, m_vGizmoStartDelta)) < 0.f)
								dAngle = -dAngle;
							
							Matrix matRotation;
							matRotation.RotationAxis(Matrix::Identity.Axis(m_iGizmoAxis), dAngle);
							m_matGizmo = matRotation * m_matGizmo;
						}
						break;
					}
						
					case GIZMO_SCALE:
						break;

					case GIZMO_NONE:
						return;
				}

				StopRenderThread();
				m_pGizmoObject->SetTransformation(m_matGizmo);
				m_fGizmoScale = m_pGizmoObject ? PosScale(m_matGizmo.Pos()) : 0.f;
				ResumeRenderThread();
			}
			else
				RotateCamera(dx, dy);
			break;
		}
		case MOUSE_RIGHT:
		{
			MoveCamera(dx, dy);
			break;
		}
		default:
		{
			int axis = m_iGizmoAxis;
			m_iGizmoAxis = -1;
			if (m_pGizmoObject)
			{
				Vec3 vTargetPos = GetFrustumPosition(x, y, 1.f);
				switch (m_gizmo)
				{
					case GIZMO_MOVE:
						for (int i = 0; i < 3; i++)
						{
							if (GetRayAxisIntersectionDelta(vTargetPos, m_matGizmo.Pos(), Vec3::Normalize(Matrix::Identity.Axis(i)), true, m_vGizmoDelta))
								m_iGizmoAxis = i;
						}
						break;

					case GIZMO_ROTATE:
						for (int i = 0; i < 3; i++)
						{
							if (CheckRayCircleIntersectionDelta(vTargetPos, m_matGizmo.Pos(), Vec3::Normalize(m_matGizmo.Axis(i)), m_vGizmoDelta))
							{
								m_vGizmoDelta.Normalize();
								m_iGizmoAxis = i;
							}
						}
						break;
						
					default:
						break;
				}
			}
			m_bShouldRedraw |= (axis != m_iGizmoAxis);
			break;
		}
	}
}

bool SceneView::SetSelection(float x, float y, Vec3 * pPos)
{
	m_pGizmoObject = NULL;

	TraceResult tr;
	tr.pos = GetFrustumPosition(x, y, 1.f);
	if (m_pBVH->TraceRay(m_matCamera.Pos(), tr.pos, tr))
	{
		for (auto it = m_models.begin(); it != m_models.end(); ++it)
		{
			if ((*it)->GetVolume() == tr.pVolume)
			{
				m_pGizmoObject = (*it);
				m_pSelectedMaterial = const_cast<IMaterial *>(tr.pTriangle->Material());
				break;
			}
		}
	}
	
	for (auto it = m_lights.begin(); it != m_lights.end(); ++it)
	{
		if ((*it)->TraceRay(m_matCamera.Pos(), tr.pos, tr))
			m_pGizmoObject = (*it);
	}
	
	m_bShouldRedraw = true;

	if (!m_pGizmoObject)
		return false;

	m_matGizmo = m_pGizmoObject->Transformation();
	m_fGizmoScale = PosScale(m_matGizmo.Pos());
	if (pPos)
		*pPos = tr.pos;

	return true;
}

bool SceneView::SetSelectionMaterial(const char *material)
{
	SceneModel *pModel = dynamic_cast<SceneModel *>(m_pGizmoObject);
	if (!pModel)
		return false;

	MaterialResource *pMaterial = dynamic_cast<MaterialResource *>(m_pSelectedMaterial);
	if (!pMaterial)
		return false;

	pugi::xml_document doc;
	if (doc.load_file(material).status != pugi::status_ok)
		return false;
	
	pugi::xml_node node = doc.child("material");
	if (node.empty())
		return false;

	StopRenderThread();

	pMaterial->Load(node);

	ResumeRenderThread();

	return true;
}

Vec3 SceneView::WorldToView(const Vec3 &pos) const
{
	return pos.TransformedCoord(m_matView);
}

void SceneView::OnMouseClick(float x, float y, eMouseButton button)
{
	m_pGizmoObject = NULL;

	TraceResult tr;
	tr.pos = GetFrustumPosition(x, y, 1.f);
	if (m_pBVH->TraceRay(m_matCamera.Pos(), tr.pos, tr))
	{
		if (button == MOUSE_MIDDLE && m_pRenderThread->Renderer())
		{
			m_pRenderThread->Renderer()->SetFocalDistance(-tr.pos.TransformedCoord(m_matView).z);
			StopRenderThread();
			ResumeRenderThread();
		}
		else
		{
			for (auto it = m_models.begin(); it != m_models.end(); ++it)
			{
				if ((*it)->GetVolume() == tr.pVolume)
				{
					m_pGizmoObject = (*it);
					break;
				}
			}
		}
	}

	for (auto it = m_lights.begin(); it != m_lights.end(); ++it)
	{
		if ((*it)->TraceRay(m_matCamera.Pos(), tr.pos, tr))
			m_pGizmoObject = (*it);
	}

	if (m_pGizmoObject)
	{
		m_matGizmo = m_pGizmoObject->Transformation();
		m_fGizmoScale = PosScale(m_matGizmo.Pos());
	}

}

// ------------------------------------------------------------------------ //

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

float SceneView::PosScale(const Vec3 &p) const
{
	Vec4 vCenter(p, 1.f);
	vCenter.Transform(m_matViewProj);
	return vCenter.w / (m_fRWidth * m_matProj.m11);
}

void SceneView::DrawGizmo(const Matrix & mat, const BBox & bbox) const
{
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glMultMatrixf(mat);
	DrawWireframeBox(bbox, Color(128, 128, 128, 128));
	glPopMatrix();

	if (m_renderMode == RM_OPENGL)
		glClear(GL_DEPTH_BUFFER_BIT);

	glEnable(GL_BLEND);

	const Color axisColor[] = {Color(255, 0, 0), Color(0, 255, 0), Color(0, 0, 255)};

	switch (m_gizmo)
	{
		case GIZMO_MOVE:
			for (int i = 0; i < 3; i++)
				DrawArrow(mat.Pos(), Matrix::Identity.Axis(i), m_iGizmoAxis == i ? Color(255, 255, 0) : axisColor[i], GIZMO_ARROW_LENGTH * m_fGizmoScale);
			break;

		case GIZMO_ROTATE:
		{
			const float r1 = GIZMO_CIRCLE_RADIUS * m_fGizmoScale;
			const float r2 = r1 - GIZMO_CIRCLE_THICKNESS * m_fGizmoScale;

			if (m_iGizmoAxis >= 0 && m_iGizmoAxis < 3)
				DrawArrow(mat.Pos(), mat.Axis(m_iGizmoAxis), axisColor[m_iGizmoAxis], m_fGizmoScale * GIZMO_ARROW_LENGTH);

			for (int i = 0; i < 3; i++)
			{
				Color c1 = axisColor[i];
				Color c2(c1.r, c1.g, c1.b, 128);
				DrawCircle(mat.Pos(), mat.Axis(i), (m_iGizmoAxis >= 0 && i != m_iGizmoAxis) ? r1 : r2, r1, c1, c2, 2.f);
			}
			break;
		}

		default:
			break;
	}

	glDisable(GL_BLEND);
}

bool SceneView::ShouldRedraw() const
{
	if (m_bShouldRedraw)
		return true;

	return (m_renderMode != RM_OPENGL) ? (m_pRenderMap && m_pRenderThread->IsRenderMapUpdated()) : false;
}

void SceneView::Draw()
{
	m_bShouldRedraw = false;
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

		for (auto it = m_models.begin(); it != m_models.end(); ++it)
		{
			CollisionVolume * pVolume = (*it)->GetVolume();

			glPushMatrix();
			glMatrixMode(GL_MODELVIEW);
			glMultMatrixf(pVolume->Transformation());

			if (m_bShowWireframe)
				(*it)->DrawWireframe();
			else
				(*it)->Draw();

			if (m_bShowNormals)
				(*it)->DrawNormals(m_fNearZ);

			if (m_bShowBVH)
				DrawCollisionNode(pVolume->Root(), 0);

			glPopMatrix();
		}
	}

	DrawRenderMap();

//	if (m_pModel)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(m_matProj);
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(m_matView);

		if (m_iMouseDown && !m_bGizmoTransformation)
			DrawArrow(m_vTargetPos, m_vTargetNormal, Color(255, 255, 0),  GIZMO_ARROW_LENGTH * PosScale(m_vTargetPos));

		if (m_pGizmoObject)
			DrawGizmo(m_matGizmo, m_pGizmoObject->OOBB());

		glEnable(GL_BLEND);
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CW);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		for (auto it = m_lights.begin(); it != m_lights.end(); ++it)
			(*it)->Draw();

		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
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
	m_bShouldRedraw = true;
	if (m_renderMode != RM_OPENGL)
	{
		m_pRenderThread->Start(m_renderMode == RM_SOFTWARE ? 0 : 1,
							   *m_pRenderMap.get(), *m_pBuffer.get(), m_bgColor, m_pEnvironmentMap.get(), m_matCamera, m_matViewProj);
	}
}
