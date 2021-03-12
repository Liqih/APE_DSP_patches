#include <misc.h>

using namespace ape;

class AudioFilePlayer
{
public:

	AudioFilePlayer() {}

	void setFile(AudioFile* which)
	{	
		file = which;
	}
	void setSpeed( float value)
	{
		speed = value;	
	}
	void setLoop( bool value)
	{
		loop = value;	
	}
	void start()
	{
		position = 0;
	}

	void setTriggers(std::vector<int>* ptr)
	{
		trigger = ptr;
	}

	bool blockPlayFile(std::vector<float>& buffer, size_t frames, float sampleRate, float gain)
	{	
		if(!file)
			abort("selected file not valid");

		auto ratio = (fpoint)file->sampleRate() / sampleRate;	

		circular_signal<const float> signal = (*file)[0];

		auto interpolate = [&] (fpoint frac)
		{	
			return hermite4(frac, history[xm1], history[x0], history[x1], history[x2]);	
		};

		const bool ok = (trigger != 0) && (trigger->size() <= frames);

		for(size_t i = 0; i < frames; ++i)
		{			
			if(ok && trigger->at(i) == 1) position = 0;
			const auto x = static_cast<long long>(position);
			const auto frac = position - x;
			const auto offset = -2;

			for(size_t h = 0; h < 7; ++h) 
				history[h] = signal(x + h + offset);  // not all history[] used by hermite4()		

			if(position < file->samples())
			{
				buffer[i] = interpolate(frac)*gain;
			}
			else
			{
				buffer[i] = 0.0f; 					
			}

			position += ratio * speed;

			while(loop && position >= file->samples()) 
				position -= file->samples(); // loop

		}

		return true;
	}

private:

	enum H
	{
		xm2,
		xm1,
		x0,
		x1,
		x2,
		x3,
	};

	uint64_t counter = 0;
	fpoint position = 0;

	fpoint history[7] {};

	float speed = 1.0f;	
	bool loop = false;
	AudioFile* file = 0;

	std::vector<float> buffer;
	std::vector<int>* trigger = 0;
};