#include "Audio.h"
#include <Windows.h>
#pragma comment(lib,"xaudio2.lib")
#include <cassert>
#include <fstream>

void Audio::Initialize(const std::string directoryPath)
{
    directoryPatha = directoryPath;
    HRESULT result;
    IXAudio2MasteringVoice* masterVoice;

    // XAudioエンジンのインスタンスを生成
    result = XAudio2Create(&xAudio2a, 0, XAUDIO2_DEFAULT_PROCESSOR);
    assert(SUCCEEDED(result));

    // マスターボイスを生成
    result = xAudio2a->CreateMasteringVoice(&masterVoice);
    assert(SUCCEEDED(result));
}

void Audio::Finalize()
{
       //XAudio2解放
       xAudio2a.Reset();
       // 音声データ解放
       std::map< std::string, SoundData>::iterator it = soundDatasa.begin();
       for (;it !=soundDatasa.end();++it)
       {

           Unload(&it->second);
       }
       soundDatasa.clear();

}

void Audio::LoadWave(const std::string filename)
{
    if (soundDatasa.find(filename) != soundDatasa.end())
    {
        //重複読み込みなので、何もせず抜ける
        return;
    }

    std::string fullpath = directoryPatha + filename;

    // ファイル入力ストリームのインスタンス
    std::ifstream file;
    // .wavファイルをバイナリモードで開く
    file.open(filename, std::ios_base::binary);
    // ファイルオープン失敗を検出する
    assert(file.is_open());

    // RIFFヘッダーの読み込み
    RiffHeader riff;
    file.read((char*)&riff, sizeof(riff));
    // ファイルがRIFFかチェック
    if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
        assert(0);
    }
    // タイプがWAVEかチェック
    if (strncmp(riff.type, "WAVE", 4) != 0) {
        assert(0);
    }

    // Formatチャンクの読み込み
    FormatChunk format = {};
    // チャンクヘッダーの確認
    file.read((char*)&format, sizeof(ChunkHeader));
    if (strncmp(format.chunk.id, "fmt ", 4) != 0) {
        assert(0);
    }
    // チャンク本体の読み込み
    assert(format.chunk.size <= sizeof(format.fmt));
    file.read((char*)&format.fmt, format.chunk.size);

    // Dataチャンクの読み込み
    ChunkHeader data;
    file.read((char*)&data, sizeof(data));
    // JUNKチャンクを検出した場合
    if (strncmp(data.id, "JUNK ", 4) == 0) {
        // 読み取り位置をJUNKチャンクの終わりまで進める
        file.seekg(data.size, std::ios_base::cur);
        // 再読み込み
        file.read((char*)&data, sizeof(data));
    }

    if (strncmp(data.id, "data ", 4) != 0) {
        assert(0);
    }

    // Dataチャンクのデータ部（波形データ）の読み込み
    char* pBuffer = new char[data.size];
    file.read(pBuffer, data.size);

    // Waveファイルを閉じる
    file.close();

    // returnする為の音声データ
    SoundData soundData = {};

    soundData.wfex = format.fmt;
    soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
    soundData.bufferSize = data.size;

    //サウンドデータを連想配列に格納
    soundDatasa.insert(std::make_pair(filename, soundData));
}

void Audio::Unload(SoundData* sounddata)
{
    // バッファのメモリを解放
    delete[] sounddata->pBuffer;

    sounddata->pBuffer = 0;
    sounddata->bufferSize = 0;
    sounddata->wfex = {};
}

void Audio::PlayWave(const std::string filename)
{
    HRESULT result;

    std::map< std::string, SoundData>::iterator it = soundDatasa.find(filename);
    assert(it != soundDatasa.end());

    SoundData& soundData = it->second;

    // 波形フォーマットを元にSourceVoiceの生成
    IXAudio2SourceVoice* pSourceVoice = nullptr;
    result = xAudio2a->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
    assert(SUCCEEDED(result));

    // 再生する波形データの設定
    XAUDIO2_BUFFER buf{};
    buf.pAudioData = soundData.pBuffer;
    buf.AudioBytes = soundData.bufferSize;
    buf.Flags = XAUDIO2_END_OF_STREAM;

    // 波形データの再生
    result = pSourceVoice->SubmitSourceBuffer(&buf);
    result = pSourceVoice->Start();
}
