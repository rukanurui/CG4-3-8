#pragma once
#include "SpriteCommon.h"

#include <wrl.h>
#include <d3d12.h>
#include <DirectXMath.h>
using namespace Microsoft::WRL;


class Sprite
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

    // 頂点データ
    struct VertexPosUv
    {
        XMFLOAT3 pos; // xyz座標
        XMFLOAT2 uv;  // uv座標
    };

    // 定数バッファ用データ構造体
    struct ConstBufferData {
        XMFLOAT4 color; // 色 (RGBA)
        XMMATRIX mat;   // ３Ｄ変換行列
    };

    static Sprite* Create(SpriteCommon* spriteCommon, UINT texNumber, XMFLOAT2 anchorpoint = { 0.5f,0.5f }, bool isFlipX = false, bool isFlipY = false);


public:

    void Initialize(SpriteCommon* spriteCommon, UINT texNumber,XMFLOAT2 anchorpoint, bool isFlipX, bool isFlipY);

    //頂点バッファの転送
    void TransferVertexBuffer();

    void Draw();

    void Update();

    //コンストラクタ

    void SetPosition(const DirectX::XMFLOAT3& position) { positiona = position; }
    void SetRotation(float rotation) { rotationa = rotation; }
    void SetSize(const DirectX::XMFLOAT2& size) { sizea = size; }
    void SetTexLeftTop(const XMFLOAT2& texLeftTop) { texLeftTopa = texLeftTop ;}
    void SetTexSize(const XMFLOAT2& texSize) { texSizea = texSize; }

protected:
    SpriteCommon* spriteCommona = nullptr;
    //頂点バッファ;
    ComPtr<ID3D12Resource> vertBuffa;
    //頂点バッファビュー;
    D3D12_VERTEX_BUFFER_VIEW vbViewa{};
    //定数バッファ;
    ComPtr<ID3D12Resource> constBuffa;
    // 頂点数
    static const int vertNum = 4;
    // Z軸回りの回転角
    float rotationa = 0.0f;
    // 座標
    XMFLOAT3 positiona = { 0,0,0 };
    // ワールド行列
    XMMATRIX matWorlda;
    // 色(RGBA)
    XMFLOAT4 colora = { 1, 1, 1, 1 };
    // テクスチャ番号
    UINT texNumbera = 0;
    // 大きさ
    XMFLOAT2 sizea= { 100, 100 };
    // アンカーポイント
    XMFLOAT2 anchorpointa = { 0.5f, 0.5f };
    // 左右反転
    bool isFlipXa = false;
    // 上下反転
    bool isFlipYa = false;
    // テクスチャ左上座標
    XMFLOAT2 texLeftTopa = { 0, 0 };
    // テクスチャ切り出しサイズ
    XMFLOAT2 texSizea = { 100, 100 };
    // 非表示
    bool isInvisiblea = false;
};

