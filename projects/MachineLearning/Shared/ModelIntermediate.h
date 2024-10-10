#pragma once

#include "ModelData.h"

#include "BoneTree.h"

class ModelIntermediate
{
public:
	void InitFromModelData(const ModelData& modelData);

public:
	BoneTree					boneTree;
	BoundingBox3D				bounds;
};