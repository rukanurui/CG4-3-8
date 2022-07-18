#pragma once

#include <DirectXMath.h>
using namespace DirectX;

class Camera
{
private:
	// DirectX::を省略
	using XMFLOAT2 = DirectX::XMFLOAT2;
	using XMFLOAT3 = DirectX::XMFLOAT3;
	using XMFLOAT4 = DirectX::XMFLOAT4;
	using XMVECTOR = DirectX::XMVECTOR;
	using XMMATRIX = DirectX::XMMATRIX;


public: // メンバ関数

	//初期化
	void Initialize(int window_width, int window_height);
	//更新
	void Update(int window_width, int window_height);
	void UpdateViewMatrix();
	void UpdateProjectionMatrix(int window_width, int window_height);
	void MoveVector(XMFLOAT3 move);

	//Getter
	//ビュー行列関連
	const XMMATRIX& GetViewMatrix() {return matView;}

	/// 視点座標の取得
	const XMFLOAT3& GetEye() { return eye; }

	/// 注視点座標の取得
	const XMFLOAT3& GetTarget() { return target; }

	const XMFLOAT3& GetUp() {return up;}


	//プロジェクション行列関連

	const XMMATRIX& GetProjectionMatrix() {return matProjection;}

	const XMMATRIX& GetViewProjectionMatrix() {return matViewProjection;}


	//Setter
	//ビュー行列関連

	/// 視点座標の設定
	void SetEye(XMFLOAT3 eye);

	/// 注視点座標の設定
	void SetTarget(XMFLOAT3 target);

	void SetUp(XMFLOAT3 up);

	void SetViewMatrix(XMMATRIX viewmatrix);




private://メンバ変数
	// ビュー行列
	static XMMATRIX matView;
	// 射影行列
	static XMMATRIX matProjection;
	// ビュープロジェクション行列
	static XMMATRIX matViewProjection;
	// 視点座標
	static XMFLOAT3 eye;
	// 注視点座標
	static XMFLOAT3 target;
	// 上方向ベクトル
	static XMFLOAT3 up;
	// アスペクト比
	float aspectRatio = 1.0f;

};

