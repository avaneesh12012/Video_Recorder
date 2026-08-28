#include "Logger.h"
#include <iostream>

namespace TechVideoEditor {

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

void Logger::Initialize(const std::wstring& logFile) {
    if (m_initialized) {
        return;
    }
    
    m_logFile.open(logFile, std::ios::out | std::ios::app);
    if (!m_logFile.is_open()) {
        std::cerr << "Failed to open log file: " << logFile << std::endl;
        return;
    }
    
    m_initialized = true;
    LOG_INFO("Logger initialized");
}

void Logger::Shutdown() {
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
    m_initialized = false;
}

void Logger::Log(LogLevel level, const std::string& message) {
    if (level < m_minLevel) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::string logLine = "[" + GetTimestamp() + "] [" + LevelToString(level) + "] " + message;
    
    if (m_consoleOutput) {
        switch (level) {
            case LogLevel::Debug:
                std::cout << logLine << std::endl;
                break;
            case LogLevel::Info:
                std::cout << logLine << std::endl;
                break;
            case LogLevel::Warning:
                std::cerr << logLine << std::endl;
                break;
            case LogLevel::Error:
                std::cerr << logLine << std::endl;
                break;
        }
    }
    
    if (m_logFile.is_open()) {
        m_logFile << logLine << std::endl;
        m_logFile.flush();
    }
}

std::string Logger::LevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        default:                return "UNKNOWN";
    }
}

std::string Logger::GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&time_t_now));
    
    return std::string(buffer) + "." + 
           (ms.count() < 10 ? "00" : ms.count() < 100 ? "0" : "") + 
           std::to_string(ms.count());
}

Logger::~Logger() {
    Shutdown();
}

} // namespace TechVideoEditor
