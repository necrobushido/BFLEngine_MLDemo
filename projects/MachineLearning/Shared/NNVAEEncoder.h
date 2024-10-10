#pragma once

#include "SharedMLHeader.h"

class NNVAEEncoderImpl : public torch::nn::Module 
{
public:
	NNVAEEncoderImpl(int transformVectorSize);

    torch::Tensor forward(torch::Tensor animImage, torch::Tensor* logVarOut, torch::Tensor* meanOut);

private:
	torch::nn::Sequential	m_seq;
};

TORCH_MODULE(NNVAEEncoder);
