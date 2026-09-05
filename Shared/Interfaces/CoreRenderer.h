#pragma once

#include "Image.h"

enum eGizmo
{
	GIZMO_NONE = 0,
	GIZMO_MOVE,
	GIZMO_ROTATE,
	GIZMO_SCALE,
	GIZMO_CAMERA,
};

class IGeometry;

#include <QQuickFramebufferObject>

class QOpenGLFramebufferObject;

class ICoreRenderer : public QQuickFramebufferObject::Renderer
{
protected:
	virtual ~ICoreRenderer() {}

public:
	virtual void invalidate() = 0;

	virtual ImagePtr createScreenshot(int width, int height) = 0;

	virtual void hoverMoveEvent(QHoverEvent *event) = 0;
	virtual void mousePressEvent(QMouseEvent * event) = 0;
	virtual void mouseMoveEvent(QMouseEvent * event) = 0;
	virtual void mouseReleaseEvent(QMouseEvent * event) = 0;
	virtual void wheelEvent(QWheelEvent * event) = 0;

	virtual const QSize & getSize() = 0;

	virtual void enablePreview(bool b) = 0;

	virtual eGizmo getGizmo() const = 0;
	virtual void setGizmo(eGizmo g) = 0;

	virtual bool getShowNormal() const = 0;
	virtual void setShowNormal(bool b) = 0;

	virtual bool getEnableDenoise() const = 0;
	virtual void setEnableDenoise(bool b) = 0;

	virtual int getMaxIterations() const = 0;
	virtual void setMaxIterations(int) = 0;

	virtual void updateFrame() = 0;

	virtual QOpenGLFramebufferObject * getFramebufferObject() const = 0;

	virtual IGeometry * getGeometry(int x, int y) const = 0;
};

Q_DECLARE_INTERFACE(ICoreRenderer, "org.miray.ICoreRenderer")
