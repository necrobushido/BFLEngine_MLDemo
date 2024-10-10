#include "Mtx33.h"
#include "Vector3.h"

#include <math.h>
#include <assert.h>

#include "MathConstants.h"

const f32	kIdentityFloats[] =
{
	1.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 1.0f
};

const Mtx33	Mtx33::IDENTITY(kIdentityFloats);

Mtx33::Mtx33(const coord_type* from)
{
	memcpy(a, from, sizeof(a));
}

void Mtx33::Identity()
{
	m[0][0] = 1;
	m[1][0] = 0;
	m[2][0] = 0;

	m[0][1] = 0;
	m[1][1] = 1;
	m[2][1] = 0;

	m[0][2] = 0;
	m[1][2] = 0;
	m[2][2] = 1;
}

void Mtx33::MultiplyVec( const Vector3 &vec, Vector3 &out ) const
{
    out.x = m[0][0]*vec.x + m[1][0]*vec.y + m[2][0]*vec.z;
    out.y = m[0][1]*vec.x + m[1][1]*vec.y + m[2][1]*vec.z;
    out.z = m[0][2]*vec.x + m[1][2]*vec.y + m[2][2]*vec.z;
}

void Mtx33::operator/=( coord_type divisor )
{
	assert(divisor != 0.0f);
	if( divisor == 0.0f )
		return;

	coord_type	denom = 1 / divisor;

	for(int i = 0; i < 9; i++)
	{
		a[i] *= denom;
	}
}

void Mtx33::Transpose( Mtx33& out ) const
{
	out.m[0][0] = m[0][0];		
	out.m[0][1] = m[1][0];		
	out.m[0][2] = m[2][0];

	out.m[1][0] = m[0][1];		
	out.m[1][1] = m[1][1];		
	out.m[1][2] = m[2][1];

	out.m[2][0] = m[0][2];		
	out.m[2][1] = m[1][2];		
	out.m[2][2] = m[2][2];		
}

void Mtx33::Inverse( Mtx33& out ) const
{
	coord_type	determinant =	m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - 
								m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) + 
								m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);

	assert(determinant != 0.0f);
	if( determinant == 0.0f )
		return;

	//	adjugate
	out.m[0][0] = m[1][1] * m[2][2] - m[1][2] * m[2][1];
	out.m[1][0] = m[1][2] * m[2][0] - m[1][0] * m[2][2];
	out.m[2][0] = m[1][0] * m[2][1] - m[1][1] * m[2][0];
		
	out.m[0][1] = m[0][2] * m[2][1] - m[0][1] * m[2][2];
	out.m[1][1] = m[0][0] * m[2][2] - m[0][2] * m[2][0];
	out.m[2][1] = m[0][1] * m[2][0] - m[0][0] * m[2][1];

	out.m[0][2] = m[0][1] * m[1][2] - m[0][2] * m[1][1];
	out.m[1][2] = m[0][2] * m[1][0] - m[0][0] * m[1][2];
	out.m[2][2] = m[0][0] * m[1][1] - m[0][1] * m[1][0];

	//
	out /= determinant;
}

void Mtx33::Multiply(const Mtx33& rhs, Mtx33& out) const
{
	out.m[0][0] = m[0][0]*rhs.m[0][0] + m[0][1]*rhs.m[1][0] + m[0][2]*rhs.m[2][0];
	out.m[0][1] = m[0][0]*rhs.m[0][1] + m[0][1]*rhs.m[1][1] + m[0][2]*rhs.m[2][1];
	out.m[0][2] = m[0][0]*rhs.m[0][2] + m[0][1]*rhs.m[1][2] + m[0][2]*rhs.m[2][2];

	out.m[1][0] = m[1][0]*rhs.m[0][0] + m[1][1]*rhs.m[1][0] + m[1][2]*rhs.m[2][0];
	out.m[1][1] = m[1][0]*rhs.m[0][1] + m[1][1]*rhs.m[1][1] + m[1][2]*rhs.m[2][1];
	out.m[1][2] = m[1][0]*rhs.m[0][2] + m[1][1]*rhs.m[1][2] + m[1][2]*rhs.m[2][2];
	
	out.m[2][0] = m[2][0]*rhs.m[0][0] + m[2][1]*rhs.m[1][0] + m[2][2]*rhs.m[2][0];
	out.m[2][1] = m[2][0]*rhs.m[0][1] + m[2][1]*rhs.m[1][1] + m[2][2]*rhs.m[2][1];
	out.m[2][2] = m[2][0]*rhs.m[0][2] + m[2][1]*rhs.m[1][2] + m[2][2]*rhs.m[2][2];
}

bool Mtx33::HandednessCheck() const
{
	const Vector3&	right = GetRight();
	const Vector3&	up = GetUp();
	const Vector3&	forward = GetForward();

	Vector3	ru = right.CrossProduct(up);

	f32		dot = ru.DotProduct(forward);

	return dot > 0.0f;
}

void Mtx33::ConstructPitchRotation(coord_type pitch)
{
	m[0][0] = 1;
	m[1][0] = 0;
	m[2][0] = 0;

	m[0][1] = 0;
	m[1][1] = (coord_type)cos(pitch);
	m[2][1] = (coord_type)sin(pitch);

	m[0][2] = 0;
	m[1][2] = (coord_type)-sin(pitch);
	m[2][2] = (coord_type)cos(pitch);
}

void Mtx33::ConstructYawRotation(coord_type yaw)
{
	m[0][0] = (coord_type)cos(yaw);
	m[1][0] = 0;
	m[2][0] = (coord_type)-sin(yaw);

	m[0][1] = 0;
	m[1][1] = 1;
	m[2][1] = 0;

	m[0][2] = (coord_type)sin(yaw);
	m[1][2] = 0;
	m[2][2] = (coord_type)cos(yaw);
}

void Mtx33::ConstructRollRotation(coord_type roll)
{
	m[0][0] = (coord_type)cos(roll);
	m[1][0] = (coord_type)sin(roll);
	m[2][0] = 0;

	m[0][1] = (coord_type)-sin(roll);
	m[1][1] = (coord_type)cos(roll);
	m[2][1] = 0;

	m[0][2] = 0;
	m[1][2] = 0;
	m[2][2] = 1;
}

//void Mtx33::ConstructEulerRotation(coord_type pitch, coord_type yaw, coord_type roll)
//{
//	Mtx33	pitchMtx;
//	Mtx33	yawMtx;
//	Mtx33	rollMtx;
//
//	pitchMtx.ConstructPitchRotation(pitch);
//	yawMtx.ConstructYawRotation(yaw);
//	rollMtx.ConstructRollRotation(roll);
//
//	*this = pitchMtx * yawMtx * rollMtx;
//}

void Mtx33::ConstructEulerRotation(coord_type pitch, coord_type yaw, coord_type roll)
{
	const f64	cY = cos((f64)yaw);
	const f64	sY = sin((f64)yaw);
	const f64	cP = cos((f64)pitch);
	const f64	sP = sin((f64)pitch);
	const f64	cR = cos((f64)roll);
	const f64	sR = sin((f64)roll);

	Identity();

	_00 = (coord_type)(cY*cR);
	_10 = (coord_type)(cY*sR);
	_20 = (coord_type)(-sY);
	_01 = (coord_type)(sP*sY*cR + cP*-sR);
	_11 = (coord_type)(sP*sY*sR + cP*cR);
	_21 = (coord_type)(sP*cY);
	_02 = (coord_type)(cP*sY*cR + sP*sR);
	_12 = (coord_type)(cP*sY*sR + -sP*cR);
	_22 = (coord_type)(cP*cY);
}

void Mtx33::ConstructEulerRotation(const Vector3& eulerAngles)
{
	ConstructEulerRotation(eulerAngles[kPitch], eulerAngles[kYaw], eulerAngles[kRoll]);
}

void Mtx33::GetEulerAngles(coord_type* pitchOut, coord_type* yawOut, coord_type* rollOut) const
{
	if( m[2][0] < 1.0f )
	{
		if( m[2][0] > -1.0f )
		{
			*yawOut = -(coord_type)asin(m[2][0]);
			*pitchOut = -(coord_type)(atan2(-m[2][1], m[2][2]));
			*rollOut = -(coord_type)(atan2(-m[1][0], m[0][0]));
		}
		else
		{
			*yawOut = -(coord_type)-M_PI / 2.0f;
			*pitchOut = -(coord_type)-(atan2(m[0][1], m[1][1]));
			*rollOut = -(coord_type)0.0f;
		}
	}
	else
	{
		*yawOut = -(coord_type)M_PI / 2.0f;
		*pitchOut = -(coord_type)(atan2(m[0][1], m[1][1]));
		*rollOut = -(coord_type)0.0f;
	}
}

void Mtx33::GetEulerAngles(Vector3* eulerAnglesOut) const
{
	GetEulerAngles(&(eulerAnglesOut->a[kPitch]), &(eulerAnglesOut->a[kYaw]), &(eulerAnglesOut->a[kRoll]));
}