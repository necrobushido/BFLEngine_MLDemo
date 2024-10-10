#pragma once

#include "types.h"
#include "Mtx44.h"

class ShaderProgram
{
public:
	ShaderProgram();
	ShaderProgram(const char* vertexShader, const char* fragmentShader);
	ShaderProgram(const char* vertexShader, const char* geometryShader, const char* fragmentShader);
	~ShaderProgram();

public:
	void Apply();
	void Disable();

	GLuint GetProgramObject(){ return m_programObject; }

protected:
	GLuint CreateShader(GLenum eShaderType, const char* strFileName);
	GLuint CreateShaderFromText(GLenum eShaderType, const char *shaderText);
	GLuint CreateProgram(const GLuint* shaderList, const int shaderListCount);

protected:
	GLuint	m_programObject;
};

extern ShaderProgram*	g_currentShader;