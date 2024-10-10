inline Mtx44::Mtx44()
{
}

inline Mtx44 Mtx44::Transpose()
{
	Mtx44	result;
	Transpose(result);

	return result;
}

inline Mtx44 Mtx44::Inverse() const
{
	Mtx44	result;
	Inverse(result);

	return result;
}

inline Mtx44 Mtx44::Multiply(const Mtx44 &rhs) const
{
	Mtx44	result;
	Multiply(rhs, result);

	return result;
}

inline Vector3 Mtx44::MultiplyVec43( const Vector3 &vec, bool translate ) const
{
	Vector3	result;
	MultiplyVec43(vec, result, translate);

	return result;
}

inline void Mtx44::SetTranslation3( const Vector3 &vec )
{
	//*((Vector3*)(&_30)) = vec;
	*((Vector3*)(&m[3])) = vec;
	m[3][3] = 1.0f;
}

inline void Mtx44::ResetScale()
{
	Vector3	&right = *((Vector3*)(&m[0]));
	Vector3	&up = *((Vector3*)(&m[1]));
	Vector3	&forward = *((Vector3*)(&m[2]));

	right.Normalize();
	up.Normalize();
	forward.Normalize();
}

inline Vector3 Mtx44::GetScale() const
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

inline void Mtx44::SetScale(const Vector3& inputScale)
{
	ResetScale();

	Vector3& right = *((Vector3*)(&m[0]));
	Vector3& up = *((Vector3*)(&m[1]));
	Vector3& forward = *((Vector3*)(&m[2]));

	right *= inputScale.x;
	up *= inputScale.y;
	forward *= inputScale.z;
}

inline void Mtx44::SetScale(f32 scalar)
{
	ResetScale();

	Vector3	&right = *((Vector3*)(&m[0]));
	Vector3	&up = *((Vector3*)(&m[1]));
	Vector3	&forward = *((Vector3*)(&m[2]));

	right *= scalar;
	up *= scalar;
	forward *= scalar;
}

inline void Mtx44::SetRight(const Vector3 &vec)
{
	//*((Vector3*)(&_00)) = vec;
	*((Vector3*)(&m[0])) = vec;
	m[0][3] = 0.0f;
}

inline void Mtx44::SetUp(const Vector3 &vec)
{
	//*((Vector3*)(&_10)) = vec;
	*((Vector3*)(&m[1])) = vec;
	m[1][3] = 0.0f;
}

inline void Mtx44::SetForward(const Vector3 &vec)
{
	//*((Vector3*)(&_20)) = vec;
	*((Vector3*)(&m[2])) = vec;
	m[2][3] = 0.0f;
}

inline const Vector3 &Mtx44::GetRight()
{
	return *((Vector3*)(&m[0]));
}

inline const Vector3 &Mtx44::GetUp()
{
	return *((Vector3*)(&m[1]));
}

inline const Vector3 &Mtx44::GetForward()
{
	return *((Vector3*)(&m[2]));
}

inline Mtx44 Mtx44::Inverse43() const
{
	Mtx44	result;
	Inverse43(result);

	return result;
}

inline Mtx44 Mtx44::operator*(const Mtx44 &rhs) const
{
	return Multiply(rhs);
}

inline void Mtx44::operator*=(const Mtx44 &rhs)
{
	Mtx44	result;
	Multiply(rhs, result);
	*this = result;
}