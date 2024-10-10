#include "NNVAELoss.h"
#include "types.h"

#include "Continuous3DRotation6.h"
#include "AnimGenBoneDictionary.h"

NNVAELossImpl::NNVAELossImpl()
{
	//	seems weird but this is how it was done in SD, with kLogVarInit passed in (but I never saw it be anything but 0)
	const f32	kLogVarInit = 0.0f;
	m_logVarParam = register_parameter("m_logVarParam", torch::ones({}, torch::kF32) * kLogVarInit);
}

NNVAELossImpl::~NNVAELossImpl()
{
}

torch::Tensor NNVAELossImpl::forward(torch::Tensor inputAnimData, torch::Tensor decodedData, torch::Tensor encoderMean, torch::Tensor encoderLogVar, torch::Tensor& lossOut, std::string& logOut)
{
	CalcLoss(inputAnimData, decodedData, encoderMean, encoderLogVar, lossOut, logOut);

	return lossOut;
}

void NNVAELossImpl::CalcLoss(torch::Tensor inputAnimData, torch::Tensor decodedData, torch::Tensor encoderMean, torch::Tensor encoderLogVar, torch::Tensor& lossOut, std::string& logOut)
{
	//CalcLossUsual(inputAnimData, decodedData, encoderMean, encoderLogVar, lossOut, logOut);
	CalcLossSD(inputAnimData, decodedData, encoderMean, encoderLogVar, lossOut, logOut);
}

/*
on	{4, 16, 16}, no reconstructionLoss
	maybe usable but not great
	could train more
Train NNVAEModelComponent 13890 : current loss = 52.2705574, recent loss avg = 54.1190491, recent loss stddev = 11.9942255
	alignment loss = 52.2445908, KLLoss loss = 0.025967855

AnimGenSeqProcessingThread::ValidationIteration : current epoch = 21, 69.637382 minutes, 13899 training iterations
VALIDATION NNVAEModelComponent : current loss = 267.260284, recent loss avg = 266.291534, recent loss stddev = 59.3484535
	alignment loss = 267.233582, KLLoss loss = 0.0266960785
	current learning rate = 3.51562512e-08, best validation avg seen = 242.236252

Train NNVAEModelComponent 13900 : current loss = 51.4709778, recent loss avg = 54.1561394, recent loss stddev = 11.9812326
	alignment loss = 51.445015, KLLoss loss = 0.0259628538
*/

/*
on	{4, 16, 16}, yes reconstructionLoss
	maybe usable but not great
	could train more
Train NNVAEModelComponent 21690 : current loss = 1428.55054, recent loss avg = 1241.56445, recent loss stddev = 166.226608
	reconstruction loss = 1321.92236, alignment loss = 106.617439, KLLoss loss = 0.0107963393

AnimGenSeqProcessingThread::ValidationIteration : current epoch = 33, 107.610101 minutes, 21699 training iterations
VALIDATION NNVAEModelComponent : current loss = 2526.29663, recent loss avg = 2354.52002, recent loss stddev = 392.898621
	reconstruction loss = 2287.59277, alignment loss = 238.693665, KLLoss loss = 0.0102813244
	current learning rate = 3.51562512e-08, best validation avg seen = 2170.16187

Train NNVAEModelComponent 21700 : current loss = 1298.84412, recent loss avg = 1242.5271, recent loss stddev = 166.821411
	reconstruction loss = 1214.60486, alignment loss = 84.2281113, KLLoss loss = 0.0111031625
*/
void NNVAELossImpl::CalcLossUsual(torch::Tensor inputAnimData, torch::Tensor decodedData, torch::Tensor encoderMean, torch::Tensor encoderLogVar, torch::Tensor& lossOut, std::string& logOut)
{
	//	alignment
	torch::Tensor		targets = inputAnimData.view({inputAnimData.size(0), inputAnimData.size(1), inputAnimData.size(2), Continuous3DRotation6::kRowCount, Continuous3DRotation6::kColumnCount});
	targets = XYBasisToRotMtx33(targets);

	torch::Tensor		outputDataRot = decodedData.view({decodedData.size(0), decodedData.size(1), decodedData.size(2), Continuous3DRotation6::kRowCount, Continuous3DRotation6::kColumnCount});
	outputDataRot = XYBasisToRotMtx33(outputDataRot);

	//torch::Tensor		alignmentLoss = DotProductAlignmentLossNN(outputDataRot, targets);
	torch::Tensor		alignmentLoss = DotProductAlignmentLoss(outputDataRot, targets, 1.0f, false);
	alignmentLoss = torch::sum(alignmentLoss) / alignmentLoss.size(0);

	//	reconstruction
	torch::Tensor		reconstructionLoss = torch::abs(inputAnimData - decodedData);
	reconstructionLoss = torch::sum(reconstructionLoss) / reconstructionLoss.size(0);
	
	//	KL
	//	this is the way they were doing it in SD, but it isn't size invariant.
	const f32			kKLCoefficient = 0.000001f;
	torch::Tensor		KLLoss = UnitGaussianKLLossBatchDim(encoderMean, encoderLogVar, {1, 2, 3});
	KLLoss = kKLCoefficient * torch::sum(KLLoss) / KLLoss.size(0);

	//	I think this would be the invariant version, assuming the SD kKLCoefficient is correct
	//const f32			kKLCoefficient = 0.016384f;
	//torch::Tensor		KLLoss = kKLCoefficient * UnitGaussianKLLossInvariant(encoderMean, encoderLogVar);

	//	total loss
	lossOut = reconstructionLoss + KLLoss + alignmentLoss;
	//lossOut = KLLoss + alignmentLoss;

	//	log
	{
		char	tempBuffer[256];
		//sprintf(tempBuffer, "alignment loss = %.9g, KLLoss loss = %.9g", alignmentLoss.item().toFloat(), KLLoss.item().toFloat());
		sprintf(tempBuffer, "reconstruction loss = %.9g, alignment loss = %.9g, KLLoss loss = %.9g", reconstructionLoss.item().toFloat(), alignmentLoss.item().toFloat(), KLLoss.item().toFloat());

		logOut = tempBuffer;
	}
}

/*
on	{4, 16, 16} with 32 GroupNorm and 128 hidden
	maybe usable but not great
	could train more
Train NNVAEModelComponent 30290 : current loss = -3658.48071, recent loss avg = -3822.96484, recent loss stddev = 147.618073
	ReconstructionLoss loss = -3741.85938, KLLoss loss = 0.0125246225, alignment loss = 83.3662262, logVarParam = -0.0968564898

AnimGenSeqProcessingThread::ValidationIteration : current epoch = 46, 151.989929 minutes, 30296 training iterations
VALIDATION NNVAEModelComponent : current loss = -2587.62573, recent loss avg = -2451.5415, recent loss stddev = 324.03717
	ReconstructionLoss loss = -2774.48071, KLLoss loss = 0.0130786933, alignment loss = 186.841705, logVarParam = -0.0968567505
	current learning rate = 3.51562512e-08, best validation avg seen = -2451.5415

Train NNVAEModelComponent 30300 : current loss = -3954.86938, recent loss avg = -3823.59717, recent loss stddev = 147.674164
	ReconstructionLoss loss = -3992.00708, KLLoss loss = 0.0132606234, alignment loss = 37.1245956, logVarParam = -0.0968568623
*/

/*
on	{8, 16, 16}, with 16 GroupNorm and 128 hidden
	pretty good but not perfect, some rolling errors etc
Train NNVAEModelComponent 48190 : current loss = -5861.20264, recent loss avg = -5891.5249, recent loss stddev = 120.987152
	ReconstructionLoss loss = -5904.27051, KLLoss loss = 0.0295766164, alignment loss = 43.0380478, logVarParam = -0.133970827

AnimGenSeqProcessingThread::ValidationIteration : current epoch = 73, 242.941435 minutes, 48199 training iterations
VALIDATION NNVAEModelComponent : current loss = -4088.41748, recent loss avg = -4661.51172, recent loss stddev = 311.08136
	ReconstructionLoss loss = -4313.02051, KLLoss loss = 0.0293586403, alignment loss = 224.573639, logVarParam = -0.133970827
	current learning rate = 5.49316426e-10, best validation avg seen = -4776.99512

Would lower learning rate, but stopped by lower bound of 9.99999972e-10

Train NNVAEModelComponent 48200 : current loss = -5669.86475, recent loss avg = -5892.27051, recent loss stddev = 120.796524
	ReconstructionLoss loss = -5727.36621, KLLoss loss = 0.0303298943, alignment loss = 57.471138, logVarParam = -0.133970827
*/

/*
on	{8, 16, 16}, with 32 GroupNorm and 256 hidden
	pretty good but not perfect, better than the above
Train NNVAEModelComponent 36390 : current loss = -5960.60742, recent loss avg = -5872.91699, recent loss stddev = 91.19767
	ReconstructionLoss loss = -5978.14258, KLLoss loss = 0.0304006599, alignment loss = 17.5049286, logVarParam = -0.128788888

AnimGenSeqProcessingThread::ValidationIteration : current epoch = 55, 466.749552 minutes, 36398 training iterations
VALIDATION NNVAEModelComponent : current loss = -4231.61572, recent loss avg = -4318.43457, recent loss stddev = 306.824341
	ReconstructionLoss loss = -4445.08545, KLLoss loss = 0.0300667379, alignment loss = 213.439362, logVarParam = -0.128789157
	current learning rate = 3.51562512e-08, best validation avg seen = -4581.64844

Train NNVAEModelComponent 36400 : current loss = -5891.15918, recent loss avg = -5872.91699, recent loss stddev = 91.2030792
	ReconstructionLoss loss = -5917.74365, KLLoss loss = 0.0307390671, alignment loss = 26.5534706, logVarParam = -0.128789186
*/

/*
on	{8, 16, 16}, with 32 GroupNorm and 256 hidden and alignment loss weighted x100
	don't think this is better than non-weighted
*/

/*
on	{8, 16, 16}, with 32 GroupNorm and 128 hidden
	metrics look better but results look slightly worse?
Train NNVAEModelComponent 39490 : current loss = -5881.95605, recent loss avg = -5918.84814, recent loss stddev = 118.624077
	nll loss = -5919.60059, KLLoss loss = 0.022288207, alignment loss = 37.6222839, logVarParam = -0.133872524

AnimGenSeqProcessingThread::ValidationIteration : current epoch = 60, 195.097872 minutes, 39499 training iterations
VALIDATION NNVAEModelComponent : current loss = -4187.8877, recent loss avg = -4621.98877, recent loss stddev = 318.58371
	nll loss = -4404.58643, KLLoss loss = 0.0215249788, alignment loss = 216.677078, logVarParam = -0.133872822
	current learning rate = 3.51562512e-08, best validation avg seen = -4731.6333

Train NNVAEModelComponent 39500 : current loss = -5894.3501, recent loss avg = -5919.91943, recent loss stddev = 118.120651
	nll loss = -5941.30078, KLLoss loss = 0.0227407999, alignment loss = 46.9276161, logVarParam = -0.133872822
*/

/*
on	{8, 16, 16}, with 32 GroupNorm and 64 hidden
	does look a bit worse
Train NNVAEModelComponent 32990 : current loss = -3950.96509, recent loss avg = -3644.59595, recent loss stddev = 201.369324
	nll loss = -4014.71191, KLLoss loss = 0.0241623707, alignment loss = 63.7226181, logVarParam = -0.102308229

AnimGenSeqProcessingThread::ValidationIteration : current epoch = 50, 111.621365 minutes, 32999 training iterations
VALIDATION NNVAEModelComponent : current loss = -2151.30469, recent loss avg = -2512.41138, recent loss stddev = 255.090652
	nll loss = -2440.05225, KLLoss loss = 0.0217229649, alignment loss = 288.725952, logVarParam = -0.102308601
	current learning rate = 3.51562512e-08, best validation avg seen = -2883.06006

Train NNVAEModelComponent 33000 : current loss = -3604.3562, recent loss avg = -3644.479, recent loss stddev = 201.417221
	nll loss = -3715.21948, KLLoss loss = 0.0235063955, alignment loss = 110.839775, logVarParam = -0.102308601
*/

/*
on	{16, 16, 16}, with 32 GroupNorm and 128 hidden
	not perfect, but prob best so far
Train NNVAEModelComponent 65780 : current loss = -9560.41211, recent loss avg = -9518.05957, recent loss stddev = 104.246559
	nll loss = -9584.41211, KLLoss loss = 0.0465005338, alignment loss = 23.9529762, logVarParam = -0.202484727

Train NNVAEModelComponent 65790 : current loss = -9551.80859, recent loss avg = -9518.49707, recent loss stddev = 104.508522
	nll loss = -9571.88086, KLLoss loss = 0.0446744636, alignment loss = 20.027401, logVarParam = -0.202484727

AnimGenSeqProcessingThread::ValidationIteration : current epoch = 100, 328.311460 minutes, 65798 training iterations
VALIDATION NNVAEModelComponent : current loss = -8408.2373, recent loss avg = -8248.66504, recent loss stddev = 407.791565
	nll loss = -8502.91699, KLLoss loss = 0.0447099134, alignment loss = 94.6349335, logVarParam = -0.202484727
	current learning rate = 5.49316426e-10, best validation avg seen = -8456.87695

Would lower learning rate, but stopped by lower bound of 9.99999972e-10	
*/

/*
on	{8, 32, 32}, with 32 GroupNorm and 128 hidden
	a bit worse than {16, 16, 16} but maybe comparable
	confounder : this loses 2 res blocks and a conv so lower parameter count and fewer transformations prior to latent
Train NNVAEModelComponent 48180 : current loss = -5900.23242, recent loss avg = -5891.59619, recent loss stddev = 112.185547
	nll loss = -5926.73047, KLLoss loss = 0.0775257573, alignment loss = 26.4201698, logVarParam = -0.134079844

Train NNVAEModelComponent 48190 : current loss = -6018.72852, recent loss avg = -5891.86572, recent loss stddev = 112.389839
	nll loss = -6045.01855, KLLoss loss = 0.0814298019, alignment loss = 26.2086449, logVarParam = -0.134079844

AnimGenSeqProcessingThread::ValidationIteration : current epoch = 73, 219.255180 minutes, 48198 training iterations
VALIDATION NNVAEModelComponent : current loss = -4613.76123, recent loss avg = -4994.58496, recent loss stddev = 238.715866
	nll loss = -4753.33691, KLLoss loss = 0.0767140165, alignment loss = 139.49884, logVarParam = -0.134079844
	current learning rate = 5.49316426e-10, best validation avg seen = -5093.69873

Would lower learning rate, but stopped by lower bound of 9.99999972e-10
*/

/*
on	{8, 32, 32}, with 32 GroupNorm and 128 hidden, numResBlocksPerEntry = 3
	comparable to above
Train NNVAEModelComponent 48690 : current loss = -5900.396, recent loss avg = -5953.40186, recent loss stddev = 107.221504
	nll loss = -5939.21729, KLLoss loss = 0.0991272479, alignment loss = 38.7222214, logVarParam = -0.134093702

AnimGenSeqProcessingThread::ValidationIteration : current epoch = 74, 275.809127 minutes, 48699 training iterations
VALIDATION NNVAEModelComponent : current loss = -4456.94189, recent loss avg = -4844.64648, recent loss stddev = 254.300354
	nll loss = -4615.69531, KLLoss loss = 0.0960710496, alignment loss = 158.657257, logVarParam = -0.134093702
	current learning rate = 5.49316426e-10, best validation avg seen = -5061.58398

Would lower learning rate, but stopped by lower bound of 9.99999972e-10

Train NNVAEModelComponent 48700 : current loss = -5945.8833, recent loss avg = -5953.82373, recent loss stddev = 107.86721
	nll loss = -5972.41113, KLLoss loss = 0.101831213, alignment loss = 26.4258633, logVarParam = -0.134093702
*/

/*
on	{16, 32, 32}, with 32 GroupNorm and 128 hidden
	best so far.  dramatically better than {8, 32, 32}.  channels seem to make a big difference?  still not perfect, but not too far off
	hesitant to make this bigger since this is comparable size to SD.  probably add problematic animations to training set from here?
Train NNVAEModelComponent 79590 : current loss = -12905.0703, recent loss avg = -13005.707, recent loss stddev = 95.1345215
	nll loss = -12927.9951, KLLoss loss = 0.177577779, alignment loss = 22.7466946, logVarParam = -0.270382613

AnimGenSeqProcessingThread::ValidationIteration : current epoch = 121, 362.984574 minutes, 79599 training iterations
VALIDATION NNVAEModelComponent : current loss = -12143.1162, recent loss avg = -12036.0674, recent loss stddev = 219.424591
	nll loss = -12208.7822, KLLoss loss = 0.184779912, alignment loss = 65.4814835, logVarParam = -0.270382613
	current learning rate = 5.49316426e-10, best validation avg seen = -12129.5469

Would lower learning rate, but stopped by lower bound of 9.99999972e-10

Train NNVAEModelComponent 79600 : current loss = -13039.5488, recent loss avg = -13005.9688, recent loss stddev = 94.8221283
	nll loss = -13052.3086, KLLoss loss = 0.179880261, alignment loss = 12.5796833, logVarParam = -0.270382613	
*/
void NNVAELossImpl::CalcLossSD(torch::Tensor inputAnimData, torch::Tensor decodedData, torch::Tensor encoderMean, torch::Tensor encoderLogVar, torch::Tensor& lossOut, std::string& logOut)
{
	//	basis vector alignment loss
	torch::Tensor		targets = inputAnimData.view({inputAnimData.size(0), inputAnimData.size(1), inputAnimData.size(2), Continuous3DRotation6::kRowCount, Continuous3DRotation6::kColumnCount});
	targets = XYBasisToRotMtx33(targets);

	torch::Tensor		outputDataRot = decodedData.view({decodedData.size(0), decodedData.size(1), decodedData.size(2), Continuous3DRotation6::kRowCount, Continuous3DRotation6::kColumnCount});
	outputDataRot = XYBasisToRotMtx33(outputDataRot);

	//torch::Tensor		alignmentLoss = DotProductAlignmentLossNN(outputDataRot, targets);
	torch::Tensor		alignmentLoss = DotProductAlignmentLoss(outputDataRot, targets, 1.0f, false);
	alignmentLoss = torch::sum(alignmentLoss) / alignmentLoss.size(0);

	//	calc nll from reconstruction
	//		note that this isn't size invariant; it will scale with larger amounts of data
	//const float			kSDImageSize = 512 * 512 * 3;	//	use SD's image size to try to match coefficients
	//const float			kOurAnimSize = inputAnimData.size(1) * inputAnimData.size(2) * inputAnimData.size(3);
	//const float			kNLLCoefficient = kSDImageSize / kOurAnimSize;
	torch::Tensor		reconstructionError = torch::abs(inputAnimData - decodedData);
	torch::Tensor		nll = reconstructionError / m_logVarParam.exp() + m_logVarParam;
	nll = torch::sum(nll) / nll.size(0);
	//nll = kNLLCoefficient * torch::sum(nll) / nll.size(0);
	//nll = kSDImageSize * torch::mean(nll);	//	so would this be the invariant method?  I wonder if they don't do this just so that the numbers are bigger?

	//	KL loss
	//const f32			kKLCoefficient = 0.016384f;
	//KLLossOut = kKLCoefficient * UnitGaussianKLLossInvariant(encoderMean, encoderLogVar);
	const f32			kKLCoefficient = 0.000001f;
	//const float			kSDLatentSize = 4 * 64 * 64;
	//const float			kOurLatentSize = encoderMean.size(1) * encoderMean.size(2) * encoderMean.size(3);
	//const f32			kKLCoefficient = 0.000001f * kSDLatentSize / kOurLatentSize;
	torch::Tensor		KLLoss = UnitGaussianKLLossBatchDim(encoderMean, encoderLogVar, {1, 2, 3});
	KLLoss = kKLCoefficient * torch::sum(KLLoss) / KLLoss.size(0);

	//	total loss
	//lossOut = nll + KLLossOut;
	lossOut = nll + KLLoss + alignmentLoss;

	//	log
	{
		char	tempBuffer[256];
		sprintf(tempBuffer, "nll loss = %.9g, KLLoss loss = %.9g, alignment loss = %.9g, logVarParam = %.9g", nll.item().toFloat(), KLLoss.item().toFloat(), alignmentLoss.item().toFloat(), m_logVarParam.item().toFloat());

		logOut = tempBuffer;
	}
}