#include "FrameProcessor.h"
#include "Logger.h"

namespace TechVideoEditor {

FrameProcessor::FrameProcessor() {
    QueryPerformanceFrequency(&m_frequency);
}

FrameProcessor::~FrameProcessor() {
    StopEncoding();
}

bool FrameProcessor::Initialize(const EncodingSettings& settings) {
    m_settings = settings;
    
    // Create D3D11 device if not provided
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
        &m_d3dDevice,
        &featureLevel,
        &m_d3dContext
    );
    
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create D3D11 device for frame processor");
        return false;
    }
    
    // Create staging texture for CPU access if needed
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = settings.width;
    texDesc.Height = settings.height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_NV12;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_STAGING;
    texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    hr = m_d3dDevice->CreateTexture2D(&texDesc, nullptr, &m_stagingTexture);
    if (FAILED(hr)) {
        LOG_WARNING("Failed to create staging texture");
    }
    
    m_initialized = true;
    LOG_INFO("Frame processor initialized: " + std::to_string(settings.width) + "x" + std::to_string(settings.height) + "@" + std::to_string(settings.fps) + "fps");
    
    return true;
}

bool FrameProcessor::StartEncoding() {
    if (!m_initialized) {
        return false;
    }
    
    if (!CreateSinkWriter()) {
        LOG_ERROR("Failed to create sink writer");
        return false;
    }
    
    m_encoding = true;
    m_paused = false;
    m_framesProcessed = 0;
    m_framesDropped = 0;
    QueryPerformanceCounter(&m_startTime);
    
    LOG_INFO("Encoding started");
    return true;
}

void FrameProcessor::StopEncoding() {
    if (!m_encoding) return;
    
    m_encoding = false;
    
    if (m_sinkWriter) {
        m_sinkWriter->Finalize();
        m_sinkWriter.Reset();
    }
    
    auto stats = GetStatistics();
    LOG_INFO("Encoding stopped. Total frames: " + std::to_string(stats.framesProcessed));
}

bool FrameProcessor::SubmitFrame(ID3D11Texture2D* texture, uint64_t timestamp) {
    if (!m_encoding || m_paused || !texture) {
        return false;
    }
    
    // Convert texture to NV12 format for Media Foundation
    IMFSample* sample = nullptr;
    if (!ConvertTextureToNV12(texture, &sample)) {
        m_framesDropped++;
        return false;
    }
    
    if (sample) {
        // Set timestamp on sample
        LONGLONG sampleTime = static_cast<LONGLONG>(timestamp / 100);  // Convert to 100-ns units
        sample->SetSampleTime(sampleTime);
        
        // Write sample
        m_sinkWriter->WriteSample(sample);
        sample->Release();
        
        m_framesProcessed++;
    }
    
    return true;
}

void FrameProcessor::PauseEncoding(bool pause) {
    m_paused = pause;
    LOG_DEBUG(pause ? "Encoding paused" : "Encoding resumed");
}

FrameProcessor::Stats FrameProcessor::GetStatistics() const {
    Stats stats;
    stats.framesProcessed = m_framesProcessed;
    stats.framesDropped = m_framesDropped;
    
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);
    
    double elapsedSeconds = static_cast<double>(currentTime.QuadPart - m_startTime.QuadPart) / m_frequency.QuadPart;
    stats.averageFPS = elapsedSeconds > 0 ? static_cast<double>(m_framesProcessed) / elapsedSeconds : 0;
    
    // Get output file size
    stats.outputSizeBytes = 0;  // Would need to track file handle
    
    return stats;
}

bool FrameProcessor::ConvertTextureToNV12(ID3D11Texture2D* input, IMFSample** output) {
    // This is a simplified implementation
    // In production, you would use Media Foundation transforms or Intel Quick Sync
    
    if (!input || !output) return false;
    
    // For hardware acceleration, we'd use:
    // 1. MFT (Media Foundation Transform) for GPU-accelerated color conversion
    // 2. Intel Quick Sync Video for hardware encoding
    // 3. Direct texture sharing between D3D11 and MF
    
    *output = nullptr;
    
    // Placeholder - in real implementation this would:
    // - Create an IMFSample with NV12 buffer
    // - Use ID3D11DeviceContext::CopyResource to copy texture
    // - Use MFT to convert BGRA to NV12
    
    return true;
}

bool FrameProcessor::CreateSinkWriter() {
    // Initialize Media Foundation
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr) && hr != MF_E_ALREADY_STARTED) {
        LOG_ERROR("MFStartup failed: " + std::to_string(hr));
        return false;
    }
    
    // Create attributes for sink writer
    ComPtr<IMFAttributes> attributes;
    hr = MFCreateAttributes(&attributes, 1);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create attributes");
        return false;
    }
    
    // Enable hardware transforms
    if (m_settings.hardwareAcceleration) {
        attributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
        attributes->SetUINT32(MF_LOW_LATENCY, TRUE);
    }
    
    // Create sink writer
    hr = MFCreateSinkWriterFromURL(
        m_settings.outputPath.c_str(),
        nullptr,
        attributes.Get(),
        &m_sinkWriter
    );
    
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create sink writer: " + std::to_string(hr));
        return false;
    }
    
    // Configure video format
    if (!ConfigureVideoFormat()) {
        return false;
    }
    
    return true;
}

bool FrameProcessor::ConfigureVideoFormat() {
    // Create media type for H.264
    ComPtr<IMFMediaType> mediaType;
    HRESULT hr = MFCreateMediaType(&mediaType);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create media type");
        return false;
    }
    
    // Set major type to video
    mediaType->SetMajorType(MFMediaType_Video);
    
    // Set subtype to H.264
    mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    
    // Set bitrate
    mediaType->SetUINT32(MF_MT_AVG_BITRATE, m_settings.bitrate);
    
    // Set interlace mode
    mediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    
    // Set frame size
    MFSetAttributeSize(mediaType.Get(), MF_MT_FRAME_SIZE, m_settings.width, m_settings.height);
    
    // Set frame rate
    MFSetAttributeRatio(mediaType.Get(), MF_MT_FRAME_RATE, m_settings.fps, 1);
    
    // Set pixel aspect ratio
    MFSetAttributeRatio(mediaType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    
    // Add stream to sink writer
    hr = m_sinkWriter->AddStream(mediaType.Get(), nullptr);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to add stream: " + std::to_string(hr));
        return false;
    }
    
    // Create input media type (what we're feeding in)
    ComPtr<IMFMediaType> inputType;
    hr = MFCreateMediaType(&inputType);
    if (FAILED(hr)) {
        return false;
    }
    
    inputType->SetMajorType(MFMediaType_Video);
    inputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);  // Or NV12
    MFSetAttributeSize(inputType.Get(), MF_MT_FRAME_SIZE, m_settings.width, m_settings.height);
    MFSetAttributeRatio(inputType.Get(), MF_MT_FRAME_RATE, m_settings.fps, 1);
    
    hr = m_sinkWriter->SetInputMediaType(0, inputType.Get(), nullptr);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to set input media type: " + std::to_string(hr));
        return false;
    }
    
    // Begin writing
    hr = m_sinkWriter->BeginWriting();
    if (FAILED(hr)) {
        LOG_ERROR("Failed to begin writing: " + std::to_string(hr));
        return false;
    }
    
    LOG_INFO("Video format configured: H.264, " + std::to_string(m_settings.width) + "x" + std::to_string(m_settings.height) + ", " + std::to_string(m_settings.bitrate / 1000) + "kbps");
    
    return true;
}

bool FrameProcessor::EnableHardwareTransforms() {
    // Hardware transforms are enabled via MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS
    // attribute when creating the sink writer
    m_hardwareEnabled = true;
    return true;
}

} // namespace TechVideoEditor
