#pragma once
#include "DebugText.h"
#include "2d/Sprite.h"
#include <string>
#include <Windows.h>

class DebugText
{
public: // 定数の宣言    
    static const int maxCharCount = 256;    // 最大文字数
    static const int fontWidth = 9;         // フォント画像内1文字分の横幅
    static const int fontHeight = 18;       // フォント画像内1文字分の縦幅
    static const int fontLineCount = 14;    // フォント画像内1行分の文字数

public: // メンバ関数
    void Initialize(SpriteCommon* spriteCommon, UINT texnumber);

    void Print(const std::string& text, float x, float y, float scale = 1.0f);

    void DrawAll();

    void Finalize();

private: // メンバ変数   
    SpriteCommon* spriteCommona = nullptr;
    // スプライトデータの配列
    Sprite* spritesa[maxCharCount];
    // スプライトデータ配列の添え字番号
    int spriteIndexa = 0;
};