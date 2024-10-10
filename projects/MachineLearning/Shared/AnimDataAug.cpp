#include "AnimDataAug.h"

#include "Mtx33.h"
#include "AnimGenBoneDictionary.h"
#include "MathConstants.h"

namespace AnimDataAug
{
	torch::Tensor RotateBoneC3DR6(torch::Tensor inputImage, int boneIdx, f32 pitch, f32 yaw, f32 roll)
	{
		Assert(boneIdx >= 0);
		Assert(boneIdx < g_animGenBoneDictionary->GetVocabSize());

		torch::NoGradGuard	no_grad;

		Mtx33			eulerRotationMtx;
		eulerRotationMtx.ConstructEulerRotation(pitch, yaw, roll);

		torch::Tensor	eulerRotationTensor = torch::from_blob(eulerRotationMtx.a, {Mtx33::kRowCount, Mtx33::kColumnCount}, torch::kF32).clone();

		torch::Tensor	result = inputImage;

		result[boneIdx] = result[boneIdx].view({-1, 2, 3}).matmul(eulerRotationTensor).view({-1, 6});

		return result;
	}

	std::string RotateBonePrompt(std::string basePrompt, int boneIdx, f32 pitch, f32 yaw, f32 roll)
	{
		Assert(boneIdx >= 0);
		Assert(boneIdx < g_animGenBoneDictionary->GetVocabSize());

		std::string	boneName = g_animGenBoneDictionary->GetBoneName(boneIdx);
		f32			pitchDegrees = (f32)((pitch + M_PI) * RADIANS_TO_DEGREES);
		f32			yawDegrees = (f32)((yaw + M_PI) * RADIANS_TO_DEGREES);
		f32			rollDegrees = (f32)((roll + M_PI) * RADIANS_TO_DEGREES);

		char	augmentBuffer[256];
		sprintf(augmentBuffer, " augRot %s p:%d y:%d r:%d", boneName.c_str(), (int)pitchDegrees, (int)yawDegrees, (int)rollDegrees);

		std::string	result = basePrompt + augmentBuffer;

		return result;
	}
};