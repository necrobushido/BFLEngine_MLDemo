#pragma once

#include "Vector2.h"
#include "Vector3.h"
#include "BoundingBox3D.h"
#include "Mtx44.h"
#include "Mtx43.h"
#include "Mtx33.h"
#include "Quat.h"

namespace Math
{
	// Return true if root1 and root2 are real
	bool QuadraticFormula(const double a, const double b, const double c, double &root1, double &root2);

	//	returns true if there is a collision
	bool SphereSphereSweep(	const double radiusA, const Vector3 &positionA0, const Vector3 &positionA1, 
							const double radiusB, const Vector3 &positionB0, const Vector3 &positionB1, 
							double &time0, double &time1 );

	bool CircleCircleIntersection(	const double radiusA, const Vector2 &positionA,
									const double radiusB, const Vector2 &positionB,
									Vector2 &normal, double &moveDist );

	bool CircleLineSegmentIntersection(	const double circleRadius, const Vector2 &circleCenter,
										const Vector2 &linePoint1, const Vector2 &linePoint2,
										Vector2 &normal, double &moveDist );

	//	very similar to CircleLineSegmentIntersection, but serves different purpose
	//	this one is for ray casting, the other is for collision depenetration
	//	consider finding a way to merge them?
	bool CircleRayIntersection(	const float circleRadius, const Vector2 &circleCenter,
								const Vector2 &rayOrigin, const Vector2 &rayEnd,
								Vector2 &intersectionPoint, Vector2 &intersectionNormal );

	bool LineSegmentLineSegmentIntersection(	const Vector2 &linePoint1A, const Vector2 &linePoint2A,
												const Vector2 &linePoint1B, const Vector2 &linePoint2B,
												Vector2 &intersectionPoint, Vector2 &intersectionNormal );

	bool SphereSphereIntersection(	const double radiusA, const Vector3 &positionA,
									const double radiusB, const Vector3 &positionB,
									Vector3 &normal, double &moveDist );

	bool SphereTriangleIntersection(	const double radiusS, const Vector3 &positionS,
										const Vector3 &posT1, const Vector3 &posT2, const Vector3 &posT3, const Vector3 &normalT, const double triD,
										Vector3 &dir, double &moveDist );

	//	these are untested so far
	bool RayAABBIntersection(const Vector3 &rayDir, const Vector3 &rayStart, const BoundingBox3D &box, f32 &intersectionDistance);
	bool RayOBBIntersection(const Vector3 &rayDir, const Vector3 &rayStart, const BoundingBox3D &box, const Mtx44 &boxTransform, f32 &intersectionDistance);

	bool AABBSphereIntersection(const Vector3& min, const Vector3& max, const Vector3& sphereCenter, const float sphereRadius);

	//	returns true if a trajectory exists
	bool FindTrajectoryAngles(const f32 speed, const Vector3& targetOffset, const f32 gravity, f32& angle1Out, f32& angle2Out);

	//
	u32 NextPowerOf2(u32 number);

	//	returns a random float on the range 0.0 to 1.0, inclusive
	float FRand();

	//	returns a random float on the range -1.0 to 1.0, inclusive
	float FRand2();

	float LN(float input);

	// returns sign of val
	template <typename T> int Sign(T val) 
	{
		return (T(0) < val) - (val < T(0));
	}

	//
	void ComposeTransform(const Quat& rotation, const Vector3& scale, const Vector3& translation, Mtx44* output);
	void DecomposeTransform(const Mtx44& transformIn, Quat* rotationOut, Vector3* scaleOut, Vector3* translationOut);
	void ComposeTransform(const Quat& rotation, const Vector3& scale, const Vector3& translation, Mtx43* output);
	void DecomposeTransform(const Mtx43& transformIn, Quat* rotationOut, Vector3* scaleOut, Vector3* translationOut);

	void NormalizeEuler(const Vector3& inputEulerAngles, Vector3* outputAngles);
	void DenormalizeEuler(const Vector3& inputAngles, Vector3* outputEulerAngles);

	bool NearEqual(const f32* data1, const f32* data2, int dataLength, f32 epsilon);
}