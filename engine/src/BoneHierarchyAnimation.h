#pragma once

#include "AnimationData.h"
#include "ModelData.h"

class BoneHierarchyAnimation
{
public:
	BoneHierarchyAnimation();
	~BoneHierarchyAnimation();

public:
	bool HasBeenInit() const;
	void Init(const AnimationData* pAnimData, const ModelData* modelData);
	void Update(f64 deltaTime);
	void SetTime(f64 seconds, bool force = false);
	void Start(bool loop = true);
	void Stop();
	bool IsPlaying();
	const Mtx44* GetBoneTransforms();
	const Mtx44& GetBoneTransform(u32 modelBoneIdx);
	u32 GetNumBones(){ return m_pAnimationData->numBones; }
	u32 GetNumKeyframes(){ return m_pAnimationData->numKeyFrames; }
	f64 GetInterpTime() const { return m_interpolationTime; }
	f64 GetDuration();

	void Draw();
	void DrawBonePositions();

protected:
	void CalcBone(int boneIdx, int nextKeyFrameIndex, int prevKeyFrameIndex);
	void DebugPrint(int boneIdx, int nextKeyFrameIndex, int prevKeyFrameIndex, Quat interpolatedRotation, Vector3 interpolatedScale, Vector3 interpolatedTranslation, f64 interpT);

private:
	const ModelData*		m_pModelData;
	const AnimationData*	m_pAnimationData;
	f64						m_interpolationTime;
	bool					m_loop;
	bool					m_isPlaying;

	Mtx44*					m_invTPoseInterpolatedBoneTransforms;
	Mtx44*					m_interpolatedBoneTransforms;
	bool*					m_boneCalculated;
};