#include "AnimSeqDataSet.h"

#include "Model.h"
#include "Animation.h"
#include "MathNamespace.h"
#include "Sprite.h"

#include "AnimGenHyperParameters.h"
#include "AnimTrainingDataFiles.h"
#include "NNTokenizer.h"

AnimSeqDataSet* g_animSeqDataSet = nullptr;

AnimSeqDataSet::AnimSeqDataSet()
{
	Assert(g_animSeqDataSet == nullptr);
	g_animSeqDataSet = this;
}

AnimSeqDataSet::~AnimSeqDataSet()
{
	Assert(g_animSeqDataSet == this);
	g_animSeqDataSet = nullptr;
}

void AnimSeqDataSet::CreateData()
{
	torch::NoGradGuard	no_grad;

	AnimTrainingDataFiles	atdf;
	atdf.FindFiles();

	int		trainingModelCount = atdf.GetModelCount(AnimTrainingDataFiles::kFileType_Training);
	int		validationModelCount = atdf.GetModelCount(AnimTrainingDataFiles::kFileType_Validation);

	bool	boneDictLoaded = m_boneDictionary.TryToLoadFromFile();
	if( !boneDictLoaded )
	{
		m_boneDictionary.Init();
		for(int modelIdx = 0; modelIdx < trainingModelCount; modelIdx++)
		{
			//	add bones from this model
			m_boneDictionary.AddFromModel(atdf.GetModelPath(AnimTrainingDataFiles::kFileType_Training, modelIdx).c_str());
		}

		m_boneDictionary.SaveToFile();
	}

	//	init anim training data
	for(int modelIdx = 0; modelIdx < trainingModelCount; modelIdx++)
	{
		int			trainingModelAnimCount = atdf.GetAnimCount(AnimTrainingDataFiles::kFileType_Training, modelIdx);
		std::string	modelPath = atdf.GetModelPath(AnimTrainingDataFiles::kFileType_Training, modelIdx);
		for(int modelAnimIdx = 0; modelAnimIdx < trainingModelAnimCount; modelAnimIdx++)
		{
			std::string					modelAnimPath = atdf.GetAnimPath(AnimTrainingDataFiles::kFileType_Training, modelIdx, modelAnimIdx);
			std::string					modelAnimDescPath = atdf.GetAnimDescPath(AnimTrainingDataFiles::kFileType_Training, modelIdx, modelAnimIdx);
			std::string					modelAnimName = atdf.GetAnimName(AnimTrainingDataFiles::kFileType_Training, modelIdx, modelAnimIdx);

			ModelRef					thisSourceModelRef = Model::MakeRef(modelPath.c_str());
			AnimationRef				thisAnimRef = Animation::MakeRef(modelAnimPath.c_str());

			AnimationIntermediateSeq	thisAnimIntermediate;
			thisAnimIntermediate.Init(*thisSourceModelRef->GetData(), *thisAnimRef->GetData(), modelAnimPath.c_str());
			thisAnimIntermediate.SetDescFromFile(modelAnimDescPath.c_str());

			m_trainingAnims.push_back(thisAnimIntermediate);
		}
	}

	//	init anim validation data
	for(int modelIdx = 0; modelIdx < validationModelCount; modelIdx++)
	{
		int			modelAnimCount = atdf.GetAnimCount(AnimTrainingDataFiles::kFileType_Validation, modelIdx);
		std::string	modelPath = atdf.GetModelPath(AnimTrainingDataFiles::kFileType_Validation, modelIdx);
		for(int modelAnimIdx = 0; modelAnimIdx < modelAnimCount; modelAnimIdx++)
		{
			std::string					modelAnimPath = atdf.GetAnimPath(AnimTrainingDataFiles::kFileType_Validation, modelIdx, modelAnimIdx);
			std::string					modelAnimDescPath = atdf.GetAnimDescPath(AnimTrainingDataFiles::kFileType_Validation, modelIdx, modelAnimIdx);
			std::string					modelAnimName = atdf.GetAnimName(AnimTrainingDataFiles::kFileType_Validation, modelIdx, modelAnimIdx);

			ModelRef					thisSourceModelRef = Model::MakeRef(modelPath.c_str());
			AnimationRef				thisAnimRef = Animation::MakeRef(modelAnimPath.c_str());

			AnimationIntermediateSeq	thisAnimIntermediate;
			thisAnimIntermediate.Init(*thisSourceModelRef->GetData(), *thisAnimRef->GetData(), modelAnimPath.c_str());
			thisAnimIntermediate.SetDescFromFile(modelAnimDescPath.c_str());

			m_validationAnims.push_back(thisAnimIntermediate);
		}
	}

	{
		torch::Tensor	eulerMean;
		torch::Tensor	eulerStd;
		torch::Tensor	C3DR6Mean;
		torch::Tensor	C3DR6Std;
		for(int trainingAnimIdx = 0; trainingAnimIdx < (int)m_trainingAnims.size(); ++trainingAnimIdx)
		{
			torch::Tensor	thisEulerImage = m_trainingAnims[trainingAnimIdx].animImageEuler.GetLocalKeyframeImageForWholeAnim();
			torch::Tensor	thisC3DR6Image = m_trainingAnims[trainingAnimIdx].animImageC3DR6.GetLocalKeyframeImageForWholeAnim();

			if( eulerMean.defined() )
			{
				eulerMean += thisEulerImage.mean();
				eulerStd += thisEulerImage.std();

				C3DR6Mean += thisC3DR6Image.mean();
				C3DR6Std += thisC3DR6Image.std();
			}
			else
			{
				eulerMean = thisEulerImage.mean();
				eulerStd = thisEulerImage.std();

				C3DR6Mean = thisC3DR6Image.mean();
				C3DR6Std = thisC3DR6Image.std();
			}
		}

		eulerStd = eulerStd / (float)m_trainingAnims.size();
		C3DR6Std = C3DR6Std / (float)m_trainingAnims.size();

		DebugPrintf("full dataset : eulerMean = %f, eulerStd = %f, C3DR6Mean = %f, C3DR6Std = %f\n", eulerMean.item().toFloat(), eulerStd.item().toFloat(), C3DR6Mean.item().toFloat(), C3DR6Std.item().toFloat());
	}

	{
		CreateConcatAnimDescFile();
	}
}

void AnimSeqDataSet::CreateTextEncoderTrainingData()
{
	std::vector<int>	textEncoderTrainingTokens;

	const char*			textEncoderTrainingFile = "TextEncoderTraining.dat";
	bool				fileExists = g_fileManager->FileExists(textEncoderTrainingFile);

	if( fileExists )
	{
		FileRef<int>		fileRef = g_fileManager->MakeRef(textEncoderTrainingFile);
		const int*			fileData = (*fileRef);
		const size_t		fileSize = fileRef.FileSize();
		
		textEncoderTrainingTokens.resize(fileSize / sizeof(int));

		memcpy(textEncoderTrainingTokens.data(), fileData, textEncoderTrainingTokens.size() * sizeof(int));

		/*for(int i = 0; i < (int)textEncoderTrainingTokens.size(); ++i)
		{
			Assert(textEncoderTrainingTokens[i] >= 0 && textEncoderTrainingTokens[i] < g_NNTokenizer->GetVocabSize());
		}*/
	}
	else
	{
		DebugPrint("Tokenizing text encoder training data; might take a while.\n");

		{
			Assert(g_NNTokenizer != nullptr);

			FileRef<char>		fileRef = g_fileManager->MakeRef("OxfordEnglishDictionary.txt");
			//FileRef<char>		fileRef = g_fileManager->MakeRef("TinyShakespeare.txt");
			const char*			fileData = (*fileRef);
			const size_t		fileSize = fileRef.FileSize();

			char*				processData = new char[fileSize+1];
			strncpy(processData, fileData, fileSize);
			processData[fileSize] = 0;

			std::string			inputString = processData;

			g_NNTokenizer->Encode(inputString, &textEncoderTrainingTokens);

			/*for(int i = 0; i < (int)textEncoderTrainingTokens.size(); ++i)
			{
				Assert(textEncoderTrainingTokens[i] >= 0 && textEncoderTrainingTokens[i] < g_NNTokenizer->GetVocabSize());
			}*/

			//	save this out so we don't have to keep doing this while iterating
			{
				const char*	dataDir = g_fileManager->GetDataDir();
				char		fullFilePath[MAX_PATH];
				strcpy(fullFilePath, dataDir);
				strcat(fullFilePath, textEncoderTrainingFile);

				FILE*		pFile = fopen(fullFilePath, "wb");
				if( pFile == NULL )
				{
					AssertMsg(0, "error opening file %s for writing\n", fullFilePath);
					return;
				}

				size_t	elementsWritten = fwrite(textEncoderTrainingTokens.data(), sizeof(int), textEncoderTrainingTokens.size(), pFile);
				Assert(elementsWritten == textEncoderTrainingTokens.size());

				fclose(pFile);
			}
		}

		DebugPrint("Finished tokenizing text encoder training data\n");
	}

	m_fullText = torch::from_blob(textEncoderTrainingTokens.data(), {(int)textEncoderTrainingTokens.size()}, torch::kInt).clone().to(torch::kInt64);	//	needs to be int64 to work with embedding tables I think

	int					testPoint = int(m_fullText.numel() * 0.9f);	//	worked at 0.2 but failed at 0.3
	torch::Tensor		printFileTest = m_fullText.slice(0, testPoint, testPoint+128);
	std::string			printFileTestStr;
	g_NNTokenizer->DecodeTensor(printFileTest, &printFileTestStr);
	DebugPrintf("File test : %s\n", printFileTestStr.c_str());

	//	split the data into training and validation
	int					dataSplitPoint = int(m_fullText.numel() * 0.9f);	//	the first 90% will be training
	m_trainingText = m_fullText.slice(0, 0, dataSplitPoint);
	m_validationText = m_fullText.slice(0, dataSplitPoint, c10::nullopt);	
}

/*
	This is in case I want to tokenize based on the text used in the training anim descriptions
*/
void AnimSeqDataSet::CreateConcatAnimDescFile()
{
	const char*	kConcatDescFileName = "ConcatAnimDesc.txt";

	bool		fileExists = g_fileManager->FileExists(kConcatDescFileName);

	if( !fileExists )
	{
		Assert(m_trainingAnims.size() > 0);

		std::string	fileDataResult = m_trainingAnims[0].descString;
		for(int i = 1; i < (int)m_trainingAnims.size(); ++i)
		{
			fileDataResult = fileDataResult + "\n" + m_trainingAnims[i].descString;
		}

		//	save this out
		{
			const char*	dataDir = g_fileManager->GetDataDir();
			char		fullFilePath[MAX_PATH];
			strcpy(fullFilePath, dataDir);
			strcat(fullFilePath, kConcatDescFileName);

			FILE*		pFile = fopen(fullFilePath, "wb");
			if( pFile == NULL )
			{
				AssertMsg(0, "error opening file %s for writing\n", fullFilePath);
				return;
			}

			size_t	elementsWritten = fwrite(fileDataResult.data(), sizeof(char), fileDataResult.size(), pFile);
			Assert(elementsWritten == fileDataResult.size());

			fclose(pFile);
		}
	}
}

int AnimSeqDataSet::GetRandomTrainingAnimIdx() const
{
	//	pick a random animation in our dataset to train from
	return rand() % m_trainingAnims.size();
}

int AnimSeqDataSet::GetRandomValidationAnimIdx() const
{
	return rand() % m_validationAnims.size();
}

void AnimSeqDataSet::ToDevice(torch::Device device)
{
}