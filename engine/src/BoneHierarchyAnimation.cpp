#include "BoneHierarchyAnimation.h"

#include "Renderer.h"
#include "Mtx44.h"
#include "MathNamespace.h"

BoneHierarchyAnimation::BoneHierarchyAnimation():
	m_interpolatedBoneTransforms(NULL),
	m_invTPoseInterpolatedBoneTransforms(NULL),
	m_boneCalculated(NULL),
	m_pModelData(NULL),
	m_pAnimationData(NULL),
	m_interpolationTime(0.0),
	m_isPlaying(false),
	m_loop(false)
{
}

BoneHierarchyAnimation::~BoneHierarchyAnimation()
{
	delete [] m_boneCalculated;
	delete [] m_interpolatedBoneTransforms;
	delete [] m_invTPoseInterpolatedBoneTransforms;
}

bool BoneHierarchyAnimation::HasBeenInit() const
{
	return m_interpolatedBoneTransforms != nullptr;
}

void BoneHierarchyAnimation::Init(const AnimationData* pAnimData, const ModelData* modelData)
{
	m_pModelData = modelData;
	m_pAnimationData = pAnimData;

	m_interpolatedBoneTransforms = new Mtx44[m_pAnimationData->numBones];
	m_invTPoseInterpolatedBoneTransforms = new Mtx44[m_pAnimationData->numBones];
	m_boneCalculated = new bool[m_pAnimationData->numBones];
	memset(m_boneCalculated, 0, sizeof(bool) * m_pAnimationData->numBones);
	for(u32 j = 0; j < m_pAnimationData->numBones; j++)
	{
		m_interpolatedBoneTransforms[j].Identity();
		m_invTPoseInterpolatedBoneTransforms[j].Identity();
	}
}

void BoneHierarchyAnimation::Update(f64 deltaTime)
{
	if( !IsPlaying() )
	{
		return;
	}

	f64	newTime = m_interpolationTime + deltaTime;

	SetTime(newTime);
}

void BoneHierarchyAnimation::CalcBone(int boneIdx, int nextKeyFrameIndex, int prevKeyFrameIndex)
{
	Mtx44	parentTransform = Mtx44::IDENTITY;
	int		parentBoneIdx = m_pAnimationData->bones[boneIdx].parentBone;
	//int		parentBoneIdx = m_pModelData->bones[boneIdx].parentBone;
	if( parentBoneIdx >= 0 )
	{
		if( !m_boneCalculated[parentBoneIdx] )
		{
			//	calculate the parent transform before moving on
			CalcBone(parentBoneIdx, nextKeyFrameIndex, prevKeyFrameIndex);
		}		
		
		//	use cached transform
		Assert(m_boneCalculated[parentBoneIdx]);
		parentTransform = m_interpolatedBoneTransforms[parentBoneIdx];
	}

	AnimationData::BoneTransform	boneLocalTransform = m_pAnimationData->GetBoneLocalTransformAtTime(boneIdx, m_interpolationTime, prevKeyFrameIndex, nextKeyFrameIndex);	

	Mtx44	interpolatedLocalTransform;
	Math::ComposeTransform(boneLocalTransform.rotation, boneLocalTransform.scale, boneLocalTransform.translation, &interpolatedLocalTransform);

	Mtx44	interpolatedWorldTransform = interpolatedLocalTransform * parentTransform;

	m_interpolatedBoneTransforms[boneIdx] = interpolatedWorldTransform;
	m_boneCalculated[boneIdx] = true;
}

void BoneHierarchyAnimation::DebugPrint(int boneIdx, int nextKeyFrameIndex, int prevKeyFrameIndex, Quat interpolatedRotation, Vector3 interpolatedScale, Vector3 interpolatedTranslation, f64 interpT)
{
	int		parentBoneIdx = m_pModelData->bones[boneIdx].parentBone;
	DebugPrintf("bone %d %s on keyframes {%d, %d} : trans {%f, %f, %f}, r {%f, %f, %f, %f}, s {%f, %f, %f}, parent %d, time %f, interpT %f\n", 
			boneIdx, 
			m_pModelData->bones[boneIdx].name,
			prevKeyFrameIndex, nextKeyFrameIndex, 
			interpolatedTranslation.x, interpolatedTranslation.y, interpolatedTranslation.z, 
			interpolatedRotation.w, interpolatedRotation.x, interpolatedRotation.y, interpolatedRotation.z,
			interpolatedScale.x, interpolatedScale.y, interpolatedScale.z, 
			parentBoneIdx, 
			m_interpolationTime, 
			interpT);
		DebugPrintf("\tkeyframe 1 : trans {%f, %f, %f}, r {%f, %f, %f, %f}\n", 
			m_pAnimationData->keyFrames[prevKeyFrameIndex].boneTransforms[boneIdx].translation.x,
			m_pAnimationData->keyFrames[prevKeyFrameIndex].boneTransforms[boneIdx].translation.y,
			m_pAnimationData->keyFrames[prevKeyFrameIndex].boneTransforms[boneIdx].translation.z,
			m_pAnimationData->keyFrames[prevKeyFrameIndex].boneTransforms[boneIdx].rotation.w,
			m_pAnimationData->keyFrames[prevKeyFrameIndex].boneTransforms[boneIdx].rotation.x,
			m_pAnimationData->keyFrames[prevKeyFrameIndex].boneTransforms[boneIdx].rotation.y,
			m_pAnimationData->keyFrames[prevKeyFrameIndex].boneTransforms[boneIdx].rotation.z);
		DebugPrintf("\tkeyframe 2 : trans {%f, %f, %f}, r {%f, %f, %f, %f}\n", 
			m_pAnimationData->keyFrames[nextKeyFrameIndex].boneTransforms[boneIdx].translation.x,
			m_pAnimationData->keyFrames[nextKeyFrameIndex].boneTransforms[boneIdx].translation.y,
			m_pAnimationData->keyFrames[nextKeyFrameIndex].boneTransforms[boneIdx].translation.z,
			m_pAnimationData->keyFrames[nextKeyFrameIndex].boneTransforms[boneIdx].rotation.w,
			m_pAnimationData->keyFrames[nextKeyFrameIndex].boneTransforms[boneIdx].rotation.x,
			m_pAnimationData->keyFrames[nextKeyFrameIndex].boneTransforms[boneIdx].rotation.y,
			m_pAnimationData->keyFrames[nextKeyFrameIndex].boneTransforms[boneIdx].rotation.z);
}

void BoneHierarchyAnimation::SetTime(f64 seconds, bool force)
{
	if( !HasBeenInit() )
	{
		return;
	}

	f64	newTime = seconds;

	if( newTime < 0 )
	{
		newTime = 0;
	}

	f64 animDuration = GetDuration();
	
	if( newTime > animDuration )
	{
		if( m_loop )
		{
			newTime = fmod(newTime, animDuration);
			Assert(!std::isnan(newTime));
		}
	}

	if( (m_interpolationTime != newTime || force) &&
		m_interpolatedBoneTransforms != NULL )
	{
		m_interpolationTime = newTime;
		Assert(!std::isnan(m_interpolationTime));

		int		prevKeyFrameIndex = -1;
		int		nextKeyFrameIndex = -1;
		m_pAnimationData->GetKeysForTime(m_interpolationTime, &prevKeyFrameIndex, &nextKeyFrameIndex);

		//	calculate the bone transforms
		memset(m_boneCalculated, 0, sizeof(bool) * m_pAnimationData->numBones);
		for(u32 boneIdx = 0; boneIdx < m_pAnimationData->numBones; boneIdx++)
		{
			if( !m_boneCalculated[boneIdx] )
			{
				CalcBone(boneIdx, nextKeyFrameIndex, prevKeyFrameIndex);
			}
		}

		//	remove t-pose influence
		for(u32 boneIdx = 0; boneIdx < m_pAnimationData->numBones; boneIdx++)
		{
			//	inverse t-pose concatenated with our calculated pose
			m_invTPoseInterpolatedBoneTransforms[boneIdx] = m_pModelData->bones[boneIdx].invTPoseWorldTransform.Multiply44(m_interpolatedBoneTransforms[boneIdx]);

			//	this sets t-pose
			//m_invTPoseInterpolatedBoneTransforms[boneIdx].Identity();
		}
	}
}

void BoneHierarchyAnimation::Start(bool loop)
{
	m_loop = loop;
	m_interpolationTime = 0.0;
	m_isPlaying = true;
}

void BoneHierarchyAnimation::Stop()
{
	m_isPlaying = false;
}

bool BoneHierarchyAnimation::IsPlaying()
{
	return m_isPlaying && (m_interpolationTime < GetDuration());
}

const Mtx44* BoneHierarchyAnimation::GetBoneTransforms()
{
	return m_interpolatedBoneTransforms;
}

const Mtx44& BoneHierarchyAnimation::GetBoneTransform(u32 modelBoneIdx)
{
	return m_interpolatedBoneTransforms[modelBoneIdx];
}

void BoneHierarchyAnimation::Draw()
{
	if( !HasBeenInit() )
	{
		return;
	}

	Renderer::SetSkinningMatrices(m_invTPoseInterpolatedBoneTransforms, GetNumBones());

	m_pModelData->Draw();
}

//	I don't like that I'm using EnableAdditionalColor here, but the vertex color wasn't working and I don't care to figure out why atm
#include "DefaultShaderProgram.h"
void BoneHierarchyAnimation::DrawBonePositions()
{
	if( !HasBeenInit() )
	{
		return;
	}

	DefaultShaderProgram	*pDefaultShaderProgram = Renderer::GetDefaultShaderProgram();
	pDefaultShaderProgram->Apply();

	//	color doesn't actually work in point and line right now but I'm leaving this in case I change it later
	const Color4	colors[] =
	{
		Color4::RED,		
		Color4::GREEN,
		Color4::BLUE		
	};

	const u32 colorCount = ARRAY_SIZE(colors);
	u32 currentColor = 0;

	for(u32 boneIdx = 0; boneIdx < m_pAnimationData->numBones; boneIdx++)
	{
		int		parentBoneIdx = m_pModelData->bones[boneIdx].parentBone;	

		Vector3	boneOrigin = m_interpolatedBoneTransforms[boneIdx].GetTranslation3();

		if( parentBoneIdx < 0 )
		{
			pDefaultShaderProgram->EnableAdditionalColor(true, &Color4::WHITE);

			//Renderer::DrawPoint(&boneOrigin, &Color4::WHITE);
			Renderer::DrawPoint(&boneOrigin);
		}
		else
		{
			pDefaultShaderProgram->EnableAdditionalColor(true, &colors[currentColor]);

			Vector3	boneParentPos = m_interpolatedBoneTransforms[parentBoneIdx].GetTranslation3();
			//Renderer::DrawLine(&boneOrigin, &boneParentPos, &colors[currentColor]);
			Renderer::DrawLine(&boneOrigin, &boneParentPos);

			currentColor = (currentColor + 1) % colorCount;
		}
	}

	pDefaultShaderProgram->EnableAdditionalColor(false, NULL);
}

f64 BoneHierarchyAnimation::GetDuration()
{
	f64	result = 0.0;
	if( m_pAnimationData && 
		m_pAnimationData->numKeyFrames > 0 )
	{
		result = m_pAnimationData->keyFrames[m_pAnimationData->numKeyFrames-1].time;
	}

	return result;
}