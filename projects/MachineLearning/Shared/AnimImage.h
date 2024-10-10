#pragma once

#include "types.h"
#include "SharedMLHeader.h"

#include "BoneTree.h"

class AnimationIntermediateSeq;

class AnimImage
{
public:
	static const bool kEulerMethodIsNormalized;
	static const bool kSumWithParentTensors;

public:
	void Init(const AnimationIntermediateSeq& animData);

	torch::Tensor GetWorldKeyframeImageForWholeAnim();
	torch::Tensor GetLocalKeyframeImageForWholeAnim();
	torch::Tensor GetLocalInWorldKeyframeImageForWholeAnim();
	torch::Tensor GetBoneTensorForWholeAnim();

	void ToDevice(torch::Device device);

protected:
	void InitEulerWorld(const AnimationIntermediateSeq& animData);
	void InitEulerLocal(const AnimationIntermediateSeq& animData);
	void InitEulerLocalInWorld(const AnimationIntermediateSeq& animData);

public:
	static torch::Tensor HierarchySumFullTensor(torch::Tensor imageTensor, const BoneTree& boneTree);	
	static torch::Tensor HierarchySubtractFullTensor(torch::Tensor imageTensor, const BoneTree& boneTree);

	static torch::Tensor EulerWorldNormalize(torch::Tensor imageTensor);
	static torch::Tensor EulerWorldDenormalize(torch::Tensor imageTensor);

protected:
	static void HierarchySumFullTensorRecurse(int boneIdx, int parentIdx, torch::Tensor& resultImageTensor, torch::Tensor sourceImageTensor, const BoneTree& boneTree);
	static void HierarchySubtractFullTensorRecurse(int boneIdx, int parentIdx, torch::Tensor& resultImageTensor, torch::Tensor sourceImageTensor, const BoneTree& boneTree);

private:
	std::vector<torch::Tensor>	m_boneKeyframesLocal;
	torch::Tensor				m_emptyRow;
	torch::Tensor				m_fullImageWorld;
	torch::Tensor				m_fullImageLocal;
	torch::Tensor				m_fullImageLocalInWorld;
	BoneTree					m_boneTree;
};