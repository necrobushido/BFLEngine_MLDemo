#include "C3DR6Sequence.h"

C3DR6Sequence::C3DR6Sequence()
{
}

void C3DR6Sequence::AddNext(const Mtx33& next)
{
	Continuous3DRotation6	nextElement;
	nextElement.SetFrom(next);

	m_elements.push_back(nextElement);
}

void C3DR6Sequence::AddNext(const Continuous3DRotation6& next)
{
	m_elements.push_back(next);
}

torch::Tensor C3DR6Sequence::ToTensor()
{
	return torch::from_blob(m_elements.data(), {(int)m_elements.size(), Continuous3DRotation6::kElementCount}, torch::kF32).clone();
}

void C3DR6Sequence::Clear()
{
	m_elements.clear();
}