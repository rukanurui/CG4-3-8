#pragma once
#include <Windows.h>
#include <d3d12.h>

using namespace Microsoft::WRL;

struct PipelineSet
{
    // パイプラインステートオブジェクト
    ComPtr<ID3D12PipelineState> pipelinestate;
    // ルートシグネチャ
    ComPtr<ID3D12RootSignature> rootsignature;
};

