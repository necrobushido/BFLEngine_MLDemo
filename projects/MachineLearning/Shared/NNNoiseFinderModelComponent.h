#pragma once

#include "NNModelComponent.h"
#include "NNScheduling.h"
#include "NNNoiseFinderEpoch.h"

class NNNoiseFinder;

typedef void (*GenProgressFunc)();

class NNNoiseFinderModelComponent : public NNModelComponent
{
public:
	NNNoiseFinderModelComponent();
	virtual ~NNNoiseFinderModelComponent();

public:
	void TrainingIteration(std::map<std::string, torch::Tensor>& tagToBatchTensorMap, bool shouldPrint) override;
	void ValidationIteration(std::map<std::string, torch::Tensor>& tagToBatchTensorMap) override;
	void Save() override;
	void Load() override;

	void TrainMode() override;
	void EvalMode() override;

	void UpdateOptimizer() override;
	void SetLearningRate(f32 learningRate) override;
	void SetDevice(torch::Device device) override;
	const std::vector<f32>* GetLossHistory() override;
	void ResetLossHistory() override;
	bool RequestTrainingEnd() override;
	bool IsTraining() override;

	int SubBatchSize() override;

	void GetBatch(int batchSize, eBatchType batchType, std::map<std::string, torch::Tensor>* tagToBatchTensorMap) override;

	torch::Tensor SampleFromLatent(torch::Tensor inputLatentAnimDesc, torch::Tensor encodedPrompt, GenProgressFunc progressFunc = nullptr);
	torch::Tensor SampleFromNoise(torch::Tensor inputNoise, torch::Tensor encodedPrompt, GenProgressFunc progressFunc = nullptr);

	torch::Tensor SampleFromNoiseWithCFG(torch::Tensor inputNoise, torch::Tensor encodedPrompt, torch::Tensor encodedNegativePrompt, double scaleCFG=7.5, GenProgressFunc progressFunc = nullptr);

protected:
	torch::Tensor Sample(torch::Tensor input, torch::Tensor encodedPrompt, GenProgressFunc progressFunc);
	torch::Tensor SampleWithCFG(torch::Tensor input, torch::Tensor encodedPrompt, torch::Tensor encodedNegativePrompt, double scaleCFG, GenProgressFunc progressFunc);

protected:
	NNNoiseFinder*				m_pNNNoiseFinder;
	//torch::optim::SGD*		m_pOptimizer;
	//torch::optim::AdamW*		m_pOptimizer;
	torch::optim::Optimizer*	m_pOptimizer;
	torch::Device				m_deviceToUse;
	f32							m_learningRate;
	std::vector<f32>			m_trainingLossHistory;
	std::vector<f32>			m_validationLossHistory;
	bool						m_requestTrainingEnd;
	int							m_iterationCount;
	NNScheduler*				m_pScheduler;

	f32							m_accumSubBatchLoss;
	int							m_currentSubBatch;
	NNNoiseFinderEpoch			m_epochGen;
	int							m_validationBatchCount;

	int							m_currentBatchSize;
	int							m_currentSubBatchesPerBatch;
};
