#pragma once

#include "undefnew.h"
#pragma warning( push )
#pragma warning( disable : 4067 )	// unexpected tokens following preprocessor directive - expected a newline
#pragma warning( disable : 4624 )	// destructor was implicitly defined as deleted
#pragma warning( disable : 4267 )
#pragma warning( disable : 4251 )
#pragma warning( disable : 4522 )
#pragma warning( disable : 4838 )
#pragma warning( disable : 4305 )
#pragma warning( disable : 4244 )
#pragma warning( disable : 4190 )
#pragma warning( disable : 4101 )
#pragma warning( disable : 4996 )
#pragma warning( disable : 4275 )
#include <torch/torch.h>
#pragma warning( pop )
#include "defnew.h"

#include <ATen/autocast_mode.h>

void PrintTensorShape(torch::Tensor inputTensor, const char* label);

void DebugPrintTensor(torch::Tensor inputTensor, const char* label);

void DoTensorNANCheck(torch::Tensor x, bool print = false);

void CheckRequiresGrad(torch::Tensor input);

torch::Tensor RecombineToMtx43(torch::Tensor rotations, torch::Tensor scales, torch::Tensor translations);

torch::Tensor QuaternionTensorSlerp(const torch::Tensor& q1, const torch::Tensor& q2, const torch::Tensor& t);

torch::Tensor XYBasisToRotMtx33(const torch::Tensor xy);

void GetParameterCount(torch::nn::Module module, int64_t* pParameterTensorCountOut, int64_t* pParameterElementCountOut);

//	align input vectors with target vectors.  
//	result is between 0 and 2, with 0 perfectly aligned.
//	"applyReduction = true" means to take the mean of all of the sub values and give a single result
//	"applyReduction = false" returns the tensor of sub-values
torch::Tensor DotProductAlignmentLossNN(torch::Tensor inputs, torch::Tensor targets, float scale = 1.0f, bool applyReduction=true);
torch::Tensor DotProductAlignmentLoss(torch::Tensor inputs, torch::Tensor targets, float scale = 1.0f, bool applyReduction=true);
torch::Tensor DotProductAlignmentSquaredLoss(torch::Tensor inputs, torch::Tensor targets, float scale = 1.0f, bool applyReduction=true);

torch::Tensor UnitGaussianKLLossBatchDim(torch::Tensor mean, torch::Tensor logVar, const at::OptionalIntArrayRef& dims);
torch::Tensor UnitGaussianKLLossInvariant(torch::Tensor mean, torch::Tensor logVar);
torch::Tensor KLLoss(torch::Tensor mean1, torch::Tensor logVar1, torch::Tensor mean2, torch::Tensor logVar2);

void CommonInitModule(torch::nn::Module& module);

class QuickGELUImpl : public torch::nn::Module 
{
public:
	QuickGELUImpl()
	{
	}

    torch::Tensor forward(torch::Tensor x)
	{
		x = x * torch::sigmoid(1.702 * x);

		return x;
	}
};

TORCH_MODULE(QuickGELU);

enum eNonLinearity
{
	kNL_Relu,
	kNL_Silu,
	kNL_Gelu,
	kNL_QuickGelu,
	kNL_Tanh
};
void AddNonLinearityModule(torch::nn::Sequential net, eNonLinearity nonLinearity, const char* name);

enum eBatchType
{
	kBatchType_Training,
	kBatchType_Validation,

	kBatchType_NumBatchTypes
};

#define DEBUG_CRAP(x) \
	try \
	{ \
		x; \
	} \
	catch (const std::exception& e) \
	{ \
		std::cout << "Exception: " << e.what() << std::endl; \
	}

#define DEBUG_CRAP2(x) \
	try \
	{ \
		x; \
	} \
	catch (const std::exception& e) \
	{ \
		DebugPrintf("Exception: %s\n", e.what()); \
	}
