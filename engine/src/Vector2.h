#pragma once

#include "dataTypes.h"

class Vector2
{
public:
	Vector2();
	Vector2( coord_type a, coord_type b);

public:
	coord_type			&operator[]( u8 column );
	const coord_type	&operator[]( u8 column ) const;
	Vector2				&operator=( const Vector2 &v );
	Vector2				operator+( const Vector2 &v ) const;
	Vector2				operator-( const Vector2 &v ) const;
	Vector2				operator*( f32 scalar ) const;
	void				operator+=( const Vector2 &v );
	void				operator-=( const Vector2 &v );
	void				operator*=( coord_type scalar );
	void				operator/=( coord_type scalar );

	Vector2		&Scale( coord_type s );
	coord_type	LengthSquared() const;
	coord_type	Length() const;
	void		Normalize();
	bool		IsZero();
	Vector2		Perp();
	Vector2		Rotate( coord_type radians ) const;
	coord_type	ToAngle() const;

	coord_type	DotProduct( const Vector2 &v ) const;

public:
	union
	{
		struct
		{
			coord_type	x;
			coord_type	y;
		};
		struct
		{
			coord_type	u;
			coord_type	v;
		};
		coord_type	a[2];
	};

public:
	static const Vector2	X_AXIS; 
	static const Vector2	Y_AXIS;
	static const Vector2	ZERO; 
};
