#pragma once

#include "types.h"
#include "GameState.h"
#include "CameraBase.h"
#include "OrthoFont.h"
#include "PerspectiveFont.h"
#include "ViewOriginCamera.h"
#include "StateLogic.h"
#include "BoneHierarchyAnimation.h"

#include "Model.h"
#include "Animation.h"

class AnimGenProcessingThreadBase;

class TestFBXGameState : public GameState
{
public:
	enum
	{
		kNumAnimationsToSample = 8
	};

public:
	TestFBXGameState();
	virtual ~TestFBXGameState();

public:
	virtual void Update(f64 deltaTime);
	virtual void ResizeCallback(s32 width, s32 height);

protected:
	void Draw2DStuff();

protected:
	ViewOriginCamera				m_perspectiveCamera;
	CameraBase						m_orthoCamera;
	OrthoFont						m_font;
	PerspectiveFont					m_hoverText;

	ModelRef						m_targetModelRef;
	AnimationData					m_testGeneratedAnim[kNumAnimationsToSample];
	BoneHierarchyAnimation			m_testGeneratedAnimationChannel[kNumAnimationsToSample];
	int								m_currentTestAnimsActive;

	AnimGenProcessingThreadBase*	m_pAnimGenProcThread;
	StateLogic						m_procThreadState;

	f64								m_currentAnimTime;
	bool							m_playingAnimation;
	bool							m_drawingBones;

	f32								m_learningRate;
};