#include <effect.h>
#include <consts.h>
#include "waveshape_oscillator.hpp"
#include "audioFilePlayer.hpp"
#include "audioBufferOps.hpp"

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

	enum class File
	{
		Kick,
		Snare,
		Hihat1,
		Hihat2
	};

	Param<Rate> rate1 { "Rate1", rateNames };
	Param<float> offset1{ "offset1" , Range(0, 45) };
	Param<File> fileParam1{ "File1", { "Kick", "Snare", "Hihat1", "Hihat2" } };
	Param<float> speed1{ "Speed1", Range(0.01, 10, Range::Exp) };
	Param<float> volume1{ "Volume1" };	

	Param<Rate> rate2 { "Rate2", rateNames };
	Param<float> offset2{ "offset2" , Range(0, 45) };
	Param<File> fileParam2{ "File2", { "Kick", "Snare", "Hihat1", "Hihat2" } };
	Param<float> speed2{ "Speed2", Range(0.01, 10, Range::Exp) };
	Param<float> volume2{ "Volume2" };
	
	MeteredValue left = MeteredValue("<");
	MeteredValue right = MeteredValue(">");

	Drumming()
	{
		phase1 = 90;
		phase2 = 90;
		rate1 = Rate::_1_4;
		rate2 = Rate::_1_6;

		volume1 = 0.4f;
		speed1 = 0.8f;	
		volume2 = 0.5f;
		speed2 = 1.2f;	
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
	std::vector<int> trigger2;

	double phase1 = 0;
	double phase2 = 0;
	bool run = false;

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
		trigger2.resize(cfg.maxBlockSize);
	}

	void process(umatrix<const float> inputs, umatrix<float> outputs, size_t frames) override
	{
		assert(outputs.channels() == 2);

		const auto shared = sharedChannels();
		const auto SR = config().sampleRate;

		auto position = getPlayHeadPosition();

		// The fundamental frequency of the project's "tempo"
		double fundamental = ((position.bpm) / position.timeSigDenominator) / 60;

		run = false;
		auto timeLocked = true;

		if(timeLocked && position.isPlaying)
		{
			auto revolutions1 = fundamental * (ratioMultiply(rate1, position.timeInSamples) / SR);
			// do range reduction while in normalized frequency
			phase1 = (revolutions1 - (long long)revolutions1) +  offset1;

			auto revolutions2 = fundamental * (ratioMultiply(rate2, position.timeInSamples) / SR);
			// do range reduction while in normalized frequency
			phase2 = (revolutions2 - (long long)revolutions2) +  offset2; 

			run = true;
		}		

		double rotation1 = ratioMultiply(rate1, fundamental) / SR;
		double rotation2 = ratioMultiply(rate2, fundamental) / SR;

		for (std::size_t n = 0; n < frames; ++n)
		{
			for (std::size_t c = 0; c < shared; ++c) // why both channels?
			{
				// phase lock oscillators with relationship)
				trigger1[n] = (run && Osc::eval(phase1, Osc::Shape::Pulse) > 0.99f) ? 1 : 0;
				trigger2[n] = (run && Osc::eval(phase2, Osc::Shape::Pulse) > 0.99f) ? 1 : 0;	
			}

			phase1 += rotation1;
			phase1 -= (int)phase1;

			phase2 += rotation2;
			phase2 -= (int)phase2;
		}

		audioFilePlayer1.setTriggers(&trigger1);
		audioFilePlayer2.setTriggers(&trigger2);					

		if(files.empty())
			abort("no files to play");

		if(volume1 > 0.0f)
		{
			audioFilePlayer1.setSpeed(speed1);
			auto& file = files[(int)(File)fileParam1];		
			audioFilePlayer1.setFile(&file);
			audioFilePlayer1.blockPlayFile(channel1, frames, SR, volume1);
			copyToStereo(channel1, outputs, frames);
		}		
		if(volume2 > 0.0f)
		{
			audioFilePlayer2.setSpeed(speed2);
			auto& file = files[(int)(File)fileParam2];		
			audioFilePlayer2.setFile(&file);				
			audioFilePlayer2.blockPlayFile(channel2, frames, SR, volume2);
			addToStereo(channel2, outputs, frames);
		}

		for (std::size_t n = 0; n < frames; ++n)
		{
			left = outputs[0][n];
			right = outputs[1][n];
		}

		//clear(outputs, shared);
	}
};