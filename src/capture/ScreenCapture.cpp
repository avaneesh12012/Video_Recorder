#include "ScreenCapture.h"
#include "Logger.h"

namespace TechVideoEditor {

ScreenCapture::ScreenCapture() {
    QueryPerformanceFrequency(&m_frequency);
}

ScreenCapture::~ScreenCapture() {
    if (m_duplication) {
        m_duplication->ReleaseFrame();
        m_duplication.Reset();
    }
    m_output.Reset();
    m_dxgiDevice.Reset();
    m_context.Reset();
    m_device.Reset();
}

bool ScreenCapture::Initialize(HWND targetWindow) {
    if (!InitializeDXGI()) {
        LOG_ERROR("Failed to initialize DXGI");
        return false;
    }
    
    if (!CreateDeviceResources()) {
        LOG_ERROR("Failed to create device resources");
        return false;
    }
    
    if (!InitializeDuplication()) {
        LOG_ERROR("Failed to initialize desktop duplication");
        return false;
    }
    
    m_initialized = true;
    LOG_INFO("Screen capture initialized successfully");
    return true;
}

bool ScreenCapture::InitializeRegion(int x, int y, int width, int height) {
    // For region capture, we still capture the full screen but crop in post
    // This is more efficient than multiple DXGI contexts
    if (!Initialize(nullptr)) {
        return false;
    }
    
    // Store region for cropping (could be used in FrameProcessor)
    LOG_DEBUG("Region capture requested: " + std::to_string(width) + "x" + std::to_string(height));
    return true;
}

std::unique_ptr<CapturedFrame> ScreenCapture::CaptureFrame() {
    if (!m_initialized || !m_duplication) {
        return nullptr;
    }
    
    IDXGIResource* desktopResource = nullptr;
    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    
    HRESULT hr = m_duplication->AcquireNextFrame(0, &frameInfo, &desktopResource);
    
    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        // No new frame available
        return nullptr;
    }
    
    if (FAILED(hr)) {
        // Desktop duplication lost, try to reinitialize
        LOG_WARNING("Desktop duplication lost, attempting to reinitialize");
        m_duplication.Reset();
        InitializeDuplication();
        return nullptr;
    }
    
    ComPtr<ID3D11Texture2D> desktopTexture;
    hr = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(desktopTexture.GetAddressOf()));
    desktopResource->Release();
    
    if (FAILED(hr)) {
        LOG_ERROR("Failed to get texture from desktop resource");
        return nullptr;
    }
    
    auto frame = std::make_unique<CapturedFrame>();
    frame->texture = desktopTexture;
    frame->width = m_width;
    frame->height = m_height;
    frame->timestamp = frameInfo.LastPresentTime.QuadPart * (10000000LL / m_frequency.QuadPart);
    
    // Dirty frame detection
    if (m_dirtyFrameDetection && !m_previousFrame.empty()) {
        frame->isDirty = !CompareFrames(nullptr, m_previousFrame.data(), m_width * m_height * 4);
    } else {
        frame->isDirty = true;
        
        // Store current frame for comparison
        if (m_dirtyFrameDetection) {
            D3D11_MAPPED_SUBRESOURCE mapped;
            hr = m_context->Map(desktopTexture.Get(), 0, D3D11_MAP_READ, 0, &mapped);
            if (SUCCEEDED(hr)) {
                m_previousFrame.resize(m_width * m_height * 4);
                uint8_t* src = static_cast<uint8_t*>(mapped.pData);
                uint8_t* dst = m_previousFrame.data();
                
                for (uint32_t y = 0; y < m_height; ++y) {
                    memcpy(dst + y * m_width * 4, src + y * mapped.RowPitch, m_width * 4);
                }
                
                m_context->Unmap(desktopTexture.Get(), 0);
            }
        }
    }
    
    return frame;
}

void ScreenCapture::EnableDirtyFrameDetection(bool enable) {
    m_dirtyFrameDetection = enable;
    if (enable) {
        m_previousFrame.clear();
    }
}

bool ScreenCapture::InitializeDXGI() {
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
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
    
    hr = m_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(m_dxgiDevice.GetAddressOf()));
    if (FAILED(hr)) {
        LOG_ERROR("Failed to get IDXGIDevice interface");
        return false;
    }
    
    return true;
}

bool ScreenCapture::CreateDeviceResources() {
    ComPtr<IDXGIAdapter> adapter;
    HRESULT hr = m_dxgiDevice->GetAdapter(&adapter);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to get DXGI adapter");
        return false;
    }
    
    ComPtr<IDXGIOutput> output;
    hr = adapter->EnumOutputs(0, &output);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to enumerate outputs");
        return false;
    }
    
    hr = output->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void**>(m_output.GetAddressOf()));
    if (FAILED(hr)) {
        LOG_ERROR("Failed to get IDXGIOutput1 interface");
        return false;
    }
    
    DXGI_OUTPUT_DESC outputDesc;
    output->GetDesc(&outputDesc);
    
    m_width = outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left;
    m_height = outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top;
    
    LOG_DEBUG("Desktop resolution: " + std::to_string(m_width) + "x" + std::to_string(m_height));
    
    return true;
}

bool ScreenCapture::InitializeDuplication() {
    HRESULT hr = m_output->DuplicateOutput(m_device.Get(), &m_duplication);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to duplicate output: " + std::to_string(hr));
        return false;
    }
    
    return true;
}

bool ScreenCapture::CompareFrames(const uint8_t* current, const uint8_t* previous, size_t size) {
    // Simple memcmp for dirty detection
    // Could be optimized with SIMD for large frames
    return memcmp(current, previous, size) == 0;
}

} // namespace TechVideoEditor
