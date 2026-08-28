#include "Engine.h"
#include "Logger.h"
#include <d3d11.h>
#include <dxgi.h>

namespace TechVideoEditor {

Engine::Engine()
    : m_hwnd(nullptr)
    , m_device(nullptr)
    , m_context(nullptr)
    , m_recording(false)
    , m_playing(false)
    , m_paused(false)
    , m_initialized(false)
    , m_lastFrameTime(0)
{
    QueryPerformanceFrequency(&m_frequency);
}

Engine::~Engine() {
    Shutdown();
}

bool Engine::Initialize(HWND hwnd, const EngineConfig& config) {
    m_hwnd = hwnd;
    m_config = config;
    
    // Initialize logger
    Logger::Instance().Initialize(L"TechVideoEditor.log");
    LOG_INFO("Engine initializing...");
    
    // Get D3D11 device from window (assumes ImGui DX11 is already initialized)
    // In practice, this would be passed from the UI initialization
    // For now, we create our own device
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };
    
    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        featureLevels,
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,
        &m_device,
        &featureLevel,
        &m_context
    );
    
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create D3D11 device: " + std::to_string(hr));
        return false;
    }
    
    // Initialize components
    m_screenCapture = std::make_unique<ScreenCapture>();
    if (!m_screenCapture->Initialize()) {
        LOG_ERROR("Failed to initialize screen capture");
        return false;
    }
    
    m_audioMixer = std::make_unique<AudioMixer>();
    if (!m_audioMixer->Initialize(48000, 2)) {
        LOG_WARNING("Failed to initialize audio mixer - audio will be disabled");
    }
    
    m_wasapiCapture = std::make_unique<WASAPICapture>();
    // WASAPI capture can fail on some systems, don't treat as fatal
    
    m_keyHook = std::make_unique<KeyHook>();
    if (!m_keyHook->Install()) {
        LOG_WARNING("Failed to install keyboard hook - key overlay disabled");
    } else {
        m_keyHook->SetOverlayEnabled(config.autoKeyOverlay);
    }
    
    m_timeline = std::make_unique<Timeline>();
    
    // Setup timeline tracks
    m_timeline->AddTrack("Video");
    m_timeline->AddTrack("Audio/TTS");
    m_timeline->AddTrack("Text/Overlays");
    
    m_uiManager = std::make_unique<UIManager>();
    if (!m_uiManager->Initialize(hwnd, m_device, m_context)) {
        LOG_ERROR("Failed to initialize UI manager");
        return false;
    }
    
    m_frameProcessor = std::make_unique<FrameProcessor>();
    
    EncodingSettings encodingSettings;
    encodingSettings.width = config.captureWidth;
    encodingSettings.height = config.captureHeight;
    encodingSettings.fps = config.fps;
    encodingSettings.bitrate = config.bitrate;
    encodingSettings.hardwareAcceleration = config.hardwareAcceleration;
    encodingSettings.outputPath = config.defaultOutputPath;
    
    if (!m_frameProcessor->Initialize(encodingSettings)) {
        LOG_WARNING("Frame processor initialization deferred until recording starts");
    }
    
    m_initialized = true;
    LOG_INFO("Engine initialized successfully");
    
    return true;
}

void Engine::Shutdown() {
    LOG_INFO("Engine shutting down...");
    
    StopRecording();
    StopPlayback();
    
    if (m_keyHook) {
        m_keyHook->Remove();
    }
    
    m_frameProcessor.reset();
    m_uiManager.reset();
    m_timeline.reset();
    m_wasapiCapture.reset();
    m_audioMixer.reset();
    m_screenCapture.reset();
    
    if (m_context) {
        m_context->ClearState();
        m_context->Flush();
    }
    
    // Release D3D device last
    if (m_context) m_context->Release();
    if (m_device) m_device->Release();
    
    Logger::Instance().Shutdown();
    m_initialized = false;
}

void Engine::Update() {
    if (!m_initialized) return;
    
    // Update playhead if playing
    if (m_playing && !m_paused) {
        UpdatePlayhead();
    }
    
    // Process captured frames during recording
    if (m_recording) {
        ProcessCapturedFrame();
    }
    
    // Update UI
    if (m_uiManager) {
        m_uiManager->BeginFrame();
    }
}

void Engine::Render() {
    if (!m_initialized || !m_uiManager) return;
    
    // Render preview texture
    RenderPreviewTexture();
    
    // Render UI
    m_uiManager->RenderEditorUI();
    m_uiManager->EndFrame();
}

bool Engine::StartRecording() {
    if (m_recording) return true;
    
    LOG_INFO("Starting recording...");
    
    // Initialize frame processor with current settings
    EncodingSettings settings;
    settings.width = m_config.captureWidth;
    settings.height = m_config.captureHeight;
    settings.fps = m_config.fps;
    settings.bitrate = m_config.bitrate;
    settings.hardwareAcceleration = m_config.hardwareAcceleration;
    settings.outputPath = m_config.defaultOutputPath;
    
    if (!m_frameProcessor->Initialize(settings)) {
        LOG_ERROR("Failed to initialize frame processor for recording");
        return false;
    }
    
    if (!m_frameProcessor->StartEncoding()) {
        LOG_ERROR("Failed to start encoding");
        return false;
    }
    
    // Enable dirty frame detection if configured
    if (m_config.dirtyFrameSkipping) {
        m_screenCapture->EnableDirtyFrameDetection(true);
    }
    
    m_recording = true;
    QueryPerformanceCounter(&m_frequency);
    m_lastFrameTime = 0;
    
    LOG_INFO("Recording started");
    return true;
}

void Engine::StopRecording() {
    if (!m_recording) return;
    
    LOG_INFO("Stopping recording...");
    
    m_recording = false;
    
    if (m_frameProcessor) {
        m_frameProcessor->StopEncoding();
        
        auto stats = m_frameProcessor->GetStatistics();
        LOG_INFO("Recording complete. Frames processed: " + std::to_string(stats.framesProcessed));
    }
}

bool Engine::StartPlayback() {
    if (m_playing) return true;
    
    LOG_INFO("Starting playback...");
    
    m_playing = true;
    m_paused = false;
    
    if (m_audioMixer) {
        m_audioMixer->StartPlayback();
    }
    
    return true;
}

void Engine::StopPlayback() {
    if (!m_playing) return;
    
    LOG_INFO("Stopping playback...");
    
    m_playing = false;
    m_paused = false;
    
    if (m_audioMixer) {
        m_audioMixer->StopPlayback();
    }
    
    // Reset playhead
    if (m_timeline) {
        m_timeline->SetPlayheadPosition(0);
    }
}

void Engine::PausePlayback(bool pause) {
    m_paused = pause;
    
    if (m_audioMixer) {
        m_audioMixer->PausePlayback(pause);
    }
}

void Engine::ProcessCapturedFrame() {
    auto frame = m_screenCapture->CaptureFrame();
    if (!frame) return;
    
    // Skip dirty frames if enabled
    if (m_config.dirtyFrameSkipping && !frame->isDirty) {
        return;
    }
    
    // Submit to frame processor
    if (m_frameProcessor && m_frameProcessor->IsEncoding()) {
        m_frameProcessor->SubmitFrame(frame->texture.Get(), frame->timestamp);
    }
}

void Engine::UpdatePlayhead() {
    if (!m_timeline) return;
    
    int64_t currentTime = m_timeline->GetPlayheadPosition();
    int64_t deltaTime = static_cast<int64_t>(1e9 / m_config.fps);  // Nanoseconds per frame
    
    currentTime += deltaTime;
    
    if (currentTime >= m_timeline->GetDuration()) {
        // End of timeline
        StopPlayback();
        m_timeline->SetPlayheadPosition(0);
    } else {
        m_timeline->SetPlayheadPosition(currentTime);
    }
}

void Engine::RenderPreviewTexture() {
    // During playback, render the current timeline frame
    // During recording, show the live capture
    // This is a simplified implementation
    
    if (m_recording && m_screenCapture && m_screenCapture->IsInitialized()) {
        // Show live capture in preview
        // Actual implementation would get the latest frame texture
    } else if (m_playing && m_timeline) {
        // Render timeline preview at current playhead position
        auto clips = m_timeline->GetClipsAtTime(m_timeline->GetPlayheadPosition());
        
        // Render video clips to preview texture
        // This would use D3D11 to composite all visible clips
    }
}

} // namespace TechVideoEditor
