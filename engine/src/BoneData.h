#pragma once

#include "Mtx43.h"

class BoneData
{
public:
	Mtx43	TPoseWorldTransform;		//	this should be baked into the model's verts, but it is here for reference
	Mtx43	invTPoseWorldTransform;
	Mtx43	TPoseLocalTransform;
	int		parentBone;
	char	name[256];
};