#include "vapch.hpp"
#include "Noise.hpp"

#include "FastNoise.h"

namespace Vanta {

	static FastNoise s_FastNoise;

	Noise::Noise(int seed)
	{
		m_FastNoise = new FastNoise(seed);
		m_FastNoise->SetNoiseType(FastNoise::Simplex);
	}

	Noise::~Noise()
	{
		delete m_FastNoise;
	}

	float Noise::GetFrequency() const
	{
		return m_FastNoise->GetFrequency();
	}

	void Noise::SetFrequency(float frequency)
	{
		m_FastNoise->SetFrequency(frequency);
	}

	int Noise::GetFractalOctaves() const
	{
		return m_FastNoise->GetFractalOctaves();
	}

	void Noise::SetFractalOctaves(int octaves)
	{
		m_FastNoise->SetFractalOctaves(octaves);
	}

	float Noise::GetFractalLacunarity() const
	{
		return m_FastNoise->GetFractalLacunarity();
	}

	void Noise::SetFractalLacunarity(float lacunarity)
	{
		return m_FastNoise->SetFractalLacunarity(lacunarity);
	}

	float Noise::GetFractalGain() const
	{
		return m_FastNoise->GetFractalGain();
	}

	void Noise::SetFractalGain(float gain)
	{
		m_FastNoise->SetFractalGain(gain);
	}

	float Noise::Get(float x, float y)
	{
		return m_FastNoise->GetNoise(x, y);
	}

	void Noise::SetSeed(int seed)
	{
		s_FastNoise.SetSeed(seed);
	}

	float Noise::PerlinNoise(float x, float y)
	{
		s_FastNoise.SetNoiseType(FastNoise::Perlin);
		float result = s_FastNoise.GetNoise(x, y); // This returns a value between -1 and 1
		return result;
	}
}