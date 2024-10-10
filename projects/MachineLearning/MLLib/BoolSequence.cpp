#include "BoolSequence.h"

BoolSequence::BoolSequence()
{
}

void BoolSequence::AddNext(bool next)
{
	//m_elements.push_back(next);
	m_elements.Add(next);
}

torch::Tensor BoolSequence::ToTensor()
{
	return torch::from_blob(m_elements.Data(), {(int)m_elements.Count()}, torch::kBool).clone();
}

void BoolSequence::Clear()
{
	//m_elements.clear();
	m_elements.Clear();
}