//
// Created by 刘文景 on 2020-03-26.
//

#ifndef Libel_LOGGING_H
#define Libel_LOGGING_H

#include "logstream.h"
#include "noncopyable.h"

namespace Libel {

extern int64_t MicroSecondsPerSecond;

class Logger {
 public:
  enum LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    NUM_LOG_LEVELS,
  };
  /// we use this class to get compile time calculation of length of c++
  /// predefine macro such as __func__ and __FILE__
  class PreDefineMacroHelper {
   public:
    template <int N>
    PreDefineMacroHelper(const char (&arr)[N]) : data_(arr), size_(N - 1) {
      const char* slash = strrchr(
          data_,
          '/');  /// find the last '/' in filename to avoid too long pathname
      if (slash) {
        data_ = slash + 1;
        size_ -= (data_ - arr);
      }
    }

//     explicit PreDefineMacroHelper(const char *name) : data_(name) {
//       size_ = strlen(data_);
//     }
    const char* data_;
    size_t size_;
  };

//  Logger(PreDefineMacroHelper fileName, PreDefineMacroHelper functionName,
//         int line, LogLevel level);
//  Logger(PreDefineMacroHelper fileName, PreDefineMacroHelper functionName,
//         int line, bool toAbort);
  Logger(PreDefineMacroHelper fileName, const char* functionName, int line, LogLevel level);
  ~Logger();

  LogStream& stream() { return insideHelper_.logStream_; }
  static LogLevel getLogLevel();
  static void setLogLevel(LogLevel level);

  typedef void (*OutputFunc)(const char* msg, size_t len);
  typedef void (*FlushFunc)();
  static void setOutput(OutputFunc);
  static void setFlush(FlushFunc);

 private:
  class InsideHelper {
   public:
    typedef Logger::LogLevel LogLevel;
//    InsideHelper(PreDefineMacroHelper fileName,
//                 PreDefineMacroHelper functionName, int line, LogLevel level,
//                 int);
    InsideHelper(PreDefineMacroHelper fileName, const char* functionName, int line, LogLevel level, int );
    void formatTime();
    void finish();

    LogStream logStream_;
//    PreDefineMacroHelper fileName_;
//    PreDefineMacroHelper functionName_;
    PreDefineMacroHelper fileName_;
    const char* functionName_;
    int line_;
    LogLevel level_;
    int64_t curtime_;
  };
  InsideHelper insideHelper_;
};

extern Logger::LogLevel AlogLevel;

inline Logger::LogLevel Logger::getLogLevel() { return AlogLevel; }

inline void Logger::setLogLevel(Libel::Logger::LogLevel level) {
  AlogLevel = level;
}

///当输出日志的级别高于日志的级别时，LOG_XXX相当于是空的，没有运行时开销
#define LOG_TRACE                                               \
  if (Libel::Logger::getLogLevel() <= Libel::Logger::TRACE) \
  Libel::Logger(__FILE__, __func__, __LINE__, Libel::Logger::TRACE).stream()
#define LOG_DEBUG                                               \
  if (Libel::Logger::getLogLevel() <= Libel::Logger::DEBUG) \
  Libel::Logger(__FILE__, __func__, __LINE__, Libel::Logger::DEBUG).stream()
#define LOG_INFO                                               \
  if (Libel::Logger::getLogLevel() <= Libel::Logger::INFO) \
  Libel::Logger(__FILE__, __func__, __LINE__, Libel::Logger::INFO).stream()
#define LOG_WARN \
  Libel::Logger(__FILE__, __func__, __LINE__, Libel::Logger::WARN).stream()
#define LOG_ERROR \
  Libel::Logger(__FILE__, __func__, __LINE__, Libel::Logger::ERROR).stream()
#define LOG_FATAL \
  Libel::Logger(__FILE__, __func__, __LINE__, Libel::Logger::FATAL).stream()

}  // namespace Libel

#endif  // Libel_LOGGING_H
