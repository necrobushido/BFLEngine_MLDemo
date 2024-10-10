#include "AnimationIntermediateSeq.h"

#include "ModelIntermediate.h"
#include "MathNamespace.h"

#include "AnimGenHyperParameters.h"

void AnimationIntermediateSeq::Init(const ModelData& sourceModelData, const AnimationData& animData, const char* animName)
{
	boneTree.Init(animData);

	name = animName;

	//	is this correct?
	//	0 t1 t2 t3 t4 t5 t6 t7 t8 t9  t0 t1 t2 t3 t4 t5 t6 t7 t8 t9  t0 t1 t2 t3 t4 t5 t6 t7 t8 t9  t0
	int	targetKeyFrameCount = (int)((animData.Duration() / AnimGenHyperParameters::kSecondsPerKeyFrame) + 1);

	duration = (f32)((double)targetKeyFrameCount * AnimGenHyperParameters::kSecondsPerKeyFrame);

	Mtx43*	tempTransformData = new Mtx43[animData.numBones];

	Assert(animData.numKeyFrames > 0);
	for(int keyFrameIdx = 0; keyFrameIdx < targetKeyFrameCount; ++keyFrameIdx)
	{
		f32			keyTime = (f32)(keyFrameIdx * AnimGenHyperParameters::kSecondsPerKeyFrame);
		KeyFrame*	pThisKeyFrame = AddKeyFrame();

		int		prevKeyFrameIndex = -1;
		int		nextKeyFrameIndex = -1;
		animData.GetKeysForTime(keyTime, &prevKeyFrameIndex, &nextKeyFrameIndex);

		animData.CalcBoneWorldTransformsBatch(keyTime, prevKeyFrameIndex, nextKeyFrameIndex, tempTransformData);

		for(u32 boneIdx = 0; boneIdx < animData.numBones; ++boneIdx)
		{
			//
			AnimationData::BoneTransform	boneLocalTransform = animData.GetBoneLocalTransformAtTime(boneIdx, keyTime, prevKeyFrameIndex, nextKeyFrameIndex);

			pThisKeyFrame->boneRotations[boneIdx] = boneLocalTransform.rotation;
			pThisKeyFrame->boneScales[boneIdx] = boneLocalTransform.scale;
			pThisKeyFrame->boneTranslations[boneIdx] = boneLocalTransform.translation;

			//
			/*AnimationData::BoneTransform	boneWorldTransform = animData.GetBoneWorldTransformAtTime(boneIdx, keyTime, prevKeyFrameIndex, nextKeyFrameIndex);

			pThisKeyFrame->worldRotations[boneIdx] = boneWorldTransform.rotation;
			pThisKeyFrame->worldScales[boneIdx] = boneWorldTransform.scale;
			pThisKeyFrame->worldTranslations[boneIdx] = boneWorldTransform.translation;*/

			Mtx43	thisBoneWorldTransform = tempTransformData[boneIdx];
			Mtx33	thisBoneWorldRotation = thisBoneWorldTransform.ExtractRotation();

			pThisKeyFrame->worldRotations[boneIdx] = Quat(thisBoneWorldRotation);
			pThisKeyFrame->worldScales[boneIdx] = thisBoneWorldTransform.GetScale();
			pThisKeyFrame->worldTranslations[boneIdx] = thisBoneWorldTransform.GetTranslation();
		}
	}

	sourceModelBounds = sourceModelData.bounds;

	animImageEuler.Init(*this);
	animImageC3DR6.Init(*this);

	/*torch::Tensor	eulerImage = animImageEuler.GetLocalKeyframeImageForWholeAnim();
	DebugPrintf("animation %s Euler: mean = %f, stdDev = %f\n",
		animName,
		eulerImage.mean().item().toFloat(),
		eulerImage.std().item().toFloat() );

	torch::Tensor	C3DR6Image = animImageC3DR6.GetLocalKeyframeImageForWholeAnim();
	DebugPrintf("animation %s C3DR6: mean = %f, stdDev = %f\n",
		animName,
		C3DR6Image.mean().item().toFloat(),
		C3DR6Image.std().item().toFloat() );*/

	delete [] tempTransformData;
}

void AnimationIntermediateSeq::SetDescFromFile(const char* descFilePath)
{
	{
		FileRef<char>		descFileRef = g_fileManager->MakeRef(descFilePath);
		const char*			descFileData = (*descFileRef);
		const size_t		descFileSize = descFileRef.FileSize();

		//	strtok modifies the input, so we need to copy over first
		char*				descProcessData = new char[descFileSize+1];
		memset(descProcessData, 0, sizeof(char) * descFileSize+1);
		strncpy(descProcessData, descFileData, descFileSize);

		descString = descProcessData;

		delete [] descProcessData;
	}
}

AnimationIntermediateSeq::KeyFrame* AnimationIntermediateSeq::AddKeyFrame()
{
	KeyFrame	thisKeyFrame;

	//	init the transform to the t-pose
	for(int boneIdx = 0; boneIdx < (int)boneTree.m_nodes.size(); ++boneIdx)
	{
		//	local
		Mtx43	thisBoneTPoseLocalTransform = boneTree.m_nodes[boneIdx].m_boneData.TPoseLocalTransform;
		Quat	thisBoneTPoseRotQuat;
		Vector3	thisBoneTPoseScale;
		Vector3	thisBoneTPoseTrans;

		Math::DecomposeTransform(thisBoneTPoseLocalTransform, &thisBoneTPoseRotQuat, &thisBoneTPoseScale, &thisBoneTPoseTrans);

		thisKeyFrame.boneRotations.push_back(thisBoneTPoseRotQuat);
		thisKeyFrame.boneScales.push_back(thisBoneTPoseScale);
		thisKeyFrame.boneTranslations.push_back(thisBoneTPoseTrans);

		//	world
		Mtx43	thisBoneTPoseWorldTransform = boneTree.m_nodes[boneIdx].m_boneData.TPoseWorldTransform;

		Math::DecomposeTransform(thisBoneTPoseWorldTransform, &thisBoneTPoseRotQuat, &thisBoneTPoseScale, &thisBoneTPoseTrans);

		thisKeyFrame.worldRotations.push_back(thisBoneTPoseRotQuat);
		thisKeyFrame.worldScales.push_back(thisBoneTPoseScale);
		thisKeyFrame.worldTranslations.push_back(thisBoneTPoseTrans);
	}

	keyFrames.push_back(thisKeyFrame);

	return &keyFrames[keyFrames.size()-1];
}

void AnimationIntermediateSeq::InitBonesFromModelData(const ModelData& modelData)
{
	boneTree.Clear();
	boneTree.Init(modelData);
}

void AnimationIntermediateSeq::InitFromModelIntermediate(const ModelIntermediate& modelData)
{
	boneTree.Clear();
	boneTree = modelData.boneTree;

	sourceModelBounds = modelData.bounds;
}

void AnimationIntermediateSeq::Clear()
{
	boneTree.Clear();
	keyFrames.clear();
}

void AnimationIntermediateSeq::ClearForGeneration()
{
	boneTree.Clear();
	keyFrames.clear();
}

void AnimationIntermediateSeq::ExportToAnimData(AnimationData* animData)
{
	animData->Allocate((u32)keyFrames.size(), (u32)boneTree.m_nodes.size());

	for(u32 boneIdx = 0; boneIdx < animData->numBones; ++boneIdx)
	{
		animData->bones[boneIdx] = boneTree.m_nodes[boneIdx].m_boneData;
	}

	for(u32 keyFrameIdx = 0; keyFrameIdx < keyFrames.size(); ++keyFrameIdx)
	{
		animData->keyFrames[keyFrameIdx].time = (f32)(keyFrameIdx * AnimGenHyperParameters::kSecondsPerKeyFrame);
		for(u32 boneIdx = 0; boneIdx < animData->numBones; ++boneIdx)
		{
			animData->keyFrames[keyFrameIdx].boneTransforms[boneIdx].rotation = keyFrames[keyFrameIdx].boneRotations[boneIdx];
			animData->keyFrames[keyFrameIdx].boneTransforms[boneIdx].scale = keyFrames[keyFrameIdx].boneScales[boneIdx];
			animData->keyFrames[keyFrameIdx].boneTransforms[boneIdx].translation = keyFrames[keyFrameIdx].boneTranslations[boneIdx];
		}
	}
}

void AnimationIntermediateSeq::CalcLocalRotationsFromWorldRotations()
{
	for(u32 keyFrameIdx = 0; keyFrameIdx < keyFrames.size(); ++keyFrameIdx)
	{
		for(u32 boneIdx = 0; boneIdx < keyFrames[keyFrameIdx].worldRotations.size(); ++boneIdx)
		{
			int		parentIdx = boneTree.m_nodes[boneIdx].m_boneData.parentBone;
			Quat	boneWorldRotation = keyFrames[keyFrameIdx].worldRotations[boneIdx];
			Quat	parentWorldRotation = Quat::IDENTITY;

			if( parentIdx >= 0 )
			{
				parentWorldRotation = keyFrames[keyFrameIdx].worldRotations[parentIdx];
			}

			//	is this correct?
			Quat::InvTransformLeft(boneWorldRotation, parentWorldRotation, &keyFrames[keyFrameIdx].boneRotations[boneIdx]);
		}
	}
}