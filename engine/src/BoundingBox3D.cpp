#include "BoundingBox3D.h"
#include "Renderer.h"
#include "ModelData.h"

BoundingBox3D::BoundingBox3D()
{
	const f32	kInfinity = 100000000000000000.0f;
	min = Vector3(kInfinity, kInfinity, kInfinity);
	max = Vector3(-kInfinity, -kInfinity, -kInfinity);
}

void BoundingBox3D::AddPoint(const Vector3& point)
{
	for(int i = 0; i < 3; i++)
	{
		if( point[i] < min[i] )
		{
			min[i] = point[i];
		}

		if( point[i] > max[i] )
		{
			max[i] = point[i];
		}
	}
}

bool BoundingBox3D::ContainsPoint(const Vector3& point, f32 epsilon)
{
	bool	result = true;
	for(int i = 0; i < 3; i++)
	{
		result = result && (point[i] + epsilon) >= min[i] && (point[i] - epsilon) <= max[i];
	}

	return result;
}

bool BoundingBox3D::IsValid()
{
	bool	returnValue = true;
	for(int i = 0; i < 3 && returnValue; i++)
	{
		returnValue = min[i] < max[i];
	}

	return returnValue;
}

void BoundingBox3D::SetFromModel(const ModelData& modelData)
{
	for(int i = 0; i < modelData.meshCount; i++)
	{
		const MeshData	*meshData = modelData.meshPtrs[i];
		for(u32 j = 0; j < meshData->vertBuffer.m_vertCount; j++)
		{
			int				vertSize = GetVertSize(meshData->vertBuffer.m_vertType);
			const u8		*vertData = &((const u8*)meshData->vertBuffer.m_pData)[j * vertSize];
			const Vector3	*vertPosition = (const Vector3*)vertData;	//	the position is always the first part of the vert
			
			AddPoint(*vertPosition);
		}
	}
}

void BoundingBox3D::Scale(f32 scalar)
{
	min *= scalar;
	max *= scalar;
}

void BoundingBox3D::Draw(bool fullLines)
{
	if( !IsValid() )
		return;

	Vector3	extents(max.x - min.x, max.y - min.y, max.z - min.z);
	Vector3	lineLengths = extents;

	if( !fullLines )
	{
		lineLengths *= 0.1f;
	}

	for(int i = 0; i < 8; i++)
	{
		Vector3	lineOrigin = min;
		lineOrigin.x += extents.x * ((i & (1 << 0)) ? 1 : 0);
		lineOrigin.y += extents.y * ((i & (1 << 1)) ? 1 : 0);
		lineOrigin.z += extents.z * ((i & (1 << 2)) ? 1 : 0);

		for(int j = 0; j < 3; j++)
		{
			Vector3	lineEnd = lineOrigin;
			lineEnd[j] += lineLengths[j] * ((i & (1 << j)) ? -1 : 1);

			//Renderer::DrawLine(&lineOrigin, &lineEnd, &Color4::WHITE);
			Renderer::DrawLine(&lineOrigin, &lineEnd);
		}
	}
}

bool BoundingBox3D::IntersectsBox(const BoundingBox3D& otherBox)
{
	for(int i = 0; i < 3; ++i)
	{
		if( max[i] < otherBox.min[i] )
		{
			return false;
		}

		if( min[i] > otherBox.max[i] )
		{
			return false;
		}
	}

	return true;
}