#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <mfapi.h>
#include <mfreadwrite.h>
#include <mftransform.h>
#include <wrl/client.h>
#include <string>
#include <memory>
#include <queue>

using Microsoft::WRL::ComPtr;

namespace TechVideoEditor {

struct EncodingSettings {
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    uint32_t bitrate;        // Bits per second
    bool hardwareAcceleration;
    std::wstring outputPath;
};

class FrameProcessor {
public:
    FrameProcessor();
    ~FrameProcessor();

    // Initialize with encoding settings
    bool Initialize(const EncodingSettings& settings);
    
    // Submit a frame for processing (D3D11 texture)
    bool SubmitFrame(ID3D11Texture2D* texture, uint64_t timestamp);
    
    // Start/stop encoding
    bool StartEncoding();
    void StopEncoding();
    bool IsEncoding() const { return m_encoding; }
    
    // Pause/resume encoding (for dirty frame skipping)
    void PauseEncoding(bool pause);
    bool IsPaused() const { return m_paused; }
    
    // Get statistics
    struct Stats {
        uint64_t framesProcessed;
        uint64_t framesDropped;
        double averageFPS;
        size_t outputSizeBytes;
    };
    Stats GetStatistics() const;
    
    // Hardware conversion: D3D11 texture -> NV12 for Media Foundation
    bool ConvertTextureToNV12(ID3D11Texture2D* input, IMFSample** output);

private:
    bool CreateSinkWriter();
    bool ConfigureVideoFormat();
    bool EnableHardwareTransforms();
    
    ComPtr<ID3D11Device> m_d3dDevice;
    ComPtr<ID3D11DeviceContext> m_d3dContext;
    ComPtr<IMFSinkWriter> m_sinkWriter;
    ComPtr<ID3D11Texture2D> m_stagingTexture;
    
    EncodingSettings m_settings;
    
    std::queue<ComPtr<ID3D11Texture2D>> m_frameQueue;
    std::mutex m_queueMutex;
    
    bool m_initialized = false;
    bool m_encoding = false;
    bool m_paused = false;
    bool m_hardwareEnabled = false;
    
    // Statistics
    uint64_t m_framesProcessed = 0;
    uint64_t m_framesDropped = 0;
    LARGE_INTEGER m_startTime;
    LARGE_INTEGER m_frequency;
};

} // namespace TechVideoEditor
