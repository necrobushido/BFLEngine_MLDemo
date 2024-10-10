#pragma once

#include "types.h"
#include "Vector3.h"
#include "SharedMLHeader.h"
#include "Tensorizable.h"

class VectorSequence : public ITensorizable
{
public:
	VectorSequence();

	void AddNext(const Vector3& next);

	torch::Tensor ToTensor() override;

	void Clear();

protected:
	std::vector<Vector3>	m_elements;
};