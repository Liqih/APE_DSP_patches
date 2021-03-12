#include <effect.h>
#include <consts.h>

using namespace ape;

GlobalData(Kazootronica, "");

class Kazootronica : public Effect
{
public:



	Param<float>    LPHz{   "LPHz", Range(500, 4000) };
	Param<float>    gate0{ "gate0", Range(0, 0.35) };
	Param<float>    bias{  "bias",  Range(0, 1) };
	Param<float>    harsh{ "harsh", Range(0, 1) };
	Param<float>    rect{  "rect",  Range(0, 1) };
	Param<float>   gain{   "gain" , Range(0, 1) };
	Param<float>    wet{   "dry/wet", Range(0, 1) };

	Kazootronica() {}

private:

	class P1Filter
	{
	public:
		P1Filter() { state = 0; }
		
		void flush() { state = 0; }		

		void setFreq(float fHz, float sr)
		{
			if(sr < 22050.0f) sr = 22050.0f;
			b1 = std::exp(-consts<float>::tau * fHz / sr);
			a0 = 1 - b1;
		}	
		float filterLP(float sample)
		{
			state = sample * a0 + state * b1;
			return	state;
		}
		float filterHP(float sample)
		{
			state = sample * a0 + state * b1;
			return	sample - state;
		}

	private:
		float state;
		float a0, b1;	
	};

	std::vector<P1Filter>  HPfilter;
	std::vector<P1Filter>  LPfilter;
	std::vector<P1Filter>  DCfilter1;
	std::vector<P1Filter>  DCfilter2;
	std::vector<float>    buffers;

	void start(const IOConfig& cfg) override
	{ 
		LPHz = 1200.0f;
		gate0 = 0.05f;	
		gain = 0.86f;
		bias = 0.0f;
		harsh = 0.05;
		rect = 1.0f;
		wet = 1.0f;

		HPfilter.resize(cfg.inputs);
		LPfilter.resize(cfg.inputs);
		DCfilter1.resize(cfg.inputs);
		DCfilter2.resize(cfg.inputs);
		buffers.resize(cfg.inputs);
	}

	void process(umatrix<const float> inputs, umatrix<float> outputs, size_t frames) override
	{
		const auto shared = sharedChannels();
		const float parVol = gain*gain*10.0f;
		const float parWet = wet;
		const float parbias = -bias;
		const float parharsh = 1.0f-harsh;
		const float parrect = rect*0.5f+0.5f;
		const float pargate0 = gate0*0.999f+0.001f;
		const float parLPHz = LPHz;
		
		for (std::size_t c = 0; c < shared; ++c)
		{
        	const float sr = config().sampleRate;
			HPfilter[c].setFreq(82.0f, sr);
			LPfilter[c].setFreq(parLPHz, sr);
			DCfilter1[c].setFreq(40.0f, sr);
			DCfilter2[c].setFreq(40.0f, sr);
		}

		for (std::size_t c = 0; c < shared; ++c)
		{
			for (std::size_t n = 0; n < frames; ++n)
			{
				const float inS = inputs[c][n] + buffers[c]*0.1f;
				const float inF = HPfilter[c].filterHP(LPfilter[c].filterLP(std::clamp(inS, -1.0f, 1.0f)));
				const float inR = inF*(1.0f-parrect) + std::fabs(inF)*parrect;

				float out = 0.0f;
				if(inR  > 0.8f ) out = 1.0f;
				else if (inR <= 0.8f && inR > 0.4f) out = 0.8f;
				else if (inR <= 0.4f && inR > pargate0) out = 0.4f;
				
				buffers[c] = inR;
				
				const float inD = inR*parharsh + (1.0f-parharsh);
				outputs[c][n] = DCfilter2[c].filterHP(DCfilter1[c].filterHP(
				std::tanh( inD*(out+parbias)*parVol*parWet+(1.0f-parWet)*inS )));
			}
		} 

		clear(outputs, shared);
	}


};