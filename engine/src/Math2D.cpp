#include "types.h"
#include "Math2D.h"

namespace Math
{
	void ConstructBasisFromOrientation(f32 orientation, Vector2& rightOut, Vector2& upOut)
	{
		rightOut.x = cos(orientation);
		rightOut.y = -sin(orientation);

		upOut.x = sin(orientation);
		upOut.y = cos(orientation);
	}

	int Test_CircleCircle(const Vector2 &centerA, float radiusA, const Vector2 &centerB, float radiusB)
	{
		Vector2	displacement = centerB - centerA;
		float	distSq = displacement.LengthSquared();
		float	radiusSum = radiusA + radiusB;

		return distSq <= radiusSum * radiusSum;
	}

	int Intersect_CircleCircle(const Vector2 &centerA, float radiusA, const Vector2 &centerB, float radiusB, float &t)
	{
		Vector2	displacement = centerB - centerA;
		float	distSq = displacement.LengthSquared();
		float	radiusSum = radiusA + radiusB;

		if( distSq > radiusSum * radiusSum )
			return 0;

		t = radiusSum - sqrt(distSq);
		return 1;
	}

	int Intersect_CircleCircleRelative(const Vector2 &centerA, float radiusA, float radiusB, float &t)
	{
		Vector2	displacement = centerA * -1.0f;
		float	distSq = displacement.LengthSquared();
		float	radiusSum = radiusA + radiusB;

		if( distSq > radiusSum * radiusSum )
			return 0;

		t = radiusSum - sqrt(distSq);
		return 1;
	}

	void ClosestPoint_PointSegment(const Vector2 &p, const Vector2 &l1, const Vector2 &l2, float &t, Vector2 &closestPoint)
	{
		Vector2	lDisp = l2 - l1;
		
		//	Project input point onto line while parameterizing
		Vector2	pDisp = p - l1;
		t = pDisp.DotProduct(lDisp) / lDisp.LengthSquared();
		
		//	clamp t to range 0 <= t <= 1
		if( t < 0.0f )
			t = 0.0f;
		if( t > 1.0f )
			t = 1.0f;
		
		//	compute position from t
		closestPoint = l1 + (lDisp * t);
	}

	int Intersect_RayCircle(const Vector2 &rayOrigin, const Vector2 &rayDir, const Vector2 &circleCenter, float circleRadius, float &t, Vector2 &intersectionPoint)
	{
		Vector2	rayCircleDisp = rayOrigin - circleCenter;
		float	b = rayCircleDisp.DotProduct(rayDir);
		float	c = rayCircleDisp.LengthSquared() - (circleRadius * circleRadius);
		
		//	early exit if ray points away from circle and is not contained by it
		if( c > 0.0f &&
			b > 0.0f )
		{
			return 0;
		}

		float	discrim = b * b - c;
		//	check to see if the ray missed the circle
		if( discrim < 0.0f )
			return 0;

		//	intersection known, compute t
		t = -b - sqrt(discrim);

		//	if t is negative, ray started inside of circle, so clamp to 0
		if( t < 0.0f )
			t = 0.0f;

		intersectionPoint = rayOrigin + (rayDir * t);
		return 1;
	}

	int Intersect_RayCircleBorder(const Vector2 &rayOrigin, const Vector2 &rayDir, const Vector2 &circleCenter, float circleRadius, float &t, Vector2 &intersectionPoint)
	{
		Vector2	rayCircleDisp = rayOrigin - circleCenter;
		float	b = rayCircleDisp.DotProduct(rayDir);
		float	c = rayCircleDisp.LengthSquared() - (circleRadius * circleRadius);
		
		//	early exit if ray points away from circle and is not contained by it
		if( c > 0.0f &&
			b > 0.0f )
		{
			return 0;
		}

		float	discrim = b * b - c;
		//	check to see if the ray missed the circle
		if( discrim < 0.0f )
			return 0;

		//	intersection known, compute t
		float	sqrtDiscrim = sqrt(discrim);
		t = -b - sqrtDiscrim;

		//	if t is negative, ray started inside of circle, so use other t
		if( t < 0.0f )
		{
			t = -b + sqrtDiscrim;
		}

		intersectionPoint = rayOrigin + (rayDir * t);
		return 1;
	}

	int Intersect_DirectedSegmentCircle(const Vector2 &rayOrigin, const Vector2 &rayEnd, const Vector2 &circleCenter, float circleRadius, float &t, Vector2 &intersectionPoint)
	{
		//	just call Intersect_RayCircle, and test to see if the intersection point lies on the segment
		Vector2	rayDir = rayEnd - rayOrigin;
		float	segmentLength = rayDir.Length();
		if( segmentLength == 0.0f )
			return 0;

		rayDir /= segmentLength;

		int		result = Intersect_RayCircle(rayOrigin, rayDir, circleCenter, circleRadius, t, intersectionPoint);
		if( result && 
			t > segmentLength )
		{
			result = 0;
		}

		return result;
	}

	int Intersect_DirectedSegmentCircleRelative(const Vector2 &rayOrigin, const Vector2 &rayEnd, float circleRadius, float &t, Vector2 &intersectionPoint)
	{
		Vector2	rayDir = rayEnd - rayOrigin;
		float	segmentLength = rayDir.Length();
		
		if( segmentLength == 0.0f )
			return 0;

		rayDir /= segmentLength;

		Vector2	rayCircleDisp = rayOrigin;
		float	b = rayCircleDisp.DotProduct(rayDir);
		float	c = rayCircleDisp.LengthSquared() - (circleRadius * circleRadius);
		
		//	early exit if ray points away from circle and is not contained by it
		if( c > 0.0f &&
			b > 0.0f )
		{
			return 0;
		}

		float	discrim = b * b - c;
		//	check to see if the ray missed the circle
		if( discrim < 0.0f )
			return 0;

		//	intersection known, compute t
		t = -b - sqrt(discrim);

		//	if t is negative, ray started inside of circle, so clamp to 0
		if( t < 0.0f )
			t = 0.0f;

		//	the intersection point is past the endpoint
		if( t > segmentLength )
			return 0;

		intersectionPoint = rayOrigin + (rayDir * t);
		return 1;
	}

	int Intersect_DirectedSegmentCircleBorderRelative(const Vector2 &rayOrigin, const Vector2 &rayEnd, float circleRadius, float &t, Vector2 &intersectionPoint)
	{
		Vector2	rayDir = rayEnd - rayOrigin;
		float	segmentLength = rayDir.Length();
		
		if( segmentLength == 0.0f )
			return 0;

		rayDir /= segmentLength;

		Vector2	rayCircleDisp = rayOrigin;
		float	b = rayCircleDisp.DotProduct(rayDir);
		float	c = rayCircleDisp.LengthSquared() - (circleRadius * circleRadius);
		
		//	early exit if ray points away from circle and is not contained by it
		if( c > 0.0f &&
			b > 0.0f )
		{
			return 0;
		}

		float	discrim = b * b - c;
		//	check to see if the ray missed the circle
		if( discrim < 0.0f )
			return 0;

		//	intersection known, compute t
		float	sqrtDiscrim = sqrt(discrim);
		t = -b - sqrtDiscrim;

		//	if t is negative, ray started inside of circle, so try other t
		if( t < 0.0f )
		{
			t = -b + sqrtDiscrim;
		}

		//	the intersection point is past the endpoint
		if( t > segmentLength )
			return 0;

		intersectionPoint = rayOrigin + (rayDir * t);
		return 1;
	}

	int Test_RayCircle(const Vector2 &rayOrigin, const Vector2 &rayDir, const Vector2 &circleCenter, float circleRadius)
	{
		Vector2	rayCircleDisp = rayOrigin - circleCenter;
		float	c = rayCircleDisp.LengthSquared() - (circleRadius * circleRadius);

		//	if the ray origin is contained by the circle, then we know they intersect
		if( c <= 0.0f )
			return 1;

		float	b = rayCircleDisp.DotProduct(rayDir);
		//	early exit if ray points away from circle and is not contained by it
		if( b > 0.0f )
			return 0;

		float	discrim = b * b - c;
		//	check to see if the ray missed the circle
		if( discrim < 0.0f )
			return 0;

		return 1;
	}

	int Intersect_RayAABB(const Vector2 &rayOrigin, const Vector2 &rayDir, const Vector2 &boxCenter, const Vector2 &boxExtents, float &t, Vector2 &intersectionPoint)
	{
		//	a ray intersects a box when the intervals of the ray contained by the box's slabs overlap
		//		slab = area between parallel sides

		t = 0.0f;
		f32	tMax = FLT_MAX;

		const f32	kEpsilon = 0.00001f;

		for(int i = 0; i < 2; i++)
		{
			if( abs(rayDir[i]) < kEpsilon )
			{
				//	ray is parallel to this box slab
				//		intersection only if ray origin is contained by slab, in which case the ray intersects the slab along it's entire length
				if( rayOrigin[i] < boxCenter[i] - boxExtents[i] || rayOrigin[i] > boxCenter[i] + boxExtents[i] )
				{
					//	no intersection
					return 0;
				}
			}
			else
			{
				//	compute t values of each intersection of ray with the sides of the slab
				f32	ood = 1.0f / rayDir[i];
				f32	t1 = (boxCenter[i] - boxExtents[i] - rayOrigin[i]) * ood;
				f32	t2 = (boxCenter[i] + boxExtents[i] - rayOrigin[i]) * ood;

				//	make sure that t1 is the point closer to the ray origin
				if( t1 > t2 )
				{
					f32	temp = t1;
					t1 = t2;
					t2 = temp;
				}

				//	intersect this interval with previous intervals
				t = max(t, t1);
				tMax = min(tMax, t2);

				//	if interval is empty, we're done (no intersection)
				if( t > tMax )
				{
					return 0;
				}
			}
		}

		//	intersection found
		intersectionPoint = rayOrigin + (rayDir * t);
		return 1;
	}

	int Intersect_RayOBB(const Vector2 &rayOrigin, const Vector2 &rayDir, const Vector2 &boxCenter, const Vector2 &boxExtents, f32 boxOrientation, float &t, Vector2 &intersectionPoint)
	{
		//	transform ray from world space to box space to remove box rotation
		Vector2	boxSpaceRayOrigin = rayOrigin - boxCenter;
		boxSpaceRayOrigin = boxSpaceRayOrigin.Rotate(-boxOrientation);

		Vector2	boxSpaceRayDir = rayDir.Rotate(-boxOrientation);

		//	call the AABB method
		int	result = Intersect_RayAABB(boxSpaceRayOrigin, boxSpaceRayDir, Vector2::ZERO, boxExtents, t, intersectionPoint);
		if( result )
		{
			//	have to transform intersection point back into world space afterward
			intersectionPoint = intersectionPoint.Rotate(boxOrientation);
			intersectionPoint += boxCenter;
		}
		
		return result;
	}

	int Intersect_DirectedSegmentOBB(const Vector2 &rayOrigin, const Vector2 &rayEnd, const Vector2 &boxCenter, const Vector2 &boxExtents, f32 boxOrientation, float &t, Vector2 &intersectionPoint)
	{
		//	just call Intersect_RayOBB, and test to see if the intersection point lies on the segment
		Vector2	rayDir = rayEnd - rayOrigin;
		float	segmentLength = rayDir.Length();
		if( segmentLength == 0.0f )
			return 0;

		rayDir /= segmentLength;

		int		result = Intersect_RayOBB(rayOrigin, rayDir, boxCenter, boxExtents, boxOrientation, t, intersectionPoint);
		if( result && 
			t > segmentLength )
		{
			result = 0;
		}

		return result;
	}

	int Intersect_RaySegment(const Vector2 &rayStart, const Vector2 &rayDir, const Vector2 &segmentP1, const Vector2 &segmentP2, float &t, Vector2 &intersectionPoint)
	{
		//	find intersection of lines
		Vector2	segmentNormal;
		segmentNormal = segmentP2 - segmentP1;
		segmentNormal.Normalize();
		segmentNormal = segmentNormal.Perp();

		Vector2	b = rayStart + rayDir;
		float	denom = segmentNormal.DotProduct(b - rayStart);

		if( denom == 0.0f )
		{
			//	lines are colinear or parallel
			//	if colinear, whichever segment endpoint is closer to rayStart and in the direction of the ray is the return value
			//	else, no intersection
			f32			signedDistRayStartToSegmentPlane = segmentNormal.DotProduct(rayStart - segmentP1);
			const f32	kColinearEpsilon = 0.01f;
			if( signedDistRayStartToSegmentPlane < kColinearEpsilon )
			{
				f32	t1 = rayDir.DotProduct(segmentP1 - rayStart);
				f32	t2 = rayDir.DotProduct(segmentP2 - rayStart);
				if( t1 >= 0.0f &&
					t2 >= 0.0f )
				{
					if( t1 < t2 )
					{
						t = t1;
					}
					else
					{
						t = t2;
					}
					intersectionPoint = rayStart + rayDir * t;
					return 1;
				}
				else
				if( t1 >= 0.0f )
				{
					t = t1;
					intersectionPoint = rayStart + rayDir * t;
					return 1;
				}
				else
				if( t2 >= 0.0f )
				{
					t = t2;
					intersectionPoint = rayStart + rayDir * t;
					return 1;
				}				
			}
		}
		else
		{
			float	numerator = segmentNormal.DotProduct(segmentP1 - rayStart);
			t = numerator / denom;
			if( t > 0 )
			{
				intersectionPoint = rayStart + rayDir * t;
				return 1;
			}
		}

		return 0;
	}

	float Signed2DTriArea(const Vector2 &a, const Vector2 &b, const Vector2 &c)
	{
		return (a.x - c.x) * (b.y - c.y) - (a.y - c.y) * (b.x - c.x);
	}	

	int Intersect_SegmentSegment0(const Vector2 &a1, const Vector2 &a2, const Vector2 &b1, const Vector2 &b2, float &t, Vector2 &intersectionPoint)
	{
		float	area1 = Signed2DTriArea(a1, a2, b2);
		float	area2 = Signed2DTriArea(a1, a2, b1);

		if( area1 != 0.0f &&
			area2 != 0.0f &&
			area1 * area2 < 0.0f )
		{
			float	area3 = Signed2DTriArea(b1, b2, a1);
			//	area1 - area2 = area3 - area4
			float	area4 = area3 + area2 - area1;

			if( area3 != 0.0f &&
				area4 != 0.0f &&
				area3 * area4 < 0.0f )
			{
				t = area3 / (area3 - area4);
				intersectionPoint = a1 + (a2 - a1) * t;
				return 1;
			}
		}

		return 0;
	}

	//	stuff used with BSP
	Plane2D SegmentToPlane(const Vector2 &p1, const Vector2 &p2)
	{
		Plane2D	returnValue;
		
		returnValue.point = p1;
		
		returnValue.normal = p2 - p1;
		returnValue.normal.Normalize();
		returnValue.normal = returnValue.normal.Perp();

		returnValue.d = returnValue.point.DotProduct(returnValue.normal);

		return returnValue;
	}

	int Intersect_SegmentSegment(const Vector2 &a1, const Vector2 &a2, const Vector2 &b1, const Vector2 &b2, float &t, Vector2 &intersectionPoint)
	{
		Vector2	aDisp = a2 - a1;
		Vector2	bDisp = b2 - b1;

		//	a1 + aDisp * aT = intersection point with line b
		//	b1 + bDisp * bT = intersection point with line a

		Plane2D	bPlane = SegmentToPlane(b1, b2);

		//	point dot normal - d = 0 if point on plane
		//	(a1 + aDisp * aT) dot normal - d = 0
		//		(a1.x + aDisp.x * aT) * normal.x + (a1.y + aDisp.y * aT) * normal.y - d = 0
		//		a1.x * normal.x + aDisp.x * aT * normal.x + a1.y * normal.y + aDisp.y * aT * normal.y - d = 0
		//		aDisp.x * aT * normal.x + aDisp.y * aT * normal.y = d - a1.x * normal.x - a1.y * normal.y
		//		aT * (aDisp.x * normal.x + aDisp.y * normal.y) = d - a1.x * normal.x - a1.y * normal.y
		//		aT = (d - a1.x * normal.x - a1.y * normal.y) / (aDisp.x * normal.x + aDisp.y * normal.y)

		//	find where line a intersects line b
		f32	aTDenom = aDisp.DotProduct(bPlane.normal);
		if( aTDenom == 0.0f )
		{
			//	parallel to plane or coincident
			//		handle coincident case?
			return 0;
		}

		f32		aT = (bPlane.d - a1.x * bPlane.normal.x - a1.y * bPlane.normal.y) / aTDenom;
		if( aT < 0.0f || aT > 1.0f )
		{
			//	intersection beyond "a" segment extents
			return 0;
		}

		Vector2	aIntersectLineB = a1 + aDisp * aT;

		//	test aIntersectLineB to see if it lies on the "b" segment extents
		//		b1 + bDisp * bT = aIntersectLineB
		//		bT = (aIntersectLineB - b1) / bDisp

		f32	bT = 0.0f;
		if( bDisp.x != 0.0f )
		{
			bT = (aIntersectLineB.x - b1.x) / bDisp.x;
		}
		else
		{
			bT = (aIntersectLineB.y - b1.y) / bDisp.y;
		}

		if( bT < 0.0f || bT > 1.0f )
		{
			//	intersection beyond "b" segment extents
			return 0;
		}

		//	intersection is good
		t = aT;
		intersectionPoint = aIntersectLineB;

		return 1;
	}

	const f32	kPlaneThicknessEpsilon = 0.01f;	//	adjust this!
	const f32	kNormalEpsilon = 0.01f;

	ePointClassification ClassifyPointToPlane(const Vector2 &point, const Plane2D &plane)
	{
		f32	signedDist = point.DotProduct(plane.normal) - plane.d;

		//	classify
		if( signedDist > kPlaneThicknessEpsilon )
		{
			return kPointFront;
		}

		if( signedDist < -kPlaneThicknessEpsilon )
		{
			return kPointBehind;
		}

		return kPointOnPlane;
	}

	bool PlanesEqual(const Plane2D &plane1, const Plane2D &plane2)
	{
		//	see if plane2's point lies on plane1, and if the normals are the same
		if( ClassifyPointToPlane(plane2.point, plane1) == kPointOnPlane &&
			(plane1.normal.x - plane2.normal.x) < kNormalEpsilon &&
			(plane1.normal.y - plane2.normal.y) < kNormalEpsilon )
		{
			return true;
		}

		return false;
	}

	eSegmentClassification ClassifySegmentToPlane_Build(const Vector2 &p1, const Vector2 &p2, const Plane2D &plane)
	{
		u32	frontCount = 0;
		u32	behindCount = 0;

		switch(ClassifyPointToPlane(p1, plane))
		{
		case kPointFront:
			frontCount++;
			break;

		case kPointBehind:
			behindCount++;
			break;

		case kPointOnPlane:
			break;
		}

		switch(ClassifyPointToPlane(p2, plane))
		{
		case kPointFront:
			frontCount++;
			break;

		case kPointBehind:
			behindCount++;
			break;

		case kPointOnPlane:
			break;
		}

		if( frontCount != 0 && behindCount != 0 )
		{
			//	straddling
			return kSegmentStraddle;
		}

		if( frontCount != 0 )
		{
			return kSegmentFront;
		}

		if( behindCount != 0 )
		{
			return kSegmentBehind;
		}

		return kSegmentCoplanar;
	}

	eSegmentClassification ClassifySegmentToPlane_Test(const Vector2 &p1, const Vector2 &p2, const Plane2D &plane)
	{
		u32	frontCount = 0;
		u32	behindCount = 0;
		u32	onPlaneCount = 0;

		switch(ClassifyPointToPlane(p1, plane))
		{
		case kPointFront:
			frontCount++;
			break;

		case kPointBehind:
			behindCount++;
			break;

		case kPointOnPlane:
			onPlaneCount++;
			break;
		}

		switch(ClassifyPointToPlane(p2, plane))
		{
		case kPointFront:
			frontCount++;
			break;

		case kPointBehind:
			behindCount++;
			break;

		case kPointOnPlane:
			onPlaneCount++;
			break;
		}

		if( (onPlaneCount == 1) || (frontCount != 0 && behindCount != 0) )
		{
			//	straddling
			return kSegmentStraddle;
		}

		if( frontCount != 0 )
		{
			return kSegmentFront;
		}

		if( behindCount != 0 )
		{
			return kSegmentBehind;
		}

		return kSegmentCoplanar;
	}
}