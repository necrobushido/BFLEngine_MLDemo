#include "NNDDPMModelComponent.h"

#include "NNDDPMSampler.h"

//	 Denoising Diffusion Probabilistic Models

NNDDPMModelComponent::NNDDPMModelComponent():
	m_pNNDDPMSampler(nullptr)
{
	m_pNNDDPMSampler = new NNDDPMSampler();
}

NNDDPMModelComponent::~NNDDPMModelComponent()
{
	delete m_pNNDDPMSampler;
}