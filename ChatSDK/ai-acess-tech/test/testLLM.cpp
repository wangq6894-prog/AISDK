#include "../sdk/include/ChatSDK.h"
#include "../sdk/include/ChatgptProvider.h"
#include "../sdk/include/DeepSeekProvider.h"
#include "../sdk/include/OllamaLLMProvider.h"
#include "../sdk/include/common.h"
#include "../sdk/include/util/myLog.h"
#include <gtest/gtest.h>
#include <iostream>
#include <istream>
#include <memory>
#include <spdlog/common.h>
#include <string>
#include <vector>
#if 0
TEST(DeepSeekProviderTest, sendMessageStream){
    auto provider = std::make_shared<ai_chat_sdk::DeepSeekProvider>();
    ASSERT_TRUE(provider != nullptr);

    std::map<std::string, std::string> modelParam;
    modelParam["api_key"] = std::getenv("deepseek_apikey");
    modelParam["endpoint"] = "https://api.deepseek.com";

    provider->initModel(modelParam);
    ASSERT_TRUE(provider->isAvailable());

    std::map<std::string, std::string> requestParam = {
        {"temperature", "0.7"},
        {"max_tokens", "2048"}
    };
    std::vector<ai_chat_sdk::Message> messages;
    messages.push_back({"user", "你是谁？"});

    // 实例化DeepSeekProvider的对象
    // 调用sendMessage方法
    //std::string response = provider->sendMessage(messages, requestParam);

    auto writeChunk = [&](const std::string& chunk, bool last){
        INFO("chunk : {}", chunk);
        if(last){
            INFO("[DONE]");
        } 
    };
    std::string fullData = provider->sendMessageStream(messages, requestParam, writeChunk);
    ASSERT_FALSE(fullData.empty());
    INFO("response : {}", fullData);
}

TEST(ChatgptProviderTest, sendMessageStream){
    auto provider = std::make_shared<ai_chat_sdk::ChatgptProvider>();
    ASSERT_TRUE(provider != nullptr);

    std::map<std::string, std::string> modelParam;
    modelParam["api_key"] = std::getenv("chatgpt_apikey");
    modelParam["endpoint"] = "https://api.openai.com";

    provider->initModel(modelParam);
    ASSERT_TRUE(provider->isAvailable());

    std::map<std::string, std::string> requestParam = {
        {"temperature", "0.7"},
        {"max_tokens", "2048"}};
    std::vector<ai_chat_sdk::Message> messages;
    messages.push_back({"user", "你是谁？"});

    // 实例化ChatgptProvider的对象
    // 调用sendMessage方法
    //std::string fullData = provider->sendMessage(messages, requestParam);

    auto writeChunk = [&](const std::string& chunk, bool last){
        INFO("chunk : {}", chunk);
        if(last){
             INFO("[DONE]");
        }
    };
    std::string fullData = provider->sendMessageStream(messages, requestParam, writeChunk);
    ASSERT_FALSE(fullData.empty());
    INFO("response : {}", fullData);
}

TEST(OllamaLLMProviderTest, sendMessage){
    auto provider = std::make_shared<ai_chat_sdk::OllamaLLMProvider>();
    ASSERT_TRUE(provider != nullptr);

    std::map<std::string, std::string> modelParam;
    modelParam["model_name"] = "deepseek-r1:1.5b";
    modelParam["model_desc"] = "本地部署deepseek-r1:1.5b模型，采用专家混合架构，专注于深度理解与推理";
    modelParam["endpoint"] = "http://localhost:11434";

    provider->initModel(modelParam);
    ASSERT_TRUE(provider->isAvailable());

    std::map<std::string, std::string> requestParam = {
        {"temperature", "0.7"},
        {"max_tokens", "2048"}
    };
    std::vector<ai_chat_sdk::Message> messages;
    messages.push_back({"user", "你是谁？"});

    // 实例化OllamaLLMProvider的对象
    // 调用sendMessage方法
    std::string fullData = provider->sendMessage(messages, requestParam);
    ASSERT_FALSE(fullData.empty());

    // auto writeChunk = [&](const std::string& chunk, bool last){ 
    //     INFO("chunk : {}", chunk);
    //     if(last){
    //         INFO("[DONE]"); 
    //     } 
    // };
    // std::string fullData = provider->sendMessageStream(messages, requestParam, writeChunk);
    // ASSERT_FALSE(fullData.empty());
    INFO("response : {}", fullData);
}

#endif

TEST(ChatSDKTest, sendMessage) {
    auto sdk = std::make_shared<ai_chat_sdk::ChatSDK>();
    ASSERT_TRUE(sdk != nullptr);
    // deepseek-v4-pro
    auto deepseekConfig = std::make_shared<ai_chat_sdk::APIConfig>();
    ASSERT_TRUE(deepseekConfig != nullptr);
    deepseekConfig->_modelName = "deepseek-chat";
    deepseekConfig->_apiKey = std::getenv("deepseek_apikey");
    deepseekConfig->_temperature = 0.7;
    deepseekConfig->_maxTokens = 2048;
    // gpt-4o-mini
    auto gptConfig = std::make_shared<ai_chat_sdk::APIConfig>();
    ASSERT_TRUE(gptConfig != nullptr);
    gptConfig->_modelName = "gpt-4o-mini";
    gptConfig->_apiKey = std::getenv("chatgpt_apikey");
    gptConfig->_temperature = 0.7;
    gptConfig->_maxTokens = 2048;
    // gemini-pro-v1
    auto geminiConfig = std::make_shared<ai_chat_sdk::APIConfig>();
    ASSERT_TRUE(geminiConfig != nullptr);
    geminiConfig->_modelName = "gemini-2.0-flash";
    geminiConfig->_apiKey = std::getenv("gemini_apikey");
    geminiConfig->_temperature = 0.7;
    geminiConfig->_maxTokens = 2048;
    // OLLAMA
    auto ollamaConfig = std::make_shared<ai_chat_sdk::OllamaConfig>();
    ASSERT_TRUE(ollamaConfig != nullptr);
    ollamaConfig->_modelName = "deepseek-r1:1.5b";
    ollamaConfig->_modelDesc = "本地部署deepseek-r1:1.5b模型，采用专家混合架构，专注于深度理解与推理";
    ollamaConfig->_endpoint = "http://localhost:11434";
    ollamaConfig->_temperature = 0.7;
    ollamaConfig->_maxTokens = 2048;
    std::vector<std::shared_ptr<ai_chat_sdk::Config>> modelConfigs = {
        deepseekConfig,
        gptConfig,
        geminiConfig,
        ollamaConfig,
    };
    sdk->initModels(modelConfigs);

    //创建对话
    auto SessionId = sdk->createSession(ollamaConfig->_modelName);
    ASSERT_FALSE(SessionId.empty());
    // 发送消息
    std::string message;
    std::cout << ">>>";
    std::getline(std::cin, message);
    auto response = sdk->sendMessage(SessionId, message);
    ASSERT_FALSE(response.empty());
    std::cout << ">>>";
    std::getline(std::cin, message);
    sdk->sendMessage(SessionId, message);
    ASSERT_FALSE(response.empty());
    INFO("response : {}", response);

    //获取会话历史消息
    auto messages = sdk->_sessionManager.getHistroyMessages(SessionId);
    std::string historyStr;
    for (const auto& msg : messages) {
       std::cout << "[" << msg._role << "]: " << msg._content << std::endl;
    }
    ASSERT_FALSE(messages.empty());
}

int main(int argc, char **argv) {
    // 初始化spdlog日志库
    bite::Logger::initLogger("testLLM", "stdout", spdlog::level::debug);

    // 初始化gtest库
    testing::InitGoogleTest(&argc, argv);

    // 执行所有的测试用例
    return RUN_ALL_TESTS();
}
