#include "Timeline.h"
#include <algorithm>

namespace TechVideoEditor {

Timeline::Timeline() 
    : m_playheadPosition(0)
    , m_duration(60000000000LL)  // Default 60 seconds in nanoseconds
    , m_zoomLevel(100.0f)        // 100 pixels per second
    , m_nextClipId(1)
{
}

Timeline::~Timeline() = default;

int Timeline::AddTrack(const std::string& name) {
    TimelineTrack track;
    track.name = name;
    track.index = static_cast<int>(m_tracks.size());
    track.visible = true;
    track.locked = false;
    track.height = 60.0f;
    
    m_tracks.push_back(track);
    NotifyChanged();
    
    return static_cast<int>(m_tracks.size()) - 1;
}

void Timeline::RemoveTrack(int trackIndex) {
    if (trackIndex < 0 || trackIndex >= static_cast<int>(m_tracks.size())) {
        return;
    }
    
    m_tracks.erase(m_tracks.begin() + trackIndex);
    
    // Update indices
    for (size_t i = 0; i < m_tracks.size(); ++i) {
        m_tracks[i].index = static_cast<int>(i);
    }
    
    NotifyChanged();
}

TimelineTrack* Timeline::GetTrack(int trackIndex) {
    if (trackIndex < 0 || trackIndex >= static_cast<int>(m_tracks.size())) {
        return nullptr;
    }
    
    return &m_tracks[trackIndex];
}

std::shared_ptr<TimelineClip> Timeline::AddClip(int trackIndex, ClipType type) {
    if (trackIndex < 0 || trackIndex >= static_cast<int>(m_tracks.size())) {
        return nullptr;
    }
    
    auto clip = std::make_shared<TimelineClip>();
    clip->id = "clip_" + std::to_string(m_nextClipId++);
    clip->type = type;
    clip->timelineRange.start = m_playheadPosition;
    clip->timelineRange.duration = 5000000000LL;  // Default 5 seconds
    clip->sourceRange.start = 0;
    clip->sourceRange.duration = clip->timelineRange.duration;
    clip->trackIndex = trackIndex;
    clip->selected = false;
    clip->scale = 1.0f;
    clip->positionX = 0.0f;
    clip->positionY = 0.0f;
    clip->rotation = 0.0f;
    
    m_tracks[trackIndex].clips.push_back(clip);
    NotifyChanged();
    
    return clip;
}

void Timeline::RemoveClip(int trackIndex, const std::string& clipId) {
    if (trackIndex < 0 || trackIndex >= static_cast<int>(m_tracks.size())) {
        return;
    }
    
    auto& clips = m_tracks[trackIndex].clips;
    clips.erase(
        std::remove_if(clips.begin(), clips.end(),
            [&clipId](const std::shared_ptr<TimelineClip>& clip) {
                return clip->id == clipId;
            }),
        clips.end()
    );
    
    NotifyChanged();
}

TimelineClip* Timeline::GetClip(int trackIndex, const std::string& clipId) {
    if (trackIndex < 0 || trackIndex >= static_cast<int>(m_tracks.size())) {
        return nullptr;
    }
    
    for (auto& clip : m_tracks[trackIndex].clips) {
        if (clip->id == clipId) {
            return clip.get();
        }
    }
    
    return nullptr;
}

void Timeline::SelectClip(int trackIndex, const std::string& clipId) {
    DeselectAll();
    
    if (trackIndex >= 0 && trackIndex < static_cast<int>(m_tracks.size())) {
        for (auto& clip : m_tracks[trackIndex].clips) {
            if (clip->id == clipId) {
                clip->selected = true;
                break;
            }
        }
    }
}

void Timeline::DeselectAll() {
    for (auto& track : m_tracks) {
        for (auto& clip : track.clips) {
            clip->selected = false;
        }
    }
}

std::vector<std::shared_ptr<TimelineClip>> Timeline::GetSelectedClips() {
    std::vector<std::shared_ptr<TimelineClip>> selected;
    
    for (auto& track : m_tracks) {
        for (auto& clip : track.clips) {
            if (clip->selected) {
                selected.push_back(clip);
            }
        }
    }
    
    return selected;
}

void Timeline::SplitClip(int trackIndex, const std::string& clipId, int64_t time) {
    TimelineClip* clip = GetClip(trackIndex, clipId);
    if (!clip) {
        return;
    }
    
    // Check if split point is within the clip
    if (time <= clip->timelineRange.start || time >= clip->timelineRange.End()) {
        return;
    }
    
    // Create new clip for the second part
    auto newClip = std::make_shared<TimelineClip>(*clip);
    newClip->id = "clip_" + std::to_string(m_nextClipId++);
    newClip->selected = false;
    
    // Adjust original clip
    int64_t firstDuration = time - clip->timelineRange.start;
    clip->timelineRange.duration = firstDuration;
    
    // Adjust new clip
    newClip->timelineRange.start = time;
    newClip->timelineRange.duration = clip->timelineRange.End() - time;
    newClip->sourceRange.start += firstDuration;
    newClip->sourceRange.duration = newClip->timelineRange.duration;
    
    m_tracks[trackIndex].clips.push_back(newClip);
    NotifyChanged();
}

void Timeline::TrimClipStart(int trackIndex, const std::string& clipId, int64_t newStart) {
    TimelineClip* clip = GetClip(trackIndex, clipId);
    if (!clip) {
        return;
    }
    
    if (newStart < clip->timelineRange.start || newStart >= clip->timelineRange.End()) {
        return;
    }
    
    int64_t delta = newStart - clip->timelineRange.start;
    clip->timelineRange.start = newStart;
    clip->timelineRange.duration -= delta;
    clip->sourceRange.start += delta;
    clip->sourceRange.duration -= delta;
    
    NotifyChanged();
}

void Timeline::TrimClipEnd(int trackIndex, const std::string& clipId, int64_t newEnd) {
    TimelineClip* clip = GetClip(trackIndex, clipId);
    if (!clip) {
        return;
    }
    
    if (newEnd <= clip->timelineRange.start || newEnd > clip->timelineRange.End()) {
        return;
    }
    
    clip->timelineRange.duration = newEnd - clip->timelineRange.start;
    clip->sourceRange.duration = clip->timelineRange.duration;
    
    NotifyChanged();
}

void Timeline::MoveClip(int trackIndex, const std::string& clipId, int64_t newStart, int newTrackIndex) {
    TimelineClip* clip = GetClip(trackIndex, clipId);
    if (!clip) {
        return;
    }
    
    int64_t delta = newStart - clip->timelineRange.start;
    clip->timelineRange.start = newStart;
    
    // Move to different track if requested
    if (newTrackIndex != trackIndex && 
        newTrackIndex >= 0 && 
        newTrackIndex < static_cast<int>(m_tracks.size())) {
        
        // Remove from current track
        auto& currentClips = m_tracks[trackIndex].clips;
        currentClips.erase(
            std::remove_if(currentClips.begin(), currentClips.end(),
                [&clipId](const std::shared_ptr<TimelineClip>& c) {
                    return c->id == clipId;
                }),
            currentClips.end()
        );
        
        // Add to new track
        for (auto& c : m_tracks[newTrackIndex].clips) {
            if (c->id == clipId) {
                c->timelineRange.start = newStart;
                c->trackIndex = newTrackIndex;
                return;
            }
        }
    }
    
    NotifyChanged();
}

void Timeline::SetPlayheadPosition(int64_t position) {
    m_playheadPosition = std::max(0LL, std::min(position, m_duration));
}

std::vector<TimelineClip*> Timeline::GetClipsAtTime(int64_t time) {
    std::vector<TimelineClip*> clips;
    
    for (auto& track : m_tracks) {
        if (!track.visible) continue;
        
        for (auto& clip : track.clips) {
            if (time >= clip->timelineRange.start && 
                time < clip->timelineRange.End()) {
                clips.push_back(clip.get());
            }
        }
    }
    
    return clips;
}

void Timeline::NotifyChanged() {
    if (m_changedCallback) {
        m_changedCallback();
    }
}

} // namespace TechVideoEditor
