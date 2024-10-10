#include "CameraBase.h"

#include "Renderer.h"

//	maintain a pointer to the last applied camera
CameraBase	*g_camera = NULL;

CameraBase::CameraBase()
{
	m_cameraMatrix.Identity();
	m_viewMatrix.Identity();	
}

CameraBase::~CameraBase()
{
}

void CameraBase::Apply()
{
	Renderer::SetProjectionMatrix(&m_projectionMatrix);
	Renderer::SetViewMatrix(&m_viewMatrix);

	g_camera = this;
}

void CameraBase::SetPerspective(f32 fovY, f32 aspectRatio, f32 zNear, f32 zFar)
{
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluPerspective( fovY, aspectRatio, zNear, zFar );
	glGetFloatv(GL_PROJECTION_MATRIX, m_projectionMatrix.a);
	m_projectionMatrix.Inverse(m_invProjectionMatrix);
	m_near = zNear;
	m_far = zFar;
	m_frustum.Init(fovY, aspectRatio, zNear, zFar);
}

void CameraBase::SetOrtho(f32 left, f32 right, f32 top, f32 bottom, f32 zNear, f32 zFar)
{
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrtho(left, right, bottom, top, zNear, zFar);
	glGetFloatv(GL_PROJECTION_MATRIX, m_projectionMatrix.a);
	m_projectionMatrix.Inverse(m_invProjectionMatrix);
	m_near = zNear;
	m_far = zFar;
	m_frustum.Init(left, right, top, bottom, zNear, zFar);
}

void CameraBase::LookAt(const Vector3 &pos, const Vector3 &up)
{
	Vector3	camPosition = m_cameraMatrix.GetTranslation3();

	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	gluLookAt(camPosition.x, camPosition.y, camPosition.z, pos.x, pos.y, pos.z, up.x, up.y, up.z);
	glGetFloatv(GL_MODELVIEW_MATRIX, m_viewMatrix.a);

	m_cameraMatrix = m_viewMatrix.OrthoNormalInvert();
}

void CameraBase::SetPosition(const Vector3 &pos)
{
	//	normal
	m_cameraMatrix.SetTranslation3(pos);

	//	inverse
	m_viewMatrix = m_cameraMatrix.OrthoNormalInvert();
}

const Vector3 &CameraBase::GetPosition() const
{
	return m_cameraMatrix.GetTranslation3();
}

void CameraBase::Displace(const Vector3 &amount)
{
	//	normal
	m_cameraMatrix._30 += amount.x;
	m_cameraMatrix._31 += amount.y;
	m_cameraMatrix._32 += amount.z;

	//	inverse
	m_viewMatrix._30 -= amount.x;
	m_viewMatrix._31 -= amount.y;
	m_viewMatrix._32 -= amount.z;
}

bool CameraBase::PointInFrustum(const Vector2 &point)
{
	//	convert the point to 3D.  assume a z value
	Vector3	worldSpacePoint(point.x, point.y, -10.0f);

	//	call the 3D version
	return PointInFrustum(worldSpacePoint);
}

bool CameraBase::CircleInFrustum(const Vector2 &center, f32 radius)
{
	//	convert the center to 3D.  assume a z value
	Vector3	worldSpaceCenter(center.x, center.y, -10.0f);

	//	call the 3D version
	return SphereInFrustum(worldSpaceCenter, radius);
}

bool CameraBase::PointInFrustum(const Vector3 &point)
{
	//	transform the point from world space to camera space
	Vector3	camSpacePoint = m_viewMatrix.MultiplyVec43(point, true);
	camSpacePoint.z *= -1.0f;

	//	do the frustum check
	return m_frustum.PointInside(camSpacePoint);
}

bool CameraBase::SphereInFrustum(const Vector3 &center, f32 radius)
{
	//	transform the center from world space to camera space
	Vector3	camSpaceCenter = m_viewMatrix.MultiplyVec43(center, true);
	camSpaceCenter.z *= -1.0f;

	//	do the frustum check
	return m_frustum.SphereInside(camSpaceCenter, radius);
}

void Swap(f32& a, f32& b)
{
	f32	temp = a;
	a = b;
	b = temp;
}

bool CameraBase::BoxInFrustum(const Mtx44 &boxTransform, const Vector3 &maxs, const Vector3 &mins)
{
	//	transform the box points from box space to world space
	Vector3	worldSpaceMaxs = boxTransform.MultiplyVec43(maxs, true);
	Vector3	worldSpaceMins = boxTransform.MultiplyVec43(mins, true);

	//	transform the points from world space to camera space
	Vector3	camSpaceMaxs = m_viewMatrix.MultiplyVec43(worldSpaceMaxs, true);
	Vector3	camSpaceMins = m_viewMatrix.MultiplyVec43(worldSpaceMins, true);
	camSpaceMaxs.z *= -1.0f;
	camSpaceMins.z *= -1.0f;

	if( camSpaceMins.x > camSpaceMaxs.x )
	{
		Swap(camSpaceMins.x, camSpaceMaxs.x);
	}

	if( camSpaceMins.y > camSpaceMaxs.y )
	{
		Swap(camSpaceMins.y, camSpaceMaxs.y);
	}
	
	if( camSpaceMins.z > camSpaceMaxs.z )
	{
		Swap(camSpaceMins.z, camSpaceMaxs.z);
	}


	//	do the frustum check
	return m_frustum.BoxInside(camSpaceMaxs, camSpaceMins);
}

void CameraBase::ScreenToWorld(s32 screenX, s32 screenY, Vector3 &nearPlanePos, Vector3 &farPlanePos)
{
	GLint	viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	//	if this function gets called a lot we could cache this
	Mtx44	invViewProj = m_invProjectionMatrix * m_cameraMatrix;

	f32	screenXf = (f32)screenX;
	f32	screenYf = (f32)screenY;

	//	convert screen coordinates to eye coordinates
	f32	eyeX = ((screenXf * 2) / ((f32)viewport[2])) - 1.0f;
	f32	eyeY = -(((screenYf * 2) / ((f32)viewport[3])) - 1.0f);

	//	multiply through
	Vector3	eyeNear(eyeX, eyeY, 0.0f);
	Vector3	eyeFar(eyeX, eyeY, 1.0f);

	invViewProj.MultiplyVec(eyeNear, nearPlanePos);
	invViewProj.MultiplyVec(eyeFar, farPlanePos);
}