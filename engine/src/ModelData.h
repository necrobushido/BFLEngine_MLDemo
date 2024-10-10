#pragma once

#include "MeshData.h"
#include "BoneData.h"
#include "MaterialData.h"
#include "CollisionMeshData.h"
#include "BoundingBox3D.h"

class ModelData
{
public:
	void Init();
	void Deinit();

	//	used in tools to handle dynamically allocated data members
	void Deallocate();

	void Draw() const;
	void DrawWireframe() const;
	void DrawBase() const;

public:
	MaterialData*		materials;
	u32					materialCount;
	BoneData*			bones;
	u32					boneCount;
	CollisionMeshData*	collisionMesh;
	MeshData**			meshPtrs;
	int					meshCount;
	BoundingBox3D		bounds;
};