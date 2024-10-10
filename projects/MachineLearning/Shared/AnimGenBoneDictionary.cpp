#include "AnimGenBoneDictionary.h"

#include "Animation.h"
#include "Model.h"
#include "BoneTree.h"

namespace
{
	const std::string	kInvalidBoneName = "";

	const char*			kSavedDictName = "AnimGenBoneDictionary.dat";
}

AnimGenBoneDictionary*	g_animGenBoneDictionary = nullptr;

AnimGenBoneDictionary::AnimGenBoneDictionary()
{
	Assert(g_animGenBoneDictionary == nullptr);
	g_animGenBoneDictionary = this;
}

AnimGenBoneDictionary::~AnimGenBoneDictionary()
{
	Assert(g_animGenBoneDictionary == this);
	g_animGenBoneDictionary = nullptr;
}

bool AnimGenBoneDictionary::IsBoneInDictionary(const std::string& boneName) const
{
	return m_boneNameToIdxMap.find(boneName) != m_boneNameToIdxMap.end();
}

void AnimGenBoneDictionary::Init()
{
	m_boneNameDictionary.push_back(kInvalidBoneName);
	m_boneNameToIdxMap[kInvalidBoneName] = 0;
}

void AnimGenBoneDictionary::AddFromAnim(const char* animFile)
{
	AnimationRef			thisAnimRef = Animation::MakeRef(animFile);
	const AnimationData*	thisAnimData = thisAnimRef->GetData();

	for(u32 boneIdx = 0; boneIdx < thisAnimData->numBones; ++boneIdx)
	{
		std::string	thisBoneName = thisAnimData->bones[boneIdx].name;

		if( !IsBoneInDictionary(thisBoneName) )
		{
			int	currentIdx = (int)m_boneNameDictionary.size();

			m_boneNameDictionary.push_back(thisBoneName);
			m_boneNameToIdxMap[thisBoneName] = currentIdx;
		}
	}
}

void AnimGenBoneDictionary::AddFromModel(const char* modelFile)
{
	ModelRef			thisModelRef = Model::MakeRef(modelFile);
	const ModelData*	thisModelData = thisModelRef->GetData();

	for(u32 boneIdx = 0; boneIdx < thisModelData->boneCount; ++boneIdx)
	{
		std::string	thisBoneName = thisModelData->bones[boneIdx].name;

		if( !IsBoneInDictionary(thisBoneName) )
		{
			int	currentIdx = (int)m_boneNameDictionary.size();

			m_boneNameDictionary.push_back(thisBoneName);
			m_boneNameToIdxMap[thisBoneName] = currentIdx;
		}
	}
}

int AnimGenBoneDictionary::GetVocabSize()
{
	return (int)m_boneNameDictionary.size();
}

int AnimGenBoneDictionary::GetMappedBoneIdx(const std::string& boneName)
{
	return m_boneNameToIdxMap[boneName];
}

int AnimGenBoneDictionary::GetInvalidBoneIdx()
{
	return m_boneNameToIdxMap[kInvalidBoneName];
}

std::string AnimGenBoneDictionary::GetInvalidBoneName()
{
	return kInvalidBoneName;
}

std::string AnimGenBoneDictionary::GetBoneName(int boneIdx)
{
	return m_boneNameDictionary[boneIdx];
}

int AnimGenBoneDictionary::ConvertReferenceBoneToDictionaryBone(int referenceBoneIdx, const BoneTree& referenceBoneTree)
{
	int	result = GetInvalidBoneIdx();
	if( referenceBoneIdx >= 0 )
	{
		result = g_animGenBoneDictionary->GetMappedBoneIdx(referenceBoneTree.m_nodes[referenceBoneIdx].m_boneData.name);
	}

	return result;
}

bool AnimGenBoneDictionary::TryToLoadFromFile()
{
	bool	fileExists = g_fileManager->FileExists(kSavedDictName);
	if( fileExists )
	{
		SetFromFile(kSavedDictName);
	}

	return fileExists;
}

void AnimGenBoneDictionary::SaveToFile()
{
	const char*	dataDir = g_fileManager->GetDataDir();

	char		fullFilePath[MAX_PATH];
	strcpy(fullFilePath, dataDir);
	strcat(fullFilePath, kSavedDictName);

	FILE*		pFile = fopen(fullFilePath, "wb");
	if( pFile == NULL )
	{
		AssertMsg(0, "error opening file %s for writing\n", fullFilePath);
	}

	const char*	endLine = "\n";
	const int	endLineLen = (int)strlen(endLine);

	//	start at 1 to skip the invalid desc
	for(int i = 1; i < (int)m_boneNameDictionary.size(); ++i)
	{
		std::string	thisString = m_boneNameDictionary[i];
		size_t		bytesWritten = fwrite(thisString.data(), 1, thisString.length(), pFile);
		Assert(bytesWritten == thisString.length());

		bytesWritten = fwrite(endLine, 1, endLineLen, pFile);
		Assert(bytesWritten == endLineLen);
	}

	fclose(pFile);
}

void AnimGenBoneDictionary::SetFromFile(const char* descFilePath)
{
	m_boneNameDictionary.clear();
	m_boneNameToIdxMap.clear();

	m_boneNameDictionary.push_back(kInvalidBoneName);

	FileRef<char>		descFileRef = g_fileManager->MakeRef(descFilePath);
	const char*			descFileData = (*descFileRef);
	const size_t		descFileSize = descFileRef.FileSize();

	//	strtok modifies the input, so we need to copy over first
	char*				descProcessData = new char[descFileSize+1];
	memset(descProcessData, 0, sizeof(char) * descFileSize+1);
	strncpy(descProcessData, descFileData, descFileSize);

	//	tokenize the input with endline as the delimiter
	const char*	tokenDelimiter = "\n\r";
	char*		token = strtok(descProcessData, tokenDelimiter);

    // Iterate through tokens
    while(token != nullptr) 
	{
		std::string	thisTag = token;

		m_boneNameDictionary.push_back(thisTag);

        token = strtok(nullptr, tokenDelimiter);
    }

	delete [] descProcessData;

	InitMapFromDict();
}

void AnimGenBoneDictionary::InitMapFromDict()
{
	for(int i = 0; i < (int)m_boneNameDictionary.size(); ++i)
	{
		m_boneNameToIdxMap[m_boneNameDictionary[i]] = i;
	}
}