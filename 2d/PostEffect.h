#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include "SpriteCommon.h"
#include "Sprite.h"
#include "../WindowsApp.h"
#include <d3dcompiler.h>
#include "../Input.h"

using namespace DirectX;
using namespace Microsoft::WRL;

#pragma comment(lib, "d3dcompiler.lib")

class PostEffect :
    public Sprite
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

    //コンストラクタ
   /* PostEffect();*/

    static PostEffect* Create(SpriteCommon* spriteCommon, UINT texNumber, XMFLOAT2 anchorpoint, bool isFlipX, bool isFlipY);
    void Initialize(SpriteCommon* spriteCommon, UINT texNumber, XMFLOAT2 anchorpoint, bool isFlipX, bool isFlipY);
    //頂点バッファの転送
    //void TransferVertexBuffer();
    //描画
    void Draw(ID3D12GraphicsCommandList* cmdList);

    //シーン描画前処理
    void PreDrawScene(ID3D12GraphicsCommandList* cmdList);
    //シーン描画後処理
    void PostDrawScene(ID3D12GraphicsCommandList* cmdList);
    //パイプライン生成
    void CreateGraphicsPipelineState();

private:
    //テクスチャバッファ
    ComPtr<ID3D12Resource> texBuff[2];
    //SRV用デスクリプタヒープ
    ComPtr<ID3D12DescriptorHeap> descHeapSRV;
    //深度バッファ
    ComPtr<ID3D12Resource> depthBuff;
    //RTV用デスクリプタヒープ
    ComPtr<ID3D12DescriptorHeap> descHeapRTV;
    //DSV用デスクリプタヒープ
    ComPtr<ID3D12DescriptorHeap> descHeapDSV;
    //グラフィックスパイプライン
    ComPtr<ID3D12PipelineState> pipelineState;
    //ルートシグネチャ
    ComPtr<ID3D12RootSignature> rootSignature;



private:
    //画面クリアカラー
    static const float clearColor[4];

};

