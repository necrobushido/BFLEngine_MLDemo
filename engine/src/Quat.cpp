#include "Quat.h"

#include "Mtx33.h"

#include <math.h>
#include <stdio.h>

const Quat	Quat::IDENTITY(1, 0, 0, 0);

Quat::Quat(const coord_type* from)
{
	memcpy(a, from, sizeof(a));
}

Quat::Quat(coord_type angle, const Vector3& normalizedDir)
{
	coord_type	cosHalf = cosf( angle / 2.0f );
	coord_type	sinHalf = sinf( angle / 2.0f );

	w = cosHalf;
	x = normalizedDir.x * sinHalf;
	y = normalizedDir.y * sinHalf;
	z = normalizedDir.z * sinHalf;
}

//Quat::Quat(const Mtx33& rotationMatrix)
//{
//	coord_type	trace = rotationMatrix.m[0][0] + rotationMatrix.m[1][1] + rotationMatrix.m[2][2];
//    coord_type	s;
//
//    if( trace > 0.0 )
//	{
//        s = 0.5f / sqrtf(trace + 1.0f);
//		w = 0.25f / s;
//		x = (rotationMatrix.m[2][1] - rotationMatrix.m[1][2]) * s;
//		y = (rotationMatrix.m[0][2] - rotationMatrix.m[2][0]) * s;
//		z = (rotationMatrix.m[1][0] - rotationMatrix.m[0][1]) * s;
//    } 
//	else if( rotationMatrix.m[0][0] > rotationMatrix.m[1][1] && rotationMatrix.m[0][0] > rotationMatrix.m[2][2] )
//	{
//        s = 2.0f * sqrtf(1.0f + rotationMatrix.m[0][0] - rotationMatrix.m[1][1] - rotationMatrix.m[2][2]);
//		w = (rotationMatrix.m[2][1] - rotationMatrix.m[1][2]) / s;
//		x = 0.25f * s;
//		y = (rotationMatrix.m[0][1] + rotationMatrix.m[1][0]) / s;
//		z = (rotationMatrix.m[0][2] + rotationMatrix.m[2][0]) / s;
//    } 
//	else if( rotationMatrix.m[1][1] > rotationMatrix.m[2][2] )
//	{
//        s = 2.0f * sqrtf(1.0f + rotationMatrix.m[1][1] - rotationMatrix.m[0][0] - rotationMatrix.m[2][2]);
//		w = (rotationMatrix.m[0][2] - rotationMatrix.m[2][0]) / s;
//		x = (rotationMatrix.m[0][1] + rotationMatrix.m[1][0]) / s;
//		y = 0.25f * s;
//		z = (rotationMatrix.m[1][2] + rotationMatrix.m[2][1]) / s;
//    }
//	else 
//	{
//        s = 2.0f * sqrtf(1.0f + rotationMatrix.m[2][2] - rotationMatrix.m[0][0] - rotationMatrix.m[1][1]);
//		w = (rotationMatrix.m[1][0] - rotationMatrix.m[0][1]) / s;
//		x = (rotationMatrix.m[0][2] + rotationMatrix.m[2][0]) / s;
//		y = (rotationMatrix.m[1][2] + rotationMatrix.m[2][1]) / s;
//		z = 0.25f * s;
//    }
//
//	Normalize();
//}

Quat::Quat(const Mtx33& rotationMatrix)
{
	coord_type	s = 0.0f;
	coord_type	q[4] = {0.0f};
	coord_type	trace = rotationMatrix.m[0][0] + rotationMatrix.m[1][1] + rotationMatrix.m[2][2];

	if( trace > 0.0f )
	{
		s = sqrtf(trace + 1.0f);
		
		q[3] = s * 0.5f;

		s = 0.5f / s;
		q[0] = (rotationMatrix.m[1][2] - rotationMatrix.m[2][1]) * s;
		q[1] = (rotationMatrix.m[2][0] - rotationMatrix.m[0][2]) * s;
		q[2] = (rotationMatrix.m[0][1] - rotationMatrix.m[1][0]) * s;
	}
	else
	{
		int nxt[3] = {1, 2, 0};
		int i = 0, j = 0, k = 0;

		if( rotationMatrix.m[1][1] > rotationMatrix.m[0][0] )
		{
			i = 1;
		}

		if( rotationMatrix.m[2][2] > rotationMatrix.m[i][i] )
		{
			i = 2;
		}

		j = nxt[i];
		k = nxt[j];
		s = sqrtf((rotationMatrix.m[i][i] - (rotationMatrix.m[j][j] + rotationMatrix.m[k][k])) + 1.0f);

		q[i] = s * 0.5f;
		s = 0.5f / s;
		q[3] = (rotationMatrix.m[j][k] - rotationMatrix.m[k][j]) * s;
		q[j] = (rotationMatrix.m[i][j] + rotationMatrix.m[j][i]) * s;
		q[k] = (rotationMatrix.m[i][k] + rotationMatrix.m[k][i]) * s;
	}

	x = q[0];
	y = q[1];
	z = q[2];
	w = q[3];

	Normalize();

	/*Mtx33	test;
	Convert(&test);

	printf("hah");*/
}

coord_type Quat::Magnitude()
{
	coord_type	m = (w * w) + (x * x) + (y * y) + (z * z);
	m = sqrtf(m);
	return m;
}

//void Quat::Convert(Mtx33* dst) const
//{
//	coord_type	xS = 2 * (x * x);
//	coord_type	yS = 2 * (y * y);
//	coord_type	zS = 2 * (z * z);
//
//	coord_type	nx = 2 * (w * x);
//	coord_type	ny = 2 * (w * y);
//	coord_type	nz = 2 * (w * z);
//
//	coord_type	xy = 2 * (x * y);
//	coord_type	xz = 2 * (x * z);
//	
//	coord_type	yz = 2 * (y * z);
//
//	dst->_00 = (1 - yS - zS);
//	dst->_01 = (xy - nz);
//	dst->_02 = (xz + ny);
//
//	dst->_10 = (xy + nz);
//	dst->_11 = (1 - xS - zS);
//	dst->_12 = (yz - nx);
//
//	dst->_20 = (xz - ny);
//	dst->_21 = (yz + nx);
//	dst->_22 = (1 - xS - yS);
//}

void Quat::Convert(Mtx33* dst) const
{
	coord_type	xS = 2 * (x * x);
	coord_type	yS = 2 * (y * y);
	coord_type	zS = 2 * (z * z);

	coord_type	nx = 2 * (w * x);
	coord_type	ny = 2 * (w * y);
	coord_type	nz = 2 * (w * z);

	coord_type	xy = 2 * (x * y);
	coord_type	xz = 2 * (x * z);
	
	coord_type	yz = 2 * (y * z);

	dst->_00 = (1 - yS - zS);
	dst->_01 = (xy + nz);
	dst->_02 = (xz - ny);

	dst->_10 = (xy - nz);
	dst->_11 = (1 - xS - zS);
	dst->_12 = (yz + nx);

	dst->_20 = (xz + ny);
	dst->_21 = (yz - nx);	
	dst->_22 = (1 - xS - yS);
}

void Quat::Lerp(const Quat& start, const Quat& end, coord_type t, Quat* dst)
{
	*dst = start * (1.0f - t) + end * t;
}

void Quat::Slerp(const Quat& start, const Quat& end, coord_type t, Quat* dst)
{
	coord_type	dot = start.x * end.x + start.y * end.y + start.z * end.z + start.w * end.w;
	coord_type	sign = 1.0f;
	if( dot < 0.0f )
	{
		dot = -dot;
		sign = -1.0f;
	}

	coord_type	a;
	coord_type	b;
	const coord_type	kEpsilon = 0.001f;
	if( dot <= 1.0f - kEpsilon )
	{
		coord_type	angle = acosf(dot);
		coord_type	c = 1.0f / sinf(angle);
		a = sinf((1.0f - t) * angle) * c;
		b = sinf(angle * t) * c;
	}
	else
	{
		a = 1.0f - t;
		b = t;
	}

	a = a * sign;

	*dst = start * a + end * b;
	dst->Normalize();
}