#include "NNScheduling.h"
#include "types.h"

//
NNLambdaWarmUpCosineScheduler::NNLambdaWarmUpCosineScheduler(int64_t warm_up_steps, float lr_min, float lr_max, float lr_start, int64_t max_decay_steps)
{
	m_lr_warm_up_steps = warm_up_steps;
	m_lr_min = lr_min;
	m_lr_max = lr_max;
	m_lr_start = lr_start;
	m_last_lr = 0.0f;
	m_max_decay_steps = max_decay_steps;
}

float NNLambdaWarmUpCosineScheduler::Schedule(int64_t n)
{
	float	result = 0.0f;

	if( n < m_lr_warm_up_steps )
	{
		result = (m_lr_max - m_lr_start) / (float)m_lr_warm_up_steps * (float)n + m_lr_start;
	}
	else
	{
		float	t = ((float)(n - m_lr_warm_up_steps) / (float)(m_max_decay_steps - m_lr_warm_up_steps));
		t = std::min(t, 1.0f);

		result = m_lr_min + 0.5f * (m_lr_max - m_lr_min) * (1 + cosf(t * (float)M_PI));
	}

	m_last_lr = result;
	return result;
}

//
NNLambdaWarmUpCosinePeriodScheduler::NNLambdaWarmUpCosinePeriodScheduler(int64_t warm_up_steps, float lr_min, float lr_max, float lr_start, int64_t max_decay_steps)
{
	m_lr_warm_up_steps = warm_up_steps;
	m_lr_min = lr_min;
	m_lr_max = lr_max;
	m_lr_start = lr_start;
	m_last_lr = 0.0f;
	m_max_decay_steps = max_decay_steps;
}

float NNLambdaWarmUpCosinePeriodScheduler::Schedule(int64_t n)
{
	float	result = 0.0f;

	if( n < m_lr_warm_up_steps )
	{
		result = (m_lr_max - m_lr_start) / (float)m_lr_warm_up_steps * (float)n + m_lr_start;
	}
	else
	{
		float	t = ((float)(n - m_lr_warm_up_steps) / (float)(m_max_decay_steps - m_lr_warm_up_steps));

		result = m_lr_min + 0.5f * (m_lr_max - m_lr_min) * (1 + cosf(t * (float)M_PI));
	}

	m_last_lr = result;
	return result;
}

//
NNLambdaWarmUpCosineAnnealingScheduler::NNLambdaWarmUpCosineAnnealingScheduler(float lr_min, float lr_max, int64_t warm_up_steps, int64_t max_decay_steps, float cycleScale)
{
	m_lr_warm_up_steps = warm_up_steps;
	m_lr_min = lr_min;
	m_lr_max = lr_max;
	m_last_lr = 0.0f;
	m_max_decay_steps = max_decay_steps;
	m_cycleScale = cycleScale;

	m_cycleSteps = m_lr_warm_up_steps + m_max_decay_steps;
}

float NNLambdaWarmUpCosineAnnealingScheduler::Schedule(int64_t n)
{
	float	result = 0.0f;

	int64_t	currentCycle = n / m_cycleSteps;
	int64_t	currentCycleStep = n % m_cycleSteps;

	if( currentCycleStep < m_lr_warm_up_steps )
	{
		result = (m_lr_max - m_lr_min) / (float)m_lr_warm_up_steps * (float)currentCycleStep + m_lr_min;
	}
	else
	{
		float	t = ((float)(currentCycleStep - m_lr_warm_up_steps) / (float)(m_max_decay_steps - m_lr_warm_up_steps));
		t = std::min(t, 1.0f);

		result = m_lr_min + 0.5f * (m_lr_max - m_lr_min) * (1 + cosf(t * (float)M_PI));
	}

	result = powf(m_cycleScale, (float)currentCycle) * result;

	m_last_lr = result;
	return result;
}

//
NNLambdaWarmUpCosineScheduler2::NNLambdaWarmUpCosineScheduler2(std::vector<int64_t> warm_up_steps, std::vector<float> f_min, std::vector<float> f_max, std::vector<float> f_start, std::vector<int64_t> cycle_lengths)
{
	Assert(warm_up_steps.size() == f_max.size());
	Assert(f_min.size() == f_max.size());
	Assert(f_start.size() == f_max.size());
	Assert(cycle_lengths.size() == f_max.size());

	m_lr_warm_up_steps = warm_up_steps;
	m_f_start = f_start;
	m_f_min = f_min;
	m_f_max = f_max;
	m_cycle_lengths = cycle_lengths;

	m_cumulativeCycles = m_cycle_lengths;
	m_cumulativeCycles.insert(m_cumulativeCycles.begin(), 0);
	for(int i = 1; i < (int)m_cumulativeCycles.size(); ++i)
	{
		m_cumulativeCycles[i] = m_cumulativeCycles[i] + m_cumulativeCycles[i-1];
	}

	m_last_f = 1.0f;
}

int64_t NNLambdaWarmUpCosineScheduler2::FindInInterval(int64_t n)
{
	int64_t	interval = 0;

	for(int64_t i = 1; i < (int64_t)m_cumulativeCycles.size(); ++i)
	{
		int64_t	cl = m_cumulativeCycles[i];
		if( n <= cl )
		{
			return interval;
		}

		interval += 1;
	}

	return -1;
}

float NNLambdaWarmUpCosineScheduler2::Schedule(int64_t n)
{
	float	result = 0.0f;
	int64_t	cycle = FindInInterval(n);

	if( cycle < 0 || cycle >= (int64_t)m_f_max.size() )
	{
		return m_last_f;
	}

	n = n - m_cumulativeCycles[cycle];

	if( n < m_lr_warm_up_steps[cycle] )
	{
		result = (m_f_max[cycle] - m_f_start[cycle]) / (float)m_lr_warm_up_steps[cycle] * (float)n + m_f_start[cycle];
	}
	else
	{
		float	t = ((float)(n - m_lr_warm_up_steps[cycle]) / (float)(m_cycle_lengths[cycle] - m_lr_warm_up_steps[cycle]));
		t = std::min(t, 1.0f);

		result = m_f_min[cycle] + 0.5f * (m_f_max[cycle] - m_f_min[cycle]) * (1 + cosf(t * (float)M_PI));
	}

	m_last_f = result;
	return result;
}

bool NNLambdaWarmUpCosineScheduler2::Finished(int64_t n)
{
	int64_t	finalIteration = m_cumulativeCycles[m_cumulativeCycles.size()-1];

	return n >= finalIteration;
}

//
NNLambdaLinearScheduler::NNLambdaLinearScheduler(std::vector<int64_t> warm_up_steps, std::vector<float> f_min, std::vector<float> f_max, std::vector<float> f_start, std::vector<int64_t> cycle_lengths):
	NNLambdaWarmUpCosineScheduler2(warm_up_steps, f_min, f_max, f_start, cycle_lengths)
{
}

float NNLambdaLinearScheduler::Schedule(int64_t n)
{
	float	result = 0.0f;
	int64_t	cycle = FindInInterval(n);

	if( cycle < 0 || cycle >= (int64_t)m_f_max.size() )
	{
		return m_last_f;
	}

	n = n - m_cumulativeCycles[cycle];

	if( n < m_lr_warm_up_steps[cycle] )
	{
		//	slowly interpolate from start up to max
		result = (m_f_max[cycle] - m_f_start[cycle]) / (float)m_lr_warm_up_steps[cycle] * (float)n + m_f_start[cycle];
	}
	else
	{
		//	slowly interpolate from max down to min over the cycle length
		result = m_f_min[cycle] + (m_f_max[cycle] - m_f_min[cycle]) * (float)(m_cycle_lengths[cycle] - n) / (float)(m_cycle_lengths[cycle]);
	}

	m_last_f = result;
	return result;
}