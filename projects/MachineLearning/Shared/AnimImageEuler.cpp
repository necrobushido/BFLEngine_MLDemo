#include "AnimImageEuler.h"

#include "AnimationIntermediateSeq.h"
#include "EulerSequence.h"
#include "AnimGenBoneDictionary.h"
#include "MathNamespace.h"
#include "AnimGenHyperParameters.h"

void AnimImageEuler::Init(const AnimationIntermediateSeq& animData)
{
	m_boneTree = animData.boneTree;

	int	animBoneCount = (int)m_boneTree.m_nodes.size();

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
			emptyKeyframeSequence.AddNext(Mtx33::IDENTITY, false);
		}
		m_emptyRow = emptyKeyframeSequence.ToTensor();
	}

	InitEulerLocal(animData);
	InitEulerWorld(animData);
}

void AnimImageEuler::InitEulerWorld(const AnimationIntermediateSeq& animData)
{
	torch::NoGradGuard	no_grad;

	m_fullImageWorld = HierarchySumFullTensor(m_fullImageLocal, m_boneTree);
}

void AnimImageEuler::InitEulerLocal(const AnimationIntermediateSeq& animData)
{
	torch::NoGradGuard	no_grad;

	int	animBoneCount = (int)m_boneTree.m_nodes.size();

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

			thisKeyframeSequence.AddNext(thisRotationMtx, false);
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

torch::Tensor AnimImageEuler::GetWorldKeyframeImageForWholeAnim()
{
	torch::NoGradGuard	no_grad;

	return m_fullImageWorld.clone();
}

torch::Tensor AnimImageEuler::GetLocalKeyframeImageForWholeAnim()
{
	torch::NoGradGuard	no_grad;

	return m_fullImageLocal.clone();
}

torch::Tensor AnimImageEuler::GetBoneTensorForWholeAnim()
{
	torch::NoGradGuard	no_grad;

	return m_boneTree.ToTensor();
}

void AnimImageEuler::HierarchySumFullTensorRecurse(int boneIdx, int parentIdx, torch::Tensor& resultWorldImageTensor, torch::Tensor sourceLocalImageTensor, const BoneTree& boneTree)
{
	//	this should have been called by our parent, so their value has been set
	//	set our world value by adding the parent value
	if( parentIdx >= 0 )
	{
		resultWorldImageTensor[boneIdx] = sourceLocalImageTensor[boneIdx] + resultWorldImageTensor[parentIdx];
	}
	else
	{
		resultWorldImageTensor[boneIdx] = sourceLocalImageTensor[boneIdx];
	}

	std::vector<int>	childBones = boneTree.GetChildBones(boneIdx);
	for(int childBoneIdx = 0; childBoneIdx < (int)childBones.size(); ++childBoneIdx)
	{
		HierarchySumFullTensorRecurse(childBones[childBoneIdx], boneIdx, resultWorldImageTensor, sourceLocalImageTensor, boneTree);
	}
}

torch::Tensor AnimImageEuler::HierarchySumFullTensor(torch::Tensor sourceLocalImageTensor, const BoneTree& boneTree)
{
	torch::NoGradGuard	no_grad;

	torch::Tensor		resultWorldImageTensor = sourceLocalImageTensor.clone();

	//	go through all of the bone tensors and add the value of their parents to them
	//	we have to go down in tree order to do it right
	for(int rootBoneIdx = 0; rootBoneIdx < (int)boneTree.m_rootNodes.size(); ++rootBoneIdx)
	{		
		HierarchySumFullTensorRecurse(boneTree.m_rootNodes[rootBoneIdx], -1, resultWorldImageTensor, sourceLocalImageTensor, boneTree);
	}

	return resultWorldImageTensor;
}

//torch::Tensor AnimImageEuler::HierarchySumFullTensor(torch::Tensor sourceLocalImageTensor, const BoneTree& boneTree)
//{
//	torch::NoGradGuard	no_grad;
//
//	//torch::Tensor		resultWorldImageTensor = sourceLocalImageTensor.clone();
//	torch::Tensor		resultWorldImageTensor = torch::zeros_like(sourceLocalImageTensor);
//
//	std::vector<int>	pendingBoneIndices;
//	for(int rootBoneIdx = 0; rootBoneIdx < (int)boneTree.m_rootNodes.size(); ++rootBoneIdx)
//	{		
//		pendingBoneIndices.push_back(boneTree.m_rootNodes[rootBoneIdx]);
//	}
//
//	while((int)pendingBoneIndices.size() > 0)
//	{
//		int	thisBoneIdx = pendingBoneIndices[0];
//		int	parentIdx = boneTree.m_nodes[thisBoneIdx].m_boneData.parentBone;
//
//		if( parentIdx >= 0 )
//		{
//			resultWorldImageTensor[thisBoneIdx] = sourceLocalImageTensor[thisBoneIdx] + resultWorldImageTensor[parentIdx];
//		}
//		else
//		{
//			resultWorldImageTensor[thisBoneIdx] = sourceLocalImageTensor[thisBoneIdx];
//		}
//
//		std::vector<int>	childBones = boneTree.GetChildBones(thisBoneIdx);
//		for(int childBoneIdx = 0; childBoneIdx < (int)childBones.size(); ++childBoneIdx)
//		{
//			pendingBoneIndices.push_back(childBones[childBoneIdx]);
//		}
//
//		//	remove the first
//		pendingBoneIndices.erase( pendingBoneIndices.begin() + 0 );
//	}
//
//
//	return resultWorldImageTensor;
//}