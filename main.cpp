
#include <vector>
#include <string>


#define DIRECTINPUT_VERSION     0x0800   // DirectInputのバージョン指定

#include "Input.h"

#include "WindowsApp.h"
#include "DXCommon.h"

#include "Object3d.h"
#include "model.h"
#include "2d/SpriteCommon.h"
#include "2d/Sprite.h"
#include"2d/PostEffect.h"
#include "DebugText.h"
#include "Audio.h"

#include "fbxsdk.h"
#include "FbxLoader.h"

#include "FBXobj3d.h"
#include "Modelfbx.h"

#include "Camera.h"



using namespace DirectX;
using namespace Microsoft::WRL;


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{

   
    //ポインタ置き場
    Input* input = nullptr;
    WindowsApp* winApp = nullptr;
    DXCommon* dxCommon = nullptr;
    FbxModel* model1 = nullptr;
    FBXobj3d* object1 = nullptr;
    Camera* camera = nullptr;

   /* Audio* audio = nullptr;*/

#pragma region WindowsAPI初期化
    winApp = new WindowsApp();
    winApp->Initialize();
#pragma endregion WindowsAPI初期化

#pragma region DirectX初期化処理
    // DirectX初期化処理　ここから
    dxCommon = new DXCommon();
    dxCommon->Initialize(winApp);


    //Object3d静的初期化
    Object3d::StaticInitialize(dxCommon->GetDevice(), WindowsApp::window_width, WindowsApp::window_height);
    //OBJからモデルデータを読み込む
    model* Model = model::LoadFromOBJ("triangle_mat");
    //3dオブジェクト生成
    Object3d* object3d = Object3d::Create();
    //モデル紐づけ
    object3d->SetModel(Model);

    // スプライト共通部分の初期化
    SpriteCommon* spriteCommon = new SpriteCommon();
    spriteCommon->Initialize(dxCommon->GetDevice(),dxCommon->GetCommandList(),winApp->window_width,winApp->window_height);

    // デバッグテキスト
    DebugText* debugText = nullptr;
    debugText = new DebugText();

    // デバッグテキスト用のテクスチャ番号を指定
    const int debugTextTexNumber = 2;
    // デバッグテキスト用のテクスチャ読み込み
    spriteCommon->LoadTexture(debugTextTexNumber, L"Resources/debugfont.png");
    // デバッグテキスト初期化
    debugText->Initialize(spriteCommon, debugTextTexNumber);

    //入力の初期化
    input = new Input();
    input->Initialize(winApp);


   //オーディオの初期化
   /* audio = new Audio();
    audio->Initialize();*/
  
  /*  XAudio2VoiceCallback voiceCallback;*/

    

    // 音声読み込み
    // audio->LoadWave("Alarm01.wav");

    //// 音声再生
    //  audio->PlayWave("Alarm01.wav");

    //カメラ生成
    camera->Initialize(winApp->window_width, winApp->window_height);


    camera->UpdateViewMatrix();
    camera->Update(winApp->window_width, winApp->window_height);


    // DirectX初期化処理　ここまで
#pragma endregion DirectX初期化処理

#pragma region 描画初期化処理
  

    //// スプライト共通データ生成
    //spriteCommon = SpriteCommonCreate(dxCommon->GetDevice(), WinApp::window_width, WinApp::window_height);
    // スプライト共通テクスチャ読み込み
    spriteCommon->LoadTexture(0, L"Resources/texture.png");
    spriteCommon->LoadTexture(1, L"Resources/house.png");

    std::vector<Sprite*> sprites;

   /* Sprite* sprite = Sprite::Create(spriteCommon, 0);*/
   

    // スプライトの生成
    for (int i = 0; i < 20; i++)
    {
        int texNumber = 0;
        Sprite* sprite = Sprite::Create(spriteCommon,texNumber, { 0,0 }, false, false);

        // スプライトの座標変更
        sprite->SetPosition({ (float)(rand() % 1280) ,(float)(rand() % 720) ,0 });
        sprite->SetRotation({ (float)(rand() % 360) });
        sprite->SetSize({ (float)(rand() % 400), (float)(rand() % 100) });
        //sprites[i].isInvisible = true;
  
        // 頂点バッファに反映
        sprite->TransferVertexBuffer();

        sprites.push_back(sprite);
        sprite->SetPosition({ 500,300,0 });
    }

    //ポストエフェクト用テクスチャの読み込み
    spriteCommon->LoadTexture(100,L"Resources/White1x1.png");
    int PFnum = 100;
    //ポストエフェクトの初期化
    PostEffect* postEffect = PostEffect::Create(spriteCommon, PFnum, { 0,0 }, false, false);
    //postEffect->SetSize({ (float)500.0f,500.0f });
    //postEffect->TransferVertexBuffer();



    FbxLoader::GetInstance()->Initialize(dxCommon->GetDevice());
    //デバイスをセット
    FBXobj3d::SetDevice(dxCommon->GetDevice());
    //カメラをセット
    FBXobj3d::SetCamera(camera);
    camera->SetEye({ 0,10.0f,10.0f });

    FBXobj3d::CreateGraphicsPipeline();

    //FbxLoader::GetInstance()->LoadModelFromFile("cube");
    model1=FbxLoader::GetInstance()->LoadModelFromFile("boneTest");

    object1 = new FBXobj3d;
    object1->Initialize();
    object1->SetModel(model1);
   

    
  
#pragma endregion 描画初期化処理

    int counter = 0; // アニメーションの経過時間カウンター

    object1->PlayAnimation();

    while (true)  // ゲームループ
    {
#pragma region ウィンドウメッセージ処理
        if (winApp->ProcessMessage())
        {
            break;
        }

#pragma endregion ウィンドウメッセージ処理

#pragma region DirectX毎フレーム処理
        // DirectX毎フレーム処理　ここから

        //入力の更新
        input->Update();

        //object3d更新
        object3d->Update();

        //スプライト更新
        for (auto& sprite : sprites) {
            sprite->Update();
        }

        object1->Update();


        // DirectX毎フレーム処理　ここまで
#pragma endregion DirectX毎フレーム処理

#pragma region グラフィックスコマンド

        // コマンドリストの取得
        ID3D12GraphicsCommandList* cmdList = dxCommon->GetCommandList();



        //レンダ―テクスチャへの描画
        postEffect->PreDrawScene(dxCommon->GetCommandList());

        

        //3dオブジェクト描画前処理
        Object3d::PreDraw(dxCommon->GetCommandList());


        //3dオブジェクトの描画
        //object3d->Draw();

        object1->Draw(cmdList);

        postEffect->PostDrawScene(dxCommon->GetCommandList());

        //描画前処理
        dxCommon->PreDraw();

        //3dオブジェクトの描画後処理
        Object3d::PostDraw();

        // ４．描画コマンドここから
       

        // スプライト共通コマンド
        spriteCommon->PreDraw();

        //ポストエフェクトの描画
        postEffect->Draw(dxCommon->GetCommandList());

        // スプライト描画

        /*for (auto& sprite : sprites) {
            sprite->Draw();
        }*/

        // デバッグテキスト描画
        debugText->DrawAll();

        // ４．描画コマンドここまで
         //描画後処理
        dxCommon->PostDraw();

    }
        //解放処理
        delete input;
        delete dxCommon;

        //3dオブジェクト解放
        delete object3d;
        //3dモデル解放
        delete Model;

        //デバッグテキスト解放
        debugText->Finalize();
        delete debugText;

        //スプライト解放
        for (auto& sprite : sprites) {
            delete sprite;
        }
        sprites.clear();
        delete spriteCommon;


        //オーディオ解放
       /* audio->Finalize();
        delete audio;*/

       // FbxLoader::GetInstance()->Finalize();

        delete object1;
        delete model1;
        delete postEffect;

        winApp->Finalize();
        delete winApp;
        winApp = nullptr;
#pragma endregion WindowsAPI後始末

        return 0;
 }