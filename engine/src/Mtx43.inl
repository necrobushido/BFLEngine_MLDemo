inline Mtx43::Mtx43()
{
}

inline Mtx43 Mtx43::Inverse() const
{
	Mtx43	result;
	Inverse(result);

	return result;
}

inline Mtx43 Mtx43::Multiply(const Mtx43& rhs) const
{
	Mtx43	result;
	Multiply(rhs, result);

	return result;
}

inline Vector3 Mtx43::MultiplyVec( const Vector3& vec, bool translate ) const
{
	Vector3	result;
	MultiplyVec(vec, result, translate);

	return result;
}

inline void Mtx43::SetTranslation( const Vector3& vec )
{
	*((Vector3*)(&m[3])) = vec;
}

inline void Mtx43::ResetScale()
{
	Vector3& right = *((Vector3*)(&m[0]));
	Vector3& up = *((Vector3*)(&m[1]));
	Vector3& forward = *((Vector3*)(&m[2]));

	right.Normalize();
	up.Normalize();
	forward.Normalize();
}

inline Vector3 Mtx43::GetScale() const
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

inline void Mtx43::SetScale(const Vector3& inputScale)
{
	ResetScale();

	Vector3& right = *((Vector3*)(&m[0]));
	Vector3& up = *((Vector3*)(&m[1]));
	Vector3& forward = *((Vector3*)(&m[2]));

	right *= inputScale.x;
	up *= inputScale.y;
	forward *= inputScale.z;
}

inline void Mtx43::SetScale(f32 scalar)
{
	ResetScale();

	Vector3& right = *((Vector3*)(&m[0]));
	Vector3& up = *((Vector3*)(&m[1]));
	Vector3& forward = *((Vector3*)(&m[2]));

	right *= scalar;
	up *= scalar;
	forward *= scalar;
}

inline void Mtx43::SetRight(const Vector3& vec)
{
	*((Vector3*)(&m[0])) = vec;
}

inline void Mtx43::SetUp(const Vector3& vec)
{
	*((Vector3*)(&m[1])) = vec;
}

inline void Mtx43::SetForward(const Vector3& vec)
{
	*((Vector3*)(&m[2])) = vec;
}

inline const Vector3& Mtx43::GetRight() const
{
	return *((Vector3*)(&m[0]));
}

inline const Vector3& Mtx43::GetUp() const
{
	return *((Vector3*)(&m[1]));
}

inline const Vector3& Mtx43::GetForward() const
{
	return *((Vector3*)(&m[2]));
}

inline Mtx43 Mtx43::operator*(const Mtx43& rhs) const
{
	return Multiply(rhs);
}

inline void Mtx43::operator*=(const Mtx43& rhs)
{
	Mtx43	result;
	Multiply(rhs, result);
	*this = result;
}