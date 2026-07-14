#include <sqlite3.h>
#include <gtest/gtest.h>
#include <string>
struct StudentInfo
{
    std::string name;
    std::string gender;
    int age;
    double gap;
};
class StudentDB
{
public:
    StudentDB(const std::string &dbName)
    {
        int rc = sqlite3_open(dbName.c_str(), &_db);
        if (rc != SQLITE_OK)
        {
            std::cerr << "Error opening database: " << sqlite3_errmsg(_db) << std::endl;
            sqlite3_close(_db);
        }
    };
    ~StudentDB()
    {
        if (_db != nullptr)
        {
            sqlite3_close(_db);
        }
    };
    bool insertStudentInfo(const StudentInfo &info)
    { // R"(...)" 内的所有字符都按原样解释， 不需要转义 。
        const std::string insertSQL = R"( 
        INSERT INTO Student (name, gender, age, gap) VALUES (?, ?, ?, ?);
        )";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, insertSQL.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            std::cerr << "Error preparing statement: " << sqlite3_errmsg(_db) << std::endl;
            return false;
        }
        sqlite3_bind_text(stmt, 1, info.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, info.gender.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, info.age);
        sqlite3_bind_double(stmt, 4, info.gap);
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE)
        {
            std::cerr << "Error inserting data: " << sqlite3_errmsg(_db) << std::endl;
            return false;
        }
        sqlite3_finalize(stmt);
        return true;
    }
    bool queryStudentInfo(const std::string &name)
    {
        // 查询学生信息
        std::string querySQL = "SELECT * FROM Student WHERE name = ?";
        sqlite3_stmt *stmt;
        // 编译SQL语句
        int rc = sqlite3_prepare_v2(_db, querySQL.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            std::cerr << "Error preparing statement: " << sqlite3_errmsg(_db) << std::endl;
            return false;
        }
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_ROW)
        {
            std::cerr << "Error querying data: " << sqlite3_errmsg(_db) << std::endl;
            sqlite3_finalize(stmt);
            return false;
        }
        int stuid = sqlite3_column_int(stmt, 0);
        std::string queryName = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        std::string queryGender = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        int queryAge = sqlite3_column_int(stmt, 3);
        double queryGap = sqlite3_column_double(stmt, 4);
        std::cout << "stuid : " << stuid << std::endl;
        std::cout << "queryName : " << queryName << std::endl;
        std::cout << "queryGender : " << queryGender << std::endl;
        std::cout << "queryAge : " << queryAge << std::endl;
        std::cout << "queryGap : " << queryGap << std::endl;
        sqlite3_finalize(stmt);
        return true;
    }
    bool queryAllStudentInfo()
    {
        std::string querySQL = R"(SELECT * FROM Student)";
        // 准备SQL语句
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, querySQL.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            std::cerr << "准备语句失败：" << sqlite3_errmsg(_db) << std::endl;
            return false;
        }

        // 执行SQL语句
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_ROW && rc != SQLITE_DONE)
        {
            std::cerr << "执行语句失败：" << sqlite3_errmsg(_db) << std::endl;
            sqlite3_finalize(stmt);
            return false;
        }

        // 提取结果
        std::cout << "-------------所有学生信息-----------------" << std::endl;
        while (rc == SQLITE_ROW)
        {
            int stuid = sqlite3_column_int(stmt, 0);
            std::string queryName = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            std::string queryGender = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            int queryAge = sqlite3_column_int(stmt, 3);
            double queryGap = sqlite3_column_double(stmt, 4);

            // 打印结果
            std::cout << "查询到学生信息：" << std::endl;
            std::cout << "stuid: " << stuid << " "
                      << "name: " << queryName << " "
                      << "gender: " << queryGender << " "
                      << "age: " << queryAge << " "
                      << "gap: " << queryGap << std::endl;

            // 继续提取下一行
            rc = sqlite3_step(stmt);
        }

        // 检查是否还有更多行
        if (rc != SQLITE_DONE)
        {
            std::cerr << "提取结果失败：" << sqlite3_errmsg(_db) << std::endl;
            sqlite3_finalize(stmt);
            return false;
        }

        // 清理
        sqlite3_finalize(stmt);
        return true;
    }
    bool updateStudentInfo(const std::string &name, const StudentInfo &info)
    { // 更新学生信息
        std::string updateSQL = R"(
    UPDATE Student SET gender = ?, age = ?, gap = ? WHERE name = ?;
)";

        // 准备SQL语句
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, updateSQL.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            std::cerr << "准备语句失败：" << sqlite3_errmsg(_db) << std::endl;
            return false;
        }

        // 绑定参数
        sqlite3_bind_text(stmt, 1, info.gender.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, info.age);
        sqlite3_bind_double(stmt, 3, info.gap);
        sqlite3_bind_text(stmt, 4, name.c_str(), -1, SQLITE_TRANSIENT);

        // 执行SQL语句
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE && rc != SQLITE_ROW)
        {
            std::cerr << "执行语句失败：" << sqlite3_errmsg(_db) << std::endl;
            sqlite3_finalize(stmt);
            return false;
        }

        // 清理
        sqlite3_finalize(stmt);
        return true;
    }
    bool deleteStudentInfo(const std::string &name)
    {
        // 删除学生信息
        std::string deleteSQL = R"(
        DELETE FROM Student WHERE name = ?;
    )";

        // 准备SQL语句
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, deleteSQL.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            std::cerr << "准备语句失败：" << sqlite3_errmsg(_db) << std::endl;
            return false;
        }

        // 绑定参数
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);

        // 执行SQL语句
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE && rc != SQLITE_ROW)
        {
            std::cerr << "执行语句失败：" << sqlite3_errmsg(_db) << std::endl;
            sqlite3_finalize(stmt);
            return false;
        }

        // 清理
        sqlite3_finalize(stmt);
        return true;
    }

private:
    bool initDataBase()
    {
        const std::string crateTableSQL = "CREATE TABLE IF NOT EXISTS Student (name TEXT, gender TEXT, age INTEGER, gap REAL)";
        int rc = sqlite3_exec(_db, crateTableSQL.c_str(), nullptr, nullptr, nullptr);
        if (rc != SQLITE_OK)
        {
            std::cerr << "Error creating table: " << sqlite3_errmsg(_db) << std::endl;
            return false;
        }
        return true;
    }
    sqlite3 *_db;
};
int main()
{
    StudentInfo info1 = {"张三", "男", 18, 3.5};
    StudentInfo info2 = {"李四", "女", 19, 3.8};
    StudentInfo info3 = {"王五", "男", 20, 4.0};
    StudentInfo info4 = {"赵六", "女", 21, 4.2};

    StudentDB db("studentDB.db");
    db.insertStudentInfo(info1);
    db.insertStudentInfo(info2);
    db.insertStudentInfo(info3);
    db.insertStudentInfo(info4);

    // 查询所有学生信息
    db.queryAllStudentInfo();

    info3.gap = 4.5;
    db.updateStudentInfo(info3.name, info3);
    db.queryStudentInfo(info3.name);

    // 删除学生信息
    db.deleteStudentInfo(info4.name);
    db.queryAllStudentInfo();

    return 0;
}