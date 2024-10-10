#pragma once

#include "NNModelComponent.h"

class NNDDPMSampler;

class NNDDPMModelComponent : public NNModelComponent
{
public:
	NNDDPMModelComponent();
	virtual ~NNDDPMModelComponent();

protected:
	NNDDPMSampler*		m_pNNDDPMSampler;
};
