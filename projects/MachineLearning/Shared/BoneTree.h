#pragma once

#include "types.h"
#include "BoneData.h"
#include "Tensorizable.h"

class ModelData;
class AnimationData;
class BoneSequence;

class BoneTreeNode
{
public:
	BoneTreeNode():
		m_boneInfluenceCount(0)
	{
	}

public:
	BoneData			m_boneData;
	std::vector<int>	m_children;
	int					m_boneInfluenceCount;	//	I added this in case I wanted to use it as a weighting in loss calculations; e.g. "multiply loss by 1 + ln(m_boneInfluenceCount)"
};

class BoneTree : public ITensorizable
{
public:
	void Init(const ModelData& modelData);
	void Init(const AnimationData& animData);

	void Clear();

	void ConstructSequenceForBone(int boneIdx, std::vector<int>* boneSequenceOut);
	void ConstructSequenceForBone(int boneIdx, BoneSequence* boneSequenceOut);

	bool IsRootNode(int nodeIdx) const;

	std::vector<int> GetChildBones(int nodeIdx) const;

	torch::Tensor ToTensor() override;

protected:
	void FinishInit();

	int InitBoneInfluenceCountRecurse(int thisBoneIdx);

public:
	std::vector<BoneTreeNode>	m_nodes;
	std::vector<int>			m_leafNodes;
	std::vector<int>			m_rootNodes;
};