#include "NNBatchingProcThread.h"

#include "SharedMLHeader.h"

NNBatchingProcThread::NNBatchingProcThread():
	m_batchOutputDevice(c10::DeviceType::CUDA),
	m_batchSize(2),
	m_batchCount(1),	//	init this to 1 so that we don't try to validate on the first batch and I'm too lazy to do it another way
	m_validationRate(100)
{
}

NNBatchingProcThread::~NNBatchingProcThread()
{
}

void NNBatchingProcThread::Run()
{
	ScopedLock	sLock(m_nextBatchMutex);

	m_nextBatch.Clear();

	//	training batch
	m_nextBatch.GetBatchData(m_batchSize, kBatchType_Training);

	//	validation batch
	if( m_batchCount % m_validationRate == 0 )
	{
		m_nextBatch.GetBatchData(m_batchSize, kBatchType_Validation);
	}

	m_nextBatch.MoveToDevice(m_batchOutputDevice);

	m_batchCount++;
}

void NNBatchingProcThread::CreateNextBatch(torch::Device outputDevice, int batchSize)
{
	m_batchSize = batchSize;
	m_batchOutputDevice = outputDevice;
	RunJob();
}

bool NNBatchingProcThread::GetNextBatch(NNBatch& batchOut)
{
	ScopedLock	sLock(m_nextBatchMutex);

	bool		batchReady = m_nextBatch.m_trainingTagToBatchTensorMap.size() > 0;

	if( batchReady )
	{
		batchOut = m_nextBatch;

		m_nextBatch.Clear();
	}

	return batchReady;
}

void NNBatchingProcThread::SetValidationRate(int validationRate)
{
	m_validationRate = validationRate;
}