#include "BoundingBox2D.h"

BoundingBox2D::BoundingBox2D()
{
	const f32	kInfinity = 100000000000000000.0f;
	min = Vector2(kInfinity, kInfinity);
	max = Vector2(-kInfinity, -kInfinity);
}

void BoundingBox2D::AddPoint(const Vector2& point)
{
	for(int i = 0; i < 2; i++)
	{
		if( point[i] < min[i] )
		{
			min[i] = point[i];
		}

		if( point[i] > max[i] )
		{
			max[i] = point[i];
		}
	}
}

bool BoundingBox2D::IsValid()
{
	bool	returnValue = true;
	for(int i = 0; i < 2 && returnValue; i++)
	{
		returnValue = min[i] < max[i];
	}

	return returnValue;
}

void BoundingBox2D::Scale(f32 scalar)
{
	min *= scalar;
	max *= scalar;
}

bool BoundingBox2D::ContainsPoint(const Vector2& point)
{
	return	point.x > min.x &&
			point.x < max.x &&
			point.y > min.y &&
			point.y < max.y;
}

Vector2 BoundingBox2D::ClampPoint(const Vector2& point)
{
	Vector2	returnValue = point;

	if( returnValue.x < min.x )
		returnValue.x = min.x;
	if( returnValue.x > max.x )
		returnValue.x = max.x;
	if( returnValue.y < min.y )
		returnValue.y = min.y;
	if( returnValue.y > max.y )
		returnValue.y = max.y;

	return returnValue;
}

void BoundingBox2D::Translate(const Vector2& translation)
{
	min += translation;
	max += translation;
}

bool BoundingBox2D::Overlaps(const BoundingBox2D& box2)
{
	bool	result = false;

	Vector2	xIntervalMinMax;
	xIntervalMinMax[0] = min.x;
	xIntervalMinMax[1] = max.x;

	if( box2.min.x > xIntervalMinMax[0] )
	{
		xIntervalMinMax[0] = box2.min.x;
	}
	if( box2.max.x < xIntervalMinMax[1] )
	{
		xIntervalMinMax[1] = box2.max.x;
	}

	if( xIntervalMinMax[0] <= xIntervalMinMax[1] )
	{
		Vector2	yIntervalMinMax;
		yIntervalMinMax[0] = min.y;
		yIntervalMinMax[1] = max.y;

		if( box2.min.y > yIntervalMinMax[0] )
		{
			yIntervalMinMax[0] = box2.min.y;
		}
		if( box2.max.y < yIntervalMinMax[1] )
		{
			yIntervalMinMax[1] = box2.max.y;
		}

		if( yIntervalMinMax[0] <= yIntervalMinMax[1] )
		{
			result = true;
		}
	}

	return result;
}