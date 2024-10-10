#include "NNResConv2DMulti.h"

#include "NNUtilityBlocks.h"
#include "SelfAttention.h"

namespace
{
	enum
	{
		kAttentionHeads = 1
	};

	const bool	kAttentionCausal = false;
}

//	This seems to be dramatically worse than the other attention method
class NNConvAttentionImpl : public torch::nn::Module 
{
public:
	NNConvAttentionImpl(int channels, int normSize):
		m_normSeq(register_module("m_seq", torch::nn::Sequential())),
		m_querySeq(register_module("m_querySeq", torch::nn::Sequential())),
		m_keySeq(register_module("m_keySeq", torch::nn::Sequential())),
		m_valueSeq(register_module("m_valueSeq", torch::nn::Sequential())),
		m_projOutSeq(register_module("m_projOutSeq", torch::nn::Sequential()))
	{
		m_normSeq->push_back(register_module("groupNorm", torch::nn::GroupNorm(normSize, channels)));

		m_querySeq->push_back(register_module("queryConv", torch::nn::Conv2d(torch::nn::Conv2dOptions(channels, channels, 1).padding(0))));
		m_keySeq->push_back(register_module("keyConv", torch::nn::Conv2d(torch::nn::Conv2dOptions(channels, channels, 1).padding(0))));
		m_valueSeq->push_back(register_module("valueConv", torch::nn::Conv2d(torch::nn::Conv2dOptions(channels, channels, 1).padding(0))));
		
		m_projOutSeq->push_back(register_module("projOutConv", torch::nn::Conv2d(torch::nn::Conv2dOptions(channels, channels, 1).padding(0))));
	}

    torch::Tensor forward(torch::Tensor x)
	{
		torch::Tensor		h_ = m_normSeq->forward(x);
		torch::Tensor		q = m_querySeq->forward(x);
		torch::Tensor		k = m_keySeq->forward(x);
		torch::Tensor		v = m_valueSeq->forward(x);

		torch::IntArrayRef	shapes = q.sizes();
		s64					inputB = shapes[0];
		s64					inputC = shapes[1];
		s64					inputH = shapes[2];
		s64					inputW = shapes[3];

		q = q.reshape({inputB, inputC, inputH*inputW});
		q = q.permute({0, 2, 1});

		k = k.reshape({inputB, inputC, inputH*inputW});

		torch::Tensor		w_ = torch::bmm(q, k);	//	bmm = batch matrix multiply
		w_ = w_ * (1.0f / sqrt(inputC));

		int	dim = 2;
		torch::nn::functional::SoftmaxFuncOptions	softMaxoptions(dim);
		w_ = torch::nn::functional::softmax(w_, softMaxoptions);

		v = v.reshape({inputB, inputC, inputH*inputW});
		w_ = w_.permute({0, 2, 1});

		h_ = torch::bmm(v, w_);
		h_ = h_.reshape({inputB, inputC, inputH, inputW});
		
		h_ = m_projOutSeq->forward(h_);

		return x + h_;
	}

	torch::nn::Sequential	m_normSeq;
	torch::nn::Sequential	m_querySeq;
	torch::nn::Sequential	m_keySeq;
	torch::nn::Sequential	m_valueSeq;
	torch::nn::Sequential	m_projOutSeq;
};

TORCH_MODULE(NNConvAttention);

//
class NNInterpolateBlockImpl : public torch::nn::Module 
{
public:
	NNInterpolateBlockImpl(double xScale, double yScale):
		m_interpX(xScale),
		m_interpY(yScale)
	{
	}

    torch::Tensor forward(torch::Tensor x)
	{
		x = torch::nn::functional::interpolate(x, torch::nn::functional::InterpolateFuncOptions().scale_factor(std::vector<double>({m_interpX, m_interpY})).mode(torch::kBilinear));

		return x;
	}

	double	m_interpX;
	double	m_interpY;
};

TORCH_MODULE(NNInterpolateBlock);

//
class NNResConv2DataSizeImpl : public torch::nn::Module 
{
public:
	NNResConv2DataSizeImpl(int sizeIn, int sizeOut, double dropout, eNonLinearity nonLinearity):
		m_seq(register_module("m_seq", torch::nn::Sequential())),
		m_residualSeq(register_module("m_residualSeq", torch::nn::Sequential()))
	{
		m_seq->push_back(register_module("layerNormInput", torch::nn::LayerNorm(torch::nn::LayerNormOptions({sizeIn}))));
		AddNonLinearityModule(m_seq, nonLinearity, "NLInput");
		m_seq->push_back(register_module("linearInput", torch::nn::Linear(sizeIn, sizeOut)));
		m_seq->push_back(register_module("dropout", torch::nn::Dropout(torch::nn::DropoutOptions(dropout))));

		m_residualSeq->push_back(register_module("linearRes", torch::nn::Linear(sizeIn, sizeOut)));
	}

    torch::Tensor forward(torch::Tensor x)
	{
		x = m_seq->forward(x) + m_residualSeq->forward(x);

		return x;
	}

	torch::nn::Sequential	m_seq;
	torch::nn::Sequential	m_residualSeq;
};

TORCH_MODULE(NNResConv2DataSize);

//
class NNResConv2InterpBlockImpl : public torch::nn::Module 
{
public:
	NNResConv2InterpBlockImpl(double xScale, double yScale, int inputChannels, int outputChannels, int normSize, double dropout, eNonLinearity nonLinearity):
		m_seq1(register_module("m_seq1", torch::nn::Sequential()))
	{
		m_seq1->push_back(register_module("interp", NNInterpolateBlock(xScale, yScale)));
		m_seq1->push_back(register_module("srb11b", NNResConv2DMultiBlock(inputChannels, outputChannels, normSize, dropout, nonLinearity)));
	}

    torch::Tensor forward(torch::Tensor x)
	{
		x = m_seq1->forward(x);

		return x;
	}

	torch::nn::Sequential	m_seq1;
};

TORCH_MODULE(NNResConv2InterpBlock);

//
class NNResConv2DDownSampleLayer2Impl : public torch::nn::Module 
{
public:
	NNResConv2DDownSampleLayer2Impl(int hiddenLayerChannels, int normSize, double dropout, eNonLinearity nonLinearity):
		m_seq1(register_module("m_seq1", torch::nn::Sequential()))
	{
		//m_seq1->push_back(register_module("groupNorm", torch::nn::GroupNorm(normSize, hiddenLayerChannels)));		
		//m_seq1->push_back(register_module("convRes1", torch::nn::Conv2d(torch::nn::Conv2dOptions(hiddenLayerChannels, hiddenLayerChannels, 3).padding(1))));
		m_seq1->push_back(register_module("interp", NNInterpolateBlock(1.0, 0.5)));
		m_seq1->push_back(register_module("srb11b", NNResConv2DMultiBlock(hiddenLayerChannels, hiddenLayerChannels, normSize, dropout, nonLinearity)));
		//m_seq1->push_back(register_module("convRes2", torch::nn::Conv2d(torch::nn::Conv2dOptions(hiddenLayerChannels, hiddenLayerChannels, 3).padding(1))));
	}

    torch::Tensor forward(torch::Tensor x)
	{
		x = m_seq1->forward(x);

		return x;
	}

	torch::nn::Sequential	m_seq1;
};

TORCH_MODULE(NNResConv2DDownSampleLayer2);

//
class NNResConv2DUpSampleLayer2Impl : public torch::nn::Module 
{
public:
	NNResConv2DUpSampleLayer2Impl(int hiddenLayerChannels, int normSize, double dropout, eNonLinearity nonLinearity):
		m_seq1(register_module("m_seq1", torch::nn::Sequential()))
	{
		//m_seq1->push_back(register_module("groupNorm", torch::nn::GroupNorm(normSize, hiddenLayerChannels)));
		//m_seq1->push_back(register_module("convRes1", torch::nn::Conv2d(torch::nn::Conv2dOptions(hiddenLayerChannels, hiddenLayerChannels, 3).padding(1))));
		m_seq1->push_back(register_module("interp", NNInterpolateBlock(1.0, 2.0)));
		m_seq1->push_back(register_module("srb11b", NNResConv2DMultiBlock(hiddenLayerChannels, hiddenLayerChannels, normSize, dropout, nonLinearity)));
		//m_seq1->push_back(register_module("convRes2", torch::nn::Conv2d(torch::nn::Conv2dOptions(hiddenLayerChannels, hiddenLayerChannels, 3).padding(1))));
	}

    torch::Tensor forward(torch::Tensor x)
	{
		x = m_seq1->forward(x);

		return x;
	}

	torch::nn::Sequential	m_seq1;
};

TORCH_MODULE(NNResConv2DUpSampleLayer2);

//
class NNResConv2DDownSampleLayerImpl : public torch::nn::Module 
{
public:
	NNResConv2DDownSampleLayerImpl(int channels, int xStride, int yStride):
		m_seq(register_module("m_seq", torch::nn::Sequential()))
	{
		//m_seq->push_back(register_module("convDown", torch::nn::Conv2d(torch::nn::Conv2dOptions(channels, channels, 3).stride({xStride, yStride}).padding(1))));
		m_seq->push_back(register_module("convDown", torch::nn::Conv2d(torch::nn::Conv2dOptions(channels, channels, 3).stride({xStride, yStride}).padding(0))));
	}

    torch::Tensor forward(torch::Tensor x)
	{
		//x = torch::nn::functional::pad(x, torch::nn::functional::PadFuncOptions({0, 1, 0, 1}));	//	{2, 2}
		x = torch::nn::functional::pad(x, torch::nn::functional::PadFuncOptions({0, 1, 1, 1}));		//	{1, 2}
		x = m_seq->forward(x);

		return x;
	}

	torch::nn::Sequential	m_seq;
};

TORCH_MODULE(NNResConv2DDownSampleLayer);

//
class NNResConv2DUpSampleLayerImpl : public torch::nn::Module 
{
public:
	NNResConv2DUpSampleLayerImpl(int channels, double xScale, double yScale):
		m_seq(register_module("m_seq", torch::nn::Sequential()))
	{
		m_seq->push_back(register_module("upsample", torch::nn::Upsample(torch::nn::UpsampleOptions().scale_factor(std::vector<double>({xScale, yScale})))));
		//m_seq->push_back(register_module("upsample", torch::nn::Upsample(torch::nn::UpsampleOptions().mode(torch::kBilinear).scale_factor(std::vector<double>({xScale, yScale})))));
		m_seq->push_back(register_module("convUp", torch::nn::Conv2d(torch::nn::Conv2dOptions(channels, channels, 3).padding(1))));
	}

    torch::Tensor forward(torch::Tensor x)
	{
		x = m_seq->forward(x);

		return x;
	}

	torch::nn::Sequential	m_seq;
};

TORCH_MODULE(NNResConv2DUpSampleLayer);

//
class NNResConvAttentionBlockImpl : public torch::nn::Module
{
public:
	NNResConvAttentionBlockImpl(int channels, double dropout):
		m_seq(register_module("m_seq", torch::nn::Sequential()))
	{
		m_seq->push_back(register_module("transpose1", Transpose12Layer()));
		m_seq->push_back(register_module("attentionLayerNorm", torch::nn::LayerNorm(torch::nn::LayerNormOptions({channels}))));
		m_seq->push_back(register_module("selfAttention", SelfAttention(kAttentionHeads, channels, false, false, kAttentionCausal, dropout)));
		m_seq->push_back(register_module("transpose2", Transpose12Layer()));
		m_seq->push_back(register_module("dropout", torch::nn::Dropout(torch::nn::DropoutOptions(dropout))));
	}

    torch::Tensor forward(torch::Tensor x)
	{
		torch::Tensor	residue = x;

		x = x.view({residue.size(0), residue.size(1), residue.size(2) * residue.size(3)});
		x = m_seq->forward(x);
		x = x.view({residue.size(0), residue.size(1), residue.size(2), residue.size(3)});

		return x + residue;
	}

public:
	torch::nn::Sequential	m_seq;

};

TORCH_MODULE(NNResConvAttentionBlock);

/*Train NNVAEModelComponent 20390 : current loss = 0.00192882493, recent loss avg = 0.00186068448
	ReconstructionLoss loss = 0.000999705167, KLLoss loss = 0.464559883

AnimGenSeqProcessingThread::ValidationIteration : current epoch = 31, 122.748189 minutes, 20399 training iterations
VALIDATION NNVAEModelComponent : current loss = 0.00230550114, recent loss avg = 0.00238809502, recent loss stddev = 0.000295919046
	ReconstructionLoss loss = 0.00140178273, KLLoss loss = 0.451859206
	current learning rate = 1.95312495e-07

Train NNVAEModelComponent 20400 : current loss = 0.00214308454, recent loss avg = 0.00186084898
	ReconstructionLoss loss = 0.00117115944, KLLoss loss = 0.48596251*/
//NNResConv2DMultiEncoderImpl::NNResConv2DMultiEncoderImpl(int inputChannels, int outputChannels, int hiddenLayerChannels, int remapDataInputSize, int remapDataSize, int normSize, double dropout, eNonLinearity nonLinearity):
//	m_seq(register_module("m_seq", torch::nn::Sequential()))
//{
//	m_seq->push_back(register_module("convInput", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, hiddenLayerChannels, 3).padding(1))));
//
//	//	{b, inputChannels, 128, 6} -> {b, inputChannels, 128, 8}
//	m_seq->push_back(register_module("dataInput", NNResConv2DataSize(remapDataInputSize, remapDataSize, dropout, nonLinearity)));
//
//	//	
//	m_seq->push_back(register_module("half1", NNResConv2InterpBlock(0.5, 0.5, hiddenLayerChannels, hiddenLayerChannels*2, normSize, dropout, nonLinearity)));
//
//	//	mid
//	m_seq->push_back(register_module("rbMid1", NNResConv2DMultiBlock(hiddenLayerChannels*2, hiddenLayerChannels*2, normSize, dropout, nonLinearity)));
//	m_seq->push_back(register_module("rbMid2", NNResConv2DMultiBlock(hiddenLayerChannels*2, hiddenLayerChannels*2, normSize, dropout, nonLinearity)));
//	m_seq->push_back(register_module("rbMid3", NNResConv2DMultiBlock(hiddenLayerChannels*2, hiddenLayerChannels*2, normSize, dropout, nonLinearity)));
//	m_seq->push_back(register_module("attentionMid", NNResConvAttentionBlock(hiddenLayerChannels*2, dropout)));
//	m_seq->push_back(register_module("rbMid4", NNResConv2DMultiBlock(hiddenLayerChannels*2, hiddenLayerChannels*2, normSize, dropout, nonLinearity)));
//
//	//	NL
//	m_seq->push_back(register_module("groupNormFinal", torch::nn::GroupNorm(normSize, hiddenLayerChannels*2)));
//	AddNonLinearityModule(m_seq, nonLinearity, "NLFinal");
//	m_seq->push_back(register_module("convFinal1", torch::nn::Conv2d(torch::nn::Conv2dOptions(hiddenLayerChannels*2, outputChannels, 3).padding(1))));
//	m_seq->push_back(register_module("convFinal2", torch::nn::Conv2d(torch::nn::Conv2dOptions(outputChannels, outputChannels, 1))));
//}
//
//torch::Tensor NNResConv2DMultiEncoderImpl::forward(torch::Tensor x)
//{
//	return m_seq->forward(x);
//}
//
////
//NNResConv2DMultiDecoderImpl::NNResConv2DMultiDecoderImpl(int inputChannels, int outputChannels, int hiddenLayerChannels, int remapDataInputSize, int remapDataSize, int normSize, double dropout, eNonLinearity nonLinearity):
//	m_seq(register_module("m_seq", torch::nn::Sequential()))
//{
//	//	NL
//	m_seq->push_back(register_module("convInput1", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, inputChannels, 1))));
//	m_seq->push_back(register_module("convInput2", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, hiddenLayerChannels*2, 3).padding(1))));
//
//	//	mid
//	m_seq->push_back(register_module("rbMid1", NNResConv2DMultiBlock(hiddenLayerChannels*2, hiddenLayerChannels*2, normSize, dropout, nonLinearity)));
//	m_seq->push_back(register_module("attentionMid", NNResConvAttentionBlock(hiddenLayerChannels*2, dropout)));
//	m_seq->push_back(register_module("rbMid2", NNResConv2DMultiBlock(hiddenLayerChannels*2, hiddenLayerChannels*2, normSize, dropout, nonLinearity)));
//	m_seq->push_back(register_module("rbMid3", NNResConv2DMultiBlock(hiddenLayerChannels*2, hiddenLayerChannels*2, normSize, dropout, nonLinearity)));	
//	m_seq->push_back(register_module("rbMid4", NNResConv2DMultiBlock(hiddenLayerChannels*2, hiddenLayerChannels*2, normSize, dropout, nonLinearity)));
//
//	//
//	m_seq->push_back(register_module("half1", NNResConv2InterpBlock(2.0, 2.0, hiddenLayerChannels*2, hiddenLayerChannels, normSize, dropout, nonLinearity)));
//
//	//	8 to 6
//	m_seq->push_back(register_module("dataOutput", NNResConv2DataSize(remapDataSize, remapDataInputSize, dropout, nonLinearity)));
//
//	//	NL
//	m_seq->push_back(register_module("groupNormFinal", torch::nn::GroupNorm(normSize, hiddenLayerChannels)));
//	AddNonLinearityModule(m_seq, nonLinearity, "NLFinal");
//	m_seq->push_back(register_module("convFinal1", torch::nn::Conv2d(torch::nn::Conv2dOptions(hiddenLayerChannels, outputChannels, 3).padding(1))));
//}
//
//torch::Tensor NNResConv2DMultiDecoderImpl::forward(torch::Tensor x)
//{
//	return m_seq->forward(x);
//}

//	saved this one with kKLCoefficient = kReconstructionCoefficient / 500.0f;
/*
Train NNVAEModelComponent 25390 : current loss = 0.00168338127, recent loss avg = 0.0018854962
	ReconstructionLoss loss = 0.000717019488, KLLoss loss = 0.483180881

AnimGenSeqProcessingThread::ValidationIteration : current epoch = 38, 283.991117 minutes, 25399 training iterations
VALIDATION NNVAEModelComponent : current loss = 0.00244532106, recent loss avg = 0.00226826337, recent loss stddev = 0.000189313636
	ReconstructionLoss loss = 0.00137490872, KLLoss loss = 0.535206199
	current learning rate = 3.9062499e-07

Lowering learning rate to = 1.95312495e-07

Train NNVAEModelComponent 25400 : current loss = 0.00190995401, recent loss avg = 0.0018863572
	ReconstructionLoss loss = 0.000889922609, KLLoss loss = 0.510015666
*/
//
//NNResConv2DMultiEncoderImpl::NNResConv2DMultiEncoderImpl(int inputChannels, int outputChannels, int hiddenLayerChannels, int remapDataInputSize, int remapDataSize, int normSize, double dropout, eNonLinearity nonLinearity):
//	m_seq(register_module("m_seq", torch::nn::Sequential()))
//{
//	m_seq->push_back(register_module("convInput", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, hiddenLayerChannels, 3).padding(1))));
//
//	//	{b, inputChannels, 128, 6} -> {b, inputChannels, 128, 8}
//	m_seq->push_back(register_module("dataInput", NNResConv2DataSize(remapDataInputSize, remapDataSize, dropout, nonLinearity)));
//
//	//	
//	m_seq->push_back(register_module("half1", NNResConv2InterpBlock(0.5, 0.5, hiddenLayerChannels, hiddenLayerChannels*2, normSize, dropout, nonLinearity)));
//
//	//	mid
//	m_seq->push_back(register_module("rbMid1", NNResConv2DMultiBlock(hiddenLayerChannels*2, hiddenLayerChannels*2, normSize, dropout, nonLinearity)));
//	m_seq->push_back(register_module("rbMid2", NNResConv2DMultiBlock(hiddenLayerChannels*2, hiddenLayerChannels*4, normSize, dropout, nonLinearity)));
//	m_seq->push_back(register_module("rbMid3", NNResConv2DMultiBlock(hiddenLayerChannels*4, hiddenLayerChannels*4, normSize, dropout, nonLinearity)));
//	m_seq->push_back(register_module("attentionMid", NNResConvAttentionBlock(hiddenLayerChannels*4, dropout)));
//	m_seq->push_back(register_module("rbMid4", NNResConv2DMultiBlock(hiddenLayerChannels*4, hiddenLayerChannels*4, normSize, dropout, nonLinearity)));
//
//	//	NL
//	m_seq->push_back(register_module("groupNormFinal", torch::nn::GroupNorm(normSize, hiddenLayerChannels*4)));
//	AddNonLinearityModule(m_seq, nonLinearity, "NLFinal");
//	m_seq->push_back(register_module("convFinal1", torch::nn::Conv2d(torch::nn::Conv2dOptions(hiddenLayerChannels*4, outputChannels, 3).padding(1))));
//	m_seq->push_back(register_module("convFinal2", torch::nn::Conv2d(torch::nn::Conv2dOptions(outputChannels, outputChannels, 1))));
//}
//
//torch::Tensor NNResConv2DMultiEncoderImpl::forward(torch::Tensor x)
//{
//	return m_seq->forward(x);
//}
//
////
//NNResConv2DMultiDecoderImpl::NNResConv2DMultiDecoderImpl(int inputChannels, int outputChannels, int hiddenLayerChannels, int remapDataInputSize, int remapDataSize, int normSize, double dropout, eNonLinearity nonLinearity):
//	m_seq(register_module("m_seq", torch::nn::Sequential()))
//{
//	//	NL
//	m_seq->push_back(register_module("convInput1", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, inputChannels, 1))));
//	m_seq->push_back(register_module("convInput2", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, hiddenLayerChannels*4, 3).padding(1))));
//
//	//	mid
//	m_seq->push_back(register_module("rbMid1", NNResConv2DMultiBlock(hiddenLayerChannels*4, hiddenLayerChannels*4, normSize, dropout, nonLinearity)));
//	m_seq->push_back(register_module("attentionMid", NNResConvAttentionBlock(hiddenLayerChannels*4, dropout)));
//	m_seq->push_back(register_module("rbMid2", NNResConv2DMultiBlock(hiddenLayerChannels*4, hiddenLayerChannels*4, normSize, dropout, nonLinearity)));
//	m_seq->push_back(register_module("rbMid3", NNResConv2DMultiBlock(hiddenLayerChannels*4, hiddenLayerChannels*2, normSize, dropout, nonLinearity)));	
//	m_seq->push_back(register_module("rbMid4", NNResConv2DMultiBlock(hiddenLayerChannels*2, hiddenLayerChannels*2, normSize, dropout, nonLinearity)));
//
//	//
//	m_seq->push_back(register_module("half1", NNResConv2InterpBlock(2.0, 2.0, hiddenLayerChannels*2, hiddenLayerChannels, normSize, dropout, nonLinearity)));
//
//	//	8 to 6
//	m_seq->push_back(register_module("dataOutput", NNResConv2DataSize(remapDataSize, remapDataInputSize, dropout, nonLinearity)));
//
//	//	NL
//	m_seq->push_back(register_module("groupNormFinal", torch::nn::GroupNorm(normSize, hiddenLayerChannels)));
//	AddNonLinearityModule(m_seq, nonLinearity, "NLFinal");
//	m_seq->push_back(register_module("convFinal1", torch::nn::Conv2d(torch::nn::Conv2dOptions(hiddenLayerChannels, outputChannels, 3).padding(1))));
//}
//
//torch::Tensor NNResConv2DMultiDecoderImpl::forward(torch::Tensor x)
//{
//	return m_seq->forward(x);
//}

/*
NNVAEOld_KLLow_SD_Quarter.pt saved on 
Train NNVAEModelComponent 22890 : current loss = 0.00014730898, recent loss avg = 0.000141124547
	ReconstructionLoss loss = 0.000111396541, KLLoss loss = 35.9124298

AnimGenSeqProcessingThread::ValidationIteration : current epoch = 34, 254.287944 minutes, 22898 training iterations
VALIDATION NNVAEModelComponent : current loss = 0.00286142202, recent loss avg = 0.00380656426, recent loss stddev = 0.00122674857
	ReconstructionLoss loss = 0.0028269731, KLLoss loss = 34.4489479
	current learning rate = 3.51562512e-08

Train NNVAEModelComponent 22900 : current loss = 0.000124379469, recent loss avg = 0.0001410353
	ReconstructionLoss loss = 8.82036766e-05, KLLoss loss = 36.1757927
*/
//
class EncoderEntryBlockImpl : public torch::nn::Module 
{
public:
	EncoderEntryBlockImpl(int inChannels, int outChannels, int numResBlocks, bool doDownSample, int normSize, double dropout, eNonLinearity nonLinearity):
		m_blockSeq(register_module("m_blockSeq", torch::nn::Sequential())),
		m_downsampleSeq(register_module("m_downsampleSeq", torch::nn::Sequential())),
		m_doDownSample(doDownSample)
	{
		int	blockIn = inChannels;
		int	blockOut = outChannels;
		for(int iBlock = 0; iBlock < numResBlocks; ++iBlock)
		{
			char	nameBuffer[256];
			sprintf(nameBuffer, "rb_%d", iBlock);
			m_blockSeq->push_back(register_module(nameBuffer, NNResConv2DMultiBlock(blockIn, blockOut, normSize, dropout, nonLinearity)));

			blockIn = blockOut;
		}

		if( m_doDownSample )
		{
			m_downsampleSeq->push_back(register_module("downsampleConv", torch::nn::Conv2d(torch::nn::Conv2dOptions(blockIn, blockIn, 3).stride(2).padding(0))));
		}
	}

    torch::Tensor forward(torch::Tensor x)
	{
		x = m_blockSeq->forward(x);

		if( m_doDownSample )
		{
			x = torch::nn::functional::pad(x, torch::nn::functional::PadFuncOptions({0, 1, 0, 1}));
			x = m_downsampleSeq->forward(x);
		}

		return x;
	}

	torch::nn::Sequential	m_blockSeq;
	torch::nn::Sequential	m_downsampleSeq;
	bool					m_doDownSample;
};

TORCH_MODULE(EncoderEntryBlock);

class DecoderEntryBlockImpl : public torch::nn::Module 
{
public:
	DecoderEntryBlockImpl(int inChannels, int outChannels, int numResBlocks, bool doUpSample, int normSize, double dropout, eNonLinearity nonLinearity):
		m_blockSeq(register_module("m_blockSeq", torch::nn::Sequential())),
		m_upsampleSeq(register_module("m_upsampleSeq", torch::nn::Sequential())),
		m_doUpSample(doUpSample)
	{
		int	blockIn = inChannels;
		int	blockOut = outChannels;
		for(int iBlock = 0; iBlock < numResBlocks+1; ++iBlock)	//	note that the decoder block has 1 more resblock than the encoder
		{
			char	nameBuffer[256];
			sprintf(nameBuffer, "rb_%d", iBlock);
			m_blockSeq->push_back(register_module(nameBuffer, NNResConv2DMultiBlock(blockIn, blockOut, normSize, dropout, nonLinearity)));

			blockIn = blockOut;
		}

		if( m_doUpSample )
		{
			m_upsampleSeq->push_back(register_module("upsampleConv", torch::nn::Conv2d(torch::nn::Conv2dOptions(blockIn, blockIn, 3).padding(1))));
		}
	}

    torch::Tensor forward(torch::Tensor x)
	{
		x = m_blockSeq->forward(x);

		if( m_doUpSample )
		{
			x = torch::nn::functional::interpolate(x, torch::nn::functional::InterpolateFuncOptions().scale_factor(std::vector<double>({2, 2})).mode(torch::kNearest));
			x = m_upsampleSeq->forward(x);
		}

		return x;
	}

	torch::nn::Sequential	m_blockSeq;
	torch::nn::Sequential	m_upsampleSeq;
	bool					m_doUpSample;
};

TORCH_MODULE(DecoderEntryBlock);

NNResConv2DMultiEncoderImpl::NNResConv2DMultiEncoderImpl(int inputChannels, int outputChannels, int hiddenLayerChannels, int remapDataInputSize, int remapDataSize, int normSize, double dropout, std::vector<int> channelMult, int numResBlocksPerEntry, eNonLinearity nonLinearity):
	m_seq(register_module("m_seq", torch::nn::Sequential()))
{
	int	numResolutions = (int)channelMult.size();
	Assert(numResolutions > 0);

	int	blockIn = hiddenLayerChannels * channelMult[0];
	int	blockOut = 0;

	//	channels up
	m_seq->push_back(register_module("convInput", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, blockIn, 3).padding(1))));

	//	data
	m_seq->push_back(register_module("dataInput", NNResConv2DataSize(remapDataInputSize, remapDataSize, dropout, nonLinearity)));

	//	downsample
	for(int iLevel = 0; iLevel < numResolutions; ++iLevel)
	{
		blockOut = hiddenLayerChannels * channelMult[iLevel];

		bool	doDownSample = iLevel < numResolutions - 1;

		char	nameBuffer[256];
		sprintf(nameBuffer, "entry_%d", iLevel);
		m_seq->push_back(register_module(nameBuffer, EncoderEntryBlock(blockIn, blockOut, numResBlocksPerEntry, doDownSample, normSize, dropout, nonLinearity)));

		blockIn = blockOut;
	}

	//	mid
	m_seq->push_back(register_module("rbMid1", NNResConv2DMultiBlock(blockIn, blockIn, normSize, dropout, nonLinearity)));
	m_seq->push_back(register_module("attentionMid", NNResConvAttentionBlock(blockIn, dropout)));
	m_seq->push_back(register_module("rbMid2", NNResConv2DMultiBlock(blockIn, blockIn, normSize, dropout, nonLinearity)));

	//	NL + channels down
	m_seq->push_back(register_module("groupNormFinal", torch::nn::GroupNorm(normSize, blockIn)));
	AddNonLinearityModule(m_seq, nonLinearity, "NLFinal");
	m_seq->push_back(register_module("convFinal1", torch::nn::Conv2d(torch::nn::Conv2dOptions(blockIn, outputChannels, 3).padding(1))));

	//	quant
	m_seq->push_back(register_module("quantConv", torch::nn::Conv2d(torch::nn::Conv2dOptions(outputChannels, outputChannels, 1).padding(0))));
}

torch::Tensor NNResConv2DMultiEncoderImpl::forward(torch::Tensor x)
{
	return m_seq->forward(x);
}

//
NNResConv2DMultiDecoderImpl::NNResConv2DMultiDecoderImpl(int inputChannels, int outputChannels, int hiddenLayerChannels, int remapDataInputSize, int remapDataSize, int normSize, double dropout, std::vector<int> channelMult, int numResBlocksPerEntry, eNonLinearity nonLinearity):
	m_seq(register_module("m_seq", torch::nn::Sequential()))
{
	int	numResolutions = (int)channelMult.size();
	Assert(numResolutions > 0);

	int	blockIn = hiddenLayerChannels * channelMult[numResolutions-1];
	int	blockOut = 0;

	//	quant
	m_seq->push_back(register_module("postQuantConv", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, inputChannels, 1).padding(0))));

	//	channels up
	m_seq->push_back(register_module("convInput2", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, blockIn, 3).padding(1))));

	//	mid
	m_seq->push_back(register_module("rbMid2", NNResConv2DMultiBlock(blockIn, blockIn, normSize, dropout, nonLinearity)));
	m_seq->push_back(register_module("attentionMid", NNResConvAttentionBlock(blockIn, dropout)));
	m_seq->push_back(register_module("rbMid1", NNResConv2DMultiBlock(blockIn, blockIn, normSize, dropout, nonLinearity)));

	//	upsample
	for(int iLevel = numResolutions - 1; iLevel >= 0; --iLevel)
	{
		blockOut = hiddenLayerChannels * channelMult[iLevel];

		bool	doUpSample = iLevel > 0;

		char	nameBuffer[256];
		sprintf(nameBuffer, "entry_%d", iLevel);
		m_seq->push_back(register_module(nameBuffer, DecoderEntryBlock(blockIn, blockOut, numResBlocksPerEntry, doUpSample, normSize, dropout, nonLinearity)));

		blockIn = blockOut;
	}

	//	data
	m_seq->push_back(register_module("dataOutput", NNResConv2DataSize(remapDataSize, remapDataInputSize, dropout, nonLinearity)));

	//	NL + channels down
	m_seq->push_back(register_module("groupNormFinal", torch::nn::GroupNorm(normSize, blockIn)));
	AddNonLinearityModule(m_seq, nonLinearity, "NLFinal");
	m_seq->push_back(register_module("convFinal1", torch::nn::Conv2d(torch::nn::Conv2dOptions(blockIn, outputChannels, 3).padding(1))));
}

torch::Tensor NNResConv2DMultiDecoderImpl::forward(torch::Tensor x)
{
	return m_seq->forward(x);
}

//	This is similar to SD but seemed bad.  maybe the compression is too much for the bone data?
//		I stopped here assuming ReconstructionLoss was stuck
/*
Train NNVAEModelComponent 4990 : current loss = 0.0825974494, recent loss avg = 19.0168209
	ReconstructionLoss loss = 0.062145967, KLLoss loss = 10.2257385

AnimGenSeqProcessingThread::ValidationIteration : current epoch = 7, 51.557535 minutes, 4999 training iterations
VALIDATION NNVAEModelComponent : current loss = 0.0665368438, recent loss avg = 0.0604867227, recent loss stddev = 0.0194418635
	ReconstructionLoss loss = 0.0474834554, KLLoss loss = 9.52669334
	current learning rate = 0.000250000012

Train NNVAEModelComponent 5000 : current loss = 0.074496299, recent loss avg = 19.0169735
	ReconstructionLoss loss = 0.0558157079, KLLoss loss = 9.34029484
*/
//NNResConv2DMultiEncoderImpl::NNResConv2DMultiEncoderImpl(int inputChannels, int outputChannels, int hiddenLayerChannels, int remapDataInputSize, int remapDataSize, int normSize, double dropout, eNonLinearity nonLinearity):
//	m_seq(register_module("m_seq", torch::nn::Sequential()))
//{
//	//	channels in
//	m_seq->push_back(register_module("convInput", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, hiddenLayerChannels, 3).padding(1))));
//
//	//	data resize
//	m_seq->push_back(register_module("dataInput", NNResConv2DataSize(remapDataInputSize, remapDataSize, dropout, nonLinearity)));
//
//	m_seq->push_back(register_module("half11", NNResConv2DMultiBlock(hiddenLayerChannels, hiddenLayerChannels, normSize, dropout, nonLinearity)));
//	m_seq->push_back(register_module("half12", NNResConv2DMultiBlock(hiddenLayerChannels, hiddenLayerChannels, normSize, dropout, nonLinearity)));
//
//	m_seq->push_back(register_module("interp1", NNInterpolateBlock(0.5, 0.5)));
//	m_seq->push_back(register_module("interpConv1", torch::nn::Conv2d(torch::nn::Conv2dOptions(hiddenLayerChannels, hiddenLayerChannels, 3).padding(1))));
//
//	m_seq->push_back(register_module("half21", NNResConv2DMultiBlock(hiddenLayerChannels, hiddenLayerChannels*2, normSize, dropout, nonLinearity)));
//	m_seq->push_back(register_module("half22", NNResConv2DMultiBlock(hiddenLayerChannels*2, hiddenLayerChannels*2, normSize, dropout, nonLinearity)));
//
//	m_seq->push_back(register_module("interp2", NNInterpolateBlock(0.5, 0.5)));
//	m_seq->push_back(register_module("interpConv2", torch::nn::Conv2d(torch::nn::Conv2dOptions(hiddenLayerChannels*2, hiddenLayerChannels*2, 3).padding(1))));
//
//	m_seq->push_back(register_module("half31", NNResConv2DMultiBlock(hiddenLayerChannels*2, hiddenLayerChannels*4, normSize, dropout, nonLinearity)));
//	m_seq->push_back(register_module("half32", NNResConv2DMultiBlock(hiddenLayerChannels*4, hiddenLayerChannels*4, normSize, dropout, nonLinearity)));
//
//	m_seq->push_back(register_module("interp3", NNInterpolateBlock(0.5, 0.5)));
//	m_seq->push_back(register_module("interpConv3", torch::nn::Conv2d(torch::nn::Conv2dOptions(hiddenLayerChannels*4, hiddenLayerChannels*4, 3).padding(1))));
//
//	//	mid
//	m_seq->push_back(register_module("rbMid1", NNResConv2DMultiBlock(hiddenLayerChannels*4, hiddenLayerChannels*4, normSize, dropout, nonLinearity)));
//	m_seq->push_back(register_module("rbMid2", NNResConv2DMultiBlock(hiddenLayerChannels*4, hiddenLayerChannels*4, normSize, dropout, nonLinearity)));
//	m_seq->push_back(register_module("attentionMid", NNResConvAttentionBlock(hiddenLayerChannels*4, dropout)));
//	m_seq->push_back(register_module("rbMid3", NNResConv2DMultiBlock(hiddenLayerChannels*4, hiddenLayerChannels*4, normSize, dropout, nonLinearity)));
//
//	//	NL
//	m_seq->push_back(register_module("groupNormFinal", torch::nn::GroupNorm(normSize, hiddenLayerChannels*4)));
//	AddNonLinearityModule(m_seq, nonLinearity, "NLFinal");
//	m_seq->push_back(register_module("convFinal1", torch::nn::Conv2d(torch::nn::Conv2dOptions(hiddenLayerChannels*4, outputChannels, 3).padding(1))));
//	m_seq->push_back(register_module("convFinal2", torch::nn::Conv2d(torch::nn::Conv2dOptions(outputChannels, outputChannels, 1))));
//}
//
//torch::Tensor NNResConv2DMultiEncoderImpl::forward(torch::Tensor x)
//{
//	return m_seq->forward(x);
//}
//
////
//NNResConv2DMultiDecoderImpl::NNResConv2DMultiDecoderImpl(int inputChannels, int outputChannels, int hiddenLayerChannels, int remapDataInputSize, int remapDataSize, int normSize, double dropout, eNonLinearity nonLinearity):
//	m_seq(register_module("m_seq", torch::nn::Sequential()))
//{
//	//	channels up
//	m_seq->push_back(register_module("convInput1", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, inputChannels, 1))));
//	m_seq->push_back(register_module("convInput2", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, hiddenLayerChannels*4, 3).padding(1))));
//
//	//
//	m_seq->push_back(register_module("rbMid1", NNResConv2DMultiBlock(hiddenLayerChannels*4, hiddenLayerChannels*4, normSize, dropout, nonLinearity)));
//	m_seq->push_back(register_module("attentionMid", NNResConvAttentionBlock(hiddenLayerChannels*4, dropout)));
//	m_seq->push_back(register_module("rbMid2", NNResConv2DMultiBlock(hiddenLayerChannels*4, hiddenLayerChannels*4, normSize, dropout, nonLinearity)));
//	m_seq->push_back(register_module("rbMid3", NNResConv2DMultiBlock(hiddenLayerChannels*4, hiddenLayerChannels*4, normSize, dropout, nonLinearity)));
//
//	m_seq->push_back(register_module("rbExtra1", NNResConv2DMultiBlock(hiddenLayerChannels*4, hiddenLayerChannels*4, normSize, dropout, nonLinearity)));
//	m_seq->push_back(register_module("rbExtra2", NNResConv2DMultiBlock(hiddenLayerChannels*4, hiddenLayerChannels*4, normSize, dropout, nonLinearity)));
//
//	m_seq->push_back(register_module("interp3", NNInterpolateBlock(2.0, 2.0)));
//	m_seq->push_back(register_module("interpConv3", torch::nn::Conv2d(torch::nn::Conv2dOptions(hiddenLayerChannels*4, hiddenLayerChannels*4, 3).padding(1))));
//
//	m_seq->push_back(register_module("half31", NNResConv2DMultiBlock(hiddenLayerChannels*4, hiddenLayerChannels*4, normSize, dropout, nonLinearity)));
//	m_seq->push_back(register_module("half32", NNResConv2DMultiBlock(hiddenLayerChannels*4, hiddenLayerChannels*4, normSize, dropout, nonLinearity)));	
//	m_seq->push_back(register_module("rbExtra3", NNResConv2DMultiBlock(hiddenLayerChannels*4, hiddenLayerChannels*4, normSize, dropout, nonLinearity)));
//
//	m_seq->push_back(register_module("interp2", NNInterpolateBlock(2.0, 2.0)));
//	m_seq->push_back(register_module("interpConv2", torch::nn::Conv2d(torch::nn::Conv2dOptions(hiddenLayerChannels*4, hiddenLayerChannels*4, 3).padding(1))));
//
//	m_seq->push_back(register_module("half21", NNResConv2DMultiBlock(hiddenLayerChannels*4, hiddenLayerChannels*2, normSize, dropout, nonLinearity)));
//	m_seq->push_back(register_module("half22", NNResConv2DMultiBlock(hiddenLayerChannels*2, hiddenLayerChannels*2, normSize, dropout, nonLinearity)));
//	m_seq->push_back(register_module("rbExtra4", NNResConv2DMultiBlock(hiddenLayerChannels*2, hiddenLayerChannels*2, normSize, dropout, nonLinearity)));
//
//	m_seq->push_back(register_module("interp1", NNInterpolateBlock(2.0, 2.0)));
//	m_seq->push_back(register_module("interpConv1", torch::nn::Conv2d(torch::nn::Conv2dOptions(hiddenLayerChannels*2, hiddenLayerChannels*2, 3).padding(1))));
//
//	m_seq->push_back(register_module("half11", NNResConv2DMultiBlock(hiddenLayerChannels*2, hiddenLayerChannels, normSize, dropout, nonLinearity)));
//	m_seq->push_back(register_module("half12", NNResConv2DMultiBlock(hiddenLayerChannels, hiddenLayerChannels, normSize, dropout, nonLinearity)));	
//	m_seq->push_back(register_module("rbExtra5", NNResConv2DMultiBlock(hiddenLayerChannels, hiddenLayerChannels, normSize, dropout, nonLinearity)));
//
//	//	data resize
//	m_seq->push_back(register_module("dataOutput", NNResConv2DataSize(remapDataSize, remapDataInputSize, dropout, nonLinearity)));
//
//	//	NL
//	m_seq->push_back(register_module("groupNormFinal", torch::nn::GroupNorm(normSize, hiddenLayerChannels)));
//	AddNonLinearityModule(m_seq, nonLinearity, "NLFinal");
//	m_seq->push_back(register_module("convFinal1", torch::nn::Conv2d(torch::nn::Conv2dOptions(hiddenLayerChannels, outputChannels, 3).padding(1))));
//}
//
//torch::Tensor NNResConv2DMultiDecoderImpl::forward(torch::Tensor x)
//{
//	return m_seq->forward(x);
//}

//
NNResConv2DMultiBlockImpl::NNResConv2DMultiBlockImpl(int inputChannels, int outputChannels, int normSize, double dropout, eNonLinearity nonLinearity):
	m_seq1(register_module("m_seq1", torch::nn::Sequential())),
	m_residualSeq(register_module("m_residualSeq", torch::nn::Sequential()))
{
	m_seq1->push_back(register_module("groupNorm1", torch::nn::GroupNorm(normSize, inputChannels)));
	AddNonLinearityModule(m_seq1, nonLinearity, "NL1");
	m_seq1->push_back(register_module("conv1", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, outputChannels, 3).bias(false).padding(1))));	//	directly followed by a norm so shouldn't need a bias?

	m_seq1->push_back(register_module("groupNorm2", torch::nn::GroupNorm(normSize, outputChannels)));
	AddNonLinearityModule(m_seq1, nonLinearity, "NL2");
	m_seq1->push_back(register_module("dropout", torch::nn::Dropout(torch::nn::DropoutOptions(dropout))));	//	the SD repository had it here
	m_seq1->push_back(register_module("conv2", torch::nn::Conv2d(torch::nn::Conv2dOptions(outputChannels, outputChannels, 3).padding(1))));
	//m_seq1->push_back(register_module("dropout", torch::nn::Dropout(torch::nn::DropoutOptions(dropout))));	//	I would think this would go here

	if( inputChannels == outputChannels )
	{
		m_residualSeq->push_back(register_module("residualLayer", torch::nn::Identity()));
	}
	else
	{
		m_residualSeq->push_back(register_module("residualLayer", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, outputChannels, 1).padding(0))));
	}
}

torch::Tensor NNResConv2DMultiBlockImpl::forward(torch::Tensor x)
{
	x = m_seq1->forward(x) + m_residualSeq->forward(x);

	return x;
}


//
//
NNResConv2DResizerEncoderImpl::NNResConv2DResizerEncoderImpl(int inputChannels, int outputChannels, int hiddenLayerChannels, int remapDataInputSize, int remapDataSize, int normSize, double dropout, eNonLinearity nonLinearity):
	m_seq(register_module("m_seq", torch::nn::Sequential()))
{
	//	6 to 8
	m_seq->push_back(register_module("dataInput", NNResConv2DataSize(remapDataInputSize, remapDataSize, dropout, nonLinearity)));

	//	input and channels up
	//m_seq->push_back(register_module("groupNormInput", torch::nn::GroupNorm(1, inputChannels)));
	////AddNonLinearityModule(m_seq, nonLinearity, "NLInput");
	//m_seq->push_back(register_module("convInput1", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, hiddenLayerChannels, 1).padding(0))));
	//m_seq->push_back(register_module("convInput2", torch::nn::Conv2d(torch::nn::Conv2dOptions(hiddenLayerChannels, hiddenLayerChannels, 3).padding(1))));

	m_seq->push_back(register_module("rbInput", NNResConv2DMultiBlock(inputChannels, hiddenLayerChannels, normSize, dropout, nonLinearity)));

	//m_seq->push_back(register_module("srb11", NNResConv2DMultiBlock(hiddenLayerChannels, hiddenLayerChannels, normSize, dropout, nonLinearity)));

	//	8->16->32->64->128
	//m_seq->push_back(register_module("sus1", NNResConv2InterpBlock(1.0, 2.0, hiddenLayerChannels, hiddenLayerChannels, normSize, dropout, nonLinearity)));
	//m_seq->push_back(register_module("sus2", NNResConv2InterpBlock(1.0, 2.0, hiddenLayerChannels, hiddenLayerChannels, normSize, dropout, nonLinearity)));
	//m_seq->push_back(register_module("sus3", NNResConv2InterpBlock(1.0, 2.0, hiddenLayerChannels, hiddenLayerChannels, normSize, dropout, nonLinearity)));
	//m_seq->push_back(register_module("sus4", NNResConv2InterpBlock(1.0, 2.0, hiddenLayerChannels, hiddenLayerChannels, normSize, dropout, nonLinearity)));

	//m_seq->push_back(register_module("half1", NNResConv2InterpBlock(0.5, 0.5, hiddenLayerChannels, hiddenLayerChannels, normSize, dropout, nonLinearity)));
	//m_seq->push_back(register_module("half2", NNResConv2InterpBlock(0.5, 0.5, hiddenLayerChannels, hiddenLayerChannels, normSize, dropout, nonLinearity)));
	m_seq->push_back(register_module("half3", NNResConv2InterpBlock(0.5, 0.5, hiddenLayerChannels, hiddenLayerChannels, normSize, dropout, nonLinearity)));

	

	//	output and channels down
	//m_seq->push_back(register_module("groupNormOutput", torch::nn::GroupNorm(normSize, hiddenLayerChannels)));
	////AddNonLinearityModule(m_seq, nonLinearity, "NLOutput");
	//m_seq->push_back(register_module("convOutput1", torch::nn::Conv2d(torch::nn::Conv2dOptions(hiddenLayerChannels, hiddenLayerChannels, 3).padding(1))));
	//m_seq->push_back(register_module("convOutput2", torch::nn::Conv2d(torch::nn::Conv2dOptions(hiddenLayerChannels, outputChannels, 1).padding(0))));

	m_seq->push_back(register_module("rbOutput", NNResConv2DMultiBlock(hiddenLayerChannels, outputChannels, normSize, dropout, nonLinearity)));
}

torch::Tensor NNResConv2DResizerEncoderImpl::forward(torch::Tensor x)
{
	return m_seq->forward(x);
}

//
NNResConv2DResizerDecoderImpl::NNResConv2DResizerDecoderImpl(int inputChannels, int outputChannels, int hiddenLayerChannels, int remapDataInputSize, int remapDataSize, int normSize, double dropout, eNonLinearity nonLinearity):
	m_seq(register_module("m_seq", torch::nn::Sequential()))
{
	//	input and channels up
	//m_seq->push_back(register_module("groupNormInput", torch::nn::GroupNorm(1, inputChannels)));
	////AddNonLinearityModule(m_seq, nonLinearity, "NLInput");
	//m_seq->push_back(register_module("convInput1", torch::nn::Conv2d(torch::nn::Conv2dOptions(inputChannels, hiddenLayerChannels, 1).padding(0))));
	//m_seq->push_back(register_module("convInput2", torch::nn::Conv2d(torch::nn::Conv2dOptions(hiddenLayerChannels, hiddenLayerChannels, 3).padding(1))));

	m_seq->push_back(register_module("rbInput", NNResConv2DMultiBlock(inputChannels, hiddenLayerChannels, normSize, dropout, nonLinearity)));

	m_seq->push_back(register_module("half1", NNResConv2InterpBlock(2.0, 2.0, hiddenLayerChannels, hiddenLayerChannels, normSize, dropout, nonLinearity)));
	//m_seq->push_back(register_module("half2", NNResConv2InterpBlock(2.0, 2.0, hiddenLayerChannels, hiddenLayerChannels, normSize, dropout, nonLinearity)));
	//m_seq->push_back(register_module("half3", NNResConv2InterpBlock(2.0, 2.0, hiddenLayerChannels, hiddenLayerChannels, normSize, dropout, nonLinearity)));

	//	128->64->32->16->8
	//m_seq->push_back(register_module("sus1", NNResConv2InterpBlock(1.0, 0.5, hiddenLayerChannels, hiddenLayerChannels, normSize, dropout, nonLinearity)));
	//m_seq->push_back(register_module("sus2", NNResConv2InterpBlock(1.0, 0.5, hiddenLayerChannels, hiddenLayerChannels, normSize, dropout, nonLinearity)));
	//m_seq->push_back(register_module("sus3", NNResConv2InterpBlock(1.0, 0.5, hiddenLayerChannels, hiddenLayerChannels, normSize, dropout, nonLinearity)));
	//m_seq->push_back(register_module("sus4", NNResConv2InterpBlock(1.0, 0.5, hiddenLayerChannels, hiddenLayerChannels, normSize, dropout, nonLinearity)));

	//m_seq->push_back(register_module("srb11b", NNResConv2DMultiBlock(hiddenLayerChannels, hiddenLayerChannels, normSize, dropout, nonLinearity)));

	//	output and channels down
	//m_seq->push_back(register_module("groupNormOutput", torch::nn::GroupNorm(normSize, hiddenLayerChannels)));
	////AddNonLinearityModule(m_seq, nonLinearity, "NLOutput");
	//m_seq->push_back(register_module("convOutput1", torch::nn::Conv2d(torch::nn::Conv2dOptions(hiddenLayerChannels, hiddenLayerChannels, 3).padding(1))));
	//m_seq->push_back(register_module("convOutput2", torch::nn::Conv2d(torch::nn::Conv2dOptions(hiddenLayerChannels, outputChannels, 1).padding(0))));

	m_seq->push_back(register_module("rbOutput", NNResConv2DMultiBlock(hiddenLayerChannels, outputChannels, normSize, dropout, nonLinearity)));

	//	8 to 6
	m_seq->push_back(register_module("dataOutput", NNResConv2DataSize(remapDataSize, remapDataInputSize, dropout, nonLinearity)));
}

torch::Tensor NNResConv2DResizerDecoderImpl::forward(torch::Tensor x)
{
	return m_seq->forward(x);
}
