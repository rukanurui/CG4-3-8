#include "DebugText.h"


void DebugText::Initialize(SpriteCommon* spriteCommon, UINT texnumber)
{
    assert(spriteCommon);
    spriteCommona = spriteCommon;

    // 全てのスプライトデータについて
    for (int i = 0; i < _countof(spritesa); i++)
    {
        // スプライトを生成する
        spritesa[i] = Sprite::Create(spriteCommona, texnumber, { 0,0 });
    }
}

void DebugText::Print(const std::string& text, float x, float y, float scale)
{
    // 全ての文字について
    for (int i = 0; i < text.size(); i++)
    {
        // 最大文字数超過
        if (spriteIndexa >= maxCharCount) {
            break;
        }

        // 1文字取り出す(※ASCIIコードでしか成り立たない)
        const unsigned char& character = text[i];

        // ASCIIコードの2段分飛ばした番号を計算
        int fontIndex = character - 32;
        if (character >= 0x7f) {
            fontIndex = 0;
        }

        int fontIndexY = fontIndex / fontLineCount;
        int fontIndexX = fontIndex % fontLineCount;

        // 座標計算

        spritesa[spriteIndexa]->SetPosition({ x + fontWidth * scale * i, y, 0 });
        spritesa[spriteIndexa]->SetTexLeftTop({ (float)fontIndexX * fontWidth, (float)fontIndexY * fontHeight });
        spritesa[spriteIndexa]->SetTexSize({ fontWidth, fontHeight });
        spritesa[spriteIndexa]->SetSize({ fontWidth * scale, fontHeight * scale });
        // 頂点バッファ転送
        spritesa[spriteIndexa]->TransferVertexBuffer();
        // 更新
        spritesa[spriteIndexa]->Update();

        // 文字を１つ進める
        spriteIndexa++;
    }
}

// まとめて描画
void DebugText::DrawAll()
{
    // 全ての文字のスプライトについて
    for (int i = 0; i < spriteIndexa; i++)
    {
        // スプライト描画
        spritesa[spriteIndexa]->Draw();
    }

    spriteIndexa = 0;
}

void DebugText::Finalize()
{
    for (auto& sprite : spritesa) {
        delete sprite;
    }
}
