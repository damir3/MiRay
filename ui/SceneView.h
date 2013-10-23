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
class ISceneLight;

class SceneModel;

enum eMouseButton
{
	MOUSE_NONE		= 0,
	MOUSE_LEFT		= 1,
	MOUSE_RIGHT		= 2,
	MOUSE_MIDDLE	= 4,
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
	Matrix	m_matCamera;
	Matrix	m_matView;
	Matrix	m_matProj;
	Matrix	m_matViewProj;
	ColorF	m_bgColor;
	bool	m_bShowGrid;
	bool	m_bShowWireframe;
	bool	m_bShowNormals;
	bool	m_bShowBVH;
	GLint	m_width, m_height;
	GLuint	m_texture;
	int		m_nFrameCount;
	eRenderMode		m_renderMode;
	BVH				*m_pBVH;
	ImageManager	*m_pImageManager;
	ModelManager	*m_pModelManager;
	Image			*m_pEnvironmentMap;
	RectI			m_rcRenderMap;
	Image			*m_pRenderMap;
	Image			*m_pBuffer;
	RenderThread	*m_pRenderThread;
	ITransformable	*m_pGizmoObject;
	int				m_iMouseDown;
	Vec3			m_vTargetPos;
	Vec3			m_vTargetNormal;
	Matrix			m_matGizmo;
	Matrix			m_matGizmoStart;
	Vec3			m_vGizmoStartDelta;
	Vec2			m_vAxisSize;
	int				m_iAxis;
	Vec3			m_vAxisDelta;

//	enum eTransformationType
//	{
//		TT_NONE,
//		TT_MOVE,
//		TT_ROTATE,
//		TT_SCALE,
//	};
//	eTransformationType	m_transformation;
	bool			m_bTransformation;

	std::vector<ISceneLight *>	m_lights;
	std::vector<SceneModel *>	m_models;

	void RemoveAllModels();
	void RemoveModel(SceneModel * pModel);

	void RemoveAllLights();

	Vec3 GetFrustumPosition(float x, float y, float z) const;
	bool GetTarget(float x, float y, Vec3 & vPos, Vec3 & vNormal);
	bool GetRayTranslationAxisDist(const Vec3 & rayTarget, int axis, bool checkIntersection, Vec3 & vIntersectionDir);
	void DrawArrow(const Vec3 & pos, const Vec3 & dir, const Color & c, float size) const;
	void DrawGizmo(const Matrix & mat, const BBox & bbox) const;

	void UpdateRenderMapTexture();
	void DrawRenderMap();

	void RotateCamera(float dx, float dy);
	void TranslateCamera(float dx, float dy);
	void UpdateMatrices();

public:
	SceneView(const char * pResourcesPath);
	~SceneView();

	eRenderMode RenderMode() const { return m_renderMode; }
	void SetRenderMode(eRenderMode rm);

	bool ShowGrid() const { return m_bShowGrid; }
	void SetShowGrid(bool b) { m_bShowGrid = b; }

	bool ShowWireframe() const { return m_bShowWireframe; }
	void SetShowWireframe(bool b) { m_bShowWireframe = b; }

	bool ShowNormals() const { return m_bShowNormals; }
	void SetShowNormals(bool b) { m_bShowNormals = b; }

	bool ShowBVH() const { return m_bShowBVH; }
	void SetShowBVH(bool b) { m_bShowBVH = b; }

	void SetBackgroundColor(const ColorF & bgColor) { m_bgColor = bgColor; }

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

	void OnMouseDown(float x, float y, eMouseButton button);
	void OnMouseUp(eMouseButton button);
	void OnMouseClick(float x, float y, eMouseButton button);
	void OnMouseMove(float x, float y, float dx, float dy, eMouseButton button);

	void ResetCamera(int i = 0);
	void Zoom(float x, float y, float d);

//	void RenderScene(int width, int height, int samples);

	void Draw();

	void StopRenderThread();
	void ResumeRenderThread();
};

}