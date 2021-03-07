//
//  Echoing.hpp
//
//  Created by Luigi Felici on 2021-05-03
//  nusofting.com
//  Copyright 2021 Luigi Felici
//  
//  # Echoing is a stereo delay effect, compatible with
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
#include "DelayLin.hpp"

using namespace ape;

GlobalData(Echoing, "");

class Echoing : public Effect
{
public:

	Param<float>    LPHz{     	"LPHz", 	Range(500, 6500, Range::Exp) }; // input filter
	Param<float>    HPHz{ 		"HPHz", 	Range(20, 1900, Range::Exp) }; // input filter
	Param<float>    length{  	"length",   Range(0, 1) };
	Param<float>    spreadXch{  "spreadXch", Range(0, 0.5f) };
	Param<float>    fdbk{  		"repeat",   Range(0, 1) };
	Param<float>    wet{   		"dry/wet", 	Range(0, 1) };

	Echoing() {}

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
	std::vector<P1Filter> Smoothing;	 
	std::vector<DelayLin>  Lines;
	const float maxSamples = float(DelayLin::BUF_MASK);	

	void start(const IOConfig& cfg) override
	{ 
		// starting preset
		HPHz = 37.0f;
		LPHz = 4400.0f;
		length = 0.5f;
		spreadXch = 0.25f;
		fdbk = 0.87f;
		wet = 1.0f;

		HPfilter.resize(cfg.inputs);
		LPfilter.resize(cfg.inputs);
		DCfilter1.resize(cfg.inputs);
		Smoothing.resize(cfg.inputs);
		Lines.resize(cfg.inputs);
		for (std::size_t c = 0; c < cfg.inputs; ++c)
		{
			Lines[c].flush();
		}		
	}

	void process(umatrix<const float> inputs, umatrix<float> outputs, size_t frames) override
	{		
		const auto shared = sharedChannels();
		const float parWet = wet;		
		const float parLPHz = LPHz;
		const float parHPHz = HPHz;
		 	  float parlength = length*length;
		const float parspreadXch = spreadXch;
		const float parfdbk = fdbk*0.998f;		

		for (std::size_t c = 0; c < shared; ++c)
		{	
			const float sr = config().sampleRate;
			HPfilter[c].setFreq(parHPHz, sr);
			LPfilter[c].setFreq(parLPHz, sr);
			DCfilter1[c].setFreq(40.0f, sr);
			Smoothing[c].setFreq(2.0f, sr);			
		}
		
		for (std::size_t c = 0; c < shared; ++c)
		{		
			parlength += c&1 ? parspreadXch : 0.0f; // even channels offset
			parlength = std::clamp(parlength, 0.0f, 1.0f);
		
			for (std::size_t n = 0; n < frames; ++n)
			{
				const float out = Lines[c].readAt(maxSamples*Smoothing[c].filterLP(parlength)); 

				const float inS = inputs[c][n];
				const float inF = inS + parfdbk * HPfilter[c].filterHP(LPfilter[c].filterLP(out));

				Lines[c].writeSample(inF);

				outputs[c][n] = DCfilter1[c].filterHP(inF*parWet+(1.0f-parWet)*inS); 
			}
		} 

		clear(outputs, shared);
	}
};