#pragma once

#include "SharedMLHeader.h"

#include "NNVAEEncoder.h"
#include "NNVAEDecoder.h"
#include "NNVAELoss.h"

class NNVAEImpl : public torch::nn::Module
{
public:
	NNVAEImpl(int transformVectorSize);
	virtual ~NNVAEImpl();

	torch::Tensor EncoderForward(torch::Tensor animImage, torch::Tensor* logVarOut, torch::Tensor* meanOut);
	torch::Tensor DecoderForward(torch::Tensor latentAnimDesc);

	torch::Tensor EvalEncoderForward(torch::Tensor animImage, torch::Tensor* logVarOut, torch::Tensor* meanOut);
	torch::Tensor EvalDecoderForward(torch::Tensor latentAnimDesc);

	void CalcLoss(torch::Tensor inputAnimData, torch::Tensor decodedData, torch::Tensor encoderMean, torch::Tensor encoderLogVar, torch::Tensor& lossOut, std::string& logOut);

protected:
	NNVAEEncoder		m_dataEncoder;
	NNVAEDecoder		m_dataDecoder;
	NNVAELoss			m_loss;
};

TORCH_MODULE(NNVAE);

extern NNVAEImpl* g_NNVAE;
