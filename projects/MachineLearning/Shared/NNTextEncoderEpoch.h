#pragma once

#include "types.h"

class NNTextEncoderEpoch
{
public:
	class BatchItemData
	{
	public:
		BatchItemData();

	public:
		int	m_dataOffset;
		int	m_sortKey;
	};

public:
	NNTextEncoderEpoch();
	virtual ~NNTextEncoderEpoch();

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