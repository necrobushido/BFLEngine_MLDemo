#pragma once

#include "types.h"

class Clock
{
public:
	Clock():
		m_invPCFreq(0.0),
		m_counterStart(0)
	{
		BOOL			querySuccess;
		LARGE_INTEGER	li;
		
		querySuccess = QueryPerformanceFrequency(&li);
		Assert(querySuccess);
		m_invPCFreq = 1.0 / ((double)li.QuadPart);

		querySuccess = QueryPerformanceCounter(&li);
		Assert(querySuccess);
		m_counterStart = li.QuadPart;
	}

public:
	double GetTimeSlice()
	{
		BOOL			querySuccess;
		LARGE_INTEGER	li;
		querySuccess = QueryPerformanceCounter(&li);
		Assert(querySuccess);

		double			returnValue = (double)(li.QuadPart - m_counterStart) * m_invPCFreq;
		m_counterStart = li.QuadPart;
		return returnValue;
	}

private:
	double	m_invPCFreq;
	__int64	m_counterStart;
};