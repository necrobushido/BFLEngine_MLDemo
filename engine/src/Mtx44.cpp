#include "Mtx44.h"
#include "Vector3.h"

#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#ifndef BFL_TOOLS
//#include "btBulletDynamicsCommon.h"

const f32	kIdentityFloats[] =
{
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
};

const Mtx44	Mtx44::IDENTITY(kIdentityFloats);

//Mtx44::Mtx44(const btTransform *from)
//{
//	from->getOpenGLMatrix(a);
//	/*const btMatrix3x3	&fromOrient = from->getBasis();
//	const btVector3		&fromPos = from->getOrigin();
//
//	for(int i = 0; i < 3; i++)
//	{
//		for(int j = 0; j < 3; j++)
//		{
//			m[i][j] = fromOrient[j].m_floats[i];
//		}
//	}
//
//	for(int j = 0; j < 3; j++)
//	{
//		m[3][j] = fromPos.m_floats[j];
//	}
//
//	m[0][3] = 0.0f;
//	m[1][3] = 0.0f;
//	m[2][3] = 0.0f;
//	m[3][3] = 1.0f;*/
//}

Mtx44::Mtx44(const coord_type *from)
{
	memcpy(a, from, sizeof(a));
}

//void Mtx44::AssignToBTTransform(btTransform &btT) const
//{
//	btT.setFromOpenGLMatrix(a);
//}
#endif

void Mtx44::Identity()
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

	m[0][3] = 0;
	m[1][3] = 0;
	m[2][3] = 0;
	m[3][3] = 1;
}

void Mtx44::Transpose( Mtx44 &out ) const
{
	out.m[0][0] = m[0][0];		
	out.m[0][1] = m[1][0];		
	out.m[0][2] = m[2][0];
	out.m[0][3] = m[3][0];

	out.m[1][0] = m[0][1];		
	out.m[1][1] = m[1][1];		
	out.m[1][2] = m[2][1];
	out.m[1][3] = m[3][1];

	out.m[2][0] = m[0][2];		
	out.m[2][1] = m[1][2];		
	out.m[2][2] = m[2][2];
	out.m[2][3] = m[3][2];

	out.m[3][0] = m[0][3];		
	out.m[3][1] = m[1][3];		
	out.m[3][2] = m[2][3];		
	out.m[3][3] = m[3][3];		
}

void Mtx44::Inverse( Mtx44& out ) const
{
	coord_type	inv[16];
	coord_type	determinant;

    inv[0] = a[5]  * a[10] * a[15] - 
             a[5]  * a[11] * a[14] - 
             a[9]  * a[6]  * a[15] + 
             a[9]  * a[7]  * a[14] +
             a[13] * a[6]  * a[11] - 
             a[13] * a[7]  * a[10];

    inv[4] = -a[4]  * a[10] * a[15] + 
              a[4]  * a[11] * a[14] + 
              a[8]  * a[6]  * a[15] - 
              a[8]  * a[7]  * a[14] - 
              a[12] * a[6]  * a[11] + 
              a[12] * a[7]  * a[10];

    inv[8] = a[4]  * a[9] * a[15] - 
             a[4]  * a[11] * a[13] - 
             a[8]  * a[5] * a[15] + 
             a[8]  * a[7] * a[13] + 
             a[12] * a[5] * a[11] - 
             a[12] * a[7] * a[9];

    inv[12] = -a[4]  * a[9] * a[14] + 
               a[4]  * a[10] * a[13] +
               a[8]  * a[5] * a[14] - 
               a[8]  * a[6] * a[13] - 
               a[12] * a[5] * a[10] + 
               a[12] * a[6] * a[9];

    inv[1] = -a[1]  * a[10] * a[15] + 
              a[1]  * a[11] * a[14] + 
              a[9]  * a[2] * a[15] - 
              a[9]  * a[3] * a[14] - 
              a[13] * a[2] * a[11] + 
              a[13] * a[3] * a[10];

    inv[5] = a[0]  * a[10] * a[15] - 
             a[0]  * a[11] * a[14] - 
             a[8]  * a[2] * a[15] + 
             a[8]  * a[3] * a[14] + 
             a[12] * a[2] * a[11] - 
             a[12] * a[3] * a[10];

    inv[9] = -a[0]  * a[9] * a[15] + 
              a[0]  * a[11] * a[13] + 
              a[8]  * a[1] * a[15] - 
              a[8]  * a[3] * a[13] - 
              a[12] * a[1] * a[11] + 
              a[12] * a[3] * a[9];

    inv[13] = a[0]  * a[9] * a[14] - 
              a[0]  * a[10] * a[13] - 
              a[8]  * a[1] * a[14] + 
              a[8]  * a[2] * a[13] + 
              a[12] * a[1] * a[10] - 
              a[12] * a[2] * a[9];

    inv[2] = a[1]  * a[6] * a[15] - 
             a[1]  * a[7] * a[14] - 
             a[5]  * a[2] * a[15] + 
             a[5]  * a[3] * a[14] + 
             a[13] * a[2] * a[7] - 
             a[13] * a[3] * a[6];

    inv[6] = -a[0]  * a[6] * a[15] + 
              a[0]  * a[7] * a[14] + 
              a[4]  * a[2] * a[15] - 
              a[4]  * a[3] * a[14] - 
              a[12] * a[2] * a[7] + 
              a[12] * a[3] * a[6];

    inv[10] = a[0]  * a[5] * a[15] - 
              a[0]  * a[7] * a[13] - 
              a[4]  * a[1] * a[15] + 
              a[4]  * a[3] * a[13] + 
              a[12] * a[1] * a[7] - 
              a[12] * a[3] * a[5];

    inv[14] = -a[0]  * a[5] * a[14] + 
               a[0]  * a[6] * a[13] + 
               a[4]  * a[1] * a[14] - 
               a[4]  * a[2] * a[13] - 
               a[12] * a[1] * a[6] + 
               a[12] * a[2] * a[5];

    inv[3] = -a[1] * a[6] * a[11] + 
              a[1] * a[7] * a[10] + 
              a[5] * a[2] * a[11] - 
              a[5] * a[3] * a[10] - 
              a[9] * a[2] * a[7] + 
              a[9] * a[3] * a[6];

    inv[7] = a[0] * a[6] * a[11] - 
             a[0] * a[7] * a[10] - 
             a[4] * a[2] * a[11] + 
             a[4] * a[3] * a[10] + 
             a[8] * a[2] * a[7] - 
             a[8] * a[3] * a[6];

    inv[11] = -a[0] * a[5] * a[11] + 
               a[0] * a[7] * a[9] + 
               a[4] * a[1] * a[11] - 
               a[4] * a[3] * a[9] - 
               a[8] * a[1] * a[7] + 
               a[8] * a[3] * a[5];

    inv[15] = a[0] * a[5] * a[10] - 
              a[0] * a[6] * a[9] - 
              a[4] * a[1] * a[10] + 
              a[4] * a[2] * a[9] + 
              a[8] * a[1] * a[6] - 
              a[8] * a[2] * a[5];

    determinant = a[0] * inv[0] + a[1] * inv[4] + a[2] * inv[8] + a[3] * inv[12];
	assert(determinant != 0.0f);

    if( determinant != 0 )
	{
		determinant = 1.0f / determinant;

		for(u32 i = 0; i < 16; i++)
		{
			out.a[i] = inv[i] * determinant;
		}
	}
}

void Mtx44::Inverse43( Mtx44& out ) const
{
	//	not sure if this function is correct, need to test
	Vector3	translation = GetTranslation3();
	translation *= -1.0f;

	Mtx33	temp33 = ExtractMtx33();
	temp33 = temp33.Inverse();
	out.SetFromMtx33(temp33);

	translation = temp33.MultiplyVec(translation);
	out.SetTranslation3(translation);
}

void Mtx44::Multiply(const Mtx44 &rhs, Mtx44 &out) const
{
	out.m[0][0] = m[0][0]*rhs.m[0][0] + m[0][1]*rhs.m[1][0] + m[0][2]*rhs.m[2][0] + m[0][3]*rhs.m[3][0];
	out.m[0][1] = m[0][0]*rhs.m[0][1] + m[0][1]*rhs.m[1][1] + m[0][2]*rhs.m[2][1] + m[0][3]*rhs.m[3][1];
	out.m[0][2] = m[0][0]*rhs.m[0][2] + m[0][1]*rhs.m[1][2] + m[0][2]*rhs.m[2][2] + m[0][3]*rhs.m[3][2];
	out.m[0][3] = m[0][0]*rhs.m[0][3] + m[0][1]*rhs.m[1][3] + m[0][2]*rhs.m[2][3] + m[0][3]*rhs.m[3][3];

	out.m[1][0] = m[1][0]*rhs.m[0][0] + m[1][1]*rhs.m[1][0] + m[1][2]*rhs.m[2][0] + m[1][3]*rhs.m[3][0];
	out.m[1][1] = m[1][0]*rhs.m[0][1] + m[1][1]*rhs.m[1][1] + m[1][2]*rhs.m[2][1] + m[1][3]*rhs.m[3][1];
	out.m[1][2] = m[1][0]*rhs.m[0][2] + m[1][1]*rhs.m[1][2] + m[1][2]*rhs.m[2][2] + m[1][3]*rhs.m[3][2];
	out.m[1][3] = m[1][0]*rhs.m[0][3] + m[1][1]*rhs.m[1][3] + m[1][2]*rhs.m[2][3] + m[1][3]*rhs.m[3][3];
	
	out.m[2][0] = m[2][0]*rhs.m[0][0] + m[2][1]*rhs.m[1][0] + m[2][2]*rhs.m[2][0] + m[2][3]*rhs.m[3][0];
	out.m[2][1] = m[2][0]*rhs.m[0][1] + m[2][1]*rhs.m[1][1] + m[2][2]*rhs.m[2][1] + m[2][3]*rhs.m[3][1];
	out.m[2][2] = m[2][0]*rhs.m[0][2] + m[2][1]*rhs.m[1][2] + m[2][2]*rhs.m[2][2] + m[2][3]*rhs.m[3][2];
	out.m[2][3] = m[2][0]*rhs.m[0][3] + m[2][1]*rhs.m[1][3] + m[2][2]*rhs.m[2][3] + m[2][3]*rhs.m[3][3];

	out.m[3][0] = m[3][0]*rhs.m[0][0] + m[3][1]*rhs.m[1][0] + m[3][2]*rhs.m[2][0] + m[3][3]*rhs.m[3][0];
	out.m[3][1] = m[3][0]*rhs.m[0][1] + m[3][1]*rhs.m[1][1] + m[3][2]*rhs.m[2][1] + m[3][3]*rhs.m[3][1];
	out.m[3][2] = m[3][0]*rhs.m[0][2] + m[3][1]*rhs.m[1][2] + m[3][2]*rhs.m[2][2] + m[3][3]*rhs.m[3][2];
	out.m[3][3] = m[3][0]*rhs.m[0][3] + m[3][1]*rhs.m[1][3] + m[3][2]*rhs.m[2][3] + m[3][3]*rhs.m[3][3];
}

void Mtx44::MultiplyVec43( const Vector3 &vec, Vector3 &out, bool translate ) const
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

void Mtx44::MultiplyVec(const Vector3 &vec, Vector3 &out) const
{
	out.x = m[0][0]*vec.x + m[1][0]*vec.y + m[2][0]*vec.z + m[3][0];
    out.y = m[0][1]*vec.x + m[1][1]*vec.y + m[2][1]*vec.z + m[3][1];
    out.z = m[0][2]*vec.x + m[1][2]*vec.y + m[2][2]*vec.z + m[3][2];
	f32	w = m[0][3]*vec.x + m[1][3]*vec.y + m[2][3]*vec.z + m[3][3];
	if( w != 0.0f )
	{
		out /= w;
	}
}

void Mtx44::ConstructPitchRotation(f32 pitch)
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

	m[0][3] = 0;
	m[1][3] = 0;
	m[2][3] = 0;
	m[3][3] = 1;
}

void Mtx44::ConstructYawRotation(f32 yaw)
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

	m[0][3] = 0;
	m[1][3] = 0;
	m[2][3] = 0;
	m[3][3] = 1;
}

void Mtx44::ConstructRollRotation(f32 roll)
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

	m[0][3] = 0;
	m[1][3] = 0;
	m[2][3] = 0;
	m[3][3] = 1;
}

void Mtx44::ConstructTransform(f32 pitch, f32 yaw, f32 roll, Vector3 translation)
{
	Mtx44	pitchMtx;
	Mtx44	yawMtx;
	Mtx44	rollMtx;

	pitchMtx.ConstructPitchRotation(pitch);
	yawMtx.ConstructYawRotation(yaw);
	rollMtx.ConstructRollRotation(roll);

	*this = pitchMtx * yawMtx * rollMtx;
	SetTranslation3(translation);
}

const Vector3 &Mtx44::GetTranslation3() const
{
	const f32		*start = &_30;
	const Vector3	*pVec = (Vector3*)start;
	return *pVec;
}

Mtx33 Mtx44::ExtractMtx33() const
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

void Mtx44::SetFromMtx33(const Mtx33 &from)
{
	_00 = from._00;
	_01 = from._01;
	_02 = from._02;
	_03 = 0.0f;
	_10 = from._10;
	_11 = from._11;
	_12 = from._12;
	_13 = 0.0f;
	_20 = from._20;
	_21 = from._21;
	_22 = from._22;
	_23 = 0.0f;
	_30 = 0.0f;
	_31 = 0.0f;
	_32 = 0.0f;
	_33 = 1.0f;
}

Mtx43 Mtx44::ExtractMtx43() const
{
	Mtx43	returnValue;

	returnValue._00 = _00;
	returnValue._01 = _01;
	returnValue._02 = _02;
	returnValue._10 = _10;
	returnValue._11 = _11;
	returnValue._12 = _12;
	returnValue._20 = _20;
	returnValue._21 = _21;
	returnValue._22 = _22;
	returnValue._30 = _30;
	returnValue._31 = _31;
	returnValue._32 = _32;

	return returnValue;
}

void Mtx44::SetFromMtx43(const Mtx43& from)
{
	_00 = from._00;
	_01 = from._01;
	_02 = from._02;
	_03 = 0.0f;
	_10 = from._10;
	_11 = from._11;
	_12 = from._12;
	_13 = 0.0f;
	_20 = from._20;
	_21 = from._21;
	_22 = from._22;
	_23 = 0.0f;
	_30 = from._30;
	_31 = from._31;
	_32 = from._32;
	_33 = 1.0f;
}

void Mtx44::Make43Like()
{	
	const coord_type	kEpsilon = 0.00001f;
	if( fabs(_33) > kEpsilon )
	{
		//	make the last row have 4th element = 1 if possible
		_30 /= _33;
		_31 /= _33;
		_32 /= _33;
		_33 /= _33;

		//	use the last row to eliminate the 4th element of the rest of the rows
		for(int i = 0; i < 2; ++i)
		{
			const coord_type	rowMult = m[i][3];

			for(int j = 0; j < 2; ++j)
			{
				m[i][j] -= m[3][j] * rowMult;
			}

			m[i][3] = 0.0f;
		}
	}
}

Mtx44 Mtx44::OrthoNormalInvert()
{
	Mtx44	result;

	Mtx33	rotate33 = ExtractMtx33();
	Mtx33	invRotate33;
	rotate33.Transpose(invRotate33);
	result.SetFromMtx33(invRotate33);

	Vector3	pos = GetTranslation3();
	pos = result.MultiplyVec43(pos, false);
	pos *= -1.0f;
	result.SetTranslation3(pos);

	return result;
}

Mtx33 Mtx44::ExtractOrthoNormalMtx33() const
{
	Mtx33	result = ExtractMtx33();
	result.ResetScale();

	return result;
}

bool Mtx44::IsEqualTo(const Mtx44 &ref, f32 tolerance) const
{
	bool	returnValue = true;
	for(int i = 0; i < 16 && returnValue; i++)
	{
		returnValue = fabs(a[i] - ref.a[i]) <= tolerance;
	}

	return returnValue;
}

bool Mtx44::operator==(const Mtx44 &ref) const
{
	bool	returnValue = true;
	for(int i = 0; i < 16 && returnValue; i++)
	{
		returnValue = a[i] == ref.a[i];
	}

	return returnValue;
}

Mtx44 Mtx44::operator*(const f32 &scalar) const
{
	Mtx44	returnValue;
	for(int i = 0; i < 16; i++)
	{
		returnValue.a[i] = a[i] * scalar;
	}

	return returnValue;
}

Mtx44 Mtx44::operator+(const Mtx44 &rhs) const
{
	Mtx44	returnValue;
	for(int i = 0; i < 16; i++)
	{
		returnValue.a[i] = a[i] + rhs.a[i];
	}

	return returnValue;
}

void Mtx44::operator+=(const Mtx44 &rhs)
{
	for(int i = 0; i < 16; i++)
	{
		a[i] += rhs.a[i];
	}
}

Mtx44 Mtx44::operator-(const Mtx44 &rhs) const
{
	Mtx44	returnValue;
	for(int i = 0; i < 16; i++)
	{
		returnValue.a[i] = a[i] - rhs.a[i];
	}

	return returnValue;
}

void Mtx44::operator-=(const Mtx44 &rhs)
{
	for(int i = 0; i < 16; i++)
	{
		a[i] -= rhs.a[i];
	}
}

void Mtx44::WriteToString(char* outputBuffer)
{
	sprintf(outputBuffer, "\n%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n", 
		a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15] );
}
