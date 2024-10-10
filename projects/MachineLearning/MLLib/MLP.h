#pragma once

#include "types.h"

#include "SharedMLHeader.h"

class MLPImpl : public torch::nn::Module 
{
public:
    MLPImpl(int inputSize, int outputSize, int hiddenLayerSize, int hiddenLayerCount, eNonLinearity nonLinearity = kNL_Relu);

    torch::Tensor forward(torch::Tensor x);

private:
	torch::nn::Sequential	m_seq;
};

TORCH_MODULE(MLP);

