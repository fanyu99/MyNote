---
title: RTSP 协议
type: 主题笔记
topic: 音视频
status: 学习中
created: 2026-10-25
updated: 2026-10-25
tags:
  - ffmpeg
  - 音视频
  - rtsp
  - 流媒体
  - 协议
series: 音视频 ffmpeg
aliases:
  - RTSP
  - 实时流协议
canvas: 白板/RTSP协议.canvas
---

# RTSP 协议

> RTSP 会话模型、OPTIONS/DESCRIBE/SETUP/PLAY 流程，以及 ffmpeg 推拉流与联调排错。

> [!abstract] 一句话
> RTSP 是流媒体的「遥控器」：它只负责 **控制**（描述、建立、播放、暂停、拆除），真正的音视频数据由 **RTP** 承载，**RTCP** 负责质量反馈与音画同步，**SDP** 负责描述「有哪些流、用什么编码、参数是什么」。

---

## 零、先建立心智模型

把一次 RTSP 会话想成「点播一台远程录像机」：

| 角色 | 类比 | 实际协议 |
| --- | --- | --- |
| 遥控器 | 按播放/暂停/停止 | **RTSP** |
| 节目单 | 告诉你有哪些频道、什么制式 | **SDP** |
| 送节目的卡车 | 真正搬运画面和声音 | **RTP** |
| 路况回执 | 报告丢包、抖动、对时 | **RTCP** |

四个协议各司其职，**不要把它们混成一个东西**：

```text
应用层：  RTSP（控制）        SDP（媒体描述）
传输层：  RTP（媒体数据）     RTCP（质量反馈 / 对时）
网络层：  UDP（低延迟、可能丢包） / TCP（可靠、延迟更高）
```

- **RTSP 本身不传音视频**，只传文本命令。
- **RTP 不负责握手**，它只管按序号和时间戳发数据。
- **RTCP 不传媒体**，它定期发发送者报告（SR）和接收者报告（RR）。
- **SDP 不是协议**，它是一段文本格式的媒体描述。

> [!tip] 最实用的一条结论
> 90% 的 RTSP 问题，最终都能归到三件事：**SDP 里的编码参数对不对、Transport 选的 UDP 还是 TCP、时间戳/关键帧是否正常**。

---

## 一、RTSP 是什么

### 1.1 定义与标准

RTSP（Real Time Streaming Protocol，实时流协议）是一个 **应用层控制协议**，用来远程控制流媒体服务器上的音视频流：建立会话、选择传输方式、开始/暂停播放、录制、拆除会话。

| 版本 | 标准 | 说明 |
| --- | --- | --- |
| RTSP 1.0 | RFC 2326（1998） | 事实标准，IP 摄像机、NVR、MediaMTX、ZLMediaKit 基本都用它 |
| RTSP 2.0 | RFC 7826（2016） | 设计更完善，但设备兼容性差，实际很少见 |

特点：

- **文本协议**，语法和 HTTP/1.1 很像：请求行 + 头部 + 空行 + 可选 body。
- **有状态**：`SETUP` 之后服务器分配 `Session`，后续请求都要带这个 ID。
- **带外控制**：控制走 RTSP 连接，媒体走另一条 RTP 通道（UDP 端口或 TCP 交织通道）。
- **支持拉流和推流**：拉流是服务器发媒体给客户端；推流是客户端发媒体给服务器。注意：**无论推还是拉，RTSP 客户端都是主动发起连接的一方**。
- **默认端口 554**（RTSP over TCP）。

URL 形如：

```text
rtsp://[user:password@]host[:port]/path[?query]
rtsp://admin:12345@192.168.1.64:554/Streaming/Channels/101
rtsp://127.0.0.1:8554/live
```

### 1.2 RTSP 与 HTTP 的区别

| 维度 | RTSP | HTTP |
| --- | --- | --- |
| 状态 | 有会话状态（Session） | 无状态 |
| 媒体通道 | 带外，RTP/RTCP | 带内，就是 HTTP body |
| 方法 | OPTIONS/DESCRIBE/SETUP/PLAY/... | GET/POST/PUT/... |
| 典型用途 | 实时监控、低延迟直播 | 网页、点播文件、HLS |
| 传输 | 控制 TCP，媒体 UDP/TCP | TCP |

RTSP 借用了 HTTP 的「方法 + 状态码 + 头部」形式，但它是为「持续控制一条实时流」设计的。

### 1.3 和其他流媒体协议对比

| 协议 | 承载 | 典型延迟 | 主要方向 | 典型场景 |
| --- | --- | --- | --- | --- |
| **RTSP/RTP** | TCP / UDP | 100ms ~ 1s | 拉流为主，也支持推流 | IP 摄像机、NVR、局域网监看 |
| RTMP | TCP | 1 ~ 3s | 推流为主 | 直播推流、旧版网页直播 |
| HTTP-FLV | TCP | 1 ~ 3s | 拉流 | 网页低延迟直播 |
| HLS | HTTP/TCP | 5 ~ 30s | 拉流 | CDN 分发、点播 |
| WebRTC | UDP/SRTP | < 500ms | 双向 | 视频会议、网页实时通话 |
| SRT | UDP | 低 | 推/拉 | 远距离抗丢包传输、上行回传 |

选择建议：

- **局域网监控、相机接入、ffmpeg 生态**：RTSP。
- **网页无插件低延迟**：WebRTC 或 HTTP-FLV。
- **大规模 CDN 分发**：HLS。
- **公网弱网上行**：SRT。

### 1.4 关键术语

| 术语 | 含义 |
| --- | --- |
| **Track** | 一路媒体流，通常是一路视频 + 一路音频 |
| **Session** | 一次 RTSP 会话，`SETUP` 后由服务器分配 ID，带超时 |
| **Aggregate control** | 用同一个 URL 控制所有 track |
| **Transport** | 媒体承载方式：UDP 单播、UDP 组播、TCP 交织 |
| **NPT** | Normal Play Time，播放进度，如 `npt=0.000-` |
| **SSRC** | RTP 流的同步源标识 |
| **PT** | RTP 负载类型，96~127 为动态类型 |
| **Marker** | RTP 头标记位，常表示一帧结束 |
| **GOP cache** | 服务器缓存最近一个 I 帧及后续帧，新客户端可立即出画面 |

---

## 二、RTSP 的通信流程

### 2.1 方法总览

| 方法 | 方向 | 作用 | 是否必需 |
| --- | --- | --- | --- |
| `OPTIONS` | 客户端 → 服务器 | 查询服务器支持哪些方法 | 常见，但非严格必需 |
| `DESCRIBE` | 客户端 → 服务器 | 获取 SDP 媒体描述 | 拉流必需 |
| `ANNOUNCE` | 客户端 → 服务器 | 推送 SDP，告诉服务器我要发什么 | 推流必需 |
| `SETUP` | 客户端 → 服务器 | 建立某一路 track 的传输方式 | 必需 |
| `PLAY` | 客户端 → 服务器 | 开始/恢复播放 | 拉流必需 |
| `PAUSE` | 客户端 → 服务器 | 暂停 | 可选 |
| `RECORD` | 客户端 → 服务器 | 开始录制/接收推流 | 推流常用 |
| `TEARDOWN` | 客户端 → 服务器 | 结束会话，释放资源 | 推荐 |
| `GET_PARAMETER` | 客户端 → 服务器 | 查询参数，常被用作保活 | 可选 |
| `SET_PARAMETER` | 客户端 → 服务器 | 设置参数，也可保活 | 可选 |
| `REDIRECT` | 服务器 → 客户端 | 让客户端重定向到另一台服务器 | 可选 |

### 2.2 拉流（PLAY）完整时序

```mermaid
sequenceDiagram
    autonumber
    participant C as RTSP Client（播放端）
    participant S as RTSP Server（相机 / MediaMTX）

    C->>S: OPTIONS rtsp://host/live RTSP/1.0<br/>CSeq: 1
    S-->>C: 200 OK<br/>Public: OPTIONS, DESCRIBE, SETUP, PLAY, ...

    C->>S: DESCRIBE rtsp://host/live RTSP/1.0<br/>CSeq: 2<br/>Accept: application/sdp
    S-->>C: 200 OK<br/>Content-Type: application/sdp<br/>Content-Base: rtsp://host/live/<br/>(SDP body)

    C->>S: SETUP rtsp://host/live/streamid=0 RTSP/1.0<br/>CSeq: 3<br/>Transport: RTP/AVP#59;unicast#59;client_port=50000-50001
    S-->>C: 200 OK<br/>Transport: ...#59;server_port=60000-60001#59;ssrc=1234ABCD<br/>Session: 12345678#59;timeout=60

    C->>S: SETUP rtsp://host/live/streamid=1 RTSP/1.0<br/>CSeq: 4<br/>Session: 12345678<br/>Transport: ...
    S-->>C: 200 OK<br/>Session: 12345678#59;timeout=60

    C->>S: PLAY rtsp://host/live RTSP/1.0<br/>CSeq: 5<br/>Session: 12345678<br/>Range: npt=0.000-
    S-->>C: 200 OK<br/>Session: 12345678<br/>RTP-Info: url=rtsp://host/live/streamid=0#59;seq=1000#59;rtptime=90000

    S-->>C: RTP 视频 / 音频（UDP 或 TCP 交织）
    S-->>C: RTCP SR / RR（对时、丢包统计）

    C->>S: GET_PARAMETER / OPTIONS（保活，带 Session）
    S-->>C: 200 OK

    C->>S: TEARDOWN rtsp://host/live RTSP/1.0<br/>CSeq: n<br/>Session: 12345678
    S-->>C: 200 OK
```

#### 逐步说明

1. **OPTIONS**
   查询服务器能力。返回的 `Public` 头列出支持的方法。
   很多客户端会跳过这一步直接 `DESCRIBE`，服务器一般也能接受。

2. **DESCRIBE**
   请求 SDP。响应头里常见：

   - `Content-Type: application/sdp`
   - `Content-Base`: 后续 track URL 的基准
   - `Content-Length`: SDP 字节数

   SDP 的 `a=control` 决定 `SETUP` 的 URL。例如 `a=control:streamid=0`，则 `SETUP rtsp://host/live/streamid=0`。

3. **SETUP（每个 track 一次）**
   协商媒体承载方式。请求里的 `Transport` 告诉服务器「我准备在哪个端口收」：

   ```text
   Transport: RTP/AVP;unicast;client_port=50000-50001
   ```

   - `50000` 收 RTP，`50001` 收 RTCP。
   - 服务器在响应里补上自己的端口和 SSRC：

   ```text
   Transport: RTP/AVP;unicast;client_port=50000-50001;server_port=60000-60001;ssrc=1234ABCD
   ```

   - 响应还会带 `Session: 12345678;timeout=60`，后续所有请求都必须带 `Session`。

   如果走 TCP 交织：

   ```text
   Transport: RTP/AVP/TCP;unicast;interleaved=0-1
   ```

   含义是 RTP 走通道 0，RTCP 走通道 1，复用同一条 RTSP TCP 连接。

4. **PLAY**
   开始播放。`Range: npt=0.000-` 表示从 0 开始、一直播（直播常用）。
   响应里的 `RTP-Info` 给出起始序号和时间戳，客户端据此判断流是否连续。

5. **RTP/RTCP**
   PLAY 成功后，服务器立即开始发 RTP。RTCP 周期发送（通常几秒一次），用于：

   - SR 提供 NTP 时间与 RTP 时间戳的映射，用于音画同步和绝对时间。
   - RR 提供丢包率、抖动等质量反馈。

6. **保活**
   会话有超时（常见 60 秒）。客户端必须在超时前发 `OPTIONS` 或 `GET_PARAMETER`（带 `Session`）续命。否则服务器清理会话，后续请求返回 `454 Session Not Found`。

   > ffmpeg 内部会自动保活；**自己实现 RTSP 客户端时必须自己做**。

7. **TEARDOWN**
   结束会话，服务器释放端口和资源。异常断开时服务器也会在超时后清理。

### 2.3 推流（ANNOUNCE / RECORD）时序

推流时，**推流端是 RTSP 客户端**，服务器是接收方。

```mermaid
sequenceDiagram
    autonumber
    participant P as 推流端（RTSP Client）
    participant S as 媒体服务器（MediaMTX）

    P->>S: OPTIONS rtsp://host/live RTSP/1.0<br/>CSeq: 1
    S-->>P: 200 OK<br/>Public: ..., ANNOUNCE, RECORD, ...

    P->>S: ANNOUNCE rtsp://host/live RTSP/1.0<br/>CSeq: 2<br/>Content-Type: application/sdp<br/>(SDP body)
    S-->>P: 200 OK

    P->>S: SETUP rtsp://host/live/streamid=0 RTSP/1.0<br/>CSeq: 3<br/>Transport: RTP/AVP#59;unicast#59;mode=record#59;client_port=50000-50001
    S-->>P: 200 OK<br/>Transport: ...#59;server_port=60000-60001<br/>Session: 12345678#59;timeout=60

    P->>S: RECORD rtsp://host/live RTSP/1.0<br/>CSeq: 4<br/>Session: 12345678
    S-->>P: 200 OK

    P-->>S: RTP 媒体流
    P->>S: TEARDOWN rtsp://host/live RTSP/1.0<br/>CSeq: n<br/>Session: 12345678
    S-->>P: 200 OK
```

要点：

- `ANNOUNCE` 的 body 是推流端自己生成的 SDP，描述编码、分辨率、SPS/PPS、AAC config。
- `SETUP` 的 `Transport` 必须带 `mode=record`，表示「我要往你这里发」。
- 有些服务器要求 `RECORD`，有些收到 RTP 就开始转发；ffmpeg 的 RTSP muxer 会按需要完成 `ANNOUNCE → SETUP → RECORD`。
- 推流端也要处理 `401` 鉴权、`Session` 超时和 `TEARDOWN`。

### 2.4 SDP 里到底有什么

一个典型的 H.264 + AAC 拉流 SDP：

```text
v=0
o=- 0 0 IN IP4 127.0.0.1
s=MediaMTX
c=IN IP4 0.0.0.0
t=0 0
m=video 0 RTP/AVP 96
a=rtpmap:96 H264/90000
a=fmtp:96 packetization-mode=1;profile-level-id=42e01f;sprop-parameter-sets=Z0LAH9kAUAW7ARAAAAMAEAAAAwPI8UKZYA==,aM48gA==
a=control:streamid=0
m=audio 0 RTP/AVP 97
a=rtpmap:97 MPEG4-GENERIC/48000/2
a=fmtp:97 profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;config=1190
a=control:streamid=1
```

逐行理解：

| 字段 | 含义 |
| --- | --- |
| `m=video 0 RTP/AVP 96` | 视频 track，RTP 承载，负载类型 96 |
| `a=rtpmap:96 H264/90000` | PT 96 对应 H.264，RTP 时钟 90kHz |
| `packetization-mode=1` | H.264 允许 FU-A/STAP-A 分片 |
| `profile-level-id=42e01f` | Baseline Profile，Level 3.1 |
| `sprop-parameter-sets=...` | Base64 编码的 SPS、PPS，客户端解码必需 |
| `a=control:streamid=0` | 该 track 的 SETUP 相对 URL |
| `m=audio 0 RTP/AVP 97` | 音频 track，PT 97 |
| `a=rtpmap:97 MPEG4-GENERIC/48000/2` | AAC，48kHz，双声道 |
| `mode=AAC-hbr` | AAC 使用带 AU Header 的封装 |
| `config=1190` | AudioSpecificConfig 的十六进制表示 |
| `a=control:streamid=1` | 音频 track 的相对 URL |

> [!warning] 常见坑：SPS/PPS 缺失
> 有些设备只在 RTP 带内重复 SPS/PPS，SDP 里没有 `sprop-parameter-sets`。此时客户端必须能从带内拿到参数集才能解码。推流时如果 SDP 缺少参数集，部分播放器会黑屏，解决方式是开启编码器的 `repeat-headers`，或让服务器在带内转发。

H.265 的 SDP 类似，但参数集字段变成：

```text
a=rtpmap:96 H265/90000
a=fmtp:96 sprop-vps=...;sprop-sps=...;sprop-pps=...
```

### 2.5 Transport：UDP、TCP 交织、组播

| 方式 | 请求头示例 | 优点 | 缺点 | 适用 |
| --- | --- | --- | --- | --- |
| UDP 单播 | `RTP/AVP;unicast;client_port=50000-50001` | 延迟低、开销小 | 丢包、穿 NAT/防火墙差 | 局域网、可控网络 |
| TCP 交织 | `RTP/AVP/TCP;unicast;interleaved=0-1` | 穿 NAT/防火墙、无丢包 | 队头阻塞、延迟抖动 | 公网、复杂网络 |
| UDP 组播 | `RTP/AVP;multicast;destination=239.1.1.1;port=5000-5001;ttl=16` | 一份数据多人收 | 需要网络支持组播 | 局域网 IPTV |

TCP 交织的帧格式（复用 RTSP 的 TCP 连接）：

```text
$ <channel:1 字节> <length:2 字节，大端> <RTP 或 RTCP 包>
```

- `$` 是 ASCII 0x24。
- 偶数通道通常是 RTP，奇数通道通常是 RTCP。
- 因为 RTP 包被包在 TCP 里，所以不会丢包，但一旦网络抖动，TCP 重传会造成延迟尖峰。

ffmpeg 的 `-rtsp_transport` 取值：

| 值 | 含义 |
| --- | --- |
| `udp` | 默认，UDP 单播 |
| `tcp` | TCP 交织 |
| `udp_multicast` | UDP 组播（输入侧） |
| `http` / `https` | RTSP over HTTP 隧道（部分设备支持） |

### 2.6 Session、保活与鉴权

#### Session

`SETUP` 成功后服务器返回：

```text
Session: 12345678;timeout=60
```

- `12345678` 是会话 ID。
- `timeout=60` 表示 60 秒内没有请求就清理会话。
- 后续 `PLAY`、`PAUSE`、`TEARDOWN`、`GET_PARAMETER` 都必须带 `Session`。

保活方式：

```text
GET_PARAMETER rtsp://host/live RTSP/1.0
CSeq: 6
Session: 12345678
```

或：

```text
OPTIONS rtsp://host/live RTSP/1.0
CSeq: 6
Session: 12345678
```

建议在 `timeout / 2` 左右发一次。

#### 鉴权

第一次请求可能返回：

```text
RTSP/1.0 401 Unauthorized
CSeq: 2
WWW-Authenticate: Basic realm="MediaMTX"
```

客户端带凭据重试：

```text
DESCRIBE rtsp://host/live RTSP/1.0
CSeq: 3
Authorization: Basic dXNlcjpwYXNz
```

Digest 鉴权更常见：

```text
WWW-Authenticate: Digest realm="IP Camera", nonce="...", algorithm=MD5
Authorization: Digest username="admin", realm="IP Camera", nonce="...",
               uri="rtsp://host/live", response="...", algorithm=MD5
```

ffmpeg 支持 URL 内嵌凭据，也支持部分版本的 `-user/-password`：

```bash
ffplay rtsp://admin:12345@192.168.1.64:554/Streaming/Channels/101
```

### 2.7 状态机

拉流客户端：

```mermaid
stateDiagram-v2
    [*] --> Init
    Init --> Ready: OPTIONS / DESCRIBE
    Ready --> Ready: SETUP（每个 track 一次）
    Ready --> Playing: PLAY
    Playing --> Paused: PAUSE
    Paused --> Playing: PLAY
    Playing --> [*]: TEARDOWN / 超时 / 断网
```

推流客户端：

```mermaid
stateDiagram-v2
    [*] --> Init
    Init --> Announced: ANNOUNCE
    Announced --> Ready: SETUP（每个 track 一次）
    Ready --> Recording: RECORD
    Recording --> [*]: TEARDOWN / 超时 / 断网
```

### 2.8 常见状态码

| 状态码 | 含义 | 常见原因 |
| --- | --- | --- |
| `200 OK` | 成功 | — |
| `301/302` | 重定向 | 服务器让客户端去另一个地址 |
| `400 Bad Request` | 请求格式错 | 头部拼错、缺少必需头 |
| `401 Unauthorized` | 需要鉴权 | 用户名密码错、没带 Authorization |
| `403 Forbidden` | 禁止访问 | 权限不足、IP 限制 |
| `404 Not Found` | 路径不存在 | URL 写错、通道号不对 |
| `453 Not Enough Bandwidth` | 带宽不足 | 服务器限流 |
| `454 Session Not Found` | 会话不存在/已超时 | 保活失败、Session 写错 |
| `455 Method Not Valid in This State` | 状态不允许 | 没 SETUP 就 PLAY |
| `461 Unsupported Transport` | 传输方式不支持 | 服务器不支持 TCP 交织/组播 |
| `500 Internal Server Error` | 服务器内部错误 | 服务器实现问题 |
| `503 Service Unavailable` | 服务不可用 | 连接数满、正在启动 |

---

## 三、ffmpeg 命令行推流 / 拉流

### 3.1 先起一个 RTSP 服务器

生产/联调推荐 **MediaMTX**（原 rtsp-simple-server），单文件、跨平台、支持 RTSP/RTMP/HLS/WebRTC。

最小配置 `mediamtx.yml`：

```yaml
logLevel: info
rtspAddress: :8554
paths:
  live:
    source: publisher
```

启动：

```bash
mediamtx
```

也可以用 ffmpeg 自己当「一次性服务器」做快速验证：

```bash
ffmpeg -re -i sample.mp4 -c copy -f rtsp -rtsp_flags listen rtsp://0.0.0.0:8554/live
```

> [!note]
> `-rtsp_flags listen` 只适合单客户端快速验证，不适合生产。生产用 MediaMTX / ZLMediaKit。

### 3.2 推流

#### 转码推流（推荐，参数可控）

```bash
ffmpeg -re -stream_loop -1 -i sample.mp4 \
  -c:v libx264 -preset veryfast -tune zerolatency \
  -profile:v baseline -g 30 -keyint_min 30 -sc_threshold 0 -bf 0 \
  -b:v 2M -maxrate 2M -bufsize 4M \
  -c:a aac -b:a 128k -ar 48000 -ac 2 \
  -f rtsp -rtsp_transport tcp \
  rtsp://127.0.0.1:8554/live
```

关键参数：

| 参数 | 作用 |
| --- | --- |
| `-re` | 按实时速率读文件。**推流必加**，否则会瞬间灌满 |
| `-stream_loop -1` | 文件循环，方便长时间测试 |
| `-tune zerolatency` | 编码器低延迟模式 |
| `-g 30` | 30 帧一个 GOP（30fps 下约 1 秒） |
| `-keyint_min 30`、`-sc_threshold 0` | 固定关键帧间隔，避免场景切换打乱 GOP |
| `-bf 0` | 禁用 B 帧，降低延迟 |
| `-profile:v baseline` | 兼容性最好，且天然无 B 帧 |
| `-f rtsp` | 指定 RTSP muxer |
| `-rtsp_transport tcp` | RTP 走 TCP 交织，抗丢包/穿 NAT |

#### 不转码推流（最低延迟、最低 CPU）

```bash
ffmpeg -re -i sample.mp4 -c copy \
  -f rtsp -rtsp_transport tcp \
  rtsp://127.0.0.1:8554/live
```

适用条件：源已经是 H.264/H.265 + AAC，且封装参数能被 RTSP muxer 正确写入 SDP。

#### 低延迟补充参数

```bash
-muxdelay 0 -muxpreload 0
```

它们减少 muxer 的缓冲。对 RTSP 推流，通常加上 `-rtsp_transport tcp` 和 `-muxdelay 0` 就够。

### 3.3 拉流

#### 播放

```bash
ffplay -rtsp_transport tcp rtsp://127.0.0.1:8554/live
```

#### 保存为文件

```bash
ffmpeg -rtsp_transport tcp -i rtsp://127.0.0.1:8554/live \
  -map 0 -c copy -y out.mp4
```

#### 查看流信息

```bash
ffprobe -v error -rtsp_transport tcp \
  -show_entries stream=index,codec_name,width,height,r_frame_rate,avg_frame_rate \
  -of json rtsp://127.0.0.1:8554/live
```

#### 只看前 5 秒的包

```bash
ffprobe -v error -rtsp_transport tcp -select_streams v:0 \
  -show_packets -show_entries packet=pts_time,dts_time,flags,size \
  -read_intervals "%+5" -of csv \
  rtsp://127.0.0.1:8554/live
```

#### 低延迟播放

```bash
ffplay -rtsp_transport tcp \
  -fflags nobuffer -flags low_delay -framedrop \
  -probesize 500000 -analyzeduration 0 \
  rtsp://127.0.0.1:8554/live
```

| 参数 | 作用 |
| --- | --- |
| `-fflags nobuffer` | 输入侧不做额外缓冲 |
| `-flags low_delay` | 解码器低延迟 |
| `-framedrop` | 落后时丢帧而不是追帧 |
| `-probesize` / `-analyzeduration` | 降低首帧探测时间，太小可能识别不出流 |
| `-max_delay 500000` | 抖动缓冲上限（微秒） |

> [!warning] 低延迟不是参数越多越好
> `-probesize` 太小会导致 `find_stream_info` 失败或误判编码；`-fflags nobuffer` 在 UDP 丢包时更容易花屏。先保证稳定，再逐项压低延迟。

### 3.4 常用排查命令

```bash
# 打开 RTSP 详细日志，观察 OPTIONS/DESCRIBE/SETUP/PLAY 与状态码
ffmpeg -loglevel debug -rtsp_transport tcp -i rtsp://127.0.0.1:8554/live -t 2 -f null -

# 检查 UDP 端口是否可达（Windows）
netstat -ano | findstr 8554

# 抓包（Wireshark 过滤器）
rtsp || rtp || rtcp
```

---

## 四、C++ 中使用 ffmpeg 推流 / 拉流与联调

### 4.1 总体架构

```mermaid
flowchart LR
    subgraph 拉流端
        N1[网络读线程<br/>av_read_frame] --> Q1[视频包队列]
        N1 --> Q2[音频包队列]
        Q1 --> D1[视频解码线程]
        Q2 --> D2[音频解码线程]
        D1 --> F1[视频帧队列]
        D2 --> F2[音频帧队列]
        F1 --> R[渲染 / 同步线程]
        F2 --> R
    end
    subgraph 推流端
        S[采集 / 生成帧] --> E[编码器]
        E --> W[av_interleaved_write_frame]
        W --> NET[RTSP 网络输出]
    end
```

核心库：

| 库 | 作用 |
| --- | --- |
| `libavformat` | RTSP 打开/读写、封装/解封装、SDP、RTP |
| `libavcodec` | 编解码 |
| `libavutil` | 帧、包、时间基、错误处理 |
| `libswscale` | 像素格式转换（YUV ↔ RGB） |
| `libswresample` | 音频重采样（可选） |

编译时链接：`avformat avcodec avutil swscale swresample`。Windows 下还需保证 FFmpeg 的 DLL 在 `PATH` 中。

### 4.2 拉流：最小可用骨架

```cpp
// rtsp_pull.cpp
#include <atomic>
#include <functional>
#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
}

namespace {
// 错误码转为字符串
std::string av_err2str(int err) {
    char buf[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(err, buf, sizeof(buf));
    return buf;
}
// ffmpeg内部会周期性的调用中断回调函数interrupt_cb，检查是否需要中断
// 中断上下文，使用原子变量，线程安全，用于中断拉流
struct InterruptState {
    std::atomic<bool> abort{false};
};
// 中断回调，1需要中断，0不中断
int interrupt_cb(void* opaque) {
    auto* s = static_cast<InterruptState*>(opaque);
    return s->abort.load(std::memory_order_relaxed) ? 1 : 0;
}

} // namespace
// 拉流器
class RtspPuller {
public:
    ~RtspPuller() { close(); }
	// 打开流
	// 1，设置各种选项
	// 2. 预分配上下文内存，设置中断回调和中断上下文，打开输入流获取格式上下文
	// 3. 查找视频流信息并获取视频流
	// 4.根据视频的解码器创建对应的解码器并设置解码器参数，打开解码器
	// 5.预分配packet、frame的内存
    bool open(const std::string& url) {
        avformat_network_init(); // 初始化网络模块，调用一次即可

		//添加选项
        AVDictionary* opts = nullptr;
        // 媒体走 TCP 交织：抗丢包、穿 NAT；局域网追求低延迟可改 "udp"
        av_dict_set(&opts, "rtsp_transport", "tcp", 0);
        // socket I/O 超时，单位微秒；新版若提示废弃，改用 timeout
        av_dict_set(&opts, "stimeout", "5000000", 0);
        // 抖动缓冲上限，单位微秒
        av_dict_set(&opts, "max_delay", "500000", 0);
        // 输入侧不额外缓存
        av_dict_set(&opts, "fflags", "nobuffer", 0);
        // 降低首帧探测耗时（太小可能识别不出流）
        av_dict_set(&opts, "probesize", "1000000", 0);
        av_dict_set(&opts, "analyzeduration", "1000000", 0);

		// 预分配整个对话上下文内存，注册中断上下文、中断回调
        fmt_ = avformat_alloc_context();
        if (!fmt_) return false;
        fmt_->interrupt_callback.callback = interrupt_cb;
        fmt_->interrupt_callback.opaque = &irq_;

		// 打开URL输入流获取上下文信息
        int ret = avformat_open_input(&fmt_, url.c_str(), nullptr, &opts);
        av_dict_free(&opts);   // 注意：opts 里未被消费的项会保留，便于排查
        if (ret < 0) {
            fprintf(stderr, "avformat_open_input: %s\n", av_err2str(ret).c_str());
            return false;
        }

		// 查找流信息
        ret = avformat_find_stream_info(fmt_, nullptr);
        if (ret < 0) {
            fprintf(stderr, "find_stream_info: %s\n", av_err2str(ret).c_str());
            return false;
        }
		// 查找到最佳的视频流下标并获取视频流
        video_index_ = av_find_best_stream(fmt_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (video_index_ < 0) {
            fprintf(stderr, "no video stream\n");
            return false;
        }
		// 获取到视频流指针
        AVStream* st = fmt_->streams[video_index_];
        // 查找到对应的解码器类型
        const AVCodec* codec = avcodec_find_decoder(st->codecpar->codec_id);
        if (!codec) return false;
		// 创建解码器
        dec_ = avcodec_alloc_context3(codec);
        if (!dec_) return false;
        avcodec_parameters_to_context(dec_, st->codecpar);// 设置解码器参数
        dec_->flags |= AV_CODEC_FLAG_LOW_DELAY;
        dec_->thread_count = 1;   // 低延迟优先；吞吐优先可设 0 让 ffmpeg 自动
		// 打开解码器
        ret = avcodec_open2(dec_, codec, nullptr);
        if (ret < 0) {
            fprintf(stderr, "avcodec_open2: %s\n", av_err2str(ret).c_str());
            return false;
        }
		// 预分配packet、frame空间
        pkt_ = av_packet_alloc();
        frame_ = av_frame_alloc();
        return pkt_ && frame_;
    }
	// 拉流循环
	// 只要未中断（中断上下文abort为false）
    // 返回 false 表示断流/出错，调用方选择重连策略
    bool read_loop(const std::function<void(AVFrame*)>& on_frame) {
        while (!irq_.abort.load()) { // 没有中断，循环执行
            int ret = av_read_frame(fmt_, pkt_);/// 解码并读取一帧
            if (ret == AVERROR_EOF) return false; // 内部彻底没有数据（缓冲区无数据，且数据已经冲刷完成）
            if (ret < 0) {
                fprintf(stderr, "av_read_frame: %s\n", av_err2str(ret).c_str());
                return false;
            }
			// 必须检查是否为视频流
            if (pkt_->stream_index == video_index_) {
                // 生产代码应处理 send 返回 EAGAIN 的情况：
                // 先 receive 清空，再重发同一个 packet
                ret = avcodec_send_packet(dec_, pkt_); // 发送解码
                if (ret < 0 && ret != AVERROR(EAGAIN)) { // 如果发送缓冲区没有满(EAGAIN)，但是出现错误
                    fprintf(stderr, "send_packet: %s\n", av_err2str(ret).c_str());
                }
                // 解码并获取帧
                while ((ret = avcodec_receive_frame(dec_, frame_)) == 0) {
                    on_frame(frame_);
                    av_frame_unref(frame_); // ！每次使用完帧都要进行unref取消绑定，防止悬空引用计数导致资源永远没有释放
                }
            }
            av_packet_unref(pkt_); // 同上，需要解除引用计数（-1）
        }
        return true;
    }
	// 设置中断
    void stop() { irq_.abort = true; }
	// 关闭并回收上下文
    void close() {
        if (pkt_) av_packet_free(&pkt_);
        if (frame_) av_frame_free(&frame_);
        if (dec_) avcodec_free_context(&dec_);
        if (fmt_) avformat_close_input(&fmt_);
        avformat_network_deinit();
    }

    AVStream* video_stream() const {
        return fmt_ ? fmt_->streams[video_index_] : nullptr;
    }

private:
    AVFormatContext* fmt_ = nullptr;
    AVCodecContext* dec_ = nullptr;
    AVPacket* pkt_ = nullptr;
    AVFrame* frame_ = nullptr;
    int video_index_ = -1;
    InterruptState irq_;
};
```

使用示例：帧回调里做渲染或转发。

```cpp
RtspPuller puller;
if (puller.open("rtsp://127.0.0.1:8554/live")) {
    puller.read_loop([&](AVFrame* f) {
        // f->pts 的时间基是 puller.video_stream()->time_base
        double pts_sec = f->pts * av_q2d(puller.video_stream()->time_base);
        // 这里做 sws_scale → RGBA → 渲染/编码
        (void)pts_sec;
    });
}
```

#### 拉流代码里的关键点

1. **超时**
   `stimeout` 管 socket I/O；`interrupt_callback` 用于主动停止。网络线程阻塞在 `av_read_frame` 时，单靠 `abort` 标志不会立即返回，必须靠超时或 interrupt 回调。

2. **首帧慢**
   RTSP 客户端要等一个 IDR 才能出画面。缩短编码端 GOP、服务器开启 GOP cache、降低 `probesize` 都有帮助。

3. **时间基**
   RTSP 视频的时间基通常是 `1/90000`，音频通常是 `1/采样率`。`frame->pts * av_q2d(stream->time_base)` 才是秒。

4. **EAGAIN**
   `avcodec_send_packet` 返回 `EAGAIN` 时，**表示发送缓冲区满**，必须先 `receive_frame`接收所有解码后的帧 直到 `EAGAIN`**读取缓冲区空**，解码器才会重新启动，再**重发同一个 packet**；上面的示例为了简洁只做了基本处理。

5. **断流判断**
   `av_read_frame` 返回负数、连续超时、或长时间没有关键帧，都应触发重连。

### 4.3 推流：编码 + RTSP 输出

```cpp
// rtsp_push.cpp
#include <string>
#include <algorithm>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
}

class RtspPusher {
public:
    ~RtspPusher() { close(); }
	// 打开推流
	//
    bool open(const std::string& url, int width, int height, int fps, int bitrate) {
        avformat_network_init();
		// 分配推流上下文
        int ret = avformat_alloc_output_context2(&fmt_, nullptr, "rtsp", url.c_str());
        if (ret < 0 || !fmt_) return false;
		// 配置编码器
        const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if (!codec) return false;

        enc_ = avcodec_alloc_context3(codec);
        if (!enc_) return false;
		// 设置编码器信息
        enc_->width = width;
        enc_->height = height;
        enc_->pix_fmt = AV_PIX_FMT_YUV420P;
        enc_->time_base = {1, fps};
        enc_->framerate = {fps, 1};
        enc_->bit_rate = bitrate;
        enc_->gop_size = fps;          // 约 1 秒一个 I 帧：首帧快、码率可控
        enc_->max_b_frames = 0;        // 低延迟：禁用 B 帧
        // RTSP muxer 需要全局头，SPS/PPS 会写入 SDP
        enc_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        av_opt_set(enc_->priv_data, "preset", "veryfast", 0);
        av_opt_set(enc_->priv_data, "tune", "zerolatency", 0);
		// 打开编码器
        ret = avcodec_open2(enc_, codec, nullptr);
        if (ret < 0) {
            fprintf(stderr, "avcodec_open2: %s\n", av_err2str(ret).c_str());
            return false;
        }
		// 创建新流用于推流
        st_ = avformat_new_stream(fmt_, nullptr);
        if (!st_) return false;
        // 设置配置信息
        st_->time_base = enc_->time_base;
        avcodec_parameters_from_context(st_->codecpar, enc_);
        st_->codecpar->codec_tag = 0;
		// 设置选项
        AVDictionary* opts = nullptr;
        av_dict_set(&opts, "rtsp_transport", "tcp", 0);
        av_dict_set(&opts, "muxdelay", "0", 0);   // 不额外缓冲
        // 多流时不等待交织，降低延迟；单流影响不大
        fmt_->max_interleave_delta = 0;

        // 内部依次完成 ANNOUNCE → SETUP → RECORD
        ret = avformat_write_header(fmt_, &opts);
        av_dict_free(&opts); // 使用完释放引用
        if (ret < 0) {
            fprintf(stderr, "write_header: %s\n", av_err2str(ret).c_str());
            return false;
        }

        pkt_ = av_packet_alloc();
        return pkt_ != nullptr;
    }

    // frame 的 format/size 必须与编码器配置一致，pts 由内部递增
    bool push(AVFrame* frame) {
        frame->pts = next_pts_++;
		// 发送帧
        int ret = avcodec_send_frame(enc_, frame);
        if (ret < 0 && ret != AVERROR(EAGAIN)) {
            fprintf(stderr, "send_frame: %s\n", av_err2str(ret).c_str());
            return false;
        }
		// 推流循环
        while ((ret = avcodec_receive_packet(enc_, pkt_)) == 0) {
            av_packet_rescale_ts(pkt_, enc_->time_base, st_->time_base);
            pkt_->stream_index = st_->index;
            ret = av_interleaved_write_frame(fmt_, pkt_);
            av_packet_unref(pkt_);
            if (ret < 0) {
                fprintf(stderr, "write_frame: %s\n", av_err2str(ret).c_str());
                return false;
            }
        }
        return true;
    }
	/// 关闭并清理资源
    void close() {
        if (enc_) {
            // 冲刷编码器
            avcodec_send_frame(enc_, nullptr);
            while (pkt_ && avcodec_receive_packet(enc_, pkt_) == 0) {
                av_packet_rescale_ts(pkt_, enc_->time_base, st_->time_base);
                pkt_->stream_index = st_->index;
                av_interleaved_write_frame(fmt_, pkt_);
                av_packet_unref(pkt_);
            }
        }
        if (fmt_) {
            av_write_trailer(fmt_);   // 发送 TEARDOWN
            avformat_free_context(fmt_);
            fmt_ = nullptr;
        }
        if (pkt_) av_packet_free(&pkt_);
        if (enc_) avcodec_free_context(&enc_);
        avformat_network_deinit();
    }

private:
    static std::string av_err2str(int err) {
        char buf[AV_ERROR_MAX_STRING_SIZE] = {};
        av_strerror(err, buf, sizeof(buf));
        return buf;
    }

    AVFormatContext* fmt_ = nullptr;
    AVCodecContext* enc_ = nullptr;
    AVStream* st_ = nullptr;
    AVPacket* pkt_ = nullptr;
    int64_t next_pts_ = 0;
};
```

#### 推流代码里的关键点

1. **不要 `avio_open`**
   RTSP muxer 带 `AVFMT_NOFILE`，由 `avformat_write_header` 自己建立 RTSP 连接。对 `rtsp://` 调 `avio_open` 会失败或行为异常。

2. **全局头**
   编码器打开前设置 `AV_CODEC_FLAG_GLOBAL_HEADER`，让 SPS/PPS 进入 `extradata`，RTSP muxer 才能写进 SDP 的 `sprop-parameter-sets`。

3. **时间基换算**
   `av_packet_rescale_ts(pkt, enc_->time_base, st_->time_base)` 不能省。否则服务器看到的时间戳会错乱，表现为快放、慢放或直接丢弃。

4. **实时节流**
   文件源推流要按帧率睡眠，模拟 `-re`：

   ```cpp
   int64_t start = av_gettime_relative();
   while (...) {
       int64_t due = av_rescale_q(frame->pts, enc_->time_base, AV_TIME_BASE_Q);
       int64_t now = av_gettime_relative() - start;
       if (due > now) av_usleep(due - now);
       // encode + write
   }
   ```

5. **`max_interleave_delta = 0`**
   多流推流时，`av_interleaved_write_frame` 默认会等待交织，可能引入延迟；设为 0 可关闭这段缓冲。

6. **断线重连**
   `av_interleaved_write_frame` 返回负数通常意味着连接已断。不要继续灌数据，应关闭并重连。

### 4.4 转发（拉流 → 推流，不转码）

联调里最常见的形态：从相机拉流，原样推到媒体服务器。

```cpp
bool relay(const std::string& in_url, const std::string& out_url) {
    AVFormatContext* in = nullptr;
    AVDictionary* iopts = nullptr;
    av_dict_set(&iopts, "rtsp_transport", "tcp", 0);
    av_dict_set(&iopts, "stimeout", "5000000", 0);

	// 获取输入和输入流的上下文
    if (avformat_open_input(&in, in_url.c_str(), nullptr, &iopts) < 0) return false;
    av_dict_free(&iopts);
    if (avformat_find_stream_info(in, nullptr) < 0) return false;

    AVFormatContext* out = nullptr;
    if (avformat_alloc_output_context2(&out, nullptr, "rtsp", out_url.c_str()) < 0) return false;

    // 输入流索引 → 输出流索引
    std::vector<int> map(in->nb_streams, -1);
    for (unsigned i = 0; i < in->nb_streams; ++i) {
        AVMediaType type = in->streams[i]->codecpar->codec_type;
        if (type != AVMEDIA_TYPE_VIDEO && type != AVMEDIA_TYPE_AUDIO) continue;

        AVStream* os = avformat_new_stream(out, nullptr);
        avcodec_parameters_copy(os->codecpar, in->streams[i]->codecpar);
        os->codecpar->codec_tag = 0;
        os->time_base = in->streams[i]->time_base;
        map[i] = os->index;
    }

    AVDictionary* oopts = nullptr;
    av_dict_set(&oopts, "rtsp_transport", "tcp", 0);
    av_dict_set(&oopts, "muxdelay", "0", 0);
    out->max_interleave_delta = 0;

    if (avformat_write_header(out, &oopts) < 0) return false;
    av_dict_free(&oopts);

	// 开始转发拉流
    AVPacket* pkt = av_packet_alloc();
    while (av_read_frame(in, pkt) >= 0) {
        int out_index = map[pkt->stream_index];
        if (out_index < 0) {
            av_packet_unref(pkt);
            continue;
        }
        AVStream* is = in->streams[pkt->stream_index];
        AVStream* os = out->streams[out_index];
        av_packet_rescale_ts(pkt, is->time_base, os->time_base);
        pkt->stream_index = out_index;
        // av_interleaved_write_frame会进行内部的DTS排序后重新写入
        // 保证音视频包“交错”排列
        av_interleaved_write_frame(out, pkt);
        av_packet_unref(pkt);
    }

    av_packet_free(&pkt);
    av_write_trailer(out);
    avformat_free_context(out);
    avformat_close_input(&in);
    return true;
}
```

注意：

- 不转码时，输入 SDP 里的 SPS/PPS 会进入 `codecpar->extradata`，输出 muxer 再写进新 SDP；大多数情况可正常工作。
- 如果输出 SDP 缺少参数集，检查输入是否只有带内参数集，必要时开启服务器的带内转发或在推流端加 `dump_extra` 类处理。
- 转发仍然要处理断流、重连和队列，不能直接在网络回调里做耗时操作。

### 4.5 线程模型与队列

不要把所有事情塞进一个线程：

```mermaid
flowchart LR
    NET[网络读线程] -->|AVPacket| VQ[视频包队列]
    NET -->|AVPacket| AQ[音频包队列]
    VQ --> VD[视频解码线程] --> VF[视频帧队列]
    AQ --> AD[音频解码线程] --> AF[音频帧队列]
    VF --> RENDER[渲染 / 编码线程]
    AF --> RENDER
```

设计要点：

| 问题   | 建议                                        |
| ---- | ----------------------------------------- |
| 队列长度 | 有界队列，视频包 30~60 个、音频包 50~100 个、帧队列 3~8 帧   |
| 队列满  | 视频丢非关键帧，音频不丢；**优先保证音频连续**                 |
| 解码阻塞 | 解码线程只做解码，不做渲染/网络 IO                       |
| 时钟   | **以音频为主时钟**，视频按 PTS 对齐；落后超过阈值就丢帧          |
| 重连   | 独立的重连线程/状态机，指数退避；重连后清空队列并等 IDR            |
| 退出   | `abort` 标志 + interrupt callback + 超时，三者配合 |

音画同步的简化逻辑：

```text
音频时钟 = 最近一个音频帧的 pts + 已播放时长
视频帧计划显示时间 = 视频 pts
差值 < -40ms  → 视频落后，丢帧
差值 > +10ms  → 视频超前，睡眠等待
否则正常渲染
```

RTSP 的 RTP 时间戳本身来自采集端，RTCP SR 提供与 NTP 的映射；ffmpeg 已经把这些换算成 `AVPacket` 的 pts/dts，应用层只需按时间基对齐。

### 4.6 联调流程：从「能通」到「稳定」

推荐分四个阶段，逐层隔离问题。

| 阶段 | 推流端 | 拉流端 | 验证目标 | 通过标准 |
| --- | --- | --- | --- | --- |
| **P0 基线** | ffmpeg CLI | ffplay / ffprobe | 服务器、网络、账号、编码参数正确 | 能稳定播放 5 分钟，无 401/404/454 |
| **P1 隔离推流** | **你的 C++ 推流** | ffplay / ffprobe | 推流端 SDP、时间戳、关键帧正确 | ffprobe 能看到正确分辨率/帧率/编码；播放无花屏、无快慢放 |
| **P2 隔离拉流** | ffmpeg CLI | **你的 C++ 拉流** | 拉流端握手、解码、同步正确 | 首帧 < 1s（局域网）、连续播放、无累积延迟 |
| **P3 端到端** | **你的 C++ 推流** | **你的 C++ 拉流** | 两端协同、重连、弱网、长时间稳定 | 2 小时无崩溃/无内存增长/重连可恢复 |

#### P0：先把服务器跑通

1. 启动 MediaMTX，确认 8554 端口监听。
2. 用 CLI 推流，另开窗口用 `ffprobe` 和 `ffplay` 拉流。
3. 记录基线指标：分辨率、帧率、码率、首帧时间、延迟。

```bash
# 终端 1：推流
ffmpeg -re -stream_loop -1 -i sample.mp4 -c:v libx264 -tune zerolatency \
  -g 30 -bf 0 -c:a aac -f rtsp -rtsp_transport tcp rtsp://127.0.0.1:8554/live

# 终端 2：探流
ffprobe -v error -rtsp_transport tcp -show_streams rtsp://127.0.0.1:8554/live

# 终端 3：播放
ffplay -rtsp_transport tcp rtsp://127.0.0.1:8554/live
```

#### P1：只测 C++ 推流

1. 让 C++ 推流端向 MediaMTX 推流。
2. 用 `ffprobe` 检查：

   - 是否有视频/音频流；
   - 编码、分辨率、帧率是否符合预期；
   - 包间隔是否均匀，`pts_time` 是否单调；
   - 关键帧间隔是否接近设定值。

3. 用 `ffplay` 看画面，重点观察：

   - 首帧是否出现；
   - 是否有花屏/绿屏；
   - 是否快放/慢放；
   - 是否几十秒后断开（Session 超时/保活问题）。

#### P2：只测 C++ 拉流

1. 保持 CLI 推流作为「已知良好源」。
2. C++ 拉流端连接，记录：

   - `avformat_open_input` 耗时；
   - `avformat_find_stream_info` 耗时；
   - 首个 `AVPacket`、首个关键帧、首个解码帧的时间；
   - 每秒解码帧数、丢帧数、队列深度。

3. 与 ffplay 对比，确认不是「源的问题」。

#### P3：端到端

1. C++ 推 → C++ 拉，先在同一台机器，再跨机器。
2. 加入异常场景：

   - 拔网线/断 Wi-Fi 10 秒，恢复后能否自动重连；
   - 服务器重启；
   - 弱网：Windows 用 `clumsy` 注入丢包/延迟，Linux 用 `tc netem`；
   - 长时间运行 2~24 小时，观察内存、句柄、重连次数、时间戳漂移。

3. 延迟测量：

   - 画面内嵌毫秒时间码 + 秒表，另一台设备拍照，算 glass-to-glass；
   - 或推流端叠加单调递增帧号，拉流端记录收到时间，估算端到端延迟；
   - 分别记录「首帧延迟」和「稳态延迟」，两者优化手段不同。

### 4.7 观测点与工具

#### 日志

```cpp
av_log_set_level(AV_LOG_DEBUG);   // 看 RTSP 报文级交互
// 或 AV_LOG_VERBOSE 看关键流程，避免日志过多
```

建议在代码里打这些时间点：

| 时间点 | 用途 |
| --- | --- |
| `open` 开始/结束 | 判断握手耗时、是否卡在鉴权/DESCRIBE |
| `find_stream_info` 结束 | 判断 SDP 是否完整、探测是否过慢 |
| 首个 packet | 判断媒体是否开始到达 |
| 首个关键帧 | 判断首帧延迟 |
| 首个解码帧 | 判断解码器是否正常 |
| 每次重连 | 统计稳定性 |
| 每 5 秒统计 | fps、码率、队列深度、丢帧、pts 间隔 |

#### 抓包

Wireshark 过滤器：

```text
rtsp
rtsp.method == "SETUP"
rtp
rtcp
tcp.port == 554
```

重点看：

- `DESCRIBE` 响应的 SDP 是否包含正确的 `rtpmap`、`fmtp`、`sprop-parameter-sets`；
- `SETUP` 响应是否是 `200 OK`，`Transport` 是否符合预期；
- `PLAY` 响应是否带 `RTP-Info`；
- 是否出现 `401`、`454`、`461`；
- TCP 交织时，是否能识别 `$` 开头的 RTP 帧（可用 Decode As → RTP）。

#### 服务器侧

- MediaMTX 日志会显示连接、path、reader/writer 数量、错误。
- 检查服务器是否开启了 GOP cache；没有的话，新客户端要等下一个 IDR。
- 检查服务器并发/带宽限制。

### 4.8 故障矩阵

| 现象 | 优先怀疑 | 排查动作 |
| --- | --- | --- |
| 完全黑屏、无数据 | 握手失败、鉴权失败、路径错 | 看 `401/404/454`；`ffprobe` 同 URL；Wireshark 看 DESCRIBE/SETUP |
| 有音频无视频 | SDP 视频 track 缺失、编码不支持 | 看 SDP `m=video`；确认解码器存在 |
| 花屏、马赛克 | UDP 丢包、MTU 过大、SPS/PPS 缺失 | 切 `-rtsp_transport tcp`；看 RTCP RR 丢包率；确认参数集 |
| 首帧很慢 | GOP 太长、服务器无 GOP cache、探测慢 | 缩短 GOP；开 GOP cache；调 `probesize/analyzeduration` |
| 延迟越来越大 | 队列积压、渲染慢、B 帧、缓冲过大 | 看队列深度；丢非关键帧；`zerolatency`、`bf=0`；`nobuffer` |
| 快放/慢放 | 时间基换算错误、pts 未 rescale | 检查 `av_packet_rescale_ts`、`stream->time_base` |
| 播几十秒后断开 | Session 超时、保活失败 | 看 `454`；客户端定时 `GET_PARAMETER/OPTIONS` |
| 卡住不返回 | 网络阻塞、无超时 | 设 `stimeout`、interrupt callback；加看门狗 |
| 重连风暴 | 无退避、服务器拒绝 | 指数退避；检查服务器限流/鉴权 |
| 内存持续增长 | 包/帧未释放、队列无界 | 检查 `av_packet_unref`、`av_frame_unref`；队列设上限 |
| 推流成功但别人拉不到 | 服务器 path 配置、SDP 参数集缺失 | 看服务器日志；`ffprobe` 服务器 URL；检查 SDP |

### 4.9 资源释放与重连

释放顺序（拉流）：

```cpp
av_packet_free(&pkt);
av_frame_free(&frame);
avcodec_free_context(&dec);
avformat_close_input(&fmt);   // 内部释放 fmt 并置空
avformat_network_deinit();    // 进程退出时调用一次即可
```

释放顺序（推流）：

```cpp
avcodec_send_frame(enc, nullptr);          // 冲刷
while (avcodec_receive_packet(enc, pkt) == 0) { write; unref; }
av_write_trailer(fmt);                     // TEARDOWN
avcodec_free_context(&enc);
avformat_free_context(fmt);
av_packet_free(&pkt);
```

重连模板：

```cpp
void run(const std::string& url) {
    int backoff_ms = 500; // 设置退避时间，防止频繁重连
    while (running_) {
        RtspPuller p;
        if (p.open(url)) {
            backoff_ms = 500;
            bool ok = p.read_loop(on_frame);
            if (ok) backoff_ms = 500;
        }
        p.close();

        // 重连前清空队列、重置解码器状态、等待新的 IDR
        std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
        backoff_ms = std::min(backoff_ms * 2, 10000);
    }
}
```

> [!important] 重连后的三个动作
> 1. 清空旧队列，避免播放过期数据。
> 2. 丢弃非关键帧，直到新的 IDR 到达。
> 3. 重新对齐音频主时钟，避免时间戳跳变导致画面卡住。
>
> ==重连的重要机制“退避”：==
>两次重连需要有一定的时间间隔（一般越来越大），防止频繁重连导致服务器雪崩

>

---

## 五、常见坑清单

1. **把 RTSP 当成传输协议**
   RTSP 只控制，媒体在 RTP。抓包时看不到「RTSP 视频包」是正常的。

2. **UDP 丢包导致花屏**
   公网/Wi-Fi 优先 `-rtsp_transport tcp`。TCP 会增加延迟，但稳定性好得多。

3. **忘记 `-re`**
   文件推流会瞬间发完，服务器缓冲暴涨，播放端表现异常。

4. **SPS/PPS 只在带内**
   某些播放器依赖 SDP 的 `sprop-parameter-sets`。推流端开启 `repeat-headers`，或确保服务器转发带内参数集。

5. **B 帧与长 GOP**
   B 帧增加编码延迟和 dts/pts 复杂度；长 GOP 让新客户端等首帧更久。低延迟场景用 `-bf 0`、1 秒 GOP。

6. **时间基没有 rescale**
   C++ 推流最常见错误：`pts` 直接用编码器时间基写出去，服务器看到的时间戳错乱。

7. **Session 超时**
   自研客户端 60 秒不发保活就会收到 `454`。按 `timeout/2` 发 `GET_PARAMETER` 或 `OPTIONS`。

8. **`avio_open` 打开 RTSP**
   RTSP muxer 是 `AVFMT_NOFILE`，不需要也不能这样打开。

9. **在解码/网络线程里做重活**
   渲染、编码、磁盘 IO 放到独立线程，否则网络读会被拖慢，延迟累积。

10. **队列无界**
    弱网时包越积越多，表现为延迟无限增长。必须有界 + 丢弃策略。

11. **忽略 `EAGAIN`**
    `send_packet`/`send_frame`/`receive_frame` 的 `EAGAIN` 是正常流控，不是错误。

12. **没有超时和中断**
    `av_read_frame` 可能永久阻塞。生产代码必须有 `stimeout`、interrupt callback 和看门狗。

---

## 六、自测清单

- [ ] 能用 `ffprobe` 看到 RTSP 流的编码、分辨率、帧率。
- [ ] 能说清 OPTIONS / DESCRIBE / SETUP / PLAY / TEARDOWN 各自作用。
- [ ] 能看懂 SDP 里的 `rtpmap`、`fmtp`、`sprop-parameter-sets`、`control`。
- [ ] 知道 UDP 与 TCP 交织的取舍。
- [ ] 能用 ffmpeg CLI 完成推流、拉流、保存、低延迟播放。
- [ ] C++ 拉流能正确处理超时、断流、重连。
- [ ] C++ 推流能正确设置全局头、时间基、关键帧间隔。
- [ ] 能解释 `454 Session Not Found` 的原因和修复方式。
- [ ] 能用 Wireshark 看到 RTSP 交互和 RTP 包。
- [ ] 做过至少 2 小时的端到端稳定性测试。

---

## 相关

- [[音视频/_MOC|音视频 ffmpeg 知识地图]]
- [[时间轴与同步|时间轴与同步]]
- [[编码原理与帧类型|编码原理与帧类型]]
- [[命令实操|命令实操]]
- [[易错点与排错|易错点与排错]]
- [RFC 2326: RTSP 1.0](https://www.rfc-editor.org/rfc/rfc2326)
- [RFC 7826: RTSP 2.0](https://www.rfc-editor.org/rfc/rfc7826)
- [RFC 6184: RTP Payload Format for H.264](https://www.rfc-editor.org/rfc/rfc6184)
- [RFC 7798: RTP Payload Format for H.265](https://www.rfc-editor.org/rfc/rfc7798)
- [RFC 3640: RTP Payload Format for AAC](https://www.rfc-editor.org/rfc/rfc3640)
- [FFmpeg Protocols Documentation](https://ffmpeg.org/ffmpeg-protocols.html)
- [MediaMTX](https://github.com/bluenviron/mediamtx)
- [ZLMediaKit](https://github.com/ZLMediaKit/ZLMediaKit)