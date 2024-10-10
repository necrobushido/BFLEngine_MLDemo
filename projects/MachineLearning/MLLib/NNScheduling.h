#pragma once

#include "SharedMLHeader.h"

//
class NNScheduler
{
public:
	virtual float Schedule(int64_t n)
	{
		return 1.0f;
	}

	virtual bool Finished(int64_t n)
	{
		return false;
	}
};

//
class NNLambdaWarmUpCosineScheduler : public NNScheduler
{
public:
	NNLambdaWarmUpCosineScheduler(int64_t warm_up_steps, float lr_min, float lr_max, float lr_start, int64_t max_decay_steps);

	float Schedule(int64_t n) override;

protected:
	int64_t		m_lr_warm_up_steps;
	float		m_lr_min;
	float		m_lr_max;
	float		m_lr_start;
	float		m_last_lr;
	int64_t		m_max_decay_steps;
};

//
class NNLambdaWarmUpCosinePeriodScheduler : public NNScheduler
{
public:
	NNLambdaWarmUpCosinePeriodScheduler(int64_t warm_up_steps, float lr_min, float lr_max, float lr_start, int64_t max_decay_steps);

	float Schedule(int64_t n) override;

protected:
	int64_t		m_lr_warm_up_steps;
	float		m_lr_min;
	float		m_lr_max;
	float		m_lr_start;
	float		m_last_lr;
	int64_t		m_max_decay_steps;
};

//
class NNLambdaWarmUpCosineAnnealingScheduler : public NNScheduler
{
public:
	NNLambdaWarmUpCosineAnnealingScheduler(float lr_min, float lr_max, int64_t warm_up_steps, int64_t max_decay_steps, float cycleScale);

	float Schedule(int64_t n) override;

protected:
	int64_t		m_lr_warm_up_steps;
	float		m_lr_min;
	float		m_lr_max;
	float		m_last_lr;
	int64_t		m_max_decay_steps;
	int64_t		m_cycleSteps;
	float		m_cycleScale;
};

//
class NNLambdaWarmUpCosineScheduler2 : public NNScheduler
{
public:
	NNLambdaWarmUpCosineScheduler2(std::vector<int64_t> warm_up_steps, std::vector<float> f_min, std::vector<float> f_max, std::vector<float> f_start, std::vector<int64_t> cycle_lengths);

	int64_t FindInInterval(int64_t n);
	float Schedule(int64_t n) override;
	bool Finished(int64_t n) override;

protected:
	std::vector<int64_t>	m_lr_warm_up_steps;
	std::vector<float>		m_f_min;
	std::vector<float>		m_f_max;
	std::vector<float>		m_f_start;
	std::vector<int64_t>	m_cycle_lengths;
	std::vector<int64_t>	m_cumulativeCycles;

	float					m_last_f;
};

//
class NNLambdaLinearScheduler : public NNLambdaWarmUpCosineScheduler2
{
public:
	NNLambdaLinearScheduler(std::vector<int64_t> warm_up_steps, std::vector<float> f_min, std::vector<float> f_max, std::vector<float> f_start, std::vector<int64_t> cycle_lengths);

	float Schedule(int64_t n) override;
};