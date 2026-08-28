#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <mutex>

namespace TechVideoEditor {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger {
public:
    static Logger& Instance();
    
    void Initialize(const std::wstring& logFile = L"TechVideoEditor.log");
    void Shutdown();
    
    void Log(LogLevel level, const std::string& message);
    void Debug(const std::string& message) { Log(LogLevel::Debug, message); }
    void Info(const std::string& message) { Log(LogLevel::Info, message); }
    void Warning(const std::string& message) { Log(LogLevel::Warning, message); }
    void Error(const std::string& message) { Log(LogLevel::Error, message); }
    
    // Set minimum log level to output
    void SetLogLevel(LogLevel level) { m_minLevel = level; }
    
    // Enable/disable console output
    void EnableConsoleOutput(bool enable) { m_consoleOutput = enable; }
    
private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    std::string LevelToString(LogLevel level);
    std::string GetTimestamp();
    
    std::wofstream m_logFile;
    std::mutex m_mutex;
    LogLevel m_minLevel = LogLevel::Debug;
    bool m_consoleOutput = true;
    bool m_initialized = false;
};

// Convenience macros
#define LOG_DEBUG(msg) TechVideoEditor::Logger::Instance().Debug(msg)
#define LOG_INFO(msg) TechVideoEditor::Logger::Instance().Info(msg)
#define LOG_WARNING(msg) TechVideoEditor::Logger::Instance().Warning(msg)
#define LOG_ERROR(msg) TechVideoEditor::Logger::Instance().Error(msg)

} // namespace TechVideoEditor
