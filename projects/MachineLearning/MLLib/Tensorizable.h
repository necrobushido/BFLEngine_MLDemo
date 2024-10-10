#pragma once

#include "SharedMLHeader.h"

class ITensorizable
{
public:
	virtual torch::Tensor ToTensor() = 0;
};
