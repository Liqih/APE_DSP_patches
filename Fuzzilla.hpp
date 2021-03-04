//
//  Fuzzilla.hpp
//
//  Created by Luigi Felici on 2021-28-02
//  nusofting.com
//  Copyright 2021 Luigi Felici
//  
//  # Fuzzilla is a 'heavy' waveshaping effect, compatible with
//  # Audio Programming Environment - Audio Plugin - v. 0.4.0.
//  
//
//  License:
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version. 


#include <effect.h>
#include <consts.h>

using namespace ape;

GlobalData(Fuzzilla, "");

class Fuzzilla : public Effect
{
public:

	Param<float>    LPHz{   "LPHz", Range(500, 4800, Range::Exp) }; // input filter
	Param<float>    HPHz{   "HPHz", Range(20, 1900, Range::Exp) }; // input filter
	Param<float>    threshold{ "threshold", Range(0, 1) }; // near zero waveshaping
	Param<float>    bias{  "bias",   Range(0, 1) };
	Param<float>    crack{ "crack",  Range(0, 1) };
	Param<float>    rect{  "rect",   Range(0, 1) };
	Param<float>    reso{  "reso",   Range(0, 1) };
	Param<float>    soft{  "soft",   Range(0, 1) };
	Param<bool>     halfThru{ "halfThru" };
	Param<float>    gain{  "gain" ,  Range(0, 1) };
	Param<float>    wet{   "dry/wet", Range(0, 1) };

	Fuzzilla() {}

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
		// starting preset
		HPHz = 37.0f;
		LPHz = 4400.0f;
		threshold = 0.05f;			
		bias = 0.1f;
		crack = 0.05f;
		rect = 1.0f;
		reso = 0.1f;
		soft = 0.5f;
		gain = 0.86f;
		wet = 0.5f;

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
		const float parcrack = 1.0f-crack;
		const float parrect = rect*0.5f+0.5f;
		const float parthreshold = 0.4f * threshold;
		const float parLPHz = LPHz;
		const float parHPHz = HPHz;
		const float parreso = reso*0.98f;
		const float parsoft = soft*0.98f;

		for (std::size_t c = 0; c < shared; ++c)
		{
			const float sr = config().sampleRate;
			HPfilter[c].setFreq(parHPHz, sr);
			LPfilter[c].setFreq(parLPHz, sr);
			DCfilter1[c].setFreq(40.0f, sr);
			DCfilter2[c].setFreq(40.0f, sr);
		}

		for (std::size_t c = 0; c < shared; ++c)
		{
			for (std::size_t n = 0; n < frames; ++n)
			{
				const float inS = inputs[c][n] - buffers[c]*parreso; // - feedback
				const float inF = HPfilter[c].filterHP(LPfilter[c].filterLP(std::clamp(inS, -1.0f, 1.0f)));
				const float inR = inF*(1.0f-parrect) + std::fabs(inF)*parrect; // blend

				float out = 0.0f;

				const float x = inR;
				if(x > 0.0f) // my custom waveshaper
				{
					const float x2 = x*x;
					const float x4 = x2*x2;
					out = std::tanh(-200.0f*x4*x+440.0f*x4-269.0f*x*x2+55.0f*x2-0.5f*x-parthreshold); 					
					out = out*(1.0f-parsoft)+(x-parthreshold)*parsoft;	// blend							
					if(out < 0.0f) halfThru? out = inS : out = 0.0f; 
				}

				buffers[c] = inR;

				const float inD = inR*parcrack + (1.0f-parcrack); // nasty
				outputs[c][n] = DCfilter2[c].filterHP(DCfilter1[c].filterHP( 
					std::tanh( inD*(out+parbias)*parVol*parWet+(1.0f-parWet)*inS ))); // maybe tanh is not needed here
			}
		} 

		clear(outputs, shared);
	}
};