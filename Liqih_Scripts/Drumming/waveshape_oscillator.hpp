#include <generator.h>
#include <consts.h>

using namespace ape;

GlobalData(WaveshapeOscillator, "");

template<typename T>
struct StatelessOscillator
{
	enum class Shape
	{
		Sine,
		Triangle,
		SawDown,
		SawUp,
		Square,
		Pulse,
	};

	static constexpr typename Param<Shape>::Names ShapeNames = {
		"Sine", "Triangle", "SawDown", "SawUp", "Square", "Pulse"
	};

	static T eval(double unitPhase, Shape s)
	{
		T sample;

		// Do range reduction to simplify oscillators
		unitPhase = unitPhase - (long long)unitPhase;
		// calculate the sample here, depending on the type
		// note that all but the sine is mathematically ideal,
		// in a way that will alias extremely much.
		// but they are quick and demonstrates the use.
		// see the additive synthesizer for a correct way to do this
		switch (s)
		{
		case Shape::Sine: // sine
			sample = std::cos(consts<T>::tau * unitPhase);
			break;
		case Shape::Triangle: // triangle
			if (unitPhase < consts<T>::half)
				sample = -1 + 4 * unitPhase;
			else
				sample = 1 - 4 * (unitPhase - consts<T>::half);
			break;
		case Shape::SawDown: // sawtooth
			sample = 1 - unitPhase * 2;
			break;
		case Shape::SawUp: // sawtooth
			sample = -1 + unitPhase * 2;
			break;
		case Shape::Square: // square
			sample = unitPhase < consts<T>::half ? 1 : -1;
			break;
		case Shape::Pulse: // short positive pulse
			sample = unitPhase < 0.01? 1 : 0;
			break;
		}

		return sample;
	}
};

/// <summary>
/// Simple controller for <see cref="StatelessOscillator{T}"/>
/// </summary>
class WaveshapeOscillator : public Generator
{
public:

	using C = consts<fpoint>;
	using Osc = StatelessOscillator<fpoint>;
	static constexpr int kNumOscillators = 3;

	Param<float> frequency { "Frequency", "Hz", Range(1, 2e4, Range::Exp) };
	Param<bool> lfo { "/ 100" };

	WaveshapeOscillator()
	{
		frequency = 80;
		
		for(int i = 0; i < kNumOscillators; ++i)
		{
			osc[i].ratio = -12 + i * 12;
			osc[i].volume = -12;
			osc[i].shape = Osc::Shape::Sine;
		}
	}

private:

	struct State
	{
		Param<int> ratio { "Ratio", "st", Range(-12, 12) };
		Param<float> volume { "Volume", "dB", Range(-60, 6) };
		Param<Osc::Shape> shape { "Shape", Osc::ShapeNames };

		double phase = 0;
	};

	State osc[kNumOscillators];

	void process(umatrix<float> buffer, size_t frames) override
	{
		const auto twelve = C::one * 12;
		const auto reduction = lfo ? 100 : 1;

		const fpoint ratios[] = {
			std::pow(C::two, osc[0].ratio / twelve) / reduction,
			std::pow(C::two, osc[1].ratio / twelve) / reduction,
			std::pow(C::two, osc[2].ratio / twelve) / reduction
		};

		for (size_t n = 0; n < frames; ++n)
		{
			auto normalized = frequency[n] / config().sampleRate;

			fpoint sample = 0;
			
			for (size_t o = 0; o < kNumOscillators; ++o)
			{
				// dB per sample is very computationally heavy.
				const auto volume = dB::from(osc[o].volume[n]);
				sample += Osc::eval(osc[o].phase, osc[o].shape) * volume;

				osc[o].phase += normalized * ratios[o];
				osc[o].phase -= (int)osc[o].phase;
			}

			for (size_t c = 0; c < buffer.channels(); ++c)
			{
				buffer[c][n] = sample;
			}

		}
	}
};