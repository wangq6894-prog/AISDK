# AISDK

大二下学期做的一个项目。想写个能跟 AI 聊天的网页，就搞了这个——底下接 DeepSeek 和 Ollama，上面搭了个 HTTP 服务。

前后一个多月，能跑。ChatGPT 和 Gemini 的框架写了但没调通，暂时没配环境就先搁那了。

## 后续更改

现在gemini和chatgpt已经可以使用
三种模型三种调用方式没绷住

## 功能
本项目是一个AISDK项目，具有会话历史记录功能
## 使用方法
先进到ChatSDK/ai-acess-tech/sdk里编译并安装，再进到ChatSDK/ai-acess-tech/ChatServer中编译，之后直接运行即可，注意要先加载环境变量

cd SDK/ChatSDK/ai-acess-tech/sdk

mkdir -p build && cd build

cmake .. && make

sudo make install   #装到 /usr/local/lib/libai_chat_sdk.a + /usr/local/include/ai_chat_sdk/

cd SDK/ChatSDK/ai-acess-tech/ChatServer

mkdir -p build && cd build

cmake .. && make     #产出可执行文件 AIChatServer

export deepseek_apikey = sk-你的key

./AIChatServer

#指定端口 / 看帮助

./AIChatServer --port=9000

./AIChatServer --help


## 用到的

C++17、CMake、cpp-httplib、jsoncpp、spdlog、gflags、SQLite3、SSE

