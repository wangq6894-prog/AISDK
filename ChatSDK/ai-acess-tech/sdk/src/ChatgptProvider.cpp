#include "../include/ChatgptProvider.h"
#include "../include/util/myLog.h"
#include <cstdint>
#include <jsoncpp/json/json.h>
#include <httplib.h>
#include <jsoncpp/json/reader.h>
namespace ai_chat_sdk
{
    bool ChatgptProvider::initModel(const std::map<std::string, std::string> &modelConfig)
    {
        // 初始化api_key
        auto it = modelConfig.find("api_key");
        if (it == modelConfig.end())
        {
            // 未找到api_key参数
            ERR("未找到api_key参数");
            return false;
        }
        else
        {
            _apiKey = it->second;
        }

        // 初始化Base_URL
        it = modelConfig.find("endpoint");
        if (it == modelConfig.end())
        {
            // 未找到endpoint参数
            _endpoint = "https://api.openai.com";
        }
        else
        {
            _endpoint = it->second;
        }
        _isAvailable = true;
        INFO("ChatgptProvider初始化成功");
        return true;
    }
    // 检测模型是否可用
    bool ChatgptProvider::isAvailable() const
    {
        return _isAvailable;
    }
    // 获取模型名称
    std::string ChatgptProvider::getModelName() const
    {
        return "gpt-4o-mini";
    }
    // 获取模型描述
    std::string ChatgptProvider::getModelDesc() const
    {
        return "OPENAI GPT-4o-mini";
    }
    // 发送消息 - 全量返回
    std::string ChatgptProvider::sendMessage(const std::vector<Message> &messages, const std::map<std::string, std::string> &requestParam)
    {
        // 检测模型是否可用
        if (!isAvailable())
        {
            ERR("模型未初始化");
            return "";
        }
        // 2.构造请求参数
        double temperature = 0.7;
        int max_output_tokens = 2048;
        if (requestParam.find("temperature") != requestParam.end())
        {
            temperature = std::stod(requestParam.at("temperature"));
        }
        if (requestParam.find("max_tokens") != requestParam.end())
        {
            max_output_tokens = std::stoi(requestParam.at("max_tokens"));
        }
        // 构造历史消息
        Json::Value messageArray(Json::arrayValue);
        for (const auto &message : messages)
        {
            Json::Value messageObject;
            messageObject["role"] = message._role;
            messageObject["content"] = message._content;
            messageArray.append(messageObject);
        }
        // 3.构造请求体
        Json::Value requestBody;
        requestBody["model"] = getModelName();
        requestBody["messages"] = messageArray;
        requestBody["temperature"] = temperature;
        requestBody["max_tokens"] = max_output_tokens;
        // 4.序列化
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "";
        std::string requestBodyStr = Json::writeString(writerBuilder, requestBody);
        INFO("请求体: {}", requestBodyStr);
        // 使用cpp-http库构造Http客户端
        httplib::Client client(_endpoint);
        client.set_connection_timeout(30, 0);
        client.set_read_timeout(60, 0);
        client.set_proxy("127.0.0.1", 7890);
        // 5.设置请求头
        httplib::Headers headers = {
            {"Authorization", "Bearer " + _apiKey},
            {"Content-Type", "application/json"}};
        // 6.发送POST请求
        auto response = client.Post("/v1/responses", headers, requestBodyStr, "application/json");
        if (!response)
        {
            ERR("请求失败: 网络错误");
            return "";
        }
        INFO("响应状态码: {}", response->status);
        INFO("响应体: {}", response->body);
        if (response->status != 200)
        {
            ERR("请求失败: {}", response->body);
            return "";
        }

        // 7.解析响应
        Json::Value responseBody;
        Json::CharReaderBuilder readerBuilder;
        std::string parserError;
        std::istringstream responseStream(response->body);
        if (!Json::parseFromStream(readerBuilder, responseStream, &responseBody, &parserError))
        {
            ERR("解析响应失败: {}", parserError);
            return "ChatgptProvider解析响应失败";
        }

        if (!responseBody.isMember("output") || !responseBody["output"].isArray() || responseBody["output"].empty())
        {
            ERR("响应格式错误: 缺少output字段");
            return "";
        }

        const auto &output = responseBody["output"][0];
        if (!output.isMember("content") || !output["content"].isArray() || output["content"].empty())
        {
            ERR("响应格式错误: output缺少content字段");
            return "";
        }

        const auto &content = output["content"][0];
        if (!content.isMember("text"))
        {
            ERR("响应格式错误: content缺少text字段");
            return "";
        }

        std::string relyContent = content["text"].asString();
        INFO("模型回复: {}", relyContent);
        return relyContent;
    }
    // 发送消息 - 增量返回 流式响应
    std::string ChatgptProvider::sendMessageStream(const std::vector<Message> &messages,
                                                   const std::map<std::string, std::string> &requestParam,
                                                   std::function<void(const std::string &, bool)> callback)
    {
        // 1. 检测模型是否可用
        if (!isAvailable())
        {
            ERR("ChatgptProvider sendMessageStream model not available");
            return "";
        }

        // 2. 构造请求参数
        double temperature = 0.7;
        int maxTokens = 2048;
        if (requestParam.find("temperature") != requestParam.end())
        {
            temperature = std::stod(requestParam.at("temperature"));
        }
        if (requestParam.find("max_tokens") != requestParam.end())
        {
            maxTokens = std::stoi(requestParam.at("max_tokens"));
        }

        // 构造历史消息
        Json::Value messageArray(Json::arrayValue);
        for (const auto &message : messages)
        {
            Json::Value messageObject;
            messageObject["role"] = message._role;
            messageObject["content"] = message._content;
            messageArray.append(messageObject);
        }

        // 3. 构造请求体
        Json::Value requestBody;
        requestBody["model"] = getModelName();
        requestBody["messages"] = messageArray;
        requestBody["temperature"] = temperature;
        requestBody["max_tokens"] = maxTokens;
        requestBody["stream"] = true;

        // 4. 序列化
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "";
        std::string requestBodyStr = Json::writeString(writerBuilder, requestBody);
        INFO("ChatgptProvider sendMessageStream requestBody: {}", requestBodyStr);

        // 5. 使用cpp-httplib库构造HTTP客户端
        httplib::Client client(_endpoint.c_str());
        client.set_connection_timeout(30, 0); // 连接超时时间为30秒
        client.set_read_timeout(300, 0);      // 流式响应需要更长的时间，设置超时时间为300秒

        // 设置请求头
        httplib::Headers headers = {
            {"Authorization", "Bearer " + _apiKey},
            {"Content-Type", "application/json"},
            {"Accept", "text/event-stream"}};

        // 流式处理变量
        std::string buffer;        // 接受流式响应的数据块
        bool gotError = false;     // 标记响应是否成功
        std::string errorMsg;      // 错误描述符
        int statusCode = 0;        // 响应状态码
        bool streamFinish = false; // 标记流式响应是否完成
        std::string fullResponse;  // 累积完整的响应

        // 创建请求对象
        httplib::Request req;
        req.method = "POST";
        req.path = "/v1/responses";
        req.headers = headers;
        req.body = requestBodyStr;
        // 设置响应处理器
        req.response_handler = [&](const httplib::Response &res)
        {
            if (res.status != 200)
            {
                gotError = true;
                errorMsg = "HTTP status code: " + std::to_string(res.status);
                return false; // 终止请求
            }
            return true; // 继续接收后续数据
        };

        // 设置数据接收处理器--解析流式响应的每个块的数据
        req.content_receiver = [&](const char *data, size_t dataLength, size_t offset, size_t totalLength)
        {
            // 验证响应头是否错误，如果出错就不需要再继续接收数据
            if (gotError)
            {
                return false;
            }

            // 追加数据到buffer
            buffer.append(data, dataLength);
            INFO("ChatgptProvider sendMessageStream buffer: {}", buffer);

            // 处理所有的流式响应的数据块，注意：数据块之间是一个\n\n分隔
            size_t pos = 0;
            while ((pos = buffer.find("\n\n", pos)) != std::string::npos)
            {
                std::string event = buffer.substr(0, pos);
                buffer.erase(0, pos + 2);

                std::istringstream eventStream(event);
                std::string eventType;
                std::string eventData;
                std::string line;
                while (std::getline(eventStream, line))
                {
                    if (line.empty())
                    {
                        continue;
                    }
                    /*event: */
                    if (line.compare(0, 6, "event:") == 0)
                    {
                        size_t start = 6;
                        while (start < line.size() && std::isspace(line[start])) start++;
                        eventType = line.substr(start);
                    }
                    else if (line.compare(0, 5, "data:") == 0)
                    {
                        size_t start = 5;
                        while (start < line.size() && std::isspace(line[start])) start++;
                        eventData = line.substr(start);
                    }
                }

                // 對模型返回的結果進行序列化
                Json::Value chunk;
                Json::CharReaderBuilder reader;
                std::string errs;
                std::istringstream eventDataStream(eventData);
                if (!Json::parseFromStream(reader, eventDataStream, &chunk, &errs))
                {
                    ERR("Error parsing JSON: {}", errs);
                    continue;
                }

                // 按照eventType处理数据
                if (eventType == "response.output_text.delta")
                {
                    if (chunk.isMember("delta") && chunk["delta"].isString())
                    {
                        std::string delta = chunk["delta"].asString();
                        fullResponse += delta;
                        callback(delta, false);
                    }
                }
                else if (eventType == "response.output_text.done")
                {
                    if (chunk.isMember("item") && chunk["item"].isObject())
                    {
                        Json::Value item = chunk["item"];
                        if (item.isMember("content") && item["content"].isArray() && !item["content"].empty())
                        {
                            Json::Value firstContent = item["content"][0];
                            if (firstContent.isObject() && firstContent.isMember("text") && firstContent["text"].isString())
                            {
                                // 这里如果是 done 事件，通常是完整内容，但如果只是为了记录 fullResponse，可以按需处理
                                // 注意：如果 delta 已经累加过了，这里可能不需要重复累加到 fullResponse
                            }
                            else if (firstContent.isString())
                            {
                                // 兼容不同格式
                            }
                        }
                    }
                }
                else if (eventType == "response.completed")
                {
                    streamFinish = true;
                    callback("", true);
                    return true;
                }
            }
            return true;
        };
        // 给模型发送请求
        auto result = client.send(req);
        if (!result)
        {
            // 请求失败
            ERR("Network error: {}", to_string(result.error()));
            return "";
        }
        if (!streamFinish)
        {
            // 流式响应未结束
            WARN("stream ended without [DONE] maker");
            callback("", true);
            return "";
        }
        return fullResponse;
    }
}
