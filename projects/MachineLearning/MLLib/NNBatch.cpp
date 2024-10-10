#include "NNBatch.h"

#include "NNModelComponent.h"

void NNBatch::Clear()
{
	m_trainingTagToBatchTensorMap.clear();
	m_validationTagToBatchTensorMap.clear();
}

void NNBatch::GetBatchData(int batchSize, eBatchType batchType)
{
	torch::NoGradGuard	no_grad;

	switch(batchType)
	{
	case kBatchType_Training:
		for(NNModelComponent* pNNModelComponent = LIST_HEAD(&NNModelComponent::s_componentList, AllComponents); (pNNModelComponent != NULL); pNNModelComponent = LIST_NEXT(pNNModelComponent, AllComponents)) 
		{ 
			if( pNNModelComponent->IsTraining() )
			{
				pNNModelComponent->GetBatch(batchSize, batchType, &m_trainingTagToBatchTensorMap);
			}
		}
		break;

	case kBatchType_Validation:
		for(NNModelComponent* pNNModelComponent = LIST_HEAD(&NNModelComponent::s_componentList, AllComponents); (pNNModelComponent != NULL); pNNModelComponent = LIST_NEXT(pNNModelComponent, AllComponents)) 
		{ 
			if( pNNModelComponent->IsTraining() )
			{
				pNNModelComponent->GetBatch(batchSize, batchType, &m_validationTagToBatchTensorMap);
			}
		}
		break;

	default:
		Assert(0);
		break;
	}
}

void NNBatch::MoveToDevice(torch::Device device)
{
	for(std::map<std::string, torch::Tensor>::iterator iter = m_trainingTagToBatchTensorMap.begin(); iter != m_trainingTagToBatchTensorMap.end(); ++iter)
	{
		iter->second = iter->second.to(device);
	}

	for(std::map<std::string, torch::Tensor>::iterator iter = m_validationTagToBatchTensorMap.begin(); iter != m_validationTagToBatchTensorMap.end(); ++iter)
	{
		iter->second = iter->second.to(device);
	}
}