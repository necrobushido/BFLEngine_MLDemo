#include "NNAnimEpoch.h"

#include "AnimSeqDataSet.h"

namespace
{
	int CompareBatchKeys(const void *a, const void *b)
	{
		const NNAnimEpoch::BatchItemData*	pArg1 = (const NNAnimEpoch::BatchItemData*)a;
		const NNAnimEpoch::BatchItemData*	pArg2 = (const NNAnimEpoch::BatchItemData*)b;

		if( pArg1->m_sortKey < pArg2->m_sortKey )
		{
			return -1;
		}

		if( pArg1->m_sortKey > pArg2->m_sortKey )
		{
			return 1;
		}

		return 0;
	}
}

NNAnimEpoch::NNAnimEpoch():
	m_batchItemData(nullptr),
	m_batchItemDataCount(0),
	m_creationCycles(0)
{
}

NNAnimEpoch::~NNAnimEpoch()
{
	delete [] m_batchItemData;
}

void NNAnimEpoch::Init()
{
	//Assert(g_animSeqDataSet != nullptr);

	//	this shouldn't exist when we're in the gui
	if( g_animSeqDataSet != nullptr )
	{
		m_batchItemDataCount = (int)g_animSeqDataSet->m_trainingAnims.size();

		m_batchItemData = new BatchItemData[m_batchItemDataCount];
		for(int animIdx = 0; animIdx < m_batchItemDataCount; ++animIdx)
		{
			BatchItemData&	thisBatchData = m_batchItemData[animIdx];

			thisBatchData.m_animIdx = animIdx;
		}
	}
}

void NNAnimEpoch::CreateBatchItems()
{
	Assert(m_batchItemDataCount > 0);
	m_batchItems.clear();

	//	randomize the batch data order
	for(int batchDataIdx = 0; batchDataIdx < m_batchItemDataCount; batchDataIdx++)
	{
		m_batchItemData[batchDataIdx].m_sortKey = rand();
	}
	qsort(m_batchItemData, m_batchItemDataCount, sizeof(BatchItemData), CompareBatchKeys);

	//	fill the items that "point" to the data
	for(int batchDataIdx = 0; batchDataIdx < m_batchItemDataCount; batchDataIdx++)
	{
		m_batchItems.push_back(batchDataIdx);
	}

	m_creationCycles++;	//	effectively the epoch count
}

/*
	Something to consider:
		the lowest timesteps seem to need more training than the higher timesteps, so we could arrange for them to actually get more training.
		
		One method I considered was having some probability of an item drawn being added back at the front of the list based on its timestep
			could instead add extra copies of entries with some probability inside of CreateBatchItems, or the constructor for m_batchItemData

		another would be to do a training run that only has low timesteps, but for this one I'd wonder if it would interfere with the weights handling of high timesteps
			maybe train on only low timesteps for a while, then switch to a normal training setup?
				gradually add higher timesteps as it progresses?  e.g. 125->250->500->1000?
			maybe worth doing anyway just to see if the same learning rate / etc work;  could the lower timesteps have problems because of such differences?
*/
NNAnimEpoch::BatchItemData NNAnimEpoch::PopNextBatchItem()
{
	if( m_batchItems.size() == 0 )
	{
		CreateBatchItems();
	}
	Assert(m_batchItems.size() > 0);

	int				nextBatchItemDataIdx = m_batchItems[m_batchItems.size() - 1];
	m_batchItems.pop_back();

	BatchItemData	result = m_batchItemData[nextBatchItemDataIdx];

	return result;
}

//
NNAnimEpoch::BatchItemData::BatchItemData():
	m_animIdx(-1)
{
}