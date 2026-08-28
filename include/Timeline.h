#pragma once

#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <chrono>

namespace TechVideoEditor {

enum class ClipType {
    Video,
    Audio,
    Text,
    Image
};

struct TimeRange {
    int64_t start;    // Nanoseconds
    int64_t duration; // Nanoseconds
    
    int64_t End() const { return start + duration; }
    void SetEnd(int64_t end) { duration = end - start; }
};

struct TimelineClip {
    std::string id;
    ClipType type;
    TimeRange timelineRange;   // Position on timeline
    TimeRange sourceRange;     // Source media range
    std::string filePath;      // For file-based clips
    std::string text;          // For text overlays
    int trackIndex;
    bool selected;
    
    // Zoom/transform properties
    float scale;
    float positionX;
    float positionY;
    float rotation;
};

struct TimelineTrack {
    std::string name;
    int index;
    bool visible;
    bool locked;
    float height;
    std::vector<std::shared_ptr<TimelineClip>> clips;
};

class Timeline {
public:
    Timeline();
    ~Timeline();

    // Track management
    int AddTrack(const std::string& name);
    void RemoveTrack(int trackIndex);
    TimelineTrack* GetTrack(int trackIndex);
    int GetTrackCount() const { return static_cast<int>(m_tracks.size()); }
    
    // Clip management
    std::shared_ptr<TimelineClip> AddClip(int trackIndex, ClipType type);
    void RemoveClip(int trackIndex, const std::string& clipId);
    TimelineClip* GetClip(int trackIndex, const std::string& clipId);
    
    // Selection
    void SelectClip(int trackIndex, const std::string& clipId);
    void DeselectAll();
    std::vector<std::shared_ptr<TimelineClip>> GetSelectedClips();
    
    // Timeline operations
    void SplitClip(int trackIndex, const std::string& clipId, int64_t time);
    void TrimClipStart(int trackIndex, const std::string& clipId, int64_t newStart);
    void TrimClipEnd(int trackIndex, const std::string& clipId, int64_t newEnd);
    void MoveClip(int trackIndex, const std::string& clipId, int64_t newStart, int newTrackIndex);
    
    // Playback
    void SetPlayheadPosition(int64_t position);
    int64_t GetPlayheadPosition() const { return m_playheadPosition; }
    void SetDuration(int64_t duration) { m_duration = duration; }
    int64_t GetDuration() const { return m_duration; }
    
    // Zoom level for UI (pixels per second)
    void SetZoomLevel(float pixelsPerSecond) { m_zoomLevel = pixelsPerSecond; }
    float GetZoomLevel() const { return m_zoomLevel; }
    
    // Get all clips at a specific time
    std::vector<TimelineClip*> GetClipsAtTime(int64_t time);
    
    // Callbacks
    using TimelineChangedCallback = std::function<void()>;
    void SetChangedCallback(TimelineChangedCallback callback) { m_changedCallback = callback; }

private:
    void NotifyChanged();
    
    std::vector<TimelineTrack> m_tracks;
    int64_t m_playheadPosition;
    int64_t m_duration;
    float m_zoomLevel;
    
    TimelineChangedCallback m_changedCallback;
    
    // Auto-increment clip IDs
    int m_nextClipId;
};

} // namespace TechVideoEditor
