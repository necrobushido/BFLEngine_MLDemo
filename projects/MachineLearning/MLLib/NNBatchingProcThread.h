#pragma once

#include "JobThread.h"
#include "Mutex.h"

#include "NNBatch.h"

class NNBatchingProcThread : public JobThread
{
public:
	NNBatchingProcThread();
	virtual ~NNBatchingProcThread();

public:
	void CreateNextBatch(torch::Device outputDevice, int batchSize);
	bool GetNextBatch(NNBatch& batchOut);

	void SetValidationRate(int validationRate);

protected:
	void Run() override;

public:
	torch::Device			m_batchOutputDevice;
	Mutex					m_nextBatchMutex;
	NNBatch					m_nextBatch;
	int						m_batchSize;
	int						m_batchCount;
	int						m_validationRate;
};