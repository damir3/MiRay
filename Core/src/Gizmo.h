#pragma once

#include "../../Shared/Interfaces/CoreRenderer.h"
#include <cstddef>

class CoreRenderer;
class CoreInstance;
class Scene;
class DrawGL;

class Gizmo
{
public:
	enum eState {
		STATE_NONE,
		STATE_CAMERA_ROTATE,
		STATE_CAMERA_PAN,
		STATE_TRANSFORMATION
	};

protected:
	CoreRenderer & m_widget;
	CoreInstance & m_core;
	Scene &        m_scene;
	DrawGL *       m_gl = nullptr;
	float	m_pixelRatio = 1.f;
	eGizmo	m_type = GIZMO_NONE;
	eState	m_state = STATE_NONE;
	QPoint	m_mousePos = { 0, 0 };
	QPoint	m_mousePressPos = { 0, 0 };
	bool	m_mouseMoved = false;
	vec3	m_targetPos = vec3(0.f);
	vec3	m_targetNormal = vec3(0.f);
	vec3	m_mouseWorldPos = vec3(0.f);
	vec3	m_mouseNormal = vec3(0.f);
	vec3	m_mouseGeomNormal = vec3(0.f);
	bool	m_localSpace = false;
	mat4	m_matGizmo = mat4(1.f);
	mat4	m_matGizmoStart = mat4(1.f);
	float	m_scale = 0.f;
	int		m_activeAxis = -1;
	int		m_gizmoFirstAxis = 0;
	bool	m_anchorPointActivated = false;
	bool	m_rotatePointActivated = false;
	bool	m_frameActivated = false;
	vec3	m_delta = vec3(0.f);
	vec3	m_startDelta = vec3(0.f);

	bool	m_nodeSelection = false;
	NodeSelection m_selection;
	class TransformationCommand * m_transformationCommand = nullptr;

	void setState(eState state);

	void rotateCamera(float dx, float dy);
	void panCamera(float dx, float dy);

	Node *getTarget(int x, int y, vec3 & pos, vec3 & normal, vec3 *geomNormal = nullptr) const;
	bool checkMouseDistance(const vec2 &p, float dist) const;
	void updateOnMouseMove();
	void updateOnMouseDrag(Qt::KeyboardModifiers modifiers);
	void drawLights() const;
	void drawSelection() const;
	bool isLocked() const { return m_state != STATE_NONE && m_state != STATE_CAMERA_ROTATE; }

public:
	Gizmo(CoreRenderer & widget, CoreInstance & core);

	void initializeGL(DrawGL * funcs, float pixelRatio);
	void destroyGL();

	void setLocalMode(bool local);

	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);

	void onSelectionChanged();
	void onCameraChanged();

	bool isMouseMoved() const { return m_mouseMoved; }
	void updateNormal();

	eGizmo type() const { return m_type; }
	void setType(eGizmo gizmo);

	void draw() const;
};
