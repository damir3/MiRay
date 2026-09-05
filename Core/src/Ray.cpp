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

#include "Ray.h"
#include "MeshNode.h"
#include "TextureImpl.h"

void Ray::updateGeometryIntersectionNormal()
{
	node = geom->meshNode();

	const auto viSrc = geom->indices().data() + hit.primID * 3;
	vi[0] = viSrc[0];
	vi[1] = viSrc[1];
	vi[2] = viSrc[2];

	const auto & geomVertices = geom->vertices();
	verts[0] = &geomVertices[vi[0]];
	verts[1] = &geomVertices[vi[1]];
	verts[2] = &geomVertices[vi[2]];

	normal = verts[0]->normal + (verts[1]->normal - verts[0]->normal) * hit.u + (verts[2]->normal - verts[0]->normal) * hit.v;
	normal = glm::normalize(glm::ttransformNormal(normal, node->inverseGlobalTransformation()));
}

void Ray::updateGeometryIntersection()
{
	updateGeometryIntersectionNormal();

	hitPoint = target(ray.tfar);
	geomNormal = glm::normalize(glm::ttransformNormal(Ng(), node->inverseGlobalTransformation()));

	normalMap = nullptr;
}

void Ray::setBump(const TextureImpl * normapMap_, float bumpDepth)
{
	normalMap = normapMap_;
	bumpTC = normalMap->getTC(*this);
	normalMap->getTangentBinormal(dpdx, dpdy, *this, bumpDepth);
	dpdz = normal;
}
