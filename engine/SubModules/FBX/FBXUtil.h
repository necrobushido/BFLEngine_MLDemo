#pragma once

#include "stdafx.h"

inline void InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene)
{
	//The first thing to do is to create the FBX Manager which is the object allocator for almost all the classes in the SDK
	pManager = FbxManager::Create();
	if( !pManager )
	{
		FBXSDK_printf("Error: Unable to create FBX Manager!\n");
		exit(1);
	}
	//else FBXSDK_printf("Autodesk FBX SDK version %s\n", pManager->GetVersion());

	//Create an IOSettings object. This object holds all import/export settings.
	FbxIOSettings* ios = FbxIOSettings::Create(pManager, IOSROOT);
	pManager->SetIOSettings(ios);

	//Load plugins from the executable directory (optional)
	FbxString lPath = FbxGetApplicationDirectory();
	pManager->LoadPluginsDirectory(lPath.Buffer());

	//Create an FBX scene. This object holds most objects imported/exported from/to files.
	pScene = FbxScene::Create(pManager, "My Scene");
	if( !pScene )
	{
		FBXSDK_printf("Error: Unable to create FBX scene!\n");
		exit(1);
	}
}

inline void DestroySdkObjects(FbxManager* pManager, bool pExitStatus)
{
	//Delete the FBX Manager. All the objects that have been allocated using the FBX Manager and that haven't been explicitly destroyed are also automatically destroyed.
	if( pManager ) pManager->Destroy();
	if( pExitStatus ) FBXSDK_printf("Program Success!\n");
}

// Find all the cameras under this node recursively.
inline void FillCameraArrayRecursive(FbxNode* pNode, FbxArray<FbxNode*>& pCameraArray)
{
    if (pNode)
    {
        if (pNode->GetNodeAttribute())
        {
            if (pNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eCamera)
            {
                pCameraArray.Add(pNode);
            }
        }

        const int lCount = pNode->GetChildCount();
        for (int i = 0; i < lCount; i++)
        {
            FillCameraArrayRecursive(pNode->GetChild(i), pCameraArray);
        }
    }
}

// Find all the cameras in this scene.
inline void FillCameraArray(FbxScene* pScene, FbxArray<FbxNode*>& pCameraArray)
{
    pCameraArray.Clear();

    FillCameraArrayRecursive(pScene->GetRootNode(), pCameraArray);
}

// Find all poses in this scene.
inline void FillPoseArray(FbxScene* pScene, FbxArray<FbxPose*>& pPoseArray)
{
    const int lPoseCount = pScene->GetPoseCount();

    for (int i=0; i < lPoseCount; ++i)
    {
        pPoseArray.Add(pScene->GetPose(i));
    }
}

//
inline void ComputeClusterInitTransform(	FbxMesh *pMesh,
											FbxCluster *pCluster, 
											FbxAMatrix &pVertexTransformMatrix,
											FbxPose *pPose)
{
	FbxCluster::ELinkMode lClusterMode = pCluster->GetLinkMode();

	FbxAMatrix lReferenceGlobalInitPosition;
	FbxAMatrix lClusterGlobalInitPosition;
	FbxAMatrix lReferenceGeometry;
	FbxAMatrix lClusterRelativeInitPosition;
	
	if (lClusterMode == FbxCluster::eAdditive && pCluster->GetAssociateModel())
	{
		//	do this later
		assert(0);
	}
	else
	{
		pCluster->GetTransformMatrix(lReferenceGlobalInitPosition);															//	transform of mesh at binding time
		lReferenceGeometry = GetGeometry(pMesh->GetNode());
		lReferenceGlobalInitPosition *= lReferenceGeometry;																	//	transform of mesh at binding time considering geometric offset
		pCluster->GetTransformLinkMatrix(lClusterGlobalInitPosition);														//	transform of bone at binding time			
		lClusterRelativeInitPosition = lClusterGlobalInitPosition.Inverse() * lReferenceGlobalInitPosition;					//	transform of bone relative to mesh at binding time

		//pVertexTransformMatrix = lClusterRelativeInitPosition;
		pVertexTransformMatrix = lClusterGlobalInitPosition.Inverse();
	}
}

//
inline void ComputeClusterCurrentTransform(	FbxMesh* pMesh,
											FbxCluster* pCluster, 
											FbxAMatrix& pVertexTransformMatrix,
											FbxTime pTime, 
											FbxPose* pPose)
{
	FbxCluster::ELinkMode	lClusterMode = pCluster->GetLinkMode();
	FbxNode					*pMeshNode = pMesh->GetNode();

	FbxAMatrix lReferenceGlobalCurrentPosition;
	FbxAMatrix lAssociateGlobalCurrentPosition;
	FbxAMatrix lClusterGlobalCurrentPosition;

	FbxAMatrix lReferenceGeometry;
	FbxAMatrix lAssociateGeometry;
	FbxAMatrix lClusterGeometry;

	FbxAMatrix lClusterRelativeCurrentPositionInverse;

	FbxAMatrix	globalPosition;
	FbxAMatrix	globalBasePosition = GetGlobalPosition(pMeshNode, pTime, pPose, NULL);
	lReferenceGeometry = GetGeometry(pMeshNode);
	globalPosition = globalBasePosition * lReferenceGeometry;
	
	if (lClusterMode == FbxCluster::eAdditive && pCluster->GetAssociateModel())
	{
		//	do this later
		assert(0);
	}
	else
	{
		lReferenceGlobalCurrentPosition = globalPosition;																	//	transform of mesh now (geometric offset should be built in)
		lClusterGlobalCurrentPosition = GetGlobalPosition(pCluster->GetLink(), pTime, pPose);								//	transform of bone now
		lClusterRelativeCurrentPositionInverse = lReferenceGlobalCurrentPosition.Inverse() * lClusterGlobalCurrentPosition;	//	transform of bone relative to mesh now

		//pVertexTransformMatrix = lClusterRelativeCurrentPositionInverse;
		pVertexTransformMatrix = lClusterGlobalCurrentPosition;
	}
}

//Compute the transform matrix that the cluster will transform the vertex.
inline void ComputeClusterDeformation(	FbxMesh* pMesh,
										FbxCluster* pCluster, 
										FbxAMatrix& pVertexTransformMatrix,
										FbxTime pTime, 
										FbxPose* pPose)
{
	FbxCluster::ELinkMode	lClusterMode = pCluster->GetLinkMode();
	FbxNode					*pMeshNode = pMesh->GetNode();

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

	FbxAMatrix	globalPosition;
	FbxAMatrix	globalBasePosition = GetGlobalPosition(pMeshNode, pTime, pPose, NULL);		
	lReferenceGeometry = GetGeometry(pMeshNode);
	globalPosition = globalBasePosition * lReferenceGeometry;
	
	if (lClusterMode == FbxCluster::eAdditive && pCluster->GetAssociateModel())
	{
		pCluster->GetTransformAssociateModelMatrix(lAssociateGlobalInitPosition);
		// Geometric transform of the model
		lAssociateGeometry = GetGeometry(pCluster->GetAssociateModel());
		lAssociateGlobalInitPosition *= lAssociateGeometry;
		lAssociateGlobalCurrentPosition = GetGlobalPosition(pCluster->GetAssociateModel(), pTime, pPose);

		pCluster->GetTransformMatrix(lReferenceGlobalInitPosition);
		// Multiply lReferenceGlobalInitPosition by Geometric Transformation
		lReferenceGlobalInitPosition *= lReferenceGeometry;
		lReferenceGlobalCurrentPosition = globalPosition;

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
		//	original code
		//pCluster->GetTransformMatrix(lReferenceGlobalInitPosition);															//	transform of mesh at binding time
		//lReferenceGlobalCurrentPosition = globalPosition;																	//	transform of mesh now (geometric offset should be built in)
		//lReferenceGlobalInitPosition *= lReferenceGeometry;																	//	transform of mesh at binding time considering geometric offset

		//pCluster->GetTransformLinkMatrix(lClusterGlobalInitPosition);														//	transform of bone at binding time
		//lClusterGlobalCurrentPosition = GetGlobalPosition(pCluster->GetLink(), pTime, pPose);								//	transform of bone now

		//lClusterRelativeInitPosition = lClusterGlobalInitPosition.Inverse() * lReferenceGlobalInitPosition;					//	transform of bone relative to mesh at binding time
		//lClusterRelativeCurrentPositionInverse = lReferenceGlobalCurrentPosition.Inverse() * lClusterGlobalCurrentPosition;	//	transform of bone relative to mesh now
		//pVertexTransformMatrix = lClusterRelativeCurrentPositionInverse * lClusterRelativeInitPosition;						//	delta transform

		//	current code : the mesh transform is baked into the verts (and assumed to be the same at all times)
		pCluster->GetTransformLinkMatrix(lClusterGlobalInitPosition);														//	transform of bone at binding time
		lClusterGlobalCurrentPosition = GetGlobalPosition(pCluster->GetLink(), pTime, pPose);								//	transform of bone now
		pVertexTransformMatrix = lClusterGlobalCurrentPosition * lClusterGlobalInitPosition.Inverse();						//	delta transform
	}
}

 // Get specific property value and connected texture if any.
// Value = Property value * Factor property value (if no factor property, multiply by 1).
inline FbxDouble3 GetMaterialProperty(	const FbxSurfaceMaterial * pMaterial,
										const char * pPropertyName,
										const char * pFactorPropertyName,
										char *pTextureName)
{
    FbxDouble3 lResult(0, 0, 0);
    const FbxProperty lProperty = pMaterial->FindProperty(pPropertyName);
    const FbxProperty lFactorProperty = pMaterial->FindProperty(pFactorPropertyName);
    if (lProperty.IsValid() && lFactorProperty.IsValid())
    {
        lResult = lProperty.Get<FbxDouble3>();
        double lFactor = lFactorProperty.Get<FbxDouble>();
        if (lFactor != 1)
        {
            lResult[0] *= lFactor;
            lResult[1] *= lFactor;
            lResult[2] *= lFactor;
        }
    }

    if (lProperty.IsValid())
    {
        const int lTextureCount = lProperty.GetSrcObjectCount<FbxFileTexture>();
        if (lTextureCount)
        {
            const FbxFileTexture* lTexture = lProperty.GetSrcObject<FbxFileTexture>();
			strcpy(pTextureName, lTexture->GetRelativeFileName());
        }
    }

    return lResult;
}

inline void CopyToMtx44FromFBX(Mtx44 &out, const FbxAMatrix &in)
{
	for(int a = 0; a < 4; a++)
	{
		for(int b = 0; b < 4; b++)
		{
			out.m[a][b] = (f32)in[a][b];
		}
	}
}

inline void CopyToMtx43FromFBX(Mtx43 &out, const FbxAMatrix &in)
{
	for(int a = 0; a < Mtx43::kRowCount; a++)
	{
		for(int b = 0; b < Mtx43::kColumnCount; b++)
		{
			out.m[a][b] = (f32)in[a][b];
		}
	}
}

inline void GetExtension(const char *originalFilename, char *outputExtension)
{
	const char	*lastPeriod = strrchr(originalFilename, '.');
	strcpy(outputExtension, &lastPeriod[1]);
}

inline void StripPath(char *filenameWithPath)
{
	const char	*lastBackSlash = strrchr(filenameWithPath, '\\');
	//const char	*lastForwardSlash = strrchr(filenameWithPath, '/');
	if( lastBackSlash )
	{
		char		filenameWithoutPath[MAX_PATH];
		strcpy(filenameWithoutPath, &lastBackSlash[1]);
		strcpy(filenameWithPath, filenameWithoutPath);
	}
}

inline void MakeStringLowerCase(char *string)
{
	size_t	length = strlen(string);
	for(size_t i = 0; i < length; i++)
	{
		string[i] = tolower(string[i]);
	}
}

inline bool IsSkeletonNode(const FbxNode *pNode)
{
	bool	returnValue = false;
	int		attributeCount = pNode->GetNodeAttributeCount();
	for(int attributeIndex = 0; attributeIndex < attributeCount && !returnValue; attributeIndex++)
	{
		const FbxNodeAttribute	*pNodeAttribute = pNode->GetNodeAttributeByIndex(attributeIndex);

		if( pNodeAttribute )
		{
			if( pNodeAttribute->GetAttributeType() == FbxNodeAttribute::eSkeleton )
			{
				returnValue = true;
			}
		}
	}

	return returnValue;
}

inline FbxSkeleton *GetFirstSkeletonAttribute(FbxNode *pNode)
{
	FbxSkeleton	*returnValue = NULL;
	int			attributeCount = pNode->GetNodeAttributeCount();
	for(int attributeIndex = 0; attributeIndex < attributeCount && !returnValue; attributeIndex++)
	{
		FbxNodeAttribute	*pNodeAttribute = pNode->GetNodeAttributeByIndex(attributeIndex);

		if( pNodeAttribute )
		{
			if( pNodeAttribute->GetAttributeType() == FbxNodeAttribute::eSkeleton )
			{
				returnValue = (FbxSkeleton*)pNodeAttribute;
			}
		}
	}

	return returnValue;
}