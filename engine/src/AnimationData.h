#pragma once

#include "Quat.h"
#include "BoneData.h"

class AnimationData
{
public:
	enum eFlags
	{
		kFlag_None = 0,
		kFlag_Asset = 1 << 0,
		kFlag_DynamicallyAllocated = 1 << 1
	};

	enum
	{
		kMaxBones = 100			//	we don't actually need a constant for this but it makes serialization easier
	};

	struct BoneTransform
	{
		Quat	rotation;
		Vector3	scale;
		Vector3	translation;

		BoneTransform operator*(const BoneTransform& rhs) const;
	};

	struct KeyFrame
	{
		f32				time;
		BoneTransform	boneTransforms[kMaxBones];
	};

public:
	AnimationData();
	~AnimationData();

	//	used in the engine to fixup pointers to data members already existing in file data / memory
	void Init();
	void Deinit();

	//	used in tools to handle dynamically allocated data members
	void Allocate(u32 keyFrameCount, u32 boneCount);
	void Deallocate();

	f32 Duration() const;
	void GetKeysForTime(const f64 time, int* startKey, int* endKey) const;
	BoneTransform GetBoneLocalTransformAtTime(const int boneIdx, const f64 time) const;
	BoneTransform GetBoneLocalTransformAtTime(const int boneIdx, const f64 time, const int startKey, const int endKey) const;

	BoneTransform GetBoneWorldTransformAtTime(const int boneIdx, const f64 time) const;
	BoneTransform GetBoneWorldTransformAtTime(const int boneIdx, const f64 time, const int startKey, const int endKey) const;

	void CalcBoneWorldTransformsBatch(const f64 time, Mtx43* transformsOut) const;
	void CalcBoneWorldTransformsBatch(const f64 time, const int startKey, const int endKey, Mtx43* transformsOut) const;

protected:
	void CalcBoneWorldTransformsBatch_Recurse(const f64 time, const int boneIdx, int prevKeyFrameIndex, int nextKeyFrameIndex, Mtx43* transformsOut, bool* visited) const;

public:
	u32			flags;
	u32			numBones;
	u32			numKeyFrames;

	BoneData*	bones;				//	base transform and name of bones;	[numBones] indexes;  I added this to make bone binding more dynamic and fault tolerant if there are mismatches between animations and models

	KeyFrame*	keyFrames;			//	time and transform at each keyframe;		[numKeyFrames] indexes;				keyFrameTimes[0] == 0	
};