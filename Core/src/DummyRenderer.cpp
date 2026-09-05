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

#include "DummyRenderer.h"
#include "MaterialImpl.h"
#include "BSDF.h"

// ------------------------------------------------------------------------ //

DummyRenderer::DummyRenderer(Scene & scene)
	: BaseRenderer(scene)
{
	m_maxPathLength = 8;
	m_russianRouletteStart = 0;
	m_numBumpLinearSteps = 10;
	m_numBumpBinarySteps = 3;
}

DummyRenderer::~DummyRenderer()
{
}

// ------------------------------------------------------------------------ //

void DummyRenderer::traceCameraPath(TraceResult & res, TraceContext & ctx)
{
	for (uint pathLength = 0; pathLength < 4; ++pathLength) {
		intersect(ctx.ray);

		if (ctx.ray.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
			if (m_bgMode == BackgroundMode_Environment)
				res.color += m_environmentLight.getRadiance(ctx.ray);
			else
				res.opacity = vec3(0.f);
			break;
		}

		if (ctx.ray.primID == RTC_INVALID_GEOMETRY_ID) {
			auto lightNode = LightNode::GetByRay(m_rtcScene, ctx.ray);
			assert(lightNode);

			ctx.ray.updateIntersectionPosition();

			res.color += lightNode->getRadiance(ctx.ray);
			break;
		}

		auto node = MeshNode::getByRay(m_rtcScene, ctx.ray);
		assert(node);
		ctx.ray.updateIntersectionPosition(node);

		auto geom = node->getGeometryByRay(ctx.ray);
		assert(geom);
		ctx.ray.updateIntersectionNormal(node, geom);

		auto material = geom->material();
		assert(material);

		res.color += material->getPreviewColor(ctx.ray);
		break;
	}
}
