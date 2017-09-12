//
// Isolated RNG
// required when using deterministic randomness on some parts of the game
// using Intel's fastrand
//

#pragma once
#include <stdint.h>

namespace quicknet
{
	class FastRand
	{
	public:
		FastRand(uint32_t seed) : m_seed(seed) {};

		int32_t GetInt(int32_t min, int32_t max)
		{
			return min + (fastrand() % (max - min + 1));
		}

		float GetFloat(float min = 0.0f, float max = 1.0f)
		{
			const float normalized = (float)fastrand() / (float)RAND_MAX;
			return min + normalized * (max - min);
		}

	private:
		int32_t fastrand()
		{
			m_seed = (214013 * m_seed + 2531011);
			return (m_seed >> 16) & 0x7FFF;
		}

		uint32_t m_seed;
	};
}
