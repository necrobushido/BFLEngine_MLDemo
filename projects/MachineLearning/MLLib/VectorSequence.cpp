#include "VectorSequence.h"

VectorSequence::VectorSequence()
{
}

void VectorSequence::AddNext(const Vector3& next)
{
	m_elements.push_back(next);
}

torch::Tensor VectorSequence::ToTensor()
{
	return torch::from_blob(m_elements.data(), {(int)m_elements.size(), Vector3::kElementCount}, torch::kF32).clone();
}

void VectorSequence::Clear()
{
	m_elements.clear();
}