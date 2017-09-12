// Copyright (c) 2017 Santiago Fernandez Ortiz
// 
// Redistribution and use in source and binary forms, with or without modification, 
// are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, 
// this list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, 
// this list of conditions and the following disclaimer in the documentation 
// and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "quicknet_time.h"

#ifdef _WIN32
#	include <Windows.h>
#else
#	include <time.h>
#	include <unistd.h>
#endif

namespace quicknet
{
	namespace Utils
	{
		/*
		// in C++11 it should go like this
		#include <chrono>
		std::chrono::steady_clock last = steady_clock::now();
		std::chrono::steady_clock now = steady_clock::now();
		double elapsedSeconds = ((now - last).count()) * (steady_clock::period::num * multiplier) / (static_cast<double>(steady_clock::period::den) / divider);
		*/

#ifdef _WIN32
		// multiplier 1 = seconds
		uint64_t elapsedMonotonicTime(uint64_t multiplier)
		{
			LARGE_INTEGER pcFreq;
			QueryPerformanceFrequency(&pcFreq);

			LARGE_INTEGER counter;
			QueryPerformanceCounter(&counter);

			static LONGLONG lastCounter = counter.QuadPart;
			LONGLONG count = (counter.QuadPart - lastCounter) * multiplier;
			return (uint64_t)(count / pcFreq.QuadPart);
		}
#else
		uint64_t elapsedMonotonicTime(uint64_t multiplier)
		{
			struct timespec time;
			clock_gettime(CLOCK_MONOTONIC, &time);

			static struct timespec lastTime = time;
			uint64_t divider = 1000000000 / multiplier; // nanoseconds in a second

			return ((time.tv_sec - lastTime.tv_sec) * multiplier + ((time.tv_nsec - lastTime.tv_nsec) / divider));
		}
#endif

		uint64_t GetElapsedSeconds()
		{
			return elapsedMonotonicTime(1);
		}

		uint64_t GetElapsedMilliseconds()
		{
			return elapsedMonotonicTime(1000);
		}

		uint64_t GetElapsedMicroseconds()
		{
			return elapsedMonotonicTime(1000000);
		}

		void SleepSeconds(uint32_t seconds)
		{
			quicknet::Utils::SleepMicroseconds(seconds * 1000000);
		}

		void SleepMilliseconds(uint32_t milliseconds)
		{
			quicknet::Utils::SleepMicroseconds(milliseconds * 1000);
		}

		// in windows microseconds are treated as signed
		void SleepMicroseconds(uint32_t microseconds)
		{
			#ifdef _WIN32
				// ::Sleep() is not accurate enough

				// The time after which the state of the timer is to be set to signaled, in 100 nanosecond intervals. 
				// Negative values indicate relative time. 
				LARGE_INTEGER time;
				time.QuadPart = -(10 * (int64_t)microseconds);

				// create the timer
				HANDLE waitableTimer = CreateWaitableTimer(NULL, TRUE, NULL);
				SetWaitableTimer(waitableTimer, &time, 0, NULL, NULL, 0);
				// wait for it
				WaitForSingleObject(waitableTimer, INFINITE);
				// remove it
				CloseHandle(waitableTimer);
			#else
				usleep(microseconds);
			#endif
		}
	}
}
