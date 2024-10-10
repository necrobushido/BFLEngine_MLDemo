#pragma once

#include "Vector3.h"

class Mtx33;

class Quat
{
public:
	Quat();
	Quat(coord_type n, coord_type a, coord_type b, coord_type c);
	Quat(const coord_type* from);
	//	angles are in radians
	Quat(coord_type angle, const Vector3& normalizedDir);
	Quat(coord_type angleX, coord_type angleY, coord_type angleZ);
	Quat(const Mtx33& rotationMatrix);

	Vector3 GetVector();
	coord_type GetScalar();

	coord_type Magnitude();
	void Normalize();
	void Negate();
	void NegateIfWNegative();

	void Convert(Mtx33* dst) const;

	static void Add(const Quat& a, const Quat& b, Quat* dst);
	static void Multiply(const Quat& a, const Quat& b, Quat* dst);
	static void Multiply(const Quat& src, const coord_type scalar, Quat* dst);
	static void Lerp(const Quat& start, const Quat& end, coord_type t, Quat* dst);
	static void Slerp(const Quat& start, const Quat& end, coord_type t, Quat* dst);
	static void Conjugate(const Quat& src, Quat* dst);

	static void Transform(const Quat& parentWorld, const Quat& childLocal, Quat* childWorld);
	static void InvTransformRight(const Quat& childWorld, const Quat& childLocal, Quat* parentWorld);
	static void InvTransformLeft(const Quat& childWorld, const Quat& parentWorld, Quat* childLocal);

	Quat operator+(const Quat& rhs) const;
	Quat operator*(const Quat& rhs) const;
	Quat operator*(const coord_type& rhs) const;
	Quat operator*=(const coord_type& rhs);

	enum
	{
		kElementCount = 4
	};

	union
	{
		struct
		{
			coord_type	w;
			coord_type	x;
			coord_type	y;
			coord_type	z;
		};
		coord_type	a[kElementCount];
	};

	static const Quat	IDENTITY;
};

#include "Quat.inl"