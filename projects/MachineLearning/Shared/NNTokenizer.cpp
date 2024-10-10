#include "NNTokenizer.h"

#include "FileManager.h"

namespace
{
	//const char*	kTrainingFile = "TinyShakespeare.txt";		//	evidently "punch" doesn't exist in shakespeare?  and "kick" shows up only once?
	//const char*	kTrainingFile = "OxfordEnglishDictionary.txt";
	const char*	kTrainingFile = "ConcatAnimDesc.txt";
	
	const char*	kSavedFile = "NNTokenizer.dat";

	//	full anim list stats
	//const int	kDesiredVocabSize = 48;		//	minimum with full anim list
	//const int	kDesiredVocabSize = 56;
	//const int	kDesiredVocabSize = 64;		//	best CLIP performing size that I've tested
	const int	kDesiredVocabSize = 77;
	//const int	kDesiredVocabSize = 80;
	//const int	kDesiredVocabSize = 96;
	//const int	kDesiredVocabSize = 128;
	//const int	kDesiredVocabSize = 256;
	//const int	kDesiredVocabSize = 512;
	//const int	kDesiredVocabSize = 4096;	//	with the full anim list this ends up with ~1290 tokens

	//	dictionary stats
	//const int	kDesiredVocabSize = 16384;	//	this is the lowest power of 2 that has " punch" as a single token
	//const int	kDesiredVocabSize = 256;	//	this is the lowest power of 2 that works with the base characters in the dictionary dataset

	/*
	Token #1391, Merge #1261 created : " run"
	Token #1894, Merge #1764 created : " walk"
	Token #2001, Merge #1871 created : " lay"
	Token #2375, Merge #2245 created : " dance"
	Token #4505, Merge #4375 created : " jump"
	Token #5413, Merge #5283 created : " kick"
	Token #11690, Merge #11560 created : " punch"
	*/

	const bool	kCaseSensitive = false;
}

//
NNTokenizer*	g_NNTokenizer = nullptr;

NNTokenizer::NNTokenizer()
{
	Assert(g_NNTokenizer == nullptr);
	g_NNTokenizer = this;
}

NNTokenizer::~NNTokenizer()
{
	Assert(g_NNTokenizer == this);
	g_NNTokenizer = nullptr;
}

void NNTokenizer::Init()
{
	bool	loadedFromDisk = m_baseTokenizer.TryToLoadFromFile(kSavedFile);
	if( !loadedFromDisk )
	{
		DebugPrintf("Building tokenizer data.  Could take a while.\n");

		FileRef<char>		fileRef = g_fileManager->MakeRef(kTrainingFile);
		const char*			fileData = (*fileRef);
		const size_t		fileSize = fileRef.FileSize();

		//	I want this as a string, so do some annoying shuffling
		//		make this more efficient
		char*				processData = new char[fileSize+1];
		strncpy(processData, fileData, fileSize);
		processData[fileSize] = 0;

		std::string			inputString = processData;

		m_baseTokenizer.Init(inputString, kDesiredVocabSize, kCaseSensitive);

		m_baseTokenizer.SaveToFile(kSavedFile);

		delete [] processData;
	}
}

void NNTokenizer::Encode(const std::string& inputString, std::vector<int>* tokensOut)
{
	m_baseTokenizer.Encode(inputString, tokensOut);
}

void NNTokenizer::Decode(const std::vector<int>& inputTokens, std::string* stringOut) const
{
	m_baseTokenizer.Decode(inputTokens, stringOut);
}

void IntTensorToVec(torch::Tensor input, std::vector<int>* output)
{
	int		bufferPos = 0;
	size_t	inputSize = input.size(0);
	for(size_t i = 0; i < inputSize; ++i)
	{
		output->push_back(input[i].item().toInt());
	}
}

torch::Tensor NNTokenizer::EncodePaddedTensor(const std::string& inputString, int maxTokens, bool padToMax, bool frontPadSpace)
{
	std::string			processString = inputString;
	if( frontPadSpace )
	{
		//	make tokenization more consistent at the beginning of a prompt (e.g. "Punch" becomes " Punch" which can often be a different token)
		processString = " " + processString;
	}

	//	base tokenization
	std::vector<int>	tokenizedString;
	Encode(processString, &tokenizedString);

	//	tensorize
	const int			kMaxPromptTokens = maxTokens - 3;	//	-2 for start and end tokens, -1 in case we want to pad a space
	torch::Tensor		animTextDescTokenized = torch::from_blob(tokenizedString.data(), {(int)tokenizedString.size()}, torch::kInt).clone();
	if( animTextDescTokenized.size(0) > kMaxPromptTokens )
	{
		animTextDescTokenized = animTextDescTokenized.slice(0, 0, kMaxPromptTokens);	//	discard tokens past the limit
	}

	//	add start and end tokens
	int				startToken = GetStartToken();
	int				endToken = GetEndToken();

	torch::Tensor	startTokenTensor = torch::tensor({startToken}).to(torch::kInt64);
	torch::Tensor	endTokenTensor = torch::tensor({endToken}).to(torch::kInt64);

	animTextDescTokenized = torch::cat({startTokenTensor, animTextDescTokenized, endTokenTensor}, 0);

	//	pad to max
	if( padToMax )
	{
		while(animTextDescTokenized.size(0) < maxTokens)
		{
			animTextDescTokenized = torch::cat({animTextDescTokenized, endTokenTensor}, 0);
		}
	}

	//	done
	return animTextDescTokenized;
}

void NNTokenizer::DecodeTensor(torch::Tensor input, std::string* output) const
{
	std::vector<int>	inputTokens;
	IntTensorToVec(input, &inputTokens);
	g_NNTokenizer->Decode(inputTokens, output);
}

int NNTokenizer::GetStartToken() const
{
	return m_baseTokenizer.GetStartToken();
}

int NNTokenizer::GetEndToken() const
{
	return m_baseTokenizer.GetEndToken();
}

int NNTokenizer::GetVocabSize() const
{
	return m_baseTokenizer.GetVocabSize();
}