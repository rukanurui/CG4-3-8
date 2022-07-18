#include <Windows.h>
#include <wrl.h>
#include "WindowsApp.h"

#define DIRECTINPUT_VERSION     0x0800   // DirectInputのバージョン指定
#include <dinput.h>
#pragma once
class Input
{
public:
	template<class T>using ComPtr = Microsoft::WRL::ComPtr<T>;
public: //メンバ関数
	//初期化
	void Initialize(WindowsApp* winApp);
	//更新
	void Update();

	static Input* GetInstance();

/// <summary>
/// キーの押下をチェック
/// </summary>
/// <param name = "keyNumber">キー番号(DIK_0など)</param>
/// <returns>押されているか</returns>
	bool PushKey(BYTE keyNumber);

/// <summary>
/// キーのトリガーをチェック
/// </summary>
/// <param name = "keyNumber">キー番号(DIK_0など)</param>
/// <returns>トリガーか</returns>
	bool TriggerKey(BYTE keyNumber);

private: //メンバ変数
	//キーボードのデバイス
	ComPtr<IDirectInputDevice8> devkeyboard;
	//全キーの入力情報を取得する
	BYTE key[256] = {};
	//前回の全キーの入力情報を取得する
	BYTE oldkey[256] = {};
	//インスタンス生成
	ComPtr<IDirectInput8> dinput = nullptr;
	//WindowsAPI
	WindowsApp* winApp = nullptr;
};

