#pragma once

#include "types.h"

#include "Mtx44.h"
#include "Frustum.h"
#include "Vector2.h"

class CameraBase
{
public:
	CameraBase();
	~CameraBase();

public:
	void Apply();

	void SetPerspective(f32 fovY, f32 aspectRatio, f32 zNear, f32 zFar);
	void SetOrtho(f32 left, f32 right, f32 top, f32 bottom, f32 zNear, f32 zFar);

	void LookAt(const Vector3 &pos, const Vector3 &up = Vector3::Y_AXIS);
	void SetPosition(const Vector3 &pos);
	const Vector3 &GetPosition() const;
	void Displace(const Vector3 &amount);

	bool PointInFrustum(const Vector2 &point);
	bool CircleInFrustum(const Vector2 &center, f32 radius);
	bool PointInFrustum(const Vector3 &point);
	bool SphereInFrustum(const Vector3 &center, f32 radius);
	bool BoxInFrustum(const Mtx44 &boxTransform, const Vector3 &maxs, const Vector3 &mins);

	void ScreenToWorld(s32 screenX, s32 screenY, Vector3 &nearPlanePos, Vector3 &farPlanePos);

	const Mtx44 &GetViewMatrix() const { return m_viewMatrix; }
	const Mtx44 &GetInvViewMatrix() const { return m_cameraMatrix; }
	const Mtx44 &GetProjection() const { return m_projectionMatrix; }
	const Mtx44 &GetInvProjection() const { return m_invProjectionMatrix; }

	const Frustum &GetFrustum() const { return m_frustum; }

protected:
	Mtx44	m_projectionMatrix;
	Mtx44	m_invProjectionMatrix;
	Mtx44	m_cameraMatrix;		//	the orientation of the camera
	Mtx44	m_viewMatrix;		//	the transform from world space to camera space (inverse camera matrix)
	f32		m_near;
	f32		m_far;
	Frustum	m_frustum;
};

extern CameraBase	*g_camera;	//	the last applied camera