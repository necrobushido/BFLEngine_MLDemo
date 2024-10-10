#include "MultiLinearLayer.h"

MultiLinearLayerImpl::MultiLinearLayerImpl(int numLinearLayers, int xDim, int yDim):
	m_xDim(xDim),
	m_yDim(yDim)
{
	m_weights = register_parameter("m_weights", torch::ones({numLinearLayers, m_xDim, m_yDim}, torch::requires_grad()));
	m_biases = register_parameter("m_biases", torch::zeros({numLinearLayers, m_yDim}, torch::requires_grad()));
}

MultiLinearLayerImpl::~MultiLinearLayerImpl()
{
}

torch::Tensor MultiLinearLayerImpl::forward(torch::Tensor input, torch::Tensor layerIndices)
{
	//	input = {319, 1}
	//	layerIndices = {319, 1}
	//	m_weights = {10, 1, 804}
	//	m_biases = {10, 804}

	torch::Tensor	layerWeights = m_weights.index({layerIndices}).squeeze(1);	//	want {319, 1, 804}, but gives {319, 1, 1, 804}
	torch::Tensor	layerBiases = m_biases.index({layerIndices}).squeeze(1);	//	want {319, 804}, but

	//layerWeights = layerWeights.view({layerIndices.size(0), m_weights.size(1), m_weights.size(2)});
	//layerBiases = layerBiases.view({layerIndices.size(0), m_biases.size(1)});

	//return input.matmul(layerWeights) + layerBiases;

	torch::Tensor	layerWeightProduct = input.matmul(layerWeights);
	//torch::Tensor	layerWeightProduct = input.view({-1, m_xDim}).matmul(layerWeights);	//	ChatGPT claims that matmul uses the last 2 dimensions to determine how matmul works, so we need to reshape input
	//layerWeightProduct = layerWeightProduct.squeeze(1);	//	and then squeeze out the extra dimension

	torch::Tensor	biasUnsqueezed = layerBiases.unsqueeze(-2);
	torch::Tensor	layerFinal = layerWeightProduct + biasUnsqueezed;
	//torch::Tensor	layerFinal = layerWeightProduct + layerBiases.unsqueeze(-2);
	//torch::Tensor	layerFinal = torch::add(layerWeightProduct, layerBiases);	//	same thing?

	return layerFinal;
}

torch::Tensor MultiLinearLayerImpl::GetLayerWeights(int layerIdx)
{
	Assert(layerIdx >= 0 && layerIdx < m_weights.size(0));
	return m_weights[layerIdx];
}

torch::Tensor MultiLinearLayerImpl::GetLayerBiases(int layerIdx)
{
	Assert(layerIdx >= 0 && layerIdx < m_biases.size(0));
	return m_biases[layerIdx];
}