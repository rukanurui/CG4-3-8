#include "PostEffect.h"
#include <d3dx12.h>
#include <DirectXTex.h>
#include <d3dcompiler.h>

const float PostEffect::clearColor[4] = { 0.25f,0.5f,0.1f,0.0f };//緑っぽい色


PostEffect* PostEffect::Create(SpriteCommon* spriteCommon, UINT texNumber, XMFLOAT2 anchorpoint, bool isFlipX, bool isFlipY)
{

    //メモリ確保
    PostEffect* instance = new PostEffect();
    //インスタンス初期化
    instance->Initialize(spriteCommon, texNumber, anchorpoint, isFlipX, isFlipY);

    return instance;
}

void PostEffect::Initialize(SpriteCommon* spriteCommon, UINT texNumber, XMFLOAT2 anchorpoint, bool isFlipX, bool isFlipY)
{
    HRESULT result = S_FALSE;

    //メンバ変数に書き込む
    spriteCommona = spriteCommon;
    texNumbera = texNumber;
    anchorpointa = anchorpoint;
    isFlipXa = isFlipX;
    isFlipYa = isFlipY;

    //パイプライン生成
    CreateGraphicsPipelineState();

    // 頂点バッファ生成
    result = spriteCommona->GetDevice()->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(sizeof(VertexPosUv)*vertNum),
        D3D12_RESOURCE_STATE_GENERIC_READ, 
        nullptr,
        IID_PPV_ARGS(&vertBuffa));
    assert(SUCCEEDED(result));

    //頂点データ
    VertexPosUv vertices[vertNum] = {
        {{-0.5f,-0.5f,0.0f},{0.0f,1.0f}},//左下
        {{-0.5f,+0.5f,0.0f},{0.0f,0.0f}},//左上
        {{+0.5f,-0.5f,0.0f},{1.0f,1.0f}},//右下
        {{+0.5f,+0.5f,0.0f},{1.0f,0.0f}},//右上
    };

    // 頂点バッファへデータ転送
    VertexPosUv* vertMap = nullptr;
    result = vertBuffa->Map(0, nullptr, (void**)&vertMap);
    if (SUCCEEDED(result))
    {
        memcpy(vertMap, vertices, sizeof(vertices));
        vertBuffa->Unmap(0, nullptr);
    }

    // 頂点バッファビューの作成
    vbViewa.BufferLocation = vertBuffa->GetGPUVirtualAddress();
    vbViewa.SizeInBytes = sizeof(VertexPosUv)*4;
    vbViewa.StrideInBytes = sizeof(VertexPosUv);

    // 定数バッファの生成
    result = spriteCommona->GetDevice()->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferData) + 0xff) & ~0xff),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&constBuffa));
    assert(SUCCEEDED(result));

    //定数バッファにデータ転送
    ConstBufferData* constMap = nullptr;
    result = constBuffa->Map(0, nullptr, (void**)&constMap);
    constMap->color = this->colora;
    constMap->mat = XMMatrixIdentity();
    this->constBuffa->Unmap(0, nullptr);

    //テクスチャリソース設定
    CD3DX12_RESOURCE_DESC texresDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R8G8B8A8_UNORM,
        WindowsApp::window_width,
        (UINT)WindowsApp::window_height,
        1, 0, 1, 0,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
    );

    for (int i = 0; i < 2; i++)
    {
        //テクスチャバッファの生成
        result = spriteCommona->GetDevice()->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
                D3D12_MEMORY_POOL_L0),
            D3D12_HEAP_FLAG_NONE,
            &texresDesc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, clearColor),
            IID_PPV_ARGS(&texBuff[i]));
        assert(SUCCEEDED(result));

        //画素数(1280x720=921600ピクセル)
        const UINT pixelCount = WindowsApp::window_width * WindowsApp::window_height;
        //画像1行分のデータサイズ
        const UINT rowPitch = sizeof(UINT) * WindowsApp::window_width;
        //画像全体のデータサイズ
        const UINT depthPitch = rowPitch * WindowsApp::window_height;
        //画像イメージ
        UINT* img = new UINT[pixelCount];
        for (int j = 0; j < pixelCount; j++) { img[j] = 0xff0000ff; }

        //テクスチャバッファにデータ転送
        result = texBuff[i]->WriteToSubresource(0, nullptr,
            img, rowPitch, depthPitch);
        assert(SUCCEEDED(result));
        delete[] img;
    }
    

    //SRV用デスクリプタヒープ設定
    D3D12_DESCRIPTOR_HEAP_DESC srvDescHeapDesc = {};
    srvDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    srvDescHeapDesc.NumDescriptors = 2;

    //SRV用デスクリプタヒープを生成
    result = spriteCommon->GetDevice()->CreateDescriptorHeap(&srvDescHeapDesc, IID_PPV_ARGS(&descHeapSRV));
    assert(SUCCEEDED(result));

    //SRV設定
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    //デスクリプタヒープにSRV生成
    for (int i = 0; i < 2; i++)
    {
        spriteCommona->GetDevice()->CreateShaderResourceView(texBuff[i].Get(),
            &srvDesc,
            CD3DX12_CPU_DESCRIPTOR_HANDLE(
                descHeapSRV->GetCPUDescriptorHandleForHeapStart(), i,
               spriteCommona->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))
        );
    }

    //RTV用デスクリプタヒープ設定
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescHeapDesc{};
    rtvDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDescHeapDesc.NumDescriptors = 2;
    //RTV用デスクリプタヒープを生成
    result = spriteCommona->GetDevice()->CreateDescriptorHeap(&rtvDescHeapDesc, IID_PPV_ARGS(&descHeapRTV));
    assert(SUCCEEDED(result));

    for (int i = 0; i < 2; i++)
    {
        //デスクリプタヒープにRTV作成
        spriteCommona->GetDevice()->CreateRenderTargetView(texBuff[i].Get(),
            nullptr,
            CD3DX12_CPU_DESCRIPTOR_HANDLE(
            descHeapRTV->GetCPUDescriptorHandleForHeapStart(),i,
                spriteCommona->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV))
        );
    }

    //深度バッファリソース設定
    CD3DX12_RESOURCE_DESC depthResDesc =
        CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_D32_FLOAT,
            WindowsApp::window_width,
            WindowsApp::window_height,
            1, 0,
            1, 0,
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
        );
    //深度バッファの生成
    result = spriteCommona->GetDevice()->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &depthResDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0),
        IID_PPV_ARGS(&depthBuff));
    assert(SUCCEEDED(result));

    //DSV用デスクリプタヒープ設定
    D3D12_DESCRIPTOR_HEAP_DESC DescHeapDesc{};
    DescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    DescHeapDesc.NumDescriptors = 1;
    //DSV用デスクリプタヒープを作成
    result = spriteCommona->GetDevice()->CreateDescriptorHeap(&DescHeapDesc, IID_PPV_ARGS(&descHeapDSV));
    assert(SUCCEEDED(result));

    //デスクリプタヒープにDSV作成
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;//深度値フォーマット
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    spriteCommona->GetDevice()->CreateDepthStencilView(depthBuff.Get(),
        &dsvDesc,
        descHeapDSV->GetCPUDescriptorHandleForHeapStart());

}


void PostEffect::Draw(ID3D12GraphicsCommandList* cmdList)
{

    //if (Input::GetInstance()->TriggerKey(DIK_SPACE))
    //{
    //    //デスクリプタヒープにSRV作成
    //    static int tex = 0;
    //    //テクスチャ番号を0と1で切り替え
    //    tex = (tex + 1) % 2;
    //    D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc{};//設定構造体
    //    srvdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    //    srvdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    //    srvdesc.ViewDimension= D3D12_SRV_DIMENSION_TEXTURE2D;//2Dテクスチャ
    //    srvdesc.Texture2D.MipLevels = 1;
    //    spriteCommona->GetDevice()->CreateShaderResourceView(texBuff[tex].Get(),//ビューと関連付けるバッファ
    //        &srvdesc,
    //        descHeapSRV->GetCPUDescriptorHandleForHeapStart()
    //    );
    //}
    
    //パイプラインステートの設定
    cmdList->SetPipelineState(pipelineState.Get());
    //ルートシグネチャの設定
    cmdList->SetGraphicsRootSignature(rootSignature.Get());
    //プリミティブ形状を設定
    cmdList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // テクスチャ用デスクリプタヒープの設定
    ID3D12DescriptorHeap* ppHeaps[] = { descHeapSRV.Get()};
    
    //ID3D12GraphicsCommandList* cmdList = spriteCommon->GetCommandList();

    // 頂点バッファをセット
    cmdList->IASetVertexBuffers(0, 1, &vbViewa);

    //デスクリプタヒープをセット
    cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    // ルートパラメータ0晩に定数バッファをセット
    cmdList->SetGraphicsRootConstantBufferView(0, constBuffa->GetGPUVirtualAddress());

    cmdList->SetGraphicsRootDescriptorTable(1,
        CD3DX12_GPU_DESCRIPTOR_HANDLE(
            descHeapSRV->GetGPUDescriptorHandleForHeapStart(), 0,
            spriteCommona->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
        )
    );

    cmdList->SetGraphicsRootDescriptorTable(2,
        CD3DX12_GPU_DESCRIPTOR_HANDLE(
            descHeapSRV->GetGPUDescriptorHandleForHeapStart(), 1,
            spriteCommona->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
        )
    );


    // ポリゴンの描画（4頂点で四角形）
    cmdList->DrawInstanced(4, 1, 0, 0);
}

void PostEffect::PreDrawScene(ID3D12GraphicsCommandList* cmdList)
{
    for (int i = 0; i < 2; i++)
    {
        //リソースバリアを変更
        cmdList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(texBuff[i].Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET));
    }
    
    //レンダ―ターゲットビュー用デスクリプタヒープのハンドルを取得
    D3D12_CPU_DESCRIPTOR_HANDLE rtvH[2];
    for (int i = 0; i < 2; i++)
    {
        rtvH[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(
            descHeapRTV->GetCPUDescriptorHandleForHeapStart(), i,
            spriteCommona->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
        );
    }
        
    //深度ステンシルビュー用デスクリプタヒープのハンドルを取得
    D3D12_CPU_DESCRIPTOR_HANDLE dsvH =
        descHeapDSV->GetCPUDescriptorHandleForHeapStart();
    //レンダ―ターゲットをセット
    cmdList->OMSetRenderTargets(2,rtvH, false, &dsvH);
    //ビューポートの設定
    CD3DX12_VIEWPORT Viewports[2];
    for (size_t i = 0; i < 2; i++)
    {
        Viewports[i] = CD3DX12_VIEWPORT(0.0f, 0.0f,
            WindowsApp::window_width, WindowsApp::window_height);
    }
    cmdList->RSSetViewports(2, Viewports);
    //シザリング矩形の設定
    CD3DX12_RECT Scissorrects[2];
    for (size_t i = 0; i < 2; i++)
    {
        Scissorrects[i] = CD3DX12_RECT(0.0f, 0.0f, WindowsApp::window_width,
            WindowsApp::window_height);
    }
    cmdList->RSSetScissorRects(2,Scissorrects);

    //全画面クリア
    for (int i = 0; i < 2; i++)
    {
        cmdList->ClearRenderTargetView(rtvH[i], clearColor, 0, nullptr);
    }
    
    //深度バッファ
    cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void PostEffect::PostDrawScene(ID3D12GraphicsCommandList* cmdList)
{
    //リソースバリアを変更(描画可能->シェーダーリソース)
    for (int i = 0; i < 2; i++)
    {
        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texBuff[i].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
    } 
}

void PostEffect::CreateGraphicsPipelineState()
{
    HRESULT result;

    ComPtr<ID3DBlob> vsBlob = nullptr; // 頂点シェーダオブジェクト
    ComPtr<ID3DBlob> psBlob = nullptr; // ピクセルシェーダオブジェクト
    ComPtr<ID3DBlob> errorBlob = nullptr; // エラーオブジェクト

    // 頂点シェーダの読み込みとコンパイル
    result = D3DCompileFromFile(
        L"Resources/shaders/PostEffectVS.hlsl",  // シェーダファイル名
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルード可能にする
        "main", "vs_5_0", // エントリーポイント名、シェーダーモデル指定
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用設定
        0,
        &vsBlob, &errorBlob);

    if (FAILED(result)) {
        // errorBlobからエラー内容をstring型にコピー
        std::string errstr;
        errstr.resize(errorBlob->GetBufferSize());

        std::copy_n((char*)errorBlob->GetBufferPointer(),
            errorBlob->GetBufferSize(),
            errstr.begin());
        errstr += "\n";
        // エラー内容を出力ウィンドウに表示
        OutputDebugStringA(errstr.c_str());
        assert(0);
    }

    // ピクセルシェーダの読み込みとコンパイル
    result = D3DCompileFromFile(
        L"Resources/shaders/PostEffectPS.hlsl",   // シェーダファイル名
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルード可能にする
        "main", "ps_5_0", // エントリーポイント名、シェーダーモデル指定
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用設定
        0,
        &psBlob, &errorBlob);

    if (FAILED(result)) {
        // errorBlobからエラー内容をstring型にコピー
        std::string errstr;
        errstr.resize(errorBlob->GetBufferSize());

        std::copy_n((char*)errorBlob->GetBufferPointer(),
            errorBlob->GetBufferSize(),
            errstr.begin());
        errstr += "\n";
        // エラー内容を出力ウィンドウに表示
        OutputDebugStringA(errstr.c_str());
        assert(0);
    }

    // 頂点レイアウト
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,      0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // グラフィックスパイプライン設定
    D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline{};

    gpipeline.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
    gpipeline.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());

    gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // 標準設定

    gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;              // 背面カリングをしない

    // レンダーターゲットのブレンド設定
    D3D12_RENDER_TARGET_BLEND_DESC& blenddesc = gpipeline.BlendState.RenderTarget[0];
    blenddesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL; // 標準設定
    blenddesc.BlendEnable = true;                   // ブレンドを有効にする
    blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;    // 加算
    blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE;      // ソースの値を100% 使う
    blenddesc.DestBlendAlpha = D3D12_BLEND_ZERO;    // デストの値を   0% 使う
    blenddesc.BlendOp = D3D12_BLEND_OP_ADD;             // 加算
    blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;         // ソースのアルファ値
    blenddesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;    // 1.0f-ソースのアルファ値

    // デプスステンシルステートの設定
    gpipeline.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    gpipeline.DepthStencilState.DepthEnable = false;    // 深度テストをしない
    //gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;       // 常に上書きルール
    gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT; // 深度値フォーマット

    gpipeline.InputLayout.pInputElementDescs = inputLayout;
    gpipeline.InputLayout.NumElements = _countof(inputLayout);

    gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    gpipeline.NumRenderTargets = 1; // 描画対象は1つ
    gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // 0～255指定のRGBA
    gpipeline.SampleDesc.Count = 1; // 1ピクセルにつき1回サンプリング

    // デスクリプタテーブルの設定
    CD3DX12_DESCRIPTOR_RANGE descRangeSRV0;
    descRangeSRV0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0 レジスタ
    CD3DX12_DESCRIPTOR_RANGE descRangeSRV1;
    descRangeSRV1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); // t1 レジスタ
    // ルートパラメータの設定
    CD3DX12_ROOT_PARAMETER rootparams[3];
    rootparams[0].InitAsConstantBufferView(0); // 定数バッファビューとして初期化(b0レジスタ)
    rootparams[1].InitAsDescriptorTable(1, &descRangeSRV0);
    rootparams[2].InitAsDescriptorTable(1, &descRangeSRV1);

    // スタティックサンプラー
    CD3DX12_STATIC_SAMPLER_DESC samplerDesc = CD3DX12_STATIC_SAMPLER_DESC(0);

    // ルートシグネチャの設定
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_0(_countof(rootparams), rootparams, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> rootSigBlob;
    // バージョン自動判定でのシリアライズ
    result = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);
    assert(SUCCEEDED(result));
    // ルートシグネチャの生成
    result = spriteCommona->GetDevice()->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    assert(SUCCEEDED(result));

    // パイプラインにルートシグネチャをセット
    gpipeline.pRootSignature = rootSignature.Get();

    result = spriteCommona->GetDevice()->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&pipelineState));
    assert(SUCCEEDED(result));
}
