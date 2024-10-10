#pragma once

#include "types.h"

class AsciiTokenizer
{
protected:
	class TokenMerge
	{
	public:
		TokenMerge()
		{
		}

		TokenMerge(int firstToken, int secondToken, int mergedToken)
		{
			m_tokensMerged[0] = firstToken;
			m_tokensMerged[1] = secondToken;
			m_mergedToken = mergedToken;
		}

		const int* GetMergedData() const { return m_tokensMerged; }
		int GetMergedToken() const { return m_mergedToken; }

		bool MatchesMerge(int token1, int token2) const
		{
			return token1 == m_tokensMerged[0] && token2 == m_tokensMerged[1];
		}

	public:
		int	m_tokensMerged[2];
		int m_mergedToken;
	};

	class PotentialTokenMerge
	{
	public:
		bool operator<(const PotentialTokenMerge& other) const
		{
			if( m_tokens[0] != other.m_tokens[0] ) 
			{
				return m_tokens[0] < other.m_tokens[0];
			}

			return m_tokens[1] < other.m_tokens[1];
		}

	public:
		int	m_tokens[2];
	};

	enum eSpecialTokens
	{
		kStartToken,
		kEndToken,

		kSpecialTokenCount
	};

public:
	AsciiTokenizer();

	void Init(const std::string& inputString, int desiredVocabSize, bool caseSensitive);

	void Encode(const std::string& inputString, std::vector<int>* tokensOut) const;
	void Decode(const std::vector<int>& inputTokens, std::string* stringOut) const;

	int GetStartToken() const;
	int GetEndToken() const;

	int GetVocabSize() const;

	bool TryToLoadFromFile(const char* filename);
	void SaveToFile(const char* filename);
	void LoadFromFile(const char* filename);

protected:
	void EncodeBase(const std::string& inputString, std::vector<int>* tokensOut) const;
	void CreateInitialTokens(const std::string& inputString, bool caseSensitive);
	void CountTokenPairs(const std::vector<int>& inputTokens, std::map<PotentialTokenMerge, int>* potentialMerges);
	void AddSpecialTokens();

protected:
	int							m_encodeAsciiToInt[256];
	std::vector<TokenMerge>		m_merges;
	std::vector<std::string>	m_tokenList;
	int							m_specialTokens[kSpecialTokenCount];
};