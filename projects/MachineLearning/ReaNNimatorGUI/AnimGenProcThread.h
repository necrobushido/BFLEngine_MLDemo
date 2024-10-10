#pragma once

#include "ModelIntermediate.h"
#include "AnimationIntermediateSeq.h"
#include "AnimGenProcThreadBase.h"

class NNModelComponent;

class NNVAEModelComponent;

class NNTextEncoderComponent;

class NNDDPMModelComponent;
class NNNoiseFinderModelComponent;

class NNTokenizerModelComponent;

class NNCLIPComponent;

typedef void (*GenProgressFunc)();

class AnimGenProcThread : public AnimGenProcessingThreadBase
{
public:
	AnimGenProcThread();
	virtual ~AnimGenProcThread();

public:
	void SetCFG(double amount){ m_CFGAmount = amount; }
	void SetSampleCount(int sampleCount) override;
	void UpdateTargetModel(const char* modelToUse) override;
	void ExportGeneratedAnim(AnimationData* animData) override;
	int ExportGeneratedAnims(AnimationData* animDataArrayOut, int animDataCount);

	void SetPrompt(const std::string& inputPrompt){ m_generationPrompt = inputPrompt; }

	void SetGenProgressFunc(GenProgressFunc funcToUse);

protected:
	void Generate() override;

	void NoiseFinderGenerate(int sampleCount);

	void DecodedDataToAnim(torch::Tensor decodedDataIn, AnimationIntermediateSeq* animOut);

protected:
	NNVAEModelComponent*			m_pNNVAE2;

	NNTextEncoderComponent*			m_NNTextEncoder;

	NNDDPMModelComponent*			m_NNDDPMModelComponent;
	NNNoiseFinderModelComponent*	m_NNNoiseFinderModelComponent;

	NNTokenizerModelComponent*		m_NNTokenizerModelComponent;

	NNCLIPComponent*				m_NNCLIPComponent;

	ModelIntermediate				m_targetModelIntermediate;
	AnimationIntermediateSeq		m_generatedAnimIntermediate;

	std::string						m_generationPrompt;

	int								m_sampleCount;
	torch::Tensor					m_lastGeneratedData;

	double							m_CFGAmount;

	GenProgressFunc					m_genProgFunc;
};