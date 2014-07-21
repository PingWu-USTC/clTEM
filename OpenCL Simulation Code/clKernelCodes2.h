#pragma once

const char* InitialiseWavefunctionSource = 
"__kernel void clInitialiseWavefunction(__global float2* InputWavefunction, int width, int height, float value) \n"
"{		\n"
"	int xid = get_global_id(0);	\n"
"	int yid = get_global_id(1);	\n"
"	if(xid < width && yid < height) \n"
"	{	\n"
"		int Index = xid + width*yid; \n"
"		InputWavefunction[Index].x = value; \n"
"		InputWavefunction[Index].y = 0; \n"
"	}	\n"
"}		\n"
;

// Slices have to start from 0
// Tried to fix so model doesnt have to have min at (0,0,0)
// Uses normal potential sliced many times not projected.
// Includes atoms onmultiple slices where they contribute
// Could be alot faster will try other methods like one kernel for each atom type with pre tabulated potentials.
// z is height of top of slice...
const char* BinnedAtomicPotentialSource = 
"__kernel void clBinnedAtomicPotential(__global float2* Potential, __global float* clAtomXPos, __global float* clAtomYPos, __global float* clAtomZPos, __global int* clAtomZNum, __constant float* clfParams, __global const int* clBlockStartPositions, int width, int height, int slice, int slices, float z, float dz, float pixelscale, int xBlocks, int yBlocks, float MaxX, float MinX, float MaxY, float MinY, int loadBlocksX, int loadBlocksY, int loadSlicesZ, float sigma) \n"
"{		\n"
"	int xid = get_global_id(0);	\n"
"	int yid = get_global_id(1);	\n"
"	if(xid < width && yid < height) \n"
"	{	\n"
"		int Index = xid + width*yid; \n"
"		int topz = slice - loadSlicesZ; \n"
"		int bottomz = slice + loadSlicesZ; \n"
"		float sumz = 0.0f; \n"
"		if(topz < 0 ) \n"
"			topz = 0; \n"
"		if(bottomz >= slices ) \n"
"			bottomz = slices-1; \n"
"		for(int k = topz; k <= bottomz; k++) \n"
"		{ \n"
"			for (int j = floor((get_group_id(1) * get_local_size(1) * yBlocks * pixelscale/ ( MaxY-MinY )) - loadBlocksY ); j <= ceil(((get_group_id(1)+1) * get_local_size(1) * yBlocks * pixelscale/ ( MaxY - MinY )) + loadBlocksY); j++) \n"
"			{ \n"
"				for (int i = floor((get_group_id(0) * get_local_size(0) * xBlocks * pixelscale / (MaxX-MinX )) - loadBlocksX ); i <= ceil(((get_group_id(0)+1) * get_local_size(0) * xBlocks * pixelscale/ ( MaxX - MinX )) + loadBlocksX); i++) \n"
"				{ \n"
"				// Check bounds to avoid unneccessarily loading blocks when i am at edge of sample. \n"
"					if(0 <= j && j < yBlocks) \n"
"					{ \n"
"						// Check bounds to avoid unneccessarily loading blocks when i am at edge of sample. \n"
"						if (0 <= i && i < xBlocks ) \n"
"						{ \n"
"							// Check if there is an atom in bin, arrays are not overwritten when there are no extra atoms so if you don't check could add contribution more than once. \n"
"							for (int l = clBlockStartPositions[k*xBlocks*yBlocks + xBlocks*j + i]; l < clBlockStartPositions[k*xBlocks*yBlocks + xBlocks*j + i+1]; l++) \n"
"							{ \n"
"								for (int h = 0; h < 10; h++) \n"
"								{ \n"
"									float rad = sqrt((xid*pixelscale-clAtomXPos[l] + MinX)*(xid*pixelscale-clAtomXPos[l] + MinX) + (yid*pixelscale-clAtomYPos[l] + MinY)*(yid*pixelscale-clAtomYPos[l] + MinY) + (z - (h*(dz/10.0f))-clAtomZPos[l])*(z - (h*(dz/10.0f))-clAtomZPos[l])); \n"
"									int ZNum = clAtomZNum[l]; \n"
"									if( rad < sqrt(5.0f)) // Should also make sure is not too small \n"
"									{ \n"
"										sumz += (150.4121417f * (1.0f/rad) * clfParams[(ZNum-1)*12]* exp( -2.0f*3.141592f*rad*sqrt(clfParams[(ZNum-1)*12+1]))); \n"
"										sumz += (150.4121417f * (1.0f/rad) * clfParams[(ZNum-1)*12+2]* exp( -2.0f*3.141592f*rad*sqrt(clfParams[(ZNum-1)*12+2+1]))); \n"
"										sumz += (150.4121417f * (1.0f/rad) * clfParams[(ZNum-1)*12+4]* exp( -2.0f*3.141592f*rad*sqrt(clfParams[(ZNum-1)*12+4+1]))); \n"
"										sumz += (266.5157269f * clfParams[(ZNum-1)*12+6] * exp (-3.141592f*rad*3.141592f*rad/clfParams[(ZNum-1)*12+6+1]) * sqrt(clfParams[(ZNum-1)*12+6+1])/(clfParams[(ZNum-1)*12+6+1]*clfParams[(ZNum-1)*12+6+1]*clfParams[(ZNum-1)*12+6+1])); \n"
"										sumz += (266.5157269f * clfParams[(ZNum-1)*12+8] * exp (-3.141592f*rad*3.141592f*rad/clfParams[(ZNum-1)*12+8+1]) * sqrt(clfParams[(ZNum-1)*12+8+1])/(clfParams[(ZNum-1)*12+8+1]*clfParams[(ZNum-1)*12+8+1]*clfParams[(ZNum-1)*12+8+1])); \n"
"										sumz += (266.5157269f * clfParams[(ZNum-1)*12+10] * exp (-3.141592f*rad*3.141592f*rad/clfParams[(ZNum-1)*12+10+1]) * sqrt(clfParams[(ZNum-1)*12+10+1])/(clfParams[(ZNum-1)*12+10+1]*clfParams[(ZNum-1)*12+10+1]*clfParams[(ZNum-1)*12+10+1])); \n"
"									} \n"
"								} \n"
"							} \n"
"						} \n"
"					} \n"
"				} \n"
"			} \n"
"		} \n"
"		Potential[Index].x = cos(sigma*sumz*dz/10.0f); \n"
"		Potential[Index].y = sin(sigma*sumz*dz/10.0f); \n"
"	}	\n"
"}	\n"
;

const char* BinnedAtomicPotentialSource2 = 
"__kernel void clBinnedAtomicPotential(__global float2* Potential, __global float* clAtomXPos, __global float* clAtomYPos, __global float* clAtomZPos, __global int* clAtomZNum, __global float* clfParams, __local float* clfParamsl, __constant int* clBlockStartPositions, int width, int height, int slice, int slices, float z, float dz, float pixelscale, int xBlocks, int yBlocks, float MaxX, float MinX, float MaxY, float MinY, int loadBlocksX, int loadBlocksY, int loadSlicesZ, float sigma) \n"
"{		\n"
"	int xid = get_global_id(0);	\n"
"	int yid = get_global_id(1);	\n"
"	if(xid < width && yid < height) \n"
"	{	\n"
"		int Index = xid + width*yid; \n"
"		int topz = slice - loadSlicesZ; \n"
"		int bottomz = slice + loadSlicesZ; \n"
"		float sumz = 0.0f; \n"
"		if(topz < 0 ) \n"
"			topz = 0; \n"
"		if(bottomz >= slices ) \n"
"			bottomz = slices-1; \n"
"		for(int k = topz; k <= bottomz; k++) \n"
"		{ \n"
"			for (int j = floor((get_group_id(1) * get_local_size(1) * yBlocks * pixelscale/ ( MaxY-MinY )) - loadBlocksY ); j <= ceil(((get_group_id(1)+1) * get_local_size(1) * yBlocks * pixelscale/ ( MaxY - MinY )) + loadBlocksY); j++) \n"
"			{ \n"
"				for (int i = floor((get_group_id(0) * get_local_size(0) * xBlocks * pixelscale / (MaxX-MinX )) - loadBlocksX ); i <= ceil(((get_group_id(0)+1) * get_local_size(0) * xBlocks * pixelscale/ ( MaxX - MinX )) + loadBlocksX); i++) \n"
"				{ \n"
"				// Check bounds to avoid unneccessarily loading blocks when i am at edge of sample. \n"
"					if(0 <= j && j < yBlocks) \n"
"					{ \n"
"						// Check bounds to avoid unneccessarily loading blocks when i am at edge of sample. \n"
"						if (0 <= i && i < xBlocks ) \n"
"						{ \n"
"							// Check if there is an atom in bin, arrays are not overwritten when there are no extra atoms so if you don't check could add contribution more than once. \n"
"							for (int l = clBlockStartPositions[k*xBlocks*yBlocks + xBlocks*j + i]; l < clBlockStartPositions[k*xBlocks*yBlocks + xBlocks*j + i+1]; l++) \n"
"							{ \n"
"								for (int h = 0; h < 10; h++) \n"
"								{ \n"
"									float rad = sqrt((xid*pixelscale-clAtomXPos[l] + MinX)*(xid*pixelscale-clAtomXPos[l] + MinX) + (yid*pixelscale-clAtomYPos[l] + MinY)*(yid*pixelscale-clAtomYPos[l] + MinY) + (z - (h*(dz/10.0f))-clAtomZPos[l])*(z - (h*(dz/10.0f))-clAtomZPos[l])); \n"
"									int ZNum = clAtomZNum[l]; \n"
"									if( rad < sqrt(5.0f)) // Should also make sure is not too small \n"
"									{ \n"
"										sumz += (150.4121417f * (1.0f/rad) * clfParams[(ZNum-1)*12]* exp( -2.0f*3.141592f*rad*sqrt(clfParams[(ZNum-1)*12+1]))); \n"
"										sumz += (150.4121417f * (1.0f/rad) * clfParams[(ZNum-1)*12+2]* exp( -2.0f*3.141592f*rad*sqrt(clfParams[(ZNum-1)*12+2+1]))); \n"
"										sumz += (150.4121417f * (1.0f/rad) * clfParams[(ZNum-1)*12+4]* exp( -2.0f*3.141592f*rad*sqrt(clfParams[(ZNum-1)*12+4+1]))); \n"
"										sumz += (266.5157269f * clfParams[(ZNum-1)*12+6] * exp (-3.141592f*rad*3.141592f*rad/clfParams[(ZNum-1)*12+6+1]) * sqrt(clfParams[(ZNum-1)*12+6+1])/(clfParams[(ZNum-1)*12+6+1]*clfParams[(ZNum-1)*12+6+1]*clfParams[(ZNum-1)*12+6+1])); \n"
"										sumz += (266.5157269f * clfParams[(ZNum-1)*12+8] * exp (-3.141592f*rad*3.141592f*rad/clfParams[(ZNum-1)*12+8+1]) * sqrt(clfParams[(ZNum-1)*12+8+1])/(clfParams[(ZNum-1)*12+8+1]*clfParams[(ZNum-1)*12+8+1]*clfParams[(ZNum-1)*12+8+1])); \n"
"										sumz += (266.5157269f * clfParams[(ZNum-1)*12+10] * exp (-3.141592f*rad*3.141592f*rad/clfParams[(ZNum-1)*12+10+1]) * sqrt(clfParams[(ZNum-1)*12+10+1])/(clfParams[(ZNum-1)*12+10+1]*clfParams[(ZNum-1)*12+10+1]*clfParams[(ZNum-1)*12+10+1])); \n"
"									} \n"
"								} \n"
"							} \n"
"						} \n"
"					} \n"
"				} \n"
"			} \n"
"		} \n"
"		Potential[Index].x = cos(sigma*sumz*dz/10.0f); \n"
"		Potential[Index].y = sin(sigma*sumz*dz/10.0f); \n"
"	}	\n"
"}	\n"
;

const char* BandLimitSource = 
"__kernel void clBandLimit(__global float2* InputWavefunction, int width, int height, float kmax, __global float* kx, __global float* ky) \n"
"{		\n"
"	int xid = get_global_id(0);	\n"
"	int yid = get_global_id(1);	\n"
"	if(xid < width && yid < height) \n"
"	{	\n"
"		int Index = xid + width*yid; \n"
"		float k = hypot(kx[xid],ky[yid]); \n"
"		InputWavefunction[Index].x *= (k<=kmax); \n"
"		InputWavefunction[Index].y *= (k<=kmax); \n"
"	}	\n"
"}		\n"
;

const char* fftShiftSource = 
"__kernel void clfftShift(__global const float2* Input, __global float2* Output, int width, int height) \n"
"{        \n"
"    //Get the work items ID \n"
"    int xid = get_global_id(0);    \n"
"    int yid = get_global_id(1); \n"
"    if(xid < width && yid < height) \n"
"    {    \n"
"        int Index = xid + yid*width; \n"
"        int Yshift = width*height/2; \n"
"        int Xshift = width/2; \n"
"        int Xmid = width/2; \n"
"        int Ymid = height/2; \n"
"        if( xid < Xmid && yid < Ymid ) \n"
"        { \n"
"            Output[Index+Yshift+Xshift].x = Input[Index].x; \n"
"            Output[Index+Yshift+Xshift].y = Input[Index].y; \n"    
"        } \n"
"        else if( xid >= Xmid && yid < Ymid ) \n"
"        { \n"
"            Output[Index+Yshift-Xshift].x = Input[Index].x; \n"
"            Output[Index+Yshift-Xshift].y = Input[Index].y; \n"    
"        } \n"
"        else if( xid < Xmid && yid >= Ymid ) \n"
"        { \n"
"            Output[Index-Yshift+Xshift].x = Input[Index].x; \n"
"            Output[Index-Yshift+Xshift].y = Input[Index].y; \n"    
"        } \n"
"        else if( xid >= Xmid && yid >= Ymid ) \n"
"        { \n"
"            Output[Index-Yshift-Xshift].x = Input[Index].x; \n"
"            Output[Index-Yshift-Xshift].y = Input[Index].y; \n"    
"        } \n"    
"    }    \n"
"}    \n"
;

const char* imagingKernelSource = 
"__kernel void clImagingKernel(__global const float2* Input, __global float2* Output, int width, int height, float Cs, float df, float a2, float a2phi, float a3, float a3phi, float objap, float wavel, __global float* clXFrequencies, __global float* clYFrequencies, float beta, float delta) \n"
"{        \n"
"    //Get the work items ID \n"
"    int xid = get_global_id(0); \n"
"    int yid = get_global_id(1); \n"
"    if(xid < width && yid < height) \n"
"    {  \n"
"       int Index = xid + yid*width; \n"
"		float objap2 = (((objap * 0.001f) / wavel ) * (( objap * 0.001f ) / wavel )); \n"
"		float Chi1 = 3.14159f * wavel; \n"
"		float Chi2 = 0.5f * Cs * wavel * wavel; \n"
"		float k2 = (clXFrequencies[xid]*clXFrequencies[xid]) + (clYFrequencies[yid]*clYFrequencies[yid]); \n"
"		float k = sqrt(k2); \n"
"		float factor = 1.0f*beta*beta/(4*wavel*wavel); \n"
"		float ecohs = exp(-factor*pow(3.14159f*k*wavel*2*df + 2*3.14159f*wavel*wavel*wavel*Cs*k2*k,2)); \n"
"		float ecohd = exp(-0.25f*delta*delta*3.14159f*3.14159f*k2*k2*wavel*wavel); \n"
"		if ( k2 < objap2){ \n"
"			float Phi = atan2(clYFrequencies[yid],clXFrequencies[xid]); \n"
"			float Chi = Chi1 * k2 * ( Chi2 * k2 + df + a2 * sin ( ( 2.0f * ( Phi - a2phi ) ) ) + 2.0f * a3 * wavel * sqrt ( k2 ) * sin ( ( 3.0f * ( Phi - a3phi ) ) ) / 3.0f ); \n"
"			Output[Index].x = ecohs*ecohd*(Input[Index].x *  cos ( Chi )  + Input[Index].y * sin ( Chi )) ; \n"
"			Output[Index].y	= ecohs*ecohd*(Input[Index].x * -1 *  sin ( Chi ) + Input[Index].y * cos ( Chi )); \n"
"		} else { \n"
"		Output[Index].x = 0.0f; \n"
"		Output[Index].y = 0.0f; \n"
"		} \n"
"	} \n"
"} \n"
;

const char* InitialiseSTEMWavefunctionSource = 
"__kernel void clInitialiseSTEMWavefunction(__global float2* Output, int width, int height, __global const float* clXFrequencies, __global const float* clYFrequencies, float posx, float posy, float apert, float pixelscale, float df, float Cs, float wavel) \n"
"{ \n"
"	//Get the work items ID \n"
"	int xid = get_global_id(0); \n"
"	int yid = get_global_id(1); \n"
"	if(xid < width && yid < height) \n"
"	{ \n"
"		int Index = xid + yid*width; \n"
"		float apert2 = ((apert * 0.001f) / wavel ); \n"
"		float k0x = clXFrequencies[xid]; \n"
"		float k0y = clYFrequencies[yid]; \n"
"		float k = sqrt(k0x*k0x + k0y*k0y); \n"
"		float Pi = 3.14159265f; \n"
"		if( k < apert2) \n"
"		{ \n"
"			Output[Index].x = cos(Pi*wavel*k*k*(Cs*wavel*wavel*k*k*0.5f + df))*cos(2*Pi*(k0x*posx*pixelscale + k0y*posy*pixelscale))  + sin(Pi*wavel*k*k*(Cs*wavel*wavel*k*k*0.5f + df))*sin(2*Pi*(k0x*posx*pixelscale + k0y*posy*pixelscale)) ; \n"
"			Output[Index].y = -cos(2*Pi*(k0x*posx*pixelscale + k0y*posy*pixelscale))*sin(Pi*wavel*k*k*(Cs*wavel*wavel*k*k*0.5f + df)) + cos(Pi*wavel*k*k*(Cs*wavel*wavel*k*k*0.5f + df))*sin(2*Pi*(k0x*posx*pixelscale + k0y*posy*pixelscale)); \n"
"		} \n"
"		else \n"
"		{ \n"
"			Output[Index].x 	= 0.0f; \n"
"			Output[Index].y 	= 0.0f; \n"
"		} \n"
"	} \n"
"} \n"
;

const char* sumReductionsource2 = 
"__kernel void clSumReduction(__global const float2* input, __global float2* output, const unsigned int size, __local float2* buffer)	\n"
"{																																		\n"
"	//Get the work items ID																												\n"
"	size_t idx = get_local_id(0);																										\n"
"	size_t stride = get_global_size(0);																									\n"
"	buffer[idx] = 0;																													\n"
"																																		\n"
"	for(size_t pos = get_global_id(0); pos < size; pos += stride )																		\n"
"		buffer[idx] += input[pos];																										\n"
"																																		\n"
"	barrier(CLK_LOCAL_MEM_FENCE);																										\n"
"																																		\n"
"	float sum = 0;																														\n"
"	if(!idx) {																															\n"
"		for(size_t i = 1; i < get_local_size(0); ++i)																					\n"
"			sum += sqrt(buffer[i].x*buffer[i].x + buffer[i].y*buffer[i].y);																											\n"
"																																		\n"
"		output[get_group_id(0)].x = sum;																								\n"
"		output[get_group_id(0)].y = 0.0f;																								\n"
"	}																																	\n"
"}																																		\n"
;

const char* floatSumReductionsource2 = 
"__kernel void clFloatSumReduction(__global const float* input, __global float* output, const unsigned int size, __local float* buffer)	\n"
"{																																		\n"
"	//Get the work items ID																												\n"
"	size_t idx = get_local_id(0);																										\n"
"	size_t stride = get_global_size(0);																									\n"
"	buffer[idx] = 0;																													\n"
"																																		\n"
"	for(size_t pos = get_global_id(0); pos < size; pos += stride )																		\n"
"		buffer[idx] += input[pos];																										\n"
"																																		\n"
"	barrier(CLK_LOCAL_MEM_FENCE);																										\n"
"																																		\n"
"	float sum = 0;																														\n"
"	if(!idx) {																															\n"
"		for(size_t i = 1; i < get_local_size(0); ++i)																					\n"
"			sum += buffer[i];																											\n"
"																																		\n"
"		output[get_group_id(0)] = sum;																								\n"
"	}																																	\n"
"}																																		\n"
;

const char* abssource2 = 
"__kernel void clAbs(__global float2* clEW, int sizeX, int sizeY)	\n"
"{	\n"
"	//Get the work items ID \n"
"	int xid = get_global_id(0);	\n"
"	int yid = get_global_id(1); \n"
"	\n"
"	if(xid<sizeX&&yid<sizeY) \n"
"	{	\n"
"		int Index = xid + yid*sizeX; \n"
"		float real = clEW[Index].x;	\n"
"		float imag = clEW[Index].y;	\n"
"		clEW[Index].x = hypot(real,imag)*hypot(real,imag);	\n"
"		clEW[Index].y = 0;	\n"
"	}	\n"
"}	\n"
;

const char* multiplySource = 
"__kernel void clMultiply(__global float2* Input, float factor, int sizeX, int sizeY)	\n"
"{	\n"
"	//Get the work items ID \n"
"	int xid = get_global_id(0);	\n"
"	int yid = get_global_id(1); \n"
"	\n"
"	if(xid<sizeX&&yid<sizeY) \n"
"	{	\n"
"		int Index = xid + yid*sizeX; \n"
"		Input[Index].x *= factor; \n"
"		Input[Index].y *= factor; \n"
"	}	\n"
"}	\n"
;

const char* bandPassSource = 
"__kernel void clBandPass(__global float2* Output, __global const float2* Input, int width, int height, float inner, float outer)	\n"
"{	\n"
"	//Get the work items ID \n"
"	int xid = get_global_id(0);	\n"
"	int yid = get_global_id(1); \n"
"	\n"
"	if(xid<width && yid<height) \n"
"	{	\n"
"		int Index = xid + yid*width; \n"
"		float centX = width/2; \n"
"		float centY = height/2; \n"
"		float radius = sqrt((xid-centX)*(xid-centX)+(yid-centY)*(yid-centY)); \n" // hypot?
"		if(radius < outer && radius > inner) \n"
"		{	\n"
"			Output[Index].x = Input[Index].x; \n"
"			Output[Index].y = Input[Index].y; \n"	
"		} \n"
"		else \n"
"		{	\n"
"			Output[Index].x = 0; \n"
"			Output[Index].y = 0; \n"	
"		} \n"
"	}	\n"
"}	\n"
;

const char* floatbandPassSource = 
"__kernel void clFloatBandPass(__global float* Output, __global const float* Input, int width, int height, float inner, float outer)	\n"
"{	\n"
"	//Get the work items ID \n"
"	int xid = get_global_id(0);	\n"
"	int yid = get_global_id(1); \n"
"	\n"
"	if(xid<width && yid<height) \n"
"	{	\n"
"		int Index = xid + yid*width; \n"
"		float centX = width/2; \n"
"		float centY = height/2; \n"
"		float radius = sqrt((xid-centX)*(xid-centX)+(yid-centY)*(yid-centY)); \n" // hypot?
"		if(radius < outer && radius > inner) \n"
"		{	\n"
"			Output[Index] = Input[Index];\n"
"		} \n"
"		else \n"
"		{	\n"
"			Output[Index] = 0; \n"	
"		} \n"
"	}	\n"
"}	\n"
;

const char* SqAbsSource = 
"__kernel void clSqAbs(__global const float2* clIm, __global float2* clAbsSq, int sizeX, int sizeY)	\n"
"{	\n"
"	//Get the work items ID \n"
"	int xid = get_global_id(0);	\n"
"	int yid = get_global_id(1); \n"
"	\n"
"	if(xid<sizeX&&yid<sizeY) \n"
"	{	\n"
"		int Index = xid + yid*sizeX; \n"
"		float real = clIm[Index].x;	\n"
"		float imag = clIm[Index].y;	\n"
"		clAbsSq[Index].x = real*real + imag*imag;	\n"
"		clAbsSq[Index].y = 0;	\n"
"	}	\n"
"}	\n"
;

const char* DQESource = 
"__kernel void clDQE(__global const float2* clIm, __global float* DQE, int sizeX, int sizeY, int binning)	\n"
"{	\n"
"	//Get the work items ID \n"
"	int xid = get_global_id(0);	\n"
"	int yid = get_global_id(1); \n"
"	\n"
"	if(xid<sizeX&&yid<sizeY) \n"
"	{	\n"
"		int Index = xid + yid*sizeX; \n"
"		int midx; \n"
"		int midy; \n"
"		if(xid < sizeX/2) \n"
"			midx=0; \n"
"		else \n"
"			midx=sizeX; \n"
"		if(yid < sizeY/2) \n"
"			midy=0; \n"
"		else \n"
"			midy=sizeY; \n"
"		float xp = xid - midx; \n"
"		float yp = yid - midy; \n"
"		float rad = hypot(xp,yp); \n"
"		int dqe = floor(rad/binning); \n"
"		float dqeval = DQE[min(dqe,724)]; \n"
"		float real = clIm[Index].x;	\n"
"		float imag = clIm[Index].y;	\n"
"		clIm[Index].x = real*sqrt(dqeval);	\n"
"		clIm[Index].y = imag*sqrt(dqeval);	\n"
"	}	\n"
"}	\n"
;

const char* NTFSource = 
"__kernel void clNTF(__global const float2* clIm, __global float* DQE, int sizeX, int sizeY, int binning)	\n"
"{	\n"
"	//Get the work items ID \n"
"	int xid = get_global_id(0);	\n"
"	int yid = get_global_id(1); \n"
"	\n"
"	if(xid<sizeX&&yid<sizeY) \n"
"	{	\n"
"		int Index = xid + yid*sizeX; \n"
"		int midx; \n"
"		int midy; \n"
"		if(xid < sizeX/2) \n"
"			midx=0; \n"
"		else \n"
"			midx=sizeX; \n"
"		if(yid < sizeY/2) \n"
"			midy=0; \n"
"		else \n"
"			midy=sizeY; \n"
"		float rad = (xid-midx)*(xid-midx) +(yid-midy)*(yid-midy); \n"
"		int dqe = floor(rad/binning); \n"
"		float dqeval = DQE[min(dqe,724)]; \n"
"		float real = clIm[Index].x;	\n"
"		float imag = clIm[Index].y;	\n"
"		clIm[Index].x = real*dqeval;	\n"
"		clIm[Index].y = imag*dqeval;	\n"
"	}	\n"
"}	\n"
;