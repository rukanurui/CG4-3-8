#pragma once
#include <string>
#include <DirectXMath.h>
#include<vector>
#include <DirectXTex.h>
#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <d3dx12.h>
#include "fbxsdk.h"
#include<cassert>

struct Node
{
	//名前
	std::string name;
	//ローカルスケール
	DirectX::XMVECTOR scaling = { 1,1,1,0 };
	//ローカル回転角
	DirectX::XMVECTOR rotation = { 0,0,0,0 };
	//ローカル移動
	DirectX::XMVECTOR translation = { 0,0,0,1 };
	//ローカル変形行列
	DirectX::XMMATRIX transform;
	//グローバル変形行列
	DirectX::XMMATRIX globalTransform;
	//親ノード
	Node* parent = nullptr;
};

class FbxModel
{
public:
	friend class FbxLoader;

	//Microsoft::WRL::を省略
	template<class T> using Comptr = Microsoft::WRL::ComPtr<T>;
	//DirectXを省略
	using XMFLOAT2 = DirectX::XMFLOAT2;
	using XMFLOAT3 = DirectX::XMFLOAT3;
	using XMFLOAT4 = DirectX::XMFLOAT4;
	using XMMATRIX = DirectX::XMMATRIX;
	using TexMetadata = DirectX::TexMetadata;
	using ScratchImage = DirectX::ScratchImage;
	//std::を省略
	using string = std::string;
	template<class T> using vector = std::vector<T>;

public://メンバ定数
	//ボーンインデックスの最大数
	static const int MAX_BONE_INDICES = 4;

	struct VertexPosNormalUVSkin
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT2 uv;
		UINT boneIndex[MAX_BONE_INDICES];//ボーン番号
		float boneweight[MAX_BONE_INDICES];//重み
	};


	//ボーン構造体
	struct Bone
	{
		//名前
		std::string name;
		//初期姿勢の逆行列
		DirectX::XMMATRIX invInitialPose;
		//クラスター(FBX側のポーン情報)
		FbxCluster* fbxCluster;
		//コンストラクタ
		Bone(const std::string& name) {
			this->name = name;
		}
	};


private://メンバ変数
	std::string name;
	//ノード配列
	std::vector<Node> nodes;
	//頂点バッファ
	Comptr<ID3D12Resource> vertBuff;
	//インデックスバッファ
	Comptr<ID3D12Resource> indexBuff;
	//テクスチャバッファ
	Comptr<ID3D12Resource> texbuff;
	// 頂点バッファビュー
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	// インデックスバッファビュー
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	//SRV用デスクリプタヒープ
	Comptr<ID3D12DescriptorHeap> descHeapSRV;

	//アンビエント影響度
	DirectX::XMFLOAT3 ambient = { 1,1,1 };
	//ディフューズ影響度
	DirectX::XMFLOAT3 diffuse = { 1,1,1 };
	//テクスチャメタデータ
	DirectX::TexMetadata metadeta = {};
	//スクラッチイメージ
	DirectX::ScratchImage scratchImg = {};
	//メッシュを持つノード
	Node* meshNode = nullptr;

	//ボーン配列
	std::vector<Bone> bones;

	//FBXシーン
	FbxScene* fbxScene = nullptr;


public:
	//バッファ生成
	void CreateBuffers(ID3D12Device* device);
	//描画
	void FbxDraw(ID3D12GraphicsCommandList* cmdList);
	//モデルの変形行列取得
	const XMMATRIX& GetModelTransform() { return meshNode->globalTransform; }
	//頂点データ配列
	std::vector<VertexPosNormalUVSkin> vertices;
	//頂点インデックス配列
	std::vector<unsigned short> indices;
	//getter
	std::vector<Bone>& GetBones() { return bones; }
	//getter
	FbxScene* GetFbxScene() { return fbxScene; }
	//デストラクタ
	~FbxModel();

};