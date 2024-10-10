#include "AnimImage.h"

#include "AnimationIntermediateSeq.h"
#include "EulerSequence.h"
#include "BoolSequence.h"
#include "BoneSequence.h"
#include "AnimGenBoneDictionary.h"

const bool AnimImage::kEulerMethodIsNormalized = true;
const bool AnimImage::kSumWithParentTensors = true;

void AnimImage::Init(const AnimationIntermediateSeq& animData)
{
	m_boneTree = animData.boneTree;

	int	animBoneCount = (int)animData.keyFrames[0].boneRotations.size();

	m_boneKeyframesLocal.clear();
	m_boneKeyframesLocal.resize(animBoneCount);

	{
		//	empty row is zero?
		//m_emptyRow = torch::zeros({AnimGenVAEHyperParameters::kImageWidth, 3});

		//	empty row is noise?
		//m_emptyRow = torch::randn({AnimGenVAEHyperParameters::kImageWidth, 3});

		//	empty row is identity?
		EulerSequence	emptyKeyframeSequence;
		for(int keyframeIdx = 0; keyframeIdx < AnimGenHyperParameters::kMaxKeyframeSequenceLength; ++keyframeIdx)
		{
			emptyKeyframeSequence.AddNext(Mtx33::IDENTITY, kEulerMethodIsNormalized);
		}
		m_emptyRow = emptyKeyframeSequence.ToTensor();
	}

	InitEulerLocal(animData);
	InitEulerWorld(animData);	
	InitEulerLocalInWorld(animData);
}

void AnimImage::InitEulerLocal(const AnimationIntermediateSeq& animData)
{
	torch::NoGradGuard	no_grad;

	int	animBoneCount = (int)animData.keyFrames[0].boneRotations.size();

	for(int boneIdx = 0; boneIdx < animBoneCount; ++boneIdx)
	{
		EulerSequence	thisKeyframeSequence;
		for(int keyframeIdx = 0; keyframeIdx < AnimGenHyperParameters::kMaxKeyframeSequenceLength; ++keyframeIdx)
		{
			Mtx33	thisRotationMtx;
			if( keyframeIdx < (int)animData.keyFrames.size() )
			{
				Quat	thisQuat = animData.keyFrames[keyframeIdx].boneRotations[boneIdx];
				thisQuat.Convert(&thisRotationMtx);
			}
			else
			{
				//	pad out the keyframes to kImageWidth

				//	by identity
				//thisRotationMtx = Mtx33::IDENTITY;

				//	by clamping to the last keyframe
				int		lastKeyframeIdx = (int)animData.keyFrames.size()-1;
				Quat	thisQuat = animData.keyFrames[lastKeyframeIdx].boneRotations[boneIdx];
				thisQuat.Convert(&thisRotationMtx);

				//	could also fill this in with noise?
				//		that might indicate that it wants to be filled in though, and that's not correct
			}

			thisKeyframeSequence.AddNext(thisRotationMtx, kEulerMethodIsNormalized);
		}

		//	{kImageWidth, 3}
		m_boneKeyframesLocal[boneIdx] = thisKeyframeSequence.ToTensor();
	}

	{
		std::vector<torch::Tensor>	boneTensors;
		for(int boneIdx = 0; boneIdx < g_animGenBoneDictionary->GetVocabSize(); ++boneIdx)
		{
			if( boneIdx < (int)m_boneTree.m_nodes.size() )
			{
				torch::Tensor	thisTensor = m_boneKeyframesLocal[boneIdx];
				boneTensors.push_back(thisTensor);
			}
			else
			{
				boneTensors.push_back(m_emptyRow);
			}
		}

		m_fullImageLocal = torch::stack(boneTensors);
	}
}

void AnimImage::InitEulerWorld(const AnimationIntermediateSeq& animData)
{
	torch::NoGradGuard	no_grad;

	m_fullImageWorld = HierarchySumFullTensor(m_fullImageLocal, m_boneTree);
	m_fullImageWorld = EulerWorldNormalize(m_fullImageWorld);	
}

void AnimImage::InitEulerLocalInWorld(const AnimationIntermediateSeq& animData)
{
	torch::NoGradGuard	no_grad;

	m_fullImageLocalInWorld = EulerWorldNormalize(m_fullImageLocal);	
}

void AnimImage::HierarchySumFullTensorRecurse(int boneIdx, int parentIdx, torch::Tensor& resultImageTensor, torch::Tensor sourceImageTensor, const BoneTree& boneTree)
{
	if( parentIdx >= 0 )
	{
		resultImageTensor[boneIdx] += sourceImageTensor[parentIdx];
	}

	std::vector<int>	childBones = boneTree.GetChildBones(boneIdx);
	for(int childBoneIdx = 0; childBoneIdx < (int)childBones.size(); ++childBoneIdx)
	{
		HierarchySumFullTensorRecurse(childBones[childBoneIdx], boneIdx, resultImageTensor, sourceImageTensor, boneTree);
	}
}

torch::Tensor AnimImage::HierarchySumFullTensor(torch::Tensor imageTensor, const BoneTree& boneTree)
{
	torch::NoGradGuard	no_grad;

	torch::Tensor		resultImageTensor = imageTensor;

	//	go through all of the bone tensors and add the value of their parents to them
	//	we have to go down in tree order to do it right
	for(int rootBoneIdx = 0; rootBoneIdx < (int)boneTree.m_rootNodes.size(); ++rootBoneIdx)
	{		
		HierarchySumFullTensorRecurse(boneTree.m_rootNodes[rootBoneIdx], -1, resultImageTensor, imageTensor, boneTree);
	}

	return resultImageTensor;
}

void AnimImage::HierarchySubtractFullTensorRecurse(int boneIdx, int parentIdx, torch::Tensor& resultImageTensor, torch::Tensor sourceImageTensor, const BoneTree& boneTree)
{
	if( parentIdx >= 0 )
	{
		resultImageTensor[boneIdx] -= sourceImageTensor[parentIdx];
	}

	std::vector<int>	childBones = boneTree.GetChildBones(boneIdx);
	for(int childBoneIdx = 0; childBoneIdx < (int)childBones.size(); ++childBoneIdx)
	{
		HierarchySubtractFullTensorRecurse(childBones[childBoneIdx], boneIdx, resultImageTensor, sourceImageTensor, boneTree);
	}
}

torch::Tensor AnimImage::HierarchySubtractFullTensor(torch::Tensor imageTensor, const BoneTree& boneTree)
{
	torch::NoGradGuard	no_grad;

	torch::Tensor		resultImageTensor = imageTensor;

	for(int rootBoneIdx = 0; rootBoneIdx < (int)boneTree.m_rootNodes.size(); ++rootBoneIdx)
	{		
		HierarchySubtractFullTensorRecurse(boneTree.m_rootNodes[rootBoneIdx], -1, resultImageTensor, imageTensor, boneTree);
	}

	return resultImageTensor;
}

torch::Tensor AnimImage::EulerWorldNormalize(torch::Tensor imageTensor)
{
	return imageTensor * 1.0f / (float)AnimGenHyperParameters::kMaxBoneSequenceLength;
}

torch::Tensor AnimImage::EulerWorldDenormalize(torch::Tensor imageTensor)
{
	 return imageTensor * (float)AnimGenHyperParameters::kMaxBoneSequenceLength;
}

void AnimImage::ToDevice(torch::Device device)
{
	/*for(int i = 0; i < (int)m_boneKeyframes.size(); ++i)
	{
		m_boneKeyframes[i] = m_boneKeyframes[i].to(device);
	}

	m_emptyRow = m_emptyRow.to(device);
	m_keyframePadMask = m_keyframePadMask.to(device);
	m_fullPadMask = m_fullPadMask.to(device);*/
}

torch::Tensor AnimImage::GetWorldKeyframeImageForWholeAnim()
{
	torch::NoGradGuard	no_grad;

	return m_fullImageWorld.clone();
}

torch::Tensor AnimImage::GetLocalKeyframeImageForWholeAnim()
{
	torch::NoGradGuard	no_grad;

	return m_fullImageLocal.clone();
}

torch::Tensor AnimImage::GetLocalInWorldKeyframeImageForWholeAnim()
{
	torch::NoGradGuard	no_grad;

	return m_fullImageLocalInWorld.clone();
}

torch::Tensor AnimImage::GetBoneTensorForWholeAnim()
{
	torch::NoGradGuard	no_grad;

	return m_boneTree.ToTensor();
}