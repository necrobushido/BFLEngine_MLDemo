#include "Renderer.h"

#include "VertexBufferBase.h"
#include "Mtx44.h"
#include "Vector3.h"
#include "Color.h"
#include "CameraBase.h"
#include "ModelData.h"
#include "DefaultShaderProgram.h"

#define GLEW_STATIC
#include "gl\glew.h"

namespace Renderer
{
	DefaultShaderProgram*	pDefaultShaderProgram = NULL;

	GLuint		globalUniformBufferObject;	
	const int	kGlobalUniformBindingIndex = 0;
	const char*	kGlobalUniformLabel = "GlobalMatrices";

	const u32	kMaxBones = 124;	//	can increase this number in the shader if necessary

	/*
	If the member is an array of scalars or vectors, 
	the base alignment and array stride are set to match the base alignment of a single array element, 
	according to rules (1), (2), and (3), 
	and rounded up to the base alignment of a vec4. 
	The array may have padding at the end; 
	the base offset of the member following the array is rounded up to the next multiple of the base alignment.
	*/
	struct GlobalMatrices
	{
		Mtx44	viewMatrix;
		Mtx44	modelViewMatrix;
		Mtx44	modelViewProjMatrix;
		Mtx44	normalModelViewMatrix;
		Mtx44	skinningMatrices[kMaxBones];
	};

	GlobalMatrices		globalMatrices;
	Mtx44				projectionMatrix;
	Mtx44				modelMtx;

	void Init()
	{
		glewExperimental = TRUE;
		GLenum	err = glewInit();
		if( GLEW_OK != err )
		{
			//	Problem: glewInit failed, something is seriously wrong.
			_Panicf("Error: %s\n", glewGetErrorString(err));
		}
		DebugPrintf("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glShadeModel(GL_SMOOTH);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClearDepth(1.0f);
		glClearStencil(0);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glEnable(GL_MULTISAMPLE);

		//glDepthFunc(GL_LESS);
		glDepthFunc(GL_LEQUAL);

		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

		{
			glGenBuffers(1, &globalUniformBufferObject);
			glBindBuffer(GL_UNIFORM_BUFFER, globalUniformBufferObject);
			glBufferData(GL_UNIFORM_BUFFER, sizeof(GlobalMatrices), NULL, GL_STREAM_DRAW);	//	allocates buffer data, but no initialization
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			//	map the buffer bind target (kGlobalUniformBindingIndex) to the memory allocated (globalUniformBufferObject)
			//int	bufferOffset = 0;
			//glBindBufferRange(GL_UNIFORM_BUFFER, kGlobalUniformBindingIndex, globalUniformBufferObject, bufferOffset, sizeof(GlobalMatrices));
			glBindBufferBase(GL_UNIFORM_BUFFER, kGlobalUniformBindingIndex, globalUniformBufferObject);

			GLint	maxUniformBufferSize;
			glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBufferSize);
			//DebugPrintf("maxUniformBufferSize = %d, sizeof(GlobalMatrices) = %d\n", maxUniformBufferSize, sizeof(GlobalMatrices));
			Assert(sizeof(GlobalMatrices) <= maxUniformBufferSize);
		}

		pDefaultShaderProgram = new DefaultShaderProgram();

		if( 0 )
		{
			GLint value;

			glGetActiveUniformBlockiv(pDefaultShaderProgram->GetProgramObject(), 0, GL_UNIFORM_BLOCK_BINDING, &value);
			DebugPrintf("GL_UNIFORM_BLOCK_BINDING = %d\n", value);

			glGetActiveUniformBlockiv(pDefaultShaderProgram->GetProgramObject(), 0, GL_UNIFORM_BLOCK_DATA_SIZE, &value);
			DebugPrintf("GL_UNIFORM_BLOCK_DATA_SIZE = %d\n", value);

			glGetActiveUniformBlockiv(pDefaultShaderProgram->GetProgramObject(), 0, GL_UNIFORM_BLOCK_NAME_LENGTH, &value);
			DebugPrintf("GL_UNIFORM_BLOCK_NAME_LENGTH = %d\n", value);

			glGetActiveUniformBlockiv(pDefaultShaderProgram->GetProgramObject(), 0, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &value);
			DebugPrintf("GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS = %d\n", value);
		}
	}

	void Shutdown()
	{
		delete pDefaultShaderProgram;
		glDeleteBuffers(1, &globalUniformBufferObject);
	}

	void PreRender()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void PostRender()
	{
	}

	DefaultShaderProgram *GetDefaultShaderProgram()
	{
		return pDefaultShaderProgram;
	}

	void SetModelMatrix(const Mtx44 *inputModelMtx)
	{
		modelMtx = *inputModelMtx;
		globalMatrices.modelViewMatrix = modelMtx * globalMatrices.viewMatrix;
		globalMatrices.modelViewProjMatrix = globalMatrices.modelViewMatrix * projectionMatrix;

		glBindBuffer(GL_UNIFORM_BUFFER, globalUniformBufferObject);

		//	set the modelView matrix
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(Mtx44), sizeof(Mtx44), globalMatrices.modelViewMatrix.a);

		//	set the modelViewProj matrix
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(Mtx44)*2, sizeof(Mtx44), globalMatrices.modelViewProjMatrix.a);

		//	set the modelView matrix for normals
		//globalMatrices.normalModelViewMatrix = modelViewMatrix.ExtractMtx33();
		globalMatrices.normalModelViewMatrix = globalMatrices.modelViewMatrix;
		globalMatrices.normalModelViewMatrix = globalMatrices.normalModelViewMatrix.Inverse().Transpose();	//	this is only necessary if we're using non-uniform scale
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(Mtx44)*3, sizeof(Mtx44), globalMatrices.normalModelViewMatrix.a);

		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void SetViewMatrix(const Mtx44 *inputViewMtx)
	{
		globalMatrices.viewMatrix = *inputViewMtx;

		glBindBuffer(GL_UNIFORM_BUFFER, globalUniformBufferObject);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Mtx44), globalMatrices.viewMatrix.a);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	void SetProjectionMatrix(const Mtx44 *inputProjMtx)
	{
		projectionMatrix = *inputProjMtx;
	}

	void SetSkinningMatrices(const Mtx44 *matrices, u32 count)
	{		
		Assert(count <= kMaxBones);
		if( count > kMaxBones )
		{
			count = kMaxBones;
		}

		memcpy(globalMatrices.skinningMatrices, matrices, sizeof(Mtx44) * count);	//	could probably skip this if it ends up being costly?

		glBindBuffer(GL_UNIFORM_BUFFER, globalUniformBufferObject);
		int	bufferPos = sizeof(Mtx44) * 4;
		glBufferSubData(GL_UNIFORM_BUFFER, bufferPos, sizeof(Mtx44) * count, globalMatrices.skinningMatrices[0].a);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	const Mtx44 *GetModelMatrix()
	{
		return &modelMtx;
	}

	const Mtx44 *GetViewMatrix()
	{
		return &globalMatrices.viewMatrix;
	}

	const Mtx44 *GetModelViewMatrix()
	{
		return &globalMatrices.modelViewMatrix;
	}

	const Mtx44 *GetProjectionMatrix()
	{
		return &projectionMatrix;
	}

	void SetAttributes(VertexBufferBase *pBuffer)
	{
		const int			vertFlags = pBuffer->GetType();
		const int			vertSize = GetVertSize(pBuffer->GetType());
		u64					vertOffset = 0;

		//	position is always set, and occupies channel 0
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, s_channelSizes[0], GL_FLOAT, GL_FALSE, vertSize, (void*)(vertOffset));
		vertOffset += s_channelSizes[0] * sizeof(f32);

		//	check the other channels
		for(int i = 1; i < kChannel_Count; i++)
		{
			if( vertFlags & (1 << (i - 1)) )
			{
				glEnableVertexAttribArray(i);
				glVertexAttribPointer(i, s_channelSizes[i], GL_FLOAT, GL_FALSE, vertSize, (void*)(vertOffset));
				vertOffset += s_channelSizes[i] * sizeof(f32);
			}
		}
	}

	void DrawBuffer(eDrawMode mode, VertexBufferBase *pBuffer)
	{
		glBindBuffer(GL_ARRAY_BUFFER, pBuffer->GetBufferIndex());
		SetAttributes(pBuffer);

		glDrawArrays((GLenum)GetGLDrawMode(mode), 0, pBuffer->GetCount());
		glBindVertexArray(0);

		for(int i = 0; i < kChannel_Count; i++)
		{
			glDisableVertexAttribArray(i);
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void DrawBuffer(eDrawMode mode, VertexBufferBase *pBuffer, IndexBufferBase *pIndexBuffer)
	{
		glBindBuffer(GL_ARRAY_BUFFER, pBuffer->GetBufferIndex());
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pIndexBuffer->GetBufferIndex());		
		SetAttributes(pBuffer);

		glDrawElements((GLenum)GetGLDrawMode(mode), pIndexBuffer->GetCount(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		for(int i = 0; i < kChannel_Count; i++)
		{
			glDisableVertexAttribArray(i);
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void DrawBuffer(eDrawMode mode, VertexBufferBase *pBuffer, IndexBufferBase *pIndexBuffer, u32 indexStart, u32 indexEnd)
	{
		glBindBuffer(GL_ARRAY_BUFFER, pBuffer->GetBufferIndex());
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pIndexBuffer->GetBufferIndex());		
		SetAttributes(pBuffer);

		glDrawRangeElements((GLenum)GetGLDrawMode(mode), indexStart, indexEnd, (indexEnd - indexStart), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		for(int i = 0; i < kChannel_Count; i++)
		{
			glDisableVertexAttribArray(i);
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void DrawBuffer(eDrawMode mode, VertexBufferBase *pBuffer, u32 vao)
	{
		glBindVertexArray(vao);
		glDrawArrays((GLenum)GetGLDrawMode(mode), 0, pBuffer->GetCount());
		glBindVertexArray(0);
	}

	void DrawBuffer(eDrawMode mode, u32 indexCount, u32 vao)
	{
		glBindVertexArray(vao);
		glDrawElements((GLenum)GetGLDrawMode(mode), indexCount, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	void DrawLine(const Vector3 *from, const Vector3 *to)
	{
		glLineWidth(1.0f);
		glBegin(GL_LINES);
			glVertex3fv((const f32*)from);
			glVertex3fv((const f32*)to);
		glEnd();
	}

	void DrawLine(const Vector3 *from, const Vector3 *to, const Color4 *color)
	{
		glLineWidth(1.0f);
		glBegin(GL_LINES);
			glVertex3fv((const f32*)from);
			glColor4fv((const f32*)color);
			glVertex3fv((const f32*)to);
			glColor4fv((const f32*)color);
		glEnd();
	}

	void DrawPoint(const Vector3 *point, const Color4 *color)
	{
		glPointSize(4.0f);
		glBegin(GL_POINTS);
			glVertex3fv((const f32*)point);
			glColor4fv((const f32*)color);
		glEnd();
	}

	void DrawPoint(const Vector3 *point)
	{
		glPointSize(4.0f);
		glBegin(GL_POINTS);
			glVertex3fv((const f32*)point);
		glEnd();
	}

	void DrawSphere(f32 radius)
	{
		const s32		slices = 10;
		const s32		stacks = 10;
		GLUquadric		*quadric = gluNewQuadric();

		gluSphere(quadric, radius, slices, stacks);
		gluDeleteQuadric(quadric);
	}

	void EnableDepthTest(bool enable)
	{
		if( enable )
		{
			glEnable(GL_DEPTH_TEST);
		}
		else
		{
			glDisable(GL_DEPTH_TEST);
		}
	}

	void EnableStencilTest(bool enable)
	{
		if( enable )
		{
			glEnable(GL_STENCIL_TEST);
		}
		else
		{
			glDisable(GL_STENCIL_TEST);
		}
	}

	void EnableDepthWrite(bool enable)
	{
		glDepthMask(enable);
	}

	void EnableFaceCull(bool enable)
	{
		if( enable )
		{
			glEnable(GL_CULL_FACE);
		}
		else
		{
			glDisable(GL_CULL_FACE);
		}
	}

	void EnableBlend(bool enable)
	{
		if( enable )
		{
			glEnable(GL_BLEND);
		}
		else
		{
			glDisable(GL_BLEND);
		}
	}

	void ClearColorBuffer()
	{
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void ClearDepthBuffer()
	{
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	void ClearStencilBuffer()
	{
		glClear(GL_STENCIL_BUFFER_BIT);
	}

	void ClearFrameBuffer()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	void SetBlendEquation(eBlendEquation blendEq)
	{
		const GLenum	equations[] =
		{
			GL_FUNC_ADD,
			GL_FUNC_SUBTRACT,
			GL_FUNC_REVERSE_SUBTRACT,
			GL_MIN,
			GL_MAX
		};

		glBlendEquation(equations[blendEq]);
	}

	void SetBlendFunc(eBlendFunc srcBlendFunc, eBlendFunc dstBlendFunc)
	{
		const GLenum	funcs[] =
		{
			GL_ZERO,
			GL_ONE,
			GL_SRC_ALPHA,
			GL_ONE_MINUS_SRC_ALPHA
		};

		glBlendFunc(funcs[srcBlendFunc], funcs[dstBlendFunc]);	
	}

	void SetFaceWinding(eFaceWindingMode mode)
	{
		glFrontFace((mode == kFaceWinding_CounterClockwise) ? GL_CCW : GL_CW);
	}

	void SetFaceCullMode(eFaceCullMode mode)
	{
		const GLenum	modes[] =
		{
			GL_FRONT,
			GL_BACK,
			GL_FRONT_AND_BACK
		};

		glCullFace(modes[mode]);
	}

	u32 GetScreenWidth()
	{
		GLint	viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);		

		return (u32)viewport[2];
	}

	u32 GetScreenHeight()
	{
		GLint	viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);		

		return (u32)viewport[3];
	}

	//	vsync code copy/pasted from online
	typedef BOOL (APIENTRY *PFNWGLSWAPINTERVALFARPROC)( int );
	PFNWGLSWAPINTERVALFARPROC wglSwapIntervalEXT = 0;

	void SetVSync(int interval)
	{
		const char*	extensions = (const char*)glGetString( GL_EXTENSIONS );

		if( strstr( extensions, "WGL_EXT_swap_control" ) == 0 )
		{
			return; // Error: WGL_EXT_swap_control extension not supported on your computer.\n");
		}
		else
		{
			wglSwapIntervalEXT = (PFNWGLSWAPINTERVALFARPROC)wglGetProcAddress( "wglSwapIntervalEXT" );

			if( wglSwapIntervalEXT )
			{
				wglSwapIntervalEXT(interval);
			}
		}
	}
}