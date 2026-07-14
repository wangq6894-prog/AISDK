#pragma once
#include <mutex>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
namespace bite {
class Logger {
public:
    static void initLogger(const std::string &loggerName, const std::string &logFile,
                           spdlog::level::level_enum loglevel = spdlog::level::info);
    static std::shared_ptr<spdlog::logger> getLogger();

private:
    Logger();
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;
    static std::shared_ptr<spdlog::logger> _logger;
    static std::mutex _mutex;
};

// fmt
// #include<format>
// string s = std::format("hello,{}",world);
#define TRACE(format, ...)                                                                                             \
    bite::Logger::getLogger()->trace(std::string("[{:>10s}:{:<4d}]") + format, __FILE__, __LINE__, ##__VA_ARGS__)
#define DBG(format, ...)                                                                                               \
    bite::Logger::getLogger()->debug(std::string("[{:>10s}:{:<4d}]") + format, __FILE__, __LINE__, ##__VA_ARGS__)
#define INFO(format, ...)                                                                                              \
    bite::Logger::getLogger()->info(std::string("[{:>10s}:{:<4d}]") + format, __FILE__, __LINE__, ##__VA_ARGS__)
#define WARN(format, ...)                                                                                              \
    bite::Logger::getLogger()->warn(std::string("[{:>10s}:{:<4d}]") + format, __FILE__, __LINE__, ##__VA_ARGS__)
#define ERR(format, ...)                                                                                               \
    bite::Logger::getLogger()->error(std::string("[{:>10s}:{:<4d}]") + format, __FILE__, __LINE__, ##__VA_ARGS__)
#define CRIT(format, ...)                                                                                              \
    bite::Logger::getLogger()->critical(std::string("[{:>10s}:{:<4d}]") + format, __FILE__, __LINE__, ##__VA_ARGS__)
} // namespace bite