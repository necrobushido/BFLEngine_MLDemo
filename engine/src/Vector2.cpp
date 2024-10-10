#include "Vector2.h"

#include <assert.h>
#include <math.h>

const Vector2	Vector2::X_AXIS(1, 0); 
const Vector2	Vector2::Y_AXIS(0, 1); 
const Vector2	Vector2::ZERO(0, 0); 

Vector2::Vector2() :
	x( 0 ),
	y( 0 )
{
}

Vector2::Vector2( coord_type a, coord_type b ) :
	x( a ),
	y( b )
{
}

const coord_type &Vector2::operator[]( u8 column ) const
{
	assert( column < 2 );
	return a[column];
}

coord_type &Vector2::operator[]( u8 column )
{
	assert( column < 2 );
	return a[column];
}

Vector2 &Vector2::operator=( const Vector2 &v )
{
	x = v.x;
	y = v.y;

	return *this;
}

Vector2 Vector2::operator+( const Vector2 &v ) const
{
	return Vector2(x + v.x, y + v.y);
}

Vector2 Vector2::operator-( const Vector2 &v ) const
{
	return Vector2(x - v.x, y - v.y);
}

Vector2 Vector2::operator*( f32 scalar ) const
{
	return Vector2(x * scalar, y * scalar);
}

void Vector2::operator+=( const Vector2 &v )
{
	x += v.x;
	y += v.y;
}

void Vector2::operator-=( const Vector2 &v )
{
	x -= v.x;
	y -= v.y;
}

void Vector2::operator*=( coord_type scalar )
{
	x *= scalar;
	y *= scalar;
}

void Vector2::operator/=( coord_type scalar )
{
	assert(scalar != 0.0f);
	coord_type	denom = 1.0f / scalar;
	x *= denom;
	y *= denom;
}

Vector2 &Vector2::Scale( coord_type s )
{
	x *= s;
	y *= s;

	return *this;
}

coord_type Vector2::LengthSquared() const
{
	return DotProduct(*this);
}

coord_type Vector2::Length() const
{
	return (coord_type)sqrt(LengthSquared());
}

void Vector2::Normalize()
{
	coord_type	len = Length();

	assert( len > 0 );

	coord_type	denom = 1 / len;

	x *= denom;
	y *= denom;
}

bool Vector2::IsZero()
{
	return ( x == 0 && y == 0 );
}

Vector2 Vector2::Perp()
{
	Vector2	returnValue(-y, x);

	return returnValue;
}

Vector2 Vector2::Rotate( coord_type radians ) const
{
	Vector2	returnValue;
	returnValue.x = (coord_type)(x * cos(radians) + y * sin(radians));
	returnValue.y = (coord_type)(-x * sin(radians) + y * cos(radians));

	return returnValue;
}

coord_type Vector2::ToAngle() const
{
	return (coord_type)atan2(y, x);
}

coord_type Vector2::DotProduct( const Vector2 &v ) const
{
	return x * v.x + y * v.y;
}