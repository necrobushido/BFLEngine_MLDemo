#pragma once

#include "types.h"
#include "Mtx33.h"
#include "SharedMLHeader.h"
#include "Tensorizable.h"

class Mtx33Sequence : public ITensorizable
{
public:
	Mtx33Sequence();

	void AddNext(const Mtx33& next);

	torch::Tensor ToTensor() override;

	void Clear();

protected:
	std::vector<Mtx33>	m_elements;
};