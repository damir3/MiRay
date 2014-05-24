//
//  SceneView.h
//  MiRay/ui
//
//  Created by Damir Sagidullin on 01.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

namespace mr
{

class Image;
class ImageManager;
class ModelManager;
class RenderThread;

class BVH;
class CollisionVolume;
class CollisionNode;
class CollisionTriangle;
struct TraceResult;

class ITransformable;
class IMaterial;
class ISceneLight;

class SceneModel;

enum eMouseButton
{
	MOUSE_NONE		= 0,
	MOUSE_LEFT		= 1,
	MOUSE_RIGHT		= 2,
	MOUSE_MIDDLE	= 4,
};

enum eGizmo
{
	GIZMO_NONE = 0,
	GIZMO_MOVE,
	GIZMO_ROTATE,
	GIZMO_SCALE,
};

class SceneView
{
public:
	enum eRenderMode
	{
		RM_OPENGL,
		RM_SOFTWARE,
		RM_OPENCL,
	};

private:
	const std::string	m_resourcesPath;
	float	m_fWidth, m_fHeight;
	float	m_fRWidth, m_fRHeight;
	float	m_fFOV, m_fNearZ, m_fFarZ;
	float	m_fFocalDistance;
	float	m_fDepthOfField;
	Matrix	m_matCamera;
	Matrix	m_matView;
	Matrix	m_matProj;
	Matrix	m_matViewProj;
	ColorF	m_bgColor;
	bool	m_bShowGrid;
	bool	m_bShowWireframe;
	bool	m_bShowNormals;
	bool	m_bShowBVH;
	bool	m_bShouldRedraw;
	GLint	m_width, m_height;
	GLuint	m_texture;
	int		m_nFrameCount;
	eRenderMode		m_renderMode;
	std::unique_ptr<BVH>			m_pBVH;
	std::shared_ptr<ImageManager>	m_pImageManager;
	std::shared_ptr<ModelManager>	m_pModelManager;
	std::shared_ptr<Image>			m_pEnvironmentMap;
	RectI			m_rcRenderMap;
	std::shared_ptr<Image>			m_pRenderMap;
	std::shared_ptr<Image>			m_pBuffer;
	std::unique_ptr<RenderThread>	m_pRenderThread;
	int				m_iMouseDown;
	Vec2			m_vMousePos;
	eGizmo			m_gizmo;
	ITransformable	*m_pGizmoObject;
	IMaterial		*m_pSelectedMaterial;
	Matrix			m_matGizmo;
	Matrix			m_matGizmoStart;
	Vec3			m_vGizmoStartDelta;
	float			m_fGizmoScale;
	float			m_fGizmoLength;
	int				m_iGizmoAxis;
	Vec2			m_vGizmoStartMousePos;
	Vec3			m_vGizmoDelta;
	bool			m_bGizmoTransformation;
	Vec3			m_vTargetPos;
	Vec3			m_vTargetNormal;

	std::vector<ISceneLight *>	m_lights;
	std::vector<SceneModel *>	m_models;

	void RemoveAllModels();
	bool RemoveModel(ITransformable * pObject);

	void RemoveAllLights();

	void DeleteObject(ITransformable *pObject);

	Vec3 GetFrustumPosition(float x, float y, float z) const;
	bool GetTarget(float x, float y, Vec3 & vPos, Vec3 & vNormal);
	float PosScale(const Vec3 & p) const;
	void UpdateGizmoSize();
	void DrawGizmo(const Matrix & mat, const BBox & bbox) const;

	void UpdateRenderMapTexture();
	void DrawRenderMap(bool blend);

	void RotateCamera(float dx, float dy);
	void MoveCamera(float dx, float dy);
	void UpdateMatrices();
	void Set3DMode();

public:
	SceneView(const char * pResourcesPath);
	~SceneView();

	eRenderMode RenderMode() const { return m_renderMode; }
	void SetRenderMode(eRenderMode rm);

	void SetGizmo(eGizmo gizmo);
	eGizmo Gizmo() const { return m_gizmo; }
	
	bool ShowGrid() const { return m_bShowGrid; }
	void SetShowGrid(bool b) { m_bShowGrid = b; m_bShouldRedraw = true; }

	bool ShowWireframe() const { return m_bShowWireframe; }
	void SetShowWireframe(bool b) { m_bShowWireframe = b; m_bShouldRedraw = true; }

	bool ShowNormals() const { return m_bShowNormals; }
	void SetShowNormals(bool b) { m_bShowNormals = b; m_bShouldRedraw = true; }

	bool ShowBVH() const { return m_bShowBVH; }
	void SetShowBVH(bool b) { m_bShowBVH = b; m_bShouldRedraw = true; }

	void SetBackgroundColor(const ColorF & bgColor) { m_bgColor = bgColor; m_bShouldRedraw = true; }

	int FramesCount() const;
	double FramesRenderTime() const;

	bool Init();
	void Done();

	void ResetScene();
	bool LoadScene(const char * pFilename);
	bool SaveScene(const char * pFilename) const;

	void AppendModel(const char * pFilename);

	void DeleteSelection();

	void Resize(float w, float h, float rw, float rh);

	bool SetEnvironmentImage(const char * pFilename);
	bool SaveImage(const char * pFilename) const;

	bool SetSelection(float x, float y, Vec3 * pPos);
	Vec3 WorldToView(const Vec3 &pos) const;
	bool SetSelectionMaterial(const char *material);

	void OnMouseDown(float x, float y, eMouseButton button);
	void OnMouseUp(float x, float y, eMouseButton button);
	void OnMouseClick(float x, float y, eMouseButton button);
	void OnMouseMove(float x, float y, float dx, float dy, eMouseButton button);

	void ResetCamera(int i = 0);
	void Zoom(float x, float y, float d);

//	void RenderScene(int width, int height, int samples);

	bool ShouldRedraw() const;
	void Draw();

	void StopRenderThread();
	void ResumeRenderThread();
};

}