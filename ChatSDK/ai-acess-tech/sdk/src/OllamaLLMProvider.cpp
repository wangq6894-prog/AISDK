#include "../include/OllamaLLMProvider.h"
#include "../include/util/myLog.h"
#include <cstdint>
#include <httplib.h>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/writer.h>

namespace ai_chat_sdk {
// OllamaLLMProvider 类的实现
bool OllamaLLMProvider::initModel(const std::map<std::string, std::string> &modelConfig) {
    if (modelConfig.find("model_name") == modelConfig.end() || modelConfig.find("model_desc") == modelConfig.end() ||
        modelConfig.find("endpoint") == modelConfig.end()) {
        ERR("OllamaLLMProvider initModel modelConfig is invalid");
        return false;
    }
    _modelName = modelConfig.at("model_name");
    _modelDesc = modelConfig.at("model_desc");
    _endpoint = modelConfig.at("endpoint");
    _isAvailable = true;
    return true;
}
bool OllamaLLMProvider::isAvailable() const {
    return _isAvailable;
}
std::string OllamaLLMProvider::getModelName() const {
    return _modelName;
}
std::string OllamaLLMProvider::getModelDesc() const {
    return _modelDesc;
}
std::string OllamaLLMProvider::sendMessage(const std::vector<Message> &messages,
                                           const std::map<std::string, std::string> &requestParam) {
    if (!_isAvailable) {
        ERR("OllamaLLMProvider sendMessage model is not available");
        return "";
    }
    double temperature = 0.7;
    int maxToken = 1024;
    if (requestParam.find("temperature") != requestParam.end()) {
        temperature = std::stod(requestParam.at("temperature"));
    }
    if (requestParam.find("max_tokens") != requestParam.end()) {
        maxToken = std::stoi(requestParam.at("max_tokens"));
    }

    Json::Value messageArray(Json::arrayValue);
    for (const auto &message : messages) {
        Json::Value messageObject(Json::objectValue);
        messageObject["role"] = message._role;
        messageObject["content"] = message._content;
        messageArray.append(messageObject);
    }

    Json::Value options(Json::objectValue);
    options["temperature"] = temperature;
    options["num_predict"] = maxToken;

    Json::Value requestBody(Json::objectValue);
    requestBody["model"] = _modelName;
    requestBody["messages"] = messageArray;
    requestBody["options"] = options;
    requestBody["stream"] = false;

    Json::StreamWriterBuilder writerBuilder;
    std::string requestBodyStr = Json::writeString(writerBuilder, requestBody);

    httplib::Client client(_endpoint.c_str());
    client.set_connection_timeout(30);
    client.set_read_timeout(60);

    httplib::Headers headers = {
        {"Content-Type", "application/json"},
        //{"Content-Length", std::to_string(requestBodyStr.size())},
    };

    // 6.发送POST请求
    auto response = client.Post("/api/chat", headers, requestBodyStr, "application/json");
    if (!response) {
        ERR("请求失败: {}", httplib::to_string(response.error()));
        return "";
    }
    INFO("响应状态码: {}", response->status);
    INFO("响应体: {}", response->body);
    if (response->status != 200) {
        ERR("请求失败: {}", response->body);
        return "";
    }

    // 7.解析响应
    Json::Value responseBody;
    Json::CharReaderBuilder readerBuilder;
    std::string parserError;
    std::istringstream responseStream(response->body);
    if (Json::parseFromStream(readerBuilder, responseStream, &responseBody, &parserError)) {
        if (responseBody.isMember("message") && responseBody["message"].isObject() &&
            responseBody["message"].isMember("content")) {
            auto modelResponse = responseBody["message"]["content"].asString();
            INFO("模型回复: {}", modelResponse);
            return modelResponse;
        }
        return "";
    }
    // 8.json解析失败
    ERR("解析响应失败: {}", parserError);
    return "OllamaLLMProvider解析响应失败";
}
std::string OllamaLLMProvider::sendMessageStream(const std::vector<Message> &messages,
                                                 const std::map<std::string, std::string> &requestParam,
                                                 std::function<void(const std::string &, bool)> callback) {
    INFO("OllamaLLMProvider::sendMessageStream: endpoint={}, model={}", _endpoint, _modelName);                                                
    // 检测模型是否可用
    if (!isAvailable()) {
        ERR("OllamaLLMProvider::sendMessageStream: model is not available");
        return "";
    }

    // 构造请求参数
    // 构造温度值和最大tokens数
    float temperature = 0.7f;
    int maxTokens = 1024;
    if (requestParam.find("temperature") != requestParam.end()) {
        temperature = std::stof(requestParam.at("temperature"));
    }
    if (requestParam.find("max_tokens") != requestParam.end()) {
        maxTokens = std::stoi(requestParam.at("max_tokens"));
    }

    // 构建历史消息
    Json::Value messageArray(Json::arrayValue);
    for (const auto &message : messages) {
        Json::Value messageObject(Json::objectValue);
        messageObject["role"] = message._role;
        messageObject["content"] = message._content;
        messageArray.append(messageObject);
    }

    // 构建请求体
    Json::Value options(Json::objectValue);
    options["temperature"] = temperature;
    options["num_predict"] = maxTokens;

    Json::Value requestBody(Json::objectValue);
    requestBody["model"] = _modelName;
    requestBody["messages"] = messageArray;
    requestBody["options"] = options;
    requestBody["stream"] = true;

    // 序列化请求体
    Json::StreamWriterBuilder writerBuilder;
    std::string requestBodyStr = Json::writeString(writerBuilder, requestBody);

    // 创建http客户端
    httplib::Client client(_endpoint.c_str());
    client.set_connection_timeout(30, 0);
    client.set_read_timeout(300, 0);
    // 设置请求头
    httplib::Headers headers = {{"Content-Type", "application/json"}};

    // 流式处理变量
    std::string buffer;
    bool gotError = false;
    std::string errorMsg;
    int statusCode = 0;
    bool streamFinish = false;
    std::string fullData;

    // 创建请求对象
    httplib::Request request;
    request.method = "POST";
    request.path = "/api/chat";
    request.headers = headers;
    request.body = requestBodyStr;
    // 响应头处理器
    request.response_handler = [&](const httplib::Response &res) {
        statusCode = res.status;
        if (statusCode != 200) {
            gotError = true;
            errorMsg =
                "OllamaLLMProvider::sendMessageStream: failed to send request, status: " + std::to_string(statusCode);
                ERR("OllamaLLMProvider::sendMessageStream: HTTP status code: {}, body preview: {}", statusCode, res.body);
            return false; // 终止请求
        }

        return true;
    };
    // 内容接收器
    request.content_receiver = [&](const char *data, size_t dataLen, size_t offset, size_t totalLength) {
        // 如果http响应头出错，就不需要接收后续数据
        if (gotError) {
            return false; // 终止接收
        }

        buffer.append(data, dataLen);

        // 处理每个数据块，数据块之间是以\n间隔的
        // 注意：此处接收到的数据块并不是模型返回的SSE格式的数据，而是经过Ollama服务器处理之后的数据
        size_t pos = 0;
        // while ((pos = buffer.find("\n", pos)) != std::string::npos) {
        //     std::string chunk = buffer.substr(0, pos);
        //     buffer.erase(0, pos + 1);

        //     if (chunk.empty()) {
        //         continue;
        //     }
        while (true) {
            size_t pos = buffer.find("\n");
            if (pos == std::string::npos)
                break;
            std::string chunk = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            if (chunk.empty())
                continue;
            // 反序列化
            Json::Value chunkJson;
            Json::CharReaderBuilder readerBuilder;
            std::string errors;
            std::istringstream chunkStream(chunk);
            if (!Json::parseFromStream(readerBuilder, chunkStream, &chunkJson, &errors)) {
                ERR("OllamaLLMProvider::sendMessageStream: failed to parse chunk json, errors: {}", errors);
                continue;
            }

            // 先检查错误
            if (chunkJson.isMember("error")) {
                std::string err = chunkJson["error"].asString();
                ERR("OllamaLLMProvider::sendMessageStream: Ollama error: {}", err);
                gotError = true;
                return false;
            }
            // 处理结束标记
            if (chunkJson.get("done", false).asBool()) {
                streamFinish = true;
                callback("", true);
                return true;
            }

            // 提取增量数据
            if (chunkJson.isMember("message") && chunkJson["message"].isMember("content")) {
                std::string delta = chunkJson["message"]["content"].asString();
                fullData += delta;
                callback(delta, false);
            }
        }
        return true;
    };

    // 给Ollama服务器发送请求
    auto response = client.send(request);
    if (!response) {
        ERR("OllamaLLMProvider::sendMessageStream: failed to send request, error: {}", to_string(response.error()));
        return "";
    }

    // 确保流式响应正常结束
    if (!streamFinish) {
        ERR("OllamaLLMProvider::sendMessageStream: stream not finish, fullData: {}", fullData);
        callback("", true);
    }

    return fullData;
}
} // namespace ai_chat_sdk
