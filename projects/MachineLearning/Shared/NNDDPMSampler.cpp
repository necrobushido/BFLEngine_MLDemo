#include "NNDDPMSampler.h"

#include "types.h"

namespace
{
	//	I guess this is "number of steps required to transform from original state to fully noised"?
	//const int	kTrainingSteps = 1024;	//	this works but takes forever and there may be overfitting
	const int	kTrainingSteps = 512;
	//const int	kTrainingSteps = 256;	//	was this too low?  I must have tried it but forgot.
	//const int	kTrainingSteps = 64;	//	too low to work

	//	these were the example values in the video
	//const float	kBetaStart = 0.00085f;
	//const float	kBetaEnd = 0.0120f;

	//	these were the example values in the paper
	//const float	kBetaStart = 0.0001f;
	//const float	kBetaEnd = 0.02f;

	//
	const float	kBetaStart = 0.00085f;
	const float	kBetaEnd = 0.0120f;

	torch::Tensor MakeBroadcastable(torch::Tensor source, torch::Tensor target)
	{
		while(source.sizes().size() < target.sizes().size())
		{
			source = source.unsqueeze(-1);
		}

		return source;
	}
}

//
NNDDPMSampler* g_NNDDPMSampler = nullptr;

torch::Tensor ExtractIntoTensor(torch::Tensor a, torch::Tensor t, torch::Tensor x)
{
	a = a.to(x.device());
	t = t.to(x.device());

	torch::Tensor	result = a.gather(-1, t);

	return MakeBroadcastable(result, x);
}

NNDDPMSampler::NNDDPMSampler():
	m_numTrainingSteps(kTrainingSteps),
	m_numInferenceSteps(50),
	m_startStep(0)
{
	Assert(g_NNDDPMSampler == nullptr);
	g_NNDDPMSampler = this;

	//	betas are Variances?
	//		the paper says that they can be held constant as hyperparameters, so I guess that's what's going on
	//		I have no idea why we would choose one set of parameters over another though
	torch::Tensor	betas = torch::square(torch::linspace(sqrt(kBetaStart), sqrt(kBetaEnd), m_numTrainingSteps, torch::kF32));	//	this line is make_beta_schedule in SD in the "linear" config
	torch::Tensor	alphas = 1.0f - betas;
	m_alphaCumulativeProduct = torch::cumprod(alphas, 0);
	m_alphaCumulativeProductPrev = torch::cat({torch::tensor({1.0f}), m_alphaCumulativeProduct.slice(0, 0, -1)}, 0);	//	the elements of m_alphaCumulativeProduct shifted right by 1, with 1.0 added in the front

	m_trainingTimeSteps = torch::arange(0, m_numTrainingSteps);

	m_sqrt_alphas_cumprod = torch::sqrt(m_alphaCumulativeProduct);
	m_sqrt_one_minus_alphas_cumprod = torch::sqrt(1.0 - m_alphaCumulativeProduct);

	m_sqrt_recip_alphas_cumprod = torch::sqrt(1.0 / m_alphaCumulativeProduct);
	m_sqrt_recipm1_alphas_cumprod = torch::sqrt(1.0 / m_alphaCumulativeProduct - 1);

	double			v_posterior = 0.0f;
	torch::Tensor	posterior_variance = (1.0 - v_posterior) * betas * (1.0 - m_alphaCumulativeProductPrev) / (1.0 - m_alphaCumulativeProduct) + v_posterior * betas;
	m_posterior_log_variance_clipped = torch::log(torch::maximum(posterior_variance, torch::tensor({1e-20})));

	m_posterior_mean_coef1 = betas * torch::sqrt(m_alphaCumulativeProductPrev) / (1.0 - m_alphaCumulativeProduct);
	m_posterior_mean_coef2 = (1.0 - m_alphaCumulativeProductPrev) * torch::sqrt(alphas) / (1.0 - m_alphaCumulativeProduct);

	m_lvlb_weights = torch::square(betas) / (2 * posterior_variance * alphas * (1 - m_alphaCumulativeProduct));
	m_lvlb_weights[0] = m_lvlb_weights[1];	//	?? SD did this but I dunno why.  not sure they did either from the comment.

	//
	/* self.register_buffer('betas', to_torch(betas))
        self.register_buffer('alphas_cumprod', to_torch(alphas_cumprod))
        self.register_buffer('alphas_cumprod_prev', to_torch(alphas_cumprod_prev))

        # calculations for diffusion q(x_t | x_{t-1}) and others
        self.register_buffer('sqrt_alphas_cumprod', to_torch(np.sqrt(alphas_cumprod)))
        self.register_buffer('sqrt_one_minus_alphas_cumprod', to_torch(np.sqrt(1. - alphas_cumprod)))
        self.register_buffer('log_one_minus_alphas_cumprod', to_torch(np.log(1. - alphas_cumprod)))
        self.register_buffer('sqrt_recip_alphas_cumprod', to_torch(np.sqrt(1. / alphas_cumprod)))
        self.register_buffer('sqrt_recipm1_alphas_cumprod', to_torch(np.sqrt(1. / alphas_cumprod - 1)))

        # calculations for posterior q(x_{t-1} | x_t, x_0)
        posterior_variance = (1 - self.v_posterior) * betas * (1. - alphas_cumprod_prev) / (
                    1. - alphas_cumprod) + self.v_posterior * betas
        # above: equal to 1. / (1. / (1. - alpha_cumprod_tm1) + alpha_t / beta_t)
        self.register_buffer('posterior_variance', to_torch(posterior_variance))
        # below: log calculation clipped because the posterior variance is 0 at the beginning of the diffusion chain
        self.register_buffer('posterior_log_variance_clipped', to_torch(np.log(np.maximum(posterior_variance, 1e-20))))
        self.register_buffer('posterior_mean_coef1', to_torch(
            betas * np.sqrt(alphas_cumprod_prev) / (1. - alphas_cumprod)))
        self.register_buffer('posterior_mean_coef2', to_torch(
            (1. - alphas_cumprod_prev) * np.sqrt(alphas) / (1. - alphas_cumprod)))*/

	//	default inference steps to the same as training
	SetInferenceTimesteps(m_numTrainingSteps);	
}

NNDDPMSampler::~NNDDPMSampler()
{
	Assert(g_NNDDPMSampler == this);
	g_NNDDPMSampler = nullptr;
}

void NNDDPMSampler::SetInferenceTimesteps(int steps)
{
	m_numInferenceSteps = steps;

	int	stepRatio = m_numTrainingSteps / m_numInferenceSteps;

	m_inferenceTimeSteps = (torch::arange(0, m_numInferenceSteps) * stepRatio).flip(-1);
}

int NNDDPMSampler::GetPreviousInferenceTimestep(int timestep)
{
	int	stepRatio = m_numTrainingSteps / m_numInferenceSteps;

	return timestep - stepRatio;
}

torch::Tensor NNDDPMSampler::GetInferenceTimestepsElement(int timestepIdx)
{
	//return m_inferenceTimeSteps[timestepIdx];

	return m_inferenceTimeSteps.slice(0, timestepIdx, timestepIdx+1);
}
//
//torch::Tensor NNDDPMSampler::GetInferenceTimesteps(torch::Tensor timeSteps)
//{
//	return m_inferenceTimeSteps.index({timeSteps});
//}

torch::Tensor NNDDPMSampler::GetTrainingTimesteps(torch::Tensor timeSteps)
{
	return m_trainingTimeSteps.index({timeSteps});
}

int NNDDPMSampler::GetInferenceTimestepCount() const
{
	return (int)m_inferenceTimeSteps.size(0);
}

int NNDDPMSampler::GetTrainingStepCount() const
{
	return m_numTrainingSteps;
}

torch::Tensor NNDDPMSampler::QSample(torch::Tensor originalSamples, torch::Tensor noise, torch::Tensor timeSteps)
{
	torch::Tensor	e1 = ExtractIntoTensor(m_sqrt_alphas_cumprod, timeSteps, originalSamples).to(originalSamples.device());
	torch::Tensor	e2 = ExtractIntoTensor(m_sqrt_one_minus_alphas_cumprod, timeSteps, originalSamples).to(originalSamples.device());
	return e1 * originalSamples + e2 * noise;
}

torch::Tensor NNDDPMSampler::PredictStartFromNoise(torch::Tensor x_t, torch::Tensor t, torch::Tensor noise)
{
	return ExtractIntoTensor(m_sqrt_recip_alphas_cumprod, t, x_t) * x_t - ExtractIntoTensor(m_sqrt_recipm1_alphas_cumprod, t, x_t) * noise;
}

void NNDDPMSampler::QPosterior(torch::Tensor x_start, torch::Tensor x_t, torch::Tensor t, torch::Tensor* pPosteriorMeanOut, torch::Tensor* pPosteriorLogVarianceOut)
{
	*pPosteriorMeanOut = ExtractIntoTensor(m_posterior_mean_coef1, t, x_t) * x_start + ExtractIntoTensor(m_posterior_mean_coef2, t, x_t) * x_t;

	*pPosteriorLogVarianceOut = ExtractIntoTensor(m_posterior_log_variance_clipped, t, x_t);
}

void NNDDPMSampler::PMeanVariance(torch::Tensor noisyInput, torch::Tensor timeStep, torch::Tensor predictedNoise, torch::Tensor* pModelMeanOut, torch::Tensor* pModelLogVarianceOut)
{
	torch::Tensor	x_recon = PredictStartFromNoise(noisyInput, timeStep, predictedNoise);

	QPosterior(x_recon, noisyInput, timeStep, pModelMeanOut, pModelLogVarianceOut);
}

torch::Tensor NNDDPMSampler::PSample(torch::Tensor noisyInput, torch::Tensor timeStep, torch::Tensor predictedNoise)
{
	torch::NoGradGuard	no_grad;

	torch::Tensor		model_mean;
	torch::Tensor		model_log_variance;
	PMeanVariance(noisyInput, timeStep, predictedNoise, &model_mean, &model_log_variance);

	torch::Tensor		noise = torch::randn(noisyInput.sizes(), noisyInput.dtype()).to(noisyInput.device());

	torch::Tensor		nonzero_mask = (1 - (timeStep == 0).to(torch::kF32));
	nonzero_mask = MakeBroadcastable(nonzero_mask, model_mean);

	return model_mean + nonzero_mask * (0.5f * model_log_variance).exp() * noise;
}