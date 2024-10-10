#pragma once

#include "Thread.h"
#include "Mutex.h"
#include "Clock.h"

#include "SharedMLHeader.h"

#include "NNBatchingProcThread.h"

class NNModelComponent;

typedef void (*LogFunc)(const char* logLine);

class NNProcessingThreadBase : public Thread
{
public:
	enum eProcessState
	{
		kProcessState_Invalid,
		kProcessState_Init,
		kProcessState_Idle,
		kProcessState_EvalLearningRate,
		kProcessState_Training,
		kProcessState_Generating,
		kProcessState_Save,
		kProcessState_Load,
		kProcessState_StateCount
	};

	static const char* GetStateName(int state);

public:
	NNProcessingThreadBase();
	virtual ~NNProcessingThreadBase();

public:
	void ChangeState(int stateToChangeTo);
	int GetState();

	void SetLogFunc(LogFunc logFuncToUse);

	virtual void UpdateTargetModel(const char* modelToUse) = 0;
	virtual void SetLearningRate(f32 learningRate);

protected:
	void Run() override;

	void UpdateState();

	virtual void EnteringState(int currentState);
	virtual void LeavingState(int prevState);

	virtual void TrainingIteration();
	virtual void ValidationIteration();
	virtual void LearningRateEval();
	virtual void Generate() = 0;
	virtual void SaveModel();
	virtual void LoadModel();

	virtual void UpdateOptimizer();

protected:
	NNBatchingProcThread			m_createBatchThread;
	NNBatch							m_currentBatch;

	std::vector<NNModelComponent*>	m_trainModelComponents;
	std::vector<NNModelComponent*>	m_evalModelComponents;

	int								m_trainingIterations;

	as32							m_currentState;
	int								m_requestedState;
	Mutex							m_requestedStateMutex;
	int								m_currentStateIterations;

	torch::Device					m_deviceToUse;

	LogFunc							m_logFunc;

	Clock							m_clock;
	f64								m_timeTraining;

	int								m_batchSize;
	abool							m_optimizerNeedsUpdate;

	int								m_printTrainingRate;
};