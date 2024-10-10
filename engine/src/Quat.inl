
inline Quat::Quat()
{
	//	explicitly do nothing in case we're using this in tool data
}

inline Quat::Quat(coord_type n, coord_type a, coord_type b, coord_type c)
{
	w = n;
	x = a;
	y = b;
	z = c;
}

inline Quat::Quat(coord_type angleX, coord_type angleY, coord_type angleZ)
{
	Quat	qx = Quat(angleX, Vector3(1.0f, 0.0f, 0.0f));
	Quat	qy = Quat(angleY, Vector3(0.0f, 1.0f, 0.0f));
	Quat	qz = Quat(angleZ, Vector3(0.0f, 0.0f, 1.0f));
	Quat	qxy = qx * qy;
	Quat	qxyz = qxy * qz;

	*this = qxyz;
	Normalize();
}

inline Vector3 Quat::GetVector()
{
	return Vector3(x, y, z);
}

inline coord_type Quat::GetScalar()
{
	return w;
}

inline void Quat::Normalize()
{
	coord_type	m = Magnitude();

	w /= m;
	x /= m;
	y /= m;
	z /= m;
}

inline void Quat::Negate()
{
	w *= -1.0f;
	x *= -1.0f;
	y *= -1.0f;
	z *= -1.0f;
}

inline void Quat::NegateIfWNegative()
{
	if( w < 0.0f )
	{
		Negate();
	}
}

inline void Quat::Add(const Quat& a, const Quat& b, Quat* dst)
{
	dst->w = a.w + b.w;
	dst->x = a.x + b.x;
	dst->y = a.y + b.y;
	dst->z = a.z + b.z;
}

inline void Quat::Multiply(const Quat& a, const Quat& b, Quat* dst)
{
	dst->w = (a.w * b.w) - (a.x * b.x) - (a.y * b.y) - (a.z * b.z);
	dst->x = (a.w * b.x) + (a.x * b.w) + (a.y * b.z) - (a.z * b.y);
	dst->y = (a.w * b.y) + (a.y * b.w) + (a.z * b.x) - (a.x * b.z);
	dst->z = (a.w * b.z) + (a.z * b.w) + (a.x * b.y) - (a.y * b.x);
}

inline void Quat::Multiply(const Quat& src, const coord_type scalar, Quat* dst)
{
	dst->w = src.w * scalar;
	dst->x = src.x * scalar;
	dst->y = src.y * scalar;
	dst->z = src.z * scalar;
}

inline void Quat::Conjugate(const Quat& src, Quat* dst)
{
	dst->w = src.w;
	dst->x = -src.x;
	dst->y = -src.y;
	dst->z = -src.z;
}

inline Quat Quat::operator+(const Quat& rhs) const
{
	Quat	result;
	Add(*this, rhs, &result);

	return result;
}

inline Quat Quat::operator*(const Quat& rhs) const
{
	Quat	result;
	Multiply(*this, rhs, &result);

	return result;
}

inline Quat Quat::operator*(const coord_type& rhs) const
{
	Quat	result;

	Multiply(*this, rhs, &result);

	return result;
}

inline Quat Quat::operator*=(const coord_type& rhs)
{
	Multiply(*this, rhs, this);

	return *this;
}

inline void Quat::Transform(const Quat& parentWorld, const Quat& childLocal, Quat* childWorld)
{
	Quat::Multiply(parentWorld, childLocal, childWorld);
}

inline void Quat::InvTransformRight(const Quat& childWorld, const Quat& childLocal, Quat* parentWorld)
{
	Quat	childLocalConj;
	Conjugate(childLocal, &childLocalConj);
	Multiply(childWorld, childLocalConj, parentWorld);
}

inline void Quat::InvTransformLeft(const Quat& childWorld, const Quat& parentWorld, Quat* childLocal)
{
	Quat	parentWorldConj;
	Conjugate(parentWorld, &parentWorldConj);
	Multiply(parentWorldConj, childWorld, childLocal);
}