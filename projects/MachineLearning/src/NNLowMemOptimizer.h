#pragma once

#include "types.h"
#include "SharedMLHeader.h"

class NNLowMemOptimizer
{
public:
	NNLowMemOptimizer();
	~NNLowMemOptimizer();

	void Init(std::vector<torch::Tensor> params, float learningRate);
	void Clear();

	bool Initialized() const;

	void SetLearningRate(float learningRate);

private:
	std::vector<torch::optim::AdamW*>	m_subOptimizers;
};