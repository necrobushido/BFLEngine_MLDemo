#pragma once

#include "SharedMLHeader.h"

class NNVAELossImpl : public torch::nn::Module
{
public:
	NNVAELossImpl();
	virtual ~NNVAELossImpl();

	torch::Tensor forward(torch::Tensor inputAnimData, torch::Tensor decodedData, torch::Tensor encoderMean, torch::Tensor encoderLogVar, torch::Tensor& lossOut, std::string& logOut);

protected:
	void CalcLoss(torch::Tensor inputAnimData, torch::Tensor decodedData, torch::Tensor encoderMean, torch::Tensor encoderLogVar, torch::Tensor& lossOut, std::string& logOut);

	void CalcLossUsual(torch::Tensor inputAnimData, torch::Tensor decodedData, torch::Tensor encoderMean, torch::Tensor encoderLogVar, torch::Tensor& lossOut, std::string& logOut);
	void CalcLossSD(torch::Tensor inputAnimData, torch::Tensor decodedData, torch::Tensor encoderMean, torch::Tensor encoderLogVar, torch::Tensor& lossOut, std::string& logOut);

private:
	torch::Tensor	m_logVarParam;
};

TORCH_MODULE(NNVAELoss);