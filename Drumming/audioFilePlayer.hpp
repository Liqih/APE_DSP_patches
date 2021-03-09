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

	bool blockPlayFile(umatrix<float>& buffer, size_t frames, float sampleRate, float gain)
	{	
		if(!file)
			abort("selected file not valid");

		auto ratio = (fpoint)file->sampleRate() / sampleRate;	

		circular_signal<const float> signal = (*file)[0];

		auto interpolate = [&] (fpoint frac)
		{	
			return hermite4(frac, history[xm1], history[x0], history[x1], history[x2]);	
		};

		for(size_t i = 0; i < frames; ++i)
		{
			const auto x = static_cast<long long>(position);
			const auto frac = position - x;
			const auto offset = -2;

			for(size_t h = 0; h < 7; ++h) 
				history[h] = signal(x + h + offset);  // not all history[] used by hermite4()		

			buffer[0][i] = interpolate(frac)*gain;
			buffer[1][i] = interpolate(frac)*gain;

			position += ratio * speed;

			while(position >= file->samples()) // loop
				position -= file->samples();
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
	AudioFile* file = 0;
};