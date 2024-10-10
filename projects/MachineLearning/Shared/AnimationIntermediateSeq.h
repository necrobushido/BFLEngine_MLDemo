#pragma once

#include "ModelData.h"
#include "AnimationData.h"
#include "types.h"

#include "SharedMLHeader.h"
#include "BoneTree.h"
#include "AnimImageEuler.h"
#include "AnimImageC3DR6.h"

class ModelIntermediate;

class AnimationIntermediateSeq
{
public:
	struct KeyFrame
	{
		std::vector<Quat>		boneRotations;
		std::vector<Vector3>	boneScales;
		std::vector<Vector3>	boneTranslations;

		std::vector<Quat>		worldRotations;
		std::vector<Vector3>	worldScales;
		std::vector<Vector3>	worldTranslations;
	};

public:
	void Init(const ModelData& sourceModelData, const AnimationData& animData, const char* animName);
	void SetDescFromFile(const char* descFilePath);

	void InitBonesFromModelData(const ModelData& modelData);
	void InitFromModelIntermediate(const ModelIntermediate& modelData);
	void Clear();
	void ClearForGeneration();

	void CalcLocalRotationsFromWorldRotations();

	KeyFrame* AddKeyFrame();

	void ExportToAnimData(AnimationData* animData);

public:
	BoneTree						boneTree;
	std::vector<KeyFrame>			keyFrames;
	f32								duration;
	BoundingBox3D					sourceModelBounds;
	std::string						name;

	std::string						descString;
	AnimImageEuler					animImageEuler;
	AnimImageC3DR6					animImageC3DR6;
};