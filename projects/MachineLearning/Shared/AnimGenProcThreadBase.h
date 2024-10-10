#pragma once

#include "NNProcThreadBase.h"

class AnimDescTagList;
class AnimationData;

class AnimGenProcessingThreadBase : public NNProcessingThreadBase
{
public:
	AnimGenProcessingThreadBase();
	virtual ~AnimGenProcessingThreadBase();

public:
	virtual void SetSampleCount(int sampleCount){}
	virtual void UpdateTargetModel(const char* modelToUse){}
	virtual void ExportGeneratedAnim(AnimationData* animData){}
	virtual int ExportGeneratedAnims(AnimationData* animDataArrayOut, int animDataCount){ return 0; }
};