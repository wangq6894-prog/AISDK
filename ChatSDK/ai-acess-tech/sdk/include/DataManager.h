#pragma once
#include "common.h"
#include <memory>
#include <mutex>
#include <sqlite3.h>
#include <string>
#include <vector>
namespace ai_chat_sdk {
class DataManager {
public:
    DataManager(const std::string &dbName);
    ~DataManager();
    // 会话相关

    // 插入新会话
    bool insertSession(const Session &session);
    // 获得指定会话信息
    std::shared_ptr<Session> getSession(const std::string &sessionId) const;
    // 更新指定会话时间戳
    bool updateSessionTimestamp(const std::string &sessionId, std::time_t timestamp);
    // 删除会话
    bool deleteSession(const std::string &sessionId);
    // 获得所有会话ID
    std::vector<std::string> getAllSessionIds() const;
    // 获得所有会话信息
    std::vector<std::shared_ptr<Session>> getAllSessions() const;
    // 获得会话总数
    size_t getSessionCount() const;
    // 信息相关

    // 插入新消息
    bool insertSessionMessage(const std::string &sessionId, const Message &message);
    // 获得指定会话的历史信息
    std::vector<Message> getSessionMessages(const std::string &sessionId) const;
    // 删除指定会话的所有消息
    bool deleteSessionMessages(const std::string &sessionId);
    // 删除所有会话
    bool clearAllSessions();
    // 获得会话消息总数
    int getSessionMessageCount(const std::string &sessionId) const;

private:
    bool initDataBase();
    bool executeSQL(const std::string &sql);

private:
    sqlite3 *_db = nullptr;
    std::string _dbName;
    mutable std::mutex _mutex;
};
} // namespace ai_chat_sdk