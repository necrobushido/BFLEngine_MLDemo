#pragma once

#include "Vector3.h"

class Plane
{
public:
	f32 SignedDistanceToPoint(const Vector3 &point) const
	{
		return normal.DotProduct(point) + d;
	}

	void ComputeD(const Vector3 &pointOnPlane)
	{
		d = -normal.DotProduct(pointOnPlane);
	}

public:
	Vector3	normal;
	f32		d;
};

class Frustum
{
public:
	enum
	{
		kNearPlane,
		kFarPlane,
		kTopPlane,
		kBottomPlane,
		kRightPlane,
		kLeftPlane,

		kPlaneCount
	};

public:
	void Init(f32 fov, f32 aspectRatio, f32 zNear, f32 zFar);
	void Init(f32 leftX, f32 rightX, f32 top, f32 bottom, f32 zNear, f32 zFar);
	bool PointInside(const Vector3 &point);
	bool SphereInside(const Vector3 &point, f32 radius) const;
	bool BoxInside(const Vector3 &maxs, const Vector3 &mins) const;

public:
	Plane	planes[kPlaneCount];
	Vector3	cornerPoints[2][2][2];	//	near/far//left/right//bottom/top
	Vector3	nlbCorner;	//	near/left/bottom
	Vector3	frtCorner;	//	far/right/top
};