#pragma once

#include "types.h"

class AnimTrainingDataEntry
{
public:
	std::string	m_directory;
	std::string	m_animationFile;
	std::string	m_descFile;
};

class ModelTrainingDataEntry
{
public:
	std::string							m_directory;
	std::string							m_sourceModel;
	std::vector<AnimTrainingDataEntry>	m_animationEntries;
};

class AnimTrainingDataFiles
{
public:
	enum eFileType
	{
		kFileType_Training,
		kFileType_Validation,

		kFileType_Count
	};

public:
	void FindFiles();

	//
	int GetModelCount(eFileType animType) const;
	int GetAnimCount(eFileType animType, int modelIdx) const;

	std::string GetModelPath(eFileType animType, int modelIdx) const;
	std::string GetAnimPath(eFileType animType, int modelIdx, int animIdx) const;
	std::string GetAnimDescPath(eFileType animType, int modelIdx, int animIdx) const;

	std::string GetModelName(eFileType animType, int modelIdx) const;
	std::string GetAnimName(eFileType animType, int modelIdx, int animIdx) const;

protected:
	//
	int TrainingModelCount() const;
	int TrainingModelAnimCount(int modelIdx) const;

	std::string GetTrainingModelPath(int modelIdx) const;
	std::string GetTrainingAnimPath(int modelIdx, int animIdx) const;
	std::string GetTrainingAnimDescPath(int modelIdx, int animIdx) const;

	std::string GetTrainingModelName(int modelIdx) const;
	std::string GetTrainingAnimName(int modelIdx, int animIdx) const;

	//
	int ValidationModelCount() const;
	int ValidationModelAnimCount(int modelIdx) const;

	std::string GetValidationModelPath(int modelIdx) const;
	std::string GetValidationAnimPath(int modelIdx, int animIdx) const;
	std::string GetValidationAnimDescPath(int modelIdx, int animIdx) const;

	std::string GetValidationModelName(int modelIdx) const;
	std::string GetValidationAnimName(int modelIdx, int animIdx) const;

protected:
	void FindFilesForType(const char* dataDir, std::vector<ModelTrainingDataEntry>& targetVector);
	void AddModelEntry(const char* inputPath, const char* directoryName, std::vector<ModelTrainingDataEntry>& targetVector);
	void AddAnimEntry(const char* inputPath, const char* parentDirectoryName, const char* directoryName, ModelTrainingDataEntry* parentModelEntry);

protected:
	std::vector<ModelTrainingDataEntry>	m_trainingModelEntries;
	std::vector<ModelTrainingDataEntry>	m_validationModelEntries;
};
