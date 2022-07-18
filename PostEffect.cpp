#include "PostEffect.h"
#include <d3dx12.h>
#include <DirectXTex.h>
#include <d3dcompiler.h>

const float PostEffect::clearColor[4] = { 0.25f,0.5f,0.1f,0.0f };//�΂��ۂ��F


PostEffect* PostEffect::Create(SpriteCommon* spriteCommon, UINT texNumber, XMFLOAT2 anchorpoint, bool isFlipX, bool isFlipY)
{

    //�������m��
    PostEffect* instance = new PostEffect();
    //�C���X�^���X������
    instance->Initialize(spriteCommon, texNumber, anchorpoint, isFlipX, isFlipY);

    return instance;
}

void PostEffect::Initialize(SpriteCommon* spriteCommon, UINT texNumber, XMFLOAT2 anchorpoint, bool isFlipX, bool isFlipY)
{
    HRESULT result = S_FALSE;

    //�����o�ϐ��ɏ�������
    spriteCommona = spriteCommon;
    texNumbera = texNumber;
    anchorpointa = anchorpoint;
    isFlipXa = isFlipX;
    isFlipYa = isFlipY;

    //�p�C�v���C������
    CreateGraphicsPipelineState();

    // ���_�o�b�t�@����
    result = spriteCommona->GetDevice()->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(sizeof(VertexPosUv)*vertNum),
        D3D12_RESOURCE_STATE_GENERIC_READ, 
        nullptr,
        IID_PPV_ARGS(&vertBuffa));
    assert(SUCCEEDED(result));

    //���_�f�[�^
    VertexPosUv vertices[vertNum] = {
        {{-0.5f,-0.5f,0.0f},{0.0f,1.0f}},//����
        {{-0.5f,+0.5f,0.0f},{0.0f,0.0f}},//����
        {{+0.5f,-0.5f,0.0f},{1.0f,1.0f}},//�E��
        {{+0.5f,+0.5f,0.0f},{1.0f,0.0f}},//�E��
    };

    // ���_�o�b�t�@�փf�[�^�]��
    VertexPosUv* vertMap = nullptr;
    result = vertBuffa->Map(0, nullptr, (void**)&vertMap);
    if (SUCCEEDED(result))
    {
        memcpy(vertMap, vertices, sizeof(vertices));
        vertBuffa->Unmap(0, nullptr);
    }

    // ���_�o�b�t�@�r���[�̍쐬
    vbViewa.BufferLocation = vertBuffa->GetGPUVirtualAddress();
    vbViewa.SizeInBytes = sizeof(VertexPosUv)*4;
    vbViewa.StrideInBytes = sizeof(VertexPosUv);

    // �萔�o�b�t�@�̐���
    result = spriteCommona->GetDevice()->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferData) + 0xff) & ~0xff),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&constBuffa));
    assert(SUCCEEDED(result));

    //�萔�o�b�t�@�Ƀf�[�^�]��
    ConstBufferData* constMap = nullptr;
    result = constBuffa->Map(0, nullptr, (void**)&constMap);
    constMap->color = this->colora;
    constMap->mat = XMMatrixIdentity();
    this->constBuffa->Unmap(0, nullptr);

    //�e�N�X�`�����\�[�X�ݒ�
    CD3DX12_RESOURCE_DESC texresDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R8G8B8A8_UNORM,
        WindowsApp::window_width,
        (UINT)WindowsApp::window_height,
        1, 0, 1, 0,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
    );

    for (int i = 0; i < 2; i++)
    {
        //�e�N�X�`���o�b�t�@�̐���
        result = spriteCommona->GetDevice()->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
                D3D12_MEMORY_POOL_L0),
            D3D12_HEAP_FLAG_NONE,
            &texresDesc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, clearColor),
            IID_PPV_ARGS(&texBuff[i]));
        assert(SUCCEEDED(result));

        //��f��(1280x720=921600�s�N�Z��)
        const UINT pixelCount = WindowsApp::window_width * WindowsApp::window_height;
        //�摜1�s���̃f�[�^�T�C�Y
        const UINT rowPitch = sizeof(UINT) * WindowsApp::window_width;
        //�摜�S�̂̃f�[�^�T�C�Y
        const UINT depthPitch = rowPitch * WindowsApp::window_height;
        //�摜�C���[�W
        UINT* img = new UINT[pixelCount];
        for (int j = 0; j < pixelCount; j++) { img[j] = 0xff0000ff; }

        //�e�N�X�`���o�b�t�@�Ƀf�[�^�]��
        result = texBuff[i]->WriteToSubresource(0, nullptr,
            img, rowPitch, depthPitch);
        assert(SUCCEEDED(result));
        delete[] img;
    }
    

    //SRV�p�f�X�N���v�^�q�[�v�ݒ�
    D3D12_DESCRIPTOR_HEAP_DESC srvDescHeapDesc = {};
    srvDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    srvDescHeapDesc.NumDescriptors = 2;

    //SRV�p�f�X�N���v�^�q�[�v�𐶐�
    result = spriteCommon->GetDevice()->CreateDescriptorHeap(&srvDescHeapDesc, IID_PPV_ARGS(&descHeapSRV));
    assert(SUCCEEDED(result));

    //SRV�ݒ�
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    //�f�X�N���v�^�q�[�v��SRV����
    for (int i = 0; i < 2; i++)
    {
        spriteCommona->GetDevice()->CreateShaderResourceView(texBuff[i].Get(),
            &srvDesc,
            CD3DX12_CPU_DESCRIPTOR_HANDLE(
                descHeapSRV->GetCPUDescriptorHandleForHeapStart(), i,
               spriteCommona->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))
        );
    }

    //RTV�p�f�X�N���v�^�q�[�v�ݒ�
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescHeapDesc{};
    rtvDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDescHeapDesc.NumDescriptors = 2;
    //RTV�p�f�X�N���v�^�q�[�v�𐶐�
    result = spriteCommona->GetDevice()->CreateDescriptorHeap(&rtvDescHeapDesc, IID_PPV_ARGS(&descHeapRTV));
    assert(SUCCEEDED(result));

    for (int i = 0; i < 2; i++)
    {
        //�f�X�N���v�^�q�[�v��RTV�쐬
        spriteCommona->GetDevice()->CreateRenderTargetView(texBuff[i].Get(),
            nullptr,
            CD3DX12_CPU_DESCRIPTOR_HANDLE(
            descHeapRTV->GetCPUDescriptorHandleForHeapStart(),i,
                spriteCommona->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV))
        );
    }

    //�[�x�o�b�t�@���\�[�X�ݒ�
    CD3DX12_RESOURCE_DESC depthResDesc =
        CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_D32_FLOAT,
            WindowsApp::window_width,
            WindowsApp::window_height,
            1, 0,
            1, 0,
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
        );
    //�[�x�o�b�t�@�̐���
    result = spriteCommona->GetDevice()->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &depthResDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0),
        IID_PPV_ARGS(&depthBuff));
    assert(SUCCEEDED(result));

    //DSV�p�f�X�N���v�^�q�[�v�ݒ�
    D3D12_DESCRIPTOR_HEAP_DESC DescHeapDesc{};
    DescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    DescHeapDesc.NumDescriptors = 1;
    //DSV�p�f�X�N���v�^�q�[�v���쐬
    result = spriteCommona->GetDevice()->CreateDescriptorHeap(&DescHeapDesc, IID_PPV_ARGS(&descHeapDSV));
    assert(SUCCEEDED(result));

    //�f�X�N���v�^�q�[�v��DSV�쐬
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;//�[�x�l�t�H�[�}�b�g
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    spriteCommona->GetDevice()->CreateDepthStencilView(depthBuff.Get(),
        &dsvDesc,
        descHeapDSV->GetCPUDescriptorHandleForHeapStart());

}


void PostEffect::Draw(ID3D12GraphicsCommandList* cmdList)
{

    //if (Input::GetInstance()->TriggerKey(DIK_SPACE))
    //{
    //    //�f�X�N���v�^�q�[�v��SRV�쐬
    //    static int tex = 0;
    //    //�e�N�X�`���ԍ���0��1�Ő؂�ւ�
    //    tex = (tex + 1) % 2;
    //    D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc{};//�ݒ�\����
    //    srvdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    //    srvdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    //    srvdesc.ViewDimension= D3D12_SRV_DIMENSION_TEXTURE2D;//2D�e�N�X�`��
    //    srvdesc.Texture2D.MipLevels = 1;
    //    spriteCommona->GetDevice()->CreateShaderResourceView(texBuff[tex].Get(),//�r���[�Ɗ֘A�t����o�b�t�@
    //        &srvdesc,
    //        descHeapSRV->GetCPUDescriptorHandleForHeapStart()
    //    );
    //}
    
    //�p�C�v���C���X�e�[�g�̐ݒ�
    cmdList->SetPipelineState(pipelineState.Get());
    //���[�g�V�O�l�`���̐ݒ�
    cmdList->SetGraphicsRootSignature(rootSignature.Get());
    //�v���~�e�B�u�`���ݒ�
    cmdList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // �e�N�X�`���p�f�X�N���v�^�q�[�v�̐ݒ�
    ID3D12DescriptorHeap* ppHeaps[] = { descHeapSRV.Get()};
    
    //ID3D12GraphicsCommandList* cmdList = spriteCommon->GetCommandList();

    // ���_�o�b�t�@���Z�b�g
    cmdList->IASetVertexBuffers(0, 1, &vbViewa);

    //�f�X�N���v�^�q�[�v���Z�b�g
    cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    // ���[�g�p�����[�^0�ӂɒ萔�o�b�t�@���Z�b�g
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


    // �|���S���̕`��i4���_�Ŏl�p�`�j
    cmdList->DrawInstanced(4, 1, 0, 0);
}

void PostEffect::PreDrawScene(ID3D12GraphicsCommandList* cmdList)
{
    for (int i = 0; i < 2; i++)
    {
        //���\�[�X�o���A��ύX
        cmdList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(texBuff[i].Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET));
    }
    
    //�����_�\�^�[�Q�b�g�r���[�p�f�X�N���v�^�q�[�v�̃n���h�����擾
    D3D12_CPU_DESCRIPTOR_HANDLE rtvH[2];
    for (int i = 0; i < 2; i++)
    {
        rtvH[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(
            descHeapRTV->GetCPUDescriptorHandleForHeapStart(), i,
            spriteCommona->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
        );
    }
        
    //�[�x�X�e���V���r���[�p�f�X�N���v�^�q�[�v�̃n���h�����擾
    D3D12_CPU_DESCRIPTOR_HANDLE dsvH =
        descHeapDSV->GetCPUDescriptorHandleForHeapStart();
    //�����_�\�^�[�Q�b�g���Z�b�g
    cmdList->OMSetRenderTargets(2,rtvH, false, &dsvH);
    //�r���[�|�[�g�̐ݒ�
    CD3DX12_VIEWPORT Viewports[2];
    for (size_t i = 0; i < 2; i++)
    {
        Viewports[i] = CD3DX12_VIEWPORT(0.0f, 0.0f,
            WindowsApp::window_width, WindowsApp::window_height);
    }
    cmdList->RSSetViewports(2, Viewports);
    //�V�U�����O��`�̐ݒ�
    CD3DX12_RECT Scissorrects[2];
    for (size_t i = 0; i < 2; i++)
    {
        Scissorrects[i] = CD3DX12_RECT(0.0f, 0.0f, WindowsApp::window_width,
            WindowsApp::window_height);
    }
    cmdList->RSSetScissorRects(2,Scissorrects);

    //�S��ʃN���A
    for (int i = 0; i < 2; i++)
    {
        cmdList->ClearRenderTargetView(rtvH[i], clearColor, 0, nullptr);
    }
    
    //�[�x�o�b�t�@
    cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void PostEffect::PostDrawScene(ID3D12GraphicsCommandList* cmdList)
{
    //���\�[�X�o���A��ύX(�`��\->�V�F�[�_�[���\�[�X)
    for (int i = 0; i < 2; i++)
    {
        cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texBuff[i].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
    } 
}

void PostEffect::CreateGraphicsPipelineState()
{
    HRESULT result;

    ComPtr<ID3DBlob> vsBlob = nullptr; // ���_�V�F�[�_�I�u�W�F�N�g
    ComPtr<ID3DBlob> psBlob = nullptr; // �s�N�Z���V�F�[�_�I�u�W�F�N�g
    ComPtr<ID3DBlob> errorBlob = nullptr; // �G���[�I�u�W�F�N�g

    // ���_�V�F�[�_�̓ǂݍ��݂ƃR���p�C��
    result = D3DCompileFromFile(
        L"Resources/shaders/PostEffectVS.hlsl",  // �V�F�[�_�t�@�C����
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE, // �C���N���[�h�\�ɂ���
        "main", "vs_5_0", // �G���g���[�|�C���g���A�V�F�[�_�[���f���w��
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // �f�o�b�O�p�ݒ�
        0,
        &vsBlob, &errorBlob);

    if (FAILED(result)) {
        // errorBlob����G���[���e��string�^�ɃR�s�[
        std::string errstr;
        errstr.resize(errorBlob->GetBufferSize());

        std::copy_n((char*)errorBlob->GetBufferPointer(),
            errorBlob->GetBufferSize(),
            errstr.begin());
        errstr += "\n";
        // �G���[���e���o�̓E�B���h�E�ɕ\��
        OutputDebugStringA(errstr.c_str());
        assert(0);
    }

    // �s�N�Z���V�F�[�_�̓ǂݍ��݂ƃR���p�C��
    result = D3DCompileFromFile(
        L"Resources/shaders/PostEffectPS.hlsl",   // �V�F�[�_�t�@�C����
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE, // �C���N���[�h�\�ɂ���
        "main", "ps_5_0", // �G���g���[�|�C���g���A�V�F�[�_�[���f���w��
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // �f�o�b�O�p�ݒ�
        0,
        &psBlob, &errorBlob);

    if (FAILED(result)) {
        // errorBlob����G���[���e��string�^�ɃR�s�[
        std::string errstr;
        errstr.resize(errorBlob->GetBufferSize());

        std::copy_n((char*)errorBlob->GetBufferPointer(),
            errorBlob->GetBufferSize(),
            errstr.begin());
        errstr += "\n";
        // �G���[���e���o�̓E�B���h�E�ɕ\��
        OutputDebugStringA(errstr.c_str());
        assert(0);
    }

    // ���_���C�A�E�g
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,      0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    // �O���t�B�b�N�X�p�C�v���C���ݒ�
    D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline{};

    gpipeline.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
    gpipeline.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());

    gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // �W���ݒ�

    gpipeline.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;              // �w�ʃJ�����O�����Ȃ�

    // �����_�[�^�[�Q�b�g�̃u�����h�ݒ�
    D3D12_RENDER_TARGET_BLEND_DESC& blenddesc = gpipeline.BlendState.RenderTarget[0];
    blenddesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL; // �W���ݒ�
    blenddesc.BlendEnable = true;                   // �u�����h��L���ɂ���
    blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;    // ���Z
    blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE;      // �\�[�X�̒l��100% �g��
    blenddesc.DestBlendAlpha = D3D12_BLEND_ZERO;    // �f�X�g�̒l��   0% �g��
    blenddesc.BlendOp = D3D12_BLEND_OP_ADD;             // ���Z
    blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;         // �\�[�X�̃A���t�@�l
    blenddesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;    // 1.0f-�\�[�X�̃A���t�@�l

    // �f�v�X�X�e���V���X�e�[�g�̐ݒ�
    gpipeline.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    gpipeline.DepthStencilState.DepthEnable = false;    // �[�x�e�X�g�����Ȃ�
    //gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;       // ��ɏ㏑�����[��
    gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT; // �[�x�l�t�H�[�}�b�g

    gpipeline.InputLayout.pInputElementDescs = inputLayout;
    gpipeline.InputLayout.NumElements = _countof(inputLayout);

    gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    gpipeline.NumRenderTargets = 1; // �`��Ώۂ�1��
    gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // 0�`255�w���RGBA
    gpipeline.SampleDesc.Count = 1; // 1�s�N�Z���ɂ�1��T���v�����O

    // �f�X�N���v�^�e�[�u���̐ݒ�
    CD3DX12_DESCRIPTOR_RANGE descRangeSRV0;
    descRangeSRV0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0 ���W�X�^
    CD3DX12_DESCRIPTOR_RANGE descRangeSRV1;
    descRangeSRV1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1); // t1 ���W�X�^
    // ���[�g�p�����[�^�̐ݒ�
    CD3DX12_ROOT_PARAMETER rootparams[3];
    rootparams[0].InitAsConstantBufferView(0); // �萔�o�b�t�@�r���[�Ƃ��ď�����(b0���W�X�^)
    rootparams[1].InitAsDescriptorTable(1, &descRangeSRV0);
    rootparams[2].InitAsDescriptorTable(1, &descRangeSRV1);

    // �X�^�e�B�b�N�T���v���[
    CD3DX12_STATIC_SAMPLER_DESC samplerDesc = CD3DX12_STATIC_SAMPLER_DESC(0);

    // ���[�g�V�O�l�`���̐ݒ�
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_0(_countof(rootparams), rootparams, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> rootSigBlob;
    // �o�[�W������������ł̃V���A���C�Y
    result = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);
    assert(SUCCEEDED(result));
    // ���[�g�V�O�l�`���̐���
    result = spriteCommona->GetDevice()->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    assert(SUCCEEDED(result));

    // �p�C�v���C���Ƀ��[�g�V�O�l�`�����Z�b�g
    gpipeline.pRootSignature = rootSignature.Get();

    result = spriteCommona->GetDevice()->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&pipelineState));
    assert(SUCCEEDED(result));
}