#pragma once

#include "SharedMLHeader.h"

//
class NNDDPMSampler
{
public:
	NNDDPMSampler();
	~NNDDPMSampler();

	void SetInferenceTimesteps(int steps = 50);
	int GetPreviousInferenceTimestep(int timestep);

	torch::Tensor GetInferenceTimestepsElement(int timestepIdx);
	//torch::Tensor GetInferenceTimesteps(torch::Tensor timeSteps);
	torch::Tensor GetTrainingTimesteps(torch::Tensor timeSteps);
	int GetInferenceTimestepCount() const;
	int GetTrainingStepCount() const;

	torch::Tensor QSample(torch::Tensor originalSamples, torch::Tensor noise, torch::Tensor timeSteps);	//	adds noise to an un-noisy latent during training; q(x) in the paper

	torch::Tensor PredictStartFromNoise(torch::Tensor x_t, torch::Tensor t, torch::Tensor noise);
	void QPosterior(torch::Tensor x_start, torch::Tensor x_t, torch::Tensor t, torch::Tensor* pPosteriorMeanOut, torch::Tensor* pPosteriorLogVarianceOut);

	void PMeanVariance(torch::Tensor noisyInput, torch::Tensor timeStep, torch::Tensor predictedNoise, torch::Tensor* pModelMeanOut, torch::Tensor* pModelLogVarianceOut);
	torch::Tensor PSample(torch::Tensor noisyInput, torch::Tensor timeStep, torch::Tensor predictedNoise);

	torch::Tensor GetLVLBWeights(){ return m_lvlb_weights; }

private:
	int				m_numTrainingSteps;
	int				m_numInferenceSteps;
	int				m_startStep;
	torch::Tensor	m_trainingTimeSteps;
	torch::Tensor	m_inferenceTimeSteps;
	torch::Tensor	m_alphaCumulativeProduct;
	torch::Tensor	m_alphaCumulativeProductPrev;

	torch::Tensor	m_sqrt_alphas_cumprod;
	torch::Tensor	m_sqrt_one_minus_alphas_cumprod;	
	torch::Tensor	m_sqrt_recip_alphas_cumprod;
	torch::Tensor	m_sqrt_recipm1_alphas_cumprod;
	torch::Tensor	m_posterior_mean_coef1;
	torch::Tensor	m_posterior_mean_coef2;
	torch::Tensor	m_posterior_log_variance_clipped;	
	torch::Tensor	m_lvlb_weights;	
};

extern NNDDPMSampler* g_NNDDPMSampler;
