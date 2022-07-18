#include "Sprite.hlsli"

Texture2D<float4> tex : register(t0);//0番目に設定されたテクスチャ
SamplerState smp : register(s0);//0番目に設定されたサンプラー


float4 main(VSOutput input) : SV_TARGET
{

	return tex.Sample(smp, input.uv) * color;
}

