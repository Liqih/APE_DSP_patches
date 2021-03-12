#include <generator.h>
#include "audioFilePlayer.hpp"

using namespace ape;

GlobalData(HitsPlaying, "");

class HitsPlaying : public Generator
{
public:

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

	HitsPlaying()
	{
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
	

	void setStereoGain(umatrix<float>& buffer, size_t frames, float gain)
	{
		for(size_t i = 0; i < frames; ++i)
		{
			buffer[0][i] *= gain;
			buffer[1][i] *= gain;
		}
	}	
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
	
	void start(const IOConfig& cfg) override
	{
	
		channel1.resize(cfg.maxBlockSize);
		channel2.resize(cfg.maxBlockSize);
	}

	void process(umatrix<float> buffer, size_t frames) override
	{	

		assert(buffer.channels() == 2);
		const float sampleRate = config().sampleRate;	
		
		
		if(files.empty())
			abort("no files to play");

		if(volume1 > 0.0f)
		{
		
			audioFilePlayer1.setSpeed(speed1);
			auto& file = files[(int)(File)fileParam1];		
			audioFilePlayer1.setFile(&file);
			audioFilePlayer1.blockPlayFile(channel1, frames, sampleRate, volume1);
			copyToStereo(channel1, buffer, frames);
		}
		
		if(volume2 > 0.0f)
		{
			audioFilePlayer2.setSpeed(speed2);
			auto& file = files[(int)(File)fileParam2];		
			audioFilePlayer2.setFile(&file);				
			audioFilePlayer2.blockPlayFile(channel2, frames, sampleRate, volume2);
			addToStereo(channel2, buffer, frames);
		}
	}
};