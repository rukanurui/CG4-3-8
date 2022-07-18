#pragma once

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include"PipelineSet.h"



class SpriteCommon
{

private: // エイリアス
// Microsoft::WRL::を省略
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;
    // DirectX::を省略
    using XMFLOAT2 = DirectX::XMFLOAT2;
    using XMFLOAT3 = DirectX::XMFLOAT3;
    using XMFLOAT4 = DirectX::XMFLOAT4;
    using XMMATRIX = DirectX::XMMATRIX;

public:
	// テクスチャの最大枚数
	static const int spriteSRVCount = 512;

public:
    // パイプラインセット
    PipelineSet pipelineSeta;
    // 射影行列
    XMMATRIX matProjectiona{};
    // テクスチャ用デスクリプタヒープの生成
    ComPtr<ID3D12DescriptorHeap> descHeapa;
    // テクスチャリソース（テクスチャバッファ）の配列
    ComPtr<ID3D12Resource> texBuffa[spriteSRVCount];

    ID3D12Device* devicea = nullptr;

    ID3D12GraphicsCommandList* cmdLista = nullptr;

private:
    void CreateGraphicsPipeline();

public://メンバ関数
    //初期化
    void Initialize(ID3D12Device* devicea, ID3D12GraphicsCommandList* cmdList, int window_width, int window_height);

    void PreDraw();

    void LoadTexture(UINT texnumber, const wchar_t* filename);

    void SetGraphicsRootDescriptorTabl(UINT rootParameterIndex,UINT texNumber);

    ID3D12Resource* GetTexBuff(int texnumber);

    ID3D12Device* GetDevice() { return devicea;};

    ID3D12GraphicsCommandList* GetCommandList() { return cmdLista; }

    const DirectX::XMMATRIX& GetMatProjection() { return matProjectiona; }



};

