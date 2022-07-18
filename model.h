#pragma once
#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <d3dx12.h>

#include<vector>
#include<cassert>



using namespace std;

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;
using namespace Microsoft::WRL;



class model
{
private: // エイリアス
// Microsoft::WRL::を省略
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;
	// DirectX::を省略
	using XMFLOAT2 = DirectX::XMFLOAT2;
	using XMFLOAT3 = DirectX::XMFLOAT3;
	using XMFLOAT4 = DirectX::XMFLOAT4;
	using XMMATRIX = DirectX::XMMATRIX;

	

public: // サブクラス
	
	// 頂点データ構造体
	struct VertexPosNormalUv
	{
		DirectX::XMFLOAT3 pos; // xyz座標
		DirectX::XMFLOAT3 normal; // 法線ベクトル
		DirectX::XMFLOAT2 uv;  // uv座標
	};


	// 定数バッファ用データ構造体B1
	struct ConstBufferDataB1
	{
		XMFLOAT3 ambient;//アンビエント係数
		float pad1;      //パディング
		XMFLOAT3 diffuse;//ディフューズ係数
		float pad2;      //パディング
		XMFLOAT3 specular;//スペキュラー係数
		float alpha;      //アルファ
	};


	//マテリアル
	struct Material
	{
		std::string name;//マテリアル名
		DirectX::XMFLOAT3 ambient;//アンビエント影響度
		DirectX::XMFLOAT3 diffuse;//ディフューズ影響度
		DirectX::XMFLOAT3 specular; //スペキュラー影響度
		float alpha; //アルファ
		std::string textureFilename;//テクスチャファイル名
		//コンストラクタ
		Material() {
			ambient = { 0.3f,0.3f,0.3f };
			diffuse = { 0.0f,0.0f,0.0f };
			specular = { 0.0f,0.0f,0.0f };
			alpha = 1.0f;
		}
	};

public://静的メンバ関数
	//objファイルから3dモデルを読み込む
	static model* LoadFromOBJ(const std::string& modelname);
	//setter
	static void SetDevice(ID3D12Device* device) { model::device = device; }
	/// <summary>
	/// 描画
	/// </summary>
	void Draw(ID3D12GraphicsCommandList*cmdList,UINT rootParamIndexMaterial);



private://静的メンバ変数
	//デバイス
	static ID3D12Device* device;
	

private://メンバ変数
	
	// デスクリプタサイズ
	UINT descriptorHandleIncrementSize;

	// コマンドリスト
	ID3D12GraphicsCommandList* cmdList;
	
	// デスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> descHeap;
	// 頂点バッファ
	ComPtr<ID3D12Resource> vertBuff;
	// インデックスバッファ
	ComPtr<ID3D12Resource> indexBuff;
	// テクスチャバッファ
	ComPtr<ID3D12Resource> texbuff;
	// シェーダリソースビューのハンドル(CPU)
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescHandleSRV;
	// シェーダリソースビューのハンドル(CPU)
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescHandleSRV;
	// ビュー行列
	XMMATRIX matView;
	// 射影行列
	XMMATRIX matProjection;
	// 視点座標
	XMFLOAT3 eye;
	// 注視点座標
	XMFLOAT3 target;
	// 上方向ベクトル
	XMFLOAT3 up;
	// 頂点バッファビュー
	D3D12_VERTEX_BUFFER_VIEW vbView;
	// インデックスバッファビュー
	D3D12_INDEX_BUFFER_VIEW ibView;
	// 頂点データ配列
	std::vector<VertexPosNormalUv> vertices;
	// 頂点インデックス配列
	std::vector<unsigned short> indices;

	Material material;

	
	ComPtr<ID3D12Resource> constBuffB1; // 定数バッファ
	// 色
	XMFLOAT4 color = { 1,1,1,1 };
	// ローカルスケール
	XMFLOAT3 scale = { 1,1,1 };
	// X,Y,Z軸回りのローカル回転角
	XMFLOAT3 rotation = { 0,0,0 };
	// ローカル座標
	XMFLOAT3 position = { 0,0,0 };
	// ローカルワールド変換行列
	XMMATRIX matWorld;

private://非公開のメンバ関数
	//OBJファイルから3Dモデルを読み込む
	void LoadFromOBJInternal(const std::string& modelname);
	//マテリアル読み込み
	void LoadMaterial(const std::string& directoryPath, const std::string& filename);
	//テクスチャ読み込み
	bool LoadTexture(const std::string& directoryPath, const std::string& filename);

	bool InitializeDescriptorHeap();

	void CreateBuffers();
};

