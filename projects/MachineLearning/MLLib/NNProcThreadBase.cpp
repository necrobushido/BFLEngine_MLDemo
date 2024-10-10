#include "NNProcThreadBase.h"

#include "FileManager.h"
#include "MathNamespace.h"

#include "NNModelComponent.h"

namespace
{
	const char*	s_stateNameStrings[] =
	{
		"Invalid",
		"Initializing",
		"Idle",
		"Evaluating Learning Rates",
		"Training",
		"Generating",
		"Saving",
		"Loading",
		"State Count / Invalid"
	};
}

const char* NNProcessingThreadBase::GetStateName(int state)
{
	Assert(state < kProcessState_StateCount && state >= 0);

	return s_stateNameStrings[state];
}

NNProcessingThreadBase::NNProcessingThreadBase():
	m_currentState(kProcessState_Init),
	m_requestedState(kProcessState_Invalid),
	m_currentStateIterations(0),
	m_deviceToUse(c10::DeviceType::CPU),
	m_logFunc(nullptr),
	m_printTrainingRate(10)
{
	const bool	CUDAIsAvailable = torch::cuda::is_available();
	if( CUDAIsAvailable )
	{
		m_deviceToUse = c10::DeviceType::CUDA;
	}
}

NNProcessingThreadBase::~NNProcessingThreadBase()
{
	for(int i = 0; i < m_trainModelComponents.size(); ++i)
	{
		delete m_trainModelComponents[i];
	}

	for(int i = 0; i < m_evalModelComponents.size(); ++i)
	{
		delete m_evalModelComponents[i];
	}
}

void NNProcessingThreadBase::Run()
{
	UpdateState();
}

void NNProcessingThreadBase::ChangeState(int stateToChangeTo)
{
	m_requestedStateMutex.Enter();
	m_requestedState = stateToChangeTo;
	m_requestedStateMutex.Exit();
}

int NNProcessingThreadBase::GetState()
{
	return m_currentState;
}

void NNProcessingThreadBase::SetLogFunc(LogFunc logFuncToUse)
{
	m_logFunc = logFuncToUse;
}

void NNProcessingThreadBase::TrainingIteration()
{
	bool	batchReady = m_createBatchThread.GetNextBatch(m_currentBatch);		//	get the last created batch
	m_createBatchThread.CreateNextBatch(m_deviceToUse, m_batchSize);			//	signal the batch thread to go ahead and start making the next batch

	if( batchReady )
	{
		++m_trainingIterations;

		m_timeTraining += m_clock.GetTimeSlice();

		if( m_optimizerNeedsUpdate )
		{
			m_optimizerNeedsUpdate = false;
			UpdateOptimizer();
		}

		//bool	printingIsOk = (m_trainingIterations % m_printTrainingRate) == 0;
		bool	printingIsOk = true;
		bool	requestEndTraining = false;

		for(int i = 0; i < m_trainModelComponents.size(); ++i)
		{
			m_trainModelComponents[i]->TrainingIteration(m_currentBatch.m_trainingTagToBatchTensorMap, printingIsOk);

			requestEndTraining = requestEndTraining || m_trainModelComponents[i]->RequestTrainingEnd();
		}

		if( printingIsOk )
		{
			f64	batchesPerSecond = (f64)m_trainingIterations / m_timeTraining;
			//DebugPrintf("%f batches per second of size %d\n", batchesPerSecond, m_batchSize);
			//DebugPrintf("training rate = %f over %f minutes\n\n", batchesPerSecond * m_batchSize, m_timeTraining / 60.0);
		}

		ValidationIteration();

		if( requestEndTraining )
		{
			DebugPrintf("\nNNProcessingThreadBase::TrainingIteration : training finished via requested end\n");

			ChangeState(kProcessState_Idle);
		}
	}
}

void NNProcessingThreadBase::ValidationIteration()
{
	if( m_currentBatch.m_validationTagToBatchTensorMap.size() > 0 )
	{
		DebugPrintf("NNProcessingThreadBase::ValidationIteration : %f minutes, %d training iterations\n", m_timeTraining / 60.0, m_trainingIterations);

		for(int i = 0; i < m_trainModelComponents.size(); ++i)
		{
			m_trainModelComponents[i]->ValidationIteration(m_currentBatch.m_validationTagToBatchTensorMap);
		}

		 m_currentBatch.m_validationTagToBatchTensorMap.clear();
	}
}

void NNProcessingThreadBase::LearningRateEval()
{
	float			lowestExpGuess = -10.0f;
	float			highestExpGuess = 0.0f;
	int				steps = 100;
	torch::Tensor	candidateLearningRateExponents = torch::linspace(lowestExpGuess, highestExpGuess, steps);
	torch::Tensor	candidateLearningRates = torch::pow(10.0, candidateLearningRateExponents);
	for(int i = 0; i < steps; ++i)
	{
		float	thisLearningRate = candidateLearningRates[i].item().toFloat();
		SetLearningRate(thisLearningRate);

		//AnimGenProcessingThreadBase::TrainingIteration();

		if( m_optimizerNeedsUpdate )
		{
			m_optimizerNeedsUpdate = false;
			UpdateOptimizer();
		}

		m_createBatchThread.GetNextBatch(m_currentBatch);					//	get the last created batch
		m_createBatchThread.CreateNextBatch(m_deviceToUse, m_batchSize);	//	signal the batch thread to go ahead and start making the next batch

		for(int trainingComponentIdx = 0; trainingComponentIdx < m_trainModelComponents.size(); ++trainingComponentIdx)
		{
			m_trainModelComponents[trainingComponentIdx]->TrainingIteration(m_currentBatch.m_trainingTagToBatchTensorMap, false);
		}
	}

	const std::vector<f32>* pLossHistory = nullptr;
	for(int i = 0; i < m_trainModelComponents.size(); ++i)
	{
		const std::vector<f32>* pThisComponentLossHistory = m_trainModelComponents[i]->GetLossHistory();
		if( pThisComponentLossHistory != nullptr )
		{
			pLossHistory = pThisComponentLossHistory;
		}
	}

	//	graph or evaluate loss values
	char	buffer[256];
	sprintf(buffer, "NNProcessingThreadBase::LearningRateEvalIteration : eval complete\n");
	m_logFunc(buffer);

	if( pLossHistory )
	{
		for(int i = 0; i < steps; ++i)
		{
			float	thisLearningRate = candidateLearningRates[i].item().toFloat();
			float	thisLoss = (*pLossHistory)[i];

			sprintf(buffer, "\t%d : lr = %.9g, loss = %.9g\n", i, thisLearningRate, thisLoss);
			m_logFunc(buffer);
		}
	}
	else
	{
		printf(buffer, "\tno loss history found!\n");
		m_logFunc(buffer);
	}

	for(int i = 0; i < m_trainModelComponents.size(); ++i)
	{
		m_trainModelComponents[i]->ResetLossHistory();
	}
}

void NNProcessingThreadBase::UpdateState()
{
	int	prevState = m_currentState;

	//	check for a state update
	m_requestedStateMutex.Enter();
	if( m_requestedState != kProcessState_Invalid && m_currentState != m_requestedState )
	{
		m_currentState = m_requestedState;
		m_currentStateIterations = 0;

		m_requestedState = kProcessState_Invalid;
	}
	m_requestedStateMutex.Exit();

	//	check for new state and do initializations
	if( m_currentStateIterations == 0 )
	{
		LeavingState(prevState);
		EnteringState(m_currentState);
	}

	//	the actual update
	switch(m_currentState)
	{
	case kProcessState_Idle:
		break;

	case kProcessState_Init:
		{
			ChangeState(kProcessState_Idle);
		}
		break;

	case kProcessState_EvalLearningRate:
		{
			LearningRateEval();
			ChangeState(kProcessState_Idle);
		}
		break;

	case kProcessState_Training:
		{
			TrainingIteration();
		}
		break;

	case kProcessState_Generating:
		{
			ChangeState(kProcessState_Idle);
		}
		break;

	case kProcessState_Save:
		{
			ChangeState(kProcessState_Idle);
			if( m_logFunc != nullptr )
			{
				char	logBuffer[256];
				sprintf(logBuffer, "finished model save\n");
				m_logFunc(logBuffer);
			}
		}
		break;

	case kProcessState_Load:
		{
			ChangeState(kProcessState_Idle);
			if( m_logFunc != nullptr )
			{
				char	logBuffer[256];
				sprintf(logBuffer, "finished model load\n");
				m_logFunc(logBuffer);
			}
		}
		break;

	default:
		break;
	}

	m_currentStateIterations++;
}

void NNProcessingThreadBase::EnteringState(int currentState)
{
	switch(m_currentState)
	{
	case kProcessState_Idle:
		{				
		}
		break;

	case kProcessState_Init:
		{
			UpdateOptimizer();

			//	pre-emptively load eval components
			{		
				for(int i = 0; i < m_evalModelComponents.size(); ++i)
				{
					m_evalModelComponents[i]->Load();
					m_evalModelComponents[i]->EvalMode();
				}
			}

			m_createBatchThread.Start();
		}
		break;

	case kProcessState_EvalLearningRate:
		{
		}
		break;

	case kProcessState_Training:
		{
			f64	trainStart = m_clock.GetTimeSlice();	//	unused output; just marking the start of measured time (TODO : make this more obvious)

			m_createBatchThread.CreateNextBatch(m_deviceToUse, m_batchSize);

			for(int i = 0; i < m_trainModelComponents.size(); ++i)
			{
				m_trainModelComponents[i]->TrainMode();
			}
		}
		break;

	case kProcessState_Generating:
		{
			for(int i = 0; i < m_trainModelComponents.size(); ++i)
			{
				m_trainModelComponents[i]->EvalMode();
			}

			Generate();
		}
		break;

	case kProcessState_Save:
		{
			SaveModel();
		}
		break;

	case kProcessState_Load:
		{
			LoadModel();
		}
		break;

	default:
		break;
	}
}

void NNProcessingThreadBase::LeavingState(int prevState)
{
	switch(prevState)
	{
	case kProcessState_Training:
		{
			//	wait for the thread to finish since it may be accessing the dataset
			while(m_createBatchThread.IsJobRunning()){}
		}
		break;

	default:
		break;
	}
}

void NNProcessingThreadBase::UpdateOptimizer()
{
	for(int i = 0; i < m_trainModelComponents.size(); ++i)
	{
		m_trainModelComponents[i]->UpdateOptimizer();
	}
}

void NNProcessingThreadBase::SaveModel()
{
	torch::NoGradGuard	no_grad;

	for(int i = 0; i < m_trainModelComponents.size(); ++i)
	{
		m_trainModelComponents[i]->Save();
	}
}

void NNProcessingThreadBase::LoadModel()
{
	torch::NoGradGuard	no_grad;

	for(int i = 0; i < m_evalModelComponents.size(); ++i)
	{
		m_evalModelComponents[i]->Load();
	}

	for(int i = 0; i < m_trainModelComponents.size(); ++i)
	{
		m_trainModelComponents[i]->Load();
	}

	UpdateOptimizer();
}

void NNProcessingThreadBase::SetLearningRate(f32 learningRate)
{
	for(int i = 0; i < m_trainModelComponents.size(); ++i)
	{
		m_trainModelComponents[i]->SetLearningRate(learningRate);
	}

	m_optimizerNeedsUpdate = true;
}