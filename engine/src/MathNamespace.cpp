#include "types.h"
#include "MathNamespace.h"
#include "MathConstants.h"

namespace Math
{
	bool QuadraticFormula(const double a, const double b, const double c, double &root1, double &root2)
	{
		const double q = b * b - 4 * a * c;
		if( q >= 0 )
		{
			//real roots
			const double sq = sqrt(q);
			const double d = 1 / (2 * a);
			root1 = ( -b + sq ) * d;
			root2 = ( -b - sq ) * d;
			return true;
		}

		//complex roots
		return false;	
	}

	bool SphereSphereSweep(	const double radiusA, const Vector3 &positionA0, const Vector3 &positionA1, 
							const double radiusB, const Vector3 &positionB0, const Vector3 &positionB1, 
							double &time0, double &time1 )
	{
		const Vector3	va = positionA1 - positionA0;
		const Vector3	vb = positionB1 - positionB0;
		const Vector3	AB = positionB0 - positionA0;

		const Vector3	vab = vb - va;		

		const double	rab = radiusA + radiusB;
		const double	rabSq = rab * rab;

		const double	a = vab.LengthSquared();		

		const double	b = 2 * vab.DotProduct(AB);		

		const double	lengthABSq = AB.LengthSquared();
		const double	c = lengthABSq - rabSq;		

		//	check if they're currently overlapping
		if( lengthABSq <= rabSq )
		{
			time0 = 0;
			time1 = 0;
			return true;
		}

		//	check if they hit each other during the frame
		if( a != 0 )
		{		
			if( QuadraticFormula(a, b, c, time0, time1) )
			{
				if( time0 > time1 )
				{
					const double	temp = time0;

					time0 = time1;
					time1 = temp;
				}

				if( 0 <= time0 && 
					time0 <= 1 )
				{
					return true;
				}

				/*if( 0 <= time1 && 
					time1 <= 1 )
				{
					const double	temp = time0;

					time0 = time1;
					time1 = temp;
					return true;
				}*/

				return false;
			}
		}
		else
		if( b != 0 )
		{
			time0 = time1 = -c / b;
			if( 0 <= time0 && 
				time0 <= 1 )
			{
				return true;
			}
		}

		return false;
	}

	bool SphereSphereIntersection(	const double radiusA, const Vector3 &positionA,
									const double radiusB, const Vector3 &positionB,
									Vector3 &normal, double &moveDist )
	{
		const Vector3	displacement = positionA - positionB;		

		const double	radiusSum = radiusA + radiusB;
		const double	radiusSumSq = radiusSum * radiusSum;
		const double	dispLenSq = displacement.LengthSquared();

		if( radiusSumSq < dispLenSq )
		{
			return false;
		}

		moveDist = radiusSum - sqrt(dispLenSq);

		if( dispLenSq > 0 )
		{
			normal = displacement;
			normal.Normalize();
		}
		else
		{
			//	chosen arbitrarily
			normal = Vector3::X_AXIS;
		}

		return true;
	}

	bool CircleCircleIntersection(	const double radiusA, const Vector2 &positionA,
									const double radiusB, const Vector2 &positionB,
									Vector2 &normal, double &moveDist )
	{
		const Vector2	displacement = positionA - positionB;		

		const double	radiusSum = radiusA + radiusB;
		const double	radiusSumSq = radiusSum * radiusSum;
		const double	dispLenSq = displacement.LengthSquared();

		if( radiusSumSq < dispLenSq )
		{
			return false;
		}

		moveDist = radiusSum - sqrt(dispLenSq);

		if( dispLenSq > 0 )
		{
			normal = displacement;
			normal.Normalize();
		}
		else
		{
			//	chosen arbitrarily
			normal = Vector2::X_AXIS;
		}

		return true;
	}

	//	this assumes that inputPoint is on the input line, it's just checking if it is bounded by the points
	bool PointOnLineOnSegment(const Vector2 &inputPoint, const Vector2 &linePoint1, const Vector2 &linePoint2)
	{
		bool		returnValue = true;
		Vector2		displacementFromPoint = inputPoint - linePoint1;
		Vector2		displacementFromSegmentStart = linePoint2 - linePoint1;
		coord_type	dispDot = displacementFromSegmentStart.DotProduct(displacementFromPoint);
		if( dispDot < 0.0 || dispDot > displacementFromSegmentStart.LengthSquared() )
		{
			return returnValue = false;
		}

		return returnValue;
	}

	bool CircleLineSegmentIntersection(	const double circleRadius, const Vector2 &circleCenter,
										const Vector2 &linePoint1, const Vector2 &linePoint2,
										Vector2 &normal, double &moveDist )
	{
		Vector2	lineDelta = linePoint2 - linePoint1;

		Vector2	intersectionPoint;
		if( lineDelta.x != 0.0 )
		{
			//	find where line and perpendicular line through circle center intersect.  this will be the point on the line closest to the circle
			coord_type	lineSlope = lineDelta.y / lineDelta.x;
			coord_type	lineSlopeSq = lineSlope * lineSlope;
			intersectionPoint.x = (lineSlope * (circleCenter.y - linePoint1.y) + lineSlopeSq * linePoint1.x + circleCenter.x) / (lineSlopeSq + 1);
			intersectionPoint.y = lineSlope * (intersectionPoint.x - linePoint1.x) + linePoint1.y;
		}
		else
		{
			//	vertical line
			//	check to see if the x value is within range of the circle
			if( (linePoint1.x > circleCenter.x + circleRadius) || (linePoint1.x < circleCenter.x - circleRadius) )
			{
				//	no intersection
				return false;
			}

			//	the line perpendicular to the segment through the circle's center is the line y = circleCenter.y
			//	the closest point to the circle is thus :
			intersectionPoint.x = linePoint1.x;
			intersectionPoint.y = circleCenter.y;
		}

		//	check to see if the intersection point lies between the points of the segment.  if it doesn't, then our actual closest point is one of the segment endpoints
		Vector2	displacementFromIntersection = intersectionPoint - linePoint1;
		Vector2	displacementFromSegmentStart = linePoint2 - linePoint1;
		coord_type	dispDot = displacementFromSegmentStart.DotProduct(displacementFromIntersection);
		if( dispDot < 0.0 )
		{
			//	point lies before segment start, so use segment start
			intersectionPoint = linePoint1;
		}
		else
		if( dispDot > displacementFromSegmentStart.LengthSquared() )
		{
			//	point lies past segment end, so use segment end
			intersectionPoint = linePoint2;
		}

		Vector2		displacementFromCenterOfCircle = circleCenter - intersectionPoint;
		coord_type	lenSq = displacementFromCenterOfCircle.LengthSquared();
		if( lenSq >= circleRadius * circleRadius )
		{
			//	no intersection
			return false;
		}			

		coord_type	length = sqrt(lenSq);
		if( length > 0.0f )
		{
			normal = displacementFromCenterOfCircle;
			normal *= 1.0f / length;
		}
		else
		{
			//	chosen arbitrarily
			normal = Vector2::X_AXIS;
		}

		moveDist = circleRadius - length;

		return true;
	}

	bool CircleRayIntersection(	const float circleRadius, const Vector2 &circleCenter,
								const Vector2 &rayOrigin, const Vector2 &rayEnd,
								Vector2 &intersectionPoint, Vector2 &intersectionNormal )
	{
		//	a line will intersect a circle in either 0, 1, or 2 points
		//	if a point is found, we will copy the one closest to the ray origin and also lying on the segment to intersectionPoint
		//	in the 1 point case, the line is tangent to the circle

		//	transform the ray into circle space
		Vector2	rayOriginInCircleSpace = rayOrigin - circleCenter;
		Vector2	rayEndInCircleSpace = rayEnd - circleCenter;

		//	find intersection
		Vector2	rayDelta = rayEndInCircleSpace - rayOriginInCircleSpace;
		if( rayDelta.x > 0.0f )
		{
			f32	raySlope = rayDelta.y / rayDelta.x;
			f32	rayYIntercept = rayOriginInCircleSpace.y - raySlope * rayOriginInCircleSpace.x;
			f32	denom = (raySlope * raySlope) + 1;
			f32	q = raySlope * rayYIntercept / denom;
			f32	determinant = ((circleRadius * circleRadius - rayYIntercept * rayYIntercept) / denom) + (q * q);
			if( determinant < 0.0f )
			{
				//	no intersection
				return false;
			}
			else
			if( determinant == 0.0f )
			{
				//	line is tangent to circle, one intersection
				intersectionPoint.x = q * -1.0f;
				intersectionPoint.y = raySlope * (intersectionPoint.x - rayOriginInCircleSpace.x) + rayOriginInCircleSpace.y;

				//	see if the point lies on the segment
				if( !PointOnLineOnSegment(intersectionPoint, rayOriginInCircleSpace, rayEndInCircleSpace) )
				{
					return false;
				}
			}
			else
			{
				//	determinant > 0.0f
				//	two intersections, find closest that lies on the segment
				Vector2	intersect[2];
				f32		sqrtDet = sqrt(determinant);
				intersect[0].x = q * -1.0f + sqrtDet;
				intersect[1].x = q * -1.0f - sqrtDet;
				intersect[0].y = raySlope * (intersect[0].x - rayOriginInCircleSpace.x) + rayOriginInCircleSpace.y;
				intersect[1].y = raySlope * (intersect[1].x - rayOriginInCircleSpace.x) + rayOriginInCircleSpace.y;

				Vector2	disp[2];
				disp[0] = intersect[0] - rayOriginInCircleSpace;
				disp[1] = intersect[1] - rayOriginInCircleSpace;
				int	closestIdx = 0;
				if( disp[0].LengthSquared() < disp[1].LengthSquared() )
				{
					closestIdx = 0;
				}
				else
				{
					closestIdx = 1;
				}

				if( PointOnLineOnSegment(intersect[closestIdx], rayOriginInCircleSpace, rayEndInCircleSpace) )
				{
					intersectionPoint = intersect[closestIdx];
				}
				else
				if( PointOnLineOnSegment(intersect[(closestIdx+1) & 0x1], rayOriginInCircleSpace, rayEndInCircleSpace) )
				{
					intersectionPoint = intersect[(closestIdx+1) & 0x1];
				}
				else
				{
					return false;
				}
			}
		}
		else
		{
			//	line is vertical
			f32	determinant = circleRadius * circleRadius - rayOriginInCircleSpace.x * rayOriginInCircleSpace.x;
			if( determinant < 0.0f )
			{
				//	no intersection
				return false;
			}
			else
			if( determinant == 0.0f )
			{
				//	line is tangent to circle, one intersection
				intersectionPoint.x = rayOriginInCircleSpace.x;
				intersectionPoint.y = 0.0f;

				//	see if the point lies on the segment
				if( !PointOnLineOnSegment(intersectionPoint, rayOriginInCircleSpace, rayEndInCircleSpace) )
				{
					return false;
				}
			}
			else
			{
				//	determinant > 0.0f
				//	two intersections, find closest that lies on the segment
				Vector2	intersect[2];
				f32		sqrtDet = sqrt(determinant);
				intersect[0].x = rayOriginInCircleSpace.x;
				intersect[1].x = rayOriginInCircleSpace.x;
				intersect[0].y = sqrtDet;
				intersect[1].y = sqrtDet * -1.0f;
				
				Vector2	disp[2];
				disp[0] = intersect[0] - rayOriginInCircleSpace;
				disp[1] = intersect[1] - rayOriginInCircleSpace;
				int	closestIdx = 0;
				if( disp[0].LengthSquared() < disp[1].LengthSquared() )
				{
					closestIdx = 0;
				}
				else
				{
					closestIdx = 1;
				}

				if( PointOnLineOnSegment(intersect[closestIdx], rayOriginInCircleSpace, rayEndInCircleSpace) )
				{
					intersectionPoint = intersect[closestIdx];
				}
				else
				if( PointOnLineOnSegment(intersect[(closestIdx+1) & 0x1], rayOriginInCircleSpace, rayEndInCircleSpace) )
				{
					intersectionPoint = intersect[(closestIdx+1) & 0x1];
				}
				else
				{
					return false;
				}
			}
		}		

		//	transform back into world space
		intersectionNormal = intersectionPoint;
		intersectionNormal.Normalize();

		intersectionPoint += circleCenter;

		//
		return true;
	}

	bool LineSegmentLineSegmentIntersection(	const Vector2 &linePoint1A, const Vector2 &linePoint2A,
												const Vector2 &linePoint1B, const Vector2 &linePoint2B,
												Vector2 &intersectionPoint, Vector2 &intersectionNormal )
	{
		//	if the lines aren't parallel, then they intersect in exactly one point
		//		it's just a question of whether the intersection point lies on the part of the lines bounded by the input points
		//	if the lines are parallel, then they either don't intersect or they intersect in every point that the segments have in common
		//		in this case, we're going to treat that as non-intersecting
		Vector2	lineDeltaA = linePoint2A - linePoint1A;
		Vector2	lineDeltaB = linePoint2B - linePoint1B;
		if( lineDeltaA.x == 0.0f &&
			lineDeltaB.x == 0.0f )
		{
			//	parallel vertical lines
			return false;
		}
		else
		if( lineDeltaA.x == 0.0f )
		{
			//	line A is vertical, line B is not
			coord_type	slopeB = lineDeltaB.y / lineDeltaB.x;
			intersectionPoint.x = linePoint1A.x;
			intersectionPoint.y = slopeB * (intersectionPoint.x - linePoint1B.x) + linePoint1B.y;
		}
		else
		if( lineDeltaB.x == 0.0f )
		{
			//	line B is vertical, line A is not
			coord_type	slopeA = lineDeltaA.y / lineDeltaA.x;
			intersectionPoint.x = linePoint1B.x;
			intersectionPoint.y = slopeA * (intersectionPoint.x - linePoint1A.x) + linePoint1A.y;
		}
		else
		{
			coord_type	slopeA = lineDeltaA.y / lineDeltaA.x;
			coord_type	slopeB = lineDeltaB.y / lineDeltaB.x;

			if( slopeA == slopeB )
			{
				//	parallel
				return false;
			}
			else
			{
				//	find intersection point
				intersectionPoint.x = (linePoint1B.y - linePoint1A.y + slopeA * linePoint1A.x - slopeB * linePoint1B.x) / (slopeA - slopeB);
				intersectionPoint.y = slopeA * (intersectionPoint.x - linePoint1A.x) + linePoint1A.y;				
			}
		}

		//	figure out if the point is bounded by the line endpoints
		if( !(PointOnLineOnSegment(intersectionPoint, linePoint1A, linePoint2A) && PointOnLineOnSegment(intersectionPoint, linePoint1B, linePoint2B)) )
		{
			//	not on segments
			return false;
		}

		intersectionNormal.x = -lineDeltaB.y;
		intersectionNormal.y = lineDeltaB.x;
		intersectionNormal.Normalize();

		return true;
	}

	bool SphereTriangleIntersection(	const double radiusS, const Vector3 &positionS,
										const Vector3 &posT1, const Vector3 &posT2, const Vector3 &posT3, const Vector3 &normalT, const double triD,
										Vector3 &dir, double &moveDist )
	{
		const double	distToPlane = positionS.DotProduct(normalT) + triD;	//	distance of sphere center to triangle's plane

		//	trivial distance rejection
		if( distToPlane > radiusS )
		{
			return false;
		}

		//	find the point on the triangle closest to the sphere using voronoi regions
		Vector3	closestPoint;
		{
			//	vertex 1
			Vector3			t1t2 = posT2 - posT1;
			Vector3			t1t3 = posT3 - posT1;
			Vector3			t1s = positionS - posT1;
			const double	d1 = t1t2.DotProduct(t1s);
			const double	d2 = t1t3.DotProduct(t1s);
			if(	d1 <= 0.0f && 
				d2 <= 0.0f )
			{
				closestPoint = posT1;
			}
			else
			{
				//	vertex 2
				Vector3			t2s = positionS - posT2;
				const double	d3 = t1t2.DotProduct(t2s);
				const double	d4 = t1t3.DotProduct(t2s);
				if(	d3 >= 0.0f && 
					d4 <= d3 ) 
				{
					closestPoint = posT2;
				}
				else
				{
					//	edge 1->2
					double		v3 = d1 * d4 - d3 * d2;
					if( v3 <= 0.0f && 
						d1 >= 0.0f && 
						d3 <= 0.0f &&
						d1 != d3 )
					{
						const double	t = d1 / (d1 - d3);
						closestPoint = posT1 + (t1t2 * (f32)t);
					}
					else
					{
						//	vertex 3
						Vector3			t3s = positionS - posT3;
						const double	d5 = t1t2.DotProduct(t3s);
						const double	d6 = t1t3.DotProduct(t3s);
						if( d6 >= 0.0f && 
							d5 <= d6 )
						{
							closestPoint = posT3;
						}
						else
						{
							//	edge 1->3
							double	v2 = d5 * d2 - d1 * d6;
							if( v2 <= 0.0f && 
								d2 >= 0.0f && 
								d6 <= 0.0f &&
								d2 != d6 )
							{
								const double	t = d2 / (d2 - d6);
								closestPoint = posT1 + (t1t3 * (f32)t);
							}
							else
							{
								//	edge 2->3
								double			v1 = d3 * d6 - d5 * d4;
								double			d3d4 = d4 - d3;
								const double	denom2 = d3d4 + (d5 - d6);
								if( v1 <= 0.0f && 
									(d4 - d3) >= 0.0f && 
									(d5 - d6) >= 0.0f &&
									denom2 != 0.0f ) 
								{
									const double	t = d3d4 / denom2;
									Vector3			t2t3 = posT3 - posT2;
									closestPoint = posT2 + (t2t3 * (f32)t);
								}
								else
								{
									//	point is inside the triangle
									closestPoint = positionS + (normalT * (-(f32)distToPlane));
								}
							}
						}
					}
				}
			}
		}

		Vector3	v = positionS;
		v -= closestPoint;
		double	distSq = v.LengthSquared();
		if( distSq < radiusS * radiusS ) 
		{
			//	we have a collision
			double	dist = sqrt(distSq);
			if( dist == 0.0f ) 
			{
				//	might as well use the triangle's normal if the sphere's point is inside of it
				dir = normalT;
				//dir *= -1.0f;
				moveDist = radiusS;
			}
			else 
			{
				dir = v;
				dir /= (f32)dist;
				//dir *= -1.0f;
				moveDist = radiusS - dist;
			}

			return true;
		}

		return false;
	}

	bool RayAABBIntersection(const Vector3 &rayDir, const Vector3 &rayStart, const BoundingBox3D &box, f32 &intersectionDistance)
	{
		const f32	kInfinity = 100000000000000000.0f;
		f32	tNear = -kInfinity;
		f32	tFar = kInfinity;

		for(int i = 0; i < 3; i++)
		{
			if( rayDir[i] == 0.0f )
			{
				//	parallel to plane
				if( rayStart[i] < box.min[i] || rayStart[i] > box.max[i] )
				{
					//	does not intersect
					return false;
				}
			}
			else
			{
				f32	t1 = (box.min[i] - rayStart[i]) / rayDir[i];
				f32	t2 = (box.max[i] - rayStart[i]) / rayDir[i];
				if( t1 > t2 )
				{
					f32	temp = t1;
					t1 = t2;
					t2 = temp;
				}
				if( t1 > tNear )
				{
					tNear = t1;
				}
				if( t2 < tFar )
				{
					tFar = t2;
				}
				if( tNear > tFar )
				{
					//	does not intersect
					return false;
				}
				if( tFar < 0 )
				{
					//	box behind ray
					return false;
				}
			}
		}

		intersectionDistance = tNear;
		return true;
	}

	bool RayOBBIntersection(const Vector3 &rayDir, const Vector3 &rayStart, const BoundingBox3D &box, const Mtx44 &boxTransform, f32 &intersectionDistance)
	{
		const f32	kInfinity = 100000000000000000.0f;
		f32	tNear = -kInfinity;
		f32	tFar = kInfinity;

		Vector3	boxPosition = boxTransform.GetTranslation3();
		Vector3	delta = boxPosition - rayStart;

		for(int i = 0; i < 3; i++)
		{
			const Vector3	&axis = *((const Vector3*)(&boxTransform.m[i]));
			f32				e = delta.DotProduct(axis);
			f32				f = rayDir.DotProduct(axis);

			if( f == 0.0f )
			{
				f32	boxDelta = box.max[i] - box.min[i];
				if( fabs(e) > boxDelta )
				{
					return false;
				}
			}
			else
			{
				float			t1 = (e + box.min[i]) / f;
				float			t2 = (e + box.max[i]) / f;

				if( t1 > t2)
				{
					f32 temp = t1;
					t1 = t2;
					t2 = temp;
				}

				if( t2 < tFar )
				{
					tFar = t2;
				}

				if( t1 > tNear )
				{
					tNear = t1;
				}

				if( tNear > tFar )
				{
					return false;
				}
			}
		}

		intersectionDistance = tNear;
		return true;
	}

	bool AABBSphereIntersection(const Vector3& min, const Vector3& max, const Vector3& sphereCenter, const float sphereRadius)
	{
		float distanceSq = sphereRadius * sphereRadius;
    
		for(int i = 0; i < 3; i++)
		{
			if( sphereCenter[i] < min[i] )
			{
				float distanceMod = sphereCenter[i] - min[i];
				distanceSq -= distanceMod * distanceMod;
			}
			else if( sphereCenter[i] > max[i] )
			{
				float distanceMod = sphereCenter[i] - max[i];
				distanceSq -= distanceMod * distanceMod;
			}
		}

		return distanceSq > 0;
	}

	bool FindTrajectoryAngles(const f32 speed, const Vector3& targetOffset, const f32 gravity, f32& angle1Out, f32& angle2Out)
	{
		//	wiki
		//		assume firing from 0,0 and pre-set velocity = v, firing at target (x,y)
		//	                     (v^2 +- sqrt(v^4 - g(gx^2 + 2yv^2))
		//	(fire angle) = arctan(---------------------------------)
		//	                     ( gx                              )


		Vector3	horizontalOffsetVec = targetOffset;
		horizontalOffsetVec.y = 0.0f;

		f32	horizontalOffset = horizontalOffsetVec.Length();
		f32	verticalOffset = targetOffset.y;


		const f32		kDeterminant = (speed*speed*speed*speed) - gravity*(gravity*horizontalOffset*horizontalOffset + 2*verticalOffset*speed*speed);	//v^4 - g(gx^2 + 2yv^2)

		if( kDeterminant < 0.0f )
		{
			//	no trajectory to the target exists
			//		initial velocity isn't great enough
			return false;
		}

		const f32		kAngleDenom = gravity * horizontalOffset;
		const f32		kDeterminantSqrt = sqrt(kDeterminant);
		const f32		kAngleNom1 = speed*speed + kDeterminantSqrt;
		const f32		kAngleNom2 = speed*speed - kDeterminantSqrt;

		angle1Out = atan(kAngleNom1 / kAngleDenom);
		angle2Out = atan(kAngleNom2 / kAngleDenom);

		return true;
	}

	u32 NextPowerOf2(u32 number)
	{
		/*u32	returnValue = 1;
		while(returnValue < number)
		{
			returnValue <<= 1;
		}
		
		return returnValue;*/

		number--;
		number |= number >> 1;
		number |= number >> 2;
		number |= number >> 4;
		number |= number >> 8;
		number |= number >> 16;
		number++;

		return number;
	}

	float FRand()
	{
		return ((float)rand()) / ((float)RAND_MAX);
	}

	float FRand2()
	{
		return (FRand() - 0.5f) * 2.0f;
	}

	float LN(float input)
	{
		return std::log(input);
	}

	void ComposeTransform(const Quat& rotation, const Vector3& scale, const Vector3& translation, Mtx44* output)
	{
		output->Identity();

		Mtx33	rotationMtx;
		rotation.Convert(&rotationMtx);

		output->SetFromMtx33(rotationMtx);
		output->SetScale(scale);
		output->SetTranslation3(translation);
	}

	void DecomposeTransform(const Mtx44& transformIn, Quat* rotationOut, Vector3* scaleOut, Vector3* translationOut)
	{
		*translationOut = transformIn.GetTranslation3();

		*scaleOut = transformIn.GetScale();

		Mtx33			rotationMtx = transformIn.ExtractMtx33();
		rotationMtx.ResetScale();
		*rotationOut = Quat(rotationMtx);
	}

	void ComposeTransform(const Quat& rotation, const Vector3& scale, const Vector3& translation, Mtx43* output)
	{
		output->Identity();

		Mtx33	rotationMtx;
		rotation.Convert(&rotationMtx);

		output->SetFromMtx33(rotationMtx);
		output->SetScale(scale);
		output->SetTranslation(translation);
	}

	void DecomposeTransform(const Mtx43& transformIn, Quat* rotationOut, Vector3* scaleOut, Vector3* translationOut)
	{
		*translationOut = transformIn.GetTranslation();

		*scaleOut = transformIn.GetScale();

		Mtx33			rotationMtx = transformIn.ExtractRotation();
		*rotationOut = Quat(rotationMtx);
	}

	void NormalizeEuler(const Vector3& inputEulerAngles, Vector3* outputAngles)
	{
		//	move from range {-pi, pi} to {-1, 1}
		*outputAngles = inputEulerAngles * (1.0f / M_PI);
	}

	void DenormalizeEuler(const Vector3& inputAngles, Vector3* outputEulerAngles)
	{
		//	move from range {-1, 1} to {-pi, pi}
		*outputEulerAngles = inputAngles * M_PI;
	}

	bool NearEqual(const f32* data1, const f32* data2, int dataLength, f32 epsilon)
	{
		bool	result = true;

		for(int i = 0; i < dataLength && result; ++i)
		{
			result = fabs(data1[i] - data2[i]) < epsilon;
		}

		return result;
	}
}