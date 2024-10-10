#pragma once

#include "NNModelComponent.h"

#include "NNTextEncoderEpoch.h"

class NNTextEncoder;
class NNScheduler;

class NNTextEncoderComponent : public NNModelComponent
{
public:
	NNTextEncoderComponent();
	virtual ~NNTextEncoderComponent();

public:
	void CalcLoss(torch::Tensor contextText, torch::Tensor targetText, torch::Tensor& lossOut);
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
	bool IsTraining() override;

	void GetBatch(int batchSize, eBatchType batchType, std::map<std::string, torch::Tensor>* tagToBatchTensorMap) override;

protected:
	NNTextEncoder*				m_pNNTextEncoder;
	torch::optim::AdamW*		m_pOptimizer;
	torch::Device				m_deviceToUse;
	f32							m_learningRate;
	std::vector<f32>			m_trainingLossHistory;
	std::vector<f32>			m_validationLossHistory;
	int							m_iterationCount;

	NNScheduler*				m_pScheduler;
	NNTextEncoderEpoch			m_epochGen;
};
