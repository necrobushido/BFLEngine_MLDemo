#include "NNVAE.h"
#include "types.h"

NNVAEImpl*	g_NNVAE = nullptr;

NNVAEImpl::NNVAEImpl(int transformVectorSize):
	m_dataEncoder(register_module("m_dataEncoder", NNVAEEncoder(transformVectorSize))),
	m_dataDecoder(register_module("m_dataDecoder", NNVAEDecoder(transformVectorSize))),
	m_loss(register_module("m_loss", NNVAELoss()))
{
	Assert(g_NNVAE == nullptr);
	g_NNVAE = this;
}

NNVAEImpl::~NNVAEImpl()
{
	Assert(g_NNVAE == this);
	g_NNVAE = nullptr;
}

torch::Tensor NNVAEImpl::EncoderForward(torch::Tensor animImage, torch::Tensor* logVarOut, torch::Tensor* meanOut)
{
	torch::Tensor	x = animImage;

	x = m_dataEncoder->forward(x, logVarOut, meanOut);

	return x;
}

torch::Tensor NNVAEImpl::DecoderForward(torch::Tensor latentAnimDesc)
{
	torch::Tensor	x = m_dataDecoder->forward(latentAnimDesc);

	return x;
}

torch::Tensor NNVAEImpl::EvalEncoderForward(torch::Tensor animImage, torch::Tensor* logVarOut, torch::Tensor* meanOut)
{
	torch::NoGradGuard	no_grad;

	torch::Tensor	x = animImage;

	x = m_dataEncoder->forward(x, logVarOut, meanOut);

	return x;
}

torch::Tensor NNVAEImpl::EvalDecoderForward(torch::Tensor latentAnimDesc)
{
	torch::NoGradGuard	no_grad;

	torch::Tensor	x = m_dataDecoder->forward(latentAnimDesc);

	return x;
}

void NNVAEImpl::CalcLoss(torch::Tensor inputAnimData, torch::Tensor decodedData, torch::Tensor encoderMean, torch::Tensor encoderLogVar, torch::Tensor& lossOut, std::string& logOut)
{
	m_loss->forward(inputAnimData, decodedData, encoderMean, encoderLogVar, lossOut, logOut);
}