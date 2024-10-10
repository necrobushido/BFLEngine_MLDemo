#pragma once

#include "types.h"

class NNAnimEpoch
{
public:
	class BatchItemData
	{
	public:
		BatchItemData();

	public:
		int	m_animIdx;
		int	m_sortKey;
	};

public:
	NNAnimEpoch();
	virtual ~NNAnimEpoch();

	void Init();

	void CreateBatchItems();
	BatchItemData PopNextBatchItem();

	int GetCreationCycles() const { return m_creationCycles; }
	int GetEpochSize() const { return m_batchItemDataCount; }

protected:
	BatchItemData*		m_batchItemData;
	int					m_batchItemDataCount;
	std::vector<int>	m_batchItems;

	int					m_creationCycles;
};