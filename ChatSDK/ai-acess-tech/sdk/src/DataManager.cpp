#include "../include/DataManager.h"
#include "../include/util/myLog.h"
namespace ai_chat_sdk {
DataManager::DataManager(const std::string &dbName) : _dbName(dbName), _db(nullptr) { // 文件名       数据库连接对象
    int rc = sqlite3_open(dbName.c_str(), &_db);
    if (rc != SQLITE_OK) {
        ERR("DataManager::DataManager: sqlite3_open failed, error: {}", sqlite3_errmsg(_db));
        return;
    }
    INFO("打开数据库成功：{}", dbName);
    if (!initDataBase()) {
        sqlite3_close(_db);
        _db = nullptr;
        ERR("DataManager::DataManager: initDataBase failed");
        return;
    }
}
DataManager::~DataManager() {
    if (_db) {
        sqlite3_close(_db);
        _db = nullptr;
    }
}
bool DataManager::initDataBase() {
    std::string createSessionTable = R"(
        CREATE TABLE IF NOT EXISTS sessions 
        (session_id TEXT PRIMARY KEY, 
        model_name TEXT NOT NULL, 
        create_time INTEGER NOT NULL,
        update_time INTEGER NOT NULL);
        )";
    if (!executeSQL(createSessionTable)) {
        ERR("DataManager::initDataBase: executeSQL failed");
        return false;
    }

    std::string createMessageTable = R"(
        CREATE TABLE IF NOT EXISTS messages 
        (message_id TEXT PRIMARY KEY, 
        session_id TEXT NOT NULL, 
        role TEXT NOT NULL, 
        content TEXT NOT NULL, 
        timestamp INTEGER NOT NULL,
        FOREIGN KEY (session_id) REFERENCES sessions (session_id) ON DELETE CASCADE);
        )"; // 外键约束
    if (!executeSQL(createMessageTable)) {
        ERR("DataManager::initDataBase: executeSQL failed");
        return false;
    }
    return true;
}
bool DataManager::executeSQL(const std::string &sql) {
    // 检测数据库
    if (!_db) {
        ERR("数据库未初始化");
        return false;
    } //                   库对象  SQL语句  回调函数  回调参数  错误信息
    char *errMsg = nullptr;
    int rc = sqlite3_exec(_db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        ERR("DataManager::executeSQL: sqlite3_exec failed, error: {}", errMsg);
        sqlite3_free(errMsg);
        errMsg = nullptr;
        return false;
    }
    return true;
}
bool DataManager::insertSession(const Session &session) {
    std::lock_guard<std::mutex> lock(_mutex);
    std::string insertSQL = R"(
        INSERT INTO sessions (session_id, model_name, create_time, update_time)
        VALUES (?, ?, ?, ?);
        )";
    sqlite3_stmt *stmt = nullptr;
    // 数据库   SQL语句      SQL语句文本长度  准备语句  错误信息
    int rc = sqlite3_prepare_v2(_db, insertSQL.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        ERR("DataManager::insertSession: sqlite3_prepare_v2 failed, error: {}", sqlite3_errmsg(_db));
        return false;
    }
    // 绑定参数
    rc = sqlite3_bind_text(stmt, 1, session._sessionId.c_str(), -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        ERR("DataManager::insertSession: sqlite3_bind_text failed, error: {}", sqlite3_errmsg(_db));
        return false;
    }
    rc = sqlite3_bind_text(stmt, 2, session._modelName.c_str(), -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        ERR("DataManager::insertSession: sqlite3_bind_text failed, error: {}", sqlite3_errmsg(_db));
        return false;
    }
    rc = sqlite3_bind_int64(stmt, 3, static_cast<int64_t>(session._createdAt));
    if (rc != SQLITE_OK) {
        ERR("DataManager::insertSession: sqlite3_bind_int64 failed, error: {}", sqlite3_errmsg(_db));
        return false;
    }
    rc = sqlite3_bind_int64(stmt, 4, static_cast<int64_t>(session._updatedAt));
    if (rc != SQLITE_OK) {
        ERR("DataManager::insertSession: sqlite3_bind_int64 failed, error: {}", sqlite3_errmsg(_db));
        return false;
    }
    // 执行语句
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        ERR("DataManager::insertSession: sqlite3_step failed, error: {}", sqlite3_errmsg(_db));
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    INFO("插入会话成功：{}", session._sessionId);
    return true;
}
std::shared_ptr<Session> DataManager::getSession(const std::string &sessionId) const {
    std::lock_guard<std::mutex> lock(_mutex);
    // 构建SQL语句
    std::string selectSQL = R"(
        SELECT model_name, create_time, update_time FROM sessions WHERE session_id = ?;
        )";
    sqlite3_stmt *stmt = nullptr;
    int rc = sqlite3_prepare_v2(_db, selectSQL.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        ERR("DataManager::getSession: sqlite3_prepare_v2 failed, error: {}", sqlite3_errmsg(_db));
        return nullptr;
    }
    // SQL语句  参数序号  参数值  参数长度 ⽤于指定SQLite会不会尝试释放该字符串：SQLITE_STATIC —
    // SQLite不会尝试释放该字符串  SQLITE_TRANSIENT — SQLite会制作该字符串的副本
    sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_TRANSIENT);
    // 执行语句
    rc = sqlite3_step(stmt); // SQLITE_ROW表示成功查到，SQLITE_DONE表示未查询到结果
    if (rc == SQLITE_DONE) {
        // 未查询到结果
        sqlite3_finalize(stmt);
        return nullptr;
    }
    if (rc != SQLITE_ROW) {
        ERR("DataManager::getSession: sqlite3_step failed, error: {}", sqlite3_errmsg(_db));
        sqlite3_finalize(stmt);
        return nullptr;
    }
    // 获取结果
    std::string modelName = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    std::shared_ptr<Session> session = std::make_shared<Session>(modelName);
    session->_sessionId = sessionId;
    session->_createdAt = static_cast<std::time_t>(sqlite3_column_int64(stmt, 1));
    session->_updatedAt = static_cast<std::time_t>(sqlite3_column_int64(stmt, 2));
    sqlite3_finalize(stmt);
    INFO("查询会话成功：{}", sessionId);

    return session;
}
// 更新指定会话的时间戳
bool DataManager::updateSessionTimestamp(const std::string &sessionId, std::time_t timestamp) {
    std::lock_guard<std::mutex> lock(_mutex);

    // 构建SQL语句
    std::string updateSQL = R"(
        UPDATE sessions SET update_time = ? WHERE session_id = ?;
        )";

    // 准备SQL语句
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(_db, updateSQL.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        ERR("updateSessionTimestamp - 准备语句失败：{}", sqlite3_errmsg(_db));
        return false;
    }

    // 绑定参数
    sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(timestamp));
    sqlite3_bind_text(stmt, 2, sessionId.c_str(), -1, SQLITE_TRANSIENT);

    // 执行SQL语句
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        ERR("updateSessionTimestamp - 执行语句失败：{}", sqlite3_errmsg(_db));
        sqlite3_finalize(stmt);
        return false;
    }

    // 释放语句
    sqlite3_finalize(stmt);
    INFO("updateSessionTimestamp - 更新会话时间戳成功：{}", sessionId);
    return true;
}

// 删除指定会话
bool DataManager::deleteSession(const std::string &sessionId) {
    std::lock_guard<std::mutex> lock(_mutex);

    // 1. 先删除该会话的所有消息
    {
        std::string deleteMessagesSQL = R"(
            DELETE FROM messages WHERE session_id = ?;
            )";

        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, deleteMessagesSQL.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            ERR("deleteSession - 删除消息准备语句失败：{}", sqlite3_errmsg(_db));
            return false;
        }

        sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_TRANSIENT);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            ERR("deleteSession - 删除消息执行语句失败：{}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return false;
        }

        sqlite3_finalize(stmt);
        INFO("deleteSession - 删除会话消息成功：{}", sessionId);
    }

    // 2. 再删除会话本身
    {
        std::string deleteSessionSQL = R"(
            DELETE FROM sessions WHERE session_id = ?;
            )";

        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, deleteSessionSQL.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            ERR("deleteSession - 删除会话准备语句失败：{}", sqlite3_errmsg(_db));
            return false;
        }

        sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_TRANSIENT);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            ERR("deleteSession - 删除会话执行语句失败：{}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return false;
        }

        sqlite3_finalize(stmt);
        INFO("deleteSession - 删除会话成功：{}", sessionId);
    }

    return true;
}

// 获取所有会话id
std::vector<std::string> DataManager::getAllSessionIds() const {
    std::lock_guard<std::mutex> lock(_mutex);

    // 构建SQL语句
    std::string selectSQL = R"(
        SELECT session_id FROM sessions ORDER BY update_time DESC;
        )";

    // 准备SQL语句
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(_db, selectSQL.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        ERR("getAllSessionIds - 准备语句失败：{}", sqlite3_errmsg(_db));
        return {};
    }

    std::vector<std::string> sessionIds;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string sessionId = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        sessionIds.push_back(sessionId);
    }

    // 释放语句
    sqlite3_finalize(stmt);
    INFO("getAllSessionIds - 获取所有会话id成功, 会话总数：{}", sessionIds.size());
    return sessionIds;
}

// 获取所有session信息，并按照更新时间降序排列
std::vector<std::shared_ptr<Session>> DataManager::getAllSessions() const {
    std::lock_guard<std::mutex> lock(_mutex);

    // 构建SQL语句
    std::string selectSQL = R"(
        SELECT session_id, model_name, create_time, update_time FROM sessions ORDER BY update_time DESC;
        )";

    // 准备SQL语句
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(_db, selectSQL.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        ERR("getAllSessions - 准备语句失败：{}", sqlite3_errmsg(_db));
        return {};
    }

    std::vector<std::shared_ptr<Session>> sessions;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string sessionId = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        std::string modelName = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        int64_t createTime = sqlite3_column_int64(stmt, 2);
        int64_t updateTime = sqlite3_column_int64(stmt, 3);

        auto session = std::make_shared<Session>(modelName);
        session->_sessionId = sessionId;
        session->_createdAt = static_cast<std::time_t>(createTime);
        session->_updatedAt = static_cast<std::time_t>(updateTime);
        sessions.push_back(session);

        // 历史消息暂时不获取，需要时再通过会话id来进行获取
    }

    // 释放语句
    sqlite3_finalize(stmt);
    INFO("getAllSessions - 获取所有会话信息成功, 会话总数：{}", sessions.size());
    return sessions;
}

// 获取会话总数
size_t DataManager::getSessionCount() const {
    std::lock_guard<std::mutex> lock(_mutex);

    // 准备SQL语句
    std::string selectSQL = R"(
        SELECT COUNT(*) FROM sessions;
        )";

    // 准备SQL语句
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(_db, selectSQL.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        ERR("getSessionCount - 准备语句失败：{}", sqlite3_errmsg(_db));
        return 0;
    }

    // 执行SQL语句
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        ERR("getSessionCount - 执行语句失败：{}", sqlite3_errmsg(_db));
        sqlite3_finalize(stmt);
        return 0;
    }

    // 获取会话总数
    size_t count = sqlite3_column_int64(stmt, 0);

    // 释放语句
    sqlite3_finalize(stmt);
    INFO("getSessionCount - 获取会话总数成功：{}", count);
    return count;
}

// 删除所有会话
bool DataManager::clearAllSessions() {
    std::lock_guard<std::mutex> lock(_mutex);

    // 构建SQL语句
    std::string deleteSQL = R"(
        DELETE FROM sessions;
        )";

    // 准备SQL语句
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(_db, deleteSQL.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        ERR("clearAllSessions - 准备语句失败：{}", sqlite3_errmsg(_db));
        return false;
    }

    // 执行SQL语句
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        ERR("clearAllSessions - 执行语句失败：{}", sqlite3_errmsg(_db));
        sqlite3_finalize(stmt);
        return false;
    }

    // 释放语句
    sqlite3_finalize(stmt);
    INFO("clearAllSessions - 删除所有会话成功");
    return true;
}

// 信息

bool DataManager::insertSessionMessage(const std::string &sessionId, const Message &message) {
    std::lock_guard<std::mutex> lock(_mutex);

    // 构建SQL语句
    std::string insertSQL = R"(
        INSERT INTO messages (message_id, session_id, role, content, timestamp)
        VALUES (?, ?, ?, ?, ?);
        )";

    // 准备SQL语句
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(_db, insertSQL.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        ERR("insertSessionMessage - 准备语句失败：{}", sqlite3_errmsg(_db));
        return false;
    }

    // 绑定参数
    sqlite3_bind_text(stmt, 1, message._messageId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, sessionId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, message._role.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, message._content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 5, static_cast<int64_t>(message._timestamp));

    // 执行SQL语句
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        ERR("insertSessionMessage - 执行语句失败：{}", sqlite3_errmsg(_db));
        sqlite3_finalize(stmt);
        return false;
    }

    // 同时更新session的update_time
    std::string updateSQL = R"(
        UPDATE sessions SET update_time = ? WHERE session_id = ?;
        )";

    // 准备SQL语句
    sqlite3_stmt *updateStmt;
    rc = sqlite3_prepare_v2(_db, updateSQL.c_str(), -1, &updateStmt, nullptr);
    if (rc != SQLITE_OK) {
        ERR("insertSessionMessage - 准备语句失败：{}", sqlite3_errmsg(_db));
        sqlite3_finalize(stmt);
        return false;
    }

    // 绑定参数
    sqlite3_bind_int64(updateStmt, 1, static_cast<int64_t>(message._timestamp));
    sqlite3_bind_text(updateStmt, 2, sessionId.c_str(), -1, SQLITE_TRANSIENT);

    // 执行SQL语句
    rc = sqlite3_step(updateStmt);
    if (rc != SQLITE_DONE) {
        ERR("insertSessionMessage - 执行语句失败：{}", sqlite3_errmsg(_db));
        sqlite3_finalize(updateStmt);
        return false;
    }

    // 释放语句
    sqlite3_finalize(stmt);
    sqlite3_finalize(updateStmt);
    INFO("insertSessionMessage - 插入消息成功：{}", message._messageId);
    return true;
}

// 获取会话中的所有消息
std::vector<Message> DataManager::getSessionMessages(const std::string &sessionId) const {
    std::lock_guard<std::mutex> lock(_mutex);

    // 准备SQL语句
    std::string selectSQL = R"(
        SELECT message_id, role, content, timestamp FROM messages WHERE session_id = ?;
        )";

    // 准备SQL语句
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(_db, selectSQL.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        ERR("getSessionMessages - 准备语句失败：{}", sqlite3_errmsg(_db));
        return {};
    }

    // 绑定参数
    sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_TRANSIENT);

    // 执行SQL语句
    std::vector<Message> messages;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Message message;
        message._messageId = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        message._role = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        message._content = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        message._timestamp = static_cast<std::time_t>(sqlite3_column_int64(stmt, 3));
        messages.push_back(message);
    }

    if (rc != SQLITE_DONE) {
        ERR("getSessionMessages - 执行语句失败：{}", sqlite3_errmsg(_db));
        sqlite3_finalize(stmt);
        return {};
    }

    // 释放语句
    sqlite3_finalize(stmt);
    return messages;
}

// 删除制定会话的历史消息
bool DataManager::deleteSessionMessages(const std::string &sessionId) {
    std::lock_guard<std::mutex> lock(_mutex);

    // 构建SQL语句
    std::string deleteSQL = R"(
        DELETE FROM messages WHERE session_id = ?;
        )";

    // 准备SQL语句
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(_db, deleteSQL.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        ERR("deleteSessionMessages - 准备语句失败：{}", sqlite3_errmsg(_db));
        return false;
    }

    // 绑定参数
    sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_TRANSIENT);

    // 执行SQL语句
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        ERR("deleteSessionMessages - 执行语句失败：{}", sqlite3_errmsg(_db));
        sqlite3_finalize(stmt);
        return false;
    }

    // 释放语句
    sqlite3_finalize(stmt);
    INFO("deleteSessionMessages - 删除会话消息成功：{}", sessionId);
    return true;
}
int DataManager::getSessionMessageCount(const std::string &sessionId) const {
    std::lock_guard<std::mutex> lock(_mutex);
    std::string sql = "SELECT COUNT(*) FROM messages WHERE session_id = ?;";
    sqlite3_stmt *stmt = nullptr;
    int rc = sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        ERR("getSessionMessageCount: prepare failed: {}", sqlite3_errmsg(_db));
        return 0;
    }
    sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    int count = 0;
    if (rc == SQLITE_ROW) {
        count = static_cast<int>(sqlite3_column_int64(stmt, 0));
    }
    sqlite3_finalize(stmt);
    return count;
}
} // namespace ai_chat_sdk
