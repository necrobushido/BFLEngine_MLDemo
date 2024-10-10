#pragma once

#include "types.h"
#include "SharedMLHeader.h"
#include "Tensorizable.h"
#include "Mtx33.h"

class EulerSequence : public ITensorizable
{
public:
	EulerSequence();

	void AddNext(const Mtx33& next, bool normalize=false);

	torch::Tensor ToTensor() override;

	void Clear();

protected:
	std::vector<Vector3>	m_elements;
};