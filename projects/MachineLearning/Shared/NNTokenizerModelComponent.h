#pragma once

#include "NNModelComponent.h"

#include "NNTokenizer.h"

class NNTokenizerModelComponent : public NNModelComponent
{
public:
	NNTokenizerModelComponent();
	virtual ~NNTokenizerModelComponent();

protected:
	NNTokenizer	m_tokenizer;
};
