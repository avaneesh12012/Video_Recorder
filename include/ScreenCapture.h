#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <vector>
#include <memory>
#include <functional>

using Microsoft::WRL::ComPtr;

namespace TechVideoEditor {

struct CapturedFrame {
    ComPtr<ID3D11Texture2D> texture;
    uint64_t timestamp;      // Nanoseconds
    uint32_t width;
    uint32_t height;
    bool isDirty;            // Frame changed from previous
};

class ScreenCapture {
public:
    ScreenCapture();
    ~ScreenCapture();

    // Initialize capture for full screen or specific window
    bool Initialize(HWND targetWindow = nullptr);
    bool InitializeRegion(int x, int y, int width, int height);
    
    // Capture a single frame (returns nullptr if no new frame)
    std::unique_ptr<CapturedFrame> CaptureFrame();
    
    // Enable dirty frame detection
    void EnableDirtyFrameDetection(bool enable);
    
    // Get current capture dimensions
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    
    // Check if initialized
    bool IsInitialized() const { return m_initialized; }

private:
    bool InitializeDXGI();
    bool CreateDeviceResources();
    bool InitializeDuplication();
    bool CompareFrames(const uint8_t* current, const uint8_t* previous, size_t size);
    
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_context;
    ComPtr<IDXGIDevice> m_dxgiDevice;
    ComPtr<IDXGIOutput1> m_output;
    ComPtr<IDXGIOutputDuplication> m_duplication;
    
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    bool m_initialized = false;
    bool m_dirtyFrameDetection = false;
    
    std::vector<uint8_t> m_previousFrame;
    LARGE_INTEGER m_frequency;
};

} // namespace TechVideoEditor
