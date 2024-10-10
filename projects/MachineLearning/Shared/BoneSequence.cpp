#include "BoneSequence.h"

#include "AnimGenBoneDictionary.h"
#include "BoneTree.h"

BoneSequence::BoneSequence():
	m_space(kAssetSpace),
	m_sequenceStartPos(kElementCount)
{
	for(int i = 0; i < kElementCount; ++i)
	{
		m_sequence[i] = -1;
	}
}

void BoneSequence::Init(const std::vector<int>& sequenceIn)
{
	int	invalidBoneIdx = -1;
	int	sequencePos = 0;
	int	padBones = std::max(1, kElementCount - (int)sequenceIn.size());	//	pad the input sequence with invalid bones to the left.  make sure that we have at least 1 pad bone (to signal start of sequence)
	Assert(sequenceIn.size() > 0);
	Assert(padBones >= 0);
	for(int sequenceIdx = 0; sequenceIdx < padBones; ++sequenceIdx)
	{
		m_sequence[sequencePos] = invalidBoneIdx;

		sequencePos++;
	}
	
	m_sequenceStartPos = sequencePos;
	for(int sequenceIdx = 0; sequenceIdx < sequenceIn.size(); ++sequenceIdx)
	{
		int	targetModelBoneIdx = sequenceIn[sequenceIdx];

		m_sequence[sequencePos] = targetModelBoneIdx;

		sequencePos++;
	}

	Assert(sequencePos == kElementCount);

	m_space = kAssetSpace;
}

//	this maps from model/animation bone indices to the dictionary's bone indices
void BoneSequence::ConvertToDictionaryBoneIndices(const BoneTree& referenceBoneTree)
{
	for(int sequenceIdx = 0; sequenceIdx < kElementCount; ++sequenceIdx)
	{
		m_sequence[sequenceIdx] = g_animGenBoneDictionary->ConvertReferenceBoneToDictionaryBone(m_sequence[sequenceIdx], referenceBoneTree);
	}

	m_space = kDictionarySpace;
}

void BoneSequence::AddNext(int nextInSequence)
{
	Assert(m_space == kAssetSpace);
	Assert(m_sequence[0] == -1);
	Assert(m_sequence[1] == -1);

	memcpy(&m_sequence[0], &m_sequence[1], (kElementCount - 1) * sizeof(m_sequence[0]));

	m_sequence[kElementCount-1] = nextInSequence;
	m_sequenceStartPos--;
	Assert(m_sequenceStartPos >= 0);
}

torch::Tensor BoneSequence::ToTensor(const BoneTree& referenceBoneTree) const
{
	BoneSequence	referenceSeq = *this;
	referenceSeq.ConvertToDictionaryBoneIndices(referenceBoneTree);

	return torch::from_blob(referenceSeq.Data(), {kElementCount}, torch::kInt).clone();
}

torch::Tensor BoneSequence::ToParentTensor(const BoneTree& referenceBoneTree) const
{
	BoneSequence	referenceSeq = *this;
	referenceSeq.ConvertToDictionaryBoneIndices(referenceBoneTree);

	return torch::from_blob(referenceSeq.Data(), {kElementCount-1}, torch::kInt).clone();
}

void BoneSequence::DebugPrint(const char* label)
{
	DebugPrintf("BoneSequence : %s\n", label);
	for(int i = 0; i < kElementCount; ++i)
	{
		char	printBuffer[256];
		sprintf(printBuffer, "%d ", m_sequence[i]);
		
		DebugPrintf(printBuffer);		
	}

	DebugPrintf("\n");
}

int BoneSequence::BoneCount() const
{
	return kElementCount - m_sequenceStartPos;
}

int BoneSequence::GetBoneByIdx(int idx) const
{
	Assert(m_sequenceStartPos + idx < kElementCount);
	return m_sequence[m_sequenceStartPos + idx];
}