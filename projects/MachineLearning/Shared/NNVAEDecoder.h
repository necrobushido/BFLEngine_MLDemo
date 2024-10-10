#pragma once

#include "SharedMLHeader.h"

class NNVAEDecoderImpl : public torch::nn::Module 
{
public:
	NNVAEDecoderImpl(int transformVectorSize);

    torch::Tensor forward(torch::Tensor latentAnimDesc);

private:
	torch::nn::Sequential	m_seq;
	int						m_transformVectorSize;
};

TORCH_MODULE(NNVAEDecoder);
