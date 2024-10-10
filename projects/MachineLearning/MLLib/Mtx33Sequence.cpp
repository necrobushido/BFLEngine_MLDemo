#include "Mtx33Sequence.h"

Mtx33Sequence::Mtx33Sequence()
{
}

void Mtx33Sequence::AddNext(const Mtx33& next)
{
	m_elements.push_back(next);
}

torch::Tensor Mtx33Sequence::ToTensor()
{
	return torch::from_blob(m_elements.data(), {(int)m_elements.size(), Mtx33::kRowCount, Mtx33::kColumnCount}, torch::kF32).clone();
}

void Mtx33Sequence::Clear()
{
	m_elements.clear();
}