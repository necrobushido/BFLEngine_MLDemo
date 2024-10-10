#include "AnimGenSeqProcThread.h"

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

namespace
{
	enum
	{
		kPrintTrainingUpdateRate = 10,
		kBatchesPerValidation = 100
	};

	enum
	{
		kTrainingVAE,
		kTrainingNoiseFinder,
		kTrainingTextEncoder,
		kTrainingCLIP,

		kFullEval,
		kNumModes
	};
}

void TorchLoggerFunc(const std::string& logString)
{
	DebugPrintf("TORCH LOG : %s\n", logString.c_str());
}

AnimGenSeqProcessingThread::AnimGenSeqProcessingThread():
	m_pNNVAE2(nullptr),
	m_NNTextEncoder(nullptr),
	m_NNDDPMModelComponent(nullptr),
	m_NNNoiseFinderModelComponent(nullptr),
	m_NNTokenizerModelComponent(nullptr),
	m_NNCLIPComponent(nullptr),
	m_mode(0),
	m_sampleCount(1)
{
	m_batchSize = 16;
	m_optimizerNeedsUpdate = false;
	m_printTrainingRate = kPrintTrainingUpdateRate;

	m_createBatchThread.SetValidationRate(kBatchesPerValidation);

	m_dataSet.CreateData();

	//m_mode = kTrainingVAE;
	//m_mode = kTrainingTextEncoder;
	m_mode = kTrainingNoiseFinder;
	//m_mode = kTrainingCLIP;
	//m_mode = kFullEval;

	torch::SetAPIUsageLogger(TorchLoggerFunc);
	//at::autocast::set_autocast_gpu_dtype(at::kBFloat16);

	//at::Context&	torchGlobalContext = at::globalContext();
	//torchGlobalContext.setSDPUseMath(false);
	//torchGlobalContext.setSDPUseFlash(true);

	//	evidently this doesn't affect convolutions, so it is probably of limited use
	//		dunno though, the noisefinder does have attention layers
	//at::Context&	torchGlobalContext = at::globalContext();
	//torchGlobalContext.setFloat32MatmulPrecision(at::Float32MatmulPrecision::HIGHEST);	//	standard float32
	//torchGlobalContext.setFloat32MatmulPrecision(at::Float32MatmulPrecision::HIGH);	//	float32 in TensorFloat32 format
	//torchGlobalContext.setFloat32MatmulPrecision(at::Float32MatmulPrecision::MEDIUM);	//	float32 in bfloat16	

	switch(m_mode)
	{
	case kTrainingVAE:
		{
			m_pNNVAE2 = new NNVAEModelComponent();
			m_pNNVAE2->SetDevice(m_deviceToUse);
			m_trainModelComponents.push_back(m_pNNVAE2);

			m_batchSize = 8;
		}
		break;

	case kTrainingNoiseFinder:
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
			m_trainModelComponents.push_back(m_NNNoiseFinderModelComponent);

			m_batchSize = m_NNNoiseFinderModelComponent->SubBatchSize();

			//int		subBatchesPerBatch = m_NNNoiseFinderModelComponent->SubBatchesPerBatch();
			//m_createBatchThread.SetValidationRate(kBatchesPerValidation * subBatchesPerBatch);	//	temp fix for the sub-batching
			m_createBatchThread.SetValidationRate(kBatchesPerValidation * 16);
		}
		break;

	case kTrainingTextEncoder:
		{
			m_NNTokenizerModelComponent = new NNTokenizerModelComponent();
			m_NNTokenizerModelComponent->SetDevice(m_deviceToUse);
			m_evalModelComponents.push_back(m_NNTokenizerModelComponent);

			m_NNTextEncoder = new NNTextEncoderComponent();
			m_NNTextEncoder->SetDevice(m_deviceToUse);
			m_trainModelComponents.push_back(m_NNTextEncoder);

			m_batchSize = 64;
		}
		break;

	case kTrainingCLIP:
		{
			m_NNTokenizerModelComponent = new NNTokenizerModelComponent();
			m_NNTokenizerModelComponent->SetDevice(m_deviceToUse);
			m_evalModelComponents.push_back(m_NNTokenizerModelComponent);

			m_pNNVAE2 = new NNVAEModelComponent();
			m_pNNVAE2->SetDevice(m_deviceToUse);
			m_evalModelComponents.push_back(m_pNNVAE2);

			m_NNCLIPComponent = new NNCLIPComponent();
			m_NNCLIPComponent->SetDevice(m_deviceToUse);
			m_trainModelComponents.push_back(m_NNCLIPComponent);

			m_batchSize = m_NNCLIPComponent->SubBatchSize();

			m_createBatchThread.SetValidationRate(kBatchesPerValidation * m_NNCLIPComponent->SubBatchesPerBatch());
		}
		break;

	case kFullEval:
		{
			m_NNTokenizerModelComponent = new NNTokenizerModelComponent();
			m_NNTokenizerModelComponent->SetDevice(m_deviceToUse);
			m_evalModelComponents.push_back(m_NNTokenizerModelComponent);

			m_NNTextEncoder = new NNTextEncoderComponent();
			m_NNTextEncoder->SetDevice(m_deviceToUse);
			m_evalModelComponents.push_back(m_NNTextEncoder);

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
		break;

	default:
		Assert(false);
		break;
	}
}

AnimGenSeqProcessingThread::~AnimGenSeqProcessingThread()
{
}

//	should prob move this somewhere else
extern void InitSourceSprite(u32* imageData, u32 imageDataSize, u32 width, u32 height, u32 bpp);
extern void InitTargetSprite(u32* imageData, u32 imageDataSize, u32 width, u32 height, u32 bpp);
extern void InitSupplementSprite(u32* imageData, u32 imageDataSize, u32 width, u32 height, u32 bpp);
enum eSpriteType
{
	kSource,
	kTarget,
	kSupplement
};
void InitDebugSprite(torch::Tensor imageTensor, eSpriteType spriteType)	//	was "bool sourceSprite"
{
	//	input expected : {height, width, 3}

	/*int				height = (int)imageTensor.size(0);
	int				width = (int)imageTensor.size(1);
	int				imageSize = height*width;
		
	u32*			tempImageData = new u32[imageSize];
	for(int y = 0; y < height; ++y)
	{
		torch::Tensor	thisRow = imageTensor[y];

		for(int x = 0; x < width; ++x)
		{
			u32	pixelValue = 0;
			for(int i = 0; i < 3; ++i)
			{
				pixelValue += ((u32)std::clamp((thisRow[x][i].item().toFloat() * 255.0f), 0.0f, 255.0f)) << (i * 8);
			}
			pixelValue += 0xff << 24;

			tempImageData[(height - y - 1) * width + x] = pixelValue;
		}
	}

	switch(spriteType)
	{
	case kSource:
		InitSourceSprite(tempImageData, imageSize, width, height, 32);
		break;

	case kTarget:
		InitTargetSprite(tempImageData, imageSize, width, height, 32);
		break;

	case kSupplement:
		InitSupplementSprite(tempImageData, imageSize, width, height, 32);
		break;
	}

	delete [] tempImageData;*/
}

void AnimGenSeqProcessingThread::Generate()
{
	torch::NoGradGuard			no_grad;

	switch(m_mode)
	{
	case kTrainingVAE:
		{
			VAEGenerate();
		}
		break;

	case kTrainingNoiseFinder:
		{
			NoiseFinderGenerate(m_sampleCount);			
		}
		break;

	case kTrainingTextEncoder:
		{
			TextEncoderGenerate();
		}
		break;

	case kTrainingCLIP:
		{
			m_lastGeneratedData = torch::tensor({});

			//	how do I test this?
		}
		break;

	default:
		Assert(0);
		break;
	}

	//	log the result
	char	buffer[256];
	sprintf(buffer, "AnimGenSeqProcessingThread::GenerateAnim : animation generation complete\n");
	m_logFunc(buffer);
}

void AnimGenSeqProcessingThread::VAEGenerate()
{
	//	this should not be used in the long run, but using it for test info right now
	//int							targetAnimIdx = 0;
	int							targetAnimIdx = g_animSeqDataSet->GetRandomValidationAnimIdx();
	AnimationIntermediateSeq*	pTargetAnim = &g_animSeqDataSet->m_validationAnims[targetAnimIdx];
	{
		//	log the result
		char	tempBuffer[256];
		sprintf(tempBuffer, "AnimGenSeqProcessingThread::GenerateAnim : target anim = %s\n", pTargetAnim->descString.c_str());
		m_logFunc(tempBuffer);
	}

	//	start with a random anim desc
	torch::Tensor	latentAnimDesc;
	torch::Tensor	latentAnimDescMean;
	torch::Tensor	latentAnimDescLogVar;

	{
		//	grab some specific animation's latent desc

		//	C3DR6
		latentAnimDesc = g_NNVAE->EncoderForward(pTargetAnim->animImageC3DR6.GetLocalKeyframeImageForWholeAnim().unsqueeze(0).to(m_deviceToUse), &latentAnimDescLogVar, &latentAnimDescMean);

		//	Euler
		//latentAnimDesc = g_NNVAE->EncoderForward(pTargetAnim->animImageEuler.GetLocalKeyframeImageForWholeAnim().unsqueeze(0).to(m_deviceToUse), &latentAnimDescLogVar, &latentAnimDescMean);
	}

	//	decode latentAnimDesc into the animation
	{
		torch::Tensor	decodedData = g_NNVAE->DecoderForward(latentAnimDesc);

		m_lastGeneratedData = decodedData;
	}
}

void AnimGenSeqProcessingThread::NoiseFinderGenerate(int sampleCount)
{
	//	this should not be used in the long run, but using it for test info right now
	//int							targetAnimIdx = 0;
	int							targetAnimIdx = g_animSeqDataSet->GetRandomValidationAnimIdx();
	AnimationIntermediateSeq*	pTargetAnim = &g_animSeqDataSet->m_validationAnims[targetAnimIdx];

	{
		//	log the result
		char	tempBuffer[256];
		sprintf(tempBuffer, "AnimGenSeqProcessingThread::GenerateAnim : target anim = %s\n", pTargetAnim->descString.c_str());
		m_logFunc(tempBuffer);
	}

	torch::Tensor	encodedCondText;
	torch::Tensor	encodedUncondText;

	int				startToken = g_NNTokenizer->GetStartToken();
	int				endToken = g_NNTokenizer->GetEndToken();

	torch::Tensor	startTokenTensor = torch::tensor({startToken}).to(torch::kInt64);
	torch::Tensor	endTokenTensor = torch::tensor({endToken}).to(torch::kInt64);

	//	encode the text prompt
	{
		//std::string		testPrompt = "walking forward, turning 180 degrees, and walking back";
		std::string		testPrompt = pTargetAnim->descString;
		//std::string		testPrompt = "performing a spinning kick";
		//std::string		testPrompt = "throwing a left jab punch";

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
		//latentAnimDesc = m_NNNoiseFinderModelComponent->SampleFromNoise(latentAnimDesc, batchedEncodedCondText);

		//	listed as a good CFG constant in the paper
		//latentAnimDesc = m_NNNoiseFinderModelComponent->SampleFromNoiseWithCFG(latentAnimDesc, batchedEncodedCondText, batchedEncodedUncondText, 0.3);

		//	was the CFG constant used in an example I saw
		latentAnimDesc = m_NNNoiseFinderModelComponent->SampleFromNoiseWithCFG(latentAnimDesc, batchedEncodedCondText, batchedEncodedUncondText, 7.5);

		//	testing
		//latentAnimDesc = m_NNNoiseFinderModelComponent->SampleFromNoiseWithCFG(latentAnimDesc, batchedEncodedCondText, batchedEncodedUncondText, 15.0);

		//	"we clearly see that increasing classifier-free guidance strength has the expected effect of decreasing sample variety and increasing individual sample fidelity."
	}

	//	decode latentAnimDesc into animation data
	{
		torch::Tensor	decodedData = g_NNVAE->DecoderForward(latentAnimDesc);

		m_lastGeneratedData = decodedData;
	}
}

void AnimGenSeqProcessingThread::DecodedDataToAnim(torch::Tensor decodedDataIn, AnimationIntermediateSeq* animOut)
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

void AnimGenSeqProcessingThread::TextEncoderGenerate()
{
	//	this part is a hack since the gamestate expects an animation to be generated after calling generate
	//FillRandomAnim();
	m_lastGeneratedData = torch::tensor({});

	int				startToken = g_NNTokenizer->GetStartToken();
	int				endToken = g_NNTokenizer->GetEndToken();

	torch::Tensor	startTokenTensor = torch::tensor({startToken}).to(torch::kInt64);
	torch::Tensor	endTokenTensor = torch::tensor({endToken}).to(torch::kInt64);

	torch::Tensor	dataTensor = g_animSeqDataSet->m_validationText;

	const int		kSliceSize = NNTextEncoderImpl::kMaxTokens - 2;

	int				randMin = 0;
	int				randMax = (int)(dataTensor.size(0) - kSliceSize - 1);
	int				textSamplePosition = rand() % randMax;

	torch::Tensor	contextText = dataTensor.slice(0, textSamplePosition, textSamplePosition + kSliceSize);
	torch::Tensor	targetText = dataTensor.slice(0, textSamplePosition + 1, textSamplePosition + kSliceSize + 1);

	contextText = torch::cat({startTokenTensor, contextText, endTokenTensor}, 0);
	targetText = torch::cat({startTokenTensor, targetText, endTokenTensor}, 0);

	torch::Tensor	encodedText = g_NNTextEncoder->forward(contextText.unsqueeze(0).to(m_deviceToUse));
	torch::Tensor	logits = g_NNTextEncoder->LanguageModelingForward(encodedText);
	logits = logits.squeeze(0).to(torch::kCPU);

	{
		// focus only on the last time step
		// logits = logits[:, -1, :] // python version
        //logits = logits.index({torch::indexing::Slice(), -1, torch::indexing::Slice()});
        
		// apply softmax to get probabilities
        torch::Tensor	probs = torch::nn::functional::softmax(logits, -1);
        
		// sample from the distribution
        torch::Tensor	nextData = torch::multinomial(probs, 1);
		nextData = nextData.squeeze(-1);
        
		// append sampled index to the running sequence
		//context = torch::cat({context, nextData}, 1); // (B, T+1)

		std::string	tempStr;

		//	convert everything to text and print it
		DebugPrintf("TextEncoderGenerate : \n");
		g_NNTokenizer->DecodeTensor(contextText, &tempStr);
		DebugPrintf("\tCONTEXT : \n%s\n\n", tempStr.c_str());
		g_NNTokenizer->DecodeTensor(targetText, &tempStr);
		DebugPrintf("\tTARGET  : \n%s\n\n", tempStr.c_str());
		g_NNTokenizer->DecodeTensor(nextData, &tempStr);
		DebugPrintf("\tRESULT  : \n%s\n\n", tempStr.c_str());
	}
}

void AnimGenSeqProcessingThread::FillRandomAnim()
{
	//	prepare for generation
	m_generatedAnimIntermediate.ClearForGeneration();
	m_generatedAnimIntermediate.InitFromModelIntermediate(m_targetModelIntermediate);
	torch::Tensor	boneData = m_generatedAnimIntermediate.boneTree.ToTensor().unsqueeze(0).to(m_deviceToUse);

	//	this should not be used in the long run, but using it for test info right now
	//int							targetAnimIdx = 0;
	int							targetAnimIdx = g_animSeqDataSet->GetRandomValidationAnimIdx();
	AnimationIntermediateSeq*	pTargetAnim = &g_animSeqDataSet->m_validationAnims[targetAnimIdx];
	
	{
		//	this keyframe count is a hack; need a better way to determine this		
		//int	goalAnimFrameCount = (int)(pTargetAnim->keyFrames.size());
		//int	goalAnimFrameCount = AnimGenHyperParameters::kMaxKeyframeSequenceLength;
		int		goalAnimFrameCount = std::min((int)(pTargetAnim->keyFrames.size()), (int)AnimGenHyperParameters::kMaxKeyframeSequenceLength);

		//	init keyframes
		for(int keyframeIdx = 0; keyframeIdx < goalAnimFrameCount; ++keyframeIdx)
		{
			AnimationIntermediateSeq::KeyFrame*	pNewKeyFrame = m_generatedAnimIntermediate.AddKeyFrame();

			for(int boneIdx = 0; boneIdx < (int)m_generatedAnimIntermediate.keyFrames[0].boneRotations.size(); ++boneIdx)
			{
				m_generatedAnimIntermediate.keyFrames[keyframeIdx].boneTranslations[boneIdx] = pTargetAnim->keyFrames[keyframeIdx].boneTranslations[boneIdx];
				m_generatedAnimIntermediate.keyFrames[keyframeIdx].boneRotations[boneIdx] = pTargetAnim->keyFrames[keyframeIdx].boneRotations[boneIdx];
				m_generatedAnimIntermediate.keyFrames[keyframeIdx].boneScales[boneIdx] = pTargetAnim->keyFrames[keyframeIdx].boneScales[boneIdx];
			}
		}
	}

	m_generatedAnimIntermediate.animImageEuler.Init(m_generatedAnimIntermediate);
	InitDebugSprite((pTargetAnim->animImageEuler.GetLocalKeyframeImageForWholeAnim().slice(1, 0, 64) + M_PI) / (2 * M_PI), kSource);
	InitDebugSprite((m_generatedAnimIntermediate.animImageEuler.GetLocalKeyframeImageForWholeAnim().slice(1, 0, 64) + M_PI) / (2 * M_PI), kTarget);
}

void AnimGenSeqProcessingThread::SetSampleCount(int sampleCount)
{
	m_sampleCount = sampleCount;
}

void AnimGenSeqProcessingThread::UpdateTargetModel(const char* modelToUse)
{
	ModelRef	targetModelRef = Model::MakeRef(modelToUse);
	m_targetModelIntermediate.InitFromModelData(*targetModelRef->GetData());
}

void AnimGenSeqProcessingThread::ExportGeneratedAnim(AnimationData* animData)
{
	m_generatedAnimIntermediate.ExportToAnimData(animData);
}

int AnimGenSeqProcessingThread::ExportGeneratedAnims(AnimationData* animDataArrayOut, int animDataCount)
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

void AnimGenSeqProcessingThread::EnteringState(int currentState)
{
	AnimGenProcessingThreadBase::EnteringState(currentState);

	switch(currentState)
	{
	case kProcessState_Training:
		{
			m_dataSet.ToDevice(m_deviceToUse);
		}
		break;

	default:
		break;
	}
}