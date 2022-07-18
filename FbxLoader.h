#pragma once

#include "fbxsdk.h"
#include <d3d12.h>
#include <d3dx12.h>
#include <string>
#include "Modelfbx.h"
#include<cassert>

class FbxLoader
{
public:
	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	/// <returns>インスタンス</returns>
	static FbxLoader* GetInstance();
	//初期化
	void Initialize(ID3D12Device* device);
	void Finalize();
	//好き人ぐ情報の読み取り
	void ParseSkin(FbxModel* model, FbxMesh* fbxMesh);


private:
	// privateなコンストラクタ（シングルトンパターン）
	FbxLoader() = default;
	// privateなデストラクタ（シングルトンパターン）
	~FbxLoader() = default;
	// コピーコンストラクタを禁止（シングルトンパターン）
	FbxLoader(const FbxLoader& obj) = delete;
	// コピー代入演算子を禁止（シングルトンパターン）
	void operator=(const FbxLoader& obj) = delete;
	//std::を省略
	using string = std::string;

	ID3D12Device* device = nullptr;

	FbxManager* fbxManager = nullptr;
	FbxImporter* fbxImporter = nullptr;

public:
	//モデル格納ルートパス
	static const string baseDirectory;
	//ファイルからFBXモデル読み込み
	FbxModel* LoadModelFromFile(const string& modelName);
	//再帰的にノード構成を解析
	void ParseNodeRecursive(FbxModel* model, FbxNode* fbxNode, Node* parent = nullptr);
	//メッシュ読み取り
	void ParseMesh(FbxModel* model, FbxNode* fbxNode);
	//頂点座標読み取り
	void ParseMeshVertices(FbxModel* model, FbxMesh* fbxMesh);
	//面情報読み取り
	void ParceMeshFaces(FbxModel* model, FbxMesh* fbxMesh);
	//マテリアル読み取り
	void ParseMaterial(FbxModel* model, FbxNode* fbxNode);
	//テクスチャ読み込み
	void LoadTexture(FbxModel* model, const std::string& fullpath);
	//ディレクトリを含んだファイルパスからファイル名を抽出する
	std::string ExtractFileName(const std::string& path);
	//テクスチャがない場合の標準テクスチャ名
	static const string defaultTextureFileName;
	//FBXの行列をXMATRIXに変換
	static void ConvertMatrixFromFbx(DirectX::XMMATRIX* dst, const FbxMatrix& src);
};