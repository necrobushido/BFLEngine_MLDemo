#pragma once

#include "types.h"
#include "AnimGenHyperParameters.h"
#include "BoneData.h"
#include "SharedMLHeader.h"

class BoneTree;

class BoneSequence
{
public:
	enum
	{
		kElementCount = AnimGenHyperParameters::kMaxBoneSequenceLength,
		kParentElementCount = AnimGenHyperParameters::kMaxBoneSequenceLength-1
	};

	enum eBoneSequenceSpace
	{
		kAssetSpace,
		kDictionarySpace,

		kNumCoordinateSpaces
	};

public:
	BoneSequence();

	void Init(const std::vector<int>& sequenceIn);

	void ConvertToDictionaryBoneIndices(const BoneTree& referenceBoneTree);

	void AddNext(int nextInSequence);

	torch::Tensor ToTensor(const BoneTree& referenceBoneTree) const;
	torch::Tensor ToParentTensor(const BoneTree& referenceBoneTree) const;

	void DebugPrint(const char* label);

	int* Data() const { return (int*)&m_sequence[0]; }

	int BoneCount() const;
	int GetBoneByIdx(int idx) const;

public:
	int					m_sequence[kElementCount];
	eBoneSequenceSpace	m_space;
	int					m_sequenceStartPos;
};
