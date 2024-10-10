#pragma once

#include "Vector3.h"

class ModelData;

class BoundingBox3D
{
public:
	BoundingBox3D();

public:
	void AddPoint(const Vector3& point);
	bool ContainsPoint(const Vector3& point, f32 epsilon = 0.01f);
	bool IsValid();
	void SetFromModel(const ModelData& modelData);
	void Scale(f32 scalar);
	void Draw(bool fullLines = false);
	bool IntersectsBox(const BoundingBox3D& otherBox);

public:
	Vector3	min;
	Vector3	max;
};