#pragma once

#include "types.h"
#include "ListMacros.h"
#include "SharedMLHeader.h"

class NNModelComponent
{
public:
	NNModelComponent();
	virtual ~NNModelComponent();

public:
	virtual void TrainingIteration(std::map<std::string, torch::Tensor>& tagToBatchTensorMap, bool shouldPrint){}
	virtual void ValidationIteration(std::map<std::string, torch::Tensor>& tagToBatchTensorMap){}
	virtual void Save(){}
	virtual void Load(){}
	virtual void TrainMode(){}
	virtual void EvalMode(){}
	virtual void UpdateOptimizer(){}
	virtual void SetLearningRate(f32 learningRate){}
	virtual void SetDevice(torch::Device device){}
	virtual const std::vector<f32>* GetLossHistory(){ return nullptr; }
	virtual void ResetLossHistory(){}
	virtual bool RequestTrainingEnd(){ return false; }
	virtual bool IsTraining(){ return false; }
	virtual int SubBatchSize(){ return 2; }
	virtual int SubBatchesPerBatch(){ return 1; }

	//	keep in mind that this will be called from another thread, in case synchronization needs to happen
	virtual void GetBatch(int batchSize, eBatchType batchType, std::map<std::string, torch::Tensor>* tagToBatchTensorMap){}

public:
	static void SaveModule(torch::nn::Module* moduleToSave, const char* fileName);
	static void LoadModule(torch::nn::Module* moduleToLoad, const char* fileName);

public:
	LIST_LINK(AllComponents, NNModelComponent);

	struct NNModelComponentList 
	{
		LIST_DECLARE(AllComponents, NNModelComponent);
	};
	static NNModelComponentList	s_componentList;
};
