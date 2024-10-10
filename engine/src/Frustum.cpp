#include "Frustum.h"

#include "types.h"

void Frustum::Init(f32 fov, f32 aspectRatio, f32 zNear, f32 zFar)
{
	f32		nearHeight = 2 * tanf(fov * 0.5f) * zNear;
	f32		nearWidth = nearHeight * aspectRatio;
	f32		farHeight = 2 * tanf(fov * 0.5f) * zFar;
	f32		farWidth = farHeight * aspectRatio;
	
	Vector3	right = Vector3::X_AXIS;
	Vector3	up = Vector3::Y_AXIS;
	Vector3	forward = Vector3::Z_AXIS;
	Vector3	origin = Vector3::ZERO;

	f32		halfHeightFar = farHeight * 0.5f;
	f32		halfWidthFar = farWidth * 0.5f;
	f32		halfHeightNear = nearHeight * 0.5f;
	f32		halfWidthNear = nearWidth * 0.5f;
	
	Vector3	farCenter = forward * zFar;
	Vector3	nearCenter = forward * zNear;

	nlbCorner = nearCenter - right * halfWidthNear - up * halfHeightNear;
	frtCorner = farCenter + right * halfWidthFar + up * halfHeightFar;

	planes[kNearPlane].normal = forward;
	planes[kNearPlane].ComputeD(nearCenter);

	planes[kFarPlane].normal = forward * -1.0f;
	planes[kFarPlane].ComputeD(farCenter);

	Vector3	a;
	a = (nearCenter + up * halfHeightNear);
	a.Normalize();
	planes[kTopPlane].normal = right.CrossProduct(a);
	planes[kTopPlane].ComputeD(origin);

	a = (nearCenter - up * halfHeightNear);
	a.Normalize();
	planes[kBottomPlane].normal = a.CrossProduct(right);
	planes[kBottomPlane].ComputeD(origin);

	a = (nearCenter + right * halfWidthNear);
	a.Normalize();
	planes[kRightPlane].normal = a.CrossProduct(up);
	planes[kRightPlane].ComputeD(origin);

	a = (nearCenter - right * halfWidthNear);
	a.Normalize();
	planes[kLeftPlane].normal = up.CrossProduct(a);
	planes[kLeftPlane].ComputeD(origin);

	//	near/far//left/right//bottom/top
	cornerPoints[0][0][0] = forward * zNear - right * halfWidthNear - up * halfHeightNear;	//	near/left/bottom
	cornerPoints[0][0][1] = forward * zNear - right * halfWidthNear + up * halfHeightNear;	//	near/left/top
	cornerPoints[0][1][0] = forward * zNear + right * halfWidthNear - up * halfHeightNear;	//	near/right/bottom
	cornerPoints[0][1][1] = forward * zNear + right * halfWidthNear + up * halfHeightNear;	//	near/right/top
	cornerPoints[1][0][0] = forward * zFar - right * halfWidthFar - up * halfHeightFar;		//	far/left/bottom
	cornerPoints[1][0][1] = forward * zFar - right * halfWidthFar - up * halfHeightFar;		//	far/left/top
	cornerPoints[1][1][0] = forward * zFar - right * halfWidthFar - up * halfHeightFar;		//	far/right/bottom
	cornerPoints[1][1][1] = forward * zFar - right * halfWidthFar - up * halfHeightFar;		//	far/right/top
}

void Frustum::Init(f32 leftX, f32 rightX, f32 top, f32 bottom, f32 zNear, f32 zFar)
{
	f32		height = top - bottom;
	f32		width = rightX - leftX;
	f32		nearHeight = height;
	f32		nearWidth = width;
	f32		farHeight = height;
	f32		farWidth = width;

	Vector3	right = Vector3::X_AXIS;
	Vector3	up = Vector3::Y_AXIS;
	Vector3	forward = Vector3::Z_AXIS;

	f32		halfHeightFar = farHeight * 0.5f;
	f32		halfWidthFar = farWidth * 0.5f;
	f32		halfHeightNear = nearHeight * 0.5f;
	f32		halfWidthNear = nearWidth * 0.5f;
	
	nlbCorner = forward * zNear;
	frtCorner = forward * zFar + right * farWidth + up * farHeight;

	planes[kNearPlane].normal = forward;
	planes[kNearPlane].ComputeD(nlbCorner);

	planes[kFarPlane].normal = forward * -1.0f;
	planes[kFarPlane].ComputeD(frtCorner);

	planes[kTopPlane].normal = up * -1.0f;
	planes[kTopPlane].ComputeD(frtCorner);

	planes[kBottomPlane].normal = up;
	planes[kBottomPlane].ComputeD(nlbCorner);

	planes[kRightPlane].normal = right * -1.0f;
	planes[kRightPlane].ComputeD(frtCorner);

	planes[kLeftPlane].normal = right;
	planes[kLeftPlane].ComputeD(nlbCorner);

	//	near/far//left/right//bottom/top
	cornerPoints[0][0][0] = forward * zNear;										//	near/left/bottom
	cornerPoints[0][0][1] = forward * zNear + up * nearHeight;						//	near/left/top
	cornerPoints[0][1][0] = forward * zNear + right * nearWidth;					//	near/right/bottom
	cornerPoints[0][1][1] = forward * zNear + up * nearHeight + right * nearWidth;	//	near/right/top
	cornerPoints[1][0][0] = forward * zFar;											//	far/left/bottom
	cornerPoints[1][0][1] = forward * zFar + up * farHeight;						//	far/left/top
	cornerPoints[1][1][0] = forward * zFar + right * farWidth;						//	far/right/bottom
	cornerPoints[1][1][1] = forward * zFar + up * farHeight + right * farWidth;		//	far/right/top
}

//void Frustum::Init(f32 leftX, f32 rightX, f32 top, f32 bottom, f32 zNear, f32 zFar)
//{
//	f32		height = top - bottom;
//	f32		width = rightX - leftX;
//	f32		nearHeight = height;
//	f32		nearWidth = width;
//	f32		farHeight = height;
//	f32		farWidth = width;
//
//	Vector3	right = Vector3::X_AXIS;
//	Vector3	up = Vector3::Y_AXIS;
//	Vector3	forward = Vector3::Z_AXIS;
//
//	f32		halfHeightFar = farHeight * 0.5f;
//	f32		halfWidthFar = farWidth * 0.5f;
//	f32		halfHeightNear = nearHeight * 0.5f;
//	f32		halfWidthNear = nearWidth * 0.5f;
//	
//	Vector3	farCenter = forward * zFar;
//	Vector3	nearCenter = forward * zNear;
//
//	planes[kNearPlane].normal = forward;
//	planes[kNearPlane].ComputeD(nearCenter);
//
//	planes[kFarPlane].normal = forward * -1.0f;
//	planes[kFarPlane].ComputeD(farCenter);
//
//	Vector3	a;
//	a = (nearCenter + up * halfHeightNear);
//	planes[kTopPlane].normal = up * -1.0f;
//	planes[kTopPlane].ComputeD(a);
//
//	a = (nearCenter - up * halfHeightNear);
//	planes[kBottomPlane].normal = up;
//	planes[kBottomPlane].ComputeD(a);
//
//	a = (nearCenter + right * halfWidthNear);
//	planes[kRightPlane].normal = right * -1.0f;
//	planes[kRightPlane].ComputeD(a);
//
//	a = (nearCenter - right * halfWidthNear);
//	planes[kLeftPlane].normal = right;
//	planes[kLeftPlane].ComputeD(a);
//}

bool Frustum::PointInside(const Vector3 &point)
{
	bool	returnValue = true;

	for(int i = 0; i < kPlaneCount && returnValue; i++) 
	{
		if( planes[i].SignedDistanceToPoint(point) < 0 )
		{
			returnValue = false;
		}
	}

	return returnValue;
}

bool Frustum::SphereInside(const Vector3 &point, f32 radius) const
{
	bool returnValue = true;

	for(int i = 0; i < kPlaneCount && returnValue; i++) 
	{
		f32	distance = planes[i].SignedDistanceToPoint(point);
		if( distance < -radius )
		{
			returnValue = false;
		}
	}

	return returnValue;
}

bool Frustum::BoxInside(const Vector3 &maxs, const Vector3 &mins) const
{
	bool returnValue = true;

	int	out;
	int	in;

	Vector3	extents((coord_type)fabs(maxs.x - mins.x), (coord_type)fabs(maxs.y - mins.y), (coord_type)fabs(maxs.z - mins.z));

	bool	done = false;
	for(int i = 0; i < kPlaneCount && !done; i++) 
	{
		out = 0;
		in = 0;

		for(int k = 0; k < 8 && (in == 0 || out == 0); k++) 
		{
			Vector3	point = mins;
			point.x += extents.x * ((k & (1 << 0)) ? 1 : 0);
			point.y += extents.y * ((k & (1 << 1)) ? 1 : 0);
			point.z += extents.z * ((k & (1 << 2)) ? 1 : 0);

			if( planes[i].SignedDistanceToPoint(point) < 0 )
			{
				out++;
			}
			else
			{
				in++;
			}
		}

		//	if all of the points are outside, the box is outside
		if( in == 0 )
		{
			returnValue = false;
			done = true;
		}
	}

	return returnValue;
}