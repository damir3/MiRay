//
//  kernel.cl
//  MiRay/rt
//
//  Created by Damir Sagidullin on 14.06.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

typedef struct
{
	float3	center;
	float3	extents;
	float4	plane;
	uint	childs[2];
	uint	beginTriangle;
	uint	endTriangle;
} Node;

typedef struct
{
	float3	pos[3];
	float3	normal[3];
	float2	tc[3];
	uint	material;
} Triangle;

// ------------------------------------------------------------------------ //

inline bool TestRayBoxIntersection(float3 rayCenter, float3 rayExtents, float3 rayHalfDir, float3 boxCenter, float3 boxExtents)
{
	float3 delta = rayCenter - boxCenter;
	float3 absDelta = fabs(delta);
	float3 extents = boxExtents + rayExtents;

	if (any(isgreater(absDelta, extents)))
		return false;

	// faster on GPU
	float3 v1 = fabs(cross(rayHalfDir, delta));
	float3 v2 = boxExtents.yzx * rayExtents.zxy + boxExtents.zxy * rayExtents.yzx;
	return all(isless(v1, v2));

//	// faster on CPU
//	if (fabs((rayHalfDir.y * delta.z) - (rayHalfDir.z * delta.y)) > (boxExtents.y * rayExtents.z + boxExtents.z * rayExtents.y))
//		return false;
//	
//	if (fabs((rayHalfDir.z * delta.x) - (rayHalfDir.x * delta.z)) > (boxExtents.z * rayExtents.x + boxExtents.x * rayExtents.z))
//		return false;
//	
//	if (fabs((rayHalfDir.x * delta.y) - (rayHalfDir.y * delta.x)) > (boxExtents.x * rayExtents.y + boxExtents.y * rayExtents.x))
//		return false;
//
//	return true;
}

// ------------------------------------------------------------------------ //

inline bool TraceRay(float3 rayStart, float3 rayDirection, const __global float3 * pos, float3 * res)
{
	float3 v0 = pos[0];
	float3 edgeU = pos[1] - v0;
	float3 edgeV = pos[2] - v0;
	float3 h = cross(rayDirection, edgeV);
	float det = dot(edgeU, h);
	if (det != 0.0f)
	{
		float invDet = 1.0f / det;
		float3 s = rayStart - v0;
		float u = invDet * dot(s, h);
		if (u >= 0.0f && u <= 1.0f)
		{
			float3 q = cross(s, edgeU);
			float v = invDet * dot(rayDirection, q);
			if (v >= 0.0f && u + v <= 1.0f)
			{
				float t = invDet * dot(edgeV, q); // ray intersection
				if ((*res).z > t && t >= 0.0f)
				{
					*res = (float3)(u, v, t);
					return true;
				}
			}
		}
	}
	return false;
}

// ------------------------------------------------------------------------ //

__kernel void MainKernel(__global float4 * result,
						 const __global Triangle * triangles,
						 const __global Node * nodes,
						 const __global uint * nodeTriangles,
						 const uint4 size,
						 const float3 camPosition,
						 const float3 camDirectionX,
						 const float3 camDirectionY,
						 const float3 camDirectionZ,
						 const float2 invSize,
						 const float fFrameBlend)
{
	uint tx = get_global_id(0);
	uint ty = get_global_id(1);
//	uint sx = get_global_size(0);
//	uint sy = get_global_size(1);

	if ((tx < size.x) && (ty < size.y))
	{
		int index = ty * size.x + tx;
		float2 p = (float2)(tx, ty) * invSize - (float2)1.0f;
		float3 rayStart = camPosition;
		float3 rayDirection = camDirectionZ + camDirectionX * p.x - camDirectionY * p.y;
		float3 rayHalfDir = rayDirection * 0.5f;
		float3 rayExtents = fabs(rayHalfDir);
		float3 rayCenter = rayStart + rayHalfDir;
		float3 res = (float3)1.0f;
		uint cti = UINT_MAX;

//		for (uint i = 0; i < size.z; i++)
//		{
////			if (TraceRay(rayStart, rayDirection, triangles[i].pos, &res))
////				cti = i;
//
//			const __global Triangle * tri = triangles + i;
//			float3 v0 = tri->pos[0];
//			float3 edgeU = tri->pos[1] - v0;
//			float3 edgeV = tri->pos[2] - v0;
//			float3 h = cross(rayDirection, edgeV);
//			float det = dot(edgeU, h);
//			if (det != 0.0f)
//			{
//				float invDet = 1.0f / det;
//				float3 s = rayStart - v0;
//				float u = invDet * dot(s, h);
//				if (u >= 0.0f && u <= 1.0f)
//				{
//					float3 q = cross(s, edgeU);
//					float v = invDet * dot(rayDirection, q);
//					if (v >= 0.0f && u + v <= 1.0f)
//					{
//						float t = invDet * dot(edgeV, q); // ray intersection
//						if (res.z > t && t >= 0.0f)
//						{
//							res = (float3)(u, v, t);
//							cti = i;
//
////							rayHalfDir = rayDirection * t;
////							rayExtents = fabs(rayHalfDir);
////							rayCenter = rayStart + rayHalfDir;
//						}
//					}
//				}
//			}
//		}

		float4 color = 0.0f;

		uint stackPos = 0;
		uint stack[64];
		stack[stackPos++] = 0;
		while (stackPos > 0)
		{
			const __global Node * node = nodes + stack[--stackPos];
			if (!TestRayBoxIntersection(rayCenter, rayExtents, rayHalfDir, node->center, node->extents))
				continue;

//			color.xyz += 0.01f;

			if (node->childs[0] != 0)
			{
				uint ni = isless(dot(rayStart, node->plane.xyz), node->plane.w);
				stack[stackPos++] = node->childs[ni];
				stack[stackPos++] = node->childs[1 - ni];
			}
			else
			{
				for (uint ti = node->beginTriangle; ti < node->endTriangle; ti++)
				{
					uint i = nodeTriangles[ti];
					if (TraceRay(rayStart, rayDirection, triangles[i].pos, &res))
					{
						cti = i;

//						rayHalfDir = rayDirection * res.z;
//						rayExtents = fabs(rayHalfDir);
//						rayCenter = rayStart + rayHalfDir;
					}
//					const __global Triangle * tri = triangles + i;
//					float3 v0 = tri->pos[0];
//					float3 edgeU = tri->pos[1] - v0;
//					float3 edgeV = tri->pos[2] - v0;
//					float3 h = cross(rayDirection, edgeV);
//					float det = dot(edgeU, h);
//					if (det != 0.0f)
//					{
//						float invDet = 1.0f / det;
//						float3 s = rayStart - v0;
//						float u = invDet * dot(s, h);
//						if (u >= 0.0f && u <= 1.0f)
//						{
//							float3 q = cross(s, edgeU);
//							float v = invDet * dot(rayDirection, q);
//							if (v >= 0.0f && u + v <= 1.0f)
//							{
//								float t = invDet * dot(edgeV, q); // ray intersection
//								if (res.z > t && t >= 0.0f)
//								{
//									res = (float3)(u, v, t);
//									cti = i;
//
////									rayHalfDir = rayDirection * t;
////									rayExtents = fabs(rayHalfDir);
////									rayCenter = rayStart + rayHalfDir;
//								}
//							}
//						}
//					}
				}

				if (res.z < 1.0)
				{
					float3 mins = node->center - node->extents;
					float3 maxs = node->center + node->extents;
					float3 rayEnd = rayStart + rayDirection * res.z;
					if (mins.x <= rayEnd.x && maxs.x >= rayEnd.x &&
						mins.y <= rayEnd.y && maxs.y >= rayEnd.y &&
						mins.z <= rayEnd.z && maxs.z >= rayEnd.z)
						stackPos = 0;
				}
			}
		}
//		const __global Node * node = nodes + nodes[0].childs[0];
//		if (TestRayBoxIntersection(rayCenter, rayExtents, rayHalfDir, node->center, node->extents))
//			color.xyz += 0.2f;

//		for (float r = 1.f; r <= 10.f; r += 0.01f)
//		{
//			if (!TestRayBoxIntersection(rayCenter, rayExtents, rayHalfDir, (float3)(0.0f, 0.0f, 5.0f), (float3)(r, r, r)))
//				color.xyz = r * 0.1f;
//		}

		if (cti != UINT_MAX)
		{
			const __global Triangle * tri = triangles + cti;
			float3 normal = normalize(tri->normal[0] * (1.0f - res.x - res.y) + tri->normal[1] * res.x + tri->normal[2] * res.y);
//			float3 lightDir = normalize((float3)(-1.0, -2.0, -3.0));
//			float f = dot(normal, lightDir) * 0.5f + 0.5f;
//			color = f;
			color.xyz += normal.xyz * (float3)0.5f + (float3)0.5f;
//			color.x = u;
//			color.y = v;
//			color.z = t;
		}

		result[index] = mix(result[index], color, fFrameBlend);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
