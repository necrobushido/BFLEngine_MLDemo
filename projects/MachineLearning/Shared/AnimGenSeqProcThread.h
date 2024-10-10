#pragma once

#include "ModelIntermediate.h"
#include "AnimationIntermediateSeq.h"
#include "AnimSeqDataSet.h"
#include "NNBatchingProcThread.h"
#include "AnimGenProcThreadBase.h"
#include "NNBatch.h"

class NNModelComponent;

class NNVAEModelComponent;

class NNTextEncoderComponent;

class NNDDPMModelComponent;
class NNNoiseFinderModelComponent;

class NNTokenizerModelComponent;

class NNCLIPComponent;

typedef void (*LogFunc)(const char* logLine);

class AnimGenSeqProcessingThread : public AnimGenProcessingThreadBase
{
public:
	AnimGenSeqProcessingThread();
	virtual ~AnimGenSeqProcessingThread();

public:
	void SetSampleCount(int sampleCount) override;
	void UpdateTargetModel(const char* modelToUse) override;
	void ExportGeneratedAnim(AnimationData* animData) override;
	int ExportGeneratedAnims(AnimationData* animDataArrayOut, int animDataCount) override;

protected:
	void EnteringState(int currentState) override;

	void Generate() override;

	void VAEGenerate();
	void NoiseFinderGenerate(int sampleCount);
	void TextEncoderGenerate();

	void DecodedDataToAnim(torch::Tensor decodedDataIn, AnimationIntermediateSeq* animOut);

	void FillRandomAnim();

protected:
	NNVAEModelComponent*			m_pNNVAE2;

	NNTextEncoderComponent*			m_NNTextEncoder;

	NNDDPMModelComponent*			m_NNDDPMModelComponent;
	NNNoiseFinderModelComponent*	m_NNNoiseFinderModelComponent;

	NNTokenizerModelComponent*		m_NNTokenizerModelComponent;

	NNCLIPComponent*				m_NNCLIPComponent;

	AnimSeqDataSet					m_dataSet;
	ModelIntermediate				m_targetModelIntermediate;
	AnimationIntermediateSeq		m_generatedAnimIntermediate;

	int								m_sampleCount;

	int								m_mode;

	torch::Tensor					m_lastGeneratedData;
};