#pragma once

#include "Vector3.h"

class Mtx33
{
public:
	enum
	{
		kRowCount = 3,
		kColumnCount = 3,
		kElementCount = kRowCount * kColumnCount
	};

    union
    {
        struct
        {
            coord_type _00, _01, _02;
            coord_type _10, _11, _12;
            coord_type _20, _21, _22;
        };
        coord_type	m[kRowCount][kColumnCount];
        coord_type	a[kElementCount];
    };

public:
	Mtx33();
	Mtx33(const coord_type* from);

public:
	void Identity();

	Mtx33 Transpose() const;
	void Transpose( Mtx33& out ) const;

	Mtx33 Inverse() const;
	void Inverse( Mtx33& out ) const;

	Mtx33 Multiply(const Mtx33& rhs) const; //This matrix operates as the lhs
	void Multiply(const Mtx33& rhs, Mtx33& out) const; //This matrix operates as the lhs

	Vector3 MultiplyVec( const Vector3& vec ) const;
	void MultiplyVec( const Vector3& vec, Vector3& out ) const;

	void ResetScale();
	Vector3 GetScale() const;
	void SetScale(const Vector3& inputScale);
	void SetScale(coord_type scalar);

	void ConstructPitchRotation(coord_type pitch);
	void ConstructYawRotation(coord_type yaw);	
	void ConstructRollRotation(coord_type roll);
	void ConstructEulerRotation(coord_type pitch, coord_type yaw, coord_type roll);
	void ConstructEulerRotation(const Vector3& eulerAngles);

	void GetEulerAngles(coord_type* pitchOut, coord_type* yawOut, coord_type* rollOut) const;

	//	for indexing the output Vector3 here on GetEulerAngles
	enum
	{
		kPitch,
		kYaw,
		kRoll
	};
	void GetEulerAngles(Vector3* eulerAnglesOut) const;

	void operator/=( coord_type divisor );
	Mtx33 operator*(const Mtx33& rhs) const;

	Mtx33 OrthoNormalInvert();

	const Vector3& GetRight() const;
	const Vector3& GetUp() const;
	const Vector3& GetForward() const;

	void SetRight(const Vector3& vec);	
	void SetUp(const Vector3& vec);
	void SetForward(const Vector3& vec);

	bool HandednessCheck() const;

	static const Mtx33	IDENTITY;
};

#include "Mtx33.inl"