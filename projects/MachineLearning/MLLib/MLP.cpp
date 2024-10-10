#include "MLP.h"

class MLPBlockImpl : public torch::nn::Module 
{
public:
    MLPBlockImpl(int hiddenLayerSize, eNonLinearity nonLinearity):
		m_net(register_module("m_net", torch::nn::Sequential()))
	{
		m_net->push_back(register_module("layerNorm", torch::nn::LayerNorm(torch::nn::LayerNormOptions({hiddenLayerSize}))));
		AddNonLinearityModule(m_net, nonLinearity, "NL");
		m_net->push_back(register_module("linear", torch::nn::Linear(hiddenLayerSize, hiddenLayerSize)));
	}

    torch::Tensor forward(torch::Tensor x)
	{
		return m_net->forward(x);
	}

private:
	torch::nn::Sequential	m_net;
};

TORCH_MODULE(MLPBlock);

//
MLPImpl::MLPImpl(int inputSize, int outputSize, int hiddenLayerSize, int hiddenLayerCount, eNonLinearity nonLinearity):
	m_seq(register_module("m_seq", torch::nn::Sequential()))
{
	m_seq->push_back(register_module("linearInput", torch::nn::Linear(inputSize, hiddenLayerSize)));

	for(int i = 0; i < hiddenLayerCount; ++i)
	{
		char	nameBuffer[256];
		sprintf(nameBuffer, "MLPBlock%d", i);
		m_seq->push_back(register_module(nameBuffer, MLPBlock(hiddenLayerSize, nonLinearity)));
	}

	m_seq->push_back(register_module("linearOutput", torch::nn::Linear(hiddenLayerSize, outputSize)));
}

torch::Tensor MLPImpl::forward(torch::Tensor x)
{
	return m_seq->forward(x);
}