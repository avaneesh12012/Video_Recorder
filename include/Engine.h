#pragma once

#include "ScreenCapture.h"
#include "FrameProcessor.h"
#include "AudioMixer.h"
#include "WASAPICapture.h"
#include "KeyHook.h"
#include "Timeline.h"
#include "UIManager.h"

#include <memory>
#include <atomic>
#include <thread>

namespace TechVideoEditor {

struct EngineConfig {
    uint32_t captureWidth = 1920;
    uint32_t captureHeight = 1080;
    uint32_t fps = 60;
    uint32_t bitrate = 8000000;  // 8 Mbps
    bool hardwareAcceleration = true;
    bool dirtyFrameSkipping = true;
    bool autoKeyOverlay = true;
    std::wstring defaultOutputPath = L"output.mp4";
};

class Engine {
public:
    Engine();
    ~Engine();

    // Initialize the entire engine
    bool Initialize(HWND hwnd, const EngineConfig& config = EngineConfig());
    
    // Shutdown
    void Shutdown();
    
    // Main loop (call from message pump)
    void Update();
    void Render();
    
    // Recording control
    bool StartRecording();
    void StopRecording();
    bool IsRecording() const { return m_recording; }
    
    // Playback control (timeline preview)
    bool StartPlayback();
    void StopPlayback();
    void PausePlayback(bool pause);
    bool IsPlaying() const { return m_playing; }
    
    // Component access
    ScreenCapture* GetScreenCapture() { return m_screenCapture.get(); }
    AudioMixer* GetAudioMixer() { return m_audioMixer.get(); }
    KeyHook* GetKeyHook() { return m_keyHook.get(); }
    Timeline* GetTimeline() { return m_timeline.get(); }
    UIManager* GetUI() { return m_uiManager.get(); }
    FrameProcessor* GetFrameProcessor() { return m_frameProcessor.get(); }
    
    // Configuration
    void SetConfig(const EngineConfig& config) { m_config = config; }
    const EngineConfig& GetConfig() const { return m_config; }

private:
    void ProcessCapturedFrame();
    void UpdatePlayhead();
    void RenderPreviewTexture();
    
    std::unique_ptr<ScreenCapture> m_screenCapture;
    std::unique_ptr<FrameProcessor> m_frameProcessor;
    std::unique_ptr<AudioMixer> m_audioMixer;
    std::unique_ptr<WASAPICapture> m_wasapiCapture;
    std::unique_ptr<KeyHook> m_keyHook;
    std::unique_ptr<Timeline> m_timeline;
    std::unique_ptr<UIManager> m_uiManager;
    
    EngineConfig m_config;
    
    HWND m_hwnd;
    ID3D11Device* m_device;
    ID3D11DeviceContext* m_context;
    
    std::atomic<bool> m_recording;
    std::atomic<bool> m_playing;
    std::atomic<bool> m_paused;
    
    std::thread m_captureThread;
    bool m_initialized;
    
    // Timing
    LARGE_INTEGER m_frequency;
    int64_t m_lastFrameTime;
};

} // namespace TechVideoEditor
