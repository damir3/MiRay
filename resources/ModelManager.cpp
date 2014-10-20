//
//  ModelManager.cpp
//  MiRay/resources
//
//  Created by Damir Sagidullin on 03.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#ifndef _WIN32
#include <unistd.h>
#else
#include <direct.h>
//#include <Shlwapi.h>
//#pragma comment(lib, "Shlwapi.lib")
#define getcwd	_getcwd
#define chdir	_chdir
#endif

#include "ModelManager.h"

using namespace mr;

// ------------------------------------------------------------------------ //

std::string mr::GetFullPath(const char * pLocalPath)
{
#ifndef _WIN32
	char * pActualPath = realpath(pLocalPath, NULL);
	if (pActualPath == NULL)
		return pLocalPath;
	
	std::string strFullPath = pActualPath;
	free(pActualPath);
	return strFullPath;
#else
	char cFullPathName[MAX_PATH];
	if (0 == GetFullPathNameA(pLocalPath, sizeof(cFullPathName), cFullPathName, NULL))
		return pLocalPath;

	return cFullPathName;
#endif
}

//std::string mr::GetLocalPath(const char * pFullPath)
//{
//	char cCurrentPath[FILENAME_MAX];
//	if (!getcwd(cCurrentPath, sizeof(cCurrentPath)))
//		return pFullPath;
//
//#ifndef _WIN32
//	const char *pDirPath = cCurrentPath;
//	while (*pFullPath == *pDirPath)
//	{
//		pFullPath++;
//		pDirPath++;
//	}
//
//	std::string strRelativePath = *pDirPath ? "../" : "";
//	while (*pDirPath)
//	{
//		if (*pDirPath++ == '/')
//			strRelativePath += "../";
//	}
//	strRelativePath += pFullPath;
//	return strRelativePath;
//#else
//    char cRelativePath[MAX_PATH] = "";	
//    if (!PathRelativePathToA(cRelativePath, cCurrentPath, FILE_ATTRIBUTE_DIRECTORY, pFullPath, FILE_ATTRIBUTE_NORMAL))
//		return pFullPath;
//
//	return cRelativePath;
//#endif
//}

std::string mr::PushDirectory(const char * pFilename)
{
	std::string strPrevDirectory;
	char cCurrentPath[FILENAME_MAX];
	if (getcwd(cCurrentPath, sizeof(cCurrentPath)))
		strPrevDirectory = cCurrentPath;

	std::string	strLocalPath = pFilename;
	size_t p1 = strLocalPath.find_last_of('/');
#ifdef _WIN32
	size_t p2 = strLocalPath.find_last_of('\\');
	if (p2 != std::string::npos)
		p1 = (p1 != std::string::npos) ? std::max(p1, p2) : p2;
#endif
	
	if (p1 != std::string::npos)
		strLocalPath.resize(p1 + 1);
	
	if (!strLocalPath.empty())
		chdir(strLocalPath.c_str());

	return strPrevDirectory;
}

void mr::PopDirectory(const char * pPrevDirectory)
{
	chdir(pPrevDirectory);
}

// ------------------------------------------------------------------------ //

ModelManager::ModelManager(ImageManager * pImageManager)
	: m_pImageManager(pImageManager)
	, m_pSdkManager(NULL)
	, m_pImporter(NULL)
{
	m_pSdkManager = FbxManager::Create();
	if (m_pSdkManager)
	{
//		AppContextSingleton::GetAppContext().GetLog()->Error("Unable to create the FBX SDK manager");
		FbxIOSettings * ios = FbxIOSettings::Create(m_pSdkManager, IOSROOT);
		m_pSdkManager->SetIOSettings(ios);

		m_pImporter = FbxImporter::Create(m_pSdkManager, "");
	}
}

ModelManager::~ModelManager()
{
	if (m_pImporter)
	{
		m_pImporter->Destroy();
		m_pImporter = NULL;
	}

	if (m_pSdkManager)
	{
		m_pSdkManager->Destroy();
		m_pSdkManager = NULL;
	}
}

// ------------------------------------------------------------------------ //

void ModelManager::Release(const std::string & name)
{
	auto it = m_resources.find(name);
	if (it != m_resources.end())
		m_resources.erase(it);
}

ModelPtr ModelManager::LoadModel(const char * strFilename, pugi::xml_node node)
{
	std::string strFullPath = GetFullPath(strFilename);

	auto it = m_resources.find(strFilename);
	if (it != m_resources.end())
	{
		ModelPtr spModel = it->second.lock();
		if (spModel)
			return spModel;
	}
	
	printf("Loading model '%s'...\n", strFullPath.c_str());

	if (!m_pSdkManager || !m_pImporter)
		return nullptr;

	FbxScene * m_pScene = FbxScene::Create(m_pSdkManager, "");
	if (!m_pScene)
		return nullptr;

	ModelPtr spModel;

	if (m_pImporter->Initialize(strFullPath.c_str(), -1, m_pSdkManager->GetIOSettings()))
	{
		int lFileMajor, lFileMinor, lFileRevision;
		m_pImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);
		if (m_pImporter->Import(m_pScene))
		{
			FbxAxisSystem SceneAxisSystem = m_pScene->GetGlobalSettings().GetAxisSystem();
			FbxAxisSystem OurAxisSystem(FbxAxisSystem::eMax);
			if (SceneAxisSystem != OurAxisSystem)
				OurAxisSystem.ConvertScene(m_pScene);
			
			// Convert Unit System to centimeter, if needed
			FbxSystemUnit SceneSystemUnit = m_pScene->GetGlobalSettings().GetSystemUnit();
			if (SceneSystemUnit.GetScaleFactor() != 1.0)
				FbxSystemUnit(1.0).ConvertScene(m_pScene);

			FbxTime time;
			spModel.reset(new Model(*this, strFullPath.c_str()));

			if (!node.empty())
				spModel->MaterialManagerPtr()->LoadMaterials(node);

			CollectMeshes(*spModel.get(), m_pScene->GetRootNode(), (FbxAnimLayer *)NULL, time);

			spModel->MaterialManagerPtr()->CleanupMaterials();

			{// load textures
				std::string strPrevDirectory = PushDirectory(strFullPath.c_str());
				spModel->MaterialManagerPtr()->LoadTextures(m_pImageManager);
				PopDirectory(strPrevDirectory.c_str());
			}
		}
	}

	m_pScene->Destroy();

	return spModel;
}


// ------------------------------------------------------------------------ //

inline Matrix FbxMatrixToMatrix(const FbxAMatrix & matFbx)
{
	const double * m = matFbx;
	return Matrix(static_cast<float>(m[0]), static_cast<float>(m[1]), static_cast<float>(m[2]), static_cast<float>(m[3]),
				  static_cast<float>(m[4]), static_cast<float>(m[5]), static_cast<float>(m[6]), static_cast<float>(m[7]),
				  static_cast<float>(m[8]), static_cast<float>(m[9]), static_cast<float>(m[10]), static_cast<float>(m[11]),
				  static_cast<float>(m[12]), static_cast<float>(m[13]), static_cast<float>(m[14]), static_cast<float>(m[15]));
}

inline Vec3 FbxVector4ToVec3(const FbxVector4 & v)
{
	return Vec3(static_cast<float>(v[0]), static_cast<float>(v[1]), static_cast<float>(v[2]));
}

inline Vec2 FbxVector2ToVec2(const FbxVector2 & v)
{
	return Vec2(static_cast<float>(v[0]), static_cast<float>(v[1]));
}

void ReadMaterialTextureName(std::string & strFilename, const FbxSurfaceMaterial * pMaterialFBX, const char * strFbxParam)
{
	FbxProperty diffuseProperty = pMaterialFBX->FindProperty(strFbxParam);
	int numSrcObjects = diffuseProperty.GetSrcObjectCount();
	for (int i = 0; i < numSrcObjects; i++)
	{
		FbxFileTexture* pTexture = FbxCast<FbxFileTexture>(diffuseProperty.GetSrcObject(i));
		if (pTexture)
		{
//			printf("1: %s\n2: %s\n3: %s\n4: %s\n",
//				   pTexture->GetName(), pTexture->GetFileName(), pTexture->GetRelativeFileName(),
//				   pTexture->GetInitialName());
			strFilename = pTexture->GetRelativeFileName();
			size_t p = strFilename.find_last_of('/');
			if (p != std::string::npos)
				strFilename.erase(0, p);
			p = strFilename.find_last_of('\\');
			if (p != std::string::npos)
				strFilename.erase(0, p);
			break;
		}
	}
}

void ModelManager::AddMesh(Model & model, FbxNode * pFbxNode, FbxAMatrix & pGlobalPosition,
						  const FbxVector4 * pControlPoints, const FbxVector4 * pNormalArray)
{
	model.m_meshes.push_back(new Model::Mesh());
	Model::Mesh & mesh = *model.m_meshes.back();
	
	FbxMesh * pFbxMesh = pFbxNode->GetMesh();
	const int numPolygons = pFbxMesh->GetPolygonCount();
	
	int numMaterials = 0;
	
	//// Count the polygon count of each material
	FbxLayerElementArrayTemplate<int>* pMatElems = NULL;
	FbxGeometryElement::EMappingMode lMaterialMappingMode = FbxGeometryElement::eNone;
	if (pFbxMesh->GetElementMaterial())
	{
		pMatElems = &pFbxMesh->GetElementMaterial()->GetIndexArray();
		lMaterialMappingMode = pFbxMesh->GetElementMaterial()->GetMappingMode();
		if (pMatElems && lMaterialMappingMode == FbxGeometryElement::eByPolygon)
		{
			assert(pMatElems->GetCount() == numPolygons);
			if (pMatElems->GetCount() == numPolygons)
			{
				// Count the faces of each material
				for (int pi = 0; pi < numPolygons; ++pi)
					numMaterials = std::max<int>(numMaterials, pMatElems->GetAt(pi));
			}
		}
	}
	
	numMaterials++;

	IndexArray triangleCount(numMaterials);
	std::fill(triangleCount.begin(), triangleCount.end(), 0);
	mesh.m_geometries.reserve(numMaterials);
	for (int i = 0; i < numMaterials; i++)
		mesh.m_geometries.push_back(new Model::Geometry());

	if (pMatElems && lMaterialMappingMode == FbxGeometryElement::eByPolygon)
	{
		assert(pMatElems->GetCount() == numPolygons);
		// Count the faces of each material
		for (int pi = 0; pi < numPolygons; ++pi)
			triangleCount[pMatElems->GetAt(pi)]++;
	}
	else
		triangleCount[0] = numPolygons;
	
	bool bAllByControlPoint = mesh.m_geometries.size() == 1 && pNormalArray == NULL;
	bool bHasNormal = pFbxMesh->GetElementNormalCount() > 0;
	bool bHasUV = pFbxMesh->GetElementUVCount() > 0;
	FbxGeometryElement::EMappingMode lNormalMappingMode = FbxGeometryElement::eNone;
	FbxGeometryElement::EMappingMode lUVMappingMode = FbxGeometryElement::eNone;
	if (bHasNormal)
	{
		lNormalMappingMode = pFbxMesh->GetElementNormal(0)->GetMappingMode();
		if (lNormalMappingMode == FbxGeometryElement::eNone)
			bHasNormal = false;
		
		if (bHasNormal && lNormalMappingMode != FbxGeometryElement::eByControlPoint)
			bAllByControlPoint = false;
	}
	if (bHasUV)
	{
		lUVMappingMode = pFbxMesh->GetElementUV(0)->GetMappingMode();
		if (lUVMappingMode == FbxGeometryElement::eNone)
			bHasUV = false;
		
		if (bHasUV && lUVMappingMode != FbxGeometryElement::eByControlPoint)
			bAllByControlPoint = false;
	}
	
	for (size_t i = 0; i < mesh.m_geometries.size(); i++)
	{
		mesh.m_geometries[i]->m_indices.reserve(triangleCount[i] * 3);
		if (!bAllByControlPoint)
			mesh.m_geometries[i]->m_vertices.reserve(triangleCount[i] * 3);
	}
	
	FbxStringList lUVNames;
	pFbxMesh->GetUVSetNames(lUVNames);
	const char * lUVName = bHasUV && lUVNames.GetCount() ? (const char *)lUVNames[0] : NULL;
	
	if (bAllByControlPoint)
	{
		const FbxGeometryElementNormal * pNormalElement = bHasNormal ? pFbxMesh->GetElementNormal(0) : NULL;
		const FbxGeometryElementUV * pUVElement = bHasUV ? pFbxMesh->GetElementUV(0) : NULL;
		
		int lPolygonVertexCount = pFbxMesh->GetControlPointsCount();
		mesh.m_geometries[0]->m_vertices.resize(lPolygonVertexCount);
		for (int vi = 0; vi < lPolygonVertexCount; ++vi)
		{
			Vertex & v = mesh.m_geometries[0]->m_vertices[vi];
			v.pos = FbxVector4ToVec3( pControlPoints[vi] );
			
			if (bHasNormal)
			{
				int ni = (pNormalElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect) ? pNormalElement->GetIndexArray().GetAt(vi) : vi;
				v.normal = FbxVector4ToVec3( pNormalElement->GetDirectArray().GetAt(ni) );
			}
			
			if (bHasUV)
			{
				int uvi = (pUVElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect) ? pUVElement->GetIndexArray().GetAt(vi) : vi;
				v.tc = FbxVector2ToVec2( pUVElement->GetDirectArray().GetAt(uvi) );
			}
		}
	}
	
	for (int pi = 0; pi < numPolygons; pi++)
	{
		int mi = (pMatElems && lMaterialMappingMode == FbxGeometryElement::eByPolygon) ? pMatElems->GetAt(pi) : 0;
		int nPolygonSize = pFbxMesh->GetPolygonSize(pi);
		
		IndexArray & indices = mesh.m_geometries[mi]->m_indices;
		if (bAllByControlPoint)
		{
			for (int vi = 2; vi < nPolygonSize; vi++)
			{
				indices.push_back(pFbxMesh->GetPolygonVertex(pi, vi));
				indices.push_back(pFbxMesh->GetPolygonVertex(pi, vi - 1));
				indices.push_back(pFbxMesh->GetPolygonVertex(pi, 0));
			}
		}
		else
		{
			VertexArray & vertices = mesh.m_geometries[mi]->m_vertices;
			uint32 nFirstVextex = static_cast<uint32>(vertices.size());
			
			for (int vi = 0; vi < nPolygonSize; vi++)
			{
				Vertex v;
				v.pos = FbxVector4ToVec3( pControlPoints[pFbxMesh->GetPolygonVertex(pi, vi)] );
				
				if (bHasNormal)
				{
					FbxVector4 normal;
					pFbxMesh->GetPolygonVertexNormal(pi, vi, normal);
					v.normal = FbxVector4ToVec3(normal);
				}
				
				if (bHasUV)
				{
					FbxVector2 uv;
					bool bUnmapped;
					pFbxMesh->GetPolygonVertexUV(pi, vi, lUVName, uv, bUnmapped);
					v.tc = FbxVector2ToVec2(uv);
				}
				
				vertices.push_back(v);
			}
			
			for (int vi = 2; vi < nPolygonSize; vi++)
			{
				indices.push_back(nFirstVextex + vi);
				indices.push_back(nFirstVextex + vi - 1);
				indices.push_back(nFirstVextex);
			}
		}
	}
	
	{
		Matrix mat = FbxMatrixToMatrix(pGlobalPosition);
//		Matrix matScale;
//		matScale.Scale(100.f);
//		mat = mat * matScale;
		Matrix matInv;
		matInv.Inverse(mat);

//		for (Model::GeometryArray::iterator itGeom = mesh.m_geometries.begin(), itGeomEnd = mesh.m_geometries.end(); itGeom != itGeomEnd; ++itGeom)
		for (size_t gi = 0; gi < mesh.m_geometries.size(); gi++)
		{
			Model::Geometry & geom = *mesh.m_geometries[gi];
			for (VertexArray::iterator it = geom.m_vertices.begin(), itEnd = geom.m_vertices.end(); it != itEnd; ++it)
			{
				it->pos.TransformCoord(mat);
				it->normal.TTransformNormal(matInv);
				it->normal.Normalize();
				it->tc.y = 1.f - it->tc.y;
				geom.m_bbox.AddToBounds(it->pos);
			}
			
			mesh.m_bbox.AddToBounds(geom.m_bbox);

			const FbxSurfaceMaterial * pFbxMaterial = pFbxNode->GetMaterial(static_cast<int>(gi));
			if (pFbxMaterial)
			{
//				printf("node '%s/%s/%s'\n", pFbxNode->GetName(), pFbxMesh->GetName(), pFbxMaterial->GetName());
				std::string strMaterial = pFbxNode->GetName();
				strMaterial += "/";
				strMaterial += pFbxMaterial->GetName();
				geom.m_pMaterial = model.MaterialManagerPtr()->Get(strMaterial.c_str());
				if (!geom.m_pMaterial)
				{
					geom.m_pMaterial = model.MaterialManagerPtr()->Create(strMaterial.c_str());
					geom.m_pMaterial->Create();

//					ReadMaterialTextureName(geom.m_pMaterial->m_diffuseTexName, pFbxMaterial, FbxSurfaceMaterial::sDiffuse);
//					ReadMaterialTextureName(geom.m_pMaterial->m_specularTexName, pFbxMaterial, FbxSurfaceMaterial::sSpecular);
//					ReadMaterialTextureName(geom.m_pMaterial->m_opacityTexName, pFbxMaterial, FbxSurfaceMaterial::sTransparentColor);
//					ReadMaterialTextureName(geom.m_pMaterial->m_bumpMapName, pFbxMaterial, FbxSurfaceMaterial::sBump);
//					ReadMaterialTextureName(geom.m_pMaterial->m_normalMapName, pFbxMaterial, FbxSurfaceMaterial::sNormalMap);

					FbxClassId classId = pFbxMaterial->GetClassId();
					if (classId.Is(FbxSurfaceLambert::ClassId) || classId.Is(FbxSurfacePhong::ClassId))
					{
//						const FbxSurfaceLambert * pLambert = (FbxSurfaceLambert *)pFbxMaterial;
//						FbxDouble3 cDiffuse = pLambert->Diffuse;
//						FbxDouble fDiffuse = pLambert->DiffuseFactor;
//						geom.m_pMaterial->m_diffuse.SetColor((float)(cDiffuse[0] * fDiffuse),
//															 (float)(cDiffuse[1] * fDiffuse),
//															 (float)(cDiffuse[2] * fDiffuse), 1.f);
//						FbxDouble3 cTransparent = pLambert->TransparentColor;
//						geom.m_pMaterial->m_refractionColor = ColorF(cTransparent[0], cTransparent[1], cTransparent[2]);
//						geom.m_pMaterial->m_opacity.SetColor(ColorF(1.f - static_cast<float>(pLambert->TransparencyFactor)));
//						geom.m_pMaterial->m_bumpLevel = static_cast<float>(pLambert->BumpFactor);

//						if (classId.Is(FbxSurfacePhong::ClassId))
//						{
//							const FbxSurfacePhong * pPhong = (FbxSurfacePhong *)pFbxMaterial;
//							geom.m_pMaterial->m_reflectionGlossiness = (logf(float(pPhong->Shininess)) / logf(2.f) - 4.f) / 4.f;
//							FbxDouble3 cReflection = pPhong->Reflection;
//							geom.m_pMaterial->m_reflectionColor = ColorF(cReflection[0], cReflection[1], cReflection[2], 1.0 - pPhong->ReflectionFactor);
//						}
					}
				}
			}
		}
	}
	
	model.m_bbox.AddToBounds(mesh.m_bbox);
}

// ------------------------------------------------------------------------ //

struct NormalKey
{
	int vi;
	Vec3 vNormal;
	
	NormalKey(int i, const Vec3 & normal) : vi(i), vNormal(normal) {}
	
	bool operator < (const NormalKey & nk) const
	{
		if (vi < nk.vi) return true;
		if (vi > nk.vi) return false;
		
		if (vNormal.x < nk.vNormal.x - DELTA_EPSILON) return true;
		if (vNormal.x > nk.vNormal.x + DELTA_EPSILON) return false;
		
		if (vNormal.y < nk.vNormal.y - DELTA_EPSILON) return true;
		if (vNormal.y > nk.vNormal.y + DELTA_EPSILON) return false;
		
		if (vNormal.z < nk.vNormal.z - DELTA_EPSILON) return true;
		if (vNormal.z > nk.vNormal.z + DELTA_EPSILON) return false;
		
		return false;
	}
};

typedef std::map<NormalKey, size_t> NormalsMap;

void RecalculateMeshNormals(FbxMesh * pFbxMesh, const FbxVector4 * pVertexArray, FbxVector4 * pNormalsArray)
{
	if (!pNormalsArray)
		return;
	
	const int numPolygons = pFbxMesh->GetPolygonCount();
	
	size_t	iNormalsCount = 0;
	std::vector<size_t>	vNormalIndex(numPolygons * 3);
	
	{
		NormalsMap	mapNormals;
		
		for (int pi = 0, ni = 0; pi < numPolygons; pi++)
		{
			for (int vi = 0; vi < 3; vi++, ni++)
			{
				int i = pFbxMesh->GetPolygonVertex(pi, vi);
				FbxVector4 lNormal;
				if (pFbxMesh->GetPolygonVertexNormal(pi, vi, lNormal))
				{
					NormalKey nk(i, Vec3((float)lNormal[0], (float)lNormal[1], (float)lNormal[2]));
					NormalsMap::const_iterator it = mapNormals.find(nk);
					if (it == mapNormals.end())
						vNormalIndex[ni] = mapNormals[nk] = iNormalsCount++;
					else
						vNormalIndex[ni] = it->second;
				}
				else
					vNormalIndex[ni] = iNormalsCount++;
			}
		}
	}
	
	std::vector<Vec3>	vNormals(iNormalsCount);
	std::fill(vNormals.begin(), vNormals.end(), Vec3::Null);
	
	for (int pi = 0, ni = 0; pi < numPolygons; pi++, ni+=3)
	{
		Vec3 v[3];
		for (int vi = 0; vi < 3; vi++)
		{
			const FbxVector4 & p = pVertexArray[pFbxMesh->GetPolygonVertex(pi, vi)];
			v[vi] = Vec3((float)p[0], (float)p[1], (float)p[2]);
		}
		Vec3 vNormal = Vec3::Cross(v[1] - v[0], v[2] - v[0]);
		vNormal.Normalize();
		vNormals[vNormalIndex[ni+0]] += vNormal;
		vNormals[vNormalIndex[ni+1]] += vNormal;
		vNormals[vNormalIndex[ni+2]] += vNormal;
	}
	
	for (size_t i = 0; i < vNormals.size(); i++)
		vNormals[i].Normalize();
	
	for (int pi = 0, ni = 0; pi < numPolygons; pi++)
	{
		for (int vi = 0; vi < 3; vi++, ni++)
		{
			const Vec3 & vNormal = vNormals[vNormalIndex[ni]];
			pNormalsArray[ni].Set(vNormal.x, vNormal.y, vNormal.z, 0.0);
		}
	}
}

// ------------------------------------------------------------------------ //

void ReadVertexCacheData(FbxMesh * pFbxMesh, FbxTime & pTime, FbxVector4 * pVertexArray)
{
	FbxVertexCacheDeformer*	lDeformer     = static_cast<FbxVertexCacheDeformer*>(pFbxMesh->GetDeformer(0, FbxDeformer::eVertexCache));
	FbxCache*				lCache        = lDeformer->GetCache();
	int						lChannelIndex = -1;
	const unsigned int		lVertexCount  = (unsigned int)pFbxMesh->GetControlPointsCount();
	bool					lReadSucceed  = false;
	double*					lReadBuf      = new double[3*lVertexCount];
	
	if (lCache->GetCacheFileFormat() == FbxCache::eMayaCache)
	{
		if ((lChannelIndex = lCache->GetChannelIndex(lDeformer->GetCacheChannel())) > -1)
		{
			lReadSucceed = lCache->Read(lChannelIndex, pTime, lReadBuf, lVertexCount);
		}
	}
	else // eMaxPointCacheV2
	{
		lReadSucceed = lCache->Read((unsigned int)pTime.GetFrameCount(), lReadBuf, lVertexCount);
	}
	
	if (lReadSucceed)
	{
		unsigned int lReadBufIndex = 0;
		
		while (lReadBufIndex < 3*lVertexCount)
		{
			// In statements like "pVertexArray[lReadBufIndex/3].SetAt(2, lReadBuf[lReadBufIndex++])",
			// on Mac platform, "lReadBufIndex++" is evaluated before "lReadBufIndex/3".
			// So separate them.
			pVertexArray[lReadBufIndex/3].mData[0] = lReadBuf[lReadBufIndex]; lReadBufIndex++;
			pVertexArray[lReadBufIndex/3].mData[1] = lReadBuf[lReadBufIndex]; lReadBufIndex++;
			pVertexArray[lReadBufIndex/3].mData[2] = lReadBuf[lReadBufIndex]; lReadBufIndex++;
		}
	}
	
	delete [] lReadBuf;
}

// Deform the vertex array with the shapes contained in the mesh.
void ComputeShapeDeformation(FbxMesh* pFbxMesh, FbxTime & pTime, FbxAnimLayer * pFbxAnimLayer, FbxVector4* pVertexArray)
{
	const int lVertexCount = pFbxMesh->GetControlPointsCount();
	
	FbxVector4* pSrcVertexArray = pVertexArray;
	FbxVector4* pDstVertexArray = new FbxVector4[lVertexCount];
	memcpy(pDstVertexArray, pVertexArray, lVertexCount * sizeof(FbxVector4));
	
	int lBlendShapeDeformerCount = pFbxMesh->GetDeformerCount(FbxDeformer::eBlendShape);
	for (int lBlendShapeIndex = 0; lBlendShapeIndex<lBlendShapeDeformerCount; ++lBlendShapeIndex)
	{
		FbxBlendShape* lBlendShape = (FbxBlendShape*)pFbxMesh->GetDeformer(lBlendShapeIndex, FbxDeformer::eBlendShape);
		
		int lBlendShapeChannelCount = lBlendShape->GetBlendShapeChannelCount();
		for (int lChannelIndex = 0; lChannelIndex<lBlendShapeChannelCount; ++lChannelIndex)
		{
			FbxBlendShapeChannel* lChannel = lBlendShape->GetBlendShapeChannel(lChannelIndex);
			
			if (lChannel)
			{
				// Get the percentage of influence of the shape.
				FbxAnimCurve* lFCurve = pFbxMesh->GetShapeChannel(lBlendShapeIndex, lChannelIndex, pFbxAnimLayer);
				if (!lFCurve) continue;
				double lWeight = lFCurve->Evaluate(pTime);
				
				//Find which shape should we use according to the weight.
				int lShapeCount = lChannel->GetTargetShapeCount();
				double* lFullWeights = lChannel->GetTargetShapeFullWeights();
				for (int lShapeIndex = 0; lShapeIndex<lShapeCount; ++lShapeIndex)
				{
					FbxShape* lShape = NULL;
					if (lWeight > 0 && lWeight <= lFullWeights[0])
					{
						lShape = lChannel->GetTargetShape(0);
					}
					if (lWeight > lFullWeights[lShapeIndex] && lWeight < lFullWeights[lShapeIndex+1])
					{
						lShape = lChannel->GetTargetShape(lShapeIndex+1);
					}
					
					if (lShape)
					{
						for (int j = 0; j < lVertexCount; j++)
						{
							// Add the influence of the shape vertex to the mesh vertex.
							FbxVector4 lInfluence = (lShape->GetControlPoints()[j] - pSrcVertexArray[j]) * lWeight * 0.01;
							pDstVertexArray[j] += lInfluence;
						}
					}
				}//For each target shape
			}//If lChannel is valid
		}//For each blend shape channel
	}//For each blend shape deformer
	
	memcpy(pVertexArray, pDstVertexArray, lVertexCount * sizeof(FbxVector4));
	
	delete [] pDstVertexArray;
}

// Get the matrix of the given pose
FbxAMatrix GetPoseMatrix(FbxPose* pPose, int iNodeIndex)
{
	FbxAMatrix lPoseMatrix;
	FbxMatrix lMatrix = pPose->GetMatrix(iNodeIndex);
	
	memcpy((double*)lPoseMatrix, (double*)lMatrix, sizeof(lMatrix.mData));
	
	return lPoseMatrix;
}

// Get the global position of the node for the current pose.
// If the specified node is not part of the pose or no pose is specified, get its
// global position at the current time.
FbxAMatrix GetGlobalPosition(FbxNode* pFbxNode, FbxTime & pTime, FbxPose* pPose, FbxAMatrix* pParentGlobalPosition = NULL)
{
	FbxAMatrix	lGlobalPosition;
	bool		bPositionFound = false;
	
	if (pPose)
	{
		int iNodeIndex = pPose->Find(pFbxNode);
		
		if (iNodeIndex > -1)
		{
			// The bind pose is always a global matrix.
			// If we have a rest pose, we need to check if it is
			// stored in global or local space.
			if (pPose->IsBindPose() || !pPose->IsLocalMatrix(iNodeIndex))
			{
				lGlobalPosition = GetPoseMatrix(pPose, iNodeIndex);
			}
			else
			{
				// We have a local matrix, we need to convert it to
				// a global space matrix.
				FbxAMatrix lParentGlobalPosition;
				
				if (pParentGlobalPosition)
				{
					lParentGlobalPosition = *pParentGlobalPosition;
				}
				else
				{
					if (pFbxNode->GetParent())
					{
						lParentGlobalPosition = GetGlobalPosition(pFbxNode->GetParent(), pTime, pPose);
					}
				}
				
				FbxAMatrix lLocalPosition = GetPoseMatrix(pPose, iNodeIndex);
				lGlobalPosition = lParentGlobalPosition * lLocalPosition;
			}
			
			bPositionFound = true;
		}
	}
	
	if (!bPositionFound)
	{
		// There is no pose entry for that node, get the current global position instead.
		
		// Ideally this would use parent global position and local position to compute the global position.
		// Unfortunately the equation
		//    lGlobalPosition = pParentGlobalPosition * lLocalPosition
		// does not hold when inheritance type is other than "Parent" (RSrs).
		// To compute the parent rotation and scaling is tricky in the RrSs and Rrs cases.
		lGlobalPosition = pFbxNode->EvaluateGlobalTransform(pTime);
	}
	
	return lGlobalPosition;
}

inline FbxAMatrix GetGeometry(FbxNode* pFbxNode)
{
	const FbxVector4 lT = pFbxNode->GetGeometricTranslation(FbxNode::eSourcePivot);
	const FbxVector4 lR = pFbxNode->GetGeometricRotation(FbxNode::eSourcePivot);
	const FbxVector4 lS = pFbxNode->GetGeometricScaling(FbxNode::eSourcePivot);
	return FbxAMatrix(lT, lR, lS);
}

// Scale all the elements of a matrix.
void MatrixScale(FbxAMatrix& pMatrix, double pValue)
{
	int i,j;
	
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			pMatrix[i][j] *= pValue;
		}
	}
}

// Add a value to all the elements in the diagonal of the matrix.
void MatrixAddToDiagonal(FbxAMatrix& pMatrix, double pValue)
{
	pMatrix[0][0] += pValue;
	pMatrix[1][1] += pValue;
	pMatrix[2][2] += pValue;
	pMatrix[3][3] += pValue;
}

// Sum two matrices element by element.
void MatrixAdd(FbxAMatrix& pDstMatrix, FbxAMatrix& pSrcMatrix)
{
	int i,j;
	
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			pDstMatrix[i][j] += pSrcMatrix[i][j];
		}
	}
}

//Compute the transform matrix that the cluster will transform the vertex.
void ModelManager::ComputeClusterDeformation(FbxAMatrix & pGlobalPosition, FbxMesh * pFbxMesh, FbxCluster * pCluster,
											FbxAMatrix & pVertexTransformMatrix, FbxTime & pTime, FbxPose * pPose)
{
	FbxCluster::ELinkMode lClusterMode = pCluster->GetLinkMode();
	
	FbxAMatrix lReferenceGlobalInitPosition;
	FbxAMatrix lReferenceGlobalCurrentPosition;
	FbxAMatrix lAssociateGlobalInitPosition;
	FbxAMatrix lAssociateGlobalCurrentPosition;
	FbxAMatrix lClusterGlobalInitPosition;
	FbxAMatrix lClusterGlobalCurrentPosition;
	
	FbxAMatrix lReferenceGeometry;
	FbxAMatrix lAssociateGeometry;
	FbxAMatrix lClusterGeometry;
	
	FbxAMatrix lClusterRelativeInitPosition;
	FbxAMatrix lClusterRelativeCurrentPositionInverse;
	
	if (lClusterMode == FbxCluster::eAdditive && pCluster->GetAssociateModel())
	{
		pCluster->GetTransformAssociateModelMatrix(lAssociateGlobalInitPosition);
		// Geometric transform of the model
		lAssociateGeometry = GetGeometry(pCluster->GetAssociateModel());
		lAssociateGlobalInitPosition *= lAssociateGeometry;
		lAssociateGlobalCurrentPosition = GetGlobalPosition(pCluster->GetAssociateModel(), pTime, pPose);
		
		pCluster->GetTransformMatrix(lReferenceGlobalInitPosition);
		// Multiply lReferenceGlobalInitPosition by Geometric Transformation
		lReferenceGeometry = GetGeometry(pFbxMesh->GetNode());
		lReferenceGlobalInitPosition *= lReferenceGeometry;
		lReferenceGlobalCurrentPosition = pGlobalPosition;
		
		// Get the link initial global position and the link current global position.
		pCluster->GetTransformLinkMatrix(lClusterGlobalInitPosition);
		// Multiply lClusterGlobalInitPosition by Geometric Transformation
		lClusterGeometry = GetGeometry(pCluster->GetLink());
		lClusterGlobalInitPosition *= lClusterGeometry;
		lClusterGlobalCurrentPosition = GetGlobalPosition(pCluster->GetLink(), pTime, pPose);
		
		// Compute the shift of the link relative to the reference.
		//ModelM-1 * AssoM * AssoGX-1 * LinkGX * LinkM-1*ModelM
		pVertexTransformMatrix = lReferenceGlobalInitPosition.Inverse() * lAssociateGlobalInitPosition * lAssociateGlobalCurrentPosition.Inverse() *
		lClusterGlobalCurrentPosition * lClusterGlobalInitPosition.Inverse() * lReferenceGlobalInitPosition;
	}
	else
	{
		pCluster->GetTransformMatrix(lReferenceGlobalInitPosition);
		lReferenceGlobalCurrentPosition = pGlobalPosition;
		
		// Multiply lReferenceGlobalInitPosition by Geometric Transformation
		lReferenceGeometry = GetGeometry(pFbxMesh->GetNode());
		lReferenceGlobalInitPosition *= lReferenceGeometry;
		
		// Get the link initial global position and the link current global position.
		pCluster->GetTransformLinkMatrix(lClusterGlobalInitPosition);
		lClusterGlobalCurrentPosition = GetGlobalPosition(pCluster->GetLink(), pTime, pPose);
		
		// Compute the initial position of the link relative to the reference.
		lClusterRelativeInitPosition = lClusterGlobalInitPosition.Inverse() * lReferenceGlobalInitPosition;
		
		// Compute the current position of the link relative to the reference.
		lClusterRelativeCurrentPositionInverse = lReferenceGlobalCurrentPosition.Inverse() * lClusterGlobalCurrentPosition;
		
		// Compute the shift of the link relative to the reference.
		pVertexTransformMatrix = lClusterRelativeCurrentPositionInverse * lClusterRelativeInitPosition;
	}
}

// Deform the vertex array in classic linear way.
void ModelManager::ComputeLinearDeformation(FbxAMatrix& pGlobalPosition, FbxMesh* pFbxMesh, FbxTime & pTime,
										   FbxVector4* pVertexArray, FbxPose* pPose)
{
	// All the links must have the same link mode.
	FbxCluster::ELinkMode lClusterMode = ((FbxSkin*)pFbxMesh->GetDeformer(0, FbxDeformer::eSkin))->GetCluster(0)->GetLinkMode();
	
	const int lVertexCount = pFbxMesh->GetControlPointsCount();
	FbxAMatrix* lClusterDeformation = new FbxAMatrix[lVertexCount];
	memset(lClusterDeformation, 0, lVertexCount * sizeof(FbxAMatrix));
	
	double* lClusterWeight = new double[lVertexCount];
	memset(lClusterWeight, 0, lVertexCount * sizeof(double));
	
	if (lClusterMode == FbxCluster::eAdditive)
	{
		for (int i = 0; i < lVertexCount; ++i)
		{
			lClusterDeformation[i].SetIdentity();
		}
	}
	
	// For all skins and all clusters, accumulate their deformation and weight
	// on each vertices and store them in lClusterDeformation and lClusterWeight.
	int lSkinCount = pFbxMesh->GetDeformerCount(FbxDeformer::eSkin);
	for ( int lSkinIndex=0; lSkinIndex<lSkinCount; ++lSkinIndex)
	{
		FbxSkin * lSkinDeformer = (FbxSkin *)pFbxMesh->GetDeformer(lSkinIndex, FbxDeformer::eSkin);
		
		int lClusterCount = lSkinDeformer->GetClusterCount();
		for ( int lClusterIndex=0; lClusterIndex<lClusterCount; ++lClusterIndex)
		{
			FbxCluster* lCluster = lSkinDeformer->GetCluster(lClusterIndex);
			if (!lCluster->GetLink())
				continue;
			
			FbxAMatrix lVertexTransformMatrix;
			ComputeClusterDeformation(pGlobalPosition, pFbxMesh, lCluster, lVertexTransformMatrix, pTime, pPose);
			
			int lVertexIndexCount = lCluster->GetControlPointIndicesCount();
			for (int k = 0; k < lVertexIndexCount; ++k)
			{
				int lIndex = lCluster->GetControlPointIndices()[k];
				
				// Sometimes, the mesh can have less points than at the time of the skinning
				// because a smooth operator was active when skinning but has been deactivated during export.
				if (lIndex >= lVertexCount)
					continue;
				
				double lWeight = lCluster->GetControlPointWeights()[k];
				if (lWeight == 0.0)
					continue;
				
				// Compute the influence of the link on the vertex.
				FbxAMatrix lInfluence = lVertexTransformMatrix;
				MatrixScale(lInfluence, lWeight);
				
				if (lClusterMode == FbxCluster::eAdditive)
				{
					// Multiply with the product of the deformations on the vertex.
					MatrixAddToDiagonal(lInfluence, 1.0 - lWeight);
					lClusterDeformation[lIndex] = lInfluence * lClusterDeformation[lIndex];
					
					// Set the link to 1.0 just to know this vertex is influenced by a link.
					lClusterWeight[lIndex] = 1.0;
				}
				else // lLinkMode == FbxCluster::eNormalize || lLinkMode == FbxCluster::eTotalOne
				{
					// Add to the sum of the deformations on the vertex.
					MatrixAdd(lClusterDeformation[lIndex], lInfluence);
					
					// Add to the sum of weights to either normalize or complete the vertex.
					lClusterWeight[lIndex] += lWeight;
				}
			}//For each vertex
		}//lClusterCount
	}
	
	//Actually deform each vertices here by information stored in lClusterDeformation and lClusterWeight
	for (int i = 0; i < lVertexCount; i++)
	{
		FbxVector4 lSrcVertex = pVertexArray[i];
		FbxVector4& lDstVertex = pVertexArray[i];
		double lWeight = lClusterWeight[i];
		
		// Deform the vertex if there was at least a link with an influence on the vertex,
		if (lWeight != 0.0)
		{
			lDstVertex = lClusterDeformation[i].MultT(lSrcVertex);
			if (lClusterMode == FbxCluster::eNormalize)
			{
				// In the normalized link mode, a vertex is always totally influenced by the links.
				lDstVertex /= lWeight;
			}
			else if (lClusterMode == FbxCluster::eTotalOne)
			{
				// In the total 1 link mode, a vertex can be partially influenced by the links.
				lSrcVertex *= (1.0 - lWeight);
				lDstVertex += lSrcVertex;
			}
		}
	}
	
	//if (pNormalArray)
	//{
	//	const int numPolygons = pFbxMesh->GetPolygonCount();
	//	for (int pi = 0, ni = 0; pi < numPolygons; pi++)
	//	{
	//		for (int vi = 0; vi < 3; vi++, ni++)
	//		{
	//			const int i = pFbxMesh->GetPolygonVertex(pi, vi);
	//			double lWeight = lClusterWeight[i];
	//			if (lWeight == 0.0)
	//				continue;
	
	//			FbxVector4 lSrcNormal = pNormalArray[ni];
	//			FbxVector4& lDstNormal = pNormalArray[ni];
	//			const double * m = lClusterDeformation[i];
	//			lDstNormal[0] = lSrcNormal[0] * m[0] + lSrcNormal[1] * m[4] + lSrcNormal[2] * m[8];
	//			lDstNormal[1] = lSrcNormal[0] * m[1] + lSrcNormal[1] * m[5] + lSrcNormal[2] * m[9];
	//			lDstNormal[2] = lSrcNormal[0] * m[2] + lSrcNormal[1] * m[6] + lSrcNormal[2] * m[10];
	//			lDstNormal[3] = 0.0;
	//			if (lClusterMode == FbxCluster::eNormalize)
	//			{
	//				// In the normalized link mode, a vertex is always totally influenced by the links.
	//				lDstNormal /= lWeight;
	//			}
	//			else if (lClusterMode == FbxCluster::eTotalOne)
	//			{
	//				// In the total 1 link mode, a vertex can be partially influenced by the links.
	//				lSrcNormal *= (1.0 - lWeight);
	//				lDstNormal += lSrcNormal;
	//			}
	//			lDstNormal.Normalize();
	//		}
	//	}
	//}
	
	delete [] lClusterDeformation;
	delete [] lClusterWeight;
}

// Deform the vertex array in Dual Quaternion Skinning way.
void ModelManager::ComputeDualQuaternionDeformation(FbxAMatrix& pGlobalPosition, FbxMesh* pFbxMesh,
												   FbxTime & pTime, FbxVector4* pVertexArray, FbxPose* pPose)
{
	// All the links must have the same link mode.
	FbxCluster::ELinkMode lClusterMode = ((FbxSkin*)pFbxMesh->GetDeformer(0, FbxDeformer::eSkin))->GetCluster(0)->GetLinkMode();
	
	const int lVertexCount = pFbxMesh->GetControlPointsCount();
	const int lSkinCount = pFbxMesh->GetDeformerCount(FbxDeformer::eSkin);
	
	FbxDualQuaternion* lDQClusterDeformation = new FbxDualQuaternion[lVertexCount];
	memset(lDQClusterDeformation, 0, lVertexCount * sizeof(FbxDualQuaternion));
	
	double* lClusterWeight = new double[lVertexCount];
	memset(lClusterWeight, 0, lVertexCount * sizeof(double));
	
	// For all skins and all clusters, accumulate their deformation and weight
	// on each vertices and store them in lClusterDeformation and lClusterWeight.
	for ( int lSkinIndex=0; lSkinIndex<lSkinCount; ++lSkinIndex)
	{
		FbxSkin * lSkinDeformer = (FbxSkin *)pFbxMesh->GetDeformer(lSkinIndex, FbxDeformer::eSkin);
		int lClusterCount = lSkinDeformer->GetClusterCount();
		for ( int lClusterIndex=0; lClusterIndex<lClusterCount; ++lClusterIndex)
		{
			FbxCluster* lCluster = lSkinDeformer->GetCluster(lClusterIndex);
			if (!lCluster->GetLink())
				continue;
			
			FbxAMatrix lVertexTransformMatrix;
			ComputeClusterDeformation(pGlobalPosition, pFbxMesh, lCluster, lVertexTransformMatrix, pTime, pPose);
			
			FbxQuaternion lQ = lVertexTransformMatrix.GetQ();
			FbxVector4 lT = lVertexTransformMatrix.GetT();
			FbxDualQuaternion lDualQuaternion(lQ, lT);
			
			int lVertexIndexCount = lCluster->GetControlPointIndicesCount();
			for (int k = 0; k < lVertexIndexCount; ++k)
			{
				int lIndex = lCluster->GetControlPointIndices()[k];
				
				// Sometimes, the mesh can have less points than at the time of the skinning
				// because a smooth operator was active when skinning but has been deactivated during export.
				if (lIndex >= lVertexCount)
					continue;
				
				double lWeight = lCluster->GetControlPointWeights()[k];
				
				if (lWeight == 0.0)
					continue;
				
				// Compute the influence of the link on the vertex.
				FbxDualQuaternion lInfluence = lDualQuaternion * lWeight;
				if (lClusterMode == FbxCluster::eAdditive)
				{
					// Simply influenced by the dual quaternion.
					lDQClusterDeformation[lIndex] = lInfluence;
					
					// Set the link to 1.0 just to know this vertex is influenced by a link.
					lClusterWeight[lIndex] = 1.0;
				}
				else // lLinkMode == FbxCluster::eNormalize || lLinkMode == FbxCluster::eTotalOne
				{
					if (lClusterIndex == 0)
					{
						lDQClusterDeformation[lIndex] = lInfluence;
					}
					else
					{
						// Add to the sum of the deformations on the vertex.
						// Make sure the deformation is accumulated in the same rotation direction.
						// Use dot product to judge the sign.
						double lSign = lDQClusterDeformation[lIndex].GetFirstQuaternion().DotProduct(lDualQuaternion.GetFirstQuaternion());
						if ( lSign >= 0.0 )
						{
							lDQClusterDeformation[lIndex] += lInfluence;
						}
						else
						{
							lDQClusterDeformation[lIndex] -= lInfluence;
						}
					}
					// Add to the sum of weights to either normalize or complete the vertex.
					lClusterWeight[lIndex] += lWeight;
				}
			}//For each vertex
		}//lClusterCount
	}
	
	//Actually deform each vertices here by information stored in lClusterDeformation and lClusterWeight
	for (int i = 0; i < lVertexCount; i++)
	{
		FbxVector4 lSrcVertex = pVertexArray[i];
		FbxVector4& lDstVertex = pVertexArray[i];
		double lWeightSum = lClusterWeight[i];
		
		// Deform the vertex if there was at least a link with an influence on the vertex,
		if (lWeightSum != 0.0)
		{
			lDQClusterDeformation[i].Normalize();
			lDstVertex = lDQClusterDeformation[i].Deform(lDstVertex);
			
			if (lClusterMode == FbxCluster::eNormalize)
			{
				// In the normalized link mode, a vertex is always totally influenced by the links.
				lDstVertex /= lWeightSum;
			}
			else if (lClusterMode == FbxCluster::eTotalOne)
			{
				// In the total 1 link mode, a vertex can be partially influenced by the links.
				lSrcVertex *= (1.0 - lWeightSum);
				lDstVertex += lSrcVertex;
			}
		}
	}
	
	delete [] lDQClusterDeformation;
	delete [] lClusterWeight;
}

// Deform the vertex array according to the links contained in the mesh and the skinning type.
void ModelManager::ComputeSkinDeformation(FbxAMatrix & pGlobalPosition, FbxMesh * pFbxMesh, FbxTime & pTime,
										 FbxVector4 * pVertexArray, FbxPose * pPose)
{
	FbxSkin * lSkinDeformer = (FbxSkin *)pFbxMesh->GetDeformer(0, FbxDeformer::eSkin);
	FbxSkin::EType lSkinningType = lSkinDeformer->GetSkinningType();
	
	if (lSkinningType == FbxSkin::eLinear || lSkinningType == FbxSkin::eRigid)
	{
		ComputeLinearDeformation(pGlobalPosition, pFbxMesh, pTime, pVertexArray, pPose);
	}
	else if (lSkinningType == FbxSkin::eDualQuaternion)
	{
		ComputeDualQuaternionDeformation(pGlobalPosition, pFbxMesh, pTime, pVertexArray, pPose);
	}
	else if (lSkinningType == FbxSkin::eBlend)
	{
		const int lVertexCount = pFbxMesh->GetControlPointsCount();
		
		FbxVector4* lVertexArrayLinear = new FbxVector4[lVertexCount];
		memcpy(lVertexArrayLinear, pFbxMesh->GetControlPoints(), lVertexCount * sizeof(FbxVector4));
		
		FbxVector4* lVertexArrayDQ = new FbxVector4[lVertexCount];
		memcpy(lVertexArrayDQ, pFbxMesh->GetControlPoints(), lVertexCount * sizeof(FbxVector4));
		
		ComputeLinearDeformation(pGlobalPosition, pFbxMesh, pTime, lVertexArrayLinear, pPose);
		ComputeDualQuaternionDeformation(pGlobalPosition, pFbxMesh, pTime, lVertexArrayDQ, pPose);
		
		// To blend the skinning according to the blend weights
		// Final vertex = DQSVertex * blend weight + LinearVertex * (1- blend weight)
		// DQSVertex: vertex that is deformed by dual quaternion skinning method;
		// LinearVertex: vertex that is deformed by classic linear skinning method;
		int lBlendWeightsCount = lSkinDeformer->GetControlPointIndicesCount();
		for (int lBWIndex = 0; lBWIndex<lBlendWeightsCount; ++lBWIndex)
		{
			double lBlendWeight = lSkinDeformer->GetControlPointBlendWeights()[lBWIndex];
			pVertexArray[lBWIndex] = lVertexArrayDQ[lBWIndex] * lBlendWeight + lVertexArrayLinear[lBWIndex] * (1 - lBlendWeight);
		}
	}
}

void ModelManager::AddAnimatedMesh(Model & model, FbxNode * pFbxNode, FbxAnimLayer * pFbxAnimLayer, FbxTime & pTime, FbxAMatrix & pGlobalPosition, FbxPose * pPose)
{
	FbxMesh* pFbxMesh = pFbxNode->GetMesh();
	const int lVertexCount = pFbxMesh->GetControlPointsCount();
	// No vertex to draw.
	if (lVertexCount == 0)
		return;
	
	// If it has some defomer connection, update the vertices position
	const bool bHasVertexCache = pFbxMesh->GetDeformerCount(FbxDeformer::eVertexCache) &&
	(static_cast<FbxVertexCacheDeformer*>(pFbxMesh->GetDeformer(0, FbxDeformer::eVertexCache)))->IsActive();
	const bool bHasShape = pFbxMesh->GetShapeCount() > 0;
	const bool bHasSkin = pFbxMesh->GetDeformerCount(FbxDeformer::eSkin) > 0;
	const bool bHasDeformation = bHasVertexCache || bHasShape || bHasSkin;
	
	if (pFbxAnimLayer && bHasDeformation)
	{
		FbxVector4* pVertexArray = new FbxVector4[lVertexCount];
		memcpy(pVertexArray, pFbxMesh->GetControlPoints(), lVertexCount * sizeof(FbxVector4));
		
		// Active vertex cache deformer will overwrite any other deformer
		if (bHasVertexCache)
		{
			ReadVertexCacheData(pFbxMesh, pTime, pVertexArray);
		}
		else
		{
			if (bHasShape)
			{
				// Deform the vertex array with the shapes.
				ComputeShapeDeformation(pFbxMesh, pTime, pFbxAnimLayer, pVertexArray);
			}
			
			//we need to get the number of clusters
			const int lSkinCount = pFbxMesh->GetDeformerCount(FbxDeformer::eSkin);
			int lClusterCount = 0;
			for (int lSkinIndex = 0; lSkinIndex < lSkinCount; ++lSkinIndex)
			{
				lClusterCount += ((FbxSkin *)(pFbxMesh->GetDeformer(lSkinIndex, FbxDeformer::eSkin)))->GetClusterCount();
			}
			if (lClusterCount)
			{
				// Deform the vertex array with the skin deformer.
				ComputeSkinDeformation(pGlobalPosition, pFbxMesh, pTime, pVertexArray, pPose);
			}
		}
		
		const bool bHasNormal = pFbxMesh->GetElementNormalCount() > 0 && pFbxMesh->GetElementNormal(0)->GetMappingMode() != FbxGeometryElement::eNone;
		const int lNormalCount = pFbxMesh->GetPolygonCount() * 3;
		FbxVector4* pNormalArray = bHasNormal ? new FbxVector4[lNormalCount] : NULL;
		RecalculateMeshNormals(pFbxMesh, pVertexArray, pNormalArray);
		
		AddMesh(model, pFbxNode, pGlobalPosition, pVertexArray, pNormalArray);
		
		delete [] pNormalArray;
		delete [] pVertexArray;
	}
	else
	{
		AddMesh(model, pFbxNode, pGlobalPosition, pFbxMesh->GetControlPoints(), NULL);
	}
	
	//if (lMeshCache)
	//{
	//	lMeshCache->BeginDraw(pShadingMode);
	//	const int lSubMeshCount = lMeshCache->GetSubMeshCount();
	//	for (int lIndex = 0; lIndex < lSubMeshCount; ++lIndex)
	//	{
	//		if (pShadingMode == SHADING_MODE_SHADED)
	//		{
	//			const FbxSurfaceMaterial * lMaterial = pFbxNode->GetMaterial(lIndex);
	//			if (lMaterial)
	//			{
	//				const MaterialCache * lMaterialCache = static_cast<const MaterialCache *>(lMaterial->GetUserDataPtr());
	//				if (lMaterialCache)
	//				{
	//					lMaterialCache->SetCurrentMaterial();
	//				}
	//			}
	//			else
	//			{
	//				// Draw green for faces without material
	//				MaterialCache::SetDefaultMaterial();
	//			}
	//		}
	
	//		lMeshCache->Draw(lIndex, pShadingMode);
	//	}
	//	lMeshCache->EndDraw();
	//}
	//else
	//{
	//	// OpenGL driver is too lower and use Immediate Mode
	//	glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
	//	const int numPolygons = pFbxMesh->GetPolygonCount();
	//	for (int lPolygonIndex = 0; lPolygonIndex < numPolygons; lPolygonIndex++)
	//	{
	//		const int lVerticeCount = pFbxMesh->GetPolygonSize(lPolygonIndex);
	//		glBegin(GL_LINE_LOOP);
	//		for (int lVerticeIndex = 0; lVerticeIndex < lVerticeCount; lVerticeIndex++)
	//		{
	//			glVertex3dv((GLdouble *)pVertexArray[pFbxMesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex)]);
	//		}
	//		glEnd();
	//	}
	//}
}

void ModelManager::CollectMeshes(Model & model, FbxNode * pFbxNode, FbxAnimLayer * pFbxAnimLayer, FbxTime & time)
{
	if (pFbxNode->GetVisibility())
	{
		FbxNodeAttribute* pNodeAttribute = pFbxNode->GetNodeAttribute();
		if (pNodeAttribute &&
			(pNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh ||
			pNodeAttribute->GetAttributeType() == FbxNodeAttribute::eNurbs ||
			pNodeAttribute->GetAttributeType() == FbxNodeAttribute::eNurbsSurface ||
			pNodeAttribute->GetAttributeType() == FbxNodeAttribute::ePatch))
		{
			FbxAMatrix lGlobalPosition = pFbxNode->EvaluateGlobalTransform(time);
			FbxAMatrix lGeometryOffset = GetGeometry(pFbxNode);
			FbxAMatrix lGlobalOffPosition = lGlobalPosition * lGeometryOffset;
			AddAnimatedMesh(model, pFbxNode, pFbxAnimLayer, time, lGlobalOffPosition, NULL);
			//AddMesh(builder, pFbxNode, lGlobalOffPosition, pFbxNode->GetMesh()->GetControlPoints());
		}
	}

	const int lChildCount = pFbxNode->GetChildCount();
	for (int lChildIndex = 0; lChildIndex < lChildCount; ++lChildIndex)
	{
		CollectMeshes(model, pFbxNode->GetChild(lChildIndex), pFbxAnimLayer, time);
	}
}

