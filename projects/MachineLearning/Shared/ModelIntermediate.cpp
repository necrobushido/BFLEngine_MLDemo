#include "ModelIntermediate.h"

void ModelIntermediate::InitFromModelData(const ModelData& modelData)
{
	boneTree.Init(modelData);

	bounds = modelData.bounds;
}