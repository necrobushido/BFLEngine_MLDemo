#include "DefaultShaderProgram.h"

#include "MaterialData.h"
#include "Renderer.h"

DefaultShaderProgram::DefaultShaderProgram():
	ShaderProgram()
{
	Apply();

	//	default texture setting for now
	GLuint textureEmissiveUnif = glGetUniformLocation(m_programObject, "textureEmissive");
	glUniform1i(textureEmissiveUnif, MaterialData::eEmissive);
	GLuint textureAmbientUnif = glGetUniformLocation(m_programObject, "textureAmbient");
	glUniform1i(textureAmbientUnif, MaterialData::eAmbient);
	GLuint textureDiffuseUnif = glGetUniformLocation(m_programObject, "textureDiffuse");
	glUniform1i(textureDiffuseUnif, MaterialData::eDiffuse);
	GLuint textureSpecularUnif = glGetUniformLocation(m_programObject, "textureSpecular");
	glUniform1i(textureSpecularUnif, MaterialData::eSpecular);

	//	directional light test
	directionalLightDirUnif = glGetUniformLocation(m_programObject, "directionalLightDir");
	lightIntensityUnif = glGetUniformLocation(m_programObject, "lightIntensity");
	ambientLightIntensityUnif = glGetUniformLocation(m_programObject, "ambientIntensity");
	cameraSpaceLightPosUnif = glGetUniformLocation(m_programObject, "cameraSpaceLightPos");

	textureEnabledUnif = glGetUniformLocation(m_programObject, "textureEnabled");
	vertexColorEnabledUnif = glGetUniformLocation(m_programObject, "vertexColorEnabled");
	lightingEnabledUnif = glGetUniformLocation(m_programObject, "lightingEnabled");
	skinningEnabledUnif = glGetUniformLocation(m_programObject, "skinningEnabled");
	
	additionalColorEnabledUnif = glGetUniformLocation(m_programObject, "additionalColorEnabled");
	additionalColorUnif = glGetUniformLocation(m_programObject, "additionalColor");
}

void DefaultShaderProgram::EnableTexture(int enabled)
{
	glUniform1i(textureEnabledUnif, enabled);
}

void DefaultShaderProgram::EnableVertexColor(int enabled)
{
	glUniform1i(vertexColorEnabledUnif, enabled);
}

void DefaultShaderProgram::EnableLighting(int enabled)
{
	glUniform1i(lightingEnabledUnif, enabled);
}

void DefaultShaderProgram::EnableSkinning(int enabled)
{
	glUniform1i(skinningEnabledUnif, enabled);
}

void DefaultShaderProgram::EnableAdditionalColor(int enabled, const Color4 *color)
{
	glUniform1i(additionalColorEnabledUnif, enabled);
	if( enabled &&
		color )
	{
		glUniform4fv(additionalColorUnif, 1, (f32*)color);			
	}
}

void DefaultShaderProgram::SetDirectionLight(const Vector3 *dir, const Color4 *intensity)
{
	lightDir = *dir;
	lightIntensity = *intensity;

	glUniform4fv(lightIntensityUnif, 1, (f32*)&lightIntensity);
}

void DefaultShaderProgram::SetAmbientLight(const Color4 *intensity)
{
	glUniform4fv(ambientLightIntensityUnif, 1, (f32*)intensity);
}

void DefaultShaderProgram::SetPointLight(const Vector3 *pos, const Color4 *intensity)
{
	pointLightPos = *pos;
	lightIntensity = *intensity;

	glUniform4fv(lightIntensityUnif, 1, (f32*)&lightIntensity);
}

void DefaultShaderProgram::SetPointLightPos(const Vector3 *pos)
{
	pointLightPos = *pos;
}

void DefaultShaderProgram::ApplyLight()
{
	const Mtx44	*viewMtx = Renderer::GetViewMatrix();

	//	point light
	Vector3	cameraSpaceLightPos;
	viewMtx->MultiplyVec43(pointLightPos, cameraSpaceLightPos, true);

	glUniform3fv(cameraSpaceLightPosUnif, 1, (f32*)&cameraSpaceLightPos);

	//	directional light	
	Vector3	directionalLightDirCamSpace;
	viewMtx->MultiplyVec43(lightDir, directionalLightDirCamSpace, false);
	directionalLightDirCamSpace *= -1.0f;

	glUniform3fv(directionalLightDirUnif, 1, (f32*)&directionalLightDirCamSpace);
}