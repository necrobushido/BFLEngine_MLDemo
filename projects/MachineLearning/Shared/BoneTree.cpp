#include "BoneTree.h"

#include "ModelData.h"
#include "AnimationData.h"

#include "AnimGenHyperParameters.h"
#include "BoneSequence.h"

#include "AnimGenBoneDictionary.h"

void BoneTree::Init(const ModelData& modelData)
{
	Clear();

	for(u32 boneIdx = 0; boneIdx < modelData.boneCount; ++boneIdx)
	{
		BoneTreeNode	thisNode;
		thisNode.m_boneData = modelData.bones[boneIdx];

		m_nodes.push_back(thisNode);
	}

	FinishInit();
}

void BoneTree::Init(const AnimationData& animData)
{
	Clear();

	for(u32 boneIdx = 0; boneIdx < animData.numBones; ++boneIdx)
	{
		BoneTreeNode	thisNode;
		thisNode.m_boneData = animData.bones[boneIdx];

		m_nodes.push_back(thisNode);
	}

	FinishInit();
}

void BoneTree::Clear()
{
	m_nodes.clear();
	m_leafNodes.clear();
	m_rootNodes.clear();
}

void BoneTree::FinishInit()
{
	//	have to make sure all of the parents are in the array before setting up the children
	for(int boneIdx = 0; boneIdx < (int)m_nodes.size(); ++boneIdx)
	{
		int	parentNodeIdx = m_nodes[boneIdx].m_boneData.parentBone;
		if( parentNodeIdx >= 0 )
		{
			m_nodes[parentNodeIdx].m_children.push_back(boneIdx);
		}
		else
		{
			//	root nodes
			m_rootNodes.push_back(boneIdx);
		}
	}

	//	find the leaf nodes
	for(int boneIdx = 0; boneIdx < (int)m_nodes.size(); ++boneIdx)
	{
		if( m_nodes[boneIdx].m_children.size() == 0 )
		{
			m_leafNodes.push_back(boneIdx);
		}
	}

	//	set bone influence counts
	for(int rootBoneIdx = 0; rootBoneIdx < (int)m_rootNodes.size(); ++rootBoneIdx)
	{
		int	thisRootNode = m_rootNodes[rootBoneIdx];

		InitBoneInfluenceCountRecurse(thisRootNode);
	}
}

void BoneTree::ConstructSequenceForBone(int boneIdx, std::vector<int>* boneSequenceOut)
{
	boneSequenceOut->clear();
	
	boneSequenceOut->push_back(boneIdx);

	int	boneParent = m_nodes[boneIdx].m_boneData.parentBone;
	while(boneParent >= 0)
	{
		boneSequenceOut->insert(boneSequenceOut->begin(), boneParent);

		boneParent = m_nodes[boneParent].m_boneData.parentBone;
	}
	Assert(boneSequenceOut->size() > 0);
	Assert(boneSequenceOut->size() < AnimGenHyperParameters::kMaxBoneSequenceLength);
}

void BoneTree::ConstructSequenceForBone(int boneIdx, BoneSequence* boneSequenceOut)
{
	std::vector<int>	boneSequenceVec;
	ConstructSequenceForBone(boneIdx, &boneSequenceVec);

	boneSequenceOut->Init(boneSequenceVec);
}

bool BoneTree::IsRootNode(int nodeIdx) const
{
	bool	result = false;
	for(int i = 0; i < (int)m_rootNodes.size() && !result; ++i)
	{
		result = m_rootNodes[i] == nodeIdx;
	}

	return result;
}

std::vector<int> BoneTree::GetChildBones(int nodeIdx) const
{
	Assert(nodeIdx < m_nodes.size());
	return m_nodes[nodeIdx].m_children;
}

torch::Tensor BoneTree::ToTensor()
{
	std::vector<int>	dictionaryBones;
	for(int sequenceIdx = 0; sequenceIdx < (int)m_nodes.size(); ++sequenceIdx)
	{
		int	thisDictionaryBone = g_animGenBoneDictionary->ConvertReferenceBoneToDictionaryBone(sequenceIdx, *this);

		dictionaryBones.push_back(thisDictionaryBone);
	}

	while(dictionaryBones.size() < g_animGenBoneDictionary->GetVocabSize())
	{
		dictionaryBones.push_back(g_animGenBoneDictionary->GetInvalidBoneIdx());
	}

	return torch::from_blob(dictionaryBones.data(), {(int)dictionaryBones.size()}, torch::kInt).clone();
}

int BoneTree::InitBoneInfluenceCountRecurse(int thisBoneIdx)
{
	int	childInfluenceTotal = 0;
	for(int childNodeIdx = 0; childNodeIdx < (int)m_nodes[thisBoneIdx].m_children.size(); ++childNodeIdx)
	{
		int	thisChildNode = m_nodes[thisBoneIdx].m_children[childNodeIdx];

		childInfluenceTotal += InitBoneInfluenceCountRecurse(thisChildNode);
	}

	m_nodes[thisBoneIdx].m_boneInfluenceCount = 1 + childInfluenceTotal;	//	1 for self

	return m_nodes[thisBoneIdx].m_boneInfluenceCount;
}
