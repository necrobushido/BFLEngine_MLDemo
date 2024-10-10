#pragma once

#include "types.h"
#include "Tensorizable.h"
#include "EngineArray.h"

class BoolSequence : public ITensorizable
{
public:
	BoolSequence();

	void AddNext(bool next);

	torch::Tensor ToTensor() override;

	void Clear();

protected:
	//std::vector<bool>	m_elements;	//	std::vector<bool> is special-cased and doesn't do what we want
	Array<bool>			m_elements;
};