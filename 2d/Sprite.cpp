#include "Sprite.h"

#include <d3dx12.h>

using namespace DirectX;

Sprite*Sprite::Create(SpriteCommon* spriteCommon, UINT texNumber, XMFLOAT2 anchorpoint, bool isFlipX, bool isFlipY)
{
    //メモリ確保
    Sprite* instance = new Sprite();
    //インスタンス初期化
    instance->Initialize(spriteCommon, texNumber, anchorpoint, isFlipX, isFlipY);

    return instance;
}

void Sprite::Initialize(SpriteCommon* spriteCommon, UINT texNumber, XMFLOAT2 anchorpoint, bool isFlipX, bool isFlipY)
{
    HRESULT result = S_FALSE;

    //メンバ変数に書き込む
    spriteCommona = spriteCommon;
    texNumbera = texNumber;
    anchorpointa = anchorpoint;
    isFlipXa = isFlipX;
    isFlipYa = isFlipY;

    // 頂点データ
    VertexPosUv vertices[4];

    // 指定番号の画像が読み込み済みなら
    if (spriteCommona->GetTexBuff(texNumbera)) {
        // テクスチャ情報取得
        D3D12_RESOURCE_DESC resDesc = spriteCommona->GetTexBuff(texNumbera)->GetDesc();

        // スプライトの大きさを画像の解像度に合わせる
        sizea = { (float)resDesc.Width, (float)resDesc.Height };
        texSizea = { (float)resDesc.Width, (float)resDesc.Height };
    }

    // 頂点バッファ生成
    result = spriteCommona->GetDevice()->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertBuffa));

    // 頂点バッファデータ転送
    TransferVertexBuffer();

    // 頂点バッファビューの作成
    vbViewa.BufferLocation = vertBuffa->GetGPUVirtualAddress();
    vbViewa.SizeInBytes = sizeof(vertices);
    vbViewa.StrideInBytes = sizeof(vertices[0]);

    // 定数バッファの生成
    result = spriteCommona->GetDevice()->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferData) + 0xff) & ~0xff),
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&constBuffa));

    // 定数バッファにデータ転送
    ConstBufferData* constMap = nullptr;
    result = constBuffa->Map(0, nullptr, (void**)&constMap);
    constMap->color = XMFLOAT4(1, 1, 1, 1); // 色指定（RGBA）
    constMap->mat = spriteCommona->GetMatProjection();  
    constBuffa->Unmap(0, nullptr);

    


}

void Sprite::TransferVertexBuffer()
{
    HRESULT result = S_FALSE;

    // 頂点データ
    VertexPosUv vertices[] = {
        //     u     v
        {{}, {0.0f, 1.0f}}, // 左下
        {{}, {0.0f, 0.0f}}, // 左上
        {{}, {1.0f, 1.0f}}, // 右下
        {{}, {1.0f, 0.0f}}, // 右上
    };

    // 左下、左上、右下、右上
    enum { LB, LT, RB, RT };

    float left = (0.0f - anchorpointa.x) * sizea.x;
    float right = (1.0f - anchorpointa.x) * sizea.x;
    float top = (0.0f - anchorpointa.y) * sizea.y;
    float bottom = (1.0f - anchorpointa.y) * sizea.y;

    if (isFlipXa)
    {// 左右入れ替え
        left = -left;
        right = -right;
    }

    if (isFlipYa)
    {// 左右入れ替え
        top = -top;
        bottom = -bottom;
    }

    vertices[LB].pos = { left, bottom,  0.0f }; // 左下
    vertices[LT].pos = { left, top,     0.0f }; // 左上
    vertices[RB].pos = { right, bottom, 0.0f }; // 右下
    vertices[RT].pos = { right, top,    0.0f }; // 右上

    // 指定番号の画像が読み込み済みなら
    if (spriteCommona->GetTexBuff(texNumbera)) {
        // テクスチャ情報取得
        D3D12_RESOURCE_DESC resDesc = spriteCommona->GetTexBuff(texNumbera)->GetDesc();

        float tex_left = texLeftTopa.x / resDesc.Width;
        float tex_right = (texLeftTopa.x + texSizea.x) / resDesc.Width;
        float tex_top = texLeftTopa.y / resDesc.Height;
        float tex_bottom = (texLeftTopa.y + texSizea.y) / resDesc.Height;

        vertices[LB].uv = { tex_left,   tex_bottom }; // 左下
        vertices[LT].uv = { tex_left,   tex_top }; // 左上
        vertices[RB].uv = { tex_right,  tex_bottom }; // 右下
        vertices[RT].uv = { tex_right,  tex_top }; // 右上
    }

    // 頂点バッファへのデータ転送
    VertexPosUv* vertMap = nullptr;
    result = vertBuffa->Map(0, nullptr, (void**)&vertMap);
    memcpy(vertMap, vertices, sizeof(vertices));
    vertBuffa->Unmap(0, nullptr);
}

void Sprite::Draw()
{
    if (isInvisiblea) {
        return;
    }

    ID3D12GraphicsCommandList* cmdList = spriteCommona->GetCommandList();

    // 頂点バッファをセット
    cmdList->IASetVertexBuffers(0, 1, &vbViewa);

    // ルートパラメータ0晩に定数バッファをセット
    cmdList->SetGraphicsRootConstantBufferView(0, constBuffa->GetGPUVirtualAddress());

    // ルートパラメータ1晩にシェーダリソースビューをセット
    spriteCommona->SetGraphicsRootDescriptorTabl(1, texNumbera);

    // ポリゴンの描画（4頂点で四角形）
    cmdList->DrawInstanced(4, 1, 0, 0);
}

void Sprite::Update()
{
    // ワールド行列の更新
    matWorlda = XMMatrixIdentity();
    // Z軸回転
    matWorlda *= XMMatrixRotationZ(XMConvertToRadians(rotationa));
    // 平行移動
    matWorlda *= XMMatrixTranslation(positiona.x, positiona.y, positiona.z);

    // 定数バッファの転送
    ConstBufferData* constMap = nullptr;
    HRESULT result = constBuffa->Map(0, nullptr, (void**)&constMap);
    constMap->mat = matWorlda * spriteCommona->GetMatProjection();
    constMap->color = colora;
    constBuffa->Unmap(0, nullptr);
}


