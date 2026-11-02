---
title: 初步认识 ffmpeg 库：开发 API 与取流/推流
type: 主题笔记
topic: 音视频
status: 学习中
created: 2026-10-29
updated: 2026-10-29
tags:
  - ffmpeg
  - 音视频
  - 开发
  - api
  - cpp
series: 音视频 ffmpeg
canvas: 白板/开发相关.canvas
---

# 初步认识 ffmpeg 库：开发 API 与取流/推流

> **概述**：本篇从「命令行工具」切换到「库」的视角，以 C API 为主线，讲清 `AVFormatContext`、`AVCodecContext`、
> `AVPacket`、`AVFrame` 的职责，解码/编码/封装/推流的完整流程，内存管理与引用计数，以及取流/推流的工程化细节。
>
> **版本**：示例基于 **FFmpeg 7.1**（libavformat 61 / libavcodec 61 / libavutil 59）。旧教程里的很多 API 已被删除，文末有新旧对照表。
>
> **前置**：[[基础概念#三、处理管线]] · [[容器与封装]] · [[时间轴与同步]] · [[命令实操]]
>
> **约定**：示例是「能独立阅读的骨架」，为了突出主流程省略了部分业务错误处理；生产代码必须检查每一个返回值。

> [!summary]- 目录
> - [[#一、先建立全局图：libav* 的职责与数据流]]
> - [[#二、核心对象与生命周期]]
> - [[#三、解封装：打开输入与取流]]
> - [[#四、解码：从 AVPacket 到 AVFrame]]
> - [[#五、编码：从 AVFrame 到 AVPacket]]
> - [[#六、封装与推流：从 AVPacket 到网络]]
> - [[#七、取流/推流专题（重点）]]
> - [[#八、转码、转封装与自定义 IO]]
> - [[#九、内存管理与引用计数（重点）]]
> - [[#十、API 速查表]]
> - [[#十一、常见坑与注意事项]]
> - [[#十二、版本迁移：旧教程 API 对照]]
> - [[#十三、一页纸总结]]
> - [[#相关笔记]]

---

## 一、先建立全局图：libav* 的职责与数据流

### 1.1 各个库分别做什么

| 库               | 职责                                    | 核心对象                                            |
| --------------- | ------------------------------------- | ----------------------------------------------- |
| `libavformat`   | 容器与协议：解封装、封装、AVIO 读写                  | `AVFormatContext`、`AVStream`、`AVIOContext`      |
| `libavcodec`    | 编解码                                   | `AVCodecContext`、`AVCodec`、`AVPacket`、`AVFrame` |
| `libavutil`     | 基础设施：内存、引用计数、时间基、错误码、声道布局             | `AVBufferRef`、`AVDictionary`、`AVRational`       |
| `libswscale`    | 视频缩放与像素格式转换                           | `SwsContext`                                    |
| `libswresample` | 音频重采样、声道转换、采样格式转换                     | `SwrContext`                                    |
| `libavfilter`   | 滤镜图（scale、format、aresample、overlay 等） | `AVFilterGraph`                                 |
| `libavdevice`   | 采集/输出设备（摄像头、屏幕、声卡）                    | 输入/输出设备格式                                       |

一句话区分：

- **`libavformat` 管「盒子」**：MP4、FLV、TS、RTSP、RTMP 都是它的事。
- **`libavcodec` 管「压缩规则」**：H.264、H.265、AAC 的编解码。
- **`libavutil` 管「地基」**：内存、引用计数、时间基、错误码。

### 1.2 数据在库里的流动

```mermaid
flowchart LR
    A["输入 URL / 文件 / 设备"] --> B["AVIOContext<br/>字节读写"]
    B --> C["AVFormatContext + demuxer<br/>解封装"]
    C -->|"AVPacket<br/>压缩数据"| D["AVCodecContext + decoder<br/>解码"]
    D -->|"AVFrame<br/>原始数据"| E["滤镜 / swscale / swresample<br/>缩放、转格式、重采样"]
    E --> F["AVCodecContext + encoder<br/>编码"]
    F -->|"AVPacket<br/>压缩数据"| G["AVFormatContext + muxer<br/>封装"]
    G --> H["AVIOContext<br/>字节读写"]
    H --> I["输出 URL / 文件 / 设备"]
```

把这张图记住，后面所有 API 都能对号入座：

- **解封装**：`avformat_open_input` → `av_read_frame` → `AVPacket`。
- **解码**：`avcodec_send_packet` → `avcodec_receive_frame` → `AVFrame`。
- **编码**：`avcodec_send_frame` → `avcodec_receive_packet` → `AVPacket`。
- **封装**：`avformat_write_header` → `av_interleaved_write_frame` → `av_write_trailer`。

### 1.3 三个上下文、两种数据、两条时间基

| 概念 | 作用 | 关键字段 |
| --- | --- | --- |
| `AVFormatContext` | 容器层：管理流、IO、demuxer/muxer | `streams`、`nb_streams`、`pb`、`iformat/oformat`、`duration` |
| `AVCodecContext` | 编解码层：管理编解码器状态 | `codec_type`、`width/height`、`pix_fmt`、`sample_fmt`、`time_base`、`bit_rate` |
| `AVIOContext` | 字节层：读写文件/网络/内存 | `read_packet`、`write_packet`、`seek`、`buffer` |
| `AVPacket` | 压缩数据（encoded） | `pts`、`dts`、`duration`、`stream_index`、`flags`、`data/size`、`buf` |
| `AVFrame` | 原始数据（decoded） | `data/linesize`、`format`、`width/height`、`nb_samples`、`ch_layout`、`pts` |
| `AVStream.time_base` | 容器时间基：packet 的时间戳单位 | 由 demuxer 设置；输出时 muxer 可能改写 |
| `AVCodecContext.time_base` | 编码器时间基：frame 的 pts 单位 | 视频常为 `1/fps`，音频常为 `1/sample_rate` |

> [!warning] 最容易出错的地方
> `AVPacket` 的 `pts/dts` 用 **stream 的 time_base**；`AVFrame` 送进编码器时，`pts` 用 **encoder 的 time_base**。两者之间必须用 `av_packet_rescale_ts()` 或 `av_rescale_q()` 转换。

---

## 二、核心对象与生命周期

### 2.1 生命周期总表

| 对象 | 创建 | 释放 | 是否引用计数 | 备注 |
| --- | --- | --- | --- | --- |
| 输入 `AVFormatContext` | `avformat_open_input` | `avformat_close_input` | 否 | 会连同所有 `AVStream` 一起释放 |
| 输出 `AVFormatContext` | `avformat_alloc_output_context2` | `avformat_free_context` | 否 | 先 `avio_closep(&ctx->pb)` |
| `AVIOContext` | `avio_open2` / `avio_alloc_context` | `avio_closep` / `avio_context_free` | 否 | 自定义 IO 的 buffer 要自己释放 |
| `AVStream` | `avformat_new_stream` | 随 `AVFormatContext` 释放 | 否 | 不要单独 free |
| `AVCodecParameters` | 随 `AVStream` 分配 | 随 `AVFormatContext` 释放 | 否 | 内含 `extradata`，不要手动 free |
| `AVCodecContext` | `avcodec_alloc_context3` | `avcodec_free_context` | 否 | 编码/解码各一个 |
| `AVCodec` | `avcodec_find_*` | 不需要释放 | 否 | 静态注册表，只读 |
| `AVPacket` | `av_packet_alloc` | `av_packet_free` | 是（`buf`） | `av_packet_unref` 只释放数据 |
| `AVFrame` | `av_frame_alloc` | `av_frame_free` | 是（`buf[]`） | `av_frame_unref` 只释放数据 |
| `AVBufferRef` | `av_buffer_alloc/create/ref` | `av_buffer_unref` | 是（原子计数） | 最后一个引用释放时回调 free |
| `AVDictionary` | `av_dict_set` | `av_dict_free` | 否 | 选项字典 |
| `SwsContext` | `sws_getContext` | `sws_freeContext` | 否 | 视频缩放 |
| `SwrContext` | `swr_alloc_set_opts2` | `swr_free` | 否 | 音频重采样 |

### 2.2 生命周期口诀

1. **谁创建，谁释放**：`alloc` 对 `free`，`open` 对 `close`。
2. **输入上下文用 `avformat_close_input`**：它会把 `AVFormatContext` 和所有流一起释放，并把指针置 `NULL`。
3. **输出上下文用 `avformat_free_context`**：释放前先关闭 `pb`。
4. **`AVPacket` / `AVFrame` 结构体与数据分开释放**：`unref` 放数据，`free` 放结构体。
5. **`AVCodec` 是静态的**：只读，永远不要 `free`。

---

## 三、解封装：打开输入与取流

### 3.1 `avformat_open_input`：打开输入

```c
int avformat_open_input(AVFormatContext **ps,
                        const char *url,
                        const AVInputFormat *fmt,
                        AVDictionary **options);
```

关键语义：

- `ps` 可以指向 `NULL`，函数内部会分配；也可以先 `avformat_alloc_context()` 再传进来。
- **预分配的主要用途**：在打开之前设置 `interrupt_callback`（中断回调）或自定义 `pb`。
- `fmt` 为 `NULL` 时自动探测容器格式；RTSP/RTMP/HTTP 通常传 `NULL`。
- `options` 是「输入选项」字典，函数返回后会被替换成「没被识别的选项」，所以无论成功失败都要 `av_dict_free`。
- **失败时**：如果 `*ps` 是用户预分配的，函数会把它释放并置 `NULL`，不要再 `close`。

```c
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <errno.h>

static void print_err(const char *what, int err)
{
    char buf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(err, buf, sizeof(buf));
    fprintf(stderr, "%s: %s\n", what, buf);
}

int open_demo(const char *url)
{
    AVFormatContext *fmt = NULL;
    AVDictionary *opts = NULL;
    int ret;

    /* 1. 先分配上下文：这样才能在打开前挂中断回调 */
    fmt = avformat_alloc_context();
    if (!fmt)
        return AVERROR(ENOMEM);

    /* 2. 中断回调：网络阻塞时能主动退出（后面 7.2 详细讲） */
    fmt->interrupt_callback.callback = my_interrupt_cb;
    fmt->interrupt_callback.opaque   = my_app;

    /* 3. 输入选项：RTSP 走 TCP，避免 UDP 丢包；超时 5 秒 */
    av_dict_set(&opts, "rtsp_transport", "tcp", 0);
    av_dict_set(&opts, "timeout", "5000000", 0);   /* 微秒，RTSP demuxer 选项 */
    av_dict_set(&opts, "rw_timeout", "5000000", 0);/* 微秒，底层 IO 读写超时 */

    /* 4. 打开输入；fmt 已预分配，传 &fmt */
    ret = avformat_open_input(&fmt, url, NULL, &opts);

    /* 5. 无论成功失败，选项字典都要释放（里面可能还有未识别项） */
    av_dict_free(&opts);

    if (ret < 0) {
        print_err("avformat_open_input", ret);
        return ret;                 /* 失败时 fmt 已被置 NULL，不要 close */
    }

    /* 6. 使用 ... */

    avformat_close_input(&fmt);     /* 释放上下文和所有流 */
    return 0;
}
```

> [!tip] 关于 `avformat_network_init()`
> 新版本里它是**可选且不推荐**的，只剩兼容老 GnuTLS/OpenSSL 线程问题的用途。除非你的构建/运行环境明确要求，否则不调用也可以。

### 3.2 `avformat_find_stream_info`：探测流信息

```c
int avformat_find_stream_info(AVFormatContext *ic, AVDictionary **options);
```

作用：读取一部分数据，补齐 `AVStream` 的编码参数（分辨率、帧率、采样率、extradata 等）。对 MPEG-TS、RTSP 这类「头信息不完整」的格式尤其重要。

代价：可能读若干秒的数据，**会拖慢首帧时间**。直播场景常用较小的 `probesize` / `analyzeduration`。

```c
/* 打开之后、找流之前设置，单位：probesize 是字节，max_analyze_duration 是 AV_TIME_BASE 单位 */
fmt->probesize           = 500000;          /* 约 500 KB */
fmt->max_analyze_duration = AV_TIME_BASE;   /* 最多分析 1 秒 */

int ret = avformat_find_stream_info(fmt, NULL);
if (ret < 0) {
    print_err("avformat_find_stream_info", ret);
    return ret;
}

/* 调试：打印所有流信息（第二个参数是流索引，0 表示全部；最后 0 表示输入） */
av_dump_format(fmt, 0, url, 0);
```

> [!note] 能不能不调用？
> MP4/FLV 这类头信息完整的文件，很多情况下可以跳过；但 RTSP、TS、裸流通常需要。直播为了低延迟，可以调小探测参数，但太小会识别失败，需要按流实测。

### 3.3 `av_find_best_stream`：选择流并找解码器

```c
int av_find_best_stream(AVFormatContext *ic,
                        enum AVMediaType type,
                        int wanted_stream_nb,
                        int related_stream,
                        const AVCodec **decoder_ret,
                        int flags);
```

返回值是流索引；失败返回负的 `AVERROR`。`decoder_ret` 非空时，成功会带回一个可用的解码器。

```c
const AVCodec *decoder = NULL;

/* 找「最好的视频流」；wanted_stream_nb = -1 表示自动选择 */
int v_idx = av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
if (v_idx < 0) {
    print_err("av_find_best_stream(video)", v_idx);
    return v_idx;
}

AVStream *vst = fmt->streams[v_idx];

printf("video: index=%d codec=%s %dx%d time_base=%d/%d\n",
       v_idx,
       avcodec_get_name(vst->codecpar->codec_id),
       vst->codecpar->width,
       vst->codecpar->height,
       vst->time_base.num, vst->time_base.den);

/* 如果 av_find_best_stream 没带回 decoder，可以自己按 codec_id 找 */
if (!decoder)
    decoder = avcodec_find_decoder(vst->codecpar->codec_id);
```

`AVStream` / `AVCodecParameters` 常用字段：

| 字段 | 含义 |
| --- | --- |
| `stream->index` | 流索引，`AVPacket.stream_index` 与它对应 |
| `stream->codecpar` | 编码参数：`codec_id`、`codec_type`、`width/height`、`format`、`sample_rate`、`ch_layout`、`extradata` |
| `stream->time_base` | 该流的时间基 |
| `stream->avg_frame_rate` | 平均帧率 |
| `stream->r_frame_rate` | 基本帧率（常用于 CFR 估计） |
| `stream->start_time` | 流起始时间（stream time_base） |
| `stream->duration` | 流时长（stream time_base） |
| `stream->disposition` | 流用途标记（默认、字幕、封面等） |

### 3.4 `av_read_frame`：读取一个压缩包

```c
int av_read_frame(AVFormatContext *s, AVPacket *pkt);
```

关键语义：

- 成功返回 `0`；`AVERROR_EOF` 表示结束；其他负值是错误。
- 返回的 `pkt` 是**引用计数**的，`pkt->buf` 有效，必须 `av_packet_unref()`。
- `pkt->stream_index` 告诉你它属于哪条流；**不要把所有包都塞给视频解码器**。
- `pkt->pts/dts/duration` 已经在该流的 `time_base` 下。
- 失败时 `pkt` 会被清空；`pkt` 可以是未初始化的，但不能带需要释放的数据。
- 网络流上它可能阻塞；靠 `interrupt_callback` 或协议超时退出。

```c
AVPacket *pkt = av_packet_alloc();
if (!pkt)
    return AVERROR(ENOMEM);

for (;;) {
    int ret = av_read_frame(fmt, pkt);
    if (ret == AVERROR_EOF)
        break;                      /* 正常结束 */
    if (ret < 0) {
        print_err("av_read_frame", ret);
        break;                      /* 网络错误 / 被中断 */
    }

    if (pkt->stream_index == v_idx) {
        /* 只把视频包送进视频解码器 */
        decode_packet(video_dec, pkt, frame, on_frame, app);
    }

    av_packet_unref(pkt);           /* 每次读完必须释放数据 */
}

av_packet_free(&pkt);
```

### 3.5 `av_seek_frame` / `avformat_seek_file`：跳转

```c
int av_seek_frame(AVFormatContext *s, int stream_index,
                  int64_t timestamp, int flags);
int avformat_seek_file(AVFormatContext *s, int stream_index,
                       int64_t min_ts, int64_t ts, int64_t max_ts, int flags);
```

- `timestamp` 单位：指定流时用该流的 `time_base`；`stream_index = -1` 时用 `AV_TIME_BASE`（微秒）。
- `AVSEEK_FLAG_BACKWARD`：跳到目标之前最近的关键帧。
- 跳转后**必须 `avcodec_flush_buffers(dec)`**，否则解码器里残留的旧帧会串出来。

```c
int64_t target = 10 * AV_TIME_BASE;   /* 10 秒，stream_index = -1 时用微秒 */

int ret = av_seek_frame(fmt, -1, target, AVSEEK_FLAG_BACKWARD);
if (ret < 0) {
    print_err("av_seek_frame", ret);
}

/* 清空解码器内部缓存的帧和参考帧 */
avcodec_flush_buffers(video_dec);
```

### 3.6 `avformat_close_input`：关闭输入

```c
void avformat_close_input(AVFormatContext **s);
```

```c
avformat_close_input(&fmt);   /* fmt 被置 NULL；所有 AVStream 一并释放 */
```

> [!warning] 不要对输入上下文再调用 `avformat_free_context`
> `avformat_close_input` 已经释放了上下文和所有流；再 `avformat_free_context` 就是重复释放。

---

## 四、解码：从 AVPacket 到 AVFrame

### 4.1 打开解码器

标准四步：

1. `avcodec_find_decoder()` / `av_find_best_stream()` 拿到 `AVCodec`。
2. `avcodec_alloc_context3()` 分配 `AVCodecContext`。
3. `avcodec_parameters_to_context()` 把 `AVStream.codecpar` 复制进上下文。
4. `avcodec_open2()` 打开解码器。

```c
AVCodecContext *dec = avcodec_alloc_context3(decoder);
if (!dec)
    return AVERROR(ENOMEM);

/* 把容器里的编码参数复制到解码上下文（extradata 也会复制） */
int ret = avcodec_parameters_to_context(dec, vst->codecpar);
if (ret < 0) {
    print_err("avcodec_parameters_to_context", ret);
    avcodec_free_context(&dec);
    return ret;
}

/* 告诉解码器：送进来的 packet 的时间基是 stream 的 time_base */
dec->pkt_timebase = vst->time_base;

/* 低延迟：单线程解码，尽量不排队；离线转码可设 0 让 FFmpeg 自动选择 */
dec->thread_count = 1;
dec->flags  |= AV_CODEC_FLAG_LOW_DELAY;
dec->flags2 |= AV_CODEC_FLAG2_FAST;

/* 解码器私有选项可以放在字典里，例如 "threads"、"low_delay" */
AVDictionary *opts = NULL;
ret = avcodec_open2(dec, decoder, &opts);
if (ret < 0) {
    print_err("avcodec_open2(decoder)", ret);
    av_dict_free(&opts);
    avcodec_free_context(&dec);
    return ret;
}
av_dict_free(&opts);
```

> [!note] 为什么需要 `avcodec_parameters_to_context`
> FFmpeg 5.0 起 `AVStream.codec` 已被移除，编码参数统一放在 `AVStream.codecpar`。解码器必须通过这个函数把参数（尤其是 `extradata`）复制进自己的上下文，否则 H.264/H.265 可能无法解析 SPS/PPS。

### 4.2 解码循环：send / receive

新 API 的核心是「生产者—消费者」模型：

- `avcodec_send_packet()`：送一个压缩包。
- `avcodec_receive_frame()`：取解码后的原始帧，可能一次取多帧，也可能暂时取不到。
- 送 `NULL` 表示 flush，把解码器内部延迟的帧全部吐出来。

```c
typedef int (*frame_cb)(AVFrame *frame, void *opaque);

/*
 * 解码一个 packet 并把所有可用的 frame 通过回调交出去。
 * pkt == NULL 表示 flush。
 *
 * 返回值：0 表示正常（包括暂时没有输出）；负值表示真正的错误。
 */
static int decode_packet(AVCodecContext *dec, AVPacket *pkt, AVFrame *frame,
                         frame_cb on_frame, void *opaque)
{
    int ret = avcodec_send_packet(dec, pkt);

    /* EAGAIN：解码器输出队列还没取空，先 receive，再重发同一个包 */
    if (ret < 0 && ret != AVERROR(EAGAIN))
        return ret;

    int need_resend = (ret == AVERROR(EAGAIN));

    for (;;) {
        ret = avcodec_receive_frame(dec, frame);

        if (ret == 0) {
            int cb_ret = on_frame(frame, opaque);
            av_frame_unref(frame);      /* 交出去后立即释放引用 */
            if (cb_ret < 0)
                return cb_ret;
            continue;
        }
		// 所有的帧都解完了，退出
        if (ret == AVERROR_EOF)
            return 0;                   /* flush 完毕 */

        if (ret != AVERROR(EAGAIN))
            return ret;                 /* 真正的解码错误 */

        /* 输出暂时取空 */
        if (!need_resend)
            return 0;

        /* 刚才 send 被 EAGAIN 拒绝，现在重发 */
        ret = avcodec_send_packet(dec, pkt);
        if (ret == 0) {
            need_resend = 0;
            continue;
        }
        if (ret == AVERROR(EAGAIN))
            continue;
        return ret;
    }
}
```

调用方式：

```c
for (;;) {
    int ret = av_read_frame(fmt, pkt);
    if (ret == AVERROR_EOF)
        break;
    if (ret < 0)
        break;

    if (pkt->stream_index == v_idx) {
        ret = decode_packet(dec, pkt, frame, on_frame, app);
        if (ret < 0) {
            print_err("decode_packet", ret);
            av_packet_unref(pkt);
            break;
        }
    }

    av_packet_unref(pkt);
}

/* 输入结束：送 NULL flush，把解码器里延迟的帧全部取出 */
decode_packet(dec, NULL, frame, on_frame, app);
```

> [!important] `EAGAIN` 和 `EOF` 不是错误
> - `AVERROR(EAGAIN)`：当前状态取不到输出，需要再送输入（或者反过来，送不进输入就先取输出）。
> - `AVERROR_EOF`：flush 完成，不会再有输出。
> - 真正的错误是其他负值。很多教程只写「send 一次 + receive 一次」，遇到多帧包或 B 帧延迟就会漏帧。

### 4.3 `AVFrame` 怎么看

| 字段 | 视频 | 音频 |
| --- | --- | --- |
| `data[]` / `extended_data` | 每个平面一个指针：YUV 的 Y/U/V | PCM 每个声道/平面一个指针 |
| `linesize[]` | 每行字节数，可能大于宽度 | 每个平面的字节跨度 |
| `format` | `AVPixelFormat` | `AVSampleFormat` |
| `width/height` | 画面尺寸 | 不使用 |
| `nb_samples` | 不使用 | 本帧采样数 |
| `ch_layout` | 不使用 | 声道布局（`AVChannelLayout`） |
| `pts` | 显示时间戳 | 显示时间戳 |
| `pkt_dts` | 触发该帧的 packet 的 DTS | 同上 |
| `best_effort_timestamp` | 解码器估算的时间戳 | 同上 |
| `duration` | 帧时长（可能为 0） | 帧时长 |
| `flags` | `AV_FRAME_FLAG_KEY` 等 | 同上 |
| `pict_type` | I/P/B 帧类型 | 不使用 |

```c
static int on_frame(AVFrame *frame, void *opaque)
{
    App *app = opaque;
    AVStream *st = app->fmt->streams[app->video_index];

    /* frame->pts 的时间基是 pkt_timebase（这里设成了 stream->time_base） */
    double pts = -1.0;
    if (frame->pts != AV_NOPTS_VALUE)
        pts = frame->pts * av_q2d(st->time_base);

    printf("frame #%lld: %dx%d fmt=%d pts=%.3f key=%d\n",
           (long long)app->frame_count++,
           frame->width, frame->height,
           frame->format,
           pts,
           !!(frame->flags & AV_FRAME_FLAG_KEY));

    /* 注意：这里只是借用 frame，返回后 decode_packet 会 av_frame_unref */
    return 0;
}
```

### 4.4 解码时间戳：谁的时间基？

- `AVPacket.pts/dts`：在 **`AVStream.time_base`** 下。
- 解码器输出 `AVFrame.pts`：默认沿用 packet 的时间戳，所以在 **`pkt_timebase`** 下；把 `dec->pkt_timebase = st->time_base` 设好，就能直接用 `st->time_base` 换算。
- `AVFrame.time_base` 字段在新版本存在，但头文件明确说「未来可能由解码器/滤镜设置，送给编码器时默认会被忽略」——**不要依赖它**，显式用 stream/codec 的时间基换算。
- `frame->best_effort_timestamp`：解码器根据 DTS、帧重排等信息估算的显示时间，遇到坏流时比 `pts` 更稳。

```c
/* 把 frame 的 pts 转成秒 */
double seconds = frame->pts == AV_NOPTS_VALUE
               ? -1.0
               : frame->pts * av_q2d(st->time_base);

/* 把 frame 的 pts 转成编码器时间基，准备重新编码 */
frame->pts = av_rescale_q(frame->pts, st->time_base, enc->time_base);
```

### 4.5 `avcodec_flush_buffers`：清空解码器状态

```c
void avcodec_flush_buffers(AVCodecContext *avctx);
```

用途：

- seek 之后。
- 断线重连、切换码流之后。
- 输入时间戳发生跳变、需要丢弃旧参考帧时。

```c
avcodec_flush_buffers(dec);
/* 注意：它只释放解码器内部的引用，调用者手里的 frame/packet 引用仍然有效 */
```

> [!warning] 重连后如果 SPS/PPS 变了
> 仅 `flush` 不够。分辨率、profile、extradata 变化时，应该重新 `avcodec_parameters_to_context` 并重新 `avcodec_open2`，或者直接销毁重建解码器。

---

## 五、编码：从 AVFrame 到 AVPacket

### 5.1 选择编码器与设置参数

```c
const AVCodec *codec = avcodec_find_encoder_by_name("libx264");
if (!codec)
    codec = avcodec_find_encoder(AV_CODEC_ID_H264);

AVCodecContext *enc = avcodec_alloc_context3(codec);
if (!enc)
    return AVERROR(ENOMEM);

enc->width      = 1280;
enc->height     = 720;
enc->pix_fmt    = AV_PIX_FMT_YUV420P;  /* libx264 最通用、兼容性最好的输入格式 */
enc->time_base  = (AVRational){1, 25}; /* 编码器时间基：1/帧率 */
enc->framerate  = (AVRational){25, 1};
enc->gop_size   = 50;                  /* 2 秒一个关键帧 */
enc->max_b_frames = 0;                 /* 直播低延迟：禁用 B 帧 */

/* 码率控制：CBR 直播常用 */
enc->bit_rate       = 2000000;
enc->rc_min_rate    = enc->bit_rate;
enc->rc_max_rate    = enc->bit_rate;
enc->rc_buffer_size = enc->bit_rate;   /* 1 秒 VBV 缓冲 */

/* 编码器私有选项 */
AVDictionary *opts = NULL;
av_dict_set(&opts, "preset", "veryfast", 0);
av_dict_set(&opts, "tune", "zerolatency", 0);
av_dict_set(&opts, "x264-params", "nal-hrd=cbr:force-cfr=1", 0);

/* 输出格式要求全局头：FLV/MP4 需要把 SPS/PPS 放进 extradata/文件头 */
if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
    enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

int ret = avcodec_open2(enc, codec, &opts);
if (ret < 0) {
    print_err("avcodec_open2(encoder)", ret);
    av_dict_free(&opts);
    avcodec_free_context(&enc);
    return ret;
}

/* 打开后检查有没有没被识别的选项 */
if (av_dict_count(opts) > 0) {
    AVDictionaryEntry *e = NULL;
    while ((e = av_dict_get(opts, "", e, AV_DICT_IGNORE_SUFFIX)))
        fprintf(stderr, "unused encoder option: %s=%s\n", e->key, e->value);
}
av_dict_free(&opts);
```

常用视频编码参数：

| 参数 | 作用 | 直播建议 |
| --- | --- | --- |
| `preset` | 编码速度/压缩率权衡 | `ultrafast` / `veryfast` / `superfast` |
| `tune` | 场景调优 | `zerolatency`（低延迟） |
| `profile` | H.264 Profile | 兼容性优先 `baseline` / `main` |
| `gop_size` | 关键帧间隔 | 帧率的 1~2 倍 |
| `max_b_frames` | B 帧数量 | 直播设 0 |
| `bit_rate` | 目标码率 | 按带宽设定 |
| `rc_min_rate/rc_max_rate/rc_buffer_size` | CBR/VBV | 三者配合 |
| `AV_CODEC_FLAG_GLOBAL_HEADER` | extradata 放全局头 | FLV/MP4 必须 |

> [!tip] 编码器不一定支持你想要的像素格式
> 编码前可以遍历 `codec->pix_fmts`，或用 `avcodec_get_supported_config()`（新版本）查询；如果输入格式不支持，先用 `swscale` 转成 `AV_PIX_FMT_YUV420P`。

### 5.2 分配 `AVFrame` 并填数据

视频帧：

```c
AVFrame *frame = av_frame_alloc();
if (!frame)
    return AVERROR(ENOMEM);

frame->format = enc->pix_fmt;
frame->width  = enc->width;
frame->height = enc->height;

/* align = 0 表示让 FFmpeg 选当前 CPU 最合适的对齐 */
int ret = av_frame_get_buffer(frame, 0);
if (ret < 0) {
    print_err("av_frame_get_buffer", ret);
    av_frame_free(&frame);
    return ret;
}
```

填一帧 YUV420P（渐变 + 固定色度）：

```c
/* 每次写之前确保可写：如果 frame 被别的对象共享，会自动拷贝一份 */
ret = av_frame_make_writable(frame);
if (ret < 0)
    return ret;

/* Y 平面：每行一个渐变 */
for (int y = 0; y < enc->height; y++) {
    uint8_t *row = frame->data[0] + y * frame->linesize[0];
    for (int x = 0; x < enc->width; x++)
        row[x] = (uint8_t)(x + y + frame_index * 3);
}

/* U/V 平面：420 采样，宽高都是亮度的一半 */
for (int y = 0; y < enc->height / 2; y++) {
    memset(frame->data[1] + y * frame->linesize[1], 128, enc->width / 2);
    memset(frame->data[2] + y * frame->linesize[2], 128, enc->width / 2);
}

/* 编码器时间基下的 pts：第 frame_index 帧 */
frame->pts = frame_index;
```

音频帧：

```c
AVFrame *aframe = av_frame_alloc();
aframe->format     = aenc->sample_fmt;     /* AAC 通常是 AV_SAMPLE_FMT_FLTP */
aframe->nb_samples = aenc->frame_size;     /* AAC 打开后通常是 1024 */
av_channel_layout_copy(&aframe->ch_layout, &aenc->ch_layout);

int ret = av_frame_get_buffer(aframe, 0);
if (ret < 0) {
    print_err("av_frame_get_buffer(audio)", ret);
    return ret;
}

/* 音频 pts 用采样数计量：每帧递增 nb_samples */
aframe->pts = total_samples;
total_samples += aframe->nb_samples;
```

> [!warning] `av_frame_get_buffer` 不能重复调用
> 如果 frame 已经有 buffer，再次调用会泄漏。重用帧时先 `av_frame_unref(frame)`，或者只对唯一的帧调用 `av_frame_make_writable()`。

> [!note] 手动包装外部内存
> 如果数据来自外部（摄像头、GPU、共享内存），可以用 `av_image_get_buffer_size()` 算大小、`av_image_fill_arrays()` 填 `data/linesize`，但这样**不会自动引用计数**。要交给解码器/编码器安全使用，最好再用 `av_buffer_create()` 包成 `AVBufferRef` 并挂到 `frame->buf[0]`；否则 frame 释放时不会管理这块内存。

### 5.3 编码循环：send / receive

```c
/* 送一帧；frame == NULL 表示 flush */
static int encode_frame(AVCodecContext *enc, AVFrame *frame,
                        AVStream *st, AVFormatContext *oc, AVPacket *pkt)
{
    int ret = avcodec_send_frame(enc, frame);
    if (ret < 0)
        return ret;

    for (;;) {
        ret = avcodec_receive_packet(enc, pkt);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return 0;               /* 暂时没有更多输出 / flush 完成 */
        if (ret < 0)
            return ret;

        /* 编码器输出的时间基是 enc->time_base，muxer 需要 stream->time_base */
        av_packet_rescale_ts(pkt, enc->time_base, st->time_base);
        pkt->stream_index = st->index;

        /* av_interleaved_write_frame 会接管引用，返回后 pkt 变空 */
        ret = av_interleaved_write_frame(oc, pkt);
        if (ret < 0)
            return ret;
    }
}
```

完整调用：

```c
for (int i = 0; i < total_frames; i++) {
    av_frame_make_writable(frame);
    fill_yuv420p(frame, i);
    frame->pts = i;                 /* encoder time_base 下 */

    int ret = encode_frame(enc, frame, vst, oc, pkt);
    if (ret < 0) {
        print_err("encode_frame", ret);
        break;
    }
}

/* flush：把编码器内部延迟的帧全部吐出来 */
encode_frame(enc, NULL, vst, oc, pkt);
```

> [!important] 编码器也会延迟
> H.264 的 B 帧、lookahead、多线程都会让「送进去的帧数」和「吐出来的包数」不相等。**必须 flush**，否则最后几帧会丢。

### 5.4 音频编码补充：AAC 与重采样

AAC 编码器常见要求：

- `sample_fmt = AV_SAMPLE_FMT_FLTP`（planar float）。
- `sample_rate = 48000`（或 44100）。
- `ch_layout` 用 `AVChannelLayout`，例如双声道。
- `frame_size` 由编码器决定，AAC 通常是 1024。
- 每帧 `nb_samples` 必须等于 `frame_size`（最后一帧可以小）。
- `pts` 以采样数为单位递增。

`SwrContext` 做输入格式转换：

```c
SwrContext *swr = NULL;

/* 输出：AAC 编码器要求的格式；输入：解码器输出的格式 */
int ret = swr_alloc_set_opts2(&swr,
        &aenc->ch_layout, aenc->sample_fmt, aenc->sample_rate,
        &adec->ch_layout, adec->sample_fmt, adec->sample_rate,
        0, NULL);
if (ret < 0)
    return ret;

ret = swr_init(swr);
if (ret < 0) {
    print_err("swr_init", ret);
    swr_free(&swr);
    return ret;
}

/* 估算输出采样数，分配输出缓冲 */
int out_count = swr_get_out_samples(swr, in_frame->nb_samples);
uint8_t **out_data = NULL;
ret = av_samples_alloc_array_and_samples(&out_data, NULL,
        aenc->ch_layout.nb_channels, out_count, aenc->sample_fmt, 0);
if (ret < 0)
    return ret;

/* 执行转换 */
int got = swr_convert(swr, out_data, out_count,
                      (const uint8_t **)in_frame->extended_data,
                      in_frame->nb_samples);
if (got < 0) {
    print_err("swr_convert", got);
}

/* 用 out_data / got 构造编码帧 ... */

/* 清理 */
av_freep(&out_data[0]);
av_freep(&out_data);
swr_free(&swr);
```

> [!note] 新版本声道 API
> FFmpeg 5.1 起推荐用 `AVChannelLayout ch_layout`，旧的 `channels` / `channel_layout` 已移除。初始化：
> `av_channel_layout_default(&layout, 2);` 或使用宏 `AV_CHANNEL_LAYOUT_STEREO`。
> 重采样用 `swr_alloc_set_opts2()`，不要再使用旧的 `swr_alloc_set_opts()`。

### 5.5 视频缩放 / 像素格式转换：`sws_scale`

```c
#include <libswscale/swscale.h>

SwsContext *sws = sws_getContext(
        src->width, src->height, src->format,     /* 输入 */
        dst_w, dst_h, AV_PIX_FMT_YUV420P,         /* 输出 */
        SWS_BILINEAR, NULL, NULL, NULL);
if (!sws)
    return AVERROR(EINVAL);

AVFrame *dst = av_frame_alloc();
dst->format = AV_PIX_FMT_YUV420P;
dst->width  = dst_w;
dst->height = dst_h;

int ret = av_frame_get_buffer(dst, 0);
if (ret < 0)
    return ret;

/* 逐帧转换：src->data/linesize 可能来自解码器，dst 是刚分配的 */
sws_scale(sws,
          (const uint8_t * const *)src->data, src->linesize,
          0, src->height,
          dst->data, dst->linesize);

/* 转换后的 dst 可以直接送编码器；记得把 pts 也带过去 */
dst->pts = src->pts;

sws_freeContext(sws);
```

> [!tip] 滤镜图可以替代 swscale/swr
> 复杂场景（裁剪、叠加、旋转、多路混流）建议用 `libavfilter` 的 `buffer` → `scale/format/aresample` → `buffersink` 图。简单的缩放/转格式用 `sws_scale` 更直接。

---

## 六、封装与推流：从 AVPacket 到网络

### 6.1 创建输出上下文和流

```c
AVFormatContext *oc = NULL;

/* 根据 URL 创建输出上下文；format_name 明确指定 "flv"、"mpegts" 等 */
int ret = avformat_alloc_output_context2(&oc, NULL, "flv", url);
if (ret < 0) {
    print_err("avformat_alloc_output_context2", ret);
    return ret;
}

/* 新建一条视频流；第二个参数传 NULL 即可（已废弃） */
AVStream *vst = avformat_new_stream(oc, NULL);
if (!vst) {
    avformat_free_context(oc);
    return AVERROR(ENOMEM);
}

/* 把编码器的参数（宽高、pix_fmt、extradata）复制到流 */
ret = avcodec_parameters_from_context(vst->codecpar, enc);
if (ret < 0) {
    print_err("avcodec_parameters_from_context", ret);
    return ret;
}

/* 告诉 muxer 期望的时间基；write_header 之后可能被改写 */
vst->time_base      = enc->time_base;
vst->avg_frame_rate = enc->framerate;

/* 让 muxer 自己决定 codec_tag（如 mp4 的 avc1、flv 的 7） */
vst->codecpar->codec_tag = 0;
```

关键点：

- `avformat_new_stream()` 创建的 `AVStream` 由 `AVFormatContext` 拥有，不要单独释放。
- `AVStream.codecpar` 也由 libavformat 分配/释放，不要手动 free。
- 输出流的 `codecpar` 必须在 `avformat_write_header()` 之前填好。
- `codec_tag = 0` 让 muxer 选；手动填错会导致封装失败或播放器不识别。

### 6.2 打开输出 IO：`avio_open2`

```c
int avio_open2(AVIOContext **s, const char *url, int flags,
               const AVIOInterruptCB *int_cb, AVDictionary **options);
```

- `flags = AVIO_FLAG_WRITE`。
- `int_cb` 是协议层中断回调，网络阻塞时能退出。
- `options` 可传 `rw_timeout` 等协议选项。
- 如果输出格式带 `AVFMT_NOFILE`（例如自定义 IO、某些设备），不要调用 `avio_open2`。

```c
if (!(oc->oformat->flags & AVFMT_NOFILE)) {
    AVDictionary *io_opts = NULL;

    /* 5 秒读写超时，单位微秒 */
    av_dict_set(&io_opts, "rw_timeout", "5000000", 0);

    ret = avio_open2(&oc->pb, url, AVIO_FLAG_WRITE,
                     &oc->interrupt_callback, &io_opts);
    if (ret < 0) {
        print_err("avio_open2", ret);
        av_dict_free(&io_opts);
        return ret;
    }
    av_dict_free(&io_opts);
}
```

### 6.3 写头、写包、写尾

```c
/* 1. 写文件头/流头；可能改写 stream->time_base */
ret = avformat_write_header(oc, NULL);
if (ret < 0) {
    print_err("avformat_write_header", ret);
    return ret;
}

/* 2. 写包 */
ret = av_interleaved_write_frame(oc, pkt);

/* 3. 写尾：索引、duration、文件结束标记等 */
av_write_trailer(oc);

/* 4. 关闭 IO，再释放上下文 */
if (!(oc->oformat->flags & AVFMT_NOFILE))
    avio_closep(&oc->pb);
avformat_free_context(oc);
```

`av_interleaved_write_frame` 的所有权语义非常重要：

- 如果 packet 是引用计数的，**函数接管这个引用**，并在合适的时候释放。
- 返回后 packet 会变成空包（`av_packet_alloc()` 的初始状态），**即使返回错误也是如此**。
- 所以**不要再 `av_packet_unref()`，更不要继续访问 `pkt->data`**。

对比 `av_write_frame`：

- **不接管所有权**，调用者仍然负责 `av_packet_unref()`。
- 某些 muxer 可能内部引用 packet，所以写入后也不要修改 packet 内容。
- 它不做交织，需要调用者自己保证音视频包顺序；一般用 `av_interleaved_write_frame`。

### 6.4 时间戳与交织

写入 muxer 前必须保证：

1. `pkt->stream_index` 正确。
2. `pkt->pts/dts/duration` 在 **该输出流的 `time_base`** 下。
3. 同一流的 **DTS 严格递增**（除非格式标记 `AVFMT_TS_NONSTRICT`，则非递减）。
4. 音视频包交给 `av_interleaved_write_frame` 自动交织。

```c
/* 编码器时间基 -> 输出流时间基 */
av_packet_rescale_ts(pkt, enc->time_base, vst->time_base);

/* 指定属于哪条流 */
pkt->stream_index = vst->index;

/* 音频同理：aenc->time_base -> ast->time_base */
av_packet_rescale_ts(apkt, aenc->time_base, ast->time_base);
apkt->stream_index = ast->index;
```

> [!warning] 为什么会出现 "Non-monotonous DTS in output stream"
> 常见原因：
> - 用错了时间基，PTS/DTS 换算后变小；
> - B 帧导致 DTS 与 PTS 不同，却没有按 DTS 排序；
> - 重连/seek 后时间戳回到 0，但没有加偏移；
> - 多线程编码导致包乱序，自己用 `av_write_frame` 而不是 `av_interleaved_write_frame`。

### 6.5 推流完整示例：H.264 → FLV → RTMP

下面这个例子不依赖输入文件，直接生成 10 秒渐变视频并推到 RTMP 服务器。重点看流程和资源释放。

```c
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/error.h>
#include <libavutil/time.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define WIDTH        1280
#define HEIGHT       720
#define FPS          25
#define DURATION_SEC 10

static volatile int g_abort = 0;

static void on_sigint(int sig)
{
    (void)sig;
    g_abort = 1;
}

static int interrupt_cb(void *opaque)
{
    (void)opaque;
    return g_abort;
}

static void print_err(const char *what, int err)
{
    char buf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(err, buf, sizeof(buf));
    fprintf(stderr, "%s: %s\n", what, buf);
}

/* 生成一帧 YUV420P：Y 平面渐变，U/V 固定 128 */
static void fill_yuv420p(AVFrame *frame, int idx)
{
    for (int y = 0; y < HEIGHT; y++) {
        uint8_t *row = frame->data[0] + y * frame->linesize[0];
        for (int x = 0; x < WIDTH; x++)
            row[x] = (uint8_t)(x + y + idx * 3);
    }

    for (int y = 0; y < HEIGHT / 2; y++) {
        memset(frame->data[1] + y * frame->linesize[1], 128, WIDTH / 2);
        memset(frame->data[2] + y * frame->linesize[2], 128, WIDTH / 2);
    }
}

int main(int argc, char **argv)
{
    const char *url = (argc > 1) ? argv[1]
                                 : "rtmp://127.0.0.1:1935/live/test";

    AVFormatContext *oc = NULL;
    AVStream *vst = NULL;
    const AVCodec *codec = NULL;
    AVCodecContext *enc = NULL;
    AVFrame *frame = NULL;
    AVPacket *pkt = NULL;
    AVDictionary *opts = NULL;
    int ret;
    int header_written = 0;

    signal(SIGINT, on_sigint);

    /* 1. 创建输出上下文：强制 FLV 封装，RTMP 的标准封装格式 */
    ret = avformat_alloc_output_context2(&oc, NULL, "flv", url);
    if (ret < 0) {
        print_err("avformat_alloc_output_context2", ret);
        goto end;
    }

    /* 给输出 IO 挂中断回调：Ctrl+C 或写超时时能退出阻塞 */
    oc->interrupt_callback.callback = interrupt_cb;
    oc->interrupt_callback.opaque   = NULL;

    /* 2. 找 H.264 编码器 */
    codec = avcodec_find_encoder_by_name("libx264");
    if (!codec)
        codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        fprintf(stderr, "H.264 encoder not found\n");
        ret = AVERROR_ENCODER_NOT_FOUND;
        goto end;
    }

    /* 3. 新建流 + 分配编码上下文 */
    vst = avformat_new_stream(oc, NULL);
    if (!vst) {
        fprintf(stderr, "avformat_new_stream failed\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    enc = avcodec_alloc_context3(codec);
    if (!enc) {
        fprintf(stderr, "avcodec_alloc_context3 failed\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    enc->width        = WIDTH;
    enc->height       = HEIGHT;
    enc->pix_fmt      = AV_PIX_FMT_YUV420P;
    enc->time_base    = (AVRational){1, FPS};
    enc->framerate    = (AVRational){FPS, 1};
    enc->gop_size     = FPS * 2;       /* 2 秒一个关键帧 */
    enc->max_b_frames = 0;             /* 直播禁用 B 帧 */
    enc->bit_rate     = 2000000;
    enc->rc_min_rate  = enc->bit_rate;
    enc->rc_max_rate  = enc->bit_rate;
    enc->rc_buffer_size = enc->bit_rate;

    /* 4. 编码器选项：低延迟 + CBR */
    av_dict_set(&opts, "preset", "veryfast", 0);
    av_dict_set(&opts, "tune", "zerolatency", 0);
    av_dict_set(&opts, "x264-params", "nal-hrd=cbr:force-cfr=1", 0);

    /* 5. FLV/MP4 要求全局头，extradata 要放进流里 */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    ret = avcodec_open2(enc, codec, &opts);
    av_dict_free(&opts);               /* 选项字典用完即释放 */
    if (ret < 0) {
        print_err("avcodec_open2", ret);
        goto end;
    }

    /* 6. 把编码器参数复制到输出流 */
    ret = avcodec_parameters_from_context(vst->codecpar, enc);
    if (ret < 0) {
        print_err("avcodec_parameters_from_context", ret);
        goto end;
    }

    vst->time_base      = enc->time_base;
    vst->avg_frame_rate = enc->framerate;
    vst->codecpar->codec_tag = 0;

    av_dump_format(oc, 0, url, 1);     /* 打印输出信息，调试用 */

    /* 7. 打开网络输出；rw_timeout 防止写阻塞永久挂住 */
    if (!(oc->oformat->flags & AVFMT_NOFILE)) {
        AVDictionary *io_opts = NULL;
        av_dict_set(&io_opts, "rw_timeout", "5000000", 0);

        ret = avio_open2(&oc->pb, url, AVIO_FLAG_WRITE,
                         &oc->interrupt_callback, &io_opts);
        av_dict_free(&io_opts);
        if (ret < 0) {
            print_err("avio_open2", ret);
            goto end;
        }
    }

    /* 8. 写头。注意：write_header 之后 vst->time_base 可能被 muxer 改写 */
    ret = avformat_write_header(oc, NULL);
    if (ret < 0) {
        print_err("avformat_write_header", ret);
        goto end;
    }
    header_written = 1;

    /* 9. 分配帧和包 */
    frame = av_frame_alloc();
    pkt   = av_packet_alloc();
    if (!frame || !pkt) {
        fprintf(stderr, "alloc frame/packet failed\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    frame->format = enc->pix_fmt;
    frame->width  = enc->width;
    frame->height = enc->height;

    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        print_err("av_frame_get_buffer", ret);
        goto end;
    }

    /* 10. 编码并推流 */
    for (int i = 0; i < FPS * DURATION_SEC && !g_abort; i++) {
        ret = av_frame_make_writable(frame);
        if (ret < 0) {
            print_err("av_frame_make_writable", ret);
            break;
        }

        fill_yuv420p(frame, i);
        frame->pts = i;                /* encoder time_base 下 */

        ret = avcodec_send_frame(enc, frame);
        if (ret < 0) {
            print_err("avcodec_send_frame", ret);
            break;
        }

        for (;;) {
            int dret = avcodec_receive_packet(enc, pkt);
            if (dret == AVERROR(EAGAIN) || dret == AVERROR_EOF)
                break;                 /* 暂时没有更多输出 / flush 完成 */
            if (dret < 0) {
                print_err("avcodec_receive_packet", dret);
                ret = dret;
                break;
            }

            /* 关键：时间基从编码器转换到输出流 */
            av_packet_rescale_ts(pkt, enc->time_base, vst->time_base);
            pkt->stream_index = vst->index;

            /* 成功或失败，pkt 都会被清空；不要 unref，也不要访问 data */
            ret = av_interleaved_write_frame(oc, pkt);
            if (ret < 0) {
                print_err("av_interleaved_write_frame", ret);
                break;
            }
        }

        if (ret < 0)
            break;

        /* 低延迟：确保数据真的写到 socket，而不是停在 AVIO 缓冲区 */
        if (oc->pb)
            avio_flush(oc->pb);
    }

    /* 11. flush 编码器：把延迟的帧全部吐出来 */
    avcodec_send_frame(enc, NULL);
    for (;;) {
        int dret = avcodec_receive_packet(enc, pkt);
        if (dret == AVERROR(EAGAIN) || dret == AVERROR_EOF)
            break;
        if (dret < 0) {
            print_err("avcodec_receive_packet(flush)", dret);
            if (ret >= 0)
                ret = dret;
            break;
        }

        av_packet_rescale_ts(pkt, enc->time_base, vst->time_base);
        pkt->stream_index = vst->index;

        int wret = av_interleaved_write_frame(oc, pkt);
        if (wret < 0) {
            print_err("av_interleaved_write_frame(flush)", wret);
            if (ret >= 0)
                ret = wret;
            break;
        }
    }

    /* 12. 写尾 */
    if (header_written) {
        int wret = av_write_trailer(oc);
        if (wret < 0 && ret >= 0) {
            print_err("av_write_trailer", wret);
            ret = wret;
        }
    }

end:
    /* 13. 释放顺序：先关网络/写尾，再释放上下文和编码器 */
    if (oc && !(oc->oformat->flags & AVFMT_NOFILE) && oc->pb)
        avio_closep(&oc->pb);

    avformat_free_context(oc);
    avcodec_free_context(&enc);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    av_dict_free(&opts);

    return ret < 0 ? 1 : 0;
}
```

> [!note] 这个例子的输入是「合成帧」
> 真实场景通常是「文件/摄像头 → 解码 → 滤镜 → 编码 → 推流」。把 `fill_yuv420p` 换成 `avcodec_receive_frame` 得到的帧，并注意 `frame->pts` 的时间基转换即可。

### 6.6 同时推音视频

流程相同，只是多一条音频流：

1. `avformat_new_stream(oc, NULL)` 再建一条 `AVMEDIA_TYPE_AUDIO` 流。
2. 音频编码器设置 `sample_fmt`、`sample_rate`、`ch_layout`、`bit_rate`、`frame_size`。
3. `avcodec_parameters_from_context(ast->codecpar, aenc)`。
4. 音频帧的 `pts` 按采样数递增。
5. 音视频包都交给 `av_interleaved_write_frame()`，让它按 DTS 交织。

```c
/* 音频流初始化骨架 */
AVStream *ast = avformat_new_stream(oc, NULL);
AVCodecContext *aenc = avcodec_alloc_context3(aac_codec);

aenc->sample_fmt  = AV_SAMPLE_FMT_FLTP;
aenc->sample_rate = 48000;
av_channel_layout_default(&aenc->ch_layout, 2);
aenc->bit_rate    = 128000;
aenc->time_base   = (AVRational){1, 48000};

if (oc->oformat->flags & AVFMT_GLOBALHEADER)
    aenc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

avcodec_open2(aenc, aac_codec, NULL);
avcodec_parameters_from_context(ast->codecpar, aenc);
ast->time_base = aenc->time_base;
```

> [!warning] 音视频时间基不同
> 视频常用 `1/90000`、`1/1000` 或 `1/fps`，音频常用 `1/sample_rate`。写包前分别用各自的编码器时间基 rescale 到各自的流时间基，**不能混用**。

---

## 七、取流/推流专题（重点）

### 7.1 协议与常用选项

| 场景 | 协议 | 常用选项 | 说明 |
| --- | --- | --- | --- |
| 摄像头/直播取流 | RTSP | `rtsp_transport=tcp`、`timeout`、`buffer_size`、`max_delay`、`reorder_queue_size` | UDP 丢包时改 TCP；`timeout` 单位微秒 |
| 直播拉流 | RTMP | `rtmp_live=1`、`rtmp_buffer`、`rw_timeout` | `timeout` 是监听超时，不是读写超时 |
| HTTP-FLV/HLS | HTTP | `reconnect=1`、`reconnect_streamed=1`、`reconnect_delay_max`、`headers`、`user_agent` | 断线自动重连是协议层能力 |
| UDP 组播/单播 | UDP | `buffer_size`、`fifo_size`、`overrun_nonfatal=1`、`pkt_size`、`localport` | 允许丢包时用 `overrun_nonfatal` |
| TCP 裸流 | TCP | `listen`、`timeout`、`tcp_nodelay` | `timeout` 单位微秒 |
| 低延迟通用 | 任意 | `fflags=nobuffer`、`probesize`、`analyzeduration`、`max_delay`；解码器另设 `AV_CODEC_FLAG_LOW_DELAY` | 用启动速度和抗抖动换延迟 |

```c
/* RTSP over TCP + 5 秒超时 + 低延迟 */
AVDictionary *opts = NULL;
av_dict_set(&opts, "rtsp_transport", "tcp", 0);
av_dict_set(&opts, "timeout", "5000000", 0);
av_dict_set(&opts, "rw_timeout", "5000000", 0);
av_dict_set(&opts, "fflags", "nobuffer", 0);
av_dict_set(&opts, "max_delay", "200000", 0);
av_dict_set(&opts, "probesize", "500000", 0);
av_dict_set(&opts, "analyzeduration", "1000000", 0);
```

> [!note] RTSP 的 `timeout` 与旧版 `stimeout`
> FFmpeg 7.1 的 RTSP demuxer 使用 `timeout`（微秒）。一些旧代码/旧版本使用 `stimeout`；升级后如果超时选项失效，先检查版本和选项名。

### 7.2 超时与中断：`AVIOInterruptCB`

网络流的 `av_read_frame()`、`avformat_open_input()`、`av_interleaved_write_frame()` 都可能长时间阻塞。仅靠协议超时不够，**必须挂中断回调**。

```c
typedef struct App {
    volatile int abort;       /* 简化示例；多线程生产代码请用原子变量 */
    int64_t deadline_us;      /* 绝对截止时间，0 表示不限制 */
} App;

/* 返回 1 表示中断当前阻塞操作；必须快速、线程安全 */
static int interrupt_cb(void *opaque)
{
    App *app = opaque;

    if (app->abort)
        return 1;

    if (app->deadline_us > 0 &&
        av_gettime_relative() > app->deadline_us)
        return 1;

    return 0;
}
```

挂载方式：

```c
/* 输入：必须在 avformat_open_input 之前挂到预分配的上下文上 */
AVFormatContext *fmt = avformat_alloc_context();
fmt->interrupt_callback.callback = interrupt_cb;
fmt->interrupt_callback.opaque   = &app;

/* 输出：avio_open2 时传给协议层 */
avio_open2(&oc->pb, url, AVIO_FLAG_WRITE,
           &oc->interrupt_callback, &opts);
```

每次读之前设置一个截止时间：

```c
app.deadline_us = av_gettime_relative() + 5 * 1000000;  /* 5 秒 */

int ret = av_read_frame(fmt, pkt);
if (ret == AVERROR_EXIT) {
    /* 被 interrupt_cb 中断：按超时处理，通常触发重连 */
}
```

> [!warning] 中断回调的注意事项
> - 回调可能在 IO 线程里调用，必须线程安全，不能做耗时操作。
> - 不要在回调里 `av_log`、加锁、分配内存。
> - `deadline_us` 的读写要么用原子变量，要么保证只在单线程写、回调只读。

### 7.3 拉流解码完整示例：RTSP → 解码 → 回调

```c
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
#include <libavutil/error.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>

typedef struct App {
    volatile int abort;
    int64_t deadline_us;
    int64_t frame_count;
} App;

typedef struct Puller {
    App *app;
    AVFormatContext *fmt;
    AVCodecContext *dec;
    int video_index;
    AVPacket *pkt;
    AVFrame *frame;
} Puller;

static App g_app;

static void on_sigint(int sig)
{
    (void)sig;
    g_app.abort = 1;
}

static int interrupt_cb(void *opaque)
{
    App *app = opaque;
    if (app->abort)
        return 1;
    if (app->deadline_us > 0 && av_gettime_relative() > app->deadline_us)
        return 1;
    return 0;
}

static void print_err(const char *what, int err)
{
    char buf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(err, buf, sizeof(buf));
    fprintf(stderr, "%s: %s\n", what, buf);
}

/* 打开输入 + 找流 + 打开解码器 */
static int open_input(Puller *p, const char *url)
{
    AVDictionary *opts = NULL;
    const AVCodec *codec = NULL;
    int ret;

    p->fmt = avformat_alloc_context();
    if (!p->fmt)
        return AVERROR(ENOMEM);

    p->fmt->interrupt_callback.callback = interrupt_cb;
    p->fmt->interrupt_callback.opaque   = p->app;

    /* RTSP over TCP，避免 UDP 丢包 */
    av_dict_set(&opts, "rtsp_transport", "tcp", 0);
    av_dict_set(&opts, "timeout", "5000000", 0);      /* RTSP demuxer，微秒 */
    av_dict_set(&opts, "rw_timeout", "5000000", 0);   /* 底层 IO，微秒 */

    /* 低延迟 */
    av_dict_set(&opts, "fflags", "nobuffer", 0);
    av_dict_set(&opts, "max_delay", "200000", 0);
    av_dict_set(&opts, "probesize", "500000", 0);
    av_dict_set(&opts, "analyzeduration", "1000000", 0);

    p->app->deadline_us = av_gettime_relative() + 10 * 1000000;

    ret = avformat_open_input(&p->fmt, url, NULL, &opts);
    av_dict_free(&opts);
    if (ret < 0) {
        print_err("avformat_open_input", ret);
        return ret;                 /* 失败时 fmt 已被置 NULL */
    }

    /* 限制探测时间，避免首帧太慢 */
    p->fmt->probesize            = 500000;
    p->fmt->max_analyze_duration = AV_TIME_BASE;   /* 1 秒 */

    p->app->deadline_us = av_gettime_relative() + 5 * 1000000;
    ret = avformat_find_stream_info(p->fmt, NULL);
    if (ret < 0) {
        print_err("avformat_find_stream_info", ret);
        return ret;
    }

    p->video_index = av_find_best_stream(p->fmt, AVMEDIA_TYPE_VIDEO,
                                         -1, -1, &codec, 0);
    if (p->video_index < 0) {
        print_err("av_find_best_stream", p->video_index);
        return p->video_index;
    }

    if (!codec)
        codec = avcodec_find_decoder(
                p->fmt->streams[p->video_index]->codecpar->codec_id);
    if (!codec) {
        fprintf(stderr, "decoder not found\n");
        return AVERROR_DECODER_NOT_FOUND;
    }

    p->dec = avcodec_alloc_context3(codec);
    if (!p->dec)
        return AVERROR(ENOMEM);

    ret = avcodec_parameters_to_context(
            p->dec, p->fmt->streams[p->video_index]->codecpar);
    if (ret < 0) {
        print_err("avcodec_parameters_to_context", ret);
        return ret;
    }

    /* 关键：packet/frame 的时间基对齐到 stream time_base */
    p->dec->pkt_timebase = p->fmt->streams[p->video_index]->time_base;

    /* 低延迟解码 */
    p->dec->thread_count = 1;
    p->dec->flags  |= AV_CODEC_FLAG_LOW_DELAY;
    p->dec->flags2 |= AV_CODEC_FLAG2_FAST;

    ret = avcodec_open2(p->dec, codec, NULL);
    if (ret < 0) {
        print_err("avcodec_open2", ret);
        return ret;
    }

    p->pkt   = av_packet_alloc();
    p->frame = av_frame_alloc();
    if (!p->pkt || !p->frame)
        return AVERROR(ENOMEM);

    return 0;
}

/* 关闭并清空，保证可重复调用 */
static void close_input(Puller *p)
{
    avcodec_free_context(&p->dec);
    avformat_close_input(&p->fmt);   /* 释放 fmt 和所有流 */
    av_packet_free(&p->pkt);
    av_frame_free(&p->frame);
    p->video_index = -1;
}

/* 解码一个 packet，取出所有可用帧 */
static int decode_packet(Puller *p)
{
    int ret = avcodec_send_packet(p->dec, p->pkt);
    if (ret < 0 && ret != AVERROR(EAGAIN))
        return ret;

    int need_resend = (ret == AVERROR(EAGAIN));

    for (;;) {
        ret = avcodec_receive_frame(p->dec, p->frame);

        if (ret == 0) {
            AVStream *st = p->fmt->streams[p->video_index];
            double pts = -1.0;

            if (p->frame->pts != AV_NOPTS_VALUE)
                pts = p->frame->pts * av_q2d(st->time_base);

            printf("[pull] frame #%lld %dx%d fmt=%d pts=%.3f key=%d\n",
                   (long long)p->app->frame_count++,
                   p->frame->width, p->frame->height,
                   p->frame->format, pts,
                   !!(p->frame->flags & AV_FRAME_FLAG_KEY));

            av_frame_unref(p->frame);
            continue;
        }

        if (ret == AVERROR_EOF)
            return 0;

        if (ret != AVERROR(EAGAIN))
            return ret;

        if (!need_resend)
            return 0;

        ret = avcodec_send_packet(p->dec, p->pkt);
        if (ret == 0) {
            need_resend = 0;
            continue;
        }
        if (ret == AVERROR(EAGAIN))
            continue;
        return ret;
    }
}

/* 读包 → 解码，直到 EOF 或错误 */
static int run(Puller *p)
{
    int ret;

    while (!p->app->abort) {
        /* 每次读设置 5 秒超时，防止网络卡死 */
        p->app->deadline_us = av_gettime_relative() + 5 * 1000000;

        ret = av_read_frame(p->fmt, p->pkt);

        if (ret == AVERROR_EOF)
            break;

        if (ret < 0) {
            if (ret != AVERROR_EXIT)
                print_err("av_read_frame", ret);
            return ret;
        }

        if (p->pkt->stream_index == p->video_index) {
            ret = decode_packet(p);
            if (ret < 0) {
                print_err("decode_packet", ret);
                av_packet_unref(p->pkt);
                return ret;
            }
        }

        av_packet_unref(p->pkt);
    }

    /* 正常结束或主动停止：flush 解码器 */
    avcodec_send_packet(p->dec, NULL);
    for (;;) {
        ret = avcodec_receive_frame(p->dec, p->frame);
        if (ret < 0)
            break;

        AVStream *st = p->fmt->streams[p->video_index];
        double pts = -1.0;
        if (p->frame->pts != AV_NOPTS_VALUE)
            pts = p->frame->pts * av_q2d(st->time_base);

        printf("[pull] flush frame #%lld pts=%.3f\n",
               (long long)p->app->frame_count++, pts);
        av_frame_unref(p->frame);
    }

    return 0;
}

int main(int argc, char **argv)
{
    const char *url = (argc > 1) ? argv[1]
                                 : "rtsp://127.0.0.1:8554/live";
    Puller p = { .app = &g_app, .video_index = -1 };
    int backoff = 1;

    signal(SIGINT, on_sigint);

    /* 外层重连循环 */
    while (!g_app.abort) {
        int ret = open_input(&p, url);

        if (ret >= 0) {
            ret = run(&p);
            if (ret == AVERROR_EXIT && g_app.abort)
                break;
        }

        close_input(&p);

        if (g_app.abort)
            break;

        fprintf(stderr, "reconnect in %d s...\n", backoff);
        for (int i = 0; i < backoff && !g_app.abort; i++)
            av_usleep(1000000);     /* 1 秒 */

        /* 指数退避，最大 30 秒 */
        backoff = (backoff < 30) ? backoff * 2 : 30;
    }

    close_input(&p);
    return 0;
}
```

### 7.4 断线重连与时间戳连续性

拉流重连的标准步骤：

1. **关闭旧输入**：`avformat_close_input`。
2. **重建输入/解码器**：如果 SPS/PPS 可能变化，直接重建解码器最稳。
3. **等待关键帧**：重连后的前几个包可能不是关键帧，先丢弃，直到 `pkt->flags & AV_PKT_FLAG_KEY`（或解码后 `frame->flags & AV_FRAME_FLAG_KEY`）。
4. **处理时间戳跳变**：新流可能从 0 开始。维护一个 `ts_offset`，把新时间戳平移到旧时间轴之后。
5. **更新统计/告警**：记录重连次数、丢包、首帧耗时。

时间戳平移示意：

```c
/* last_out_pts 是上一次输出到业务层的时间戳（同一时间基） */
int64_t new_pts = av_rescale_q(pkt->pts, st->time_base, out_tb);

if (!first_after_reconnect) {
    int64_t delta = new_pts - last_out_pts;
    if (delta < 0 || delta > 5 * out_tb.den)  /* 回退或跳太远 */
        ts_offset = last_out_pts + frame_duration - new_pts;
    first_after_reconnect = 0;
}

int64_t adjusted = new_pts + ts_offset;
last_out_pts = adjusted;
```

> [!warning] 不要用 `avcodec_flush_buffers` 代替重建
> 如果编码参数变了（分辨率、profile、extradata），仅 flush 会导致解码器按旧参数解新流，出现花屏/崩溃。重建解码器更安全。

### 7.5 低延迟拉流清单

| 环节 | 做法 | 代价 |
| --- | --- | --- |
| 协议 | RTSP 用 TCP（稳定）或 UDP（低延迟但怕丢包） | TCP 重传会增加延迟 |
| 探测 | 减小 `probesize` / `analyzeduration` | 太小可能识别失败 |
| 缓冲 | `fflags=nobuffer`、`max_delay` 调小 | 抗抖动能力下降 |
| 解码 | `thread_count=1`、`AV_CODEC_FLAG_LOW_DELAY`、`AV_CODEC_FLAG2_FAST` | 吞吐下降 |
| 渲染 | 不额外缓存帧；按 PTS 调度 | 需要自己的抖动缓冲策略 |
| 重连 | 快速失败 + 指数退避；等待关键帧 | 重连期间画面中断 |

### 7.6 推流：连接、重连与背压

推流和拉流最大的区别：**网络写是阻塞的**。如果服务器/网络变慢，`av_interleaved_write_frame` 会把编码线程拖住，导致延迟越来越大。

工程上通常这样做：

1. **编码线程**：持续产生 `AVPacket`，放进有界队列。
2. **发送线程**：从队列取包，`av_interleaved_write_frame`。
3. **队列满**：丢非关键帧（或最旧的包），不要无限增长。
4. **写超时/失败**：关闭 `pb`，重连，重新写头；时间戳保持单调。
5. **重连后**：等待编码器下一个关键帧再恢复发送，避免新连接开头花屏。

重连函数骨架：

```c
static int muxer_connect(AVFormatContext *oc, const char *url)
{
    AVDictionary *opts = NULL;
    int ret;

    if (oc->pb)
        avio_closep(&oc->pb);

    av_dict_set(&opts, "rw_timeout", "5000000", 0);
    ret = avio_open2(&oc->pb, url, AVIO_FLAG_WRITE,
                     &oc->interrupt_callback, &opts);
    av_dict_free(&opts);
    if (ret < 0)
        return ret;

    /* 新连接必须重新写头（FLV/RTMP 会开启新的 publish 会话） */
    ret = avformat_write_header(oc, NULL);
    if (ret < 0) {
        avio_closep(&oc->pb);
        return ret;
    }

    return 0;
}
```

> [!warning] 重写头的兼容性
> 不同 muxer 对「同一 `AVFormatContext` 重复 `avformat_write_header`」的支持程度不同。生产代码更稳妥的做法是：**销毁输出上下文并重建**（保留编码器），用 `codecpar` 里的 extradata 重新写头。

### 7.7 低延迟推流清单

| 环节 | 做法 |
| --- | --- |
| 编码器 | `preset=ultrafast/veryfast`、`tune=zerolatency` |
| B 帧 | `max_b_frames=0` |
| 关键帧 | `gop_size = fps * 1~2`，场景切换可强制 IDR |
| 码率 | CBR：`rc_min_rate = rc_max_rate = bit_rate`，配 `rc_buffer_size` |
| 封装 | FLV/RTMP 或 MPEG-TS/SRT；避免需要 seek 的格式 |
| 发送 | `avio_flush`；有界队列；必要时丢帧 |
| 重连 | 超时 + 退避 + 重写头 + 等关键帧 |

### 7.8 取流/推流故障对照表

| 现象 | 常见原因 | 处理 |
| --- | --- | --- |
| 拉流卡住不返回 | 没有中断回调；网络半开 | 挂 `AVIOInterruptCB`，设 `rw_timeout`/`timeout` |
| 首帧很慢 | `avformat_find_stream_info` 探测太久 | 减小 `probesize`/`analyzeduration` |
| 画面花屏/绿屏 | UDP 丢包；重连后没等关键帧 | 改 TCP；丢弃到关键帧再解码 |
| 音画不同步 | 时间基混用；没有按 PTS 调度 | 检查 rescale；用 `av_compare_ts` |
| 推流播放器打不开 | 缺全局头/extradata | 设置 `AV_CODEC_FLAG_GLOBAL_HEADER` |
| 推流花屏开头 | 新连接从非关键帧开始 | 重连后等 IDR |
| "Non-monotonous DTS" | 时间戳回退/混用时间基 | 加偏移；按 DTS 排序；用交织写入 |
| 推流延迟越来越大 | 网络阻塞导致队列堆积 | 有界队列 + 丢帧 + 独立发送线程 |
| 进程退出时崩溃 | 释放顺序错误/重复释放 | 先 trailer、关 pb，再 free 上下文 |
| 内存持续上涨 | packet/frame 没 unref；上下文没 free | 用 ASan/Valgrind 排查；检查每个循环 |

---

## 八、转码、转封装与自定义 IO

### 8.1 转封装：只换容器，不解码

适用：MP4 → TS、FLV → MP4、调整封装参数。速度快、画质无损。

```c
/* 输入已打开，输出已 alloc_output_context2 + new_stream */
for (int i = 0; i < ifmt->nb_streams; i++) {
    AVStream *ist = ifmt->streams[i];
    AVStream *ost = avformat_new_stream(ofmt, NULL);

    /* 直接复制编码参数，包括 extradata */
    avcodec_parameters_copy(ost->codecpar, ist->codecpar);
    ost->codecpar->codec_tag = 0;
    ost->time_base = ist->time_base;
}

avio_open2(&ofmt->pb, out_url, AVIO_FLAG_WRITE, NULL, NULL);
avformat_write_header(ofmt, NULL);

while (av_read_frame(ifmt, pkt) >= 0) {
    AVStream *ist = ifmt->streams[pkt->stream_index];
    AVStream *ost = ofmt->streams[pkt->stream_index];

    /* 输入流时间基 -> 输出流时间基 */
    av_packet_rescale_ts(pkt, ist->time_base, ost->time_base);
    pkt->stream_index = ost->index;

    /* 接管 pkt 引用；返回后 pkt 为空 */
    av_interleaved_write_frame(ofmt, pkt);
}

av_write_trailer(ofmt);
```

> [!warning] 转封装不是万能的
> 目标容器必须支持源编码。例如 FLV 只支持 H.264/AAC 等少数编码；把 HEVC 直接塞进 FLV 会失败，需要转码或换容器（TS/MP4）。

### 8.2 转码：解码 → 编码

骨架：

```c
/* 输入：open_input + 找流 + 打开解码器 */
/* 输出：alloc_output_context2 + new_stream + 打开编码器 */

while (av_read_frame(ifmt, pkt) >= 0) {
    if (pkt->stream_index == v_idx) {
        /* 1. 解码 */
        avcodec_send_packet(vdec, pkt);
        while (avcodec_receive_frame(vdec, frame) == 0) {
            /* 2. 可选：sws_scale / filter 转成编码器需要的格式 */
            /* 3. 时间基转换：输入流 -> 编码器 */
            frame->pts = av_rescale_q(frame->pts,
                                      ist->time_base,
                                      venc->time_base);

            /* 4. 编码 */
            avcodec_send_frame(venc, frame);
            while (avcodec_receive_packet(venc, opkt) == 0) {
                /* 5. 时间基转换：编码器 -> 输出流 */
                av_packet_rescale_ts(opkt, venc->time_base, ost->time_base);
                opkt->stream_index = ost->index;
                av_interleaved_write_frame(ofmt, opkt);
            }

            av_frame_unref(frame);
        }
    }
    av_packet_unref(pkt);
}

/* flush 解码器、编码器 */
avcodec_send_packet(vdec, NULL);
/* ... 取出剩余帧并编码 ... */
avcodec_send_frame(venc, NULL);
/* ... 取出剩余包并写入 ... */
av_write_trailer(ofmt);
```

转码的完整工程还要处理：

- 音视频两条链路分别 decode/encode。
- 音频重采样（`SwrContext`）。
- 视频缩放/像素格式（`SwsContext` 或 filter）。
- 编码器延迟与 flush。
- 输入/输出时间基不同。
- 可变帧率（VFR）与 `AVFMT_VARIABLE_FPS`。

### 8.3 自定义 AVIO：内存、加密、回调

适用：数据不来自文件/URL，而来自自定义网络库、加密层、内存队列。

```c
typedef struct MyIO {
    uint8_t *data;
    size_t   size;
    size_t   pos;
} MyIO;

/* 读回调：返回读到的字节数；流结束返回 AVERROR_EOF；错误返回负值 */
static int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
    MyIO *io = opaque;
    size_t remain = io->size - io->pos;
    if (remain == 0)
        return AVERROR_EOF;

    int n = (remain < (size_t)buf_size) ? (int)remain : buf_size;
    memcpy(buf, io->data + io->pos, n);
    io->pos += n;
    return n;
}

/* 写回调：返回写入字节数，或负的错误码 */
static int write_packet(void *opaque, const uint8_t *buf, int buf_size)
{
    MyIO *io = opaque;
    /* 这里可以写 socket、加密、入队 ... */
    (void)io;
    return buf_size;
}

/* seek 回调：返回新位置，或 AVERROR(ENOSYS) */
static int64_t seek(void *opaque, int64_t offset, int whence)
{
    MyIO *io = opaque;
    if (whence == AVSEEK_SIZE)
        return io->size;
    if (whence == SEEK_SET)
        io->pos = offset;
    else if (whence == SEEK_CUR)
        io->pos += offset;
    else if (whence == SEEK_END)
        io->pos = io->size + offset;
    else
        return AVERROR(ENOSYS);
    return io->pos;
}

/* 使用 */
MyIO io = { .data = my_data, .size = my_size, .pos = 0 };

uint8_t *iobuf = av_malloc(4096);
AVIOContext *avio = avio_alloc_context(iobuf, 4096, 0, &io,
                                       read_packet, NULL, seek);

AVFormatContext *fmt = avformat_alloc_context();
fmt->pb = avio;
fmt->flags |= AVFMT_FLAG_CUSTOM_IO;   /* 告诉 libavformat 不要关闭自定义 IO */

avformat_open_input(&fmt, NULL, NULL, NULL);

/* ... 正常 demux ... */

/* 清理：自定义 IO 的 buffer 由调用者释放 */
avformat_close_input(&fmt);
av_freep(&avio->buffer);
avio_context_free(&avio);
```

> [!warning] 自定义 IO 的释放顺序
> `avformat_close_input()` 不会关闭/释放你挂上去的 `AVIOContext`（`AVFMT_FLAG_CUSTOM_IO` 的作用）。必须自己：
> `av_freep(&avio->buffer); avio_context_free(&avio);`
> 不要先 `avio_context_free` 再访问 `avio->buffer`。

---

## 九、内存管理与引用计数（重点）

### 9.1 FFmpeg 自己的内存分配器

| API | 用途 |
| --- | --- |
| `av_malloc(size)` | 分配对齐内存 |
| `av_mallocz(size)` | 分配并清零 |
| `av_realloc(ptr, size)` | 重新分配 |
| `av_free(ptr)` | 释放 |
| `av_freep(&ptr)` | 释放并把指针置 `NULL` |
| `av_strdup(s)` | 复制字符串 |

规则：

- **`av_malloc` 分配的内存只能用 `av_free` 释放**，不要用 `free()`。
- `av_freep` 比 `av_free` 安全：避免悬垂指针和重复释放。
- 编码/解码器要求 packet 数据末尾有 `AV_INPUT_BUFFER_PADDING_SIZE` 的填充；用 `av_packet_alloc` / `av_new_packet` / `av_packet_from_data` 分配时已经考虑。

```c
uint8_t *buf = av_malloc(1024);
if (!buf)
    return AVERROR(ENOMEM);

/* 使用 ... */

av_freep(&buf);    /* 释放并置 NULL */
```

### 9.2 `AVBufferRef`：引用计数的核心

```c
typedef struct AVBufferRef {
    AVBuffer *buffer;   /* 真正的缓冲区，内含原子引用计数 */
    uint8_t  *data;     /* 指向 buffer->data，可能带偏移 */
    int       size;     /* 可用字节数 */
} AVBufferRef;
```

- `AVBuffer` 内部有原子引用计数和 free 回调。
- 每调用一次 `av_buffer_ref()`，计数 +1。
- 每调用一次 `av_buffer_unref()`，计数 -1；减到 0 时调用 free 回调。
- `AVBufferRef` 本身是「一个引用」，`data` 可能指向 buffer 内部偏移位置。

```c
/* 1. 创建：free 回调负责释放 data；这里用默认的 av_free */
uint8_t *data = av_malloc(1024);
AVBufferRef *ref1 = av_buffer_create(data, 1024,
                                     av_buffer_default_free, NULL, 0);
if (!ref1) {
    av_free(data);
    return AVERROR(ENOMEM);
}

/* 2. 增加引用：refcount = 2 */
AVBufferRef *ref2 = av_buffer_ref(ref1);
if (!ref2) {
    av_buffer_unref(&ref1);
    return AVERROR(ENOMEM);
}

/* 3. 释放引用：refcount 2 -> 1，data 仍然有效 */
av_buffer_unref(&ref2);     /* ref2 被置 NULL */

/* 4. 最后一个引用：refcount 1 -> 0，调用 av_buffer_default_free(data) */
av_buffer_unref(&ref1);
```

常用变体：

```c
AVBufferRef *buf = av_buffer_alloc(1024);          /* 分配 + 引用计数 1 */
AVBufferRef *buf2 = av_buffer_allocz(1024);        /* 清零 */
int writable = av_buffer_is_writable(buf);         /* 是否只有当前引用 */
av_buffer_make_writable(&buf);                     /* 共享时复制一份 */
av_buffer_realloc(&buf, 2048);                     /* 重新分配 */
```

### 9.3 `AVFrame` 的引用计数

`AVFrame` 自身是普通结构体，但它的像素/采样数据通过 `AVBufferRef` 管理：

- `frame->buf[i]`：每个平面的 buffer 引用。
- `frame->extended_buf` / `extended_data`：平面数超过 `AV_NUM_DATA_POINTERS` 时使用。
- `frame->data[i]`：指向 `buf[i]->data + 偏移`。

```c
/* 1. 分配 frame 结构体（不分配数据） */
AVFrame *a = av_frame_alloc();

/* 2. 设置格式和尺寸，再分配数据 */
a->format = AV_PIX_FMT_YUV420P;
a->width  = 1280;
a->height = 720;
av_frame_get_buffer(a, 0);     /* 每个平面一个 AVBufferRef */

/* 3. 浅拷贝：b 和 a 共享底层 buffer，引用计数 +1 */
AVFrame *b = av_frame_alloc();
av_frame_ref(b, a);            /* 要求 b 已 unref 或刚 alloc */

/* 4. 释放 a 的引用，b 仍然有效 */
av_frame_unref(a);

/* 5. 想修改 b 的数据：确保可写（共享时会自动拷贝） */
av_frame_make_writable(b);

/* 6. 最后一个引用释放 */
av_frame_free(&b);
```

关键 API：

| API | 作用 | 注意 |
| --- | --- | --- |
| `av_frame_alloc` | 分配结构体 | 不分配数据 |
| `av_frame_get_buffer` | 分配数据 buffer | frame 已有数据时再调用会泄漏 |
| `av_frame_ref(dst, src)` | 浅拷贝并增加 buffer 引用 | `dst` 必须先 unref 或新 alloc |
| `av_frame_unref` | 释放 buffer 引用并重置字段 | 结构体仍可复用 |
| `av_frame_move_ref(dst, src)` | 转移引用，src 重置 | `dst` 不会被自动 unref，先手动 unref |
| `av_frame_clone(src)` | alloc + ref | 返回新 frame，需 free |
| `av_frame_make_writable` | 确保数据可写 | 共享时复制，可能失败 |
| `av_frame_is_writable` | 判断是否唯一引用 | 返回 1 表示可安全修改 |
| `av_frame_copy(dst, src)` | 拷贝像素/采样数据 | dst 必须已分配且格式相同 |
| `av_frame_copy_props(dst, src)` | 拷贝 pts、metadata、side data | 不拷贝数据布局 |

> [!important] 解码器输出帧是引用计数的
> `avcodec_receive_frame()` 返回的 frame 由解码器分配，调用者持有引用。处理完必须 `av_frame_unref()`；如果要在回调之外长期保存，用 `av_frame_ref()` 增加引用。

### 9.4 `AVPacket` 的引用计数

`AVPacket` 通常只有一个 `buf`，`data` 指向 `buf->data + offset`。

```c
/* 1. 分配结构体 */
AVPacket *p1 = av_packet_alloc();

/* 2. 从解码器/解封装器得到数据（例如 av_read_frame） */
/* p1 现在持有 buf 引用 */

/* 3. 浅拷贝：p2 和 p1 共享 data，引用计数 +1 */
AVPacket *p2 = av_packet_alloc();
av_packet_ref(p2, p1);

/* 4. 释放 p1 的数据引用；p2 仍然有效 */
av_packet_unref(p1);

/* 5. 修改 p2 前确保可写 */
av_packet_make_writable(p2);

/* 6. 释放 */
av_packet_free(&p2);
```

关键 API：

| API | 作用 | 注意 |
| --- | --- | --- |
| `av_packet_alloc` | 分配空 packet | 数据为空 |
| `av_packet_unref` | 释放数据引用并重置字段 | packet 结构体可复用 |
| `av_packet_ref(dst, src)` | 浅拷贝并增加引用 | 两者共享 data |
| `av_packet_move_ref(dst, src)` | 转移引用 | src 被重置 |
| `av_packet_clone(src)` | alloc + ref | 返回新 packet，需 free |
| `av_packet_make_writable` | 确保可写 | 共享时复制 |
| `av_packet_from_data(pkt, data, size)` | 用已有内存构造 packet | **接管 data 所有权**；data 必须带 padding |
| `av_new_packet(pkt, size)` | 分配 size 字节数据 | 自动加 padding |
| `av_grow_packet` / `av_shrink_packet` | 调整大小 | 可能重新分配 |

> [!warning] `av_packet_from_data` 的所有权
> 它接管 `data`，之后由 `av_packet_unref` 释放。`data` 必须是用 `av_malloc` 分配、并且末尾有 `AV_INPUT_BUFFER_PADDING_SIZE` 填充的缓冲区；不要传栈内存或 `malloc` 内存。

### 9.5 谁拥有什么：API 所有权速查

| API | 输入所有权 | 输出/返回 | 调用者要做什么 |
| --- | --- | --- | --- |
| `av_read_frame(fmt, pkt)` | `pkt` 由调用者提供 | 返回引用计数 packet | `av_packet_unref(pkt)` |
| `avcodec_send_packet(dec, pkt)` | **调用者保留**；解码器可能内部引用 | 0/EAGAIN/EOF | send 后可 `unref`；不要再改 `pkt->data` |
| `avcodec_receive_frame(dec, frame)` | `frame` 由调用者提供 | 引用计数 frame；函数先 unref 旧内容 | 处理完 `av_frame_unref(frame)` |
| `avcodec_send_frame(enc, frame)` | **调用者保留**；编码器可能内部引用 | 0/EAGAIN/EOF | send 后可 `unref`；不要再改像素数据 |
| `avcodec_receive_packet(enc, pkt)` | `pkt` 由调用者提供 | 引用计数 packet；函数先 unref 旧内容 | 处理完 `av_packet_unref(pkt)` |
| `av_interleaved_write_frame(oc, pkt)` | **接管引用** | 0/负值 | 返回后 `pkt` 为空；不要再 unref/访问 data |
| `av_write_frame(oc, pkt)` | **不接管** | 0/负值 | 仍需 `av_packet_unref(pkt)` |
| `av_frame_get_buffer(frame, 0)` | `frame` 由调用者提供 | 分配 buffer | `av_frame_unref` 或 `av_frame_free` |
| `av_frame_ref(dst, src)` | `src` 不变 | `dst` 增加引用 | `dst` 必须为空/已 unref；两者都要 unref |
| `av_frame_move_ref(dst, src)` | 转移 | `src` 重置 | 先 `av_frame_unref(dst)` |
| `av_packet_ref(dst, src)` | `src` 不变 | `dst` 增加引用 | 两者都要 unref |
| `av_packet_move_ref(dst, src)` | 转移 | `src` 重置 | 先 `av_packet_unref(dst)` |
| `avcodec_parameters_copy(dst, src)` | `src` 不变 | `dst` 复制 extradata | 随参数/上下文释放，不要手动 free |
| `avformat_open_input` | 预分配 ctx 会被接管 | 成功返回 ctx | `avformat_close_input` |
| `avformat_alloc_output_context2` | — | 新 ctx | `avformat_free_context` |
| `avio_open2` | — | 新 `AVIOContext` | `avio_closep` |
| `av_dict_set` | — | 字典持有字符串 | `av_dict_free` |

### 9.6 自定义 `get_buffer2` 与 buffer pool

默认情况下，解码器通过 `avcodec_default_get_buffer2()` 用 `av_buffer_alloc` 分配帧，已经带引用计数。只有做零拷贝、GPU 帧、内存池时才需要自定义：

```c
/* 最小自定义：完全委托默认实现 */
static int my_get_buffer2(AVCodecContext *avctx, AVFrame *frame, int flags)
{
    return avcodec_default_get_buffer2(avctx, frame, flags);
}

/* 挂到解码器上（在 avcodec_open2 之前设置） */
dec->get_buffer2 = my_get_buffer2;
```

如果要做池化，核心是：

- 为每个平面从 `AVBufferPool` 取 `AVBufferRef`。
- 填 `frame->buf[i]`、`frame->data[i]`、`frame->linesize[i]`。
- 确保返回的 buffer 是引用计数的；解码器释放 frame 时，buffer 自动归还池。

```c
/* 创建一个固定大小的 buffer 池 */
AVBufferPool *pool = av_buffer_pool_init(1920 * 1080 * 3 / 2, NULL);
if (!pool)
    return AVERROR(ENOMEM);

/* 取一个引用 */
AVBufferRef *buf = av_buffer_pool_get(pool);
if (!buf) {
    av_buffer_pool_uninit(&pool);
    return AVERROR(ENOMEM);
}

/* 使用 ... */

/* 归还：最后一个引用释放时自动回池 */
av_buffer_unref(&buf);

/* 不再需要池时标记销毁；等所有 buffer 归还后自动释放 */
av_buffer_pool_uninit(&pool);
```

> [!note] 大多数应用不需要自定义 `get_buffer2`
> 默认实现已经足够快，且正确处理对齐、padding、平面布局。自己实现容易踩「linesize 不对」「buffer 不可写」「忘记 extended_buf」的坑。

### 9.7 常见泄漏与悬垂指针

| 反模式 | 后果 | 正确做法 |
| --- | --- | --- |
| 循环里 `av_read_frame` 后不 `unref` | 内存持续上涨 | 每次读完 `av_packet_unref` |
| `av_frame_get_buffer` 重复调用 | 旧 buffer 泄漏 | 先 `av_frame_unref` |
| `av_frame_ref` 前不 unref dst | 旧引用泄漏 / UB | 先 `av_frame_unref(dst)` |
| `av_frame_move_ref` 前不 unref dst | 旧引用泄漏 | 先 `av_frame_unref(dst)` |
| 保存 `pkt->data` 后 `unref` | 悬垂指针 | 需要长期保存就 `av_packet_ref` |
| `av_interleaved_write_frame` 后继续用 pkt | 访问已释放/空包 | 返回后视为已消费 |
| `av_interleaved_write_frame` 后再 `unref` | 通常无害但是错误习惯 | 不要 unref |
| `avformat_close_input` 后又 `avformat_free_context` | 重复释放 | 输入只用 close |
| 输出只 `avformat_free_context` 不关 pb | IO 泄漏/数据未 flush | 先 `avio_closep` |
| 自定义 IO 只 `avio_context_free` | buffer 泄漏 | 先 `av_freep(&avio->buffer)` |
| `free()` 释放 `av_malloc` 内存 | 堆损坏 | 用 `av_free` |
| 手动 free `codecpar->extradata` | 重复释放 | 随上下文释放 |
| 多线程同时操作同一个 `AVCodecContext` | 崩溃/花屏 | 一个上下文一个线程 |
| 共享 frame 不 `make_writable` 就改 | 破坏其他使用者 | 先 `av_frame_make_writable` |

---

## 十、API 速查表

### 10.1 解封装

| API | 作用 | 最小用法 |
| --- | --- | --- |
| `avformat_alloc_context` | 分配输入上下文 | `AVFormatContext *fmt = avformat_alloc_context();` |
| `avformat_open_input` | 打开输入 | `avformat_open_input(&fmt, url, NULL, &opts);` |
| `avformat_find_stream_info` | 探测流信息 | `avformat_find_stream_info(fmt, NULL);` |
| `av_find_best_stream` | 找最佳流/解码器 | `av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);` |
| `av_read_frame` | 读一个压缩包 | `av_read_frame(fmt, pkt);` |
| `av_seek_frame` | 跳转 | `av_seek_frame(fmt, -1, ts, AVSEEK_FLAG_BACKWARD);` |
| `avformat_flush` | 清空解封装缓冲 | `avformat_flush(fmt);` |
| `avformat_close_input` | 关闭输入并释放 | `avformat_close_input(&fmt);` |
| `av_dump_format` | 打印流信息 | `av_dump_format(fmt, 0, url, 0);` |

### 10.2 解码

| API | 作用 | 最小用法 |
| --- | --- | --- |
| `avcodec_find_decoder` | 按 codec_id 找解码器 | `avcodec_find_decoder(codec_id);` |
| `avcodec_find_decoder_by_name` | 按名字找解码器 | `avcodec_find_decoder_by_name("h264");` |
| `avcodec_alloc_context3` | 分配解码上下文 | `avcodec_alloc_context3(decoder);` |
| `avcodec_parameters_to_context` | 复制流参数到上下文 | `avcodec_parameters_to_context(dec, st->codecpar);` |
| `avcodec_open2` | 打开解码器 | `avcodec_open2(dec, decoder, &opts);` |
| `avcodec_send_packet` | 送压缩包 | `avcodec_send_packet(dec, pkt);` |
| `avcodec_receive_frame` | 取解码帧 | `avcodec_receive_frame(dec, frame);` |
| `avcodec_flush_buffers` | 清空解码器状态 | `avcodec_flush_buffers(dec);` |
| `avcodec_free_context` | 释放解码上下文 | `avcodec_free_context(&dec);` |

### 10.3 编码

| API | 作用 | 最小用法 |
| --- | --- | --- |
| `avcodec_find_encoder` | 按 codec_id 找编码器 | `avcodec_find_encoder(AV_CODEC_ID_H264);` |
| `avcodec_find_encoder_by_name` | 按名字找编码器 | `avcodec_find_encoder_by_name("libx264");` |
| `avcodec_alloc_context3` | 分配编码上下文 | `avcodec_alloc_context3(codec);` |
| `avcodec_parameters_from_context` | 编码器参数复制到流 | `avcodec_parameters_from_context(st->codecpar, enc);` |
| `avcodec_open2` | 打开编码器 | `avcodec_open2(enc, codec, &opts);` |
| `avcodec_send_frame` | 送原始帧 | `avcodec_send_frame(enc, frame);` |
| `avcodec_receive_packet` | 取编码包 | `avcodec_receive_packet(enc, pkt);` |
| `avcodec_free_context` | 释放编码上下文 | `avcodec_free_context(&enc);` |

### 10.4 封装/推流

| API | 作用 | 最小用法 |
| --- | --- | --- |
| `avformat_alloc_output_context2` | 创建输出上下文 | `avformat_alloc_output_context2(&oc, NULL, "flv", url);` |
| `avformat_new_stream` | 新建输出流 | `avformat_new_stream(oc, NULL);` |
| `avcodec_parameters_copy` | 复制编码参数（转封装） | `avcodec_parameters_copy(ost->codecpar, ist->codecpar);` |
| `avio_open2` | 打开输出 IO | `avio_open2(&oc->pb, url, AVIO_FLAG_WRITE, &cb, &opts);` |
| `avformat_write_header` | 写头 | `avformat_write_header(oc, NULL);` |
| `av_interleaved_write_frame` | 交织写包（接管引用） | `av_interleaved_write_frame(oc, pkt);` |
| `av_write_frame` | 直接写包（不接管） | `av_write_frame(oc, pkt);` |
| `av_write_trailer` | 写尾 | `av_write_trailer(oc);` |
| `avio_flush` | 刷新 IO 缓冲 | `avio_flush(oc->pb);` |
| `avio_closep` | 关闭并释放 IO | `avio_closep(&oc->pb);` |
| `avformat_free_context` | 释放输出上下文 | `avformat_free_context(oc);` |

### 10.5 内存/工具

| API | 作用 | 最小用法 |
| --- | --- | --- |
| `av_packet_alloc` | 分配 packet | `AVPacket *pkt = av_packet_alloc();` |
| `av_packet_free` | 释放 packet | `av_packet_free(&pkt);` |
| `av_packet_unref` | 释放 packet 数据 | `av_packet_unref(pkt);` |
| `av_packet_ref` | 增加 packet 引用 | `av_packet_ref(dst, src);` |
| `av_packet_move_ref` | 转移 packet 引用 | `av_packet_move_ref(dst, src);` |
| `av_packet_clone` | 克隆 packet | `AVPacket *p = av_packet_clone(src);` |
| `av_packet_make_writable` | 确保 packet 可写 | `av_packet_make_writable(pkt);` |
| `av_frame_alloc` | 分配 frame | `AVFrame *f = av_frame_alloc();` |
| `av_frame_free` | 释放 frame | `av_frame_free(&f);` |
| `av_frame_unref` | 释放 frame 数据 | `av_frame_unref(f);` |
| `av_frame_ref` | 增加 frame 引用 | `av_frame_ref(dst, src);` |
| `av_frame_move_ref` | 转移 frame 引用 | `av_frame_move_ref(dst, src);` |
| `av_frame_get_buffer` | 分配帧数据 | `av_frame_get_buffer(f, 0);` |
| `av_frame_make_writable` | 确保帧数据可写 | `av_frame_make_writable(f);` |
| `av_buffer_alloc` | 分配 buffer | `AVBufferRef *b = av_buffer_alloc(1024);` |
| `av_buffer_create` | 包装已有内存 | `av_buffer_create(data, size, free_cb, opaque, 0);` |
| `av_buffer_ref` | 增加 buffer 引用 | `AVBufferRef *b2 = av_buffer_ref(b);` |
| `av_buffer_unref` | 释放 buffer 引用 | `av_buffer_unref(&b);` |
| `av_buffer_pool_init` | 创建 buffer 池 | `AVBufferPool *pool = av_buffer_pool_init(size, NULL);` |
| `av_buffer_pool_get` | 从池取 buffer | `AVBufferRef *b = av_buffer_pool_get(pool);` |
| `av_buffer_pool_uninit` | 销毁池 | `av_buffer_pool_uninit(&pool);` |
| `av_dict_set` | 设置选项 | `av_dict_set(&opts, "preset", "veryfast", 0);` |
| `av_dict_get` | 读取选项 | `av_dict_get(opts, "preset", NULL, 0);` |
| `av_dict_free` | 释放字典 | `av_dict_free(&opts);` |
| `av_opt_set` | 设置对象私有选项 | `av_opt_set(oc->priv_data, "flvflags", "no_duration_filesize", 0);` |
| `avcodec_get_name` | codec_id 转可读名字 | `avcodec_get_name(st->codecpar->codec_id);` |
| `av_strerror` | 错误码转字符串 | `av_strerror(ret, buf, sizeof(buf));` |
| `av_rescale_q` | 时间基转换 | `av_rescale_q(ts, tb1, tb2);` |
| `av_packet_rescale_ts` | packet 时间戳转换 | `av_packet_rescale_ts(pkt, tb1, tb2);` |
| `av_compare_ts` | 比较不同时间基的时间戳 | `av_compare_ts(ts1, tb1, ts2, tb2);` |
| `av_q2d` | 有理数转 double | `av_q2d(time_base);` |
| `av_gettime_relative` | 单调时钟（微秒） | `av_gettime_relative();` |
| `av_usleep` | 睡眠（微秒） | `av_usleep(1000000);` |
| `av_channel_layout_default` | 默认声道布局 | `av_channel_layout_default(&layout, 2);` |

### 10.6 缩放/重采样

| API | 作用 | 最小用法 |
| --- | --- | --- |
| `sws_getContext` | 创建缩放上下文 | `sws_getContext(sw, sh, sf, dw, dh, df, SWS_BILINEAR, NULL, NULL, NULL);` |
| `sws_scale` | 执行缩放/转格式 | `sws_scale(sws, src->data, src->linesize, 0, src->height, dst->data, dst->linesize);` |
| `sws_freeContext` | 释放缩放上下文 | `sws_freeContext(sws);` |
| `swr_alloc_set_opts2` | 创建重采样上下文 | `swr_alloc_set_opts2(&swr, &out_layout, out_fmt, out_rate, &in_layout, in_fmt, in_rate, 0, NULL);` |
| `swr_init` | 初始化重采样 | `swr_init(swr);` |
| `swr_convert` | 执行重采样 | `swr_convert(swr, out, out_count, in, in_count);` |
| `swr_get_out_samples` | 估算输出采样数 | `swr_get_out_samples(swr, in_samples);` |
| `swr_free` | 释放重采样上下文 | `swr_free(&swr);` |

---

## 十一、常见坑与注意事项

### 11.1 时间基与时间戳

- **Packet 用 stream time_base，Frame 送编码器用 encoder time_base**。这是最高频的错误。
- `avformat_write_header()` 可能改写 `AVStream.time_base`，**写头之后再取**它做 rescale。
- 不要假设 `pts == dts`。有 B 帧时两者不同；muxer 要求 DTS 单调。
- 时间戳可能是 `AV_NOPTS_VALUE`，要单独处理。
- 直播重连后时间戳可能回到 0，需要加偏移保持单调。
- RTMP 时间戳是 32 位毫秒，约 49.7 天回绕；超长直播要关注。
- 不要自己用浮点秒去算时间戳，统一用整数 + `av_rescale_q`。

### 11.2 封装/推流

- **FLV/MP4 必须带全局头**：`AV_CODEC_FLAG_GLOBAL_HEADER`，否则播放器拿不到 SPS/PPS/AAC config。
- `av_interleaved_write_frame` 会接管 packet 引用，返回后不要再 unref。
- `av_write_frame` 不接管，需要自己 unref。
- 一个流内 DTS 必须严格递增；多流用交织写入。
- 输出流 `codecpar` 必须在 `write_header` 前填好。
- `codec_tag = 0` 交给 muxer 选，别乱填。
- 目标容器可能不支持源编码（HEVC 进 FLV），转封装会失败。
- `av_write_trailer` 只在 `write_header` 成功后调用。
- 网络推流要 `avio_flush`，否则数据可能停在缓冲区。
- 推流阻塞会反压编码器，最好用独立发送线程 + 有界队列。

### 11.3 解码/编码

- `send` / `receive` 必须循环，不能假设一次一帧。
- `AVERROR(EAGAIN)` 不是错误，是状态；`AVERROR_EOF` 是正常结束。
- 输入结束后必须 flush（`send NULL`），否则延迟帧会丢。
- `avcodec_send_packet` 后不要再修改 `pkt->data`，解码器可能还引用。
- `avcodec_send_frame` 后不要再修改 `frame->data`，编码器可能还引用。
- 送进解码器的 packet 数据末尾要有 `AV_INPUT_BUFFER_PADDING_SIZE` 填充。
- 编码器可能不支持你选的 `pix_fmt` / `sample_fmt`，要检查或转换。
- 音频每帧 `nb_samples` 通常必须等于 `frame_size`（最后一帧除外）。
- 音频 `pts` 以采样数计量，不是毫秒。
- 低延迟场景禁用 B 帧、单线程解码、`tune=zerolatency`。

### 11.4 网络/取流

- 网络阻塞必须靠 `AVIOInterruptCB` + 协议超时，不能只靠 `av_read_frame` 的返回值。
- RTSP UDP 丢包会导致花屏，优先 `rtsp_transport=tcp`。
- `avformat_find_stream_info` 可能很慢，直播要限制探测。
- 重连后等关键帧再解码/发送，否则开头花屏。
- 重连后时间戳要加偏移，保持单调。
- SPS/PPS 变化时重建解码器，不要只 flush。
- `av_read_frame` 返回所有流的包，必须按 `stream_index` 分发。
- 拉流线程退出时，先 `abort` 再 `join`，不要在阻塞读时直接释放上下文。

### 11.5 内存/引用计数

- `av_packet_unref` / `av_frame_unref` 只释放数据，不释放结构体。
- `av_packet_free` / `av_frame_free` 释放结构体，并把指针置 NULL。
- `av_frame_get_buffer` 重复调用会泄漏。
- `av_frame_ref` / `av_frame_move_ref` 对 dst 有前置条件，先 unref。
- `av_interleaved_write_frame` 后 packet 为空，不要再访问。
- 自定义 IO 的 buffer 由调用者释放。
- `avformat_close_input` 释放整个输入上下文，不要再 free。
- 输出上下文先 `avio_closep` 再 `avformat_free_context`。
- 不要用 `free()` 释放 `av_malloc` 的内存。
- 不要手动释放 `AVCodecParameters.extradata`。
- 多线程共享 `AVFrame` 时，修改前 `av_frame_make_writable`。

### 11.6 线程与性能

- `AVFormatContext`、`AVCodecContext` 都不是线程安全的；一个上下文只在一个线程里用。
- `AVBufferRef` 的引用计数是原子的，但 `AVFrame` 的字段不是。
- 解码/编码的 `send`/`receive` 不要并发调用同一个上下文。
- 离线转码可以 `thread_count=0` 自动多线程；直播低延迟用 1。
- 零拷贝优先：直接传递 packet/frame 引用，避免 `memcpy`。
- 高并发推流用 buffer pool，减少 malloc/free。
- 硬件加速：`hw_frames_ctx` / `AV_PIX_FMT_CUDA` 等；需要 `av_hwframe_transfer_data` 在 GPU/CPU 之间搬数据。

---

## 十二、版本迁移：旧教程 API 对照

很多网上教程基于 FFmpeg 2.x/3.x，直接抄会编译失败。常见对照：

| 旧 API | 新 API / 做法 | 说明 |
| --- | --- | --- |
| `av_register_all()` | 删除 | FFmpeg 5.0 起移除，不再需要注册 |
| `avcodec_register_all()` | 删除 | 同上 |
| `avcodec_decode_video2()` | `avcodec_send_packet` + `avcodec_receive_frame` | 旧解码 API 已移除 |
| `avcodec_decode_audio4()` | 同上 | 音频也走 send/receive |
| `avcodec_encode_video2()` | `avcodec_send_frame` + `avcodec_receive_packet` | 旧编码 API 已移除 |
| `avcodec_encode_audio2()` | 同上 | 同上 |
| `AVStream.codec` | `AVStream.codecpar` + `avcodec_parameters_to_context` | FFmpeg 6.0 起移除 |
| `av_init_packet()` | `av_packet_alloc()` | 旧初始化 API 已移除 |
| `avcodec_close()` | `avcodec_free_context()` | 旧 close 已废弃 |
| `avpicture_*` | `av_image_*` | 旧图像 API 已移除 |
| `av_frame_get_best_effort_timestamp()` | `frame->best_effort_timestamp` | 旧访问器已移除，字段仍可用 |
| `channels` / `channel_layout` | `ch_layout`（`AVChannelLayout`） | FFmpeg 5.1 起推荐，旧字段已移除 |
| `swr_alloc_set_opts()` | `swr_alloc_set_opts2()` | 适配 `AVChannelLayout` |
| `avformat_alloc_output_context()` | `avformat_alloc_output_context2()` | 旧函数已移除 |
| `avio_open()` | `avio_open2()` | 需要超时/中断回调时用 `avio_open2` |
| `av_guess_format()` | 仍可用 | 但通常直接用 `avformat_alloc_output_context2` |
| `AVFMT_RAWPICTURE` | 删除 | 旧原始画面封装方式已移除 |
| `avcodec_get_frame_defaults()` | `av_frame_alloc()` | 新帧默认值由 alloc 初始化 |
| `av_frame_get_buffer()` 旧签名 | 新签名相同 | 但音频要求 `ch_layout` 而非 `channel_layout` |

> [!tip] 判断教程是否过时
> 看到 `av_register_all`、`avcodec_decode_video2`、`AVStream->codec`、`av_init_packet`，基本可以判断是 FFmpeg 4.x 以前的写法，不能直接用于 7.x。

---

## 十三、一页纸总结

### 13.1 五条铁律

1. **三层上下文**：`AVFormatContext` 管容器，`AVCodecContext` 管编解码，`AVIOContext` 管字节。
2. **两种数据**：`AVPacket` 是压缩数据，`AVFrame` 是原始数据；两者都引用计数，用完 `unref`。
3. **send/receive 循环**：`EAGAIN` 不是错误，`EOF` 是正常结束；输入结束必须 flush。
4. **时间基必转**：packet 用 stream time_base，frame 用 encoder time_base；写包前 `av_packet_rescale_ts`。
5. **网络必设超时/中断/重连**：`AVIOInterruptCB` + `rw_timeout`，重连后等关键帧、处理时间戳跳变。

### 13.2 取流 10 步

1. `avformat_alloc_context()` + 挂 `interrupt_callback`。
2. 设置协议选项（RTSP TCP、超时、低延迟）。
3. `avformat_open_input()`。
4. 限制 `probesize` / `max_analyze_duration`，`avformat_find_stream_info()`。
5. `av_find_best_stream()` 选流。
6. `avcodec_alloc_context3()` + `avcodec_parameters_to_context()`。
7. 设置 `pkt_timebase`、低延迟参数，`avcodec_open2()`。
8. 循环 `av_read_frame()`，按 `stream_index` 分发。
9. `avcodec_send_packet()` + `avcodec_receive_frame()` 循环。
10. EOF/错误后 flush、关闭、按需重连。

### 13.3 推流 10 步

1. `avformat_alloc_output_context2()`（FLV/MPEG-TS）。
2. `avformat_new_stream()` 建流。
3. 配置编码器（分辨率、帧率、码率、GOP、无 B 帧）。
4. `AV_CODEC_FLAG_GLOBAL_HEADER`（FLV/MP4）。
5. `avcodec_open2()`。
6. `avcodec_parameters_from_context()` 填 `codecpar`。
7. `avio_open2()` 打开网络输出（`rw_timeout`）。
8. `avformat_write_header()`。
9. 循环 `avcodec_send_frame()` + `avcodec_receive_packet()` + `av_packet_rescale_ts()` + `av_interleaved_write_frame()`。
10. flush 编码器、`av_write_trailer()`、`avio_closep()`、释放。

### 13.4 一句话记忆

> **解封装拿 Packet，解码得 Frame；编码送 Frame，封装写 Packet。时间基在边界转，引用计数在循环里放，网络阻塞用回调断。**

---

## 相关笔记

- [[基础概念]] —— ffmpeg 是什么、工具链、处理管线
- [[容器与封装]] —— MP4/FLV/TS、muxer/demuxer、重封装
- [[时间轴与同步]] —— PTS/DTS、time_base、音画同步
- [[命令实操]] —— 先用命令行验证参数，再写代码
- [[易错点与排错]] —— 常见报错与排错顺序
- [[RTSP协议]] —— RTSP 交互与传输细节
- [[音视频/_MOC|音视频 ffmpeg 知识地图]]

---

> [!note] 编译链接
> Linux/macOS 下通常链接：`-lavformat -lavcodec -lavutil -lswscale -lswresample`；Windows/MSVC 下链接对应的 `avformat.lib`、`avcodec.lib`、`avutil.lib`、`swscale.lib`、`swresample.lib`。具体以你的 FFmpeg 构建为准。