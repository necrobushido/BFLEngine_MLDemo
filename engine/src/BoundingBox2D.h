#pragma once

#include "Vector2.h"

class BoundingBox2D
{
public:
	BoundingBox2D();

public:
	void AddPoint(const Vector2& point);
	bool IsValid();
	void Scale(f32 scalar);
	bool ContainsPoint(const Vector2& point);
	Vector2 ClampPoint(const Vector2& point);
	void Translate(const Vector2& translation);
	bool Overlaps(const BoundingBox2D& box2);

public:
	Vector2	min;
	Vector2	max;
};