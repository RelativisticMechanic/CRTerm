#ifndef TIMER_H
#define TIMER_H

#include "SDL.h"

class Timer
{
public:
	uint64_t interval_ms;
	uint64_t previous;

	Timer()
	{

	}

	Timer(uint64_t interval)
	{
		this->interval_ms = interval;
		this->previous = SDL_GetTicks64();
	}

	bool Elapsed(void)
	{
		uint64_t current = SDL_GetTicks64();
		if (current - previous >= this->interval_ms)
		{
			previous = current;
			return true; 
		}
		else
		{
			return false;
		}
	}

	float NormalizedTime(void)
	{
		if (this->Elapsed())
		{
			return 0.0f;
		}
		return (float)(SDL_GetTicks64() - previous) / (float)interval_ms;
	}
	void Reset(void)
	{
		this->previous = SDL_GetTicks64();
	}

};

#endif