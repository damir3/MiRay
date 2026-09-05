/*
 * Copyright (C) 2014, Petr Vevoda, Martin Sik (http://cgg.mff.cuni.cz/~sik/),
 * Tomas Davidovic (http://www.davidovic.cz), Iliyan Georgiev (http://www.iliyan.com/),
 * Jaroslav Krivanek (http://cgg.mff.cuni.cz/~jaroslav/)
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * (The above is MIT License: http://en.wikipedia.origin/wiki/MIT_License)
 */

#ifndef __HASHGRID_HXX__
#define __HASHGRID_HXX__

/**
 * @brief	A hash grid used for photon lookup in surface photon mapping (PPM, BPM) and PP3D.
 */
class HashGrid
{
public:
	void Reserve(int aNumCells)
	{
		mNumCells = aNumCells;
		mCellEnds.resize(aNumCells);
	}

	template<typename tParticle>
	void Build(const tParticle * aParticles, const size_t aParticlesCount, float aRadius, uint aType = 0)
	{
		mRadius      = aRadius;
		mRadiusSqr   = sqr(mRadius);
		mCellSize    = mRadius * 2.f;
		mInvCellSize = 1.f / mCellSize;

		mBBox.clear();

		size_t matchedCount = 0;

		for (auto p = aParticles, pend = aParticles + aParticlesCount; p != pend; ++p) {
			if (p->MatchesType(aType)) {
				mBBox.addToBounds(p->GetPosition());
				matchedCount++;
			}
		}

		mIndices.resize(matchedCount);
		memset(mCellEnds.data(), 0, mCellEnds.size() * sizeof(int));

		// set mCellEnds[x] to number of particles within x
		for (auto p = aParticles, pend = aParticles + aParticlesCount; p != pend; ++p) {
			if (p->MatchesType(aType)) {
				const vec3 &pos = p->GetPosition();
				mCellEnds[GetCellIndex(pos)]++;
			}
		}

		// run exclusive prefix sum to really get the cell starts
		// mCellEnds[x] is now where the cell starts
		int sum = 0;
		for (uint i = 0; i < mNumCells; i++) {
			int temp = mCellEnds[i];
			mCellEnds[i] = sum;
			sum += temp;
		}

		for (size_t i = 0; i < aParticlesCount; i++) {
			if (aParticles[i].MatchesType(aType)) {
				const vec3 &pos = aParticles[i].GetPosition();
				const int targetIdx = mCellEnds[GetCellIndex(pos)]++;
				mIndices[targetIdx] = int(i);
			}
		}

		// now mCellEnds[x] points to the index right after the last
		// element of cell x

		//// DEBUG
		//for (size_t i=0; i<aParticles.size(); i++)
		//{
		//    const vec3 &pos  = aParticles[i].GetPosition();
		//    ivec2 range = GetCellRange(GetCellIndex(pos));
		//    bool found = false;
		//    for (;range.x() < range.y(); range.x()++)
		//    {
		//        if (mIndices[range.x()] == i)
		//            found = true;
		//    }
		//    if (!found)
		//        printf("Error at particle %d\n", i);
		//}
	}

	template<typename tParticle, typename tQuery>
	void process(const tParticle * aParticles, tQuery& aQuery)
	{
		const vec3 queryPos = aQuery.GetPosition();

		const vec3 distMin = queryPos - mBBox.min;
		const vec3 distMax = mBBox.max - queryPos;
		for (int i = 0; i < 3; i++) {
			if (distMin[i] < 0.f) return;
			if (distMax[i] < 0.f) return;
		}

		const vec3 cellPt = mInvCellSize * distMin;
		const vec3 coordF(std::floor(cellPt.x),
						  std::floor(cellPt.y),
						  std::floor(cellPt.z));

		const int  px = int(coordF.x);
		const int  py = int(coordF.y);
		const int  pz = int(coordF.z);

		const vec3 fractCoord = cellPt - coordF;

		const int  pxo = px + (fractCoord.x < 0.5f ? -1 : +1);
		const int  pyo = py + (fractCoord.y < 0.5f ? -1 : +1);
		const int  pzo = pz + (fractCoord.z < 0.5f ? -1 : +1);

		for (int j = 0; j < 8; j++) {
			ivec2 activeRange;
			switch (j) {
				case 0: activeRange = GetCellRange(GetCellIndex(px , py , pz )); break;
				case 1: activeRange = GetCellRange(GetCellIndex(px , py , pzo)); break;
				case 2: activeRange = GetCellRange(GetCellIndex(px , pyo, pz )); break;
				case 3: activeRange = GetCellRange(GetCellIndex(px , pyo, pzo)); break;
				case 4: activeRange = GetCellRange(GetCellIndex(pxo, py , pz )); break;
				case 5: activeRange = GetCellRange(GetCellIndex(pxo, py , pzo)); break;
				case 6: activeRange = GetCellRange(GetCellIndex(pxo, pyo, pz )); break;
				case 7: activeRange = GetCellRange(GetCellIndex(pxo, pyo, pzo)); break;
			}

			for (; activeRange[0] < activeRange[1]; activeRange[0]++) {
				const int particleIndex   = mIndices[activeRange[0]];
				const tParticle &particle = aParticles[particleIndex];

				const float distSqr = glm::length2(aQuery.GetPosition() - particle.GetPosition());

				if (distSqr <= mRadiusSqr)
					aQuery.process(particle);
			}
		}
	}

private:

	ivec2 GetCellRange(int aCellIndex) const
	{
		if (aCellIndex == 0) return ivec2(0, mCellEnds[0]);
		return ivec2(mCellEnds[aCellIndex-1], mCellEnds[aCellIndex]);
	}

	int GetCellIndex(const int x, const int y, const int z) const
	{
		uint ux = uint(x);
		uint uy = uint(y);
		uint uz = uint(z);

		return int(((ux * 73856093) ^ (uy * 19349663) ^ (uz * 83492791)) % mNumCells);
	}

	int GetCellIndex(const vec3 &aPoint) const
	{
		const vec3 distMin = aPoint - mBBox.min;

		const vec3 coordF(std::floor(mInvCellSize * distMin.x),
						  std::floor(mInvCellSize * distMin.y),
						  std::floor(mInvCellSize * distMin.z));

		return GetCellIndex(int(coordF.x), int(coordF.y), int(coordF.z));
	}

private:

	BBox mBBox;
	std::vector<int> mIndices;
	std::vector<int> mCellEnds;

	uint mNumCells;
	float mRadius;
	float mRadiusSqr;
	float mCellSize;
	float mInvCellSize;
};

#endif //__HASHGRID_HXX__
