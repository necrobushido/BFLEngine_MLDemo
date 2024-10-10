#pragma once

#include "types.h"
#include "SharedMLHeader.h"

#include "BoneTree.h"

class AnimationIntermediateSeq;

class AnimImageC3DR6
{
public:
	void Init(const AnimationIntermediateSeq& animData);

	torch::Tensor GetWorldKeyframeImageForWholeAnim();
	torch::Tensor GetLocalKeyframeImageForWholeAnim();
	torch::Tensor GetBoneTensorForWholeAnim();

protected:
	void InitC3DR6World(const AnimationIntermediateSeq& animData);
	void InitC3DR6Local(const AnimationIntermediateSeq& animData);

private:
	std::vector<torch::Tensor>	m_boneKeyframesWorld;	//	each should be {AnimGenVAEHyperParameters::kImageWidth}
	std::vector<torch::Tensor>	m_boneKeyframesLocal;
	torch::Tensor				m_emptyRow;
	torch::Tensor				m_fullImageWorld;
	torch::Tensor				m_fullImageLocal;
	BoneTree					m_boneTree;
};