#pragma once

#include "types.h"
#include "GameState.h"
#include "ViewOriginCamera.h"

#include "Model.h"
#include "Animation.h"
#include "BoneHierarchyAnimation.h"

#include "PerspectiveFont.h"

class PreviewScene : public GameState
{
public:
	enum
	{
		kMaxAnimationsToSample = 8
	};

public:
	PreviewScene();
	virtual ~PreviewScene();

public:
	virtual void Update(f64 deltaTime);
	virtual void ResizeCallback(s32 width, s32 height);

	//	used to get the data from the generation thread
	AnimationData* GetAnimDataPtr(){ return m_testGeneratedAnim; }
	void SetupGeneratedAnims();
	void SetActiveAnimCount(int count){ m_currentTestAnimsActive = count; }

protected:
	ViewOriginCamera				m_perspectiveCamera;
	CameraBase						m_orthoCamera;

	PerspectiveFont					m_hoverText;

	ModelRef						m_targetModelRef;
	AnimationData					m_testGeneratedAnim[kMaxAnimationsToSample];
	BoneHierarchyAnimation			m_testGeneratedAnimationChannel[kMaxAnimationsToSample];
	int								m_currentTestAnimsActive;

	f64								m_currentAnimTime;
	bool							m_playingAnimation;
	bool							m_drawingBones;
};

extern PreviewScene*	g_PreviewScene;