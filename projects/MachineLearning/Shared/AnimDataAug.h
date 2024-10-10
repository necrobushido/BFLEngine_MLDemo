#pragma once

#include "types.h"
#include "SharedMLHeader.h"

namespace AnimDataAug
{
	torch::Tensor RotateBoneC3DR6(torch::Tensor inputImage, int boneIdx, f32 pitch, f32 yaw, f32 roll);
	std::string RotateBonePrompt(std::string basePrompt, int boneIdx, f32 pitch, f32 yaw, f32 roll);
};