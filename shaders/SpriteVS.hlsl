#include "Sprite.hlsli"

VSOutput main(float4 pos : POSITION, float2 uv: TEXCOORD)
{
	VSOutput output;
	output.svpos = mul(mat, pos);//À•W‚És—ñ‚ğæZ
	output.uv = uv;
	return output;
}