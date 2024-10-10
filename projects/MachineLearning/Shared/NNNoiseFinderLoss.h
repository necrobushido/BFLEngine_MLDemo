#pragma once

#include "SharedMLHeader.h"

class NNNoiseFinderLossImpl : public torch::nn::Module
{
public:
	NNNoiseFinderLossImpl();
	virtual ~NNNoiseFinderLossImpl();

	torch::Tensor forward(torch::Tensor predictedNoise, torch::Tensor baseNoise, torch::Tensor timeSteps, torch::Tensor* lossOut, std::string* logOut);

protected:
	void CalcLossUsual(torch::Tensor predictedNoise, torch::Tensor baseNoise, torch::Tensor timeSteps, torch::Tensor* lossOut, std::string* logOut);
	void CalcLossNew(torch::Tensor predictedNoise, torch::Tensor baseNoise, torch::Tensor timeSteps, torch::Tensor* lossOut, std::string* logOut);

private:
	torch::Tensor	m_logVarParams;
};

TORCH_MODULE(NNNoiseFinderLoss);