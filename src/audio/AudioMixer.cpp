#include "AudioMixer.h"
#include "Logger.h"

namespace TechVideoEditor {

AudioMixer::AudioMixer()
    : m_sampleRate(48000)
    , m_channels(2)
    , m_masterVolume(1.0f)
    , m_playing(false)
    , m_paused(false)
    , m_nextTrackId(1)
{
}

AudioMixer::~AudioMixer() {
    StopPlayback();
}

bool AudioMixer::Initialize(uint32_t sampleRate, uint32_t channels) {
    m_sampleRate = sampleRate;
    m_channels = channels;
    
    // Initialize miniaudio playback device
    ma_result result = ma_device_config_init(ma_device_type_playback, &m_deviceConfig);
    if (result != MA_SUCCESS) {
        LOG_WARNING("Failed to initialize miniaudio device config");
        return false;
    }
    
    m_deviceConfig.playback.format   = ma_format_f32;
    m_deviceConfig.playback.channels = channels;
    m_deviceConfig.sampleRate        = sampleRate;
    m_deviceConfig.dataCallback      = DataCallback;
    m_deviceConfig.pUserData         = this;
    
    result = ma_device_init(nullptr, &m_deviceConfig, &m_playbackDevice);
    if (result != MA_SUCCESS) {
        LOG_WARNING("Failed to initialize miniaudio playback device - audio disabled");
        return false;
    }
    
    LOG_INFO("Audio mixer initialized: " + std::to_string(sampleRate) + "Hz, " + std::to_string(channels) + " channels");
    return true;
}

int AudioMixer::AddTrack(AudioTrackType type) {
    std::lock_guard<std::mutex> lock(m_trackMutex);
    
    AudioTrack track;
    track.type = type;
    track.buffer = nullptr;
    track.volume = 1.0f;
    track.muted = false;
    track.solo = false;
    track.pan = 0;
    
    m_tracks.push_back(track);
    return m_nextTrackId++;
}

void AudioMixer::RemoveTrack(int trackId) {
    std::lock_guard<std::mutex> lock(m_trackMutex);
    
    // Find and remove track by ID (simplified - would need proper ID tracking)
    if (trackId >= 0 && trackId < static_cast<int>(m_tracks.size())) {
        m_tracks.erase(m_tracks.begin() + trackId);
    }
}

AudioTrack* AudioMixer::GetTrack(int trackId) {
    std::lock_guard<std::mutex> lock(m_trackMutex);
    
    if (trackId >= 0 && trackId < static_cast<int>(m_tracks.size())) {
        return &m_tracks[trackId];
    }
    
    return nullptr;
}

void AudioMixer::Mix(AudioBuffer& output) {
    std::lock_guard<std::mutex> lock(m_trackMutex);
    
    // Clear output buffer
    output.samples.clear();
    
    // Mix all active tracks
    bool hasSolo = false;
    for (const auto& track : m_tracks) {
        if (track.solo) {
            hasSolo = true;
            break;
        }
    }
    
    // In a real implementation, this would mix all active track buffers
    // with proper resampling, panning, and volume control
}

bool AudioMixer::StartPlayback() {
    if (m_playing) return true;
    
    ma_result result = ma_device_start(&m_playbackDevice);
    if (result != MA_SUCCESS) {
        LOG_WARNING("Failed to start audio playback");
        return false;
    }
    
    m_playing = true;
    m_paused = false;
    LOG_DEBUG("Audio playback started");
    return true;
}

void AudioMixer::StopPlayback() {
    if (!m_playing) return;
    
    ma_device_stop(&m_playbackDevice);
    m_playing = false;
    m_paused = false;
    LOG_DEBUG("Audio playback stopped");
}

void AudioMixer::PausePlayback(bool pause) {
    m_paused = pause;
    LOG_DEBUG(pause ? "Audio playback paused" : "Audio playback resumed");
}

void AudioMixer::SetMasterVolume(float volume) {
    m_masterVolume = std::clamp(volume, 0.0f, 1.0f);
}

void AudioMixer::SetTrackVolume(int trackId, float volume) {
    std::lock_guard<std::mutex> lock(m_trackMutex);
    
    if (trackId >= 0 && trackId < static_cast<int>(m_tracks.size())) {
        m_tracks[trackId].volume = std::clamp(volume, 0.0f, 1.0f);
    }
}

void AudioMixer::MuteTrack(int trackId, bool mute) {
    std::lock_guard<std::mutex> lock(m_trackMutex);
    
    if (trackId >= 0 && trackId < static_cast<int>(m_tracks.size())) {
        m_tracks[trackId].muted = mute;
    }
}

void AudioMixer::SoloTrack(int trackId, bool solo) {
    std::lock_guard<std::mutex> lock(m_trackMutex);
    
    if (trackId >= 0 && trackId < static_cast<int>(m_tracks.size())) {
        m_tracks[trackId].solo = solo;
    }
}

bool AudioMixer::LoadAudioFromFile(const wchar_t* filePath, int trackId) {
    // In a real implementation, this would use miniaudio's decoder API
    // to load audio files (MP3, WAV, etc.) into the track buffer
    LOG_DEBUG("Loading audio file: " + std::string(filePath, filePath + wcslen(filePath)));
    return true;
}

std::vector<float> AudioMixer::GenerateWaveform(int trackId, int numSamples) {
    std::vector<float> waveform;
    
    std::lock_guard<std::mutex> lock(m_trackMutex);
    
    if (trackId < 0 || trackId >= static_cast<int>(m_tracks.size())) {
        return waveform;
    }
    
    // Generate simplified waveform visualization data
    // In production, this would analyze the actual audio buffer
    waveform.resize(numSamples, 0.0f);
    
    return waveform;
}

void AudioMixer::DataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    // This is the miniaudio callback - called when audio buffer needs filling
    AudioMixer* mixer = reinterpret_cast<AudioMixer*>(pDevice->pUserData);
    
    if (mixer && !mixer->m_paused) {
        mixer->MixTracks(static_cast<float*>(pOutput), frameCount);
    } else {
        // Output silence when paused
        memset(pOutput, 0, frameCount * mixer->m_channels * sizeof(float));
    }
    
    (void)pInput; // Playback doesn't use input
}

void AudioMixer::MixTracks(float* output, ma_uint32 frameCount) {
    std::lock_guard<std::mutex> lock(m_trackMutex);
    
    size_t sampleCount = frameCount * m_channels;
    
    // Clear output buffer
    memset(output, 0, sampleCount * sizeof(float));
    
    // Check for solo tracks
    bool hasSolo = false;
    for (const auto& track : m_tracks) {
        if (track.solo && !track.muted && track.buffer) {
            hasSolo = true;
            break;
        }
    }
    
    // Mix each track
    for (auto& track : m_tracks) {
        if (track.muted || !track.buffer) {
            continue;
        }
        
        if (hasSolo && !track.solo) {
            continue;
        }
        
        // Apply volume and pan
        float volume = track.volume * m_masterVolume;
        
        // In production, this would properly mix the track samples
        // with resampling if needed and apply panning
    }
}

} // namespace TechVideoEditor
