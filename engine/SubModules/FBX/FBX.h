#pragma once

#include "stdafx.h"

class VertInsertionList;
class KeyFrameList;

class FBX
{
public:
	FBX();
	FBX(const char* filename);
	~FBX();

public:
	void ExportToModel(ModelData& modelData);
	static void CleanupModel(ModelData &modelData);			//	this needs to be called on ModelData once you're done with it if you call ExportToModel on it
	void ExportToAnimation(AnimationData& animationData, bool localBoneSpace = false);
	static void CleanupAnim(AnimationData &animData);		//	this needs to be called on AnimationData once you're done with it if you call ExportToAnimation on it
	bool HasCharAnimation();

	void WriteToFbx(const char* filename);
	void AddAnimDataToScene(const AnimationData& animData);

private:
	void InitKeyFrameListFromAnimCurve(KeyFrameList* keyFrameList);
	void InitKeyFrameListFromDuration(KeyFrameList* keyFrameList);
	bool SetCurrentAnimStack(int pIndex);
	void BuildSkeletonList();
	void BuildMaterialList();
	void BuildMeshList();
	int GetVertType(FbxMesh* pMesh);
	void BuildSkeletonVertData(FbxMesh* pMesh, VertInsertionList* vertMap, int charAnimOffset);
	void BuildMaterials(ModelData& modelData);
	void BuildCollisionMesh(ModelData& modelData);
	void BuildBones(ModelData& modelData);
	void BuildBones(AnimationData& animationData);
	void BuildMesh(FbxMesh* pMesh, MeshData* meshDataOut);

private:
	FbxManager*						m_pSdkManager;
    FbxScene*						m_pScene;
    FbxAnimLayer*					m_pCurrentAnimLayer;

	FbxArray<FbxString*>			m_animStackNameArray;
	FbxArray<FbxNode*>				m_cameraArray;
    FbxArray<FbxPose*>				m_poseArray;

	FbxTime							m_startTime;
	FbxTime							m_stopTime;

	FbxArray<FbxSkeleton*>			m_skeletonList;
	FbxArray<FbxCluster*>			m_clusterList;
	FbxArray<FbxSurfaceMaterial*>	m_materialList;
	FbxArray<FbxMesh*>				m_meshList;
	FbxArray<FbxMesh*>				m_collisionMeshList;
};