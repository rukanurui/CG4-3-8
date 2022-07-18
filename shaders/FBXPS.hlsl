#include"FBX.hlsli"

//0番スロットに設定されたテクスチャ
Texture2D<float4> tex : register(t0);
//0番スロットに追加されたサンプラー
SamplerState smp : register(s0);
struct PSOutput
{
	float4 target0 : SV_TARGET0;
	float4 target1 : SV_TARGET1;
};
//エントリーポイント
PSOutput main(VSOutput input)
{
	PSOutput output;
	//テクスチャマッピング
	float4 texcolor = tex.Sample(smp,input.uv);
	//Lambert反射
	float3 Light = normalize(float3(1, -1, 1));//右下奥向きのライト
	float diffuse = saturate(dot(-Light, input.normal));
	float brightness = diffuse + 0.3f;
	float4 shadecolor = float4(brightness, brightness, brightness, 1.0f);
	//陰影とテクスチャの色を合成
	output.target0 = shadecolor * texcolor;
	output.target1 = float4(1 - (shadecolor * texcolor).rgb, 1);
	return output;
}