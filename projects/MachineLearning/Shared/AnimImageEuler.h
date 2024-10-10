#pragma once

#include "types.h"
#include "SharedMLHeader.h"

#include "BoneTree.h"

class AnimationIntermediateSeq;

class AnimImageEuler
{
public:
	void Init(const AnimationIntermediateSeq& animData);

	torch::Tensor GetWorldKeyframeImageForWholeAnim();
	torch::Tensor GetLocalKeyframeImageForWholeAnim();
	torch::Tensor GetBoneTensorForWholeAnim();

protected:
	void InitEulerWorld(const AnimationIntermediateSeq& animData);
	void InitEulerLocal(const AnimationIntermediateSeq& animData);

	void HierarchySumFullTensorRecurse(int boneIdx, int parentIdx, torch::Tensor& resultWorldImageTensor, torch::Tensor sourceLocalImageTensor, const BoneTree& boneTree);
	torch::Tensor HierarchySumFullTensor(torch::Tensor sourceLocalImageTensor, const BoneTree& boneTree);

private:
	std::vector<torch::Tensor>	m_boneKeyframesLocal;
	torch::Tensor				m_emptyRow;
	torch::Tensor				m_fullImageWorld;
	torch::Tensor				m_fullImageLocal;
	BoneTree					m_boneTree;
};