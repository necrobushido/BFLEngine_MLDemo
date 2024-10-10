#pragma once

#include "SharedMLHeader.h"
#include "NNNoiseFinderLoss.h"

//
class NNNoiseFinderImpl : public torch::nn::Module 
{
public:
	NNNoiseFinderImpl();
	virtual ~NNNoiseFinderImpl();

    torch::Tensor forward(torch::Tensor x, torch::Tensor context, torch::Tensor time);

	void CalcLoss(torch::Tensor predictedNoise, torch::Tensor baseNoise, torch::Tensor timeSteps, torch::Tensor* lossOut, std::string* logOut);

private:
	torch::nn::Sequential	m_seq;
	torch::nn::Sequential	m_timeInputSeq;

	NNNoiseFinderLoss		m_loss;
};

TORCH_MODULE(NNNoiseFinder);

extern NNNoiseFinderImpl* g_NNNoiseFinderImpl;