#pragma once

#include "types.h"
#include "DrawModes.h"

class VertexBufferBase;
class IndexBufferBase;
class Mtx44;
class Vector3;
struct Color4;
class DefaultShaderProgram;

namespace Renderer
{
	void Init();
	void Shutdown();

	void PreRender();
	void PostRender();

	DefaultShaderProgram *GetDefaultShaderProgram();

	void SetModelMatrix(const Mtx44 *inputModelMtx);
	void SetViewMatrix(const Mtx44 *inputViewMtx);
	void SetProjectionMatrix(const Mtx44 *inputProjMtx);
	void SetSkinningMatrices(const Mtx44 *matrices, u32 count);

	const Mtx44 *GetModelMatrix();
	const Mtx44 *GetViewMatrix();
	const Mtx44 *GetModelViewMatrix();
	const Mtx44 *GetProjectionMatrix();

	void SetAttributes(VertexBufferBase *pBuffer);
	void DrawBuffer(eDrawMode mode, VertexBufferBase *pBuffer);
	void DrawBuffer(eDrawMode mode, VertexBufferBase *pBuffer, IndexBufferBase *pIndexBuffer);
	void DrawBuffer(eDrawMode mode, VertexBufferBase *pBuffer, IndexBufferBase *pIndexBuffer, u32 indexStart, u32 indexEnd);
	void DrawBuffer(eDrawMode mode, VertexBufferBase *pBuffer, u32 vao);
	void DrawBuffer(eDrawMode mode, u32 indexCount, u32 vao);
	void DrawLine(const Vector3 *from, const Vector3 *to);
	void DrawLine(const Vector3 *from, const Vector3 *to, const Color4 *color);
	void DrawPoint(const Vector3 *point);
	void DrawPoint(const Vector3 *point, const Color4 *color);
	void DrawSphere(f32 radius);

	void EnableDepthTest(bool enable);	//	compare against depth buffer and write to it when passing (if depth write is enabled)
	void EnableStencilTest(bool enable);

	void EnableDepthWrite(bool enable);	//	write to depth buffer after successful test?
	void EnableFaceCull(bool enable);
	void EnableBlend(bool enable);

	void ClearColorBuffer();
	void ClearDepthBuffer();
	void ClearStencilBuffer();
	void ClearFrameBuffer();

	enum eBlendEquation
	{
		kBlendEquation_Add,
		kBlendEquation_Subtract,
		kBlendEquation_ReverseSubtract,
		kBlendEquation_Min,
		kBlendEquation_Max
	};
	void SetBlendEquation(eBlendEquation blendEq);

	//	this list isn't exhaustive; there are some as yet unimplemented
	enum eBlendFunc
	{
		kBlendFunc_Zero,
		kBlendFunc_One,
		kBlendFunc_SrcAlpha,
		kBlendFunc_InvSrcAlpha
	};
	void SetBlendFunc(eBlendFunc srcBlendFunc, eBlendFunc dstBlendFunc);

	enum eFaceWindingMode
	{
		//kFaceWinding_None,			//	deal with this later if needed
		kFaceWinding_CounterClockwise,	//	default value on startup
		kFaceWinding_Clockwise
	};
	void SetFaceWinding(eFaceWindingMode mode);

	enum eFaceCullMode
	{
		kFaceCull_Front,
		kFaceCull_Back,
		kFaceCull_Both
	};
	void SetFaceCullMode(eFaceCullMode mode);

	u32 GetScreenWidth();
	u32 GetScreenHeight();

	void SetVSync(int interval = 1);

	extern const int	kGlobalUniformBindingIndex;
	extern const char	*kGlobalUniformLabel;
}