#include <effect.h>
#include <consts.h>
#include "../generators/waveshape_oscillator.hpp"
#include "audioFilePlayer.hpp"
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

	Param<Rate> rate { "Rate", rateNames };

	enum class File
	{
		Kick,
		Snare,
		Hihat1,
		Hihat2
	};

	Param<File> fileParam1{ "File1", { "Kick", "Snare", "Hihat1", "Hihat2" } };
	Param<float> speed1{ "Speed1", Range(0.001, 10, Range::Exp) };
	Param<float> volume1{ "Volume1" };
	
	Param<File> fileParam2{ "File2", { "Kick", "Snare", "Hihat1", "Hihat2" } };
	Param<float> speed2{ "Speed2", Range(0.001, 10, Range::Exp) };
	Param<float> volume2{ "Volume2" };
	
	Param<bool> trigger { "Trigger" };
	


	Drumming()
	{
		phase = 90;
		//rate = _1_4;

		volume1 = 0.5f;
		speed1 = 1.0f;	
		volume2 = 0.5f;
		speed2 = 1.0f;	

	}

private:

	//The files must exist in the same folder fo this C++ file
	std::vector<AudioFile> files {		
		"KICK 1 CLOSE.wav",
		"SNARE 2 CLOSE.wav",
		"CLOSED HAT 4 CLOSE.wav",
		"OPEN HAT 1 CLOSE.wav"
	};

	AudioFilePlayer audioFilePlayer1;
	AudioFilePlayer audioFilePlayer2;
	AudioFilePlayer audioFilePlayer3;
	AudioFilePlayer audioFilePlayer4;
	
	std::vector<float> channel1;
	std::vector<float> channel2;

	std::vector<int> trigger1;
	

	void copyToStereo(std::vector<float>& channelN, umatrix<float>& buffer, size_t frames)
	{
		for(size_t i = 0; i < frames; ++i)
		{
			buffer[0][i] = channelN[i];
			buffer[1][i] = channelN[i];
		}
	}
	void addToStereo(std::vector<float>& channelN, umatrix<float>& buffer, size_t frames)
	{
		for(size_t i = 0; i < frames; ++i)
		{
			buffer[0][i] += channelN[i];
			buffer[1][i] += channelN[i];
		}
	}

	double phase = 0;
	
	static double ratioMultiply(Rate r, double input)
	{
		auto ratio = exactRatios[(int)r];
		return (input * ratio.numerator) / ratio.denominator;
	}

	void start(const IOConfig& cfg) override
	{
	
		channel1.resize(cfg.maxBlockSize);
		channel2.resize(cfg.maxBlockSize);
		trigger1.resize(cfg.maxBlockSize);
	}

	void process(umatrix<const float> inputs, umatrix<float> outputs, size_t frames) override
	{
		const auto shared = sharedChannels();
	
		auto position = getPlayHeadPosition();

		double fundamental;
		
		// The fundamental frequency of the project's "tempo"

		fundamental = ((position.bpm) / position.timeSigDenominator) / 60;

		auto timeLocked = true;

		if(timeLocked && position.isPlaying)
		{
			// Keep big exponents together, helps precision especially when keeping the math rational
			auto revolutions = fundamental * (ratioMultiply(rate, position.timeInSamples) / config().sampleRate);
			// do range reduction while in normalized frequency
			phase = (revolutions - (long long)revolutions);
		}
		
		const auto trig = trigger.changed() ? 1.0f : 0.0f;
		
		double rotation = 0;
		
		rotation = ratioMultiply(rate, fundamental) / config().sampleRate;

		for (std::size_t n = 0; n < frames; ++n)
		{
			for (std::size_t c = 0; c < shared; ++c)
			{
				// phase lock oscillators with relationship
				if(Osc::eval(phase, Osc::Shape::SawDown) > 0.99f) 
				{
					trigger1[n] = 1;
				}
				else
					trigger1[n] = 0;
				
			}

			phase += rotation;
			phase -= (int)phase;

		}
				
		
		
		if(trig) audioFilePlayer1.start();

		audioFilePlayer1.setTriggers(&trigger1);

	
		assert(outputs.channels() == 2);
		const float sampleRate = config().sampleRate;			
		
		if(files.empty())
			abort("no files to play");

		if(volume1 > 0.0f)
		{
		
			audioFilePlayer1.setSpeed(speed1);
			auto& file = files[(int)(File)fileParam1];		
			audioFilePlayer1.setFile(&file);
			audioFilePlayer1.blockPlayFile(channel1, frames, sampleRate, volume1);
			copyToStereo(channel1, outputs, frames);
		}
		
		if(volume2 > 0.0f)
		{
			audioFilePlayer2.setSpeed(speed2);
			auto& file = files[(int)(File)fileParam2];		
			audioFilePlayer2.setFile(&file);				
			audioFilePlayer2.blockPlayFile(channel2, frames, sampleRate, volume2);
			addToStereo(channel2, outputs, frames);
		}

		//clear(outputs, shared);
	}
};