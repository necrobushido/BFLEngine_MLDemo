#pragma once

#include "dataTypes.h"

class Vector3
{
public:
	Vector3(){}
	Vector3(coord_type a, coord_type b, coord_type c);
	Vector3(const coord_type* from);

public:
	coord_type& operator[](u8 column);
	const coord_type& operator[](u8 column) const;
	Vector3& operator=(const Vector3& v);
	Vector3 operator+(const Vector3& v) const;
	Vector3	operator-(const Vector3& v) const;
	Vector3	operator*(coord_type scalar) const;
	void operator+=(coord_type scalar);
	void operator+=(const Vector3& v);
	void operator-=(coord_type scalar);
	void operator-=(const Vector3& v);
	void operator*=(coord_type scalar);
	void operator/=(coord_type divisor);
	void operator/=(const Vector3& v);

	Vector3& Scale(coord_type s);
	coord_type LengthSquared() const;
	coord_type Length() const;
	void Normalize();
	bool IsZero() const;

	coord_type DotProduct(const Vector3& v) const;
	Vector3 CrossProduct(const Vector3& v) const;

	void SetFromString(const char* string);
	void WriteToString(char* outputBuffer);

	Vector3 RotateXY(coord_type radians) const;
	Vector3 RotateXZ(coord_type radians) const;

public:
	enum
	{
		kElementCount = 3
	};

	union
	{
		struct
		{
			coord_type	x;
			coord_type	y;
			coord_type	z;
		};
		coord_type	a[kElementCount];
	};

public:
	static const Vector3	X_AXIS; 
	static const Vector3	Y_AXIS; 
	static const Vector3	Z_AXIS;
	static const Vector3	ZERO;
	static const Vector3	ONE;
};
