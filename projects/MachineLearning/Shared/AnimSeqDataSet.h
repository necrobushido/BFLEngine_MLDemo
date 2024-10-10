#pragma once

#include "types.h"

#include "SharedMLHeader.h"

#include "AnimationIntermediateSeq.h"
#include "AnimGenBoneDictionary.h"
#include "AnimGenDescDictionary.h"

#include "Mtx33Sequence.h"
#include "C3DR6Sequence.h"
#include "BoneSequence.h"

class AnimSeqDataSet
{
public:
	AnimSeqDataSet();
	~AnimSeqDataSet();

	void CreateData();
	void CreateTextEncoderTrainingData();
	void CreateConcatAnimDescFile();

	int GetRandomTrainingAnimIdx() const;
	int GetRandomValidationAnimIdx() const;

	//	this doesn't move everything, just some stuff
	void ToDevice(torch::Device device);

public:
	AnimGenBoneDictionary					m_boneDictionary;
	std::vector<AnimationIntermediateSeq>	m_trainingAnims;
	std::vector<AnimationIntermediateSeq>	m_validationAnims;

	torch::Tensor							m_fullText;
	torch::Tensor							m_trainingText;
	torch::Tensor							m_validationText;
};

extern AnimSeqDataSet* g_animSeqDataSet;
