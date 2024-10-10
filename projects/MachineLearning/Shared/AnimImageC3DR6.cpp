#include "AnimImageC3DR6.h"

#include "AnimationIntermediateSeq.h"
#include "C3DR6Sequence.h"
#include "BoolSequence.h"
#include "BoneSequence.h"
#include "AnimGenBoneDictionary.h"

void AnimImageC3DR6::Init(const AnimationIntermediateSeq& animData)
{
	m_boneTree = animData.boneTree;

	int	animBoneCount = (int)m_boneTree.m_nodes.size();

	m_boneKeyframesWorld.clear();
	m_boneKeyframesWorld.resize(animBoneCount);

	m_boneKeyframesLocal.clear();
	m_boneKeyframesLocal.resize(animBoneCount);

	{
		//	empty row is zero?
		//m_emptyRow = torch::zeros({AnimGenVAEHyperParameters::kImageWidth, Continuous3DRotation6::kElementCount});

		//	empty row is noise?
		//m_emptyRow = torch::randn({AnimGenVAEHyperParameters::kImageWidth, Continuous3DRotation6::kElementCount});

		//	empty row is identity?
		C3DR6Sequence	emptyKeyframeSequence;
		for(int keyframeIdx = 0; keyframeIdx < AnimGenHyperParameters::kMaxKeyframeSequenceLength; ++keyframeIdx)
		{
			emptyKeyframeSequence.AddNext(Mtx33::IDENTITY);
		}
		m_emptyRow = emptyKeyframeSequence.ToTensor();
	}

	InitC3DR6World(animData);
	InitC3DR6Local(animData);
}

void AnimImageC3DR6::InitC3DR6World(const AnimationIntermediateSeq& animData)
{
	torch::NoGradGuard	no_grad;

	int	animBoneCount = (int)m_boneTree.m_nodes.size();

	for(int boneIdx = 0; boneIdx < animBoneCount; ++boneIdx)
	{
		C3DR6Sequence	thisKeyframeSequence;
		for(int keyframeIdx = 0; keyframeIdx < AnimGenHyperParameters::kMaxKeyframeSequenceLength; ++keyframeIdx)
		{
			Mtx33	thisRotationMtx;
			if( keyframeIdx < (int)animData.keyFrames.size() )
			{
				Quat	thisQuat = animData.keyFrames[keyframeIdx].worldRotations[boneIdx];
				thisQuat.Convert(&thisRotationMtx);
			}
			else
			{
				//	pad out the keyframes to kImageWidth

				//	by identity
				//thisRotationMtx = Mtx33::IDENTITY;

				//	by clamping to the last keyframe
				int		lastKeyframeIdx = (int)animData.keyFrames.size()-1;
				Quat	thisQuat = animData.keyFrames[lastKeyframeIdx].worldRotations[boneIdx];
				thisQuat.Convert(&thisRotationMtx);

				//	could also fill this in with noise?
				//		that might indicate that it wants to be filled in though, and that's not correct
			}

			thisKeyframeSequence.AddNext(thisRotationMtx);
		}

		//	{kImageWidth, Continuous3DRotation6::kElementCount}
		m_boneKeyframesWorld[boneIdx] = thisKeyframeSequence.ToTensor();
	}

	{
		std::vector<torch::Tensor>	boneTensors;
		for(int boneIdx = 0; boneIdx < g_animGenBoneDictionary->GetVocabSize(); ++boneIdx)
		{
			if( boneIdx < (int)m_boneTree.m_nodes.size() )
			{
				torch::Tensor	thisTensor = m_boneKeyframesWorld[boneIdx];
				boneTensors.push_back(thisTensor);
			}
			else
			{
				boneTensors.push_back(m_emptyRow);
			}
		}

		m_fullImageWorld = torch::stack(boneTensors);
	}
}

void AnimImageC3DR6::InitC3DR6Local(const AnimationIntermediateSeq& animData)
{
	torch::NoGradGuard	no_grad;

	int	animBoneCount = (int)m_boneTree.m_nodes.size();

	for(int boneIdx = 0; boneIdx < animBoneCount; ++boneIdx)
	{
		C3DR6Sequence	thisKeyframeSequence;
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

			thisKeyframeSequence.AddNext(thisRotationMtx);
		}

		//	{kImageWidth, Continuous3DRotation6::kElementCount}
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

torch::Tensor AnimImageC3DR6::GetWorldKeyframeImageForWholeAnim()
{
	torch::NoGradGuard	no_grad;

	return m_fullImageWorld.clone();
}

torch::Tensor AnimImageC3DR6::GetLocalKeyframeImageForWholeAnim()
{
	torch::NoGradGuard	no_grad;

	return m_fullImageLocal.clone();
}

torch::Tensor AnimImageC3DR6::GetBoneTensorForWholeAnim()
{
	torch::NoGradGuard	no_grad;

	return m_boneTree.ToTensor();
}