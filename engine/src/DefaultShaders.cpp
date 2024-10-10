#include "DefaultShaders.h"

const char*	s_defaultVertexShader = 
"#version 330\n\
\n\
layout(location = 0) in vec4 position;\n\
layout(location = 1) in vec2 texCoord;\n\
layout(location = 2) in vec4 color;\n\
layout(location = 3) in vec3 normal;\n\
layout(location = 4) in vec4 boneIndices;\n\
layout(location = 5) in vec4 boneWeights;\n\
\n\
out vec3 vertexPosition;\n\
out vec2 fragTexCoord;\n\
out vec4 vertexColor;\n\
out vec3 vertexNormal;\n\
\n\
uniform bool skinningEnabled;\n\
\n\
layout(std140) uniform GlobalMatrices\n\
{\n\
	mat4	viewMatrix;\n\
	mat4	modelViewMatrix;\n\
    mat4	modelViewProjMatrix;\n\
	mat4	normalModelViewMatrix;\n\
	mat4	skinningMatrices[124];\n\
};\n\
\n\
void main()\n\
{\n\
	//	position\n\
	vec4	skinnedVertexPosition = position;\n\
	vec3	skinnedNormal = normal;\n\
	if( skinningEnabled )\n\
	{\n\
		mat4	skinMtx = mat4(0);\n\
		int		i;\n\
		float	weightSum = 0;\n\
		for(i = 0; i < 4; i++)\n\
		{\n\
			if( boneIndices[i] >= 0 )\n\
			{\n\
				highp int boneIndex = int(boneIndices[i]);\n\
				skinMtx += boneWeights[i] * skinningMatrices[boneIndex];\n\
				weightSum += boneWeights[i];\n\
			}\n\
		}\n\
		\n\
		if( weightSum > 0 )\n\
		{\n\
			skinnedVertexPosition = skinMtx * position;\n\
			skinnedVertexPosition /= weightSum;\n\
			\n\
			mat3	normalSkinMtx = mat3(skinMtx);\n\
			skinnedNormal = normalSkinMtx * normal;\n\
		}\n\
	}\n\
	\n\
	gl_Position = modelViewProjMatrix * skinnedVertexPosition;\n\
	\n\
	//	texture\n\
	fragTexCoord = texCoord;\n\
	\n\
	//	color\n\
	vertexColor = color;\n\
	\n\
	//	pass some stuff to the fragment shader to test fragment point lighting\n\
	vec4	cameraSpaceVertexPosition = modelViewMatrix * skinnedVertexPosition;\n\
    vertexPosition = cameraSpaceVertexPosition.xyz;\n\
    \n\
    mat3	normMtx = mat3(normalModelViewMatrix);\n\
	vertexNormal = normMtx * skinnedNormal;\n\
}";

const char*	s_defaultFragmentShader =
"#version 330\n\
	\n\
//	all inputs from a vertex shader to a fragment shader are interpolated between the 3 triangle verts in question\n\
in vec3				vertexPosition;\n\
in vec2				fragTexCoord;\n\
in vec4				vertexColor;\n\
in vec3				vertexNormal;\n\
\n\
layout(location = 0) out vec4	outputColor;\n\
\n\
uniform sampler2D	textureEmissive;\n\
uniform sampler2D	textureAmbient;\n\
uniform sampler2D	textureDiffuse;\n\
uniform sampler2D	textureSpecular;\n\
\n\
uniform vec3		directionalLightDir;\n\
\n\
uniform vec3		cameraSpaceLightPos;\n\
uniform vec4		lightIntensity;\n\
uniform vec4		ambientIntensity;\n\
uniform vec4		additionalColor;\n\
\n\
uniform bool		textureEnabled;\n\
uniform bool		vertexColorEnabled;\n\
uniform bool		lightingEnabled;\n\
uniform bool		additionalColorEnabled;\n\
\n\
const vec4			specularColor = vec4(0.15, 0.15, 0.15, 1.0);\n\
const vec4			gamma = vec4(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2, 1.0);\n\
\n\
vec4 CalcLightIntensity(in vec3 cameraSpacePosition, out vec3 lightDirection)\n\
{\n\
	vec3	lightDisplacement = cameraSpaceLightPos - cameraSpacePosition;\n\
	float	lightDistanceSquared = dot(lightDisplacement, lightDisplacement);\n\
	lightDirection = lightDisplacement * inversesqrt(lightDistanceSquared);\n\
	\n\
	float	distFactor = lightDistanceSquared;			//	inverse square distance\n\
	//float	distFactor = sqrt(lightDistanceSquared);	//	inverse distance\n\
\n\
	float	lightAttenuationConstant = 0.000001f;	//	1 / ((distance at which half of the light intensity is lost) ^ 2) using inverse square method\n\
	return lightIntensity * (1 / ( 1.0 + lightAttenuationConstant * distFactor));\n\
}\n\
\n\
vec4 ApplyLight(in vec4 intensity, in vec3 lightDirection, in vec3 normal, in vec3 viewDirection)\n\
{\n\
	vec4	outputLight = vec4(0);\n\
	\n\
	//	diffuse lighting\n\
	float	cosAngIncidence = dot(normal, lightDirection);\n\
    cosAngIncidence = clamp(cosAngIncidence, 0, 1);\n\
    outputLight += (intensity * cosAngIncidence);\n\
    \n\
    //	specular lighting\n\
    vec3	halfAngle = normalize(lightDirection + viewDirection);\n\
    int		specularMode = 1;\n\
    float	specularTerm = 0;\n\
    \n\
    if( specularMode == 0 )\n\
    {\n\
		//	blinn-phong\n\
		float	shininessFactor = 400.0;\n\
		specularTerm = dot(normal, halfAngle);\n\
		specularTerm = clamp(specularTerm, 0, 1);\n\
		if( cosAngIncidence == 0.0 )\n\
		{\n\
			specularTerm = 0;\n\
		}\n\
		specularTerm = pow(specularTerm, shininessFactor);\n\
	}\n\
	else\n\
	{\n\
		//	gaussian\n\
		float	smoothnessFactor = 0.1;	//	0 < smoothnessFactor <= 1\n\
		float	angleNormalHalf = acos(dot(halfAngle, normal));\n\
		float	exponent = angleNormalHalf / smoothnessFactor;\n\
		exponent = -(exponent * exponent);\n\
		specularTerm = exp(exponent);\n\
	}\n\
	\n\
	outputLight += (specularColor * intensity * specularTerm);\n\
	\n\
	return outputLight;	\n\
}\n\
\n\
void main()\n\
{	\n\
	outputColor = vec4(1.0);\n\
	if( lightingEnabled )\n\
	{\n\
		//	ambient lighting\n\
		outputColor = ambientIntensity;\n\
	\n\
		//	point lighting\n\
		vec3	pointLightDir = vec3(0.0);\n\
		vec4	attenIntensity = CalcLightIntensity(vertexPosition, pointLightDir);\n\
		\n\
		vec3	surfaceNormal = normalize(vertexNormal);\n\
		vec3	viewDirection = normalize(-vertexPosition);\n\
		outputColor += ApplyLight(lightIntensity, directionalLightDir, surfaceNormal, viewDirection);\n\
		outputColor += ApplyLight(attenIntensity, pointLightDir, surfaceNormal, viewDirection);\n\
	}\n\
	\n\
	//	texturing\n\
	if( textureEnabled )\n\
	{\n\
		outputColor *= texture(textureDiffuse, fragTexCoord);\n\
		\n\
		if( outputColor.a == 0.0 )\n\
			discard;\n\
	}\n\
	\n\
	//	color\n\
	if( vertexColorEnabled )\n\
	{\n\
		outputColor *= vertexColor;\n\
	}\n\
	\n\
	if( additionalColorEnabled )\n\
	{\n\
		outputColor *= additionalColor;\n\
	}\n\
	\n\
	//	gamma correction\n\
	outputColor = pow(outputColor, gamma);\n\
}";