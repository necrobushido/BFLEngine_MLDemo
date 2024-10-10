inline Mtx33::Mtx33()
{
}

inline Mtx33 Mtx33::Transpose() const
{
	Mtx33	result;
	Transpose(result);

	return result;
}

inline Mtx33 Mtx33::OrthoNormalInvert()
{
	Mtx33	result = Transpose();

	return result;
}

inline Mtx33 Mtx33::Inverse() const
{
	Mtx33	returnValue;
	Inverse(returnValue);
	
	return returnValue;
}

inline Mtx33 Mtx33::Multiply(const Mtx33& rhs) const
{
	Mtx33	result;
	Multiply(rhs, result);

	return result;
}

inline Vector3 Mtx33::MultiplyVec( const Vector3 &vec ) const
{
	Vector3 result;

    MultiplyVec(vec, result);

	return result;
}

inline void Mtx33::ResetScale()
{
	Vector3& right = *((Vector3*)(&m[0]));
	Vector3& up = *((Vector3*)(&m[1]));
	Vector3& forward = *((Vector3*)(&m[2]));

	right.Normalize();
	up.Normalize();
	forward.Normalize();
}

inline Vector3 Mtx33::GetScale() const
{
	Vector3& right = *((Vector3*)(&m[0]));
	Vector3& up = *((Vector3*)(&m[1]));
	Vector3& forward = *((Vector3*)(&m[2]));

	Vector3	result;
	result.x = right.Length();
	result.y = up.Length();
	result.z = forward.Length();

	return result;
}

inline void Mtx33::SetScale(const Vector3& inputScale)
{
	ResetScale();

	Vector3& right = *((Vector3*)(&m[0]));
	Vector3& up = *((Vector3*)(&m[1]));
	Vector3& forward = *((Vector3*)(&m[2]));

	right *= inputScale.x;
	up *= inputScale.y;
	forward *= inputScale.z;
}

inline void Mtx33::SetScale(coord_type scalar)
{
	ResetScale();

	Vector3& right = *((Vector3*)(&m[0]));
	Vector3& up = *((Vector3*)(&m[1]));
	Vector3& forward = *((Vector3*)(&m[2]));

	right *= scalar;
	up *= scalar;
	forward *= scalar;
}

inline void Mtx33::SetRight(const Vector3 &vec)
{
	*((Vector3*)(&m[0])) = vec;
}

inline void Mtx33::SetUp(const Vector3 &vec)
{
	*((Vector3*)(&m[1])) = vec;
}

inline void Mtx33::SetForward(const Vector3 &vec)
{
	*((Vector3*)(&m[2])) = vec;
}

inline const Vector3& Mtx33::GetRight() const
{
	return *((Vector3*)(&m[0]));
}

inline const Vector3& Mtx33::GetUp() const
{
	return *((Vector3*)(&m[1]));
}

inline const Vector3& Mtx33::GetForward() const
{
	return *((Vector3*)(&m[2]));
}

inline Mtx33 Mtx33::operator*(const Mtx33 &rhs) const
{
	return Multiply(rhs);
}