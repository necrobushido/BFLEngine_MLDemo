#include "NNTextEncoderEpoch.h"

#include "AnimSeqDataSet.h"

#include "NNTextEncoder.h"

namespace
{
	int CompareBatchKeys(const void *a, const void *b)
	{
		const NNTextEncoderEpoch::BatchItemData*	pArg1 = (const NNTextEncoderEpoch::BatchItemData*)a;
		const NNTextEncoderEpoch::BatchItemData*	pArg2 = (const NNTextEncoderEpoch::BatchItemData*)b;

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

NNTextEncoderEpoch::NNTextEncoderEpoch():
	m_batchItemData(nullptr),
	m_batchItemDataCount(0),
	m_creationCycles(0)
{
}

NNTextEncoderEpoch::~NNTextEncoderEpoch()
{
	delete [] m_batchItemData;
}

void NNTextEncoderEpoch::Init()
{	
	//Assert(g_animSeqDataSet != nullptr);

	//	this shouldn't exist when we're in the gui
	if( g_animSeqDataSet != nullptr )
	{
		//	-2 for start and end tokens, -1 for next token
		int	numDataOffsets = (int)g_animSeqDataSet->m_trainingText.size(0) - (NNTextEncoderImpl::kMaxTokens - 2) - 1;

		m_batchItemDataCount = numDataOffsets;

		m_batchItemData = new BatchItemData[m_batchItemDataCount];
		for(int batchItemIdx = 0; batchItemIdx < m_batchItemDataCount; ++batchItemIdx)
		{
			BatchItemData&	thisBatchData = m_batchItemData[batchItemIdx];

			thisBatchData.m_dataOffset = batchItemIdx;
		}
	}
}

void NNTextEncoderEpoch::CreateBatchItems()
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

NNTextEncoderEpoch::BatchItemData NNTextEncoderEpoch::PopNextBatchItem()
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
NNTextEncoderEpoch::BatchItemData::BatchItemData():
	m_dataOffset(-1)
{
}