#pragma once
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "DataManager.h"
#include "common.h"
namespace ai_chat_sdk {
// SessionManager 类
class SessionManager
{
   public:
    SessionManager(const std::string &dbName = "chatDB.db");
    ~SessionManager();
    std::string createSession(const std::string &modelName);
    std::shared_ptr<Session> getSession(const std::string &sessionId);
    bool addMessage(const std::string &sessionId, const Message &message);
    std::vector<Message> getHistroyMessages(const std::string &sessionId);
    std::vector<std::string> getSessionLists() const;
    bool deleteSession(const std::string &sessionId);
    void updateSessionTimestamp(const std::string &sessionId);
    void clearAllSessions();
    int64_t getSessionCount();

   private:
    std::string generateSessionId();
    std::string generateMessageId(size_t messageCount);
    // 用于管理所有的会话信息 会话ID -> 会话对象
    std::unordered_map<std::string, std::shared_ptr<Session>> _sessions;
    // 用于保护会话对象的互斥锁
    mutable std::mutex _mutex;
    std::atomic<int64_t> _sessionCounter = {0};
    // 数据库管理器
    DataManager _dataManager;
};
}  // namespace ai_chat_sdk