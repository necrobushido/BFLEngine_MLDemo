#pragma once

#include "types.h"

class BoneTree;

class AnimGenBoneDictionary
{
public:
	AnimGenBoneDictionary();
	~AnimGenBoneDictionary();

	int GetVocabSize();
	int GetMappedBoneIdx(const std::string& boneName);
	int GetInvalidBoneIdx();
	std::string GetInvalidBoneName();
	std::string GetBoneName(int boneIdx);

	int ConvertReferenceBoneToDictionaryBone(int referenceBoneIdx, const BoneTree& referenceBoneTree);

	void Init();
	void AddFromAnim(const char* animFile);
	void AddFromModel(const char* modelFile);

	bool TryToLoadFromFile();
	void SaveToFile();
	void SetFromFile(const char* descFilePath);

protected:
	bool IsBoneInDictionary(const std::string& boneName) const;
	void InitMapFromDict();

private:
	std::vector<std::string>	m_boneNameDictionary;
	std::map<std::string, int>	m_boneNameToIdxMap;
};

extern AnimGenBoneDictionary* g_animGenBoneDictionary;
