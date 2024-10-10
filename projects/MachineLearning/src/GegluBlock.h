#pragma once

#include "SharedMLHeader.h"

class GegluBlockImpl : public torch::nn::Module
{
public:
	GegluBlockImpl(int inputSize):
		m_linear1(register_module("m_linear1", torch::nn::Linear(inputSize, inputSize * 4 * 2))),
		m_linear2(register_module("m_linear2", torch::nn::Linear(inputSize * 4, inputSize)))
	{
	}

	torch::Tensor forward(torch::Tensor x)
	{
		enum
		{
			kValue,
			kGate,
			kNumSplits
		};
		std::vector<torch::Tensor>	gegluComponents = m_linear1->forward(x).chunk(kNumSplits, -1);
		x = gegluComponents[kValue] * torch::nn::functional::gelu(gegluComponents[kGate]);
		x = m_linear2->forward(x);

		return x;
	}

private:
	torch::nn::Linear		m_linear1;
	torch::nn::Linear		m_linear2;
};

TORCH_MODULE(GegluBlock);

//	a debug test replacement
//class GegluBlockImpl : public torch::nn::Module
//{
//public:
//	GegluBlockImpl(int inputSize):
//		m_linear1(register_module("m_linear1", torch::nn::Linear(inputSize, inputSize)))
//	{
//	}
//
//    torch::Tensor forward(torch::Tensor x)
//	{
//		x = torch::nn::functional::silu(x);
//		x = m_linear1->forward(x);
//
//		return x;
//	}
//
//private:
//	torch::nn::Linear		m_linear1;
//};
//
//TORCH_MODULE(GegluBlock);