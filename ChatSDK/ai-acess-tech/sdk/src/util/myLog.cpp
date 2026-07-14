#include "../../include/util/myLog.h"
#include <memory>
#include <mutex>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>
namespace bite
{
    std::shared_ptr<spdlog::logger> Logger::_logger;
    std::mutex Logger::_mutex;
    Logger::Logger()
    {
    }
    void Logger::initLogger(const std::string &loggerName, const std::string &logFile, spdlog::level::level_enum loglevel)
    {
        if (nullptr == _logger)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (nullptr == _logger)
            {
                // 启用异步日志，将日志信息存放在信息存放队列中，由后台线程负责写入文件
                spdlog::init_thread_pool(32768, 1); // 第一个参数为线程池大小，第二个参数为线程数
                if (logFile == "stdout")
                {
                    // 创建一个带颜色的输出到控制台的日志器
                    _logger = spdlog::stdout_color_mt(loggerName);
                }
                else
                {
                    // 创建一个文件输出的日志器，日志会被写入指定文件中
                    _logger = spdlog::basic_logger_mt<spdlog::async_factory>(loggerName, logFile);
                }
            }

            // 格式设置
            _logger->set_pattern("[%H:%M:%S][%n][%-7l]%v");
            _logger->set_level(loglevel);
            spdlog::set_default_logger(_logger);
        }
    }

    std::shared_ptr<spdlog::logger> Logger::getLogger()
    {
        return _logger;
    }
} // end bite