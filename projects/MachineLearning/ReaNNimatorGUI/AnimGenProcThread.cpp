#include "AnimGenProcThread.h"

#include "SharedMLHeader.h"

#include "Model.h"
#include "Continuous3DRotation6.h"
#include "MathNamespace.h"

#include "NNVAEModelComponent.h"
#include "NNVAE.h"

#include "NNTextEncoderComponent.h"
#include "NNTextEncoder.h"

#include "NNDDPMModelComponent.h"
#include "NNNoiseFinderModelComponent.h"
#include "NNNoiseFinder.h"

#include "NNTokenizerModelComponent.h"

#include "NNCLIPComponent.h"
#include "NNCLIP.h"

#include "AnimGenDescDictionary.h"
#include "AnimGenHyperParameters.h"

namespace
{
	enum
	{
		kPrintTrainingUpdateRate = 10,
		kValidationRate = 100
	};
}

AnimGenProcThread::AnimGenProcThread():
	m_pNNVAE2(nullptr),
	m_NNTextEncoder(nullptr),
	m_NNDDPMModelComponent(nullptr),
	m_NNNoiseFinderModelComponent(nullptr),
	m_NNTokenizerModelComponent(nullptr),
	m_NNCLIPComponent(nullptr),
	m_sampleCount(1),
	m_CFGAmount(0.3),
	m_genProgFunc(nullptr)
{
	m_batchSize = 16;
	m_optimizerNeedsUpdate = false;
	m_printTrainingRate = kPrintTrainingUpdateRate;

	m_createBatchThread.SetValidationRate(kValidationRate);

	{
		m_NNTokenizerModelComponent = new NNTokenizerModelComponent();
		m_NNTokenizerModelComponent->SetDevice(m_deviceToUse);
		m_evalModelComponents.push_back(m_NNTokenizerModelComponent);

		//m_NNTextEncoder = new NNTextEncoderComponent();
		//m_NNTextEncoder->SetDevice(m_deviceToUse);
		//m_evalModelComponents.push_back(m_NNTextEncoder);

		m_NNCLIPComponent = new NNCLIPComponent();
		m_NNCLIPComponent->SetDevice(m_deviceToUse);
		m_evalModelComponents.push_back(m_NNCLIPComponent);

		m_pNNVAE2 = new NNVAEModelComponent();
		m_pNNVAE2->SetDevice(m_deviceToUse);
		m_evalModelComponents.push_back(m_pNNVAE2);

		m_NNDDPMModelComponent = new NNDDPMModelComponent();
		m_NNDDPMModelComponent->SetDevice(m_deviceToUse);
		m_evalModelComponents.push_back(m_NNDDPMModelComponent);

		m_NNNoiseFinderModelComponent = new NNNoiseFinderModelComponent();
		m_NNNoiseFinderModelComponent->SetDevice(m_deviceToUse);
		m_evalModelComponents.push_back(m_NNNoiseFinderModelComponent);

		m_batchSize = 6;
	}
}

AnimGenProcThread::~AnimGenProcThread()
{
}

void AnimGenProcThread::Generate()
{
	torch::NoGradGuard			no_grad;

	NoiseFinderGenerate(m_sampleCount);

	//	log the result
	char	buffer[256];
	sprintf(buffer, "AnimGenProcThread::GenerateAnim : animation generation complete\n");
	m_logFunc(buffer);
}

void AnimGenProcThread::NoiseFinderGenerate(int sampleCount)
{
	//	prepare for generation
	m_generatedAnimIntermediate.ClearForGeneration();
	m_generatedAnimIntermediate.InitFromModelIntermediate(m_targetModelIntermediate);
	torch::Tensor	boneData = m_generatedAnimIntermediate.boneTree.ToTensor().unsqueeze(0).to(m_deviceToUse);

	torch::Tensor	encodedCondText;
	torch::Tensor	encodedUncondText;

	int				startToken = g_NNTokenizer->GetStartToken();
	int				endToken = g_NNTokenizer->GetEndToken();

	torch::Tensor	startTokenTensor = torch::tensor({startToken}).to(torch::kInt64);
	torch::Tensor	endTokenTensor = torch::tensor({endToken}).to(torch::kInt64);

	//	encode the text prompt
	{
		//std::string		testPrompt = "walking forward, turning 180 degrees, and walking back";
		//std::string		testPrompt = pTargetAnim->descString;
		//std::string		testPrompt = "performing a spinning kick";
		std::string		testPrompt = m_generationPrompt;		
		
		torch::Tensor	animTextDescTokenized = g_NNTokenizer->EncodePaddedTensor(testPrompt, NNCLIPImpl::kMaxTokens, true, true);

		animTextDescTokenized = animTextDescTokenized.unsqueeze(0).to(m_deviceToUse);

		//encodedCondText = g_NNTextEncoder->forward(animTextDescTokenized);
		encodedCondText = g_NNCLIP->forward(animTextDescTokenized);
	}

	//	encode the negative/unconditional prompt
	{
		std::string		testPrompt = "";

		torch::Tensor	animTextDescTokenized = g_NNTokenizer->EncodePaddedTensor(testPrompt, NNCLIPImpl::kMaxTokens, true, true);

		animTextDescTokenized = animTextDescTokenized.unsqueeze(0).to(m_deviceToUse);

		//encodedUncondText = g_NNTextEncoder->forward(animTextDescTokenized);
		encodedUncondText = g_NNCLIP->forward(animTextDescTokenized);
	}

	//	start with random noise anim desc
	torch::Tensor	latentAnimDesc = torch::randn({sampleCount, 16, 32, 32}, m_deviceToUse);

	//	pass it through the diffusion process
	{
		torch::Tensor	batchedEncodedCondText = encodedCondText.repeat({sampleCount, 1, 1});
		torch::Tensor	batchedEncodedUncondText = encodedUncondText.repeat({sampleCount, 1, 1});

		//	no CFG
		//latentAnimDesc = m_NNNoiseFinderModelComponent->SampleFromNoise(latentAnimDesc, batchedEncodedCondText, m_genProgFunc);

		//	listed as a good CFG constant in the paper
		//latentAnimDesc = m_NNNoiseFinderModelComponent->SampleFromNoiseWithCFG(latentAnimDesc, batchedEncodedCondText, batchedEncodedUncondText, 0.3, m_genProgFunc);

		//	was the CFG constant used in an example I saw
		//latentAnimDesc = m_NNNoiseFinderModelComponent->SampleFromNoiseWithCFG(latentAnimDesc, batchedEncodedCondText, batchedEncodedUncondText, 7.5, m_genProgFunc);

		//	testing
		//latentAnimDesc = m_NNNoiseFinderModelComponent->SampleFromNoiseWithCFG(latentAnimDesc, batchedEncodedCondText, batchedEncodedUncondText, 15.0, m_genProgFunc);

		//	"we clearly see that increasing classifier-free guidance strength has the expected effect of decreasing sample variety and increasing individual sample fidelity."
		latentAnimDesc = m_NNNoiseFinderModelComponent->SampleFromNoiseWithCFG(latentAnimDesc, batchedEncodedCondText, batchedEncodedUncondText, m_CFGAmount, m_genProgFunc);
	}

	//	decode latentAnimDesc into animation data
	{
		torch::Tensor	decodedData = g_NNVAE->DecoderForward(latentAnimDesc);

		m_lastGeneratedData = decodedData;
	}
}

void AnimGenProcThread::SetSampleCount(int sampleCount)
{
	m_sampleCount = sampleCount;
}

void AnimGenProcThread::SetGenProgressFunc(GenProgressFunc funcToUse)
{
	m_genProgFunc = funcToUse;
}

void AnimGenProcThread::UpdateTargetModel(const char* modelToUse)
{
	ModelRef	targetModelRef = Model::MakeRef(modelToUse);
	m_targetModelIntermediate.InitFromModelData(*targetModelRef->GetData());
}

void AnimGenProcThread::ExportGeneratedAnim(AnimationData* animData)
{
	m_generatedAnimIntermediate.ExportToAnimData(animData);
}

int AnimGenProcThread::ExportGeneratedAnims(AnimationData* animDataArrayOut, int animDataCount)
{
	if( !m_lastGeneratedData.defined() )
	{
		return 0;
	}

	int	generatedBatches = (int)m_lastGeneratedData.size(0);

	int	outputBatches = std::min(generatedBatches, animDataCount);

	for(int i = 0; i < outputBatches; ++i)
	{
		torch::Tensor	thisBatchOutput = m_lastGeneratedData[i];
		
		DecodedDataToAnim(thisBatchOutput, &m_generatedAnimIntermediate);

		m_generatedAnimIntermediate.ExportToAnimData(&animDataArrayOut[i]);
	}

	return outputBatches;
}

void AnimGenProcThread::DecodedDataToAnim(torch::Tensor decodedDataIn, AnimationIntermediateSeq* animOut)
{
	animOut->ClearForGeneration();
	animOut->InitFromModelIntermediate(m_targetModelIntermediate);

	//	this keyframe count is a hack; need a better way to determine this
	//int	goalAnimFrameCount = (int)(pTargetAnim->keyFrames.size());
	int	goalAnimFrameCount = AnimGenHyperParameters::kMaxKeyframeSequenceLength;
	//int		goalAnimFrameCount = std::min((int)(pTargetAnim->keyFrames.size()), (int)AnimGenHyperParameters::kMaxKeyframeSequenceLength);

	//	init keyframes
	for(int keyframeIdx = 0; keyframeIdx < goalAnimFrameCount; ++keyframeIdx)
	{
		AnimationIntermediateSeq::KeyFrame*	pNewKeyFrame = animOut->AddKeyFrame();

		//	test : copy translations to better eyeball how well things are working
		/*for(int boneIdx = 0; boneIdx < (int)animOut->keyFrames[0].boneRotations.size(); ++boneIdx)
		{
			animOut->keyFrames[keyframeIdx].boneTranslations[boneIdx] = pTargetAnim->keyFrames[keyframeIdx].boneTranslations[boneIdx];
		}*/
	}

	torch::Tensor	outputDataRot = decodedDataIn.view({decodedDataIn.size(0), decodedDataIn.size(1), Continuous3DRotation6::kRowCount, Continuous3DRotation6::kColumnCount});		
	outputDataRot = XYBasisToRotMtx33(outputDataRot);

	for(int boneIdx = 0; boneIdx < (int)animOut->keyFrames[0].boneRotations.size(); ++boneIdx)
	{
		for(int keyframeIdx = 0; keyframeIdx < (int)animOut->keyFrames.size(); ++keyframeIdx)
		{
			torch::Tensor	elementTensor = outputDataRot[boneIdx][keyframeIdx];
			Mtx33			elementRotMtx((coord_type*)elementTensor.to(torch::kCPU).data_ptr());
			Quat			elementRotQuat(elementRotMtx);

			animOut->keyFrames[keyframeIdx].boneRotations[boneIdx] = elementRotQuat;

			//animOut->keyFrames[keyframeIdx].worldRotations[boneIdx] = elementRotQuat;
		}
	}

	//animOut->CalcLocalRotationsFromWorldRotations();	

	//m_generatedAnimIntermediate.animImageEuler.Init(m_generatedAnimIntermediate);
	//InitDebugSprite((pTargetAnim->animImageEuler.GetLocalKeyframeImageForWholeAnim().slice(1, 0, 64) + M_PI) / (2 * M_PI), kSource);
	//InitDebugSprite((m_generatedAnimIntermediate.animImageEuler.GetLocalKeyframeImageForWholeAnim().slice(1, 0, 64) + M_PI) / (2 * M_PI), kTarget);
}