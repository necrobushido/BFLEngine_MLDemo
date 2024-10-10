#pragma once

#include "CameraBase.h"

class ViewOriginCamera : public CameraBase
{
public:
	ViewOriginCamera();

public:
	void Update(f64 deltaTime);
	void SetViewFocus(const Vector3& viewFocus);

protected:
	f64		m_theta;
	f64		m_phi;
	f64		m_camDist;
	Vector3	m_viewFocus;
};