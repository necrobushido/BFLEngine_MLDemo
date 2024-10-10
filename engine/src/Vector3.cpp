#include "Vector3.h"

#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

const Vector3	Vector3::X_AXIS(1, 0, 0); 
const Vector3	Vector3::Y_AXIS(0, 1, 0); 
const Vector3	Vector3::Z_AXIS(0, 0, 1);
const Vector3	Vector3::ZERO(0, 0, 0);
const Vector3	Vector3::ONE(1, 1, 1);

Vector3::Vector3(coord_type a, coord_type b, coord_type c) :
	x( a ),
	y( b ),
	z( c )
{
}

Vector3::Vector3(const coord_type* from)
{
	memcpy(a, from, sizeof(a));
}

const coord_type& Vector3::operator[](u8 column) const
{
	assert( column < 3 );
	return a[column];
}

coord_type& Vector3::operator[](u8 column)
{
	assert( column < 3 );
	return a[column];
}

Vector3& Vector3::operator=(const Vector3& v)
{
	x = v.x;
	y = v.y;
	z = v.z;

	return *this;
}

Vector3 Vector3::operator+(const Vector3& v) const
{
	return Vector3(x + v.x, y + v.y, z + v.z);
}

Vector3 Vector3::operator-(const Vector3& v) const
{
	return Vector3(x - v.x, y - v.y, z - v.z);
}

Vector3 Vector3::operator*(coord_type scalar) const
{
	return Vector3(x * scalar, y * scalar, z * scalar);
}

void Vector3::operator+=(coord_type scalar)
{
	x += scalar;
	y += scalar;
	z += scalar;
}

void Vector3::operator+=(const Vector3 &v)
{
	x += v.x;
	y += v.y;
	z += v.z;
}

void Vector3::operator-=(coord_type scalar)
{
	x -= scalar;
	y -= scalar;
	z -= scalar;
}

void Vector3::operator-=(const Vector3 &v)
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
}

void Vector3::operator*=(coord_type scalar)
{
	x *= scalar;
	y *= scalar;
	z *= scalar;
}

void Vector3::operator/=(coord_type divisor)
{
	coord_type	denom = 1 / divisor;

	x *= denom;
	y *= denom;
	z *= denom;
}

void Vector3::operator/=(const Vector3& v)
{
	x /= v.x;
	y /= v.y;
	z /= v.z;
}

Vector3& Vector3::Scale(coord_type s)
{
	x *= s;
	y *= s;
	z *= s;

	return *this;
}

coord_type Vector3::LengthSquared() const
{
	return DotProduct(*this);
}

coord_type Vector3::Length() const
{
	return (coord_type)sqrt(LengthSquared());
}

void Vector3::Normalize()
{
	coord_type	len = Length();

	assert( len > 0 );

	coord_type	denom = 1 / len;

	x *= denom;
	y *= denom;
	z *= denom;
}

bool Vector3::IsZero() const
{
	return ( x == 0 && y == 0 && z == 0 );
}

coord_type Vector3::DotProduct(const Vector3& v) const
{
	return x * v.x + y * v.y + z * v.z;
}

Vector3 Vector3::CrossProduct(const Vector3& v) const
{
	return ( Vector3( (y * v.z - z * v.y), (z * v.x - x * v.z), (x * v.y - y * v.x) ) ); 
}

void Vector3::SetFromString(const char* string)
{
	const char	*firstNumberStart = string;
	const char	*firstComma = strchr(firstNumberStart, ',');
	const char	*secondNumberStart = &firstComma[1];
	const char	*secondComma = strchr(secondNumberStart, ',');	
	const char	*thirdNumberStart = &secondComma[1];

	char		number[256];
	u64			numCharsInFloat;
	numCharsInFloat = firstComma - firstNumberStart;
	strncpy(number, firstNumberStart, numCharsInFloat);
	number[numCharsInFloat] = '\0';
	x = (f32)atof(number);

	numCharsInFloat = secondComma - secondNumberStart;
	strncpy(number, secondNumberStart, numCharsInFloat);
	number[numCharsInFloat] = '\0';
	y = (f32)atof(number);

	strcpy(number, thirdNumberStart);
	z = (f32)atof(number);
}

void Vector3::WriteToString(char* outputBuffer)
{
	sprintf(outputBuffer, "%f, %f, %f", x, y, z);
}

Vector3 Vector3::RotateXY(coord_type radians) const
{
	Vector3	returnValue;
	returnValue.x = (coord_type)(x * cos(radians) + y * sin(radians));
	returnValue.y = (coord_type)(-x * sin(radians) + y * cos(radians));
	returnValue.z = z;

	return returnValue;
}

Vector3 Vector3::RotateXZ(coord_type radians) const
{
	Vector3	returnValue;
	returnValue.x = (coord_type)(x * cos(radians) + z * sin(radians));
	returnValue.y = y;
	returnValue.z = (coord_type)(-x * sin(radians) + z * cos(radians));

	return returnValue;
}
