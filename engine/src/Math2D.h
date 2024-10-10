#pragma once

#include "Vector2.h"

struct Plane2D	//	a line, lulz
{
	Vector2	normal;
	Vector2	point;
	f32		d;	//	point dot normal; gets used all the time, so cached
				//		not that slow, and processor is fast ... maybe better to remove either d or point?
};

namespace Math
{
	class Circle
	{
	public:
		Vector2	center;
		float	radius;
	};

	class LineSegment
	{
	public:
		Vector2	endPoints[2];
	};

	class Ray
	{
	public:
		Vector2	origin;
		Vector2	direction;
	};

	//
	void ConstructBasisFromOrientation(f32 orientation, Vector2& rightOut, Vector2& upOut);

	//	returns 0 for no intersection, 1 for intersection
	int Test_CircleCircle(const Vector2 &centerA, float radiusA, const Vector2 &centerB, float radiusB);

	//	returns 0 for no intersection, 1 for intersection	
	//	t = distance to travel for depenetration
	int Intersect_CircleCircle(const Vector2 &centerA, float radiusA, const Vector2 &centerB, float radiusB, float &t);

	//	as above, but circle A is relative to Circle B
	int Intersect_CircleCircleRelative(const Vector2 &centerA, float radiusA, float radiusB, float &t);

	//	given point p and line segment (l1, l2), returns closest point on line segment via closestPoint
	void ClosestPoint_PointSegment(const Vector2 &p, const Vector2 &l1, const Vector2 &l2, float &t, Vector2 &closestPoint);

	//	returns 0 for no intersection, 1 for intersection
	//	a ray that starts inside of the circle will be detected as an intersection at the ray origin
	//	this is for a ray with an origin but infinite length
	int Intersect_RayCircle(const Vector2 &rayOrigin, const Vector2 &rayDir, const Vector2 &circleCenter, float circleRadius, float &t, Vector2 &intersectionPoint);

	//	as above, but a ray that starts inside of the circle will return an intersection on the border of the circle
	int Intersect_RayCircleBorder(const Vector2 &rayOrigin, const Vector2 &rayDir, const Vector2 &circleCenter, float circleRadius, float &t, Vector2 &intersectionPoint);

	//	returns 0 for no intersection, 1 for intersection
	//	a ray that starts inside of the circle will be detected as an intersection at the ray origin
	//	this is for a ray with an origin and finite length (directed segment)
	int Intersect_DirectedSegmentCircle(const Vector2 &rayOrigin, const Vector2 &rayEnd, const Vector2 &circleCenter, float circleRadius, float &t, Vector2 &intersectionPoint);

	//	as above, but segment is relative to circle (circle is at origin)
	int Intersect_DirectedSegmentCircleRelative(const Vector2 &rayOrigin, const Vector2 &rayEnd, float circleRadius, float &t, Vector2 &intersectionPoint);

	//	as above, but segment that starts inside of the circle will return an intersection on the border of the circle if one exists, and returns 0 is not
	int Intersect_DirectedSegmentCircleBorderRelative(const Vector2 &rayOrigin, const Vector2 &rayEnd, float circleRadius, float &t, Vector2 &intersectionPoint);

	//	returns 0 for no intersection, 1 for intersection
	//	a ray that starts inside of the circle will be detected as an intersection
	int Test_RayCircle(const Vector2 &rayOrigin, const Vector2 &rayDir, const Vector2 &circleCenter, float circleRadius);

	//	returns 0 for no intersection, 1 for intersection
	//	a ray that starts inside of the box will be detected as an intersection at the ray origin
	//	this is for a ray with an origin but infinite length
	int Intersect_RayAABB(const Vector2 &rayOrigin, const Vector2 &rayDir, const Vector2 &boxCenter, const Vector2 &boxExtents, float &t, Vector2 &intersectionPoint);

	//	returns 0 for no intersection, 1 for intersection
	//	a ray that starts inside of the box will be detected as an intersection at the ray origin
	//	this is for a ray with an origin but infinite length
	int Intersect_RayOBB(const Vector2 &rayOrigin, const Vector2 &rayDir, const Vector2 &boxCenter, const Vector2 &boxExtents, f32 boxOrientation, float &t, Vector2 &intersectionPoint);

	//	returns 0 for no intersection, 1 for intersection
	//	a ray that starts inside of the box will be detected as an intersection at the ray origin
	//	this is for a ray with an origin and finite length (directed segment)
	int Intersect_DirectedSegmentOBB(const Vector2 &rayOrigin, const Vector2 &rayEnd, const Vector2 &boxCenter, const Vector2 &boxExtents, f32 boxOrientation, float &t, Vector2 &intersectionPoint);

	//	returns 0 for no intersection, 1 for intersection
	int Intersect_RaySegment(const Vector2 &rayStart, const Vector2 &rayDir, const Vector2 &segmentP1, const Vector2 &segmentP2, float &t, Vector2 &intersectionPoint);

	//	returns 0 for no intersection, 1 for intersection
	int Intersect_SegmentSegment(const Vector2 &a1, const Vector2 &a2, const Vector2 &b1, const Vector2 &b2, float &t, Vector2 &intersectionPoint);

	//	stuff used with BSP
	Plane2D SegmentToPlane(const Vector2 &p1, const Vector2 &p2);

	enum ePointClassification
	{
		kPointFront,
		kPointBehind,
		kPointOnPlane,

		kPointNumClassifications
	};

	ePointClassification ClassifyPointToPlane(const Vector2 &point, const Plane2D &plane);

	bool PlanesEqual(const Plane2D &plane1, const Plane2D &plane2);

	enum eSegmentClassification
	{
		kSegmentCoplanar,
		kSegmentFront,
		kSegmentBehind,
		kSegmentStraddle,

		kSegmentNumClassifications
	};

	eSegmentClassification ClassifySegmentToPlane_Build(const Vector2 &p1, const Vector2 &p2, const Plane2D &plane);
	eSegmentClassification ClassifySegmentToPlane_Test(const Vector2 &p1, const Vector2 &p2, const Plane2D &plane);
}