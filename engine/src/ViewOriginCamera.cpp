#include "ViewOriginCamera.h"

#include "Input.h"

ViewOriginCamera::ViewOriginCamera()
{
	m_theta = 0.0f;
	m_phi = 0.0f;
	m_camDist = 500.0f;

	m_viewFocus = Vector3::ZERO;
}

void ViewOriginCamera::Update(f64 deltaTime)
{
	const f64	kRotateSpeedFactor = 1.0f;
	const f64	kZoomSpeedFactor = 1000.0f;
	if( Input::KeyHeld('J') )
	{
		m_theta -= deltaTime * kRotateSpeedFactor;
	}
	if( Input::KeyHeld('L') )
	{
		m_theta += deltaTime * kRotateSpeedFactor;
	}
	if( Input::KeyHeld('I') )
	{
		m_phi -= deltaTime * kRotateSpeedFactor;
	}
	if( Input::KeyHeld('K') )
	{
		m_phi += deltaTime * kRotateSpeedFactor;
	}
	if( Input::KeyHeld('U') )
	{
		m_camDist -= deltaTime * kZoomSpeedFactor;
		if( m_camDist < m_near )
		{
			m_camDist = m_near;
		}
	}
	if( Input::KeyHeld('O') )
	{
		m_camDist += deltaTime * kZoomSpeedFactor;
	}
	//	camera position	
	Vector3	camPos((coord_type)(sin(m_theta) * cos(m_phi)), (coord_type)sin(m_phi), (coord_type)(cos(m_theta) * cos(m_phi)));
	camPos *= (coord_type)m_camDist;
	camPos += m_viewFocus;
	SetPosition(camPos);

	LookAt(m_viewFocus);
}

void ViewOriginCamera::SetViewFocus(const Vector3& viewFocus)
{
	m_viewFocus = viewFocus;
}