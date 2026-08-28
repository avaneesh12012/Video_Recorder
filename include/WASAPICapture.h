#pragma once

#include <Windows.h>
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <wrl/client.h>
#include <vector>
#include <memory>

using Microsoft::WRL::ComPtr;

namespace TechVideoEditor {

class WASAPICapture {
public:
    WASAPICapture();
    ~WASAPICapture();

    // Initialize for loopback capture (system audio)
    bool InitializeLoopback();
    
    // Initialize for microphone capture
    bool InitializeMicrophone(const wchar_t* deviceName = nullptr);
    
    // Start/stop capture
    bool StartCapture();
    void StopCapture();
    bool IsCapturing() const { return m_capturing; }
    
    // Get captured audio data (called by AudioMixer)
    std::vector<float> GetAudioBuffer(size_t maxFrames = 1024);
    
    // Device enumeration
    struct DeviceInfo {
        std::wstring name;
        std::wstring id;
        bool isDefault;
    };
    static std::vector<DeviceInfo> EnumeratePlaybackDevices();
    static std::vector<DeviceInfo> EnumerateCaptureDevices();

private:
    bool InitializeDevice(bool loopback);
    static DWORD WINAPI CaptureThread(LPVOID lpParameter);
    void ProcessAudioData(const float* data, size_t frameCount);
    
    ComPtr<IMMDeviceEnumerator> m_enumerator;
    ComPtr<IMMDevice> m_device;
    ComPtr<IAudioClient> m_audioClient;
    ComPtr<IAudioCaptureClient> m_captureClient;
    
    WAVEFORMATEX m_waveFormat;
    HANDLE m_captureEvent;
    HANDLE m_threadHandle;
    
    std::vector<float> m_audioBuffer;
    std::mutex m_bufferMutex;
    
    bool m_initialized = false;
    bool m_capturing = false;
    bool m_loopback = false;
};

} // namespace TechVideoEditor
