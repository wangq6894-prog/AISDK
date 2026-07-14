#include "../include/SessionManager.h"
#include "../include/util/myLog.h"
#include <sstream>
#include <iomanip>
namespace ai_chat_sdk
{
    SessionManager::SessionManager(const std::string &dbName)
        : _dataManager(dbName)
    {
        auto sessions = _dataManager.getAllSessions();
        for (auto session : sessions)
        {
            _sessions[session->_sessionId] = session;
        }
        _sessionCounter = _sessions.size();
    }
    SessionManager::~SessionManager()
    {
        clearAllSessions();
    }
    // 生成会话ID
    std::string SessionManager::generateSessionId()
    {
        // 会话计数自增
        _sessionCounter.fetch_add(1);
        std::time_t time = std::time(nullptr);
        std::ostringstream os;
        // session_12345678_0000000001
        os << "session_" << time << std::setw(10) << std::setfill('0') << _sessionCounter;
        return os.str();
    }
    // 生成消息ID
    std::string SessionManager::generateMessageId(size_t messageCount)
    {
        // 消息计数自增
        messageCount++;
        std::time_t time = std::time(nullptr);
        std::ostringstream os;
        // message_12345678_0000000001
        os << "message_" << time << std::setw(10) << std::setfill('0') << messageCount;
        return os.str();
    }
    // 创建会话
    std::string SessionManager::createSession(const std::string &modelName)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto session = std::make_shared<Session>(modelName);
        std::string sessionId = generateSessionId();
        session->_sessionId  = sessionId;
        session->_createdAt = std::time(nullptr);
        session->_updatedAt = std::time(nullptr);
        _sessions[sessionId] = session;
        _dataManager.insertSession(*session);
        return sessionId;
    }
    std::shared_ptr<Session> SessionManager::getSession(const std::string &sessionId)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _sessions.find(sessionId);
        if (it == _sessions.end())
        {
            auto session = _dataManager.getSession(sessionId);
            if (session)
            {
                auto it = _sessions.find(sessionId);
                if (it == _sessions.end())
                {
                    _sessions[sessionId] = session;
                }
                session->_messages = _dataManager.getSessionMessages(sessionId);
                return session;
            }
            WARN("getSession sessionId: {} not found", sessionId);
            return nullptr;
        }
        //获取当前会话的历史消息
        it->second->_messages = _dataManager.getSessionMessages(sessionId);
        return it->second;
    }
    bool SessionManager::addMessage(const std::string &sessionId, const Message &message)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _sessions.find(sessionId);
        if (it == _sessions.end())
        {
            return false;
        }
        //创建消息
        Message msg(message._role, message._content);

        msg._messageId = generateMessageId(it->second->_messages.size());//size是消息个数
        msg._timestamp = std::time(nullptr);
        it->second->_messages.push_back(msg);
        it->second->_updatedAt = std::time(nullptr);
        INFO("addMessage sessionId: {}, message: {}", sessionId, msg._messageId);
        _dataManager.insertSessionMessage(sessionId, msg);
        return true;
    }
    std::vector<Message> SessionManager::getHistroyMessages(const std::string &sessionId)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _sessions.find(sessionId);
        if (it == _sessions.end())
        {
            return _dataManager.getSessionMessages(sessionId);
        }
        return it->second->_messages;
    }
    std::vector<std::string> SessionManager::getSessionLists() const
    {
        ///检测数据库中是否有未添加到_sessions的会话
        auto sessions = _dataManager.getAllSessions();
        std::lock_guard<std::mutex> lock(_mutex);
        std::vector<std::pair<time_t,std::shared_ptr<Session>>> temp;
        temp.reserve(_sessions.size());
        for (auto it : _sessions)
        {
            temp.push_back({it.second->_updatedAt, it.second});
        }
        for (auto session : sessions)
        {
            temp.push_back({session->_updatedAt, session});
        }
        std::sort(temp.begin(), temp.end(), [](const auto &a, const auto &b)
        {
            return a.first > b.first;
        });
        std::vector<std::string> sessionIds;
        for (auto it : temp)
        {
            sessionIds.push_back(it.second->_sessionId);
        }
        return sessionIds;
    }
    bool SessionManager::deleteSession(const std::string &sessionId)
    { 
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _sessions.find(sessionId);
        if(it == _sessions.end())
        {
            return false;
        }
        _sessions.erase(it);
        _dataManager.deleteSession(sessionId);
        return true;    
    }
    void SessionManager::updateSessionTimestamp(const std::string &sessionId)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _sessions.find(sessionId);
        if(it != _sessions.end())
        {
            it->second->_updatedAt = std::time(nullptr);
            _dataManager.updateSessionTimestamp(sessionId, it->second->_updatedAt);
        }
    }
    void SessionManager::clearAllSessions()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _sessions.clear();
        _dataManager.clearAllSessions();
        INFO("clear %d session", _sessions.size());
        
    }
    int64_t SessionManager::getSessionCount()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _sessions.size();//_sessionCounter 是"历史创建总数"（像数据库的自增 ID）
        //  _sessions.size() 是"当前存活数"
    }
}
