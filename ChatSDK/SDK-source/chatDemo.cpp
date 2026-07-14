#include<iostream>
#include<ai_chat_sdk/chat_sdk.h>
#include<ai_chat_sdk/util/my_logger.h>
void sendMessageStream(ai_chat_sdk::ChatSDK& chat_sdk, const std::string& session_id) {
    std::cout << "--------------------------------send message--------------------------------" << std::endl;
    std::cout << "user消息 > ";
    std::string message;
    std::getline(std::cin, message);

    std::cout << "--------------------------------消息发送完成--------------------------------" << std::endl;

    chat_sdk.sendMessageStream(sessionId: session_id, message, callback: [](const std::string& response, bool done) -> void {
        std::cout << "assistant消息 > " << response << std::endl;
        if (done) {
            std::cout << "--------------------------------消息接收完成--------------------------------" << std::endl;
        }
    });
}
int main(){
    bite::Logger::init_logger("aiChatServer", "stdout", spdlog::level::info);
    ai_chat_sdk::ChatSDK chat_sdk;
    ai_chat_sdk::ApiConfig deekseek;
    deekseek.api_key = std::getenv("DEEKSEEK_APIKEY");
    deekseek.temperature = 0.7;
    deekseek.max_token = 2048;
    deekseek.model_name = "deekseek-chat";
    std::vector<std::shared_ptr<ai_chat_sdk::Config>> configs;
    configs.push_back(std::make_shared<ai_chat_sdk::ApiConfig>(deekseek));
    //初始化模型
    chat_sdk.initModles(configs);
    std::cout<<"--------------------------------创建会话--------------------------------"<<std::endl;
    std::string session_id = chat_sdk.createSession(modelName: "deekseek-chat");
    std::cout<<"session_id: "<<session_id<<std::endl;

int userOP = 1;
while(true){
    std::cout<<"-----------1. send message   0. exit--------------------------------"<<std::endl;
    std::cin>>userOP;
    if(userOP == 0){
        break;
    }

    getchar();  // 输入用户操作选择之后，需要接收到缓冲区中的回车
    sendMessageStream(& chat_sdk, session_id);
}
    std::cout << "--------------程序退出--------------" << std::endl;
    return 0;
}
