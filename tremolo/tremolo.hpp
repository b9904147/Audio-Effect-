#include <effect.h>
#include <consts.h>
#include "waveshape_oscillator.hpp"
#include <vector>

using namespace ape;
GlobalData(Tremolo, "");

class Tremolo : public TransportEffect
{
public:

	enum class Rate
	{
		_8, _6, _4, _3, _2, _1dot5, _1, _3_4, _1_2, _3_8, _1_3, _5_16, _1_4, 
		_3_16, _1_6, _1_8, _1_12, _1_16, _1_24,_1_32, _1_48, _1_64
	};

	static constexpr Param<Rate>::Names rateNames {
		"8", "6", "4", "3", "2", "1.5", "1", "3/4", "1/2", "3/8", "1/3", "5/16", "1/4",
		"3/16", "1/6", "1/8", "1/12", "1/16", "1/24", "1/32", "1/48", "1/64"
	};

	struct Ratio
	{
		int numerator, denominator;
	};

	static constexpr Ratio exactRatios[] {
		{1, 8}, {1, 6}, {1, 4}, {1, 3}, {1, 2}, {2, 3}, {1, 1}, {4, 3}, {2, 1}, {8, 3}, {3, 1}, {16, 5}, {4, 1},
		{16, 3}, {6, 1}, {8, 1}, {12, 1}, {16, 1}, {24, 1}, {32, 1}, {48, 1}, {64, 1}
	};

	using Osc = StatelessOscillator<fpoint>;

	Param<float> mix { "Mix", Range(0, 1) };
	Param<float> frequency { "Frequency", Range(0.05, 10, Range::Exp) };
	Param<bool> tempo { "Tempo" };
	Param<Rate> rate { "Rate", rateNames };
	Param<bool> symmetric { "Symmetric" };
	Param<bool> timeLocked { "Time locked" };
	Param<float> spread { "Spread", "degrees", Range(0, 360) };
	Param<Osc::Shape> shape { "Shape", Osc::ShapeNames };

	Tremolo()
	{
		tempo = true;
		symmetric = false;
		phase = 90;
		mix = 0.5f;
		spread = 90;
		frequency = 1;
	}

private:

	double phase = 0;
	
	static double ratioMultiply(Rate r, double input)
	{
		auto ratio = exactRatios[(int)r];
		return (input * ratio.numerator) / ratio.denominator;
	}

	void process(umatrix<const float> inputs, umatrix<float> outputs, size_t frames) override
	{
		const auto shared = sharedChannels();
		Osc::Shape waveShape = shape;

		auto position = getPlayHeadPosition();

		double fundamental;
		
		if(tempo){
			fundamental = ((position.bpm) / position.timeSigDenominator) / 60;
		}		
		else{
			fundamental = frequency;
		}
			
		if(timeLocked && position.isPlaying)
		{
			// Keep big exponents together, helps precision especially when keeping the math rational
			auto revolutions = fundamental * (ratioMultiply(rate, position.timeInSamples) / config().sampleRate);
			// do range reduction while in normalized frequency
			phase = (revolutions - (long long)revolutions);

		}
		
		double rotation = 0;
		
		rotation = ratioMultiply(rate, fundamental) / config().sampleRate;

		const auto offset = symmetric ? 0 : 0.5;
		const auto multiplier = symmetric ? 1 : 0.5;

		for (std::size_t n = 0; n < frames; ++n)
		{
			for (std::size_t c = 0; c < shared; ++c)
			{
				// phase lock oscillators with relationship
				const auto modulation = offset + multiplier * Osc::eval(phase + c * spread[n] / 360, waveShape);
				outputs[c][n] = mix[n] * inputs[c][n] * modulation + (1 - mix[n]) * inputs[c][n];
			}

			phase += rotation;
			phase -= (int)phase;

		}

		clear(outputs, shared);
	}
};