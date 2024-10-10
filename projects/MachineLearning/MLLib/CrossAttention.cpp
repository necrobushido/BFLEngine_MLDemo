#include "CrossAttention.h"

#include "types.h"

//
CrossAttentionImpl::CrossAttentionImpl(int numHeads, int numEmbedDimensions, int crossEmbedDimension, bool inputProjBias, bool outputProjBias, bool causalMask):
	m_queryMtx(torch::nn::LinearOptions(numEmbedDimensions, numEmbedDimensions).bias(inputProjBias)),
	m_keyMtx(torch::nn::LinearOptions(crossEmbedDimension, numEmbedDimensions).bias(inputProjBias)),
	m_valueMtx(torch::nn::LinearOptions(crossEmbedDimension, numEmbedDimensions).bias(inputProjBias)),
	m_proj(torch::nn::LinearOptions(numEmbedDimensions, numEmbedDimensions).bias(outputProjBias)),
	m_numEmbedDimensions(numEmbedDimensions),
	m_numHeads(numHeads)
{
	m_headEmbedSize = m_numEmbedDimensions / m_numHeads;

	register_module("m_queryMtx", m_queryMtx);
	register_module("m_keyMtx", m_keyMtx);
	register_module("m_valueMtx", m_valueMtx);
	register_module("m_proj", m_proj);
}

torch::Tensor CrossAttentionImpl::forward(torch::Tensor x, torch::Tensor y)
{
	//	inputs 
	//	x		: should be {batch, seqQ, m_numEmbedDimensions}
	//	y		: should be {batch, seqKV, crossEmbedDimension}

	torch::IntArrayRef  inputSizes = x.sizes();
	s64					inputB = inputSizes[0];
	s64					inputT = inputSizes[1];
	s64					inputC = inputSizes[2];

	torch::IntArrayRef  crossSizes = y.sizes();
	s64					crossB = crossSizes[0];
	s64					crossT = crossSizes[1];
	s64					crossC = crossSizes[2];

	torch::Tensor	q = m_queryMtx->forward(x);
	torch::Tensor	k = m_keyMtx->forward(y);
	torch::Tensor	v = m_valueMtx->forward(y);

	//	{inputB, inputT, inputC} -> {inputB, inputT, m_numHeads, m_headEmbedSize} -> {inputB, m_numHeads, inputT, m_headEmbedSize}
	q = q.view({inputB, inputT, m_numHeads, m_headEmbedSize}).transpose(1, 2);
	k = k.view({crossB, crossT, m_numHeads, m_headEmbedSize}).transpose(1, 2);
	v = v.view({crossB, crossT, m_numHeads, m_headEmbedSize}).transpose(1, 2);

	//	{inputB, m_numHeads, inputT, m_headEmbedSize} -> {inputB, m_numHeads, inputT, inputT}
	torch::Tensor	weights = q.matmul(k.transpose(-1, -2));

	weights = weights / sqrt(m_headEmbedSize);
	
	int	dim = -1;
	torch::nn::functional::SoftmaxFuncOptions	softMaxoptions(dim);
	weights = torch::nn::functional::softmax(weights, softMaxoptions);

	//	{inputB, m_numHeads, inputT, inputT} -> {inputB, m_numHeads, inputT, m_headEmbedSize}
	torch::Tensor	output = weights.matmul(v);

	//	{inputB, m_numHeads, inputT, inputT} -> {inputB, inputT, m_numHeads, m_headEmbedSize}
	output = output.transpose(1, 2).contiguous();

	//	{inputB, inputT, m_numHeads, m_headEmbedSize} -> {inputB, inputT, inputC}
	output = output.view({inputB, inputT, inputC});

	//	{inputB, inputT, inputC} -> {inputB, inputT, inputC}
	output = m_proj->forward(output);

	return output;
}

//
FlashCrossAttentionImpl::FlashCrossAttentionImpl(int numHeads, int numEmbedDimensions, int crossEmbedDimension, bool inputProjBias, bool outputProjBias, bool causalMask):
	m_queryMtx(torch::nn::LinearOptions(numEmbedDimensions, numEmbedDimensions).bias(inputProjBias)),
	m_keyMtx(torch::nn::LinearOptions(crossEmbedDimension, numEmbedDimensions).bias(inputProjBias)),
	m_valueMtx(torch::nn::LinearOptions(crossEmbedDimension, numEmbedDimensions).bias(inputProjBias)),
	m_proj(torch::nn::LinearOptions(numEmbedDimensions, numEmbedDimensions).bias(outputProjBias)),
	m_numEmbedDimensions(numEmbedDimensions),
	m_numHeads(numHeads)
{
	m_headEmbedSize = m_numEmbedDimensions / m_numHeads;

	register_module("m_queryMtx", m_queryMtx);
	register_module("m_keyMtx", m_keyMtx);
	register_module("m_valueMtx", m_valueMtx);
	register_module("m_proj", m_proj);
}

torch::Tensor FlashCrossAttentionImpl::forward(torch::Tensor x, torch::Tensor y)
{
	//	inputs 
	//	x		: should be {batch, seqQ, m_numEmbedDimensions}
	//	y		: should be {batch, seqKV, crossEmbedDimension}

	torch::IntArrayRef  inputSizes = x.sizes();
	s64					inputB = inputSizes[0];
	s64					inputT = inputSizes[1];
	s64					inputC = inputSizes[2];

	torch::IntArrayRef  crossSizes = y.sizes();
	s64					crossB = crossSizes[0];
	s64					crossT = crossSizes[1];
	s64					crossC = crossSizes[2];

	torch::Tensor	q = m_queryMtx->forward(x);
	torch::Tensor	k = m_keyMtx->forward(y);
	torch::Tensor	v = m_valueMtx->forward(y);

	//	{inputB, inputT, inputC} -> {inputB, inputT, m_numHeads, m_headEmbedSize} -> {inputB, m_numHeads, inputT, m_headEmbedSize}
	q = q.view({inputB, inputT, m_numHeads, m_headEmbedSize}).transpose(1, 2);
	k = k.view({crossB, crossT, m_numHeads, m_headEmbedSize}).transpose(1, 2);
	v = v.view({crossB, crossT, m_numHeads, m_headEmbedSize}).transpose(1, 2);

	//	{inputB, m_numHeads, inputT, m_headEmbedSize} -> {inputB, m_numHeads, inputT, inputT}
	//torch::Tensor	weights = q.matmul(k.transpose(-1, -2));

	//weights = weights / sqrt(m_headEmbedSize);
	//
	//int	dim = -1;
	//torch::nn::functional::SoftmaxFuncOptions	softMaxoptions(dim);
	//weights = torch::nn::functional::softmax(weights, softMaxoptions);

	////	{inputB, m_numHeads, inputT, inputT} -> {inputB, m_numHeads, inputT, m_headEmbedSize}
	//torch::Tensor	output = weights.matmul(v);

	torch::Tensor	output = torch::scaled_dot_product_attention(q, k, v, {}, 0.0, false);

	//	{inputB, m_numHeads, inputT, inputT} -> {inputB, inputT, m_numHeads, m_headEmbedSize}
	output = output.transpose(1, 2).contiguous();

	//	{inputB, inputT, m_numHeads, m_headEmbedSize} -> {inputB, inputT, inputC}
	output = output.view({inputB, inputT, inputC});

	//	{inputB, inputT, inputC} -> {inputB, inputT, inputC}
	output = m_proj->forward(output);

	return output;
}