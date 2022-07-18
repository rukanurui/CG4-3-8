#pragma once
#include <xaudio2.h>

#include <cstdint>
#include <wrl.h>
#include <map>
#include <string>

class Audio
{
public:
    // チャンクヘッダ
    struct ChunkHeader
    {
        char id[4]; // チャンク毎のID
        int32_t size;  // チャンクサイズ
    };

    // RIFFヘッダチャンク
    struct RiffHeader
    {
        ChunkHeader chunk;   // "RIFF"
        char type[4]; // "WAVE"
    };

    // FMTチャンク
    struct FormatChunk
    {
        ChunkHeader chunk; // "fmt "
        WAVEFORMATEX fmt; // 波形フォーマット
    };

    class XAudio2VoiceCallback : public IXAudio2VoiceCallback
    {
    public:
        // ボイス処理パスの開始時
        STDMETHOD_(void, OnVoiceProcessingPassStart) (THIS_ UINT32 BytesRequired) {};
        // ボイス処理パスの終了時
        STDMETHOD_(void, OnVoiceProcessingPassEnd) (THIS) {};
        // バッファストリームの再生が終了した時
        STDMETHOD_(void, OnStreamEnd) (THIS) {};
        // バッファの使用開始時
        STDMETHOD_(void, OnBufferStart) (THIS_ void* pBufferContext) {};
        // バッファの末尾に達した時
        STDMETHOD_(void, OnBufferEnd) (THIS_ void* pBufferContext) {};
        // 再生がループ位置に達した時
        STDMETHOD_(void, OnLoopEnd) (THIS_ void* pBufferContext) {};
        // ボイスの実行エラー時
        STDMETHOD_(void, OnVoiceError) (THIS_ void* pBufferContext, HRESULT Error) {};
    };

    // 音声データ
    struct SoundData
    {
        // 波形フォーマット
        WAVEFORMATEX wfex;
        // バッファの先頭アドレス
        BYTE* pBuffer;
        // バッファのサイズ
        unsigned int bufferSize;
    };

    void Initialize(const std::string directoryPath="Resources/");

    void Finalize();

    void LoadWave(const std::string filename);

    void Unload(SoundData* sounddata);

    void PlayWave(const std::string filename);

private:
    //Xaudio2のインスタンス
    Microsoft::WRL::ComPtr<IXAudio2> xAudio2a;

    std::map<std::string, SoundData> soundDatasa;
    //サウンド格納ディレクトリ
    std::string directoryPatha;
};

