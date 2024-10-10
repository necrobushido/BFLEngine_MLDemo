
#include "SharedMLHeader.h"

#include "types.h"

// defined here, declared in SharedHeader.h
void PrintTensorShape(torch::Tensor inputTensor, const char* label)
{
	c10::IntArrayRef	inputTensorSizes = inputTensor.sizes();
	DebugPrintf("%s shape = ", label);
	for(int i = 0; i < inputTensorSizes.size(); ++i)
	{
		int	thisSize = (int)inputTensorSizes[i];
		DebugPrintf("%d ", thisSize);
	}

	caffe2::TypeMeta	dTypeMeta = inputTensor.dtype();
	Assert(dTypeMeta.isScalarType());
	torch::ScalarType	dataType = dTypeMeta.toScalarType();
	std::string			dataTypeStr = torch::toString(dataType);

	DebugPrintf(" of type %s\n", dataTypeStr.c_str());

	c10::Device	tensorDevice = inputTensor.device();
	std::string	deviceStr = torch::toString(tensorDevice);
	DebugPrintf(" on device %s\n", deviceStr.c_str());
}

void DebugPrintTensorBasic(torch::Tensor inputTensor, const char* label)
{
	DebugPrintf("Printing tensor %s :\n", label);

	for(int i = 0; i < inputTensor.size(0); ++i)
	{
		std::stringstream	ss;
		ss << inputTensor[i];
		DebugPrintf("%d :\n %s\n", i, ss.str().c_str());
	}
}

void DebugPrintTensorRecurse(torch::Tensor inputTensor, int depth)
{
	c10::ArrayRef	sizes = inputTensor.sizes();
	int	numSizes = (int)sizes.size();

	if( numSizes > 1 )
	{
		for(int i = 0; i < inputTensor.size(0); ++i)
		{
			for(int tabIdx = 0; tabIdx < depth; ++tabIdx)
			{
				DebugPrintf("\t");
			}
			DebugPrintf("%d :\n", i);
			DebugPrintTensorRecurse(inputTensor[i], depth+1);
		}
	}
	else
	{
		for(int tabIdx = 0; tabIdx < depth; ++tabIdx)
		{
			DebugPrintf("\t");
		}

		c10::ScalarType		dataType = inputTensor.scalar_type();
		//std::stringstream	ss;
		for(int i = 0; i < inputTensor.size(0); ++i)
		{
			/*switch(dataType)
			{
			case c10::kFloat:
				{
					resultStr << inputTensor[i].item().toFloat();
				}
				break;

			case c10::kInt:
				break;
			}*/

			std::stringstream	ss;
			ss << inputTensor[i].item();
			ss << " ";
			DebugPrintf("%s", ss.str().c_str());
		}

		//DebugPrintf("%s\n", ss.str().c_str());
		DebugPrintf("\n");

		/*std::stringstream	ss;
		ss << inputTensor;
		DebugPrintf("%s\n", ss.str().c_str());*/
	}
}

void DebugPrintTensor(torch::Tensor inputTensor, const char* label)
{
	DebugPrintf("Printing tensor %s :\n", label);

	DebugPrintTensorRecurse(inputTensor, 0);

	DebugPrintf("\n");
}

void DoTensorNANCheck(torch::Tensor x, bool print)
{
	bool	nanDetected = torch::isnan(x).any().item<bool>();
	if( nanDetected )
	{
		if( print )
		{
			DebugPrintf("NaN values detected in tensor\n");
			DebugPrintTensor(x, "NANTensor");
			Assert(false);
		}
		else
		{
			AssertMsg(false, "NaN values detected\n");
		}
	}
}

void CheckRequiresGrad(torch::Tensor input)
{
	Assert(input.requires_grad());
}

torch::Tensor RecombineToMtx43(torch::Tensor rotations, torch::Tensor scales, torch::Tensor translations)
{
	// Check dimensions
    TORCH_CHECK(rotations.size(0) == scales.size(0) && rotations.size(0) == translations.size(0), "Input tensor dimensions do not match.");

    // Multiply rotation matrix by scale using broadcasting
    torch::Tensor	scaledRotations = rotations * scales.view({-1, 1, 3});

    // Concatenate translation to form a 4x3 matrix
    torch::Tensor	matrices = torch::cat({scaledRotations, translations.view({-1, 1, 3})}, 1);

    return matrices;
}

/*
torch::norm's second parameter (according to ChatGPT):
	1: L1 norm (Manhattan norm)
	2: L2 norm (Euclidean norm)
	INFINITY or -1: Infinity norm (maximum absolute row sum)
	-INFINITY or -2: Negative infinity norm (minimum absolute row sum)
*/

//	this function is kinda slow
//	a non-tensor version I wrote also didn't work very well, so there could be some problems in this still
torch::Tensor QuaternionTensorSlerp(const torch::Tensor& q1, const torch::Tensor& q2, const torch::Tensor& t)
{
	Assert(q1.size(-1) == 4);
	Assert(q2.size(-1) == 4);

	// Normalize input quaternions
	torch::Tensor	q1Normalized = q1 / torch::norm(q1, 2, -1, true);
	torch::Tensor	q2Normalized = q2 / torch::norm(q2, 2, -1, true);

	// Compute the dot product between the quaternions
	torch::Tensor	dotProduct = torch::sum(q1Normalized * q2Normalized, -1, true);

	const f32		kEpsilon = 0.001f;
	torch::Tensor	largeDotMask = torch::abs(dotProduct) > (1.0f - kEpsilon);

	dotProduct = torch::clamp(dotProduct, kEpsilon - 1.0f, 1.0f - kEpsilon);	//	necessary for numeric stability along with the mask; was getting a lot of NaN without it

	// Calculate the angle between the quaternions
	torch::Tensor	theta = torch::acos(dotProduct);		//	should be 0 to pi
	torch::Tensor	invSinTheta = 1.0f / torch::sin(theta);

	torch::Tensor	invInterpT = torch::where(
        largeDotMask,
        1.0f - t,
        torch::sin((1.0f - t) * theta) * invSinTheta
    );

	torch::Tensor	interpT = torch::where(
        largeDotMask,
        t,
        torch::sin(t * theta) * invSinTheta
    );

	torch::Tensor	slerpResult = q1Normalized * invInterpT + q2Normalized * interpT;

	torch::Tensor	result = slerpResult / torch::norm(slerpResult, 2, -1, true);

	return result;
}

torch::Tensor XYBasisToRotMtx33(const torch::Tensor xy)
{
	Assert(xy.sizes().size() >= 2);
	Assert(xy.size(-1) == 3);
	Assert(xy.size(-2) >= 2);	//	if this is greater than 2 we're just ignoring the rest though

	torch::Tensor	a1 = xy.slice(-2, 0, 1);
	torch::Tensor	a2 = xy.slice(-2, 1, 2);

	torch::Tensor	right = a1 / torch::norm(a1, 2, -1, true);

	torch::Tensor	rightDotA2 = torch::sum(right * a2, -1, true);

	//torch::Tensor	up = a2 - (right * right.dot(a2));
	torch::Tensor	up = a2 - (right * rightDotA2);
	up = up / torch::norm(up, 2, -1, true);

	torch::Tensor	forward = right.cross(up);

	return torch::cat({right, up, forward}, -2);
}

void GetParameterCount(torch::nn::Module module, int64_t* pParameterTensorCountOut, int64_t* pParameterElementCountOut)
{
	Assert(pParameterTensorCountOut != nullptr);
	Assert(pParameterElementCountOut != nullptr);

	torch::autograd::variable_list	parameterList = module.parameters(true);

	*pParameterElementCountOut = 0;
	*pParameterTensorCountOut = (int64_t)parameterList.size();
	
	for(size_t parameterIdx = 0; parameterIdx < parameterList.size(); ++parameterIdx)
	{
		*pParameterElementCountOut += parameterList[parameterIdx].numel();
	}
}

torch::Tensor DotProductAlignmentLossNN(torch::Tensor inputs, torch::Tensor targets, float scale, bool applyReduction)
{
	Assert(inputs.size(-1) == targets.size(-1));

	torch::nn::functional::CosineSimilarityFuncOptions	csOptions;
	csOptions.dim() = -1;

	torch::Tensor	dot = torch::nn::functional::cosine_similarity(inputs, targets, csOptions);

	//	dot should be between -1 and 1
	//	we'd like a result that aligns the input vectors, so we want the dot products to tend toward 1
	//	if we negate dot and add 1 the result is from 0 to 2, with the original 1 mapped to 0, and that gives us something we can minimize
	torch::Tensor	loss = (dot * -1.0) + 1.0;
	loss = loss * scale;

	if( applyReduction )
	{
		loss = loss.mean();
	}

	return loss;
}

torch::Tensor DotProductAlignmentLoss(torch::Tensor inputs, torch::Tensor targets, float scale, bool applyReduction)
{
	Assert(inputs.size(-1) == targets.size(-1));

	//	normalize input and target (really target should already be normalized, but just in case)
	torch::Tensor	inputNormalized = inputs / torch::norm(inputs, 2, -1, true);
	torch::Tensor	targetNormalized = targets / torch::norm(targets, 2, -1, true);

	//	perform dot product
	//torch::Tensor	dot = torch::dot(inputNormalized, targetNormalized);	//	this only works on 1-d tensors
	torch::Tensor	dot = (inputNormalized * targetNormalized).sum(-1);

	//	dot should be between -1 and 1 since we normalized
	//	we'd like a result that aligns the input vectors, so we want the dot products to tend toward 1
	//	if we negate dot and add 1 the result is from 0 to 2, with the original 1 mapped to 0, and that gives us something we can minimize
	torch::Tensor	loss = (dot * -1.0) + 1.0;
	loss = loss * scale;

	if( applyReduction )
	{
		loss = loss.mean();
	}

	return loss;
}

torch::Tensor DotProductAlignmentSquaredLoss(torch::Tensor inputs, torch::Tensor targets, float scale, bool applyReduction)
{
	Assert(inputs.size(-1) == targets.size(-1));

	//	normalize input and target (really target should already be normalized, but just in case)
	torch::Tensor	inputNormalized = inputs / torch::norm(inputs, 2, -1, true);
	torch::Tensor	targetNormalized = targets / torch::norm(targets, 2, -1, true);

	//	perform dot product
	//torch::Tensor	dot = torch::dot(inputNormalized, targetNormalized);	//	this only works on 1-d tensors
	torch::Tensor	dot = (inputNormalized * targetNormalized).sum(-1);

	//	dot should be between -1 and 1 since we normalized
	//	we'd like a result that aligns the input vectors, so we want the dot products to tend toward 1
	//	if we negate dot and add 1 the result is from 0 to 2, with the original 1 mapped to 0, and that gives us something we can minimize
	torch::Tensor	loss = (dot * -1.0) + 1.0;
	loss = loss * scale;
	loss = loss * loss;

	if( applyReduction )
	{
		loss = loss.mean();
	}

	return loss;
}

void CommonInitModule(torch::nn::Module& module)
{
	torch::NoGradGuard			no_grad;

	if( torch::nn::LinearImpl* linear = dynamic_cast<torch::nn::LinearImpl*>(&module) )
    {
		//torch::nn::init::normal_(linear->weight, 0.0, 02);
		torch::nn::init::kaiming_normal_(linear->weight);
		//torch::nn::init::normal_(linear->weight, 0.0, 1.0 / sqrt(linear->weight.size(0)));
		//torch::nn::init::normal_(linear->weight, 0.0, 1.0 / linear->weight.size(0));

		//int	fanIn = linear->weight.size(0);
		//linear->weight = torch::randn(linear->weight.sizes()) * ((5.0f / 3.0f) / sqrt(fanIn));

		if( linear->bias.defined() )
		{
			//	what the hell?  zeros_ was causing NaN in one scenario ...
			torch::nn::init::zeros_(linear->bias);
			//torch::nn::init::constant_(linear->bias, 0.00001);
		}
    } 
    else if( torch::nn::EmbeddingImpl* embedding = dynamic_cast<torch::nn::EmbeddingImpl*>(&module) )
    {
		//torch::nn::init::constant_(embedding->weight, 0.02);
		torch::nn::init::normal_(embedding->weight, 0.0, 0.02);
		//torch::nn::init::kaiming_normal_(embedding->weight);
    }
	else if( torch::nn::Conv2dImpl* conv2d = dynamic_cast<torch::nn::Conv2dImpl*>(&module) )
    {
        //torch::nn::init::normal_(conv2d->weight, 0.0, 0.02);
		torch::nn::init::kaiming_normal_(conv2d->weight);
		if( conv2d->bias.defined() )
		{
			torch::nn::init::zeros_(conv2d->bias);
			//torch::nn::init::constant_(conv2d->bias, 0.00001);
		}
    }
	//else if( torch::nn::GroupNormImpl* groupNorm = dynamic_cast<torch::nn::GroupNormImpl*>(&module) )
	//{
	//	//torch::nn::init::normal_(groupNorm->weight, 0.0, 0.02);
	//	//torch::nn::init::kaiming_normal_(groupNorm->weight);
	//	torch::nn::init::ones_(groupNorm->weight);
	//	if( groupNorm->bias.defined() )
	//	{
	//		torch::nn::init::zeros_(groupNorm->bias);
	//		//torch::nn::init::constant_(groupNorm->bias, 0.00001);
	//	}
	//}
	//else if( torch::nn::LayerNormImpl* layerNorm = dynamic_cast<torch::nn::LayerNormImpl*>(&module) )
	//{
	//	torch::nn::init::ones_(layerNorm->weight);

	//	if( layerNorm->bias.defined() )
	//	{
	//		torch::nn::init::zeros_(layerNorm->bias);
	//	}
	//}
}

void AddNonLinearityModule(torch::nn::Sequential net, eNonLinearity nonLinearity, const char* name)
{
	switch(nonLinearity)
	{
	case kNL_Tanh:
		net->push_back(net->register_module(name, torch::nn::Tanh()));
		break;

	case kNL_Silu:
		net->push_back(net->register_module(name, torch::nn::SiLU()));
		break;

	case kNL_Gelu:
		net->push_back(net->register_module(name, torch::nn::GELU()));
		break;

	case kNL_QuickGelu:
		net->push_back(net->register_module(name, QuickGELU()));
		break;

	default:
	case kNL_Relu:
		net->push_back(net->register_module(name, torch::nn::ReLU()));
		break;
	}
}

//	see Appendix B from VAE paper:
//	Kingma and Welling. Auto-Encoding Variational Bayes. ICLR, 2014
//	https://arxiv.org/abs/1312.6114

torch::Tensor UnitGaussianKLLossBatchDim(torch::Tensor mean, torch::Tensor logVar, const at::OptionalIntArrayRef& dims)
{
	return 0.5 * torch::sum(mean.square() + logVar.exp() - logVar - 1, dims);
}

torch::Tensor UnitGaussianKLLossInvariant(torch::Tensor mean, torch::Tensor logVar)
{
	return 0.5 * torch::mean(mean.square() + logVar.exp() - logVar - 1);
}

//	hasn't been tested at all, might be wrong
torch::Tensor KLLoss(torch::Tensor mean1, torch::Tensor logVar1, torch::Tensor mean2, torch::Tensor logVar2)
{
	torch::Tensor	KLD	= 0.5 * (-1.0 + logVar2 - logVar1 + (logVar1 - logVar2).exp() + ((mean1 - mean2).square()) * (-logVar2).exp());
	return KLD.mean();
}