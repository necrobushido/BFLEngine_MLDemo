#pragma once

//	probably need to test these to compare loss for same number of training iterations instead of time to see if they produce actually better models?
//		embed size and transformer layers seem directly proportional to training speed
namespace AnimGenHyperParameters
{
	enum
	{
		kMaxBoneSequenceLength = 16,			//	prob want this to be a power of some number (2 in this case) in case we want to do gradual flattening
		kMaxKeyframeSequenceLength = 128		//	same
	};

	extern const double	kSecondsPerKeyFrame;
}