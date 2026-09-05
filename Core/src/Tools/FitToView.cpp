/*
	Copyright (C) 2013-2020 Damir Sagidullin

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
	documentation files (the "Software"), to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions
	of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
	TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.

	(The above is MIT License: http://en.wikipedia.origin/wiki/MIT_License)
*/

#include "FitToView.h"

FitToView::FitToView(Scene & scene)
	: m_scene(scene)
	, m_camera(scene.camera())
	, m_state(scene.camera().getCameraState())
	, m_justSelectedObjects(true, PID_FIT_TO_VIEW_JUST_SELECTED_OBJECTS, *this)
	, m_includingChildrenObjects(true, PID_FIT_TO_VIEW_INCLUDING_CHILDREN_OBJECTS, *this)
	, m_keepAspect(true, PID_FIT_TO_VIEW_KEEP_ASPECT, *this)
	, m_padding(10.f, 0.f, 100.f, 1, PID_FIT_TO_VIEW_PADDING, *this)
	, m_accept(false)
	, m_aspect(scene.camera().aspect().get())
{
	m_justSelectedObjects.setEnabled(!scene.selection().empty());
	m_includingChildrenObjects.setEnabled(m_justSelectedObjects.isEnabled() && m_justSelectedObjects.get());
	doIt();
}

FitToView::~FitToView()
{
	if (!m_accept)
		m_camera.setState(m_state);
}

// ------------------------------------------------------------------------ //

namespace // some internal stuff inside
{

typedef std::vector<vec3>	Vec3Array;

void findVertices(Vec3Array & vertices, Geometry * geom)
{
	auto & matTransform = geom->parent()->globalTransformation();
	auto & geomVertices = geom->vertices();
	Vec3Array tmp;
	tmp.reserve(geomVertices.size());
	for (const auto & v : geomVertices)
		tmp.emplace_back(glm::transformCoord(v.pos, matTransform));

	vertices.insert(vertices.end(), tmp.begin(), tmp.end());
}

void findVerticesRecursive(Vec3Array & vertices, ISceneElement * elem)
{
	if (elem->type() == SceneElement_Geometry) {
		findVertices(vertices, static_cast<Geometry *>(elem));
		return;
	}

	if (!static_cast<Node *>(elem)->visible().get())
		return;

	for (size_t i = 0; i < elem->numChildren(SceneElement_All); i++)
		findVerticesRecursive(vertices, elem->child(i, SceneElement_All));
}

void findGeometriesRecursive(std::set<Geometry *> & geometries, ISceneElement * elem)
{
	if (elem->type() == SceneElement_Geometry) {
		geometries.insert(static_cast<Geometry *>(elem));
		return;
	}

	if (!static_cast<Node *>(elem)->visible().get())
		return;

	for (size_t i = 0; i < elem->numChildren(SceneElement_All); i++)
		findGeometriesRecursive(geometries, elem->child(i, SceneElement_All));
}

struct VerticesBlock {
	const vec3 * begin;
	const vec3 * end;
	const mat4 * matViewProj;
	RectF rect;
	volatile bool * result;

	static void processIsInside(VerticesBlock &block)
	{
		for (auto p = block.begin; p != block.end; p++) {
			if (!block.result) break;

			auto v = *block.matViewProj * vec4(*p, 1.f);
			auto sp = vec2(v.x, v.y) / v.w;
			if (sp.x < -1.f || sp.x > 1.f || sp.y < -1.f || sp.y > 1.f) {
				*block.result = false;
				break;
			}
		}
	}

	static void processGetRect(VerticesBlock &block)
	{
		for (auto p = block.begin; p != block.end; p++) {
			auto v = *block.matViewProj * vec4(*p, 1.f);
			assert(v.w > 0.f);
			auto sp = vec2(v.x, v.y) / v.w;
			block.rect.left = std::min<float>(block.rect.left, sp.x);
			block.rect.right = std::max<float>(block.rect.right, sp.x);
			block.rect.top = std::min<float>(block.rect.top, sp.y);
			block.rect.bottom = std::max<float>(block.rect.bottom, sp.y);
		}
	}
};

std::vector<VerticesBlock> makeBlocks(const Vec3Array &vertices, const mat4 &matViewProj, size_t blockSize)
{
	std::vector<VerticesBlock> vertexBlocks;
	for (size_t i = 0; i < vertices.size(); i += blockSize) {
		vertexBlocks.emplace_back(VerticesBlock{
			&vertices[i],
			i + blockSize < vertices.size() ? &vertices[i + blockSize] : &vertices[0] + vertices.size(),
			&matViewProj,
			RectF(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX)
		});
	}

	return vertexBlocks;
}

RectF getVertsRect(const Vec3Array & vertices, const mat4 &matCamera, const mat4 &matProj)
{
	auto vertexBlocks = makeBlocks(vertices, matProj * glm::inverse(matCamera), 0x10000);

	if (vertexBlocks.size() == 1)
		VerticesBlock::processGetRect(vertexBlocks[0]);
	else
		QtConcurrent::blockingMap(vertexBlocks, VerticesBlock::processGetRect);

	RectF rc(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (auto & vb : vertexBlocks) {
		rc.left = std::min<float>(rc.left, vb.rect.left);
		rc.right = std::max<float>(rc.right, vb.rect.right);
		rc.top = std::min<float>(rc.top, vb.rect.top);
		rc.bottom = std::max<float>(rc.bottom, vb.rect.bottom);
	}

	return rc;
}

bool isVertsInsideCamera(const Vec3Array & vertices, const mat4 &matCamera, const mat4 &matProj)
{
	auto vertexBlocks = makeBlocks(vertices, matProj * glm::inverse(matCamera), 0x10000);

	bool isInside = true;
	for (auto & vb : vertexBlocks)
		vb.result = &isInside;

	if (vertexBlocks.size() == 1)
		VerticesBlock::processIsInside(vertexBlocks[0]);
	else
		QtConcurrent::blockingMap(vertexBlocks, VerticesBlock::processIsInside);

	return isInside;
}

void correctCameraDistance(int numSteps, const Vec3Array & vertices, Camera &camera)
{
	assert(camera.projection().getIndex() != CP_ORTHOGRAPHIC);
	const auto matProj = camera.projMatrix();
	while (!isVertsInsideCamera(vertices, camera.matrix(), matProj)) {
		auto distance = camera.distance().get() * 2.f;
		camera.distance()._set(distance);
		camera.updateCameraMatrix();

		if (camera.distance().get() < distance)
			break; // we're too far and can't move further
	}

	auto d1 = 0.f;
	auto d2 = camera.distance().get();
	for (int i = 0; i < numSteps; i++) {
		auto d = (d1 + d2) * 0.5f;
		camera.distance()._set(d);
		camera.updateCameraMatrix();
		if (isVertsInsideCamera(vertices, camera.matrix(), matProj))
			d2 = d;
		else
			d1 = d;
	}
}

void correctCameraPosition(int numSteps, const Vec3Array & vertices, Camera &camera)
{
	assert(camera.projection().getIndex() != CP_ORTHOGRAPHIC);
	const auto matCamera = camera.matrix();
	const auto matProj = camera.projMatrix();
	auto dist = camera.distance().get();
	for (int i = 0; i < numSteps; i++) {
		auto rc = getVertsRect(vertices, camera.matrix(), matProj);
		auto dx = dist * (rc.left + rc.right) * 0.1f / matProj[0][0];
		auto dy = dist * (rc.top + rc.bottom) * 0.1f / matProj[1][1];
		camera.target()._set(camera.target().get() + glm::axisX(matCamera) * dx + glm::axisY(matCamera) * dy);
		camera.updateCameraMatrix();
	}
}

} // namespace

// ------------------------------------------------------------------------ //

void FitToView::accept()
{
	m_accept = true;
	cameraChangeState(m_camera, m_state, m_camera.getCameraState());
}

void FitToView::doIt()
{
	enum {NUM_ITERATIONS = 4, NUM_STEPS = 10};

	Vec3Array vertices;
	if (m_justSelectedObjects.get() && m_justSelectedObjects.isEnabled()) {
		if (m_includingChildrenObjects.get()) {
			std::set<Geometry *> geometries;
			for (auto elem : m_scene.selection())
				findGeometriesRecursive(geometries, elem);

			for (auto geom : geometries)
				findVertices(vertices, geom);
		} else {
			for (auto elem : m_scene.selection()) {
				if (elem->type() == SceneElement_Geometry)
					findVertices(vertices, static_cast<Geometry *>(elem));
			}

		}
	} else
		findVerticesRecursive(vertices, &m_scene.root());

	if (vertices.empty())
		return;

	BBox bbox;
	bbox.clear();
	for (auto it = vertices.begin(); it != vertices.end(); ++it)
		bbox.addToBounds(*it);

	if (bbox.isNull())
		return;

	m_camera.target()._set(bbox.center());
	m_camera.distance()._set(glm::length(bbox.size()));
	m_camera.updateCameraMatrix();

	m_camera.aspect()._set(m_keepAspect.get() ? m_aspect : 1.f);
	m_camera.updateProjMatrix();

	if (m_camera.projection().getIndex() == CP_ORTHOGRAPHIC) {
		auto matCamera = m_camera.matrix();
		auto matProj = m_camera.projMatrix();
		auto rc = getVertsRect(vertices, matCamera, matProj);
		auto dx = (rc.left + rc.right) * 0.5f / matProj[0][0];
		auto dy = (rc.top + rc.bottom) * 0.5f / matProj[1][1];
		auto sx = (rc.right - rc.left) * 0.5f;
		auto sy = (rc.bottom - rc.top) * 0.5f;
		m_distance = m_camera.distance().get();

		m_camera.target()._set(m_camera.target().get() + glm::axisX(matCamera) * dx + glm::axisY(matCamera) * dy);

		if (!m_keepAspect.get()) { // correct camera aspect
			auto aspect = m_camera.aspect().value() * sx / sy;
			m_camera.aspect()._set(aspect);
			m_distance *= sy;
		} else
			m_distance *= std::max(sx, sy);

		m_camera.distance()._set(m_distance * (1.f + m_padding.get() * 0.01f));

		m_camera.updateCameraMatrix();
		m_camera.updateProjMatrix();
	} else {
		for (size_t j = 0; j < NUM_ITERATIONS; j++) {
			correctCameraDistance(NUM_STEPS, vertices, m_camera);
			correctCameraPosition(NUM_STEPS, vertices, m_camera);
			if (!m_keepAspect.get()) { // correct camera aspect
				auto rc = getVertsRect(vertices, m_camera.matrix(), m_camera.projMatrix());
				auto aspect = m_camera.aspect().value() * rc.width() / rc.height();
				m_camera.aspect()._set(aspect);
				m_camera.updateProjMatrix();
			}
		}

		correctCameraDistance(NUM_STEPS, vertices, m_camera);
		m_distance = m_camera.distance().get();

		if (m_padding.get() > 0.f) {
			m_camera.distance()._set(m_distance * (1.f + m_padding.get() * 0.01f));
			m_camera.updateCameraMatrix();
		}
	}

	m_camera.focusDistance()._set(m_camera.distance().get());

	auto minFarZ = m_distance * DEFAULT_ZFAR_CAM_DIST_RATIO;
	if (m_camera.farZ().get() < minFarZ) {
		m_camera.farZ()._set(minFarZ);
		if (m_camera.nearZ().get() < minFarZ * MIN_ZNEAR_ZFAR_RATIO)
			m_camera.nearZ()._set(minFarZ * MIN_ZNEAR_ZFAR_RATIO);

		m_camera.updateProjMatrix();
	}

	m_camera.updateMatrices();
}

// ------------------------------------------------------------------------ //

CoreInstance & FitToView::core() const
{
	return m_camera.core();
}

void FitToView::pushCommand(QUndoCommand *cmd)
{
	cmd->redo();
	delete cmd;
}

void FitToView::fireChanged(eParamId paramId)
{
	switch (paramId) {
		case PID_FIT_TO_VIEW_JUST_SELECTED_OBJECTS:
		case PID_FIT_TO_VIEW_INCLUDING_CHILDREN_OBJECTS:
		case PID_FIT_TO_VIEW_KEEP_ASPECT:
			m_includingChildrenObjects.setEnabled(m_justSelectedObjects.get());
			doIt();
			break;
		case PID_FIT_TO_VIEW_PADDING:
			m_camera.distance()._set(m_distance * (1.f + m_padding.get() * 0.01f));
			m_camera.focusDistance()._set(m_camera.distance().get());
			m_camera.updateCameraMatrix();
			m_camera.updateProjMatrix();
			m_camera.updateMatrices();
			break;
	}
}
