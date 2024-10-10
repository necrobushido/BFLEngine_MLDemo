#pragma once

#include "types.h"
#include "Continuous3DRotation6.h"
#include "SharedMLHeader.h"
#include "Tensorizable.h"

class C3DR6Sequence : public ITensorizable
{
public:
	C3DR6Sequence();

	void AddNext(const Mtx33& next);
	void AddNext(const Continuous3DRotation6& next);

	torch::Tensor ToTensor() override;

	void Clear();

protected:
	std::vector<Continuous3DRotation6>	m_elements;
};