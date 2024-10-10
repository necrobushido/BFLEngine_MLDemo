#pragma once

#include "types.h"
#include "SharedMLHeader.h"

class MultiLinearLayerImpl : public torch::nn::Module
{
public:
	MultiLinearLayerImpl(int numLinearLayers, int xDim, int yDim);
	virtual ~MultiLinearLayerImpl();
	
	torch::Tensor forward(torch::Tensor input, torch::Tensor layerIndices);
	torch::Tensor GetLayerWeights(int layerIdx);
	torch::Tensor GetLayerBiases(int layerIdx);

public:
	torch::Tensor	m_weights;
	torch::Tensor	m_biases;
	int				m_xDim;
	int				m_yDim;
};

TORCH_MODULE(MultiLinearLayer);