#include "UnmanagedOpenCL.h"



#pragma once
using namespace System;

#pragma managed

#include "clix.h"

namespace ManagedOpenCLWrapper {

	public ref class ManagedOpenCL {
	private:
		UnmanagedOpenCL* _UMOpenCL;

	public:
		void ImportStructure(String^ filepath);
		void GetStructureDetails(Int32% Len, float% MinX, float% MinY, float% MinZ, float% MaxX, float% MaxY, float% MaxZ );
		void GetNumberSlices(Int32% Slices);
		void UploadParameterisation();
		void SortStructure();
		void SetTemParams(float df, float astigmag, float astigang, float kilovoltage, float spherical, float beta, float delta, float aperture);
		void SetStemParams(float df, float astigmag, float astigang, float kilovoltage, float spherical, float beta, float delta, float aperture);
		void InitialiseSimulation(int resolution);
		void MultisliceStep(int stepno, int steps);
		void GetCTEMImage(array<float>^ data, int resolution);
		ManagedOpenCL();
		~ManagedOpenCL();

	};
}