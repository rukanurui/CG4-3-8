//#include "math.h"
//#include <DirectXMath.h>



cbuffer cbuff0 : register(b0)
{
	float4 color;
	matrix mat;
	float Time;
	float Bar;
};

struct VSOutput
{
	float4 svpos : SV_POSITION;
	float2 uv : TEXCOORD;
};