#pragma once

#include "Vector3.h"
#include "Mtx33.h"
#include "Mtx43.h"

class btTransform;

class Mtx44
{
public:
	union
    {
        struct
        {
            coord_type _00, _01, _02, _03;
            coord_type _10, _11, _12, _13;
            coord_type _20, _21, _22, _23;
			coord_type _30, _31, _32, _33;
        };
        coord_type m[4][4];
        coord_type a[16];
    };

	static const Mtx44	IDENTITY;

public:
	Mtx44();
	Mtx44(const btTransform *from);
	Mtx44(const coord_type *from);

public:
	void Identity();

	Mtx44 Transpose();
	void Transpose(Mtx44 &out) const;

	Mtx44 Inverse() const;
	void Inverse( Mtx44& out ) const;

	Mtx44 Inverse43() const;
	void Inverse43( Mtx44& out ) const;

	Mtx44 Multiply(const Mtx44 &rhs) const; //This matrix operates as the lhs
	void Multiply(const Mtx44 &rhs, Mtx44 &out) const; //This matrix operates as the lhs

	Vector3 MultiplyVec43(const Vector3 &vec, bool translate = true) const;
	void MultiplyVec43(const Vector3 &vec, Vector3 &out, bool translate = true) const;
	void MultiplyVec(const Vector3 &vec, Vector3 &out) const;
	
	void ConstructPitchRotation(f32 pitch);
	void ConstructYawRotation(f32 yaw);	
	void ConstructRollRotation(f32 roll);
	void ConstructTransform(f32 pitch, f32 yaw, f32 roll, Vector3 translation);

	void SetTranslation3( const Vector3 &vec );
	const Vector3 &GetTranslation3() const;

	void ResetScale();
	Vector3 GetScale() const;
	void SetScale(const Vector3& inputScale);
	void SetScale(f32 scalar);

	void SetRight(const Vector3 &vec);	
	void SetUp(const Vector3 &vec);
	void SetForward(const Vector3 &vec);

	const Vector3 &GetRight();
	const Vector3 &GetUp();
	const Vector3 &GetForward();

	Mtx33 ExtractMtx33() const;
	void SetFromMtx33(const Mtx33 &from);

	Mtx43 ExtractMtx43() const;
	void SetFromMtx43(const Mtx43 &from);

	void Make43Like();

	Mtx44 OrthoNormalInvert();
	Mtx33 ExtractOrthoNormalMtx33() const;

	void WriteToString(char* outputBuffer);

	bool IsEqualTo(const Mtx44 &ref, f32 tolerance) const;
	bool operator==(const Mtx44 &ref) const;
	Mtx44 operator*(const Mtx44 &rhs) const;
	void operator*=(const Mtx44 &rhs);
	void operator+=(const Mtx44 &rhs);
	void operator-=(const Mtx44 &rhs);

	Mtx44 operator*(const f32 &scalar) const;
	Mtx44 operator+(const Mtx44 &rhs) const;
	Mtx44 operator-(const Mtx44 &rhs) const;

	void AssignToBTTransform(btTransform &btT) const;
};

#include "Mtx44.inl"