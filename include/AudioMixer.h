#pragma once

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <vector>
#include <mutex>
#include <memory>
#include <atomic>

namespace TechVideoEditor {

struct AudioBuffer {
    std::vector<float> samples;  // Interleaved or planar
    uint32_t sampleRate;
    uint32_t channels;
    uint64_t timestamp;  // Nanoseconds
    double duration;     // Seconds
};

enum class AudioTrackType {
    SystemAudio,      // WASAPI Loopback
    Microphone,       // Mic input
    TTS,              // Text-to-speech
    Music             // Background music
};

struct AudioTrack {
    AudioTrackType type;
    std::unique_ptr<AudioBuffer> buffer;
    float volume;           // 0.0 to 1.0
    bool muted;
    bool solo;              // Solo this track
    int pan;                // -100 (left) to 100 (right)
};

class AudioMixer {
public:
    AudioMixer();
    ~AudioMixer();

    // Initialize with target sample rate and channels
    bool Initialize(uint32_t sampleRate = 48000, uint32_t channels = 2);
    
    // Add/remove audio tracks
    int AddTrack(AudioTrackType type);
    void RemoveTrack(int trackId);
    
    // Get track by ID
    AudioTrack* GetTrack(int trackId);
    
    // Mix all active tracks into output buffer
    void Mix(AudioBuffer& output);
    
    // Playback control
    bool StartPlayback();
    void StopPlayback();
    void PausePlayback(bool pause);
    bool IsPlaying() const { return m_playing; }
    
    // Volume controls (global and per-track)
    void SetMasterVolume(float volume);
    void SetTrackVolume(int trackId, float volume);
    
    // Mute/Solo
    void MuteTrack(int trackId, bool mute);
    void SoloTrack(int trackId, bool solo);
    
    // Load audio from file (for TTS/music)
    bool LoadAudioFromFile(const wchar_t* filePath, int trackId);
    
    // Waveform generation for UI
    std::vector<float> GenerateWaveform(int trackId, int numSamples);

private:
    static void DataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
    void MixTracks(float* output, ma_uint32 frameCount);
    
    ma_device_config m_deviceConfig;
    ma_device m_playbackDevice;
    ma_resampler_config m_resamplerConfig;
    
    std::vector<AudioTrack> m_tracks;
    std::mutex m_trackMutex;
    
    uint32_t m_sampleRate;
    uint32_t m_channels;
    float m_masterVolume;
    
    std::atomic<bool> m_playing;
    std::atomic<bool> m_paused;
    
    int m_nextTrackId;
};

} // namespace TechVideoEditor
