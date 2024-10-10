#pragma once

#include "NNModelComponent.h"

class NNVAE;

class NNVAEModelComponent : public NNModelComponent
{
public:
	NNVAEModelComponent();
	virtual ~NNVAEModelComponent();

public:
	void CalcLoss(torch::Tensor inputAnimData, torch::Tensor& lossOut, std::string& logOut);
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

	void GetBatch(int batchSize, eBatchType batchType, std::map<std::string, torch::Tensor>* tagToBatchTensorMap) override;

protected:
	NNVAE*					m_pNNVAE;
	torch::optim::AdamW*	m_pOptimizer;
	torch::Device			m_deviceToUse;
	f32						m_learningRate;
	std::vector<f32>		m_trainingLossHistory;
	std::vector<f32>		m_validationLossHistory;
	bool					m_requestTrainingEnd;
};
