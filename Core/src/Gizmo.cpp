#include "Gizmo.h"
#include "RenderUtils.h"
#include "CoreRenderer.h"

static const float GIZMO_ARROW_LENGTH = 200.f;
static const float GIZMO_CIRCLE_RADIUS = 200.f;
static const float GIZMO_CIRCLE_THICKNESS = 20.f;
static const float GIZMO_BOX_SIZE = 30.f;
static const float GIZMO_MIN_SCALE = 0.1f;
static const float GIZMO_AXIS_LENGTH = 50000.f;
static const vec4  GIZMO_COLOR1(1.f, 1.f, 0.f, 1.f);
static const vec4  GIZMO_COLOR2(0.f, 0.5f, 1.f, 1.f);
static const vec4  GIZMO_ACTIVE_COLOR(1.f, 1.f, 0, 1.f);
static const vec4  GIZMO_AXIS_COLOR[] = { vec4(1.f, 0, 0, 1.f), vec4(0, 1.f, 0, 1.f), vec4(0, 0, 1.f, 1.f), vec4(1.f, 1.f, 1.f, 1.f) };

Gizmo::Gizmo(CoreRenderer & widget, CoreInstance & core)
	: m_widget(widget)
	, m_core(core)
	, m_scene(core.scene())
{
}

// ------------------------------------------------------------------------ //

void Gizmo::initializeGL(DrawGL * gl, float pixelRatio)
{
	m_gl = gl;
	m_pixelRatio = pixelRatio;
}

void Gizmo::destroyGL()
{
	m_gl = nullptr;
}

// ------------------------------------------------------------------------ //

void Gizmo::setType(eGizmo gizmo)
{
	if (m_type != gizmo) {
		if (gizmo == GIZMO_CAMERA) {
			m_matGizmo = glm::translate(glm::mat4(1.f), m_scene.camera().target().get());
			m_matGizmoStart = m_matGizmo;
			m_scale = m_widget.getScale(glm::translation(m_matGizmo));
		}

		m_type = gizmo;
		m_activeAxis = -1;
		onSelectionChanged();
		updateOnMouseMove();
		m_widget.update();
	}
}

void Gizmo::setState(eState state)
{
	m_state = state;
}

// ------------------------------------------------------------------------ //

void Gizmo::drawLights() const
{
	const auto & root = m_scene.root();
	for (size_t i = 0; i < root.numChildren(SceneElement_Node); i++) {
		const auto node = root.child(i, SceneElement_Node);
		if (node->type() == SceneElement_Light) {
			if (const auto lightNode = qobject_cast<LightNode *>(node)) {
				auto scale = m_widget.getScale(lightNode->position().get());
				if (scale > 0.f && lightNode->visible().get())
					m_gl->drawGeosphere(lightNode->position().get(), lightNode->radius().get(), vec4(lightNode->color().get(), 1.f));
			}
		}
	}
}

void Gizmo::drawSelection() const
{
	const auto & selection = m_scene.selection();
	if (selection.empty()) return;

	for (const auto node : m_scene.nodeSelection()) {
		switch (node->type()) {
			case SceneElement_Node:
			case SceneElement_MeshNode:
			case SceneElement_Light: {
				const auto & mat = node->globalTransformation();
				if (m_type == GIZMO_NONE) {
					const auto & pos = glm::translation(mat);
					auto l = 0.5f * GIZMO_ARROW_LENGTH * m_widget.getScale(pos);
					for (int i = 0; i < 3; i++)
						m_gl->drawLine(pos, pos + glm::normalize(glm::axis(mat, i)) * l, GIZMO_AXIS_COLOR[i], 1.f);
				}

				m_gl->drawWireframeBox(node->oobb(), mat, vec4(1.f, 1.f, 0.f, 1.f), m_pixelRatio, 6.f, 1.f);
				break;
			}
			case SceneElement_DirectionalLight: {
				const auto light = static_cast<DirectionalLight *>(node);
				const auto & mat = light->globalTransformation();
				const auto & pos = m_scene.camera().target().get();
				auto l = GIZMO_ARROW_LENGTH * m_widget.getScale(pos);
				const auto & dir = glm::axisX(mat);
				m_gl->drawArrow(pos + dir * (l * 0.6f), -dir, l * 1.2f, vec4(light->color().get(), 1.f));
				break;
			}
		}
	}

	for (const auto geom : m_scene.geomSelection())
		m_gl->drawWireframeBox(static_cast<Geometry *>(geom)->bbox(), geom->parent()->globalTransformation(), vec4(0.f, 1.f, 0.f, 1.f), m_pixelRatio, 6.f);
}

void Gizmo::draw() const
{
	m_gl->glEnable(GL_DEPTH_TEST);
	m_gl->glDisable(GL_BLEND);

	if (m_mouseMoved)
		m_gl->drawGrid();

	drawLights();

	drawSelection();

	m_gl->glEnable(GL_BLEND);
	m_gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	m_gl->glClear(GL_DEPTH_BUFFER_BIT);

	if (m_widget.getShowNormal() && (m_activeAxis == -1) &&
		(m_state == STATE_NONE || m_state == STATE_CAMERA_ROTATE || m_state == STATE_CAMERA_PAN)) {
		const auto length = GIZMO_ARROW_LENGTH * m_widget.getScale(m_mouseWorldPos);
		if (glm::length2(m_mouseGeomNormal) > 0.f)
			m_gl->drawArrow(m_mouseWorldPos, m_mouseGeomNormal, 0.75f * length, GIZMO_COLOR2);

		if (glm::length2(m_mouseNormal) > 0.f)
			m_gl->drawArrow(m_mouseWorldPos, m_mouseNormal, length, GIZMO_COLOR1);
	}

	if (m_nodeSelection || m_type == GIZMO_CAMERA) {
		const auto gizmoLength = GIZMO_ARROW_LENGTH * m_scale;
		const auto lineLength = GIZMO_AXIS_LENGTH * m_scale;
		const auto & pos = glm::translation(m_matGizmo);
		const auto matCoordSystem = m_localSpace || m_type == GIZMO_SCALE ? m_matGizmo : mat4(1.f);

		switch (m_type) {
			case GIZMO_CAMERA:
			case GIZMO_MOVE:
				for (int i = 0; i < 3; i++) {
					const auto axis = glm::normalize(glm::axis(matCoordSystem, i));
					if (m_activeAxis < 0 || i == m_activeAxis) {
						m_gl->drawArrow(pos, axis, gizmoLength, i == m_activeAxis ? GIZMO_ACTIVE_COLOR : GIZMO_AXIS_COLOR[i]);
						if (i == m_activeAxis)
							m_gl->drawLine(pos - axis * lineLength, pos + axis * lineLength, GIZMO_AXIS_COLOR[i], 2.f);
					} else
						m_gl->drawLine(pos, pos + axis * gizmoLength, GIZMO_AXIS_COLOR[i], 1.f);
				}
				break;

			case GIZMO_ROTATE: {
				const float r1 = GIZMO_CIRCLE_RADIUS * m_scale;
				const float r2 = (GIZMO_CIRCLE_RADIUS - GIZMO_CIRCLE_THICKNESS) * m_scale;

				for (int i = m_gizmoFirstAxis; i < 3; i++) {
					const auto axis = glm::normalize(glm::axis(matCoordSystem, i));
					vec4 c1 = i == m_activeAxis ? GIZMO_ACTIVE_COLOR : GIZMO_AXIS_COLOR[i];
					vec4 c2(c1.r, c1.g, c1.b, 0.5f);
					if (i == m_activeAxis)
						m_gl->drawLine(pos - axis * lineLength, pos + axis * lineLength, GIZMO_AXIS_COLOR[i], 2.f);
					else
						m_gl->drawLine(pos - axis * r2, pos + axis * r2, GIZMO_AXIS_COLOR[i], 1.f);

					if (i >= m_gizmoFirstAxis && (m_activeAxis < 0 || i == m_activeAxis))
						m_gl->drawCircle(pos, axis, r1, c1, c2);
				}
				break;
			}
			case GIZMO_SCALE: {
				vec3 s(gizmoLength);
				vec3 ds(0.5f * GIZMO_BOX_SIZE * m_scale);
				float r = s.x * 0.02f;
				if (m_state == STATE_TRANSFORMATION && m_activeAxis >= 0 && m_activeAxis < 3)
					s[m_activeAxis] *= glm::length(glm::axis(m_matGizmo, m_activeAxis)) / glm::length(glm::axis(m_matGizmoStart, m_activeAxis));

				for (int i = 0; i < 3; i++) {
					const auto axis = glm::normalize(glm::axis(m_matGizmo, i));
					m_gl->drawCylinder(pos, axis, (s[i] - ds[i] * 2.f), r, i == m_activeAxis ? GIZMO_ACTIVE_COLOR : GIZMO_AXIS_COLOR[i]);
				}

				auto matNormalized = m_matGizmo;
				glm::setAxisX(matNormalized, glm::normalize(glm::axisX(matNormalized)));
				glm::setAxisY(matNormalized, glm::normalize(glm::axisY(matNormalized)));
				glm::setAxisZ(matNormalized, glm::normalize(glm::axisZ(matNormalized)));

				for (int i = 0; i < 4; i++) {
					auto p = i < 3 ? glm::axis(mat4(1.f), i) * (s[i] - ds[i]) : vec3(0.f);
					m_gl->drawBox(BBox(p - ds, p + ds), matNormalized, i == m_activeAxis ? GIZMO_ACTIVE_COLOR : GIZMO_AXIS_COLOR[i]);
				}
				break;
			}

			default:
				break;
		}
	}
}

// ------------------------------------------------------------------------ //

bool Gizmo::checkMouseDistance(const vec2 & p, float dist) const
{
	return glm::length2(vec2(m_mousePos.x(), m_mousePos.y()) - (p * vec2(0.5f, -0.5f) + 0.5f) * m_gl->getViewportSize()) < dist * dist;
}

void traceBumpMap(Ray & ray, TextureScalarParameterImpl & bump, int numLinearSearchSteps, int numBinarySearchSteps);

Node* Gizmo::getTarget(int x, int y, vec3 & pos, vec3 & normal, vec3 *geomNormal) const
{
	pos = vec3(0.f);
	normal = vec3(0.f);
	if (geomNormal) *geomNormal = vec3(0.f);

	if (!m_scene.isValid())
		return nullptr;

	Ray ray;
	ray.init(m_widget.getFrustumPosition(x, y, -1.f), m_widget.getFrustumPosition(x, y, 1.f));
	rtcIntersect1(m_scene.rtcScene(), &ray);
	if (ray.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
		if (ray.hit.primID == RTC_INVALID_GEOMETRY_ID) { // light node
			ray.updateLightIntersection();
			return nullptr;
		}

		if (ray.geom) {
			// mesh node
			auto node = ray.geom->meshNode();

			ray.updateGeometryIntersection();

			auto material = ray.geom->material();
			if (material->hasBump())
				traceBumpMap(ray, material->bump(), 16, 5);

			pos = ray.hitPoint;
			normal = ray.normal;
			if (geomNormal) *geomNormal = ray.geomNormal;

			return node;
		}
	}

	pos = ray.target(m_scene.camera().distance().get());

	return nullptr;
}

static eSelectionOperation GetSelectionOperation(Qt::KeyboardModifiers modifiers)
{
	return modifiers & Qt::ControlModifier ? SelectionOperation_Invert :
		modifiers & Qt::ShiftModifier ? SelectionOperation_Add :
		modifiers & Qt::AltModifier ? SelectionOperation_Remove :
		SelectionOperation_Set;
}

// ------------------------------------------------------------------------ //

void Gizmo::mousePressEvent(QMouseEvent *event)
{
	if (m_state != STATE_NONE) return;
	m_mousePos = m_mousePressPos = event->pos();
	m_mouseMoved = false;

	if (event->button() == Qt::RightButton) {
		getTarget(m_mousePos.x(), m_mousePos.y(), m_targetPos, m_targetNormal);
		LogInformation() << QString().asprintf("Target pos(%.2f %.2f %.2f) normal(%.3f %.3f %.3f)",
											  m_targetPos.x, m_targetPos.y, m_targetPos.z,
											  m_targetNormal.x, m_targetNormal.y, m_targetNormal.z);
		m_widget.update();
		setState(STATE_CAMERA_PAN);
		return;
	}

	if (event->button() != Qt::LeftButton)
		return;

	if (m_activeAxis >= 0) {
		setState(STATE_TRANSFORMATION);
		m_matGizmoStart = m_matGizmo;
		m_startDelta = m_delta;
	} else {
		setState(STATE_CAMERA_ROTATE);
	}
}

void Gizmo::updateNormal()
{
	if (m_widget.getShowNormal() && m_state == STATE_NONE) {
		getTarget(m_mousePos.x(), m_mousePos.y(), m_mouseWorldPos, m_mouseNormal, &m_mouseGeomNormal);
		m_widget.update();
	}
}

void Gizmo::mouseMoveEvent(QMouseEvent *event)
{
	auto pos = event->pos();
	float dx = pos.x() - m_mousePos.x();
	float dy = pos.y() - m_mousePos.y();
	m_mousePos = pos;
	m_mouseMoved |= (m_state != STATE_NONE) && (std::max(std::fabs(pos.x() - m_mousePressPos.x()), std::fabs(pos.y() - m_mousePressPos.y())) > 3);
	updateNormal();
	if (m_mouseMoved)
		m_widget.switchToDummyRenderer();

	switch (m_state) {
		case STATE_NONE:
			updateOnMouseMove();
			break;
		case STATE_CAMERA_ROTATE:
			rotateCamera(dx, dy);
			break;
		case STATE_CAMERA_PAN:
			panCamera(dx, dy);
			break;
		case STATE_TRANSFORMATION:
			updateOnMouseDrag(event->modifiers());
			break;
	}
}

void Gizmo::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		switch (m_state) {
			case STATE_CAMERA_ROTATE:
				if (!m_mouseMoved) {
					m_scene.setSelection(m_widget.getX(event->pos().x()), m_widget.getY(event->pos().y()), GetSelectionOperation(event->modifiers()));

					updateOnMouseMove();
				}
				break;

			case STATE_TRANSFORMATION:
				if (m_type == GIZMO_CAMERA)
					m_scene.camera().target().set(glm::translation(m_matGizmo));
				else if (m_transformationCommand) {
					m_scene.pushCommand(m_transformationCommand);
					m_transformationCommand = nullptr;
				}

				updateOnMouseMove();
				m_widget.onSelectionChanged();
				break;
		}
	} else if (event->button() == Qt::RightButton) {
		if (m_state == STATE_CAMERA_PAN && !m_mouseMoved) {
			if (event->modifiers() & Qt::ControlModifier) { // set camera target position
				vec3 target, normal;
				if (getTarget(event->pos().x(), event->pos().y(), target, normal))
					m_scene.camera().setTarget(target);
			} else if (event->modifiers() & Qt::AltModifier) {// set focus target
				vec3 target, normal;
				if (getTarget(event->pos().x(), event->pos().y(), target, normal))
					m_scene.camera().setFocus(target);
			}
		}
	}

	setState(STATE_NONE);
	m_mouseMoved = false;
	m_targetNormal = vec3(0.f);
	updateNormal();
	m_widget.update();
}

// ------------------------------------------------------------------------ //

void Gizmo::updateOnMouseMove()
{
	auto axis = m_activeAxis;
	m_activeAxis = -1;
	m_scale = m_widget.getScale(glm::translation(m_matGizmo));
	if (m_scale > 0.f && (m_nodeSelection || m_type == GIZMO_CAMERA)) {
		const auto matCoordSystem = m_localSpace ? m_matGizmoStart : mat4(1.f);
		const auto rayStart = m_widget.getFrustumPosition(m_mousePos.x(), m_mousePos.y(), -1.f);
		auto rayEnd = m_widget.getFrustumPosition(m_mousePos.x(), m_mousePos.y(), 1.f);
		const auto & pos = glm::translation(m_matGizmo);
		const auto gizmoLength = GIZMO_ARROW_LENGTH * m_scale;

		switch (m_type) {
			case GIZMO_CAMERA:
			case GIZMO_MOVE:
				for (int i = 0; i < 3; i++) {
					if (getRayAxisIntersectionDelta(rayStart, rayEnd, pos, glm::normalize(glm::axis(matCoordSystem, i)),
													gizmoLength, gizmoLength * 0.05f, m_delta)) {
						rayEnd = pos + m_delta;
						m_activeAxis = i;
					}
				}
				break;

			case GIZMO_ROTATE: {
				const auto r1 = GIZMO_CIRCLE_RADIUS * m_scale;
				const auto r2 = (GIZMO_CIRCLE_RADIUS - GIZMO_CIRCLE_THICKNESS) * m_scale;
				for (int i = m_gizmoFirstAxis; i < 3; i++) {
					vec3 delta;
					if (getRayPlaneIntersectionDelta(rayStart, rayEnd, pos, glm::normalize(glm::axis(matCoordSystem, i)), delta)) {
						auto l = glm::length(delta);
						if (l > r2 && l < r1) {
							rayEnd = pos + delta;
							m_delta = delta / l;
							m_activeAxis = i;
						}
					}
				}
				break;
			}

			case GIZMO_SCALE: {
				auto matNormalized = m_matGizmo;
				glm::setAxisX(matNormalized, glm::normalize(glm::axisX(matNormalized)));
				glm::setAxisY(matNormalized, glm::normalize(glm::axisY(matNormalized)));
				glm::setAxisZ(matNormalized, glm::normalize(glm::axisZ(matNormalized)));

				auto matGizmoInv = glm::inverse(matNormalized);
				auto rayStartLocal = glm::transformCoord(rayStart, matGizmoInv);
				auto rayEndLocal = glm::transformCoord(rayEnd, matGizmoInv);

				vec3 ds(0.5f * GIZMO_BOX_SIZE * m_scale);
				for (int i = 0; i < 4; i++) {
					auto p = i < 3 ? glm::axis(mat4(1.f), i) * (gizmoLength - ds.x) : vec3(0.f);
					auto f = getRayBoxIntersection(rayStartLocal, rayEndLocal, BBox(p - ds, p + ds));
					if (f < 1.f) {
						m_activeAxis = i;
						rayEndLocal = glm::mix(rayStart, rayEndLocal, f);
					}
				}

				if (m_activeAxis >= 0 && m_activeAxis < 3)
					getRayAxisIntersectionDelta(rayStart, rayEnd, pos, glm::normalize(glm::axis(m_matGizmo, m_activeAxis)), 0.f, 0.f, m_delta);
				break;
			}

			default:
				break;
		}
	}

	if (axis != m_activeAxis)
		m_widget.update();
}

void Gizmo::updateOnMouseDrag(Qt::KeyboardModifiers modifiers)
{
	const auto rayStart = m_widget.getFrustumPosition(m_mousePos.x(), m_mousePos.y(), -1.f);
	const auto rayEnd = m_widget.getFrustumPosition(m_mousePos.x(), m_mousePos.y(), 1.f);

	const auto matBasis = m_localSpace || m_type == GIZMO_SCALE ? m_matGizmoStart : mat4(1.f);
	const auto axisDir = glm::normalize(glm::axis(matBasis, m_activeAxis));
	const auto & axisPos = glm::translation(m_matGizmoStart);
	const auto gizmoLength = GIZMO_ARROW_LENGTH * m_scale;
	m_matGizmo = m_matGizmoStart;

	if (!m_transformationCommand && m_type != GIZMO_CAMERA)
		m_transformationCommand = new TransformationCommand(m_scene, m_selection, true);

	switch (m_type) {
		case GIZMO_CAMERA:
		case GIZMO_MOVE:
			if (getRayAxisIntersectionDelta(rayStart, rayEnd, axisPos, axisDir, 0.f, 0.f, m_delta))
				glm::setTranslation(m_matGizmo, glm::translation(m_matGizmo) + m_delta - m_startDelta);
			break;

		case GIZMO_ROTATE:
			if (getRayPlaneIntersectionDelta(rayStart, rayEnd, axisPos, axisDir, m_delta)) {
				int axis2 = (m_activeAxis + 1) % 3;
				const auto axis2Dir = glm::normalize(glm::axis(matBasis, axis2));
				vec3 dir(0.f);
				float dp = FLT_MAX;
				for (int i = 0; i < 3; i++) {
					auto tdir = glm::normalize(glm::axis(m_matGizmoStart, i));
					auto tdp = std::fabs(glm::dot(axisDir, tdir));
					if (dp > tdp) {
						dp = tdp;
						dir = tdir;
					}
				}

				dir = glm::normalize(dir - axisDir * glm::dot(dir, axisDir));
				auto angle = std::copysign(std::acos(std::clamp(glm::dot(dir, axis2Dir), -1.f, 1.f)), glm::dot(dir, glm::cross(axisDir, axis2Dir)));

				m_delta = glm::normalize(m_delta);
				auto dAngle = std::acos(std::clamp(glm::dot(m_startDelta, m_delta), -1.f, 1.f));
				if (glm::dot(m_delta, glm::cross(axisDir, m_startDelta)) < 0.f)
					dAngle = -dAngle;

				if (!(modifiers & Qt::ShiftModifier)) {
					angle += dAngle;
					const auto angle2 = std::round(angle / M_PI_4f) * M_PI_4f;
					if (std::fabs(angle2 - angle) < glm::radians(5.f))
						dAngle += angle2 - angle;
				}

				auto matRotation = glm::rotate(glm::mat4(1.f), dAngle, axisDir);
				matRotation[3][0] = axisPos.x - (axisPos.x * matRotation[0][0] + axisPos.y * matRotation[1][0] + axisPos.z * matRotation[2][0]);
				matRotation[3][1] = axisPos.y - (axisPos.x * matRotation[0][1] + axisPos.y * matRotation[1][1] + axisPos.z * matRotation[2][1]);
				matRotation[3][2] = axisPos.z - (axisPos.x * matRotation[0][2] + axisPos.y * matRotation[1][2] + axisPos.z * matRotation[2][2]);
				m_matGizmo = matRotation * m_matGizmo;
			}
			break;

		case GIZMO_SCALE: {
			vec3 scale(1.f);
			if (m_activeAxis < 3) {
				if (getRayAxisIntersectionDelta(rayStart, rayEnd, axisPos, axisDir, 0.f, 0.f, m_delta))
					scale[m_activeAxis] = std::max(1.f + glm::dot(m_delta - m_startDelta, axisDir) / gizmoLength, GIZMO_MIN_SCALE);
			} else if (m_activeAxis == 3) {
				float f = (m_mousePressPos.y() - m_mousePos.y()) * 2.f / GIZMO_ARROW_LENGTH;
				scale = vec3(f >= 0.f ? 1.f + f : 1.f / (1.f - f));
			}
			m_matGizmo = m_matGizmo * glm::scale(glm::mat4(1.f), scale);
			break;
		}

		default:
			return;
	}

	if (m_transformationCommand) {
		m_transformationCommand->setTransformation(m_matGizmo * glm::inverse(m_matGizmoStart));
	} else if (m_type == GIZMO_CAMERA) {
		m_widget.update();
	}
}

// ------------------------------------------------------------------------ //

void Gizmo::rotateCamera(float dx, float dy)
{
	m_scene.camera().rotate(dx * -0.5f, dy * 0.5f);
}

void Gizmo::panCamera(float dx, float dy)
{
	auto delta = vec2(dx, dy) * 2.f * m_widget.aspectScale() * m_gl->getInvViewportSize();
	m_scene.camera().pan(delta.x, delta.y, glm::length2(m_targetNormal) > 0 ? &m_targetPos : nullptr);
}

// ------------------------------------------------------------------------ //

void Gizmo::setLocalMode(bool local)
{
	if (m_localSpace == local)
		return;

	m_localSpace = local;
	updateOnMouseMove();
	m_widget.update();
}

// ------------------------------------------------------------------------ //

void Gizmo::onSelectionChanged()
{
	if (isLocked())
		return;

	m_gizmoFirstAxis = 0;
	m_selection.clear();
	if (m_type == GIZMO_CAMERA) {
		m_nodeSelection = false;
		return;
	}

	auto & selection = m_scene.selection();
	if (selection.size() == 1 && (*selection.begin())->type() == SceneElement_DirectionalLight && m_type == GIZMO_ROTATE) {
		auto node = qobject_cast<DirectionalLight *>(*selection.begin());
		m_gizmoFirstAxis = 1;
		m_selection.push_back(node);
		m_matGizmo = node->parent()->globalTransformation() * glm::rotate(glm::mat4(1.f), glm::radians(node->rotation().get().z), vec3(0.f, 0.f, 1.f));
		glm::setTranslation(m_matGizmo, m_scene.camera().target().get());
		m_matGizmoStart = m_matGizmo;
		m_nodeSelection = true;
		m_scale = m_widget.getScale(glm::translation(m_matGizmo));
		m_widget.update();
		return;
	}

	for (auto elem : selection) {
		if (elem->type() != SceneElement_Geometry) {
			if (auto node = static_cast<Node *>(elem)) {
				if (m_scene.root().hasChild(node)) // skip detached nodes
					m_selection.push_back(node);
			}
		}
	}

	m_nodeSelection = !m_selection.empty();
	if (!m_nodeSelection) {
		m_widget.update();
		return;
	}

	if (m_selection.size() == 1) {
		m_matGizmo = m_selection.front()->globalTransformation();
		//auto & tm = m_matGizmo;
		//bool flipFacing = glm::dot(glm::cross(glm::axisX(tm), glm::axisY(tm)), glm::axisZ(tm)) < 0.f;
		//LogInformation() << QString().asprintf("%p %d", m_selection.front(), flipFacing ? 1 : 0);
	} else {
		BBox bbox;
		bbox.clear();
		for (auto node : m_selection) {
			BBox aabb = node->oobb();
			aabb.transform(node->globalTransformation());
			bbox.addToBounds(aabb);
		}

		m_matGizmo = glm::translate(glm::mat4(1.f), bbox.center());
	}

	m_matGizmoStart = m_matGizmo;
	m_scale = m_widget.getScale(glm::translation(m_matGizmo));
	m_widget.update();
}

void Gizmo::onCameraChanged()
{
	auto & camera = m_scene.camera();
	if (m_type == GIZMO_CAMERA) {
		m_matGizmo = glm::translate(glm::mat4(1.f), camera.target().get());
		m_matGizmoStart = m_matGizmo;
	} else if (m_type == GIZMO_ROTATE && m_selection.size() == 1 && m_selection.front()->type() == SceneElement_DirectionalLight) {
		glm::setTranslation(m_matGizmoStart, camera.target().get());
		glm::setTranslation(m_matGizmo, camera.target().get());
	}

	m_scale = m_widget.getScale(glm::translation(m_matGizmo));
}
