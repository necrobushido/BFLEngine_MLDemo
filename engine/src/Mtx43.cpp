#include "Mtx43.h"
#include "Vector3.h"
#include "Mtx44.h"

#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#ifndef BFL_TOOLS

const f32	kIdentityFloats[] =
{
	1.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 0.0f,
};

const Mtx43	Mtx43::IDENTITY(kIdentityFloats);

Mtx43::Mtx43(const coord_type* from)
{
	memcpy(a, from, sizeof(a));
}

#endif

void Mtx43::Identity()
{
	m[0][0] = 1;
	m[1][0] = 0;
	m[2][0] = 0;
	m[3][0] = 0;

	m[0][1] = 0;
	m[1][1] = 1;
	m[2][1] = 0;
	m[3][1] = 0;

	m[0][2] = 0;
	m[1][2] = 0;
	m[2][2] = 1;
	m[3][2] = 0;
}

void Mtx43::Inverse( Mtx43& out ) const
{
	//	not sure if this function is correct, need to test
	Vector3	translation = GetTranslation();
	translation *= -1.0f;

	Mtx33	temp33 = ExtractMtx33();
	temp33 = temp33.Inverse();
	out.SetFromMtx33(temp33);

	translation = temp33.MultiplyVec(translation);
	out.SetTranslation(translation);
}

void Mtx43::Multiply(const Mtx43& rhs, Mtx43& out) const
{
	//	treat these as 4x4 implicitly, with a {0, 0, 0, 1} 4th column
	out.m[0][0] = m[0][0]*rhs.m[0][0] + m[0][1]*rhs.m[1][0] + m[0][2]*rhs.m[2][0];
	out.m[0][1] = m[0][0]*rhs.m[0][1] + m[0][1]*rhs.m[1][1] + m[0][2]*rhs.m[2][1];
	out.m[0][2] = m[0][0]*rhs.m[0][2] + m[0][1]*rhs.m[1][2] + m[0][2]*rhs.m[2][2];

	out.m[1][0] = m[1][0]*rhs.m[0][0] + m[1][1]*rhs.m[1][0] + m[1][2]*rhs.m[2][0];
	out.m[1][1] = m[1][0]*rhs.m[0][1] + m[1][1]*rhs.m[1][1] + m[1][2]*rhs.m[2][1];
	out.m[1][2] = m[1][0]*rhs.m[0][2] + m[1][1]*rhs.m[1][2] + m[1][2]*rhs.m[2][2];
	
	out.m[2][0] = m[2][0]*rhs.m[0][0] + m[2][1]*rhs.m[1][0] + m[2][2]*rhs.m[2][0];
	out.m[2][1] = m[2][0]*rhs.m[0][1] + m[2][1]*rhs.m[1][1] + m[2][2]*rhs.m[2][1];
	out.m[2][2] = m[2][0]*rhs.m[0][2] + m[2][1]*rhs.m[1][2] + m[2][2]*rhs.m[2][2];

	out.m[3][0] = m[3][0]*rhs.m[0][0] + m[3][1]*rhs.m[1][0] + m[3][2]*rhs.m[2][0] + rhs.m[3][0];
	out.m[3][1] = m[3][0]*rhs.m[0][1] + m[3][1]*rhs.m[1][1] + m[3][2]*rhs.m[2][1] + rhs.m[3][1];
	out.m[3][2] = m[3][0]*rhs.m[0][2] + m[3][1]*rhs.m[1][2] + m[3][2]*rhs.m[2][2] + rhs.m[3][2];
}

Mtx44 Mtx43::Multiply44(const Mtx44& rhs) const
{
	Mtx44	result;
	Multiply44(rhs, result);

	return result;
}

void Mtx43::Multiply44(const Mtx44& rhs, Mtx44& out) const
{
	//	treat these as 4x4 implicitly, with a {0, 0, 0, 1} 4th column
	out.m[0][0] = m[0][0]*rhs.m[0][0] + m[0][1]*rhs.m[1][0] + m[0][2]*rhs.m[2][0];
	out.m[0][1] = m[0][0]*rhs.m[0][1] + m[0][1]*rhs.m[1][1] + m[0][2]*rhs.m[2][1];
	out.m[0][2] = m[0][0]*rhs.m[0][2] + m[0][1]*rhs.m[1][2] + m[0][2]*rhs.m[2][2];
	out.m[0][3] = m[0][0]*rhs.m[0][3] + m[0][1]*rhs.m[1][3] + m[0][2]*rhs.m[2][3];

	out.m[1][0] = m[1][0]*rhs.m[0][0] + m[1][1]*rhs.m[1][0] + m[1][2]*rhs.m[2][0];
	out.m[1][1] = m[1][0]*rhs.m[0][1] + m[1][1]*rhs.m[1][1] + m[1][2]*rhs.m[2][1];
	out.m[1][2] = m[1][0]*rhs.m[0][2] + m[1][1]*rhs.m[1][2] + m[1][2]*rhs.m[2][2];
	out.m[1][3] = m[1][0]*rhs.m[0][3] + m[1][1]*rhs.m[1][3] + m[1][2]*rhs.m[2][3];
	
	out.m[2][0] = m[2][0]*rhs.m[0][0] + m[2][1]*rhs.m[1][0] + m[2][2]*rhs.m[2][0];
	out.m[2][1] = m[2][0]*rhs.m[0][1] + m[2][1]*rhs.m[1][1] + m[2][2]*rhs.m[2][1];
	out.m[2][2] = m[2][0]*rhs.m[0][2] + m[2][1]*rhs.m[1][2] + m[2][2]*rhs.m[2][2];
	out.m[2][3] = m[2][0]*rhs.m[0][3] + m[2][1]*rhs.m[1][3] + m[2][2]*rhs.m[2][3];

	out.m[3][0] = m[3][0]*rhs.m[0][0] + m[3][1]*rhs.m[1][0] + m[3][2]*rhs.m[2][0] + rhs.m[3][0];
	out.m[3][1] = m[3][0]*rhs.m[0][1] + m[3][1]*rhs.m[1][1] + m[3][2]*rhs.m[2][1] + rhs.m[3][1];
	out.m[3][2] = m[3][0]*rhs.m[0][2] + m[3][1]*rhs.m[1][2] + m[3][2]*rhs.m[2][2] + rhs.m[3][2];
	out.m[3][3] = m[3][0]*rhs.m[0][3] + m[3][1]*rhs.m[1][3] + m[3][2]*rhs.m[2][3] + rhs.m[3][3];
}

void Mtx43::MultiplyVec( const Vector3& vec, Vector3& out, bool translate ) const
{
	out.x = m[0][0]*vec.x + m[1][0]*vec.y + m[2][0]*vec.z;
    out.y = m[0][1]*vec.x + m[1][1]*vec.y + m[2][1]*vec.z;
    out.z = m[0][2]*vec.x + m[1][2]*vec.y + m[2][2]*vec.z;

	if( translate )
	{
		out.x += m[3][0];
		out.y += m[3][1];
		out.z += m[3][2];
	}
}

void Mtx43::ConstructPitchRotation(f32 pitch)
{
	m[0][0] = 1;
	m[1][0] = 0;
	m[2][0] = 0;
	m[3][0] = 0;

	m[0][1] = 0;
	m[1][1] = (coord_type)cos(pitch);
	m[2][1] = (coord_type)sin(pitch);
	m[3][1] = 0;

	m[0][2] = 0;
	m[1][2] = (coord_type)-sin(pitch);
	m[2][2] = (coord_type)cos(pitch);
	m[3][2] = 0;
}

void Mtx43::ConstructYawRotation(f32 yaw)
{
	m[0][0] = (coord_type)cos(yaw);
	m[1][0] = 0;
	m[2][0] = (coord_type)-sin(yaw);
	m[3][0] = 0;

	m[0][1] = 0;
	m[1][1] = 1;
	m[2][1] = 0;
	m[3][1] = 0;

	m[0][2] = (coord_type)sin(yaw);
	m[1][2] = 0;
	m[2][2] = (coord_type)cos(yaw);
	m[3][2] = 0;
}

void Mtx43::ConstructRollRotation(f32 roll)
{
	m[0][0] = (coord_type)cos(roll);
	m[1][0] = (coord_type)sin(roll);
	m[2][0] = 0;
	m[3][0] = 0;

	m[0][1] = (coord_type)-sin(roll);
	m[1][1] = (coord_type)cos(roll);
	m[2][1] = 0;
	m[3][1] = 0;

	m[0][2] = 0;
	m[1][2] = 0;
	m[2][2] = 1;
	m[3][2] = 0;
}

void Mtx43::ConstructTransform(f32 pitch, f32 yaw, f32 roll, Vector3 translation)
{
	Mtx43	pitchMtx;
	Mtx43	yawMtx;
	Mtx43	rollMtx;

	pitchMtx.ConstructPitchRotation(pitch);
	yawMtx.ConstructYawRotation(yaw);
	rollMtx.ConstructRollRotation(roll);

	*this = pitchMtx * yawMtx * rollMtx;
	SetTranslation(translation);
}

const Vector3& Mtx43::GetTranslation() const
{
	const f32*		start = &_30;
	const Vector3*	pVec = (Vector3*)start;
	return *pVec;
}

Mtx33 Mtx43::ExtractMtx33() const
{
	Mtx33	returnValue;

	returnValue._00 = _00;
	returnValue._01 = _01;
	returnValue._02 = _02;
	returnValue._10 = _10;
	returnValue._11 = _11;
	returnValue._12 = _12;
	returnValue._20 = _20;
	returnValue._21 = _21;
	returnValue._22 = _22;

	return returnValue;
}

void Mtx43::SetFromMtx33(const Mtx33& from)
{
	_00 = from._00;
	_01 = from._01;
	_02 = from._02;
	_10 = from._10;
	_11 = from._11;
	_12 = from._12;
	_20 = from._20;
	_21 = from._21;
	_22 = from._22;

	//	no source translation so zero it out
	_30 = 0.0f;
	_31 = 0.0f;
	_32 = 0.0f;
}

Mtx33 Mtx43::ExtractRotation() const
{
	Mtx33	result;
	result.Identity();

	Vector3	right = GetRight();
	Vector3 up = GetUp();
	Vector3 forward = GetForward();

	right.Normalize();
	up.Normalize();
	forward.Normalize();

	result.SetRight(right);
	result.SetUp(up);
	result.SetForward(forward);

	return result;
}

Mtx43 Mtx43::OrthoNormalInvert()
{
	Mtx43	result;

	Mtx33	rotate33 = ExtractMtx33();
	Mtx33	invRotate33;
	rotate33.Transpose(invRotate33);
	result.SetFromMtx33(invRotate33);

	Vector3	pos = GetTranslation();
	pos = result.MultiplyVec(pos, false);
	pos *= -1.0f;
	result.SetTranslation(pos);

	return result;
}

bool Mtx43::IsEqualTo(const Mtx43& ref, f32 tolerance) const
{
	bool	returnValue = true;
	for(int i = 0; i < kElementCount && returnValue; i++)
	{
		returnValue = fabs(a[i] - ref.a[i]) <= tolerance;
	}

	return returnValue;
}

bool Mtx43::operator==(const Mtx43& ref) const
{
	bool	returnValue = true;
	for(int i = 0; i < kElementCount && returnValue; i++)
	{
		returnValue = a[i] == ref.a[i];
	}

	return returnValue;
}

Mtx43 Mtx43::operator*(const f32& scalar) const
{
	Mtx43	returnValue;
	for(int i = 0; i < kElementCount; i++)
	{
		returnValue.a[i] = a[i] * scalar;
	}

	return returnValue;
}

Mtx43 Mtx43::operator+(const Mtx43& rhs) const
{
	Mtx43	returnValue;
	for(int i = 0; i < kElementCount; i++)
	{
		returnValue.a[i] = a[i] + rhs.a[i];
	}

	return returnValue;
}

void Mtx43::operator+=(const Mtx43& rhs)
{
	for(int i = 0; i < kElementCount; i++)
	{
		a[i] += rhs.a[i];
	}
}

void Mtx43::WriteToString(char* outputBuffer)
{
	sprintf(outputBuffer, "\n%f, %f, %f\n%f, %f, %f\n%f, %f, %f\n%f, %f, %f\n", 
		a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11] );
}
