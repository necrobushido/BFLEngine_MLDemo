#pragma once

#include "ShaderProgram.h"
#include "Vector3.h"
#include "Color.h"

class DefaultShaderProgram : public ShaderProgram
{
public:
	DefaultShaderProgram();

public:
	//	need to apply the shader before using any of these methods
	void EnableTexture(int enabled);
	void EnableVertexColor(int enabled);
	void EnableLighting(int enabled);
	void EnableSkinning(int enabled);
	void EnableAdditionalColor(int enabled, const Color4 *color);
	
	void SetDirectionLight(const Vector3 *dir, const Color4 *intensity);
	void SetAmbientLight(const Color4 *intensity);
	void SetPointLight(const Vector3 *pos, const Color4 *intensity);
	void SetPointLightPos(const Vector3 *pos);

	void ApplyLight();

protected:
	Vector3	lightDir;
	Color4	lightIntensity;
	Vector3	pointLightPos;
	GLuint	directionalLightDirUnif;
	GLuint	lightIntensityUnif;
	GLuint	ambientLightIntensityUnif;
	GLuint	cameraSpaceLightPosUnif;
	
	GLuint	textureEnabledUnif;
	GLuint	vertexColorEnabledUnif;
	GLuint	lightingEnabledUnif;
	GLuint	skinningEnabledUnif;
	
	GLuint	additionalColorEnabledUnif;
	GLuint	additionalColorUnif;
};