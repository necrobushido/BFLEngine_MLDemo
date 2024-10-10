#include "FBX.h"

#include "GetPosition.h"
#include "VertInsertionList.h"
#include "IndexList.h"
#include "KeyFrameList.h"
#include "FBXUtil.h"
#include "MathNamespace.h"
#include "EngineArray.h"
#include "MathConstants.h"

FBX::FBX():
	m_pSdkManager(NULL),
	m_pScene(NULL),
	m_pCurrentAnimLayer(NULL)
{
	// Create the FBX SDK manager which is the object allocator for almost 
	// all the classes in the SDK and create the scene.
	InitializeSdkObjects(m_pSdkManager, m_pScene);

	assert(m_pSdkManager);

	//	Convert Axis System
	FbxAxisSystem	sceneAxisSystem = m_pScene->GetGlobalSettings().GetAxisSystem();
	FbxAxisSystem	ourAxisSystem = FbxAxisSystem::OpenGL;
	if( sceneAxisSystem != ourAxisSystem )
	{
		ourAxisSystem.ConvertScene(m_pScene);			//	this only really changes the root node from the sound of things
		//ourAxisSystem.DeepConvertScene(m_pScene);		//	supposedly this goes through the full hierarchy and performs conversions
	}
}

FBX::FBX(const char* filename):
	m_pSdkManager(NULL),
	m_pScene(NULL),
	m_pCurrentAnimLayer(NULL)
{
	// Create the FBX SDK manager which is the object allocator for almost 
	// all the classes in the SDK and create the scene.
	InitializeSdkObjects(m_pSdkManager, m_pScene);

	assert(m_pSdkManager);
	
	// Create the importer.
	const int	kDaeFileFormat = 4;	//	HACK : determined by using the debugger, probably subject to change on new versions of FBX; would be better if I knew a string description as below
	int	fileFormat = -1;
	FbxImporter*	pImporter = FbxImporter::Create(m_pSdkManager, "");
	if( !m_pSdkManager->GetIOPluginRegistry()->DetectReaderFileFormat(filename, fileFormat) )
	{
		char	fileExtension[MAX_PATH];
		GetExtension(filename, fileExtension);
		if( strcmp(fileExtension, "daem") == 0 || strcmp(fileExtension, "daea") == 0 )
		{
			fileFormat = kDaeFileFormat;
		}
		else
		{
			// Unrecognizable file format. Try to fall back to FbxImporter::eFBX_BINARY
			fileFormat = m_pSdkManager->GetIOPluginRegistry()->FindReaderIDByDescription("FBX binary (*.fbx)");

			//	consider using FindReaderIDByExtension with the modified extension instead
		}
	}

	// Initialize the importer by providing a filename.
	bool	importInitSuccess = pImporter->Initialize(filename, fileFormat);
	if( !importInitSuccess )
	{
		char	buffer[256];
		sprintf(buffer, "Call to FbxImporter::Initialize() failed.\nError returned: %s\n\n", pImporter->GetStatus().GetErrorString());
		assert(importInitSuccess);
    }

	bool	importSuccess = pImporter->Import(m_pScene);
	assert(importSuccess);

	//	Convert Axis System to what is used in this example, if needed
	FbxAxisSystem	sceneAxisSystem = m_pScene->GetGlobalSettings().GetAxisSystem();
	//FbxAxisSystem	ourAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
	FbxAxisSystem	ourAxisSystem = FbxAxisSystem::OpenGL;
	if( sceneAxisSystem != ourAxisSystem )
	{
		ourAxisSystem.ConvertScene(m_pScene);			//	this only really changes the root node from the sound of things
		//ourAxisSystem.DeepConvertScene(m_pScene);		//	supposedly this goes through the full hierarchy and performs conversions
	}

	//	Convert Unit System to what is used in this example, if needed
	//FbxSystemUnit	sceneSystemUnit = m_pScene->GetGlobalSettings().GetSystemUnit();
	//if( sceneSystemUnit.GetScaleFactor() != 1.0 )
	//{
		//	The unit in this example is centimeter.
	//	FbxSystemUnit::cm.ConvertScene(m_pScene);
	//}

	//	Get the list of all the animation stack.
	m_pScene->FillAnimStackNameArray(m_animStackNameArray);

	//	Get the list of all the cameras in the scene.
	FillCameraArray(m_pScene, m_cameraArray);

	//
	bool					convertSuccess;
	FbxGeometryConverter	geomConverter(m_pSdkManager);

	//	Convert mesh, NURBS and patch into triangle mesh
	convertSuccess = geomConverter.Triangulate(m_pScene, true);
	assert(convertSuccess);

	//	Split meshes per material, so that we only have one material per mesh (for VBO support)
	//convertSuccess = geomConverter.SplitMeshesPerMaterial(m_pScene, true);
	//assert(convertSuccess);

	//	Get the list of pose in the scene
	FillPoseArray(m_pScene, m_poseArray);

	//	Destroy the importer to release the file.
	pImporter->Destroy();
	pImporter = NULL;

	//
	//SetCurrentAnimStack(0);
	SetCurrentAnimStack(m_animStackNameArray.GetCount()-1);	//	the Mixamo animations seem to have more than one anim stack, and the last one is the one that actually has the animation?

	//
	BuildMeshList();
	BuildMaterialList();
	BuildSkeletonList();
}

FBX::~FBX()
{
	FbxArrayDelete(m_animStackNameArray);

	// Delete the FBX SDK manager. All the objects that have been allocated 
	// using the FBX SDK manager and that haven't been explicitly destroyed 
    // are automatically destroyed at the same time.
	DestroySdkObjects(m_pSdkManager, true);
}

bool FBX::SetCurrentAnimStack(int pIndex)
{
	const int	animStackCount = m_animStackNameArray.GetCount();
	if( !animStackCount || pIndex >= animStackCount)
	{
		return false;
	}

	//	select the base layer from the animation stack
	FbxAnimStack	*currentAnimationStack = m_pScene->FindMember<FbxAnimStack>(m_animStackNameArray[pIndex]->Buffer());
	if( currentAnimationStack == NULL )
	{
	   //	this is a problem. The anim stack should be found in the scene!
	   return false;
	}

	//	we assume that the first animation layer connected to the animation stack is the base layer
	//	(this is the assumption made in the FBXSDK)
	m_pCurrentAnimLayer = currentAnimationStack->GetMember<FbxAnimLayer>();
	
	//	before SDK update
	//m_pScene->GetEvaluator()->SetContext(currentAnimationStack);

	//	after SDK update
	m_pScene->SetCurrentAnimationStack(currentAnimationStack);

	FbxTakeInfo	*currentTakeInfo = m_pScene->GetTakeInfo(*(m_animStackNameArray[pIndex]));
	if( currentTakeInfo )
	{
		m_startTime = currentTakeInfo->mLocalTimeSpan.GetStart();
		m_stopTime = currentTakeInfo->mLocalTimeSpan.GetStop();
	}
	else
	{
		// Take the time line value
		FbxTimeSpan	timeLineTimeSpan;
		m_pScene->GetGlobalSettings().GetTimelineDefaultTimeSpan(timeLineTimeSpan);

		m_startTime = timeLineTimeSpan.GetStart();
		m_stopTime  = timeLineTimeSpan.GetStop();
	}

	return true;
}

bool FBX::HasCharAnimation()
{
	/*int	clusterCount = 0;
	for(int i = 0; i < m_meshList.GetCount(); i++)
	{
		FbxMesh		*pMesh = m_meshList[i];
			
		int	skinCount = pMesh->GetDeformerCount(FbxDeformer::eSkin);
		
		for(int j = 0; j < skinCount; j++)
		{
			clusterCount += ((FbxSkin*)(pMesh->GetDeformer(j, FbxDeformer::eSkin)))->GetClusterCount();
		}
	}

	return clusterCount > 0;*/

	return m_skeletonList.GetCount() > 0;
}

int SkeletonCompareFunc(const void* left, const void* right)
{
	const FbxSkeleton	*pLeftSkeleton = *((FbxSkeleton**)left);
	const FbxSkeleton	*pRightSkeleton = *((FbxSkeleton**)right);

	int	returnValue = strcmp(pLeftSkeleton->GetNode()->GetName(), pRightSkeleton->GetNode()->GetName());
	assert(returnValue != 0);	//	should not have skeleton nodes with dupe names in the file, but it is possible for an FBX file to contain them

	return returnValue;
}

void FBX::BuildSkeletonList()
{
	for(int i = 0; i < m_pScene->GetNodeCount(); i++)
	{
		FbxNode				*pNode = m_pScene->GetNode(i);
		int					attributeCount = pNode->GetNodeAttributeCount();
		for(int attributeIndex = 0; attributeIndex < attributeCount; attributeIndex++)
		{
			FbxNodeAttribute	*pNodeAttribute = pNode->GetNodeAttributeByIndex(attributeIndex);

			if( pNodeAttribute )
			{
				if( pNodeAttribute->GetAttributeType() == FbxNodeAttribute::eSkeleton )
				{
					FbxSkeleton	*pSkeleton = (FbxSkeleton*)pNode->GetNodeAttribute();

					m_skeletonList.AddUnique(pSkeleton);
				}
			}
		}
	}

	if( m_skeletonList.Size() > 0 )
	{
		qsort(m_skeletonList.GetArray(), m_skeletonList.Size(), sizeof(FbxSkeleton*), SkeletonCompareFunc);

		//
		m_clusterList.Resize(m_skeletonList.Size());

		for(int i = 0; i < m_skeletonList.Size(); i++)
		{
			m_clusterList.SetAt(i, NULL);
		}
		
		for(int i = 0; i < m_meshList.GetCount(); i++)
		{
			FbxMesh		*pMesh = m_meshList[i];
			FbxNode		*pNode = pMesh->GetNode();

			int	skinCount = pMesh->GetDeformerCount(FbxDeformer::eSkin);

			for(int skinIdx = 0; skinIdx < skinCount; skinIdx++)
			{
				FbxSkin	*thisSkin = ((FbxSkin*)pMesh->GetDeformer(skinIdx, FbxDeformer::eSkin));
				int		clusterCount = thisSkin->GetClusterCount();
				for(int clusterIdx = 0; clusterIdx < clusterCount; clusterIdx++)
				{
					FbxCluster	*cluster = thisSkin->GetCluster(clusterIdx);
					if( !cluster->GetLink() )
						continue;

					FbxNodeAttribute	*pNodeAttribute = cluster->GetLink()->GetNodeAttribute();
					assert(pNodeAttribute->GetAttributeType() == FbxNodeAttribute::eSkeleton);
					int					boneIndex = m_skeletonList.Find((FbxSkeleton*)pNodeAttribute);
					assert(boneIndex >= 0);

					m_clusterList.SetAt(boneIndex, cluster);
				}
			}
		}
	}
}

void FBX::BuildMaterialList()
{
	for(int i = 0; i < m_meshList.GetCount(); i++)
	{
		FbxMesh					*pMesh = m_meshList[i];
		FbxNode					*pNode = pMesh->GetNode();
		int						materialCount = pNode->GetMaterialCount();
		FbxLayerElementMaterial	*materialElement = pMesh->GetLayer(0)->GetMaterials();

		for(int materialIdx = 0; materialIdx < materialCount; materialIdx++)
		{
			FbxSurfaceMaterial	*pMaterial = pNode->GetMaterial(materialIdx);

			int	materialUsedCount = 0;
			for(int polyIndex = 0; polyIndex < pMesh->GetPolygonCount(); polyIndex++)
			{
				FbxSurfaceMaterial	*polygonMaterial = NULL;
				if( materialElement->GetMappingMode() == FbxLayerElement::eAllSame )
				{
					int	polyMaterialIndex = materialElement->GetIndexArray()[0];
					polygonMaterial = pNode->GetMaterial(polyMaterialIndex);
				}
				else
				if( materialElement->GetMappingMode() == FbxLayerElement::eByPolygon )
				{
					int	polyMaterialIndex = materialElement->GetIndexArray()[polyIndex];
					polygonMaterial = pNode->GetMaterial(polyMaterialIndex);				
				}
				else
				{
					assert(0);
				}

				if( polygonMaterial != pMaterial )
					continue;

				materialUsedCount++;
			}

			if( materialUsedCount > 0 )
			{
				m_materialList.AddUnique(pMaterial);
			}
		}
	}
}

void FBX::BuildMeshList()
{
	for(int i = 0; i < m_pScene->GetNodeCount(); i++)
	{
		FbxNode				*pNode = m_pScene->GetNode(i);
		int					attributeCount = pNode->GetNodeAttributeCount();
		for(int attributeIndex = 0; attributeIndex < attributeCount; attributeIndex++)
		{
			FbxNodeAttribute	*pNodeAttribute = pNode->GetNodeAttributeByIndex(attributeIndex);

			if( pNodeAttribute )
			{
				if( pNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh )
				{
					FbxMesh	*pMesh = (FbxMesh*)pNodeAttribute;
					char	nameBuffer[256];
					strcpy(nameBuffer, pNode->GetName());
					MakeStringLowerCase(nameBuffer);
					bool	isCollisionMesh = strstr(nameBuffer, "collision") != NULL;

					if( isCollisionMesh )
					{
						m_collisionMeshList.AddUnique(pMesh);
					}
					else
					{
						m_meshList.AddUnique(pMesh);
					}					
				}
			}
		}
	}
}

int FBX::GetVertType(FbxMesh* pMesh)
{
	//	the mesh must have position information
	u32	vertexFlags = kChannel_Position;

	FbxLayer	*layer0 = pMesh->GetLayer(0);

	//	determine if the mesh is textured
    {
		FbxLayerElement::EMappingMode	mappingMode = FbxLayerElement::eNone;

		int	materialCount = pMesh->GetNode()->GetMaterialCount();

		if( materialCount > 0 )
		{
			//	it's possible that we'll need to check every layer instead of just 0 for UVs
			if( layer0 && 
				layer0->GetUVs() )
			{
				mappingMode = layer0->GetUVs()->GetMappingMode();
			}

			if( mappingMode == FbxLayerElement::eByPolygonVertex ||
				mappingMode == FbxLayerElement::eByControlPoint )
			{
				vertexFlags |= kChannel_Texcoord;
			}
		}
    }

	//	check for vert colors
	{
		//	it's possible that we'll need to check every layer instead of just 0
		FbxLayerElementVertexColor	*vertexColorElement = layer0->GetVertexColors();
        if( vertexColorElement )
        {
			if( vertexColorElement->GetMappingMode() == FbxLayerElement::eByControlPoint ||
				vertexColorElement->GetMappingMode() == FbxLayerElement::eByPolygonVertex )
			{
				vertexFlags |= kChannel_Color;
			}
		}
	}

	//	check for normals
	{
		//	Normals info should be only in layer0        
        if( layer0 )
        {
            //	get the normal element.
            FbxLayerElementNormal	*normalElement = layer0->GetNormals();
            if( normalElement )
            {
				if( normalElement->GetMappingMode() == FbxLayerElement::eByControlPoint ||
					normalElement->GetMappingMode() == FbxLayerElement::eByPolygonVertex )
				{
					vertexFlags |= kChannel_Normal;
				}
            }
        }
	}

	//	check for character animation
	{
		int	skinCount = pMesh->GetDeformerCount(FbxDeformer::eSkin);
		int	clusterCount = 0;
        for(int i = 0; i < skinCount; i++)
		{
            clusterCount += ((FbxSkin*)(pMesh->GetDeformer(i, FbxDeformer::eSkin)))->GetClusterCount();
		}

		if( clusterCount > 0 )
		{
			vertexFlags |= kChannel_CharacterAnimation_BoneIndices;
			vertexFlags |= kChannel_CharacterAnimation_BoneWeights;
		}
	}

	return vertexFlags;
}

void FBX::BuildSkeletonVertData(FbxMesh* pMesh, VertInsertionList* vertMap, int charAnimOffset)
{
	int	vertexCount = pMesh->GetControlPointsCount();
    int	skinCount = pMesh->GetDeformerCount(FbxDeformer::eSkin);

    for(int i = 0; i < skinCount; i++)
    {
		FbxSkin	*thisSkin = ((FbxSkin*)pMesh->GetDeformer(i, FbxDeformer::eSkin));
        int		clusterCount = thisSkin->GetClusterCount();
        for(int j = 0; j < clusterCount; j++)
        {
            FbxCluster	*cluster = thisSkin->GetCluster(j);
            if( !cluster->GetLink() )
                continue;

			FbxNodeAttribute	*pNodeAttribute = cluster->GetLink()->GetNodeAttribute();
			assert(pNodeAttribute->GetAttributeType() == FbxNodeAttribute::eSkeleton);
			int					boneIndex = m_skeletonList.Find((FbxSkeleton*)pNodeAttribute);
			assert(boneIndex >= 0);
			
			int					clusterVertCount = cluster->GetControlPointIndicesCount();

            for(int k = 0; k < clusterVertCount; k++) 
            {            
                int	clusterVertIndex = cluster->GetControlPointIndices()[k];

                //	Sometimes, the mesh can have less points than at the time of the skinning
                //	because a smooth operator was active when skinning but has been deactivated during export.
                if( clusterVertIndex >= vertexCount )
                    continue;

                f32	weight = (f32)cluster->GetControlPointWeights()[k];

                if( weight == 0.0f )
                {
                    continue;
                }

				//	find all verts in the vertMap associated with the clusterVertIndex (original control point index)
				//	those will be the verts used in the output bone in place of the original vert
				const VertInsertionList::Node	*currentVertNode = vertMap->m_pRootNode;
				int								finalVertIdx = 0;

				while(currentVertNode)
				{
					const IndexList::Node	*currentControlPointIndexNode = currentVertNode->m_controlPointIndices.m_pRootNode;
					while(currentControlPointIndexNode)
					{
						if( currentControlPointIndexNode->m_index == clusterVertIndex )
						{
							f32	*vertBoneIndices = (f32*)(&currentVertNode->m_vertData[charAnimOffset]);
							f32	*vertBoneWeights = (f32*)(&currentVertNode->m_vertData[charAnimOffset + sizeof(f32) * kNumFloatsPerCharAnimBoneIdx]);

							bool	found = false;
							int		smallestWeightIdx = 0;
							f32		smallestWeight = 0;
							for(int vertBoneArrayIdx = 0; vertBoneArrayIdx < kNumFloatsPerCharAnimBoneIdx && !found; vertBoneArrayIdx++)
							{
								if( vertBoneIndices[vertBoneArrayIdx] < 0 )
								{
									vertBoneIndices[vertBoneArrayIdx] = (f32)boneIndex;
									vertBoneWeights[vertBoneArrayIdx] = weight;
									found = true;
								}
								else
								{
									if( vertBoneWeights[vertBoneArrayIdx] < smallestWeight )
									{
										smallestWeight = vertBoneWeights[vertBoneArrayIdx];
										smallestWeightIdx = vertBoneArrayIdx;
									}
								}
							}

							//	we only use the 4 strongest weights
							//	look to see if we need to overwrite one
							if( !found )
							{
								if( weight > smallestWeight )
								{
									vertBoneIndices[smallestWeightIdx] = (f32)boneIndex;
									vertBoneWeights[smallestWeightIdx] = weight;
								}
							}	
						}

						currentControlPointIndexNode = currentControlPointIndexNode->m_pNext;
					}

					currentVertNode = currentVertNode->m_pNext;
					finalVertIdx++;
				}
            }
		}
	}
}

void FBX::BuildMaterials(ModelData& modelData)
{
	modelData.materialCount = m_materialList.GetCount();
	if( modelData.materialCount > 0 )
	{
		modelData.materials = new MaterialData[modelData.materialCount];
		memset(modelData.materials, 0, sizeof(MaterialData) * modelData.materialCount);

		for(int i = 0; i < m_materialList.GetCount(); i++)
		{
			FbxSurfaceMaterial			*pMaterial = m_materialList[i];

			const FbxDouble3			emissive = GetMaterialProperty(	pMaterial, 
																		FbxSurfaceMaterial::sEmissive, 
																		FbxSurfaceMaterial::sEmissiveFactor, 
																		modelData.materials[i].channels[MaterialData::eEmissive].texture);
			modelData.materials[i].channels[MaterialData::eEmissive].textureRefPad = 0;
			modelData.materials[i].channels[MaterialData::eEmissive].color.r = (f32)emissive[0];
			modelData.materials[i].channels[MaterialData::eEmissive].color.g = (f32)emissive[1];
			modelData.materials[i].channels[MaterialData::eEmissive].color.b = (f32)emissive[2];
			modelData.materials[i].channels[MaterialData::eEmissive].color.a = 1.0f;

			const FbxDouble3			ambient = GetMaterialProperty(	pMaterial, 
																		FbxSurfaceMaterial::sAmbient, 
																		FbxSurfaceMaterial::sAmbientFactor, 
																		modelData.materials[i].channels[MaterialData::eAmbient].texture);
			modelData.materials[i].channels[MaterialData::eAmbient].textureRefPad = 0;
			modelData.materials[i].channels[MaterialData::eAmbient].color.r = (f32)ambient[0];
			modelData.materials[i].channels[MaterialData::eAmbient].color.g = (f32)ambient[1];
			modelData.materials[i].channels[MaterialData::eAmbient].color.b = (f32)ambient[2];
			modelData.materials[i].channels[MaterialData::eAmbient].color.a = 1.0f;

			const FbxDouble3			diffuse = GetMaterialProperty(	pMaterial, 
																		FbxSurfaceMaterial::sDiffuse, 
																		FbxSurfaceMaterial::sDiffuseFactor, 
																		modelData.materials[i].channels[MaterialData::eDiffuse].texture);
			modelData.materials[i].channels[MaterialData::eDiffuse].textureRefPad = 0;
			modelData.materials[i].channels[MaterialData::eDiffuse].color.r = (f32)diffuse[0];
			modelData.materials[i].channels[MaterialData::eDiffuse].color.g = (f32)diffuse[1];
			modelData.materials[i].channels[MaterialData::eDiffuse].color.b = (f32)diffuse[2];
			modelData.materials[i].channels[MaterialData::eDiffuse].color.a = 1.0f;

			const FbxDouble3			specular = GetMaterialProperty(	pMaterial, 
																		FbxSurfaceMaterial::sSpecular, 
																		FbxSurfaceMaterial::sSpecularFactor, 
																		modelData.materials[i].channels[MaterialData::eSpecular].texture);
			modelData.materials[i].channels[MaterialData::eSpecular].textureRefPad = 0;
			modelData.materials[i].channels[MaterialData::eSpecular].color.r = (f32)specular[0];
			modelData.materials[i].channels[MaterialData::eSpecular].color.g = (f32)specular[1];
			modelData.materials[i].channels[MaterialData::eSpecular].color.b = (f32)specular[2];
			modelData.materials[i].channels[MaterialData::eSpecular].color.a = 1.0f;

			FbxProperty	shininessProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sShininess);
			if( shininessProperty.IsValid() )
			{
				double	shininess = shininessProperty.Get<FbxDouble>();
				modelData.materials[i].specularShininess = static_cast<f32>(shininess);
			}

			for(int matChannelIdx = 0; matChannelIdx < MaterialData::kMaterialChannelCount; matChannelIdx++)
			{
				StripPath(modelData.materials[i].channels[matChannelIdx].texture);
			}
		}
	}
	else
	{
		modelData.materials = NULL;
	}
}

void FBX::BuildCollisionMesh(ModelData& modelData)
{
	if( m_collisionMeshList.GetCount() == 0 )
	{
		modelData.collisionMesh = NULL;
		return;
	}

	int					vertType = kChannel_Position;
	u32					vertSize = GetVertSize(vertType);
	VertInsertionList	vertInsertionList(vertSize);
	IndexList			indexList;

	for(int i = 0; i < m_collisionMeshList.GetCount(); i++)
	{
		FbxMesh				*pMesh = m_collisionMeshList[i];
		FbxNode				*pNode = pMesh->GetNode();		

		FbxAMatrix			nodeTransform = GetGlobalPosition(pNode, FBXSDK_TIME_INFINITE);
		FbxAMatrix			geometryOffset = GetGeometry(pNode);
		nodeTransform *= geometryOffset;

		u8					*currentVertData = new u8[vertSize];
		memset(currentVertData, 0, vertSize * sizeof(u8));

		FbxVector4			*vertPositions = pMesh->GetControlPoints();

		for(int polyIndex = 0; polyIndex < pMesh->GetPolygonCount(); polyIndex++)
		{
			int	polyVertCount = pMesh->GetPolygonSize(polyIndex);
			
			//	only supporting triangles
			assert(polyVertCount == 3);
			if( polyVertCount != 3 )
				continue;

			for(int polyVertIndex = 0; polyVertIndex < polyVertCount; polyVertIndex++)
			{
				u32	vertOffset = 0;

				//	position data
				int			controlPointIndex = pMesh->GetPolygonVertex(polyIndex, polyVertIndex);
				FbxVector4	*thisVertPos = &vertPositions[controlPointIndex];
				FbxVector4	thisTransformedVertPos;
				thisTransformedVertPos = nodeTransform.MultT(*thisVertPos);
				for(int posIdx = 0; posIdx < kNumFloatsPerPosition; posIdx++)
				{
					f32	tempFloat = (f32)thisTransformedVertPos[posIdx];
					memcpy(&currentVertData[vertOffset], &tempFloat, sizeof(f32));
					vertOffset += sizeof(f32);
				}

				//	insert the vert
				int	outputVertIdx = vertInsertionList.Insert(currentVertData, controlPointIndex);

				//	add the index
				indexList.Insert(outputVertIdx, false);
			}
		}
	}

	//	create the collision mesh
	CollisionMeshData	*collisionMesh = new CollisionMeshData;
	
	//	create index buffer
	collisionMesh->indexBuffer.m_pIndices = new u32[indexList.m_indexCount];
	collisionMesh->indexBuffer.m_bufferIndex = 0;
	collisionMesh->indexBuffer.m_indexCount = indexList.m_indexCount;
	
	//	copy index list
	IndexList::Node	*currentIndexNode = indexList.m_pRootNode;
	int						indexOffset = 0;
	while(currentIndexNode)
	{
		collisionMesh->indexBuffer.m_pIndices[indexOffset] = currentIndexNode->m_index;
		currentIndexNode = currentIndexNode->m_pNext;
		indexOffset++;
	}

	//	create the vert buffer
	collisionMesh->vertBuffer.m_pData = new u8[vertInsertionList.m_vertCount * vertSize];
	collisionMesh->vertBuffer.m_bufferIndex = 0;
	collisionMesh->vertBuffer.m_vertCount = vertInsertionList.m_vertCount;
	collisionMesh->vertBuffer.m_vertType = vertType;

	//	copy vert list
	VertInsertionList::Node	*currentVertNode = vertInsertionList.m_pRootNode;
	int						vertOffset = 0;
	while(currentVertNode)
	{
		memcpy(&((u8*)collisionMesh->vertBuffer.m_pData)[vertOffset], currentVertNode->m_vertData, vertSize);
		vertOffset += vertSize;
		currentVertNode = currentVertNode->m_pNext;
	}

	modelData.collisionMesh = collisionMesh;
}

void FBX::BuildBones(ModelData& modelData)
{
	modelData.boneCount = m_skeletonList.GetCount();
	modelData.bones = new BoneData[modelData.boneCount];
	memset(modelData.bones, 0, sizeof(BoneData) * modelData.boneCount);

	for(int i = 0; i < m_skeletonList.GetCount(); i++)
	{
		FbxSkeleton	*pSkeleton = m_skeletonList[i];
		FbxNode		*pNode = pSkeleton->GetNode();
		FbxPose		*pPose = NULL;

		FbxAMatrix	baseTransform;
		if( m_clusterList[i] != NULL )
		{
			m_clusterList[i]->GetTransformLinkMatrix(baseTransform);
		}
		else
		{
			baseTransform = GetGlobalPosition(pNode, FBXSDK_TIME_INFINITE, pPose, NULL);
		}

		CopyToMtx43FromFBX(modelData.bones[i].TPoseWorldTransform, baseTransform);

		baseTransform = baseTransform.Inverse();

		//CopyToMtx44FromFBX(modelData.bones[i].invTPoseWorldTransform, baseTransform);
		CopyToMtx43FromFBX(modelData.bones[i].invTPoseWorldTransform, baseTransform);
		strcpy(modelData.bones[i].name, pNode->GetName());

		FbxNode		*pParentNode = pNode->GetParent();
		if( IsSkeletonNode(pParentNode) )
		{
			FbxSkeleton	*pParentSkeleton = GetFirstSkeletonAttribute(pParentNode);
			int			parentIndex = m_skeletonList.Find(pParentSkeleton);
			modelData.bones[i].parentBone = parentIndex;
		}
		else
		{
			modelData.bones[i].parentBone = -1;
		}
	}

	{
		//	store bone local T-Pose transform for reference
		for(u32 boneIdx = 0; boneIdx < modelData.boneCount; boneIdx++)
		{
			int	parentBoneIdx = modelData.bones[boneIdx].parentBone;
			if( parentBoneIdx >= 0 )
			{
				Mtx43	ourWorldTransform = modelData.bones[boneIdx].TPoseWorldTransform;
				Mtx43	parentWorldTransformInv = modelData.bones[parentBoneIdx].invTPoseWorldTransform;
				Mtx43	ourLocalTransform = ourWorldTransform * parentWorldTransformInv;

				modelData.bones[boneIdx].TPoseLocalTransform = ourLocalTransform;
			}
			else
			{
				modelData.bones[boneIdx].TPoseLocalTransform = Mtx43::IDENTITY;
			}
		}
	}
}

void FBX::BuildBones(AnimationData& animationData)
{
	assert(animationData.numBones == m_skeletonList.GetCount());
	animationData.bones = new BoneData[animationData.numBones];
	memset(animationData.bones, 0, sizeof(BoneData) * animationData.numBones);

	for(int i = 0; i < m_skeletonList.GetCount(); i++)
	{
		FbxSkeleton	*pSkeleton = m_skeletonList[i];
		FbxNode		*pNode = pSkeleton->GetNode();
		FbxPose		*pPose = NULL;

		FbxAMatrix	baseTransform;
		if( m_clusterList[i] != NULL )
		{
			m_clusterList[i]->GetTransformLinkMatrix(baseTransform);
		}
		else
		{
			baseTransform = GetGlobalPosition(pNode, FBXSDK_TIME_INFINITE, pPose, NULL);
		}

		CopyToMtx43FromFBX(animationData.bones[i].TPoseWorldTransform, baseTransform);

		baseTransform = baseTransform.Inverse();

		//CopyToMtx44FromFBX(animationData.bones[i].invTPoseWorldTransform, baseTransform);
		CopyToMtx43FromFBX(animationData.bones[i].invTPoseWorldTransform, baseTransform);
		strcpy(animationData.bones[i].name, pNode->GetName());

		FbxNode		*pParentNode = pNode->GetParent();
		if( IsSkeletonNode(pParentNode) )
		{
			FbxSkeleton	*pParentSkeleton = GetFirstSkeletonAttribute(pParentNode);
			int			parentIndex = m_skeletonList.Find(pParentSkeleton);
			animationData.bones[i].parentBone = parentIndex;
		}
		else
		{
			animationData.bones[i].parentBone = -1;
		}
	}

	{
		//	store bone local T-Pose transform for reference
		for(u32 boneIdx = 0; boneIdx < animationData.numBones; boneIdx++)
		{
			int	parentBoneIdx = animationData.bones[boneIdx].parentBone;
			if( parentBoneIdx >= 0 )
			{
				Mtx43	ourWorldTransform = animationData.bones[boneIdx].TPoseWorldTransform;
				Mtx43	parentWorldTransformInv = animationData.bones[parentBoneIdx].invTPoseWorldTransform;
				Mtx43	ourLocalTransform = ourWorldTransform * parentWorldTransformInv;

				animationData.bones[boneIdx].TPoseLocalTransform = ourLocalTransform;
			}			
			else
			{
				animationData.bones[boneIdx].TPoseLocalTransform = Mtx43::IDENTITY;
			}
		}
	}
}

void FBX::BuildMesh(FbxMesh* pMesh, MeshData* meshDataOut)
{
	int					vertType = GetVertType(pMesh);
	u32					vertSize = GetVertSize(vertType);
	VertInsertionList	vertInsertionList(vertSize);

	FbxNode				*pMeshNode = pMesh->GetNode();
	FbxAMatrix			nodeTransform = GetGlobalPosition(pMeshNode, FBXSDK_TIME_INFINITE);
	FbxAMatrix			geometryOffset = GetGeometry(pMeshNode);
	nodeTransform *= geometryOffset;
	FbxAMatrix			normalTransform = nodeTransform.Inverse().Transpose();

	u8					*currentVertData = new u8[vertSize];
	memset(currentVertData, 0, vertSize * sizeof(u8));

	FbxVector4			*vertPositions = pMesh->GetControlPoints();

	FbxLayer			*layer0 = pMesh->GetLayer(0);	//	may end up needing to do all layers instead of just 0

	//	textures
	FbxLayerElementArrayTemplate<FbxVector2>	*UVArray = NULL;
	FbxLayerElement::EMappingMode				mappingMode = FbxLayerElement::eNone;

	pMesh->GetTextureUV(&UVArray, FbxLayerElement::eTextureDiffuse);
	
	if( layer0 && 
		layer0->GetUVs() )
	{
		mappingMode = pMesh->GetLayer(0)->GetUVs()->GetMappingMode();
	}

	//	colors
	FbxLayerElementVertexColor	*vertexColorElement = layer0->GetVertexColors();

	//	normals	
	FbxLayerElementNormal	*normalElement = NULL;
	if( layer0 )
	{
		normalElement = layer0->GetNormals();	//	normals should only be on layer 0
	}

	//	materials
	FbxLayerElementMaterial	*materialElement = NULL;
	if( layer0 )
	{
		materialElement = layer0->GetMaterials();
	}
	assert(materialElement);

	//	
	int	charAnimOffset = 0;

	//	iterate through the materials and create a separate index buffer for each
	FbxArray<IndexBufferBase*>	indexBufferList;
	FbxArray<u32>				materialIndexList;
	
	for(int meshMaterialIdx = 0; meshMaterialIdx < pMeshNode->GetMaterialCount(); meshMaterialIdx++)
	{
		IndexList			indexList;
		FbxSurfaceMaterial	*pMaterial = pMeshNode->GetMaterial(meshMaterialIdx);
		int					materialIndex = m_materialList.Find(pMaterial);
		if( materialIndex < 0 )
			continue;

		materialIndexList.Add(materialIndex);

		//	iterate through the polygon list to find the appropriate values
		for(int polyIndex = 0; polyIndex < pMesh->GetPolygonCount(); polyIndex++)
		{
			//	only process polys that use this material
			FbxSurfaceMaterial	*polygonMaterial = NULL;
			if( materialElement->GetMappingMode() == FbxLayerElement::eAllSame )
			{
				int	polyMaterialIndex = materialElement->GetIndexArray()[0];
				polygonMaterial = pMeshNode->GetMaterial(polyMaterialIndex);
			}
			else
			if( materialElement->GetMappingMode() == FbxLayerElement::eByPolygon )
			{
				int	polyMaterialIndex = materialElement->GetIndexArray()[polyIndex];
				polygonMaterial = pMeshNode->GetMaterial(polyMaterialIndex);				
			}
			else
			{
				assert(0);
			}

			if( polygonMaterial != pMaterial )
				continue;

			int	polyVertCount = pMesh->GetPolygonSize(polyIndex);
			
			//	only supporting triangles
			assert(polyVertCount == 3);
			if( polyVertCount != 3 )
				continue;

			//	for each vert in this poly, find the data and insert it into the proto-buffer
			for(int polyVertIndex = 0; polyVertIndex < polyVertCount; polyVertIndex++)
			{
				u32	vertOffset = 0;

				//	position data
				int			controlPointIndex = pMesh->GetPolygonVertex(polyIndex, polyVertIndex);
				FbxVector4	*thisVertPos = &vertPositions[controlPointIndex];
				FbxVector4	thisTransformedVertPos;
				thisTransformedVertPos = nodeTransform.MultT(*thisVertPos);
				for(int posIdx = 0; posIdx < kNumFloatsPerPosition; posIdx++)
				{
					f32	tempFloat = (f32)thisTransformedVertPos[posIdx];
					memcpy(&currentVertData[vertOffset], &tempFloat, sizeof(f32));
					vertOffset += sizeof(f32);
				}

				//	texture coordinates
				if( vertType & kChannel_Texcoord )
				{
					int	currentUVIndex;

					if( mappingMode == FbxLayerElement::eByPolygonVertex )
					{
						currentUVIndex = pMesh->GetTextureUVIndex(polyIndex, polyVertIndex);
					}
					else // KFbxLayerElement::eBY_CONTROL_POINT
					{
						currentUVIndex = pMesh->GetPolygonVertex(polyIndex, polyVertIndex);
					}
					assert(UVArray);
					if( UVArray )
					{
						FbxVector2	thisUV = UVArray->GetAt(currentUVIndex);
						for(int uvIdx = 0; uvIdx < kNumFloatsPerTexcoord; uvIdx++)
						{
							f32	tempFloat = (f32)thisUV.mData[uvIdx];
							memcpy(&currentVertData[vertOffset], &tempFloat, sizeof(f32));
							vertOffset += sizeof(f32);
						}
					}
				}

				//	colors
				if( vertType & kChannel_Color )
				{
					assert(vertexColorElement);
					if( vertexColorElement )
					{
						int	colorIndex = 0;
						if( vertexColorElement->GetMappingMode() == FbxLayerElement::eByControlPoint )
						{						
							if( vertexColorElement->GetReferenceMode() == FbxLayerElement::eDirect )
								colorIndex = controlPointIndex;

							if( vertexColorElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect )
								colorIndex = vertexColorElement->GetIndexArray().GetAt(controlPointIndex);	
						}
						else
						//	vertexColorElement->GetMappingMode() == KFbxLayerElement::eBY_POLYGON_VERTEX )
						{
							int	indexByPolygonVertex = polyIndex * 3 + polyVertIndex;	//	this works because we're only allowing triangles
							if( vertexColorElement->GetReferenceMode() == FbxLayerElement::eDirect )
								colorIndex = indexByPolygonVertex;

							if( vertexColorElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect )
								colorIndex = vertexColorElement->GetIndexArray().GetAt(indexByPolygonVertex);	
						}

						//	Got color
						FbxColor	color = vertexColorElement->GetDirectArray().GetAt(colorIndex);
						f32			tempFloat;
						
						tempFloat = (f32)color.mRed;
						memcpy(&currentVertData[vertOffset], &tempFloat, sizeof(f32));
						vertOffset += sizeof(f32);

						tempFloat = (f32)color.mGreen;
						memcpy(&currentVertData[vertOffset], &tempFloat, sizeof(f32));
						vertOffset += sizeof(f32);

						tempFloat = (f32)color.mBlue;
						memcpy(&currentVertData[vertOffset], &tempFloat, sizeof(f32));
						vertOffset += sizeof(f32);

						tempFloat = (f32)color.mAlpha;
						memcpy(&currentVertData[vertOffset], &tempFloat, sizeof(f32));
						vertOffset += sizeof(f32);
						/*for(int colorIdx = 0; colorIdx < kNumFloatsPerColor; colorIdx++)
						{
							f32	tempFloat = (f32)color[colorIdx];
							memcpy(&currentVertData[vertOffset], &tempFloat, sizeof(f32));
							vertOffset += sizeof(f32);
						}*/
					}
				}

				//	normals
				if( vertType & kChannel_Normal )
				{
					assert(normalElement);
					if( normalElement )
					{
						int	normalIndex = 0;

						//	mapping mode is by control points. The mesh should be smooth and soft.
						//	we can get normals by retrieving each control point
						if( normalElement->GetMappingMode() == FbxLayerElement::eByControlPoint )
						{
							//	Let's get normals of each vertex, since the mapping mode of normal element is by control point							
							//	reference mode is direct, the normal index is same as vertex index.
							//	get normals by the index of control vertex
							if( normalElement->GetReferenceMode() == FbxLayerElement::eDirect )
								normalIndex = controlPointIndex;

							//	reference mode is index-to-direct, get normals by the index-to-direct
							if( normalElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect )
								normalIndex = normalElement->GetIndexArray().GetAt(controlPointIndex);							
						}
						//	mapping mode is by polygon-vertex.
						//	we can get normals by retrieving polygon-vertex.
						else 
						if( normalElement->GetMappingMode() == FbxLayerElement::eByPolygonVertex )
						{
							int	indexByPolygonVertex = polyIndex * 3 + polyVertIndex;	//	this works because we're only allowing triangles
							//	reference mode is direct, the normal index is same as lIndexByPolygonVertex.
							if( normalElement->GetReferenceMode() == FbxLayerElement::eDirect )
								normalIndex = indexByPolygonVertex;

							//	reference mode is index-to-direct, get normals by the index-to-direct
							if( normalElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect )
								normalIndex = normalElement->GetIndexArray().GetAt(indexByPolygonVertex);
						}

						//	Got normal
						FbxVector4	normal = normalElement->GetDirectArray().GetAt(normalIndex);
						normal[3] = 0;
						FbxVector4	thisTransformedNormal = normalTransform.MultT(normal);
						thisTransformedNormal.Normalize();
						for(int normIdx = 0; normIdx < kNumFloatsPerNormal; normIdx++)
						{
							f32	tempFloat = (f32)thisTransformedNormal[normIdx];
							memcpy(&currentVertData[vertOffset], &tempFloat, sizeof(f32));
							vertOffset += sizeof(f32);
						}
					}
				}

				//	initialize the bone indices to -1; they'll be set later
				if( vertType & kChannel_CharacterAnimation_BoneIndices )
				{
					charAnimOffset = vertOffset;
					for(int idx = 0; idx < kNumFloatsPerCharAnimBoneIdx; idx++)
					{
						f32	tempFloat = -1.0f;
						memcpy(&currentVertData[vertOffset], &tempFloat, sizeof(f32));
						vertOffset += sizeof(f32);
					}
				}

				//
				if( vertType & kChannel_CharacterAnimation_BoneWeights )
				{
					//	do nothing now, but make sure that they're 0
					for(int idx = 0; idx < kNumFloatsPerCharAnimBoneWeight; idx++)
					{
						f32	tempFloat = 0.0f;
						memcpy(&currentVertData[vertOffset], &tempFloat, sizeof(f32));
						vertOffset += sizeof(f32);
					}
				}

				//	insert the vert
				int	outputVertIdx = vertInsertionList.Insert(currentVertData, controlPointIndex);

				//	add the index
				indexList.Insert(outputVertIdx, false);
			}
		}

		if( indexList.m_indexCount > 0 )
		{
			//	create index buffer
			IndexBufferBase	*thisIndexBuffer = new IndexBufferBase;
			thisIndexBuffer->m_pIndices = new u32[indexList.m_indexCount];
			thisIndexBuffer->m_bufferIndex = 0;
			thisIndexBuffer->m_indexCount = indexList.m_indexCount;
			
			//	copy index list
			IndexList::Node	*currentIndexNode = indexList.m_pRootNode;
			int						indexOffset = 0;
			while(currentIndexNode)
			{
				thisIndexBuffer->m_pIndices[indexOffset] = currentIndexNode->m_index;
				currentIndexNode = currentIndexNode->m_pNext;
				indexOffset++;
			}

			indexBufferList.Add(thisIndexBuffer);
		}
		else
		{
			materialIndexList.RemoveLast();
		}
	}

	assert(materialIndexList.GetCount() == indexBufferList.GetCount());
	meshDataOut->indexBufferCount = indexBufferList.GetCount();
	meshDataOut->indexBuffers = new IndexBufferBase[meshDataOut->indexBufferCount];
	meshDataOut->materialIndices = new u32[meshDataOut->indexBufferCount];
	for(u32 i = 0; i < meshDataOut->indexBufferCount; i++)
	{
		memcpy(&meshDataOut->indexBuffers[i], indexBufferList[i], sizeof(IndexBufferBase));			
	}
	memcpy(meshDataOut->materialIndices, materialIndexList.GetArray(), sizeof(u32) * materialIndexList.GetCount());

	delete [] currentVertData;

	//	skeleton
	if( vertType & kChannel_CharacterAnimation_BoneIndices )
	{
		assert(vertType & kChannel_CharacterAnimation_BoneWeights);	//	both flags should be set
		BuildSkeletonVertData(pMesh, &vertInsertionList, charAnimOffset);
	}

	//	create the vert buffer
	meshDataOut->vertBuffer.m_pData = new u8[vertInsertionList.m_vertCount * vertSize];
	meshDataOut->vertBuffer.m_bufferIndex = 0;
	meshDataOut->vertBuffer.m_vertCount = vertInsertionList.m_vertCount;
	meshDataOut->vertBuffer.m_vertType = vertType;

	//	copy vert list
	VertInsertionList::Node	*currentVertNode = vertInsertionList.m_pRootNode;
	int						vertOffset = 0;
	while(currentVertNode)
	{
		memcpy(&((u8*)meshDataOut->vertBuffer.m_pData)[vertOffset], currentVertNode->m_vertData, vertSize);
		vertOffset += vertSize;
		currentVertNode = currentVertNode->m_pNext;
	}
}

void FBX::ExportToModel(ModelData& modelData)
{
	modelData.meshCount = m_meshList.GetCount();
	modelData.meshPtrs = new MeshData*[modelData.meshCount];
	modelData.bones = NULL;
	modelData.boneCount = 0;
	modelData.materials = NULL;
	modelData.materialCount = 0;

	for(int i = 0; i < m_meshList.GetCount(); i++)
	{
		FbxMesh		*pMesh = m_meshList[i];
		FbxNode		*pNode = pMesh->GetNode();

		//	create MeshData
		modelData.meshPtrs[i] = new MeshData;
		modelData.meshPtrs[i]->drawMode = kTriangles;
		modelData.meshPtrs[i]->vaos = NULL;

		//	write verts
		BuildMesh(pMesh, modelData.meshPtrs[i]);
	}

	BuildMaterials(modelData);
	BuildCollisionMesh(modelData);
	BuildBones(modelData);

	modelData.bounds.SetFromModel(modelData);
}

void FBX::InitKeyFrameListFromAnimCurve(KeyFrameList* keyFrameList)
{
	const f32		kTimeConversionFactor = 1.0f / 1000.0f;

	int				animCurveNodeCount = m_pCurrentAnimLayer->GetMemberCount();
	for(int i = 0; i < animCurveNodeCount; i++)
	{
		FbxAnimCurveNode*	animCurveNode = (FbxAnimCurveNode*)m_pCurrentAnimLayer->GetMember(i);
		for(unsigned int j = 0; j < animCurveNode->GetChannelsCount(); j++)
		{
			for(int k = 0; k < animCurveNode->GetCurveCount(j); k++)
			{
				FbxAnimCurve*	animCurve = animCurveNode->GetCurve(j, k);
				if( animCurve == NULL )
					continue;

				for(int l = 0; l < animCurve->KeyGetCount(); l++)
				{
					FbxAnimCurveKey	animCurveKey = animCurve->KeyGet(l);
					FbxTime			keyTime = animCurveKey.GetTime();
					f32				fKeyTime = keyTime.GetMilliSeconds() * kTimeConversionFactor;

					//	insert keyTime into list, removing dupes, sorted from least to greatest
					keyFrameList->Insert(fKeyTime);
				}
			}
		}
	}
}

void FBX::InitKeyFrameListFromDuration(KeyFrameList* keyFrameList)
{
	//	sample data at 60fps rather than using the imported keyframes
	const f32	kTimeConversionFactor = 1.0f / 1000.0f;

	FbxTimeSpan	timeSpan;
	m_pScene->GetGlobalSettings().GetTimelineDefaultTimeSpan( timeSpan );
	FbxTime		startTime = timeSpan.GetStart();
	FbxTime		duration = timeSpan.GetDuration();
	f32			animStartTime = (f32)startTime.GetSecondDouble();
	f32			animDuration = (f32)duration.GetSecondDouble();

	const u32	numFrames = (u32)((animDuration * 60.0f ) + 1.5f);	//	round up plus 1

	FbxTime		keyTime;
	for(u32 frameIdx = 0; frameIdx < numFrames; ++frameIdx)
	{
		keyTime.SetFrame( frameIdx, FbxTime::eFrames60 );

		f32		fKeyTime = keyTime.GetMilliSeconds() * kTimeConversionFactor;

		//	insert keyTime into list, removing dupes, sorted from least to greatest
		keyFrameList->Insert(fKeyTime);
	}
}

void FBX::ExportToAnimation(AnimationData& animationData, bool localBoneSpace)
{
	//	create list of keyframes
	KeyFrameList	keyFrameList;
	//keyFrameList.Insert(m_startTime);
	//keyFrameList.Insert(m_stopTime);

	InitKeyFrameListFromAnimCurve(&keyFrameList);
	//InitKeyFrameListFromDuration(&keyFrameList);

	//	init AnimationData
	animationData.Allocate(keyFrameList.m_keyframeCount, m_skeletonList.Size());

	//	change the flag to make it an asset
	animationData.flags = AnimationData::kFlag_Asset;

	//	copy keyframe times in
	{
		u32					keyFrameIdx = 0;
		KeyFrameList::Node*	currentKeyFrameNode = keyFrameList.m_pRootNode;
		while(currentKeyFrameNode && keyFrameIdx < animationData.numKeyFrames)
		{
			animationData.keyFrames[keyFrameIdx].time = currentKeyFrameNode->m_time;

			currentKeyFrameNode = currentKeyFrameNode->m_pNext;
			keyFrameIdx++;
		}
	}

	//	iterate through bones to find transforms at keyframes
	for(int i = 0; i < m_skeletonList.GetCount(); i++)
	{
		FbxSkeleton	*pSkeleton = m_skeletonList[i];
		FbxNode		*pSkeletonNode = pSkeleton->GetNode();
		FbxPose		*pPose = NULL;

		int					keyFrameIdx = 0;
		KeyFrameList::Node	*currentKeyFrameNode = keyFrameList.m_pRootNode;
		while(currentKeyFrameNode)
		{
			FbxAMatrix	linkTransformAtThisKeyframe;
			FbxTime		keyTime;
			keyTime = FbxTimeSeconds(currentKeyFrameNode->m_time);
			//keyTime.SetSecondDouble(currentKeyFrameNode->m_time);
			linkTransformAtThisKeyframe = GetGlobalPosition(pSkeletonNode, keyTime, pPose, NULL);
			//linkTransformAtThisKeyframe = pSkeletonNode->EvaluateLocalTransform(currentKeyFrameNode->m_time);

			/*FbxAMatrix	linkTransformAtBindTime;
			if( m_clusterList[i] != NULL )
			{
				m_clusterList[i]->GetTransformLinkMatrix(linkTransformAtBindTime);
			}
			else
			{
				linkTransformAtBindTime = GetGlobalPosition(pSkeletonNode, FBXSDK_TIME_INFINITE, pPose, NULL);
			}
			linkTransformAtThisKeyframe = linkTransformAtThisKeyframe * linkTransformAtBindTime.Inverse();*/
			
			//ComputeClusterCurrentTransform(pMesh, cluster, linkTransformAtThisKeyframe, currentKeyFrameNode->m_time, pPose);
			//ComputeClusterDeformation(pMesh, cluster, linkTransformAtThisKeyframe, currentKeyFrameNode->m_time, pPose);				

			//
			Mtx44	thisBoneTransform;
			CopyToMtx44FromFBX(thisBoneTransform, linkTransformAtThisKeyframe);

			Mtx33	thisBoneRotationMtx = thisBoneTransform.ExtractOrthoNormalMtx33();
			Quat	thisBoneRotation(thisBoneRotationMtx);
			animationData.keyFrames[keyFrameIdx].boneTransforms[i].rotation = thisBoneRotation;

			animationData.keyFrames[keyFrameIdx].boneTransforms[i].scale = thisBoneTransform.GetScale();

			animationData.keyFrames[keyFrameIdx].boneTransforms[i].translation = thisBoneTransform.GetTranslation3();

			currentKeyFrameNode = currentKeyFrameNode->m_pNext;
			keyFrameIdx++;
		}		
	}

	//	re-normalize keyframe times to make first keyframe = time 0
	f32	smallestTime = animationData.keyFrames[0].time;
	for(u32 i = 0; i < animationData.numKeyFrames; i++)
	{
		animationData.keyFrames[i].time -= smallestTime;
	}

	//	build base bones
	BuildBones(animationData);

	if( localBoneSpace )
	{
		//	shift transforms from world space to local space
		for(u32 keyFrameIdx = 0; keyFrameIdx < animationData.numKeyFrames; keyFrameIdx++)
		{			
			AnimationData::KeyFrame	worldSpaceKeyFrames = animationData.keyFrames[keyFrameIdx];

			for(u32 boneIdx = 0; boneIdx < animationData.numBones; boneIdx++)
			{
				int	parentBoneIdx = animationData.bones[boneIdx].parentBone;
				if( parentBoneIdx >= 0 )
				{
					AnimationData::BoneTransform	parentBoneTransform = worldSpaceKeyFrames.boneTransforms[parentBoneIdx];
					AnimationData::BoneTransform*	pOurBoneTransform = &animationData.keyFrames[keyFrameIdx].boneTransforms[boneIdx];

					Mtx44	parentWorldTransform;
					Math::ComposeTransform(parentBoneTransform.rotation, parentBoneTransform.scale, parentBoneTransform.translation, &parentWorldTransform);

					Mtx44	ourWorldTransform;
					Math::ComposeTransform(pOurBoneTransform->rotation, pOurBoneTransform->scale, pOurBoneTransform->translation, &ourWorldTransform);

					Mtx44	parentWorldTransformInv = parentWorldTransform.Inverse43();

					//Mtx44	ourLocalTransform = parentWorldTransformInv * ourWorldTransform;
					Mtx44	ourLocalTransform = ourWorldTransform * parentWorldTransformInv;

					pOurBoneTransform->translation = ourLocalTransform.GetTranslation3();

					pOurBoneTransform->scale = ourLocalTransform.GetScale();

					Mtx33	ourLocalRotation = ourLocalTransform.ExtractOrthoNormalMtx33();
					pOurBoneTransform->rotation = Quat(ourLocalRotation);
				}
				else
				{
					//	if the bone has no parent then leave it as-is in world space
				}
			}
		}
	}
}

void FBX::WriteToFbx(const char* filename)
{
	FbxExporter*	pExporter = FbxExporter::Create(m_pSdkManager, "");

	// Initialize the exporter.
	bool			exportInitSuccess = pExporter->Initialize(filename, -1, m_pSdkManager->GetIOSettings());
	if( !exportInitSuccess )
	{
		char	buffer[256];
		sprintf(buffer, "Call to FbxExporter::Initialize() failed.\nError returned: %s\n\n", pExporter->GetStatus().GetErrorString());
		assert(exportInitSuccess);
    }

	//	add data to m_pScene prior to this call?

	//	Export the scene to the file.
	pExporter->Export(m_pScene);

	//	Destroy the exporter.
	pExporter->Destroy();
}

//
FbxNode* CreateBoneFromAnimData(FbxScene* pParentScene, u32 boneIdx, const AnimationData& animData)
{
	BoneData*		pThisBoneData = &animData.bones[boneIdx];

	FbxString		boneNodeName(pThisBoneData->name);

	FbxSkeleton*	pBoneNodeAttribute = FbxSkeleton::Create(pParentScene, boneNodeName);
	pBoneNodeAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
	pBoneNodeAttribute->Size.Set(1.0);	//	is this scale or something?

	FbxNode*		pBoneNode = FbxNode::Create(pParentScene, boneNodeName.Buffer());
	pBoneNode->SetNodeAttribute(pBoneNodeAttribute);

	//	was not sure if first frame or TPose was correct; need to revisit
	AnimationData::BoneTransform	thisBoneFirstFrameBoneTransform = animData.keyFrames[0].boneTransforms[boneIdx];	
	Mtx43			thisBoneTransform = pThisBoneData->TPoseLocalTransform;	

	//	translation
	Vector3			thisBoneTranslation = thisBoneTransform.GetTranslation();
	//Vector3			thisBoneTranslation = thisBoneFirstFrameBoneTransform.translation;
	FbxVector4		thisBoneFBXTranslation(thisBoneTranslation.x, thisBoneTranslation.y, thisBoneTranslation.z);
	pBoneNode->LclTranslation.Set(thisBoneFBXTranslation);

	//	rotation
	Quat			thisBoneQuat(thisBoneTransform.ExtractRotation());
	//Quat			thisBoneQuat = thisBoneFirstFrameBoneTransform.rotation;
	FbxQuaternion	fbxBoneQuat(thisBoneQuat.x, thisBoneQuat.y, thisBoneQuat.z, thisBoneQuat.w);
	FbxVector4		thisBoneFBXEulerAngles;
	thisBoneFBXEulerAngles.SetXYZ(fbxBoneQuat);
	pBoneNode->LclRotation.Set(thisBoneFBXEulerAngles);

	//	scale
	Vector3			thisBoneScale = thisBoneTransform.GetScale();
	//Vector3			thisBoneScale = thisBoneFirstFrameBoneTransform.scale;
	FbxVector4		thisBoneFBXScale(thisBoneScale.x, thisBoneScale.y, thisBoneScale.z);
	pBoneNode->LclScaling.Set(thisBoneFBXScale);

	//	done
	return pBoneNode;
}

//	
FbxNode* CreateSkeletonFromAnimData(FbxScene* pParentScene, const char* skeletonName, const AnimationData& animData, Array<FbxNode*>* boneNodesOut)
{
	assert(boneNodesOut != nullptr);
	boneNodesOut->Clear();

	FbxString		rootNodeName(skeletonName);
    rootNodeName += "Root";

    FbxSkeleton*	pSkeletonRootAttribute = FbxSkeleton::Create(pParentScene, skeletonName);
    pSkeletonRootAttribute->SetSkeletonType(FbxSkeleton::eRoot);

	FbxNode*		pSkeletonRootNode = FbxNode::Create(pParentScene, rootNodeName.Buffer());
	pSkeletonRootNode->SetNodeAttribute(pSkeletonRootAttribute);    
	pSkeletonRootNode->LclTranslation.Set(FbxVector4(0.0, 0.0, 0.0));

	//	create bone nodes
	boneNodesOut->Alloc(animData.numBones);
	for(u32 i = 0; i < animData.numBones; ++i)
	{
		FbxNode*	thisBoneNode = CreateBoneFromAnimData(pParentScene, i, animData);
		boneNodesOut->Add(thisBoneNode);
	}

	//	attach bone nodes to each other
	for(u32 i = 0; i < boneNodesOut->Count(); ++i)
	{
		BoneData*	pThisBoneData = &animData.bones[i];

		if( pThisBoneData->parentBone < 0 )
		{
			pSkeletonRootNode->AddChild((*boneNodesOut)[i]);
		}
		else
		{
			(*boneNodesOut)[pThisBoneData->parentBone]->AddChild((*boneNodesOut)[i]);
		}
	}

	//	do we need to delete all of the stuff that was created above this?
	return pSkeletonRootNode;
}

//	
void StoreRestPoseFromAnimData(FbxScene* pScene, const AnimationData& animData, Array<FbxNode*>& boneNodes)
{
	// This example show an arbitrary rest pose assignment.
	// This rest pose will set the bone rotation to the same value 
	// as time 1 second in the first stack of animation, but the 
	// position of the bone will be set elsewhere in the scene.
	FbxMatrix	transformMatrix;
	FbxVector4	fbxTranslation;
	FbxVector4	fbxRotation;
	FbxVector4	fbxScale;

	// Create the rest pose
	FbxPose*	pPose = FbxPose::Create(pScene, "BindPose");

	for(u32 i = 0; i < boneNodes.Count(); ++i)
	{
		const BoneData*	thisBoneData = &animData.bones[i];
		Mtx43			thisBoneTransform = thisBoneData->TPoseLocalTransform;
		bool			boneIsRoot = thisBoneData->parentBone < 0;

		Vector3			translation = thisBoneTransform.GetTranslation();
		
		Mtx33			rotMtx = thisBoneTransform.ExtractRotation();
		Quat			rotQuat(rotMtx);
		//Vector3			eulerAngles;
		//rotMtx.GetEulerAngles(&eulerAngles);
		//eulerAngles = eulerAngles * RADIANS_TO_DEGREES;
		FbxQuaternion	fbxQuat(rotQuat.x, rotQuat.y, rotQuat.z, rotQuat.w);
		FbxVector4		eulerAngles;
		eulerAngles.SetXYZ(fbxQuat);

		Vector3			scale = thisBoneTransform.GetScale();

		fbxTranslation.Set(translation.x, translation.y, translation.z);
		//fbxRotation.Set(eulerAngles.x, eulerAngles.y, eulerAngles.z);
		fbxRotation.Set(eulerAngles[0], eulerAngles[1], eulerAngles[2]);
		fbxScale.Set(scale.x, scale.y, scale.z);

		transformMatrix.SetTRS(fbxTranslation, fbxRotation, fbxScale);

		pPose->Add(boneNodes[i], transformMatrix, !boneIsRoot);
	}

	//	Now add the pose to the scene
	pScene->AddPose(pPose);
}

//	
void AnimateSkeletonFromAnimData(FbxScene* pScene, const AnimationData& animData, Array<FbxNode*>& boneNodes)
{
    FbxString		animStackName;
    FbxTime			time;
    int				keyIndex = 0;

    // First animation stack.
    animStackName = "AnimStackName";
    FbxAnimStack*	pAnimStack = FbxAnimStack::Create(pScene, animStackName);

    // The animation nodes can only exist on AnimLayers therefore it is mandatory to
    // add at least one AnimLayer to the AnimStack. And for the purpose of this example,
    // one layer is all we need.
    FbxAnimLayer*	pAnimLayer = FbxAnimLayer::Create(pScene, "BaseLayerName");
    pAnimStack->AddMember(pAnimLayer);

	//	fill out the AnimCurves for each bone
	for(u32 i = 0; i < boneNodes.Count(); ++i)
	{
		//	translation
		//if( false )
		{
			FbxAnimCurve*	pTransXCurve = boneNodes[i]->LclTranslation.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
			FbxAnimCurve*	pTransYCurve = boneNodes[i]->LclTranslation.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
			FbxAnimCurve*	pTransZCurve = boneNodes[i]->LclTranslation.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);

			assert(pTransXCurve != nullptr);
			assert(pTransYCurve != nullptr);
			assert(pTransZCurve != nullptr);

			pTransXCurve->KeyModifyBegin();
			pTransYCurve->KeyModifyBegin();
			pTransZCurve->KeyModifyBegin();
				for(u32 keyIdx = 0; keyIdx < animData.numKeyFrames; ++keyIdx)
				{
					time.SetSecondDouble(animData.keyFrames[keyIdx].time);

					keyIndex = pTransXCurve->KeyAdd(time);
					pTransXCurve->KeySetValue(keyIndex, animData.keyFrames[keyIdx].boneTransforms[i].translation.x);
					pTransXCurve->KeySetInterpolation(keyIndex, FbxAnimCurveDef::eInterpolationLinear);

					keyIndex = pTransYCurve->KeyAdd(time);
					pTransYCurve->KeySetValue(keyIndex, animData.keyFrames[keyIdx].boneTransforms[i].translation.y);
					pTransYCurve->KeySetInterpolation(keyIndex, FbxAnimCurveDef::eInterpolationLinear);

					keyIndex = pTransZCurve->KeyAdd(time);
					pTransZCurve->KeySetValue(keyIndex, animData.keyFrames[keyIdx].boneTransforms[i].translation.z);
					pTransZCurve->KeySetInterpolation(keyIndex, FbxAnimCurveDef::eInterpolationLinear);
				}
			pTransZCurve->KeyModifyEnd();
			pTransYCurve->KeyModifyEnd();
			pTransXCurve->KeyModifyEnd();
		}

		//	rotation
		//if( false )
		{
			FbxAnimCurve*	pRotXCurve = boneNodes[i]->LclRotation.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
			FbxAnimCurve*	pRotYCurve = boneNodes[i]->LclRotation.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
			FbxAnimCurve*	pRotZCurve = boneNodes[i]->LclRotation.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);
		
			assert(pRotXCurve != nullptr);
			assert(pRotYCurve != nullptr);
			assert(pRotZCurve != nullptr);

			pRotXCurve->KeyModifyBegin();
			pRotYCurve->KeyModifyBegin();
			pRotZCurve->KeyModifyBegin();
				for(u32 keyIdx = 0; keyIdx < animData.numKeyFrames; ++keyIdx)
				{
					time.SetSecondDouble(animData.keyFrames[keyIdx].time);

					//Vector3	eulerAngles;
					//Mtx33	rotationMtx;
					//animData.keyFrames[keyIdx].boneTransforms[i].rotation.Convert(&rotationMtx);
					//rotationMtx.GetEulerAngles(&eulerAngles);
					//eulerAngles = eulerAngles * RADIANS_TO_DEGREES;
					Quat			rotQuat = animData.keyFrames[keyIdx].boneTransforms[i].rotation;

					FbxQuaternion	fbxQuat(rotQuat.x, rotQuat.y, rotQuat.z, rotQuat.w);
					FbxVector4		eulerAngles;
					eulerAngles.SetXYZ(fbxQuat);

					keyIndex = pRotXCurve->KeyAdd(time);
					pRotXCurve->KeySetValue(keyIndex, (float)eulerAngles[0]);
					pRotXCurve->KeySetInterpolation(keyIndex, FbxAnimCurveDef::eInterpolationLinear);

					keyIndex = pRotYCurve->KeyAdd(time);
					pRotYCurve->KeySetValue(keyIndex, (float)eulerAngles[1]);
					pRotYCurve->KeySetInterpolation(keyIndex, FbxAnimCurveDef::eInterpolationLinear);

					keyIndex = pRotZCurve->KeyAdd(time);
					pRotZCurve->KeySetValue(keyIndex, (float)eulerAngles[2]);
					pRotZCurve->KeySetInterpolation(keyIndex, FbxAnimCurveDef::eInterpolationLinear);
				}
			pRotZCurve->KeyModifyEnd();
			pRotYCurve->KeyModifyEnd();
			pRotXCurve->KeyModifyEnd();
		}

		//	scale
		//if( false )
		{
			FbxAnimCurve*	pScaleXCurve = boneNodes[i]->LclScaling.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
			FbxAnimCurve*	pScaleYCurve = boneNodes[i]->LclScaling.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
			FbxAnimCurve*	pScaleZCurve = boneNodes[i]->LclScaling.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);

			assert(pScaleXCurve != nullptr);
			assert(pScaleYCurve != nullptr);
			assert(pScaleZCurve != nullptr);

			pScaleXCurve->KeyModifyBegin();
			pScaleYCurve->KeyModifyBegin();
			pScaleZCurve->KeyModifyBegin();
				for(u32 keyIdx = 0; keyIdx < animData.numKeyFrames; ++keyIdx)
				{
					time.SetSecondDouble(animData.keyFrames[keyIdx].time);

					keyIndex = pScaleXCurve->KeyAdd(time);
					pScaleXCurve->KeySetValue(keyIndex, animData.keyFrames[keyIdx].boneTransforms[i].scale.x);
					pScaleXCurve->KeySetInterpolation(keyIndex, FbxAnimCurveDef::eInterpolationLinear);

					keyIndex = pScaleYCurve->KeyAdd(time);
					pScaleYCurve->KeySetValue(keyIndex, animData.keyFrames[keyIdx].boneTransforms[i].scale.y);
					pScaleYCurve->KeySetInterpolation(keyIndex, FbxAnimCurveDef::eInterpolationLinear);

					keyIndex = pScaleZCurve->KeyAdd(time);
					pScaleZCurve->KeySetValue(keyIndex, animData.keyFrames[keyIdx].boneTransforms[i].scale.z);
					pScaleZCurve->KeySetInterpolation(keyIndex, FbxAnimCurveDef::eInterpolationLinear);
				}
			pScaleZCurve->KeyModifyEnd();
			pScaleYCurve->KeyModifyEnd();
			pScaleXCurve->KeyModifyEnd();
		}
	}
}

void FBX::AddAnimDataToScene(const AnimationData& animData)
{
    // create scene info
    FbxDocumentInfo*	pSceneInfo = FbxDocumentInfo::Create(m_pSdkManager, "SceneInfo");
    pSceneInfo->mTitle = "FBXExportScene";
    pSceneInfo->mSubject = "Is Cool";
    pSceneInfo->mAuthor = "Yo Momma";
    pSceneInfo->mRevision = "rev. 1.0";
    pSceneInfo->mKeywords = "whoa";
    pSceneInfo->mComment = "dude";
    m_pScene->SetSceneInfo(pSceneInfo);

	//
    Array<FbxNode*>	boneNodes;
    FbxNode*		pSkeletonRoot = CreateSkeletonFromAnimData(m_pScene, "Skeleton", animData, &boneNodes);

    // Build the node tree.
    FbxNode*		pRootNode = m_pScene->GetRootNode();
    pRootNode->AddChild(pSkeletonRoot);

    // Store poses
    StoreRestPoseFromAnimData(m_pScene, animData, boneNodes);

    // Animation
    AnimateSkeletonFromAnimData(m_pScene, animData, boneNodes);
}

//	static
void FBX::CleanupModel(ModelData &modelData)
{
	modelData.Deallocate();
}

void FBX::CleanupAnim(AnimationData &animData)
{
	animData.Deallocate();
}

	