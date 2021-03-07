//
//  DelayLin.hpp
//
//  Created by Luigi Felici on 2021-05-03
//  nusofting.com
//  Copyright 2021 Luigi Felici
//  
//
//  License:
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version. 


class DelayLin
{
public:
	DelayLin() { flush(); }

	void flush() 
	{  
		for (int i = 0; i < BUF_SIZE; i++)	
		{
			buffer[i] = 0.0f;
		}
	}		

	float readAt(double samples)
	{
		int	iIndex = int(samples);
		const double fIndex = samples-iIndex;


		iIndex = write - iIndex;
		if (iIndex < 0) iIndex += BUF_SIZE;// wrap
		assert(iIndex >= 0);

		float	x0 = buffer[iIndex & BUF_MASK];
		float	x1 = buffer[(iIndex + 1) & BUF_MASK];

		return float(x0 + (x1 - x0) * fIndex);

	}	
	void writeSample(float input)
	{
		buffer[write] = input; 		
		if( ++write > BUF_MASK )  write = 0; 	
	}

	enum { BUF_SIZE = 4096*4, BUF_MASK = BUF_SIZE -1 };


private:
	float buffer[BUF_SIZE];			
	int write = 0;	
};