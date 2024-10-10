#include "Continuous3DRotation6.h"

#include "Mtx33.h"
#include "Quat.h"

Continuous3DRotation6::Continuous3DRotation6(const coord_type* from)
{
	memcpy(a, from, sizeof(a));
}

void Continuous3DRotation6::SetFrom(const Quat& quatIn)
{
	Mtx33	quatMtx;
	quatIn.Convert(&quatMtx);

	SetFrom(quatMtx);
}

void Continuous3DRotation6::SetFrom(const Mtx33& mtxIn)
{
	//	just dump the last row

	for(int rowIdx = 0; rowIdx < kRowCount; ++rowIdx)
	{
		for(int columnIdx = 0; columnIdx < kColumnCount; ++columnIdx)
		{
			m[rowIdx][columnIdx] = mtxIn.m[rowIdx][columnIdx];
		}
	}
}

void Continuous3DRotation6::ConvertTo(Quat* quatOut)
{
	Mtx33	quatMtx;
	ConvertTo(&quatMtx);

	*quatOut = Quat(quatMtx);
}

void Continuous3DRotation6::ConvertTo(Mtx33* mtxOut)
{
	//	make right and up orthonormal, then use cross product to get forward

	Vector3	a1(&_00);
	Vector3	a2(&_10);

	Vector3	right = a1;
	right.Normalize();

	Vector3	up = a2 - (right * right.DotProduct(a2));
	up.Normalize();

	Vector3	forward = right.CrossProduct(up);

	mtxOut->SetRight(right);
	mtxOut->SetUp(up);
	mtxOut->SetForward(forward);
}