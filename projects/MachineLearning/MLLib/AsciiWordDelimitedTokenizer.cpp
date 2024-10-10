#include "AsciiWordDelimitedTokenizer.h"

#include "FileManager.h"

#include <regex>

namespace
{
	bool ArrayContains(const std::vector<std::string>& inputArray, std::string testToken)
	{
		bool	result = false;
		
		for(u32 i = 0; i < inputArray.size() && !result; ++i)
		{
			result = inputArray[i] == testToken;
		}

		return result;
	}

	std::string RemoveExtraWhitespace(const std::string& inputText) 
	{
		// Step 1: Use regex to replace sequences of whitespace with a single space
		std::regex	whitespaceRegexPattern("\\s+");
		std::string	result = std::regex_replace(inputText, whitespaceRegexPattern, " ");
    
		// Step 2: Remove leading and trailing whitespace
		result.erase(result.begin(), std::find_if(result.begin(), result.end(), 
			[](unsigned char ch)
			{
				return !std::isspace(ch);
			}));
    
		result.erase(std::find_if(result.rbegin(), result.rend(), 
			[](unsigned char ch) 
			{
				return !std::isspace(ch);
			}).base(), result.end());
    
		return result;
	}

	//	this one was going to be an attempt to have sequences such as "(2)" end up as a single token, but hasn't been tested
	const std::string	gpt2WithParentheses = R"('s|'t|'re|'ve|'m|'ll|'d| ?[a-zA-Z]+| ?[0-9]+| ?\([^a-zA-Z0-9\s]+\)| ?[^a-zA-Z0-9\s]+|\s+(?!\S)|\s+)";

	const std::string	gpt2Pattern = R"('s|'t|'re|'ve|'m|'ll|'d| ?[a-zA-Z]+| ?[0-9]+| ?[^a-zA-Z0-9\s]+|\s+(?!\S)|\s+)";
	//const std::string	cl100kPattern = R"('(?i:[sdmt]|ll|ve|re)|[^\r\na-zA-Z0-9]?+[a-zA-Z]+|[0-9]{1,3}| ?[^a-zA-Z0-9\s]++[\r\n]*|\s*[\r\n]|\s+(?!\S)|\s+)";
	//const std::string	cl100kPattern = R"('([sSdDmMtT]|ll|LL|ve|VE|re|RE)|[^\r\na-zA-Z0-9]?+[a-zA-Z]+|[0-9]{1,3}| ?[^a-zA-Z0-9\s]+[\r\n]*|\s*[\r\n]|\s+(?!\S)|\s+)";
	//const std::string	cl100kPattern = R"('([sSdDmMtT]|ll|LL|ve|VE|re|RE)|[^\r\na-zA-Z0-9]*[a-zA-Z]+|[0-9]{1,3}| ?[^a-zA-Z0-9\s]+[\r\n]*|\s*[\r\n]|\s+(?!\S)|\s+)";
	//const std::string	cl100kPattern = R"('([sSdDmMtT]|ll|ve|re)|[^\\r\\n\\w]?+[a-zA-Z]+|\d{1,3}| ?[^\s\w]+[\r\n]*|\s*[\r\n]|\s+(?!\S)|\s+)";

	//std::string		pat = R"('s|'t|'re|'ve|'m|'ll|'d| ?[[:alpha:]]+| ?[[:digit:]]+| ?[^\s[:alpha:][:digit:]]+|\s+(?!\S)|\s+)";	//	taken from another example.  looks similar to gpt2Pattern above.

	//const std::string	cl100kPattern = 
	//	R"('([sSdDmMtT]|ll|ve|re))"		// Contractions or abbreviations
	//	R"(|[^\\r\\n\\w]?+[a-zA-Z]+)"	// Sequences of letters
	//	R"(|\d{1,3})"					// Sequences of numbers
	//	R"(| ?[^\s\w]+[\r\n]*)"			// Special sequences
	//	R"(|\s*[\r\n])"					// Whitespace followed by line breaks
	//	R"(|\s+(?!\S))"					// Trailing whitespace with negative lookahead
	//	R"(|\s+)";						// One or more whitespace characters

	/*
	def cl100k_base():
    mergeable_ranks = load_tiktoken_bpe(
        "https://openaipublic.blob.core.windows.net/encodings/cl100k_base.tiktoken",
        expected_hash="223921b76ee99bde995b7ff738513eef100fb51d18c93597a113bcffe865b2a7",
    )
    special_tokens = {
        ENDOFTEXT: 100257,	//	256 base, 100k merges, then add special characters, some of which are listed here
        FIM_PREFIX: 100258,
        FIM_MIDDLE: 100259,
        FIM_SUFFIX: 100260,
        ENDOFPROMPT: 100276,
    }
    return {
        "name": "cl100k_base",
        "pat_str": r"""'(?i:[sdmt]|ll|ve|re)|[^\r\n\p{L}\p{N}]?+\p{L}+|\p{N}{1,3}| ?[^\s\p{L}\p{N}]++[\r\n]*|\s*[\r\n]|\s+(?!\S)|\s+""",
        "mergeable_ranks": mergeable_ranks,
        "special_tokens": special_tokens,
    }
	*/

	const std::string	kRegexPatternToUse = gpt2Pattern;
}

AsciiWordDelimitedTokenizer::AsciiWordDelimitedTokenizer():
	m_caseSensitive(false)
{
}

void AsciiWordDelimitedTokenizer::Init(const std::string& inputString, int desiredVocabSize, bool caseSensitive)
{
	m_caseSensitive = caseSensitive;

	m_tokenList.clear();
	m_tokenMap.clear();

	std::string	emptyStr = "";
	m_tokenList.push_back(emptyStr);	//	special 0th token
	m_tokenMap[emptyStr] = (int)m_tokenList.size() - 1;

	//	split the input into chunks based on regex
	std::regex						regexPat(kRegexPatternToUse, std::regex_constants::ECMAScript | std::regex_constants::icase);

	std::sregex_token_iterator		inputStringRegexIter(inputString.begin(), inputString.end(), regexPat);
	std::sregex_token_iterator		inputStringRegexEnd;

	std::vector<std::vector<int>>	inputSubstringTokens;
	while(inputStringRegexIter != inputStringRegexEnd)
	{
		std::vector<int>	thisSubstrTokens;
		std::string			thisSubStr = RemoveExtraWhitespace(inputStringRegexIter->str());

		InitialEncoding(thisSubStr, &thisSubstrTokens, true);

		inputSubstringTokens.push_back(thisSubstrTokens);

		inputStringRegexIter++;
	}

	DebugPrintf("Initial tokens:\n");
	for(int tIdx = 0; (int)tIdx < m_tokenList.size(); tIdx++)
	{
		DebugPrintf("\tToken #%d, : \"%s\"\n", tIdx, m_tokenList[tIdx].c_str());
	}

	DebugPrintf("Merged tokens:\n");

	int	nonSpecialTokensDesired = desiredVocabSize - kSpecialTokenCount;

	//	reduce token count in the input and increase token vocabulary
	while(m_tokenList.size() < nonSpecialTokensDesired)
	{
		//	count token pair occurances
		std::map<PotentialTokenMerge, int>	potentialMerges;
		for(int inputSubstrIdx = 0; inputSubstrIdx < inputSubstringTokens.size(); ++inputSubstrIdx)
		{
			CountTokenPairs(inputSubstringTokens[inputSubstrIdx], &potentialMerges);
		}

		//	find the token pair with the largest occurance
		int			largestOccurance = 0;
		int			firstTokenIdx;
		int			secondTokenIdx;
		for(std::map<PotentialTokenMerge, int>::const_iterator it = potentialMerges.begin(); it != potentialMerges.end(); ++it)
		{
			if( it->second > largestOccurance ) 
			{
				largestOccurance = it->second;

				firstTokenIdx = it->first.m_tokens[0];
				secondTokenIdx = it->first.m_tokens[1];
			}
		}

		if( largestOccurance > 1 )
		{
			//	create a merge for that token pair
			std::string	mergedTokenData = m_tokenList[firstTokenIdx] + m_tokenList[secondTokenIdx];

			m_tokenList.push_back(mergedTokenData);
			m_tokenMap[mergedTokenData] = (int)m_tokenList.size() - 1;

			TokenMerge			thisMerge(firstTokenIdx, secondTokenIdx, (int)m_tokenList.size()-1);
			m_merges.push_back(thisMerge);

			DebugPrintf("\tToken #%d, Merge #%d created : \"%s\"\n", (int)m_tokenList.size()-1, (int)m_merges.size(), mergedTokenData.c_str());

			//	rebuild the substring tokens by just replacing the newly merged ones
			for(int subStrIdx = 0; subStrIdx < (int)inputSubstringTokens.size(); subStrIdx++)
			{
				std::vector<int>	newSubstrTokens;
				int subStrTokenIdx = 0;
				for(subStrTokenIdx = 0; subStrTokenIdx < (int)inputSubstringTokens[subStrIdx].size()-1; subStrTokenIdx++)
				{
					if( inputSubstringTokens[subStrIdx][subStrTokenIdx] == firstTokenIdx && 
						inputSubstringTokens[subStrIdx][subStrTokenIdx+1] == secondTokenIdx )
					{
						newSubstrTokens.push_back((int)m_tokenList.size()-1);
						subStrTokenIdx++;	//	count for two tokens
					}
					else
					{
						newSubstrTokens.push_back(inputSubstringTokens[subStrIdx][subStrTokenIdx]);
					}
				}

				if( subStrTokenIdx == (int)inputSubstringTokens[subStrIdx].size()-1 )
				{
					//	last token wasn't merged so add it
					newSubstrTokens.push_back(inputSubstringTokens[subStrIdx][(int)inputSubstringTokens[subStrIdx].size()-1]);
				}

				inputSubstringTokens[subStrIdx] = newSubstrTokens;
			}
		}
		else
		{
			//	no reason to continue if there are no duplicated pairs left
			break;
		}
	}

	//	add special characters
	{
		AddSpecialTokens();
	}
}

void AsciiWordDelimitedTokenizer::InitialEncoding(const std::string& inputString, std::vector<int>* tokensOut, bool constructTokens)
{
	for(int thisTokenDataIdx = 0; thisTokenDataIdx < (int)inputString.size(); ++thisTokenDataIdx)
	{
		char		charAsString[2];		
		charAsString[0] = inputString[thisTokenDataIdx];
		charAsString[1] = '\0';

		std::string	tokenStr = charAsString;

		if( !m_caseSensitive &&
			tokenStr[0] > 0x40 && 
			tokenStr[0] < 0x5B )
		{
			//	make the character lower case
			tokenStr[0] = (char)(tokenStr[0] + 0x20);
		}

		if( thisTokenDataIdx == (int)inputString.size() - 1 )
		{
			//	this is a special "end of word" token
			tokenStr = tokenStr + "<\\w>";
		}

		if( constructTokens &&
			m_tokenMap.find(tokenStr) == m_tokenMap.end() )
		{
			//	token not found, so add it
			m_tokenList.push_back(tokenStr);
			m_tokenMap[tokenStr] = (int)m_tokenList.size() - 1;
		}

		int	thisToken = m_tokenMap[tokenStr];

		tokensOut->push_back(thisToken);
	}
}

/*
	remove leading and trailing whitespace
	add "end of word" marker

	RemoveExtraWhitespace
*/
void AsciiWordDelimitedTokenizer::EncodeBase(const std::string& inputString, std::vector<int>* tokensOut)
{
	(*tokensOut).clear();

	//	initial encoding
	InitialEncoding(inputString, tokensOut, false);

	//Assert(tokensOut->size() > 0);

	//	perform merges
	for(size_t thisMergeIdx = 0; thisMergeIdx < m_merges.size(); ++thisMergeIdx)
	{
		TokenMerge			thisMerge = m_merges[thisMergeIdx];

		std::vector<int>	mergedTokens;

		int					currentTokenCount = (int)tokensOut->size();

		if( currentTokenCount > 1 )
		{
			int	i = 0;
			for(i = 0; i < currentTokenCount-1; ++i)
			{
				int	firstToken = (*tokensOut)[i];
				int	secondToken = (*tokensOut)[i+1];

				if( thisMerge.MatchesMerge(firstToken, secondToken) )
				{
					mergedTokens.push_back(thisMerge.GetMergedToken());
					i++;	//	we want this to count for two original tokens
				}
				else
				{
					mergedTokens.push_back(firstToken);
				}
			}

			if( i == currentTokenCount-1 )
			{
				//	this indicates that the last operation in the loop was not a merge, so we need to add the last token
				mergedTokens.push_back((*tokensOut)[i]);
			}

			Assert(mergedTokens.size() > 0);

			*tokensOut = mergedTokens;
		}
		else
		{
			//	no merging possible with only 1 token
		}		
	}

	//Assert(tokensOut->size() > 0);
}

void AsciiWordDelimitedTokenizer::Encode(const std::string& inputString, std::vector<int>* tokensOut)
{
	//	init
	(*tokensOut).clear();

	//	split the inputString into substrings
	std::regex						regexPat(kRegexPatternToUse, std::regex_constants::ECMAScript | std::regex_constants::icase);

	std::sregex_token_iterator		inputStringRegexIter(inputString.begin(), inputString.end(), regexPat);
	std::sregex_token_iterator		inputStringRegexEnd;

	//	encode substrings
	std::vector<std::vector<int>>	inputSubstringTokens;
	while(inputStringRegexIter != inputStringRegexEnd)
	{
		std::vector<int>	thisSubstrTokens;
		std::string			thisSubStr = RemoveExtraWhitespace(inputStringRegexIter->str());

		EncodeBase(thisSubStr, &thisSubstrTokens);

		inputSubstringTokens.push_back(thisSubstrTokens);

		inputStringRegexIter++;
	}

	if( inputSubstringTokens.size() == 0 )
	{
		//	consider as a single null token?  would prefer not to I think.
		//(*tokensOut).push_back(0);
	}
	else
	{
		//	concat all of the encoded values
		(*tokensOut) = inputSubstringTokens[0];
		for(int substrIdx = 1; substrIdx < inputSubstringTokens.size(); ++substrIdx)
		{
			for(int i = 0; i < inputSubstringTokens[substrIdx].size(); ++i)
			{
				(*tokensOut).push_back(inputSubstringTokens[substrIdx][i]);
			}
		}
	}
}

void AsciiWordDelimitedTokenizer::Decode(const std::vector<int>& inputTokens, std::string* stringOut) const
{
	*stringOut = "";

	for(size_t tokenIdx = 0; tokenIdx < inputTokens.size(); ++tokenIdx)
	{
		*stringOut = (*stringOut) + m_tokenList[inputTokens[tokenIdx]];
	}
}

void AsciiWordDelimitedTokenizer::CountTokenPairs(const std::vector<int>& inputTokens, std::map<PotentialTokenMerge, int>* potentialMerges)
{
	//	don't clear potentialMerges here, because we want to add our counts to previous counts

	for(int i = 0; i < (int)inputTokens.size()-1; ++i)
	{
		PotentialTokenMerge	thisPotentialMerge;
		thisPotentialMerge.m_tokens[0] = inputTokens[i];
		thisPotentialMerge.m_tokens[1] = inputTokens[i+1];

		int	prevCount = (*potentialMerges)[thisPotentialMerge];	//	I'm not an expert on std::map, but ChatGPT claims this will insert a 0 if it does not currently exist
		(*potentialMerges)[thisPotentialMerge] = prevCount + 1;
	}
}

void AsciiWordDelimitedTokenizer::AddSpecialTokens()
{
	m_specialTokens[kStartSequenceToken] = (int)m_tokenList.size();
	std::string	startTokenDecodeTxt = "<start>";
	m_tokenList.push_back(startTokenDecodeTxt);
	m_tokenMap[startTokenDecodeTxt] = (int)m_tokenList.size() - 1;

	m_specialTokens[kEndSequenceToken] = (int)m_tokenList.size();
	std::string	endTokenDecodeTxt = "<end>";
	m_tokenList.push_back(endTokenDecodeTxt);
	m_tokenMap[endTokenDecodeTxt] = (int)m_tokenList.size() - 1;
}

bool AsciiWordDelimitedTokenizer::TryToLoadFromFile(const char* filename)
{
	bool	fileExists = g_fileManager->FileExists(filename);
	if( fileExists )
	{
		LoadFromFile(filename);
	}

	return fileExists;
}

void AsciiWordDelimitedTokenizer::SaveToFile(const char* filename)
{
	const char*	dataDir = g_fileManager->GetDataDir();

	char		fullFilePath[MAX_PATH];
	strcpy(fullFilePath, dataDir);
	strcat(fullFilePath, filename);

	FILE*		pFile = fopen(fullFilePath, "wb");
	if( pFile == NULL )
	{
		AssertMsg(0, "error opening file %s for writing\n", fullFilePath);
		return;
	}

	size_t		elementsWritten = 0;

	//	m_encodeAsciiToInt
	{
		elementsWritten = fwrite(&m_caseSensitive, sizeof(bool), 1, pFile);
		Assert(elementsWritten == 1);
	}

	//	m_merges
	{
		int	mergeCount = (int)m_merges.size();
		elementsWritten = fwrite(&mergeCount, sizeof(int), 1, pFile);
		Assert(elementsWritten == 1);

		elementsWritten = fwrite(m_merges.data(), sizeof(TokenMerge), mergeCount, pFile);
		Assert(elementsWritten == mergeCount);
	}

	//	m_tokenList
	{
		int	tokenCount = (int)m_tokenList.size() - kSpecialTokenCount;	//	don't save the special tokens, we'll just manually add them on load
		elementsWritten = fwrite(&tokenCount, sizeof(int), 1, pFile);
		Assert(elementsWritten == 1);

		for(int i = 0; i < tokenCount; ++i)
		{
			int	stringLength = (int)m_tokenList[i].length();
			elementsWritten = fwrite(m_tokenList[i].data(), sizeof(char), stringLength, pFile);
			Assert(elementsWritten == stringLength);

			char	nullChar = '\0';
			elementsWritten = fwrite(&nullChar, sizeof(char), 1, pFile);
			Assert(elementsWritten == 1);
		}
	}

	fclose(pFile);
}

void AsciiWordDelimitedTokenizer::LoadFromFile(const char* filename)
{
	FileRef<char>		fileRef = g_fileManager->MakeRef(filename);
	const char*			fileData = (*fileRef);
	const size_t		fileSize = fileRef.FileSize();

	const char*			fileDataPtr = fileData;

	//	m_encodeAsciiToInt
	{
		memcpy(&m_caseSensitive, fileDataPtr, sizeof(bool));
		fileDataPtr += sizeof(bool);
	}

	//	m_merges
	{
		int	mergeCount = ((int*)fileDataPtr)[0];
		fileDataPtr += sizeof(int);

		m_merges.resize(mergeCount);

		memcpy(m_merges.data(), fileDataPtr, sizeof(TokenMerge) * mergeCount);

		fileDataPtr += sizeof(TokenMerge) * mergeCount;
	}

	//	m_tokenList
	{
		int	tokenCount = ((int*)fileDataPtr)[0];
		fileDataPtr += sizeof(int);

		for(int i = 0; i < tokenCount; ++i)
		{
			char	strBuffer[1024];
			int		strBufferIdx = 0;
			while(fileDataPtr[0] != '\0' && strBufferIdx < 1023)
			{
				strBuffer[strBufferIdx] = *fileDataPtr;

				strBufferIdx++;
				fileDataPtr++;
			}

			Assert(fileDataPtr[0] == '\0');
			fileDataPtr++;

			strBuffer[strBufferIdx] = '\0';

			std::string	thisToken = strBuffer;

			m_tokenList.push_back(thisToken);
			m_tokenMap[thisToken] = (int)m_tokenList.size() - 1;
		}
	}

	//	special characters
	AddSpecialTokens();
}

int AsciiWordDelimitedTokenizer::GetStartToken() const
{
	return m_specialTokens[kStartSequenceToken];
}

int AsciiWordDelimitedTokenizer::GetEndToken() const
{
	return m_specialTokens[kEndSequenceToken];
}

int AsciiWordDelimitedTokenizer::GetVocabSize() const
{
	return (int)m_tokenList.size();
}