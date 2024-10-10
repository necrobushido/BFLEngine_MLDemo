#include "SelfAttention.h"

#include "types.h"

//
SelfAttentionImpl::SelfAttentionImpl(int numHeads, int numEmbedDimensions, bool inputProjBias, bool outputProjBias, bool causalMask, double weightsDropout):
	m_attentionComponentWeights(torch::nn::LinearOptions(numEmbedDimensions, 3 * numEmbedDimensions).bias(inputProjBias)),
	m_proj(torch::nn::LinearOptions(numEmbedDimensions, numEmbedDimensions).bias(outputProjBias)),
	m_weightsDropout(register_module("weightsDropout", torch::nn::Dropout(torch::nn::DropoutOptions(weightsDropout)))),
	m_numEmbedDimensions(numEmbedDimensions),
	m_numHeads(numHeads),
	m_causalMask(causalMask)
{
	register_module("m_attentionComponentWeights", m_attentionComponentWeights);
	register_module("m_proj", m_proj);

	m_headEmbedSize = m_numEmbedDimensions / m_numHeads;
}

torch::Tensor SelfAttentionImpl::forward(torch::Tensor x)
{
	//	inputs 
	//	x		: should be {batch, seq, m_numEmbedDimensions}

	torch::IntArrayRef  inputSizes = x.sizes();
	s64					inputB = inputSizes[0];
	s64					inputT = inputSizes[1];
	s64					inputC = inputSizes[2];

	enum
	{
		kQuery,
		kKey,			
		kValue,
		kNumSplits
	};
	std::vector<torch::Tensor>	attentionComponents = m_attentionComponentWeights->forward(x).split(m_numEmbedDimensions, 2);
	assert(attentionComponents.size() == kNumSplits);

	attentionComponents[kQuery] = attentionComponents[kQuery].view({inputB, inputT, m_numHeads, m_headEmbedSize}).transpose(1, 2);
	attentionComponents[kKey] = attentionComponents[kKey].view({inputB, inputT, m_numHeads, m_headEmbedSize}).transpose(1, 2);
	attentionComponents[kValue] = attentionComponents[kValue].view({inputB, inputT, m_numHeads, m_headEmbedSize}).transpose(1, 2);

	torch::Tensor	weights = attentionComponents[kQuery].matmul(attentionComponents[kKey].transpose(-1, -2));

	if( m_causalMask )
	{
		torch::Tensor	mask = torch::ones_like(weights, torch::kBool).triu(1);
		weights.masked_fill(mask, std::numeric_limits<float>::lowest());
	}

	weights = weights / sqrt(m_headEmbedSize);
	
	int	dim = -1;
	torch::nn::functional::SoftmaxFuncOptions	softMaxoptions(dim);
	weights = torch::nn::functional::softmax(weights, softMaxoptions);
	//weights = m_weightsDropout->forward(weights);	//	I think this is wrong

	torch::Tensor	output = weights.matmul(attentionComponents[kValue]);

	output = output.transpose(1, 2);
	output = output.contiguous();

	output = output.view({inputB, inputT, inputC});

	output = m_proj->forward(output);

	return output;
}

//
FlashSelfAttentionImpl::FlashSelfAttentionImpl(int numHeads, int numEmbedDimensions, bool inputProjBias, bool outputProjBias, bool causalMask, double weightsDropout):
	m_attentionComponentWeights(torch::nn::LinearOptions(numEmbedDimensions, 3 * numEmbedDimensions).bias(inputProjBias)),
	m_proj(torch::nn::LinearOptions(numEmbedDimensions, numEmbedDimensions).bias(outputProjBias)),
	m_weightsDropout(weightsDropout),
	m_numEmbedDimensions(numEmbedDimensions),
	m_numHeads(numHeads),
	m_causalMask(causalMask)
{
	register_module("m_attentionComponentWeights", m_attentionComponentWeights);
	register_module("m_proj", m_proj);

	m_headEmbedSize = m_numEmbedDimensions / m_numHeads;
}

//torch::Tensor CreateCausalMask(s64 batchSize, s64 numHeads, s64 seqLength) 
//{
//    torch::Tensor	mask = torch::ones({seqLength, seqLength}, torch::kBool).triu(1);
//    
//	mask = mask.to(torch::kFloat32).masked_fill(mask, -std::numeric_limits<float>::infinity());
//    
//	mask = mask.unsqueeze(0).unsqueeze(0);
//    mask = mask.expand({batchSize, numHeads, seqLength, seqLength});
//
//    return mask;
//}

torch::Tensor CreateCausalMask(s64 batchSize, s64 numHeads, s64 seqLength) 
{
	//torch::Tensor	mask = torch::ones({seqLength, seqLength}, torch::kBool).triu(1).logical_not();
	torch::Tensor	mask = torch::ones({seqLength, seqLength}, torch::kBool).tril(0);
    
	mask = mask.unsqueeze(0).unsqueeze(0);
	mask = mask.expand({batchSize, numHeads, seqLength, seqLength});

	return mask;
}

torch::Tensor FlashSelfAttentionImpl::forward(torch::Tensor x)
{
	//	inputs 
	//	x		: should be {batch, seq, m_numEmbedDimensions}

	torch::IntArrayRef  inputSizes = x.sizes();
	s64					inputB = inputSizes[0];
	s64					inputT = inputSizes[1];
	s64					inputC = inputSizes[2];

	enum
	{
		kQuery,
		kKey,			
		kValue,
		kNumSplits
	};
	std::vector<torch::Tensor>	attentionComponents = m_attentionComponentWeights->forward(x).split(m_numEmbedDimensions, 2);
	assert(attentionComponents.size() == kNumSplits);

	attentionComponents[kQuery] = attentionComponents[kQuery].view({inputB, inputT, m_numHeads, m_headEmbedSize}).transpose(1, 2);
	attentionComponents[kKey] = attentionComponents[kKey].view({inputB, inputT, m_numHeads, m_headEmbedSize}).transpose(1, 2);
	attentionComponents[kValue] = attentionComponents[kValue].view({inputB, inputT, m_numHeads, m_headEmbedSize}).transpose(1, 2);

	/*torch::Tensor	weights = attentionComponents[kQuery].matmul(attentionComponents[kKey].transpose(-1, -2));

	if( m_causalMask )
	{
		torch::Tensor	mask = torch::ones_like(weights, torch::kBool).triu(1);
		weights.masked_fill(mask, std::numeric_limits<float>::lowest());
	}

	weights = weights / sqrt(m_headEmbedSize);
	
	int	dim = -1;
	torch::nn::functional::SoftmaxFuncOptions	softMaxoptions(dim);
	weights = torch::nn::functional::softmax(weights, softMaxoptions);
	weights = m_weightsDropout->forward(weights);

	torch::Tensor	output = weights.matmul(attentionComponents[kValue]);*/

	//	none of this crap seems to work.  tried a new LibTorch version but it had problem linking due to too many objects or something.
	//torch::Tensor	output = torch::scaled_dot_product_attention(attentionComponents[kQuery], attentionComponents[kKey], attentionComponents[kValue], {}, m_weightsDropout, m_causalMask);

	torch::Tensor	mask = {};
	/*if( m_causalMask )
	{
		mask = CreateCausalMask(inputB, m_numHeads, inputT).to(x.device());
	}*/
	torch::Tensor	output = torch::scaled_dot_product_attention(attentionComponents[kQuery], attentionComponents[kKey], attentionComponents[kValue], mask, m_weightsDropout, m_causalMask);
	//torch::Tensor	output = torch::scaled_dot_product_attention(attentionComponents[kQuery], attentionComponents[kKey], attentionComponents[kValue], mask, m_weightsDropout);

	//auto			out_lse_softmax = torch::_scaled_dot_product_flash_attention(attentionComponents[kQuery], attentionComponents[kKey], attentionComponents[kValue], m_weightsDropout, m_causalMask);
	//auto			out_lse_softmax = torch::_scaled_dot_product_efficient_attention(attentionComponents[kQuery], attentionComponents[kKey], attentionComponents[kValue], {}, attentionComponents[kQuery].requires_grad(), m_weightsDropout, m_causalMask);
	//auto			out_lse_softmax = torch::_scaled_dot_product_attention_math(attentionComponents[kQuery], attentionComponents[kKey], attentionComponents[kValue], {}, m_weightsDropout, m_causalMask);	
	//torch::Tensor	output = std::get<0>(out_lse_softmax);

	//int64_t			sdpChoice = torch::_fused_sdp_choice(attentionComponents[kQuery], attentionComponents[kKey], attentionComponents[kValue], {}, m_weightsDropout, m_causalMask);
	//Assert(sdpChoice == 1);

	output = output.transpose(1, 2);
	output = output.contiguous();

	output = output.view({inputB, inputT, inputC});

	output = m_proj->forward(output);

	return output;
}