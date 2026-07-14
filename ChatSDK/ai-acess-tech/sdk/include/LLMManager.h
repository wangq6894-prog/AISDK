#pragma once
#include "LLMProvider.h"
#include <string>
#include <map>
#include <vector>
#include <functional>
#include "common.h"
#include <httplib.h>
#include "LLMProvider.h"
#include <functional>
#include <string>
#include <map>
#include <vector>
#include "common.h"
namespace ai_chat_sdk
{
    // LLMProvider 类
    class LLMManager
    {
    public:
        // 注册模型提供者
        bool registerProvider(const std::string& modelName, std::unique_ptr<LLMProvider> provider);
        //初始化指定模型
        bool initModel(const std::string& modelName, const std::map<std::string, std::string>& modelParam);
        //获取可用模型
        std::vector<ModelInfo> getAvailableModels() const;
        //检查模型是否可用
        bool isModelAvailable(const std::string& modelName) const;
        //发送消息给指定模型
        std::string sendMessage(const std::string& modelName, const std::vector<Message>& messages, const std::map<std::string, std::string>& requestParam);
        //发送消息给指定模型 - 增量返回 - 流式响应
        std::string sendMessageStream(const std::string& modelName, const std::vector<Message>& messages, const std::map<std::string, std::string>& requestParam, std::function<void(const std::string&, bool)> callback);
        
    private:
        //<模型名称，模型提供者>
        std::map<std::string, std::unique_ptr<LLMProvider>> _providers;
        //<模型名称，模型信息>
        std::map<std::string, ModelInfo> _modelInfos;
    };
} // end ai_chat_sdk