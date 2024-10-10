#include "stdafx.h"

#include "WriteAnim.h"

void WriteAnim(AnimationData *anim, FILE *pFile)
{
	intptr_t		currentOffset = sizeof(AnimationData);

	//	write the anim struct itself
	BoneData*	tempBones = anim->bones;
	anim->bones = (BoneData*)(currentOffset);

	AnimationData::KeyFrame*	tempKeyFrames = anim->keyFrames;
	anim->keyFrames = (AnimationData::KeyFrame*)(currentOffset + sizeof(BoneData) * anim->numBones);

	size_t	bytesWritten;
	bytesWritten = fwrite(anim, 1, sizeof(AnimationData), pFile);
	assert(bytesWritten == sizeof(AnimationData));

	anim->bones = tempBones;
	anim->keyFrames = tempKeyFrames;

	//	now write the bones	
	bytesWritten = fwrite(anim->bones, 1, sizeof(BoneData) * anim->numBones, pFile);
	assert(bytesWritten == sizeof(BoneData) * anim->numBones);
	currentOffset += sizeof(BoneData) * anim->numBones;

	//	now write the keyframes	
	bytesWritten = fwrite(anim->keyFrames, 1, sizeof(AnimationData::KeyFrame) * anim->numKeyFrames, pFile);
	assert(bytesWritten == sizeof(AnimationData::KeyFrame) * anim->numKeyFrames);
	currentOffset += sizeof(AnimationData::KeyFrame) * anim->numKeyFrames;
}

void WriteAnimToFile(AnimationData &animData, const char *filename)
{
	FILE	*pFile = fopen(filename, "wb");
	if( pFile == NULL )
	{
		printf("error opening file %s\n", filename);
		assert(0);
	}

	WriteAnim(&animData, pFile);

	fclose(pFile);
}