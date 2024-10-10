#include "types.h"

#include "AnimationData.h"
#include "MathNamespace.h"

AnimationData::AnimationData():
	flags(kFlag_None)
{
}

AnimationData::~AnimationData()
{
	if( (flags & kFlag_DynamicallyAllocated) != 0 )
	{
		Deallocate();
	}
}

void AnimationData::Init()
{
	Assert((flags & kFlag_Asset) != 0);

	//	fixup the pointers
	bones = (BoneData*)((intptr_t)bones + (intptr_t)this);
	keyFrames = (KeyFrame*)((intptr_t)keyFrames + (intptr_t)this);

	Assert(numBones < kMaxBones);
}

void AnimationData::Deinit()
{
}

void AnimationData::Allocate(u32 keyFrameCount, u32 boneCount)
{
	Assert(boneCount < kMaxBones);
	Assert((flags & kFlag_Asset) == 0);

	if( (flags & kFlag_DynamicallyAllocated) != 0 )
	{
		Deallocate();
	}

	flags |= kFlag_DynamicallyAllocated;

	numKeyFrames = keyFrameCount;
	numBones = boneCount;

	bones = new BoneData[numBones];
	keyFrames = new KeyFrame[numKeyFrames];
}

void AnimationData::Deallocate()
{
	delete [] bones;
	bones = nullptr;

	delete [] keyFrames;
	keyFrames = nullptr;
}

f32 AnimationData::Duration() const
{
	return keyFrames[numKeyFrames-1].time;
}

void AnimationData::GetKeysForTime(const f64 time, int* startKey, int* endKey) const
{
	*startKey = -1;
	*endKey = -1;

	f32	animDuration = Duration();

	if( time >= animDuration )
	{
		*startKey = numKeyFrames;
		*endKey = numKeyFrames;
	}
	else
	{
		for(u32 i = 0; i < numKeyFrames && *endKey < 0; i++)
		{
			if( time < keyFrames[i].time )
			{
				*endKey = i;
				*startKey = i - 1;
			}
		}
	}
}

AnimationData::BoneTransform AnimationData::GetBoneLocalTransformAtTime(const int boneIdx, const f64 time) const
{
	BoneTransform	result;

	int				startKey;
	int				endKey;
	GetKeysForTime(time, &startKey, &endKey);

	return GetBoneLocalTransformAtTime(boneIdx, time, startKey, endKey);
}

AnimationData::BoneTransform AnimationData::GetBoneLocalTransformAtTime(const int boneIdx, const f64 time, const int startKey, const int endKey) const
{
	BoneTransform	result;
	
	Quat			interpolatedRotation;
	Vector3			interpolatedScale;
	Vector3			interpolatedTranslation;

	if( startKey < 0 )
	{		
		interpolatedRotation = keyFrames[0].boneTransforms[boneIdx].rotation;
		interpolatedScale = keyFrames[0].boneTransforms[boneIdx].scale;
		interpolatedTranslation = keyFrames[0].boneTransforms[boneIdx].translation;
	}
	else
	if( startKey >= (int)numKeyFrames )
	{
		int		finalKeyFrameidx = numKeyFrames-1;
		interpolatedRotation = keyFrames[finalKeyFrameidx].boneTransforms[boneIdx].rotation;
		interpolatedScale = keyFrames[finalKeyFrameidx].boneTransforms[boneIdx].scale;
		interpolatedTranslation = keyFrames[finalKeyFrameidx].boneTransforms[boneIdx].translation;
	}
	else
	{
		f64	prevTime = keyFrames[startKey].time;
		f64	nextTime = keyFrames[endKey].time;
		f64	timeDifference = nextTime - prevTime;
		Assert(timeDifference > 0.0);
		f64	keyFramePoint = time - prevTime;
		f64	weight = keyFramePoint / timeDifference;
		
		Quat::Slerp(
			keyFrames[startKey].boneTransforms[boneIdx].rotation,
			keyFrames[endKey].boneTransforms[boneIdx].rotation,
			(coord_type)weight,
			&interpolatedRotation);

		interpolatedScale = 
			keyFrames[startKey].boneTransforms[boneIdx].scale * (coord_type)(1.0 - weight) +
			keyFrames[endKey].boneTransforms[boneIdx].scale * (coord_type)(weight);

		interpolatedTranslation = 
			keyFrames[startKey].boneTransforms[boneIdx].translation * (coord_type)(1.0 - weight) +
			keyFrames[endKey].boneTransforms[boneIdx].translation * (coord_type)(weight);
	}

	result.rotation = interpolatedRotation;
	result.scale = interpolatedScale;
	result.translation = interpolatedTranslation;

	return result;
}

//	note : prob comparatively expensive due to all the of the recursion and matrix composition
//	could be done much more efficiently, especially in batches
AnimationData::BoneTransform AnimationData::GetBoneWorldTransformAtTime(const int boneIdx, const f64 time) const
{
	BoneTransform	result;

	int				startKey;
	int				endKey;
	GetKeysForTime(time, &startKey, &endKey);

	return GetBoneWorldTransformAtTime(boneIdx, time, startKey, endKey);
}

AnimationData::BoneTransform AnimationData::GetBoneWorldTransformAtTime(const int boneIdx, const f64 time, const int startKey, const int endKey) const
{
	BoneTransform	localTransform = GetBoneLocalTransformAtTime(boneIdx, time, startKey, endKey);
	BoneTransform	parentTransform;
	parentTransform.rotation = Quat::IDENTITY;
	parentTransform.scale = Vector3::ONE;
	parentTransform.translation = Vector3::ZERO;

	int	parentBoneIdx = bones[boneIdx].parentBone;
	if( parentBoneIdx >= 0 )
	{
		parentTransform = GetBoneWorldTransformAtTime(parentBoneIdx, time, startKey, endKey);
	}

	BoneTransform	result = localTransform * parentTransform;

	return result;
}

AnimationData::BoneTransform AnimationData::BoneTransform::operator*(const AnimationData::BoneTransform& rhs) const
{
	BoneTransform	result;

	Mtx43			lhsMtx;
	Math::ComposeTransform(rotation, scale, translation, &lhsMtx);

	Mtx43			rhsMtx;
	Math::ComposeTransform(rhs.rotation, rhs.scale, rhs.translation, &rhsMtx);

	Mtx43			resultMtx = lhsMtx * rhsMtx;

	Math::DecomposeTransform(resultMtx, &result.rotation, &result.scale, &result.translation);

	return result;
}

void AnimationData::CalcBoneWorldTransformsBatch_Recurse(const f64 time, const int boneIdx, int prevKeyFrameIndex, int nextKeyFrameIndex, Mtx43* transformsOut, bool* visited) const
{
	if( visited[boneIdx] )
	{
		return;
	}

	AnimationData::BoneTransform	boneLocalTransform = GetBoneLocalTransformAtTime(boneIdx, time, prevKeyFrameIndex, nextKeyFrameIndex);

	int		thisBoneParentIdx = bones[boneIdx].parentBone;

	Mtx43	parentTransform = Mtx43::IDENTITY;

	if( thisBoneParentIdx >= 0 )
	{
		CalcBoneWorldTransformsBatch_Recurse(time, thisBoneParentIdx, prevKeyFrameIndex, nextKeyFrameIndex, transformsOut, visited);

		parentTransform = transformsOut[thisBoneParentIdx];
	}

	Mtx43	ourLocalTransform;
	Math::ComposeTransform(boneLocalTransform.rotation, boneLocalTransform.scale, boneLocalTransform.translation, &ourLocalTransform);

	transformsOut[boneIdx] = ourLocalTransform * parentTransform;
	visited[boneIdx] = true;
}

void AnimationData::CalcBoneWorldTransformsBatch(const f64 time, Mtx43* transformsOut) const
{
	//	get the keys we're looking at
	int		prevKeyFrameIndex = -1;
	int		nextKeyFrameIndex = -1;
	GetKeysForTime(time, &prevKeyFrameIndex, &nextKeyFrameIndex);

	CalcBoneWorldTransformsBatch(time, prevKeyFrameIndex, nextKeyFrameIndex, transformsOut);
}

void AnimationData::CalcBoneWorldTransformsBatch(const f64 time, const int startKey, const int endKey, Mtx43* transformsOut) const
{
	//	we assume that the caller has allocated "transformsOut" of array size equal to our bones
	Assert(numBones > 0);
	Assert(transformsOut != nullptr);

	//	allocate something that we will use to keep track of which bones we have already calculated
	bool*	visited = new bool[numBones];
	memset(visited, 0, sizeof(bool) * numBones);

	//	calc transforms
	for(u32 boneIdx = 0; boneIdx <  numBones; ++boneIdx)
	{
		CalcBoneWorldTransformsBatch_Recurse(time, boneIdx, startKey, endKey, transformsOut, visited);
	}

	//	done
	delete [] visited;
}