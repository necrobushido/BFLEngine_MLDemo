#include "NNModelComponent.h"

#include "FileManager.h"

NNModelComponent::NNModelComponentList	NNModelComponent::s_componentList;

NNModelComponent::NNModelComponent()
{
	LIST_ADD(&s_componentList, AllComponents, this);
}

NNModelComponent::~NNModelComponent()
{
	LIST_REMOVE(&s_componentList, AllComponents, this);
}

void NNModelComponent::SaveModule(torch::nn::Module* moduleToSave, const char* fileName)
{
	torch::NoGradGuard	no_grad;

	//	we could maybe override this functionality to use our filemanager directly somehow

	const char*	dataDir = g_fileManager->GetDataDir();
	const char*	savedFilename = fileName;
	char		filepathBuffer[256];
	filepathBuffer[0] = '\0';
	strcat(filepathBuffer, dataDir);
	strcat(filepathBuffer, savedFilename);

	moduleToSave->eval();

	torch::serialize::OutputArchive	outputModelArchive;
	moduleToSave->save(outputModelArchive);
	outputModelArchive.save_to(filepathBuffer);
}

void NNModelComponent::LoadModule(torch::nn::Module* moduleToLoad, const char* fileName)
{
	torch::NoGradGuard	no_grad;

	const char*	dataDir = g_fileManager->GetDataDir();
	const char*	savedFilename = fileName;
	char		filepathBuffer[256];
	filepathBuffer[0] = '\0';
	strcat(filepathBuffer, dataDir);
	strcat(filepathBuffer, savedFilename);

	if( g_fileManager->AbsoluteFileExists(filepathBuffer) )
	{
		torch::serialize::InputArchive	inputArchive;
		inputArchive.load_from(filepathBuffer);
		moduleToLoad->load(inputArchive);

		moduleToLoad->eval();

		DebugPrintf("LoadModule : file %s finished\n", filepathBuffer);
	}
	else
	{
		DebugPrintf("LoadModule : file %s not found\n", filepathBuffer);
	}
}