#pragma once

namespace mr
{

template<typename T>
struct TBBox
{
	TVec3<T>	vMins;
	TVec3<T>	vMaxs;

	TBBox() {}
	TBBox(const TBBox<double> & box) : vMins(box.vMins), vMaxs(box.vMaxs) {}
	TBBox(const TBBox<float> & box) : vMins(box.vMins), vMaxs(box.vMaxs) {}
	TBBox(const TVec3<T> & mins, const TVec3<T> & maxs) : vMins(mins), vMaxs(maxs) {}

	//const TBBox & operator = (const TBBox<double> & bbox)
	//{
	//	vMins = bbox.vMins;
	//	vMaxs = bbox.vMaxs;
	//	return *this;
	//}

	//const TBBox & operator = (const TBBox<float> & bbox)
	//{
	//	vMins = bbox.vMins;
	//	vMaxs = bbox.vMaxs;
	//	return *this;
	//}

	void ClearBounds()
	{
		vMins.x = vMins.y = vMins.z = FLT_MAX;
		vMaxs.x = vMaxs.y = vMaxs.z = -FLT_MAX;
	}

	void AddToBounds(const TVec3<T> & v)
	{
		if (vMins.x > v.x) vMins.x = v.x;
		if (vMaxs.x < v.x) vMaxs.x = v.x;
		if (vMins.y > v.y) vMins.y = v.y;
		if (vMaxs.y < v.y) vMaxs.y = v.y;
		if (vMins.z > v.z) vMins.z = v.z;
		if (vMaxs.z < v.z) vMaxs.z = v.z;
	}

	void AddToBounds(const TBBox & box)
	{
		if (vMins.x > box.vMins.x) vMins.x = box.vMins.x;
		if (vMaxs.x < box.vMaxs.x) vMaxs.x = box.vMaxs.x;
		if (vMins.y > box.vMins.y) vMins.y = box.vMins.y;
		if (vMaxs.y < box.vMaxs.y) vMaxs.y = box.vMaxs.y;
		if (vMins.z > box.vMins.z) vMins.z = box.vMins.z;
		if (vMaxs.z < box.vMaxs.z) vMaxs.z = box.vMaxs.z;
	}

	bool IsNull() const
	{
		return vMins.x > vMaxs.x || vMins.y > vMaxs.y || vMins.z > vMaxs.z;
	}

	bool IsEmpty() const
	{
		return vMins.x >= vMaxs.x || vMins.y >= vMaxs.y || vMins.z >= vMaxs.z;
	}

	TVec3<T> Center() const
	{
		return (vMins + vMaxs) * (T)0.5;
	}

	TVec3<T> Size() const
	{
		if (vMins.x > vMaxs.x || vMins.y > vMaxs.y || vMins.z > vMaxs.z)
			return Vec3::Null;

		return vMaxs - vMins;
	}

	T DX() const {return vMaxs.x - vMins.x;}
	T DY() const {return vMaxs.y - vMins.y;}
	T DZ() const {return vMaxs.z - vMins.z;}

	T Area() const
	{
		TVec3<T> vSize = Size();
		return (vSize.x * vSize.y + vSize.y * vSize.z + vSize.z * vSize.x) * (T)2.0;
	}

	void GetVertices(TVec3<T> out[8]) const
	{
		out[0].x = out[2].x = out[4].x = out[6].x = vMins.x;
		out[1].x = out[3].x = out[5].x = out[7].x = vMaxs.x;
		out[0].y = out[1].y = out[4].y = out[5].y = vMins.y;
		out[2].y = out[3].y = out[6].y = out[7].y = vMaxs.y;
		out[0].z = out[1].z = out[2].z = out[3].z = vMins.z;
		out[4].z = out[5].z = out[6].z = out[7].z = vMaxs.z;
	}

	bool Test(const TBBox & box) const
	{
		if (box.vMaxs.x < vMins.x) return false;
		if (box.vMins.x > vMaxs.x) return false;
		if (box.vMaxs.y < vMins.y) return false;
		if (box.vMins.y > vMaxs.y) return false;
		if (box.vMaxs.z < vMins.z) return false;
		if (box.vMins.z > vMaxs.z) return false;
		return true;
	}
	bool Test(const TVec3<T> & v) const
	{
		if (v.x < vMins.x) return false;
		if (v.x > vMaxs.x) return false;
		if (v.y < vMins.y) return false;
		if (v.y > vMaxs.y) return false;
		if (v.z < vMins.z) return false;
		if (v.z > vMaxs.z) return false;
		return true;
	}

	void Intersect(const TBBox & bb1, const TBBox & bb2)
	{
		vMins.x = std::max(bb1.vMins.x, bb2.vMins.x);
		vMins.y = std::max(bb1.vMins.y, bb2.vMins.y);
		vMins.z = std::max(bb1.vMins.z, bb2.vMins.z);
		vMaxs.x = std::min(bb1.vMaxs.x, bb2.vMaxs.x);
		vMaxs.y = std::min(bb1.vMaxs.y, bb2.vMaxs.y);
		vMaxs.z = std::min(bb1.vMaxs.z, bb2.vMaxs.z);
	}
	void Union(const TBBox & bb1, const TBBox & bb2)
	{
		vMins.x = std::min(bb1.vMins.x, bb2.vMins.x);
		vMins.y = std::min(bb1.vMins.y, bb2.vMins.y);
		vMins.z = std::min(bb1.vMins.z, bb2.vMins.z);
		vMaxs.x = std::max(bb1.vMaxs.x, bb2.vMaxs.x);
		vMaxs.y = std::max(bb1.vMaxs.y, bb2.vMaxs.y);
		vMaxs.z = std::max(bb1.vMaxs.z, bb2.vMaxs.z);
	}
	
	void Transform(const Matrix & mat)
	{
		Vec3 vCenter = Center();
		Vec3 vExtents = vCenter - vMins;
		vCenter.TransformCoord( mat );
		Vec3 vNewExtents;
		vNewExtents.x =
			fabsf(mat.m11 * vExtents.x) +
			fabsf(mat.m21 * vExtents.y) +
			fabsf(mat.m31 * vExtents.z);
		vNewExtents.y =
			fabsf(mat.m12 * vExtents.x) +
			fabsf(mat.m22 * vExtents.y) +
			fabsf(mat.m32 * vExtents.z);
		vNewExtents.z =
			fabsf(mat.m13 * vExtents.x) +
			fabsf(mat.m23 * vExtents.y) +
			fabsf(mat.m33 * vExtents.z);
		vMins = vCenter - vNewExtents;
		vMaxs = vCenter + vNewExtents;
	}

	float GetMinSquaredDist(const Vec3 & vPoint) const
	{
		float sqrDist = 0;
		for (int i = 0; i < 3; i++)
		{
			if (vPoint[i] < vMins[i])
				sqrDist += sqr(vMins[i] - vPoint[i]);
			else if (vPoint[i] > vMaxs[i])
				sqrDist += sqr(vPoint[i] - vMaxs[i]);
		}
		return sqrDist;
	}

	bool operator == (const TBBox &ref) const 
	{
		return vMins == ref.vMins && vMaxs == ref.vMaxs; 
	}

	bool operator != (const TBBox &ref) const 
	{
		return vMins != ref.vMins || vMaxs != ref.vMaxs; 
	}
};

typedef TBBox<float>	BBox;
typedef TBBox<float>	BBoxF;
typedef TBBox<double>	BBoxD;

}