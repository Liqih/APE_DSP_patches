#include <vector>
using namespace ape;

static void copyToStereo(std::vector<float>& channelN, umatrix<float>& buffer, size_t frames)
{
	for(size_t i = 0; i < frames; ++i)
	{
		buffer[0][i] = channelN[i];
		buffer[1][i] = channelN[i];
	}
};

static void addToStereo(std::vector<float>& channelN, umatrix<float>& buffer, size_t frames)
{
	for(size_t i = 0; i < frames; ++i)
	{
		buffer[0][i] += channelN[i];
		buffer[1][i] += channelN[i];
	}
};