#pragma once

#include "types.h"

#include "SharedMLHeader.h"

class NNBatch
{
public:
	void Clear();
	void GetBatchData(int batchSize, eBatchType batchType);
	void MoveToDevice(torch::Device	device);

	std::map<std::string, torch::Tensor>	m_trainingTagToBatchTensorMap;
	std::map<std::string, torch::Tensor>	m_validationTagToBatchTensorMap;
};
