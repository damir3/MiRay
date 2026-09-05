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

#pragma once

#include "Geometry.h"

enum eRayMask {
	RAY_MASK_GEOMETRY		= (1 << 0),
	RAY_MASK_LIGHT			= (1 << 1),
	RAY_MASK_SELECTION		= (1 << 2),
};

struct Ray final : public RTCRayHit {
	vec3  hitPoint;
	vec3  geomNormal;
	vec3  normal;

	uint32_t vi[3];
	const Vertex *verts[3];
	const Node * node;
	const Geometry * geom;
	const class TextureImpl * normalMap;
	void * boundaryStack;

	vec3  dpdx; // tangent
	vec3  dpdy; // binormal
	vec3  dpdz; // normal
	vec2  bumpTC;
	float bumpZ;
	float bumpDepth;

	bool  hitBack;
	bool  hitBoth;

	void init(const vec3 & origin, const vec3 & direction, float length)
	{
		setOrigin(origin);
		setDirection(direction);

		ray.tnear = 0.f;
		ray.tfar = length;
		ray.mask = 0xFFFFFFFF;
		ray.time = 0.f;

		resetHit();

		boundaryStack = nullptr;
		normalMap = nullptr;
		hitBoth = false;
	}

	void init(const vec3 & origin, const vec3 & destination)
	{
		auto direction = destination - origin;
		auto length = glm::length(direction);
		init(origin, direction * (1.f / length), length);
	}

	inline const vec3 & origin() const { return *(vec3 *)&ray.org_x; }
	void setOrigin(const vec3 & origin)
	{
		ray.org_x = origin.x;
		ray.org_y = origin.y;
		ray.org_z = origin.z;
	}

	inline const vec3 & direction() const { return *(vec3 *)&ray.dir_x; }
	void setDirection(const vec3 & ndir)
	{
		ray.dir_x = ndir.x;
		ray.dir_y = ndir.y;
		ray.dir_z = ndir.z;
	}

	void resetHit()
	{
		hit.geomID = RTC_INVALID_GEOMETRY_ID;
		hit.primID = RTC_INVALID_GEOMETRY_ID;
		geom = nullptr;
	}

	inline const vec3 & Ng() const { return *(vec3 *)&hit.Ng_x; }

	inline vec3 target(float t) const
	{
		return origin() + direction() * t;
	}

	void updateLightIntersection()
	{
		hitPoint = target(ray.tfar);
		normal = geomNormal = Ng();
	}

	void updateGeometryIntersection();
	void updateGeometryIntersectionNormal();

	vec3 dpdu() const
	{
		return glm::transformNormal(verts[1]->pos - verts[0]->pos, node->globalTransformation());
	}

	vec3 dpdv() const
	{
		 return glm::transformNormal(verts[2]->pos - verts[0]->pos, node->globalTransformation());
	}

//	void GetTangentBinormal(vec3 & tangent, vec3 & binormal, float bumpDepth) const
//	{
//		auto f = dTC1.x * dTC2.y - dTC1.y * dTC2.x;
//		f *= bumpDepth;
//
//		// tangent
//		tangent = (dpdu * dTC2.y - dpdv * dTC1.y);
//		auto tl2 = glm::length2(tangent);
//		if (tl2 > 0.f) {
//			tangent *= f / tl2;
//			tangent -= normal * glm::dot(tangent, normal);
//		}
////		assert(isfinite(tangent.x) && isfinite(tangent.y) && isfinite(tangent.z));
//
//		// binormal
//		binormal = (dpdv * dTC1.x - dpdu * dTC2.x);
//		auto bl2 = glm::length2(binormal);
//		if (bl2 > 0.f) {
//			binormal *= f / bl2;
//			binormal -= normal * glm::dot(binormal, normal);
//		}
////		assert(isfinite(binormal.x) && isfinite(binormal.y) && isfinite(binormal.z));
//
////		// tangent
////		tangent = dpdu * (bumpDepth / glm::length2(dpdu));
////		tangent -= normal * glm::dot(tangent, normal);
////
////		// binormal
////		binormal = dpdv * (bumpDepth / glm::length2(dpdv));
////		binormal -= normal * glm::dot(binormal, normal);
//	}

	void setBump(const TextureImpl * normapMap, float bumpDepth);

	vec3 tangentDir(int index) const
	{
		vec3 tangent;
		const auto & uvSet = geom->uvSet(index);
		if (!uvSet.empty()) {
			auto & tc0 = uvSet[vi[0]];
			auto dTC1 = uvSet[vi[1]] - tc0;
			auto dTC2 = uvSet[vi[2]] - tc0;
			if (dTC2.y != 0.f || dTC1.y != 0.f) {
				tangent = dpdu() * dTC2.y - dpdv() * dTC1.y;
				if ((dTC1.x * dTC2.y - dTC1.y * dTC2.x) < 0.f)
					tangent = -tangent;
			} else // bad texture coordinates
				tangent = dpdu();
		} else // no texture coordinates
			tangent = dpdu();

		tangent -= normal * glm::dot(tangent, normal);
		tangent = glm::normalize(tangent);

		return tangent;
	}

	inline vec2 textureCoords(int index) const
	{
		const auto & uvSet = geom->uvSet(index);
		if (uvSet.empty())
			return vec2(hit.u, hit.v);

		auto & tc0 = uvSet[vi[0]];
		auto dTC1 = uvSet[vi[1]] - tc0;
		auto dTC2 = uvSet[vi[2]] - tc0;
		return tc0 + dTC1 * hit.u + dTC2 * hit.v;
	}
};
