#include "NNLowMemOptimizer.h"

/*
	https://pytorch.org/tutorials/intermediate/optimizer_step_in_backward_tutorial.html

	The idea is that this thing fuses the optimizer step into the backward pass,
	by applying the gradient right when it is calculated and then dumping it.
	Make sure not to use the usual step() and zero_grad() calls with this, 
	since they are called internally on the individual gradients.

	In my first test on NoiseFinder this used more memory, not less
*/

NNLowMemOptimizer::NNLowMemOptimizer()
{
}

NNLowMemOptimizer::~NNLowMemOptimizer()
{
	Clear();
}

void NNLowMemOptimizer::Init(std::vector<torch::Tensor> params, float learningRate)
{
	for(std::vector<torch::Tensor>::iterator paramIter = params.begin(); paramIter != params.end(); paramIter++)
	{
		std::vector<torch::Tensor>	thisParamSet;
		thisParamSet.push_back(*paramIter);

		torch::optim::AdamWOptions	adamwOptions(learningRate);
		torch::optim::AdamW*		pThisOptimizer = new torch::optim::AdamW(thisParamSet, adamwOptions);

		auto GradChangeCallback = [pThisOptimizer](const at::TensorBase& gradient)
			{
				pThisOptimizer->step();
				pThisOptimizer->zero_grad(true);
			};

		paramIter->register_hook(GradChangeCallback);

		m_subOptimizers.push_back(pThisOptimizer);
	}
}

void NNLowMemOptimizer::Clear()
{
	for(std::vector<torch::optim::AdamW*>::iterator subOptimizerIter = m_subOptimizers.begin(); subOptimizerIter != m_subOptimizers.end(); subOptimizerIter++)
	{
		torch::optim::AdamW*	thisSubOptimizer = *subOptimizerIter;
		delete thisSubOptimizer;
	}

	m_subOptimizers.clear();
}

bool NNLowMemOptimizer::Initialized() const
{
	return m_subOptimizers.size() > 0;
}

void NNLowMemOptimizer::SetLearningRate(float learningRate)
{
	for(std::vector<torch::optim::AdamW*>::iterator subOptimizerIter = m_subOptimizers.begin(); subOptimizerIter != m_subOptimizers.end(); subOptimizerIter++)
	{
		torch::optim::AdamW*	thisSubOptimizer = *subOptimizerIter;

		std::vector<torch::optim::OptimizerParamGroup>&	paramGroups = thisSubOptimizer->param_groups();
		for(std::vector<torch::optim::OptimizerParamGroup>::iterator iter = paramGroups.begin(); iter != paramGroups.end(); iter++)
		{
			iter->options().set_lr(learningRate);
		}
	}
}