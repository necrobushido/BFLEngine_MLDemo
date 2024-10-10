#pragma once

#include "Vector3.h"
#include "Mtx33.h"

class Mtx44;

class Mtx43
{
public:
	enum
	{
		kRowCount = 4,
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
			coord_type _30, _31, _32;
        };
        coord_type m[kRowCount][kColumnCount];
        coord_type a[kElementCount];
    };

	static const Mtx43	IDENTITY;

public:
	Mtx43();
	Mtx43(const coord_type* from);

public:
	void Identity();

	Mtx43 Inverse() const;
	void Inverse( Mtx43& out ) const;

	Mtx43 Multiply(const Mtx43& rhs) const; //This matrix operates as the lhs
	void Multiply(const Mtx43& rhs, Mtx43& out) const; //This matrix operates as the lhs

	Mtx44 Multiply44(const Mtx44& rhs) const; //This matrix operates as the lhs
	void Multiply44(const Mtx44& rhs, Mtx44& out) const; //This matrix operates as the lhs

	Vector3 MultiplyVec(const Vector3& vec, bool translate = true) const;
	void MultiplyVec(const Vector3& vec, Vector3& out, bool translate = true) const;
	
	void ConstructPitchRotation(f32 pitch);
	void ConstructYawRotation(f32 yaw);	
	void ConstructRollRotation(f32 roll);
	void ConstructTransform(f32 pitch, f32 yaw, f32 roll, Vector3 translation);

	void SetTranslation( const Vector3& vec );
	const Vector3& GetTranslation() const;

	void ResetScale();
	Vector3 GetScale() const;
	void SetScale(const Vector3& inputScale);
	void SetScale(f32 scalar);

	void SetRight(const Vector3& vec);
	void SetUp(const Vector3& vec);
	void SetForward(const Vector3& vec);

	const Vector3& GetRight() const;
	const Vector3& GetUp() const;
	const Vector3& GetForward() const;

	Mtx33 ExtractMtx33() const;
	void SetFromMtx33(const Mtx33& from);

	Mtx33 ExtractRotation() const;

	Mtx43 OrthoNormalInvert();

	void WriteToString(char* outputBuffer);

	bool IsEqualTo(const Mtx43& ref, f32 tolerance) const;
	bool operator==(const Mtx43& ref) const;
	Mtx43 operator*(const Mtx43& rhs) const;
	void operator*=(const Mtx43& rhs);
	void operator+=(const Mtx43& rhs);

	Mtx43 operator*(const f32& scalar) const;
	Mtx43 operator+(const Mtx43& rhs) const;
};

#include "Mtx43.inl"