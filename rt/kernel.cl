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
	float3 v1 = fabs(cross(rayHalfDir, rayCenter - boxCenter));
	float3 v2 = boxExtents.yzx * rayExtents.zxy + boxExtents.zxy * rayExtents.yzx;
	return all(isless(v1, v2));
}

// ------------------------------------------------------------------------ //

inline bool TraceRay(float3 rayOrigin, float3 rayDirection, const __global float3 * pos, float3 * res)
{
	float3 h = cross(rayDirection, pos[2]);
	float det = dot(pos[1], h);
	
	if (det > 0.f)
	{// back facing triangle
		float3 s = rayOrigin - pos[0];
		float u = dot(s, h);
		if (u >= 0.0f && u <= det)
		{
			float3 q = cross(s, pos[1]);
			float v = dot(rayDirection, q);
			if (v >= 0.0f && u + v <= det)
			{
				float t = dot(pos[2], q); // ray intersection
				if ((*res).z * det > t && t >= 0.0f)
				{
					*res = (float3)(u, v, t) / det;
					return true;
				}
			}
		}
	}

	if (det < 0.f)
	{// front facing triangle
		float3 s = rayOrigin - pos[0];
		float u = dot(s, h);
		if (u <= 0.0f && u >= det)
		{
			float3 q = cross(s, pos[1]);
			float v = dot(rayDirection, q);
			if (v <= 0.0f && u + v >= det)
			{
				float t = dot(pos[2], q); // ray intersection
				if ((*res).z * det < t && t <= 0.0f)
				{
					*res = (float3)(u, v, t) / det;
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
		float3 rayOrigin = camPosition;
		float3 rayDirection = camDirectionZ + camDirectionX * p.x - camDirectionY * p.y;
		float3 rayHalfDir = rayDirection * 0.5f;
		float3 rayExtents = fabs(rayHalfDir);
		float3 rayCenter = rayOrigin + rayHalfDir;
		float3 res = (float3)1.0f;
		uint cti = UINT_MAX;

//		for (uint i = 0; i < size.z; i++)
//		{
//			if (TraceRay(rayOrigin, rayDirection, triangles[i].pos, &res))
//				cti = i;
//		}

		float4 color = 0.0f;

		uint stack[64];
		uint stackPos = 0;
		stack[stackPos++] = 0;
		while (stackPos > 0)
		{
			const __global Node * node = nodes + stack[--stackPos];
			if (!TestRayBoxIntersection(rayCenter, rayExtents, rayHalfDir, node->center, node->extents))
				continue;

//			color.xyz += 0.01f;

			if (node->childs[0] != 0)
			{
				uint ni = isless(dot(rayOrigin, node->plane.xyz), node->plane.w);
				stack[stackPos++] = node->childs[ni];
				stack[stackPos++] = node->childs[1 - ni];
			}
			else
			{
				for (uint ti = node->beginTriangle; ti < node->endTriangle; ti++)
				{
					uint i = nodeTriangles[ti];
					if (TraceRay(rayOrigin, rayDirection, triangles[i].pos, &res))
					{
						cti = i;
					}
				}

				if (res.z < 1.0)
				{
					float3 rayEnd = mad(rayDirection, res.z, rayOrigin);
					if (all(isgreaterequal(rayEnd, node->center - node->extents)) && all(islessequal(rayEnd, node->center + node->extents)))
						stackPos = 0; // break;
				}
			}
		}
//		const __global Node * node = nodes + nodes[0].childs[0];
//		if (TestRayBoxIntersection(rayCenter, rayExtents, rayHalfDir, node->center, node->extents))
//			color.xyz += 0.2f;

		if (cti != UINT_MAX)
		{
			const __global Triangle * tri = triangles + cti;
			float3 normal = normalize(tri->normal[0] + tri->normal[1] * res.x + tri->normal[2] * res.y);
//			float3 lightDir = normalize((float3)(-1.0, -2.0, -3.0));
//			float f = dot(normal, lightDir) * 0.5f + 0.5f;
//			color = f;
			color.xyz += normal.xyz * (float3)0.5f + (float3)0.5f;
//			color.x = u;
//			color.y = v;
//			color.z = t;
		}

//		// ray box intersection test
//		for (float r = 1.f; r <= 10.f; r += 0.01f)
//		{
//			if (!TestRayBoxIntersection(rayCenter, rayExtents, rayHalfDir, (float3)(0.0f, 0.0f, 5.0f), (float3)(r, r, r)))
//				color.xyz = r * 0.1f;
//		}

		result[index] = mix(result[index], color, fFrameBlend);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
