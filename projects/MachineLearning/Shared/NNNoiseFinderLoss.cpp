#include "NNNoiseFinderLoss.h"
#include "types.h"

#include "NNDDPMSampler.h"

NNNoiseFinderLossImpl::NNNoiseFinderLossImpl()
{
	const f32	kLogVarInit = 0.0f;
	const int	kNumTimesteps = g_NNDDPMSampler->GetTrainingStepCount();
	m_logVarParams = register_parameter("m_logVarParams", torch::full({kNumTimesteps}, kLogVarInit));
}

NNNoiseFinderLossImpl::~NNNoiseFinderLossImpl()
{
}

torch::Tensor NNNoiseFinderLossImpl::forward(torch::Tensor predictedNoise, torch::Tensor baseNoise, torch::Tensor timeSteps, torch::Tensor* lossOut, std::string* logOut)
{
	CalcLossUsual(predictedNoise, baseNoise, timeSteps, lossOut, logOut);
	//CalcLossNew(predictedNoise, baseNoise, timeSteps, lossOut, logOut);

	return *lossOut;
}

void NNNoiseFinderLossImpl::CalcLossUsual(torch::Tensor predictedNoise, torch::Tensor baseNoise, torch::Tensor timeSteps, torch::Tensor* lossOut, std::string* logOut)
{
	//	calc loss between predicted and actual noise
	//	this is the final step in equation 14

	//	total absolute error
	//torch::Tensor	taeLoss = torch::abs(predictedNoise - baseNoise);
	//taeLoss = torch::sum(taeLoss) / taeLoss.size(0);

	//	mean absolute error
	//torch::Tensor	maeLoss = torch::abs(predictedNoise - baseNoise);
	//maeLoss = maeLoss.mean();

	//	mean squared error
	//torch::nn::functional::MSELossFuncOptions	options;
	//options.reduction() = torch::kNone;
	//torch::Tensor	mseLoss = torch::nn::functional::mse_loss(predictedNoise, baseNoise, options);
	torch::Tensor	mseLoss = torch::nn::functional::mse_loss(predictedNoise, baseNoise);

	//	the is the equivalent of just calling mean right?  or even doing reduction?
	//		this is the simplified version of what SD was doing on this config
	//mseLoss = mseLoss.mean({1, 2, 3});
	//mseLoss = mseLoss.mean();

	//	total squared error
	torch::Tensor	tseLoss = predictedNoise - baseNoise;
	tseLoss = tseLoss * tseLoss;
	tseLoss = torch::sum(tseLoss) / tseLoss.size(0);

	//	total loss
	*lossOut = mseLoss;
	//*lossOut = tseLoss;
	//*lossOut = mseLoss + tseLoss;	//	should we have some coefficient?

	//	log
	{
		char	tempBuffer[256];
		sprintf(tempBuffer, "mseLoss loss = %.9g, tseLoss loss = %.9g", mseLoss.item().toFloat(), tseLoss.item().toFloat());

		*logOut = tempBuffer;
	}
}

void NNNoiseFinderLossImpl::CalcLossNew(torch::Tensor predictedNoise, torch::Tensor baseNoise, torch::Tensor timeSteps, torch::Tensor* lossOut, std::string* logOut)
{
	torch::nn::functional::MSELossFuncOptions	options;
	options.reduction() = torch::kNone;
	torch::Tensor	mseLoss = torch::nn::functional::mse_loss(predictedNoise, baseNoise, options);
	mseLoss = mseLoss.mean({1, 2, 3});

	torch::Tensor	logvar_t = m_logVarParams.index({timeSteps});

	*lossOut = mseLoss / torch::exp(logvar_t) + logvar_t;

	const f32	kSimpleWeight = 1.0f;
	*lossOut = kSimpleWeight * lossOut->mean();

	/*
	const double	kOriginal_ELBO_Weight = 0.0;	//	this needs to be non-zero for this term to matter
	torch::Tensor	lvlbWeights = g_NNDDPMSampler2->GetLVLBWeights();
	torch::Tensor	loss_vlb = (lvlbWeights.index({timeSteps}) * mseLoss).mean();	
	lossOut += (kOriginal_ELBO_Weight * loss_vlb);
	*/

	//	total squared error
	torch::Tensor	tseLoss = predictedNoise - baseNoise;
	tseLoss = tseLoss * tseLoss;
	tseLoss = torch::sum(tseLoss) / tseLoss.size(0);

	//	log
	{
		char	tempBuffer[256];
		sprintf(tempBuffer, "mseLoss loss = %.9g, tseLoss loss = %.9g", mseLoss.mean().item().toFloat(), tseLoss.item().toFloat());

		*logOut = tempBuffer;
	}
}