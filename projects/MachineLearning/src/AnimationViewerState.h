#pragma once

#include "types.h"
#include "GameState.h"
#include "CameraBase.h"
#include "OrthoFont.h"
#include "PerspectiveFont.h"
#include "ViewOriginCamera.h"
#include "BoneHierarchyAnimation.h"
#include "AnimTrainingDataFiles.h"
#include "Model.h"
#include "Animation.h"

class AnimationViewerState : public GameState
{
public:
	AnimationViewerState();
	virtual ~AnimationViewerState();

public:
	virtual void Update(f64 deltaTime);
	virtual void ResizeCallback(s32 width, s32 height);

protected:
	void DrawStuff();

	void UpdateAnimation();

protected:
	CameraBase							m_orthoCamera;
	OrthoFont							m_font;

	AnimTrainingDataFiles				m_atdf;

	ViewOriginCamera					m_perspectiveCamera;
	PerspectiveFont						m_hoverText;

	ModelRef							m_testModelRef;
	AnimationRef						m_testAnimRef;
	BoneHierarchyAnimation				m_testAnimationChannel;

	int									m_modelIdx;
	int									m_animIdx;
	std::string							m_animDesc;

	f64									m_currentAnimTime;
	bool								m_playingAnimation;
	bool								m_drawingBones;
	AnimTrainingDataFiles::eFileType	m_atdfFileType;
};