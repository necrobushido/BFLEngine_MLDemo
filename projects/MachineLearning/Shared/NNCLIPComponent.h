#pragma once

#include "NNModelComponent.h"

#include "NNAnimEpoch.h"

class NNCLIP;
class NNScheduler;

class NNCLIPComponent : public NNModelComponent
{
public:
	NNCLIPComponent();
	virtual ~NNCLIPComponent();

public:
	void CalcLoss(torch::Tensor promptText, torch::Tensor latentAnimData, torch::Tensor* pLossOut, std::string* pLossLogOut);
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

	int SubBatchSize() override;
	int SubBatchesPerBatch() override;

	void GetBatch(int batchSize, eBatchType batchType, std::map<std::string, torch::Tensor>* tagToBatchTensorMap) override;

protected:
	NNCLIP*						m_pNNCLIP;
	torch::optim::AdamW*		m_pDefaultOptimizer;
	torch::optim::AdamW*		m_pGoBOptimizer;
	torch::optim::AdamW*		m_pNonGoBOptimizer;
	torch::Device				m_deviceToUse;
	f32							m_learningRate;
	std::vector<f32>			m_trainingLossHistory;
	std::vector<f32>			m_validationLossHistory;
	int							m_iterationCount;

	NNScheduler*				m_pScheduler;
	NNAnimEpoch					m_epochGen;

	f32							m_accumSubBatchLoss;
	int							m_currentSubBatch;
};
