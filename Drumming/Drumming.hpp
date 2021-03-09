#include <effect.h>
#include <consts.h>
#include "../generators/waveshape_oscillator.hpp"
#include <vector>

using namespace ape;
GlobalData(Drumming, "");


class Drumming : public TransportEffect
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

	Param<bool> tempo { "Tempo" };
	Param<Rate> rate { "Rate", rateNames };
	Param<bool> timeLocked { "Time locked" };

	Drumming()
	{
		tempo = true;
		phase = 90;

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
		Osc::Shape waveShape = Osc::Shape::Square;

		auto position = getPlayHeadPosition();

		double fundamental;
		
		// The fundamental frequency of the project's "tempo"

		fundamental = ((position.bpm) / position.timeSigDenominator) / 60;


		if(timeLocked && position.isPlaying)
		{
			// Keep big exponents together, helps precision especially when keeping the math rational
			auto revolutions = fundamental * (ratioMultiply(rate, position.timeInSamples) / config().sampleRate);
			// do range reduction while in normalized frequency
			phase = (revolutions - (long long)revolutions);
		}
		
		double rotation = 0;
		
		rotation = ratioMultiply(rate, fundamental) / config().sampleRate;

		for (std::size_t n = 0; n < frames; ++n)
		{
			for (std::size_t c = 0; c < shared; ++c)
			{
				// phase lock oscillators with relationship
				const auto modulation = 1.0 +  Osc::eval(phase, waveShape);
				outputs[c][n] = inputs[c][n] * modulation;
			}

			phase += rotation;
			phase -= (int)phase;

		}

		clear(outputs, shared);
	}
};