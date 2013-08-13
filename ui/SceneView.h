//
//  SceneView.h
//  MiRay/ui
//
//  Created by Damir Sagidullin on 01.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

#include "../resources/Model.h"
#include "../rt/Light.h"

namespace mr
{
	class ImageManager;
	class ModelManager;
	class RenderModel;

	class BVH;
	class CollisionNode;
	class CollisionTriangle;
}

namespace mr
{

class RenderThread;

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
	bool	m_bGesture;
	Vec3	m_vGestureTarget;
	Vec3	m_vGestureTargetNormal;
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
	ImageManager	*m_pImageManager;
	ModelManager	*m_pModelManager;
	Model			*m_pModel;
	RenderModel		*m_pRenderModel;
	Image			*m_pEnvironmentMap;
	BVH				*m_pCS;
	RectI			m_rcRenderMap;
	Image			*m_pRenderMap;
	Image			*m_pBuffer;
	RenderThread	*m_pRenderThread;
	Vec3	m_vCollisionTriangle[3];

	class OmniLight : public ILight
	{
		Vec3	m_origin;
		float	m_radius;
		Vec3	m_intensity;
		
	public:
		void SetOrigin(const Vec3 & o) { m_origin = o; }
		void SetRadius(float r) { m_radius = r; }
		void SetIntensity(const Vec3 & i) { m_intensity = i; }

		const Vec3 & Origin() const { return m_origin; }
		float Radius() const { return m_radius; }
		const Vec3 & Intensity() const { return m_intensity; }

		Vec3 Position(const Vec3 & p) const;
		Vec3 Intensity(float squared_distance) const; // diffuse
		Vec3 Intensity(const Vec3 & rayPos, const Vec3 & rayDir) const; // specular
	};

	OmniLight		m_light;

	void CreateBVH();

	Vec3 GetTarget(float x, float y) const;
	void DrawGrid();
	void DrawCollisionNode(const CollisionNode * pCN, byte level) const;
	void DrawWireframeBox(const BBox & box, const Color & c) const;
	void DrawBox(const BBox & box, const Color & c) const;

	void UpdateRenderMapTexture();
	void DrawRenderMap();

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
	
	bool LoadScene(const char * pFilename);
	bool SetEnvironmentImage(const char * pFilename);
	bool SaveImage(const char * pFilename) const;

	void BeginGesture(float x, float y);
	void Pan(float dx, float dy);
	void Rotate(float dx, float dy);
	void EndGesture();
	void Zoom(float x, float y, float d);

	void ResetCamera(int i = 0);
//	void RenderScene(int width, int height, int samples);

	void OtherMouseDown(float x, float y);

	void Resize(float w, float h, float rw, float rh);
	void Draw();

	void StopRenderThread();
	void ResumeRenderThread();
};

}