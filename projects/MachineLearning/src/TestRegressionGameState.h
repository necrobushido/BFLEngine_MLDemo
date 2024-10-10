#pragma once

#include "types.h"
#include "GameState.h"
#include "CameraBase.h"
#include "OrthoFont.h"

#include "SharedMLHeader.h"

class TestRegressionGameState : public GameState
{
public:
	TestRegressionGameState();
	virtual ~TestRegressionGameState();

public:
	virtual void Update(f64 deltaTime);
	virtual void ResizeCallback(s32 width, s32 height);

protected:
	void DrawStuff();

	void TrainingIteration();

protected:
	CameraBase				m_orthoCamera;
	OrthoFont				m_font;

	// regression vars
	torch::Tensor			m_weightsTarget;
	torch::Tensor			m_biasesTarget;
	torch::nn::Linear		m_fc;
	torch::optim::SGD		m_optim;
	f32						m_loss;
	s64						m_batchIdx;
	bool					m_trainingFinished;
};