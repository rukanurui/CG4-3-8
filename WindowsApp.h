#include <Windows.h>

#pragma once
class WindowsApp
{
public://メンバ変数
	static LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	//getter
	HWND GetHwnd() { return hwnd; }
	//getter
	HINSTANCE GetHInstance() { return w.hInstance;}
	
private:
	//ウインドウハンドル
	HWND hwnd = nullptr;
	//ウインドウクラスの設定
	WNDCLASSEX w{};
public: //メンバ関数
//初期化
	void Initialize();
	//更新
	void Update();
	//終了
	void Finalize();
	//メッセージの処理
	bool ProcessMessage();
public://定数
	// ウィンドウサイズ
	static const int window_width = 1280;  // 横幅
	static const int window_height = 720;  // 縦幅
};

