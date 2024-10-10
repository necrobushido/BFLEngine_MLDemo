#include "AnimTrainingDataFiles.h"

#include "FileManager.h"

namespace
{
	const char*			kTrainingDataDir = "TrainingData\\";
	const char*			kValidationDataDir = "ValidationData\\";
}

void AnimTrainingDataFiles::FindFilesForType(const char* dataDir, std::vector<ModelTrainingDataEntry>& targetVector)
{
	WIN32_FIND_DATAA	findFileData;
	HANDLE				hFind;

	//	subdirectories are model entries
	char	fileExpression[MAX_PATH];
	strcpy(fileExpression, dataDir);
	strcat(fileExpression, "*");
	hFind = FindFirstFileA(fileExpression, &findFileData);

	int	fileFound = hFind != INVALID_HANDLE_VALUE;
	while(fileFound)
	{
		if( (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			strcmp(findFileData.cFileName, ".") != 0 &&
			strcmp(findFileData.cFileName, "..") != 0 )
		{
			AddModelEntry(dataDir, findFileData.cFileName, targetVector);		
		}

		fileFound = FindNextFileA(hFind, &findFileData);
	}
}

void AnimTrainingDataFiles::FindFiles()
{
	char	fullDataDir[MAX_PATH];

	m_trainingModelEntries.clear();	
	strcpy(fullDataDir, g_fileManager->GetDataDir());
	strcat(fullDataDir, kTrainingDataDir);
	FindFilesForType(fullDataDir, m_trainingModelEntries);

	m_validationModelEntries.clear();
	strcpy(fullDataDir, g_fileManager->GetDataDir());
	strcat(fullDataDir, kValidationDataDir);
	FindFilesForType(fullDataDir, m_validationModelEntries);
}

void AnimTrainingDataFiles::AddModelEntry(const char* inputPath, const char* directoryName, std::vector<ModelTrainingDataEntry>& targetVector)
{
	WIN32_FIND_DATAA		findFileData;
	HANDLE					hFind;

	const char*				kModelExtension = "mdl";
	ModelTrainingDataEntry*	pModelEntry = nullptr;

	char					thisModelDir[MAX_PATH];
	strcpy(thisModelDir, inputPath);
	strcat(thisModelDir, directoryName);
	strcat(thisModelDir, "\\");

	//	this directory
	{
		char	fileExpression[MAX_PATH];
		strcpy(fileExpression, thisModelDir);
		strcat(fileExpression, "*.");
		strcat(fileExpression, kModelExtension);

		hFind = FindFirstFileA(fileExpression, &findFileData);

		//	just grab the first model file we find.  there should only be one
		int	fileFound = hFind != INVALID_HANDLE_VALUE;
		if( fileFound )
		{
			ModelTrainingDataEntry	thisModelEntry;
			thisModelEntry.m_directory = directoryName;
			thisModelEntry.m_sourceModel = findFileData.cFileName;

			targetVector.push_back(thisModelEntry);

			pModelEntry = &targetVector[targetVector.size()-1];
		}
	}

	//	subdirectories are animation entries
	if( pModelEntry != nullptr )
	{
		char	fileExpression[MAX_PATH];
		strcpy(fileExpression, thisModelDir);
		strcat(fileExpression, "*");
		hFind = FindFirstFileA(fileExpression, &findFileData);

		int	fileFound = hFind != INVALID_HANDLE_VALUE;
		while(fileFound)
		{
			if( (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
				strcmp(findFileData.cFileName, ".") != 0 &&
				strcmp(findFileData.cFileName, "..") != 0 )
			{
				//	found a subdirectory; recurse into it
				AddAnimEntry(inputPath, directoryName, findFileData.cFileName, pModelEntry);
			}

			fileFound = FindNextFileA(hFind, &findFileData);
		}
	}
}

void AnimTrainingDataFiles::AddAnimEntry(const char* inputPath, const char* parentDirectoryName, const char* directoryName, ModelTrainingDataEntry* parentModelEntry)
{
	WIN32_FIND_DATAA	findFileData;
	HANDLE				hFind;

	const char*			kAnimExtension = "anm";
	const char*			kDescName = "Desc.txt";

	//	this directory
	{
		char	fileExpression[MAX_PATH];
		strcpy(fileExpression, inputPath);
		strcat(fileExpression, parentDirectoryName);
		strcat(fileExpression, "\\");
		strcat(fileExpression, directoryName);
		strcat(fileExpression, "\\");
		strcat(fileExpression, "*.");
		strcat(fileExpression, kAnimExtension);

		hFind = FindFirstFileA(fileExpression, &findFileData);

		//	just grab the first anim file we find.  there should only be one
		int	fileFound = hFind != INVALID_HANDLE_VALUE;
		if( fileFound )
		{
			AnimTrainingDataEntry	thisAnimEntry;
			thisAnimEntry.m_directory = directoryName;
			thisAnimEntry.m_animationFile = findFileData.cFileName;
			thisAnimEntry.m_descFile = kDescName;

			parentModelEntry->m_animationEntries.push_back(thisAnimEntry);
		}
	}
}

int AnimTrainingDataFiles::GetModelCount(eFileType animType) const
{
	switch(animType)
	{
	default:
	case kFileType_Training:
		break;

	case kFileType_Validation:
		return ValidationModelCount();
	}

	//	case kFileType_Training
	return TrainingModelCount();
}

int AnimTrainingDataFiles::GetAnimCount(eFileType animType, int modelIdx) const
{
	switch(animType)
	{
	default:
	case kFileType_Training:
		break;

	case kFileType_Validation:
		return ValidationModelAnimCount(modelIdx);
	}

	//	case kFileType_Training
	return TrainingModelAnimCount(modelIdx);
}

std::string AnimTrainingDataFiles::GetModelPath(eFileType animType, int modelIdx) const
{
	switch(animType)
	{
	default:
	case kFileType_Training:
		break;

	case kFileType_Validation:
		return GetValidationModelPath(modelIdx);
	}

	//	case kFileType_Training
	return GetTrainingModelPath(modelIdx);
}

std::string AnimTrainingDataFiles::GetAnimPath(eFileType animType, int modelIdx, int animIdx) const
{
	switch(animType)
	{
	default:
	case kFileType_Training:
		break;

	case kFileType_Validation:
		return GetValidationAnimPath(modelIdx, animIdx);
	}

	//	case kFileType_Training
	return GetTrainingAnimPath(modelIdx, animIdx);
}

std::string AnimTrainingDataFiles::GetAnimDescPath(eFileType animType, int modelIdx, int animIdx) const
{
	switch(animType)
	{
	default:
	case kFileType_Training:
		break;

	case kFileType_Validation:
		return GetValidationAnimDescPath(modelIdx, animIdx);
	}

	//	case kFileType_Training
	return GetTrainingAnimDescPath(modelIdx, animIdx);
}

std::string AnimTrainingDataFiles::GetModelName(eFileType animType, int modelIdx) const
{
	switch(animType)
	{
	default:
	case kFileType_Training:
		break;

	case kFileType_Validation:
		return GetValidationModelName(modelIdx);
	}

	//	case kFileType_Training
	return GetTrainingModelName(modelIdx);
}

std::string AnimTrainingDataFiles::GetAnimName(eFileType animType, int modelIdx, int animIdx) const
{
	switch(animType)
	{
	default:
	case kFileType_Training:
		break;

	case kFileType_Validation:
		return GetValidationAnimName(modelIdx, animIdx);
	}

	//	case kFileType_Training
	return GetTrainingAnimName(modelIdx, animIdx);
}

int AnimTrainingDataFiles::TrainingModelCount() const
{
	return (int)m_trainingModelEntries.size();
}

int AnimTrainingDataFiles::TrainingModelAnimCount(int modelIdx) const
{
	Assert(modelIdx < TrainingModelCount());
	return (int)m_trainingModelEntries[modelIdx].m_animationEntries.size();
}

std::string AnimTrainingDataFiles::GetTrainingModelPath(int modelIdx) const
{
	Assert(modelIdx < TrainingModelCount());
	std::string	result = kTrainingDataDir;
	result = result + m_trainingModelEntries[modelIdx].m_directory;
	result = result + "\\";
	result = result + m_trainingModelEntries[modelIdx].m_sourceModel;

	return result;
}

std::string AnimTrainingDataFiles::GetTrainingAnimPath(int modelIdx, int animIdx) const
{
	Assert(modelIdx < TrainingModelCount());
	Assert(animIdx < TrainingModelAnimCount(modelIdx));

	std::string	result = kTrainingDataDir;
	result = result + m_trainingModelEntries[modelIdx].m_directory;
	result = result + "\\";
	result = result + m_trainingModelEntries[modelIdx].m_animationEntries[animIdx].m_directory;
	result = result + "\\";
	result = result + m_trainingModelEntries[modelIdx].m_animationEntries[animIdx].m_animationFile;

	return result;
}

std::string AnimTrainingDataFiles::GetTrainingAnimDescPath(int modelIdx, int animIdx) const
{
	Assert(modelIdx < TrainingModelCount());
	Assert(animIdx < TrainingModelAnimCount(modelIdx));

	std::string	result = kTrainingDataDir;
	result = result + m_trainingModelEntries[modelIdx].m_directory;
	result = result + "\\";
	result = result + m_trainingModelEntries[modelIdx].m_animationEntries[animIdx].m_directory;
	result = result + "\\";
	result = result + m_trainingModelEntries[modelIdx].m_animationEntries[animIdx].m_descFile;

	return result;
}

std::string AnimTrainingDataFiles::GetTrainingModelName(int modelIdx) const
{
	Assert(modelIdx < TrainingModelCount());
	return m_trainingModelEntries[modelIdx].m_sourceModel;
}

std::string AnimTrainingDataFiles::GetTrainingAnimName(int modelIdx, int animIdx) const
{
	Assert(modelIdx < TrainingModelCount());
	Assert(animIdx < TrainingModelAnimCount(modelIdx));
	return m_trainingModelEntries[modelIdx].m_animationEntries[animIdx].m_animationFile;
}

int AnimTrainingDataFiles::ValidationModelCount() const
{
	return (int)m_validationModelEntries.size();
}

int AnimTrainingDataFiles::ValidationModelAnimCount(int modelIdx) const
{
	Assert(modelIdx < ValidationModelCount());
	return (int)m_validationModelEntries[modelIdx].m_animationEntries.size();
}

std::string AnimTrainingDataFiles::GetValidationModelPath(int modelIdx) const
{
	Assert(modelIdx < ValidationModelCount());
	std::string	result = kValidationDataDir;
	result = result + m_validationModelEntries[modelIdx].m_directory;
	result = result + "\\";
	result = result + m_validationModelEntries[modelIdx].m_sourceModel;

	return result;
}

std::string AnimTrainingDataFiles::GetValidationAnimPath(int modelIdx, int animIdx) const
{
	Assert(modelIdx < ValidationModelCount());
	Assert(animIdx < ValidationModelAnimCount(modelIdx));

	std::string	result = kValidationDataDir;
	result = result + m_validationModelEntries[modelIdx].m_directory;
	result = result + "\\";
	result = result + m_validationModelEntries[modelIdx].m_animationEntries[animIdx].m_directory;
	result = result + "\\";
	result = result + m_validationModelEntries[modelIdx].m_animationEntries[animIdx].m_animationFile;

	return result;
}

std::string AnimTrainingDataFiles::GetValidationAnimDescPath(int modelIdx, int animIdx) const
{
	Assert(modelIdx < ValidationModelCount());
	Assert(animIdx < ValidationModelAnimCount(modelIdx));

	std::string	result = kValidationDataDir;
	result = result + m_validationModelEntries[modelIdx].m_directory;
	result = result + "\\";
	result = result + m_validationModelEntries[modelIdx].m_animationEntries[animIdx].m_directory;
	result = result + "\\";
	result = result + m_validationModelEntries[modelIdx].m_animationEntries[animIdx].m_descFile;

	return result;
}

std::string AnimTrainingDataFiles::GetValidationModelName(int modelIdx) const
{
	Assert(modelIdx < ValidationModelCount());
	return m_validationModelEntries[modelIdx].m_sourceModel;
}

std::string AnimTrainingDataFiles::GetValidationAnimName(int modelIdx, int animIdx) const
{
	Assert(modelIdx < ValidationModelCount());
	Assert(animIdx < ValidationModelAnimCount(modelIdx));
	return m_validationModelEntries[modelIdx].m_animationEntries[animIdx].m_animationFile;
}