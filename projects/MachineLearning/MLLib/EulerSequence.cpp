#include "EulerSequence.h"

#include "MathNamespace.h"

EulerSequence::EulerSequence()
{
}

void EulerSequence::AddNext(const Mtx33& next, bool normalize)
{
	Vector3	eulerAngles;
	next.GetEulerAngles(&eulerAngles);

	//	test matrix reconstruction from euler angles
	//Mtx33	eulerTestMtx;
	//eulerTestMtx.ConstructEulerRotation(eulerAngles);
	//Assert(Math::NearEqual(next.a, eulerTestMtx.a, Mtx33::kElementCount, 0.01f));

	Vector3	nextElement = eulerAngles;
	if( normalize )
	{
		//	test euler normalization
		//Math::NormalizeEuler(eulerAngles, &nextElement);
		//Math::DenormalizeEuler(nextElement, &nextElement);
		//eulerTestMtx.ConstructEulerRotation(nextElement);
		//Assert(Math::NearEqual(next.a, eulerTestMtx.a, Mtx33::kElementCount, 0.01f));

		Math::NormalizeEuler(eulerAngles, &nextElement);
	}

	m_elements.push_back(nextElement);
}

torch::Tensor EulerSequence::ToTensor()
{
	return torch::from_blob(m_elements.data(), {(int)m_elements.size(), Vector3::kElementCount}, torch::kF32).clone();
}

void EulerSequence::Clear()
{
	m_elements.clear();
}