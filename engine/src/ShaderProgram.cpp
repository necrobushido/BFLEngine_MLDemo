#include "ShaderProgram.h"

#include "FileManager.h"
#include "Renderer.h"
#include "DefaultShaders.h"

ShaderProgram*	g_currentShader = NULL;

ShaderProgram::ShaderProgram()
{
	const int	kNumShaders = 2;
	GLuint		shaderArray[kNumShaders];
	shaderArray[0] = CreateShaderFromText(GL_VERTEX_SHADER, s_defaultVertexShader);
	shaderArray[1] = CreateShaderFromText(GL_FRAGMENT_SHADER, s_defaultFragmentShader);
	m_programObject = CreateProgram(shaderArray, kNumShaders);

	for(int i = 0; i < kNumShaders; i++)
	{
		glDeleteShader(shaderArray[i]);
	}
}

ShaderProgram::ShaderProgram(const char* vertexShader, const char* fragmentShader)
{
	const int	kNumShaders = 2;
	GLuint		shaderArray[kNumShaders];
	shaderArray[0] = CreateShader(GL_VERTEX_SHADER, vertexShader);
	shaderArray[1] = CreateShader(GL_FRAGMENT_SHADER, fragmentShader);
	m_programObject = CreateProgram(shaderArray, kNumShaders);

	for(int i = 0; i < kNumShaders; i++)
	{
		glDeleteShader(shaderArray[i]);
	}
}

ShaderProgram::ShaderProgram(const char* vertexShader, const char* geometryShader, const char* fragmentShader)
{
	const int	kNumShaders = 3;
	GLuint		shaderArray[kNumShaders];
	shaderArray[0] = CreateShader(GL_VERTEX_SHADER, vertexShader);
	shaderArray[1] = CreateShader(GL_GEOMETRY_SHADER, geometryShader);
	shaderArray[2] = CreateShader(GL_FRAGMENT_SHADER, fragmentShader);
	m_programObject = CreateProgram(shaderArray, kNumShaders);

	for(int i = 0; i < kNumShaders; i++)
	{
		glDeleteShader(shaderArray[i]);
	}
}

ShaderProgram::~ShaderProgram()
{
	glDeleteProgram(m_programObject);
}

void ShaderProgram::Apply()
{
	glUseProgram(m_programObject);

	g_currentShader = this;
}

void ShaderProgram::Disable()
{
	glUseProgram(0);

	g_currentShader = NULL;
}

GLuint ShaderProgram::CreateShader(GLenum eShaderType, const char *strFileName)
{
	FileRef<char>	shaderFileRef = g_fileManager->MakeRef(strFileName);
	GLuint			shader = glCreateShader(eShaderType);
	const char		*fileData = *shaderFileRef;
	const size_t	fileSize = shaderFileRef.FileSize();
	const GLint		fileSizeGL = (GLint)fileSize;
	
	glShaderSource(shader, 1, &fileData, &fileSizeGL);
	glCompileShader(shader);

	GLint		status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if( status == GL_FALSE )
	{
		GLint	infoLogLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar	*strInfoLog = new GLchar[infoLogLength + 1];
		glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);

		const char	*strShaderType = NULL;
		switch(eShaderType)
		{
		case GL_VERTEX_SHADER: 
			strShaderType = "vertex"; 
			break;

		case GL_GEOMETRY_SHADER: 
			strShaderType = "geometry"; 
			break;

		case GL_FRAGMENT_SHADER: 
			strShaderType = "fragment"; 
			break;
		}

		_Panicf("Compile failure in %s shader:\n%s\n", strShaderType, strInfoLog);
		delete [] strInfoLog;
	}

	return shader;
}

GLuint ShaderProgram::CreateShaderFromText(GLenum eShaderType, const char *shaderText)
{
	GLuint			shader = glCreateShader(eShaderType);
	const char		*fileData = shaderText;
	const size_t	fileSize = strlen(shaderText);
	const GLint		fileSizeGL = (GLint)fileSize;
	
	glShaderSource(shader, 1, &fileData, &fileSizeGL);
	glCompileShader(shader);

	GLint		status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if( status == GL_FALSE )
	{
		GLint	infoLogLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar	*strInfoLog = new GLchar[infoLogLength + 1];
		glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);

		const char	*strShaderType = NULL;
		switch(eShaderType)
		{
		case GL_VERTEX_SHADER: 
			strShaderType = "vertex"; 
			break;

		case GL_GEOMETRY_SHADER: 
			strShaderType = "geometry"; 
			break;

		case GL_FRAGMENT_SHADER: 
			strShaderType = "fragment"; 
			break;
		}

		_Panicf("Compile failure in %s shader:\n%s\n", strShaderType, strInfoLog);
		delete [] strInfoLog;
	}

	return shader;
}

GLuint ShaderProgram::CreateProgram(const GLuint *shaderList, const int shaderListCount)
{
	GLuint	program = glCreateProgram();

	for(int i = 0; i < shaderListCount; i++)
	{
		glAttachShader(program, shaderList[i]);
	}

	glLinkProgram(program);

	GLint	status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if( status == GL_FALSE )
	{
		GLint	infoLogLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar	*strInfoLog = new GLchar[infoLogLength + 1];
		glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
		delete [] strInfoLog;
		_Panicf("Linker failure: %s\n", strInfoLog);			
	}

	for(int i = 0; i < shaderListCount; i++)
	{
		glDetachShader(program, shaderList[i]);
	}

	GLint	globalUniformBlockIndex = glGetUniformBlockIndex(program, Renderer::kGlobalUniformLabel);
	glUniformBlockBinding(program, globalUniformBlockIndex, Renderer::kGlobalUniformBindingIndex);

	return program;
}