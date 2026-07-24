# AISDK

大二下学期做的一个项目。当时想写个能跟 AI 聊天的网页，就搞了这个——底下接了 DeepSeek 和 Ollama，上面搭了个 HTTP 服务，前端页面调接口就行。

前前后后搞了一个多月，能跑。ChatGPT 和 Gemini 的框架写了但没调通，因为暂时没有配环境，暂时搁置

## 干了什么

### 统一模型接入

不同模型的 API 请求格式、返回格式、鉴权方式都不一样。写了个基类 `LLMProvider`，统一定了四个接口：

```cpp
virtual bool initModel(...);          // 初始化（塞 API Key 之类）
virtual bool isAvailable() const;     // 能不能用
virtual string sendMessage(...);      // 同步
virtual string sendMessageStream(...); // 流式
```

DeepSeek 和 Ollama 各实现了一个子类。之后想加新模型的话，继承基类实现这四个方法，注册进去就行，别的地方一行不用改。ChatGPT 和 Gemini 的类也写了，但 `sendMessage` 和 `sendMessageStream` 还没调通。

### 流式输出

就是那种 AI 一个字一个字往外蹦的效果。用的 SSE，大概流程：

1. 前端发消息过来
2. 后端用 httplib 向模型发请求，设了 `content_receiver` 回调
3. 模型那边返回一点数据，回调就触发一次
4. 数据拼到 buffer 里，按 `\n\n` 切块——SSE 协议规定事件之间用空行隔开
5. 切出来的块解析 JSON，取 `choices[0].delta.content`，这就是新生成的一小段文字
6. 推给前端
7. 直到收到 `[DONE]`，结束

这里有个细节：TCP 是流式的，不能保证一个数据块刚好对应一条完整的 SSE 消息。所以用 buffer 拼接 + `\n\n` 分割，没切出来的留在 buffer 里等下批数据到了继续拼。

### 接口

写了 7 个 REST 接口：

```
POST   /api/session              建会话
GET    /api/sessions             查会话列表
GET    /api/models               查模型列表
DELETE /api/session/{id}         删会话
GET    /api/session/{id}/history 看聊天记录
POST   /api/message              发消息（一次性返回）
POST   /api/message/async        发消息（流式）
```

每个接口处理逻辑都一样：解析 JSON 请求体 → 检查必填参数 → 调 SDK 对应方法 → 拼 JSON 响应返回。跨域头统一加了三个。

### 存数据

用的 SQLite。表就两张，sessions 和 messages，外键关联。

内存里用 `unordered_map<string, shared_ptr<Session>>` 也存了一份。读的时候先看内存有没有，没有再查数据库，查到了塞回内存。消息是懒加载的——列举会话的时候不查消息，只看某个会话的历史记录时才去数据库取。

并发这块，`SessionManager` 和 `DataManager` 各有一把 `mutex`，所有公开方法进来先加锁。session id 用 `atomic<int64_t>` 自增，不会撞。

### 工程

- CMake 构建，SDK 编成静态库 `libai_chat_sdk.a`，ChatServer 编成可执行文件
- gflags 处理命令行参数，端口、温度、最大 token 都能在启动时指定
- API Key 从环境变量读，不写代码里也不写配置文件里（最开始写配置文件里，后来发现会不小心提交到 git，赶紧改了）
- spdlog 打日志，五个级别

## 跑起来

Ubuntu 22.04，先装依赖：

```bash
sudo apt install build-essential cmake libssl-dev libsqlite3-dev libjsoncpp-dev libspdlog-dev libgflags-dev libfmt-dev
```

编 SDK：

```bash
cd sdk && mkdir build && cd build && cmake .. && make && sudo make install
```

编服务端：

```bash
cd ../../ChatServer && mkdir build && cd build && cmake .. && make
```

启动：

```bash
export deepseek_apikey=sk-xxxxx
./AIChatServer --port=8080
```

试一下：

```bash
# 建会话
curl -X POST http://localhost:8080/api/session \
  -H "Content-Type: application/json" \
  -d '{"model":"deepseek-chat"}'

# 发消息
curl -X POST http://localhost:8080/api/message \
  -H "Content-Type: application/json" \
  -d '{"session_id":"刚才返回那个","message":"你好"}'
```

## 自己觉得不满意的地方

- SessionManager 一把锁锁整个 map，请求多了要排队。应该按 sessionId 哈希分桶
- getSession 每次都查数据库重新加载消息，就算刚刚查过也是。加个简单的时间戳判断就能省很多次 IO
- insertSessionMessage 里面写消息和更新会话时间戳是两次 sqlite3_step，中间没有事务包裹
- ChatGPT 和 Gemini 没调通

## 用到的

C++17、CMake、cpp-httplib、jsoncpp、spdlog、gflags、SQLite3、SSE
