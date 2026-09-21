---
title: YOLO 第九课：视频推理与目标跟踪
type: 课程笔记
课程: YOLO 目标检测
课次: 9
created: 2026-09-21
updated: 2026-09-21
tags:
  - yolo
  - 目标检测
  - 深度学习
  - 课程笔记
  - 视频分析
  - 目标跟踪
status: 进行中
---

# 第九课：视频推理与目标跟踪

> [!abstract] 本课目标
> 把图片推理扩展到视频：理解**逐帧检测**的完整流水线，掌握 `stream=True`、`imgsz`、`vid_stride` 等视频推理参数，学会用 `model.track()` + `persist=True` 做目标跟踪，并能区分**整体 FPS** 与**推理 FPS**。
>
> **前置 / 对应实现**：
> - [[YOLO第七课 模型加载与图片推理|第七课]]——`model.predict()` 参数与 `Results` 对象
> - [[YOLO第四课 检测输出、边界框解码与NMS|第四课]]——`conf` 与 NMS `iou` 的分工
> - [[YOLO第八课 训练结果分析,错误诊断与优化|第八课]]——用 Precision / Recall / mAP 判断模型优劣

视频目标检测，本质上就是对视频的**每一帧**连续执行图片目标检测。

完整流水线：

```
视频解码
    ↓
读取视频帧
    ↓
图像预处理
    ↓
YOLO 前向推理
    ↓
置信度过滤和 NMS
    ↓
绘制检测结果
    ↓
视频编码
    ↓
输出检测视频
```

## 1. 视频检测与目标跟踪

### 1.1 视频检测

视频检测对视频中的**每一帧都是独立检测**：帧与帧之间没有关联，同一个目标在不同帧里的框彼此独立，模型并不知道"这一帧的框"和"上一帧的框"是不是同一个目标。

### 1.2 目标跟踪

目标跟踪会给每个目标分配一个唯一编号（`track_id`），然后回答这类问题：

- 哪些框属于同一个目标；
- 每个目标移动到了哪里；
- 某个目标出现了多少帧；
- 有多少个不同的目标；
- 目标是否穿过某条线；
- 目标是否进入或离开某个区域。

> [!tip] 检测 ≠ 跟踪
> **检测**回答"这一帧里有什么、在哪里"；**跟踪**在此之上回答"这一帧的它，和上一帧的它是不是同一个它"。计数、越线、轨迹分析都建立在跟踪之上。

跟踪的基本过程：

```
YOLO 在当前帧检测目标
          ↓
跟踪器读取检测框
          ↓
与上一帧中的目标进行匹配
          ↓
给匹配成功的目标保留原来的 ID
          ↓
给新出现的目标分配新 ID
```

## 2. 视频推理基础

最基本的视频推理：

```python
from ultralytics import YOLO

model = YOLO(
    r"D:\python\YOLO\runs\detect\wildlife_yolo26n_b50\weights\best.pt"
)

results = model.predict(
    source=r"D:\python\YOLO\videos\wildlife.mp4",
    stream=True,
    conf=0.2,
    iou=0.7,
    imgsz=640,
    device=0,
    save=True,
    project=r"D:\python\YOLO\runs\detect",
    name="wildlife_video_stream"
)

for frame_index, result in enumerate(results):
    print(f"正在处理第 {frame_index + 1} 帧")
```

相比图片推理（见 [[YOLO第七课 模型加载与图片推理|第七课]]），`source` 换成视频路径之外，最关键的两个新增项是 **`stream=True`** 和 **`imgsz`** 的取舍。

### 2.1 `source`

输入源可以是：

1. **本地文件**：图片、视频；
2. **摄像头**：

```python
source = 0                       # 第一个摄像头
```

3. **网络视频流**：

```python
source = "rtsp://..."            # RTSP 视频流地址
```

### 2.2 `imgsz`

`imgsz`（推理分辨率）在**实时视频推理**中尤为重要，它直接决定"小目标能不能检出来"和"能不能跑得动"：

| 调整方向 | 小目标检测 | 显存占用 | 推理速度 |
| --- | --- | --- | --- |
| `imgsz` 增大 | 可能更容易检出 | 增加 | 下降 |
| `imgsz` 减小 | 可能变差 | 下降 | 提高 |

> [!note] 与第八课的结论呼应
> 分辨率不是"越大越好"：它会同时改变精度、显存和速度，必须结合部署设备一起权衡。`imgsz` 对模型优劣的影响见 [[YOLO第八课 训练结果分析,错误诊断与优化|第八课]]。

### 2.3 长视频必须使用 `stream=True`

不使用流式方式时，推理结果会作为一个**列表**返回，每一帧的结果都保留在内存里，长视频会造成很大的内存压力。

> [!warning] 开了 `stream=True`，`results` 就变成了生成器
> 此时不能用索引访问（`results[0]` 取不到东西），必须用 `for` 循环逐帧迭代：
>
> ```python
> results = model.predict(source, stream=True, ...)
>
> for result in results:      # 逐帧迭代
>     # 在这里处理每一帧
> ```

## 3. 逐目标读取检测结果

```python
# results 是一个生成器，需要循环读取
for frame_index, result in enumerate(results):
    boxes = result.boxes                          # 获取检测框
    for box in boxes:
        class_id = int(box.cls.item())
        confidence = float(box.conf.item())
        coordinates = box.xyxy[0].tolist()
        class_name = result.names[class_id]
        print(
            f"帧={frame_index + 1}, "
            f"类别={class_name}, "
            f"置信度={confidence:.3f}, "
            f"坐标={coordinates}"
        )
```

## 4. 视频推理速度与实时性

假设原视频为 `30 FPS`（每秒产生 30 帧）：

| 模型速度 | 相对 30 FPS | 结论与对策 |
| --- | --- | --- |
| 60 FPS | 快于视频产生速度 | 可以实现**实时检测** |
| 20 FPS | 慢于视频产生速度 | 无法实时。要么**丢帧**、优先显示最新画面；要么不丢帧、但延迟会越积越高 |
| 10 FPS | 远低于视频产生速度 | 实时性极低，必须优化推理参数或模型 |

当模型只能跑到 `10 FPS` 左右时，可从以下方向优化：

1. 降低分辨率 `imgsz`；
2. 使用更小的模型（如 `yolo26n`）；
3. 跳帧（`vid_stride`）；
4. 使用 GPU 加速；
5. 导出 `ONNX` 或 `TensorRT`；
6. 减少绘制和保存操作。

> [!note] "实时"的两种情况
> - **模型快于源**：处理一帧比源产生一帧更快，可直接实时；若要**按原速播放**，还需按原始帧率主动 `sleep` 限速，否则会"快进"式播放。
> - **模型慢于源**：必然要面对"丢帧（保最新画面）"还是"积压（保完整序列但延迟增大）"的取舍，两者不可兼得。

## 5. 用 `vid_stride` 跳帧

`vid_stride` 表示视频**帧采样步长**：

```python
vid_stride = 2
```

表示每 2 帧处理 1 帧（跳过中间的 1 帧），即只处理第 0、2、4… 帧，从而加快处理速度。

### 适合跳帧的场景

- 目标移动速度较慢；
- 只需要大致统计；
- 视频帧率很高；
- GPU 性能不足；
- 不要求精确捕捉每帧、对连续性要求低的目标。

### 不适合跳帧的场景

- 高速运动目标；
- 目标会快速进入或离开画面；
- 越线计数；
- 短暂出现的目标；
- 对轨迹连续性要求高。

> [!warning] 跳帧会破坏轨迹连续性
> 跳帧直接降低了时间采样率，越线计数、快速目标这类任务一旦跳帧，很容易**漏掉事件或把两个目标接错 ID**。这类任务应优先降低 `imgsz` 或换小模型，而不是跳帧。

## 6. 检测框抖动

视频中检测框抖动（相邻帧的框位置跳来跳去）的常见原因：

- 视频压缩产生的噪声；
- 目标本身正在移动；
- 摄像头抖动；
- 运动模糊；
- 遮挡程度变化。

改善方式：

- 使用**目标跟踪**模式（用 ID 关联前后帧，天然抑制单帧抖动）；
- 对检测框坐标做**时域平滑**（例如指数移动平均 EMA）；
- 提高视频质量；
- 增加相关数据集，例如运动模糊 / 遮挡场景；
- 调整推理分辨率。

> [!tip] 抖动多半是"帧间无关联"造成的
> 逐帧独立检测天然会抖动。**跟踪是更根本的手段**——跟踪器用运动模型预测下一帧位置，再与检测框匹配，可以显著平滑轨迹。

## 7. 用 `model.track` 做目标跟踪

```python
from ultralytics import YOLO

model = YOLO(
    r"D:\python\YOLO\runs\detect\wildlife_yolo26n_b50\weights\best.pt"
)

results = model.track(
    source=r"D:\python\YOLO\videos\wildlife.mp4",
    stream=True,
    conf=0.2,
    iou=0.7,
    imgsz=640,
    device=0,
    tracker="botsort.yaml",          # 设置跟踪器配置等
    save=True,
    project=r"D:\python\YOLO\runs\detect",
    name="wildlife_video_track"
)

for frame_index, result in enumerate(results):
    boxes = result.boxes

    if boxes is not None and boxes.is_track:
        track_ids = boxes.id.int().cpu().tolist()      # 目标跟踪的目标 ID
        class_ids = boxes.cls.int().cpu().tolist()
        confidences = boxes.conf.cpu().tolist()
        coordinates = boxes.xyxy.cpu().tolist()

        for track_id, class_id, confidence, xyxy in zip(
            track_ids,
            class_ids,
            confidences,
            coordinates
        ):
            class_name = result.names[class_id]

            print(
                f"帧={frame_index + 1}, "
                f"ID={track_id}, "
                f"类别={class_name}, "
                f"置信度={confidence:.3f}, "
                f"坐标={xyxy}"
            )
```

### 7.1 `predict` 和 `track` 的区别

| 方法 | 是否可用视频 | 是否分配 ID | 帧间是否关联 |
| --- | --- | --- | --- |
| `model.predict()` | 可以 | 否 | 否，逐帧独立检测 |
| `model.track()` | 可以 | 是（`boxes.id`） | 是，同一目标跨帧保持同一 `track_id` |

> [!warning] 常见误解
> `predict` **不是"只能处理图片"**，它对视频同样适用，只是**不分配 ID、不做帧间关联**；`track` 则会在检测结果上额外给出 `track_id` 用于跟踪。

### 7.2 `persist=True` 保留连续帧状态和 ID

当自己用 OpenCV 读数每一帧、再逐帧交给 YOLO 时（此时是"一张张图"，模型不知道它们属于同一段视频），必须用 `persist=True` 保留跟踪状态：

```python
import cv2
from ultralytics import YOLO

model = YOLO(
    r"D:\python\YOLO\runs\detect\wildlife_yolo26n_b50\weights\best.pt"
)

cap = cv2.VideoCapture(
    r"D:\python\YOLO\videos\wildlife.mp4"
)

while cap.isOpened():
    success, frame = cap.read()                  # 获取每帧

    if not success:
        break

    results = model.track(
        frame,
        persist=True,                            # 保留每帧的状态/目标 ID
        conf=0.2,
        iou=0.7,
        imgsz=640,
        device=0
    )

    annotated_frame = results[0].plot()

    cv2.imshow("YOLO Tracking", annotated_frame)

    if cv2.waitKey(1) & 0xFF == ord("q"):
        break

cap.release()
cv2.destroyAllWindows()
```

> [!warning] `persist=True` 不能跨无关的视频/图片流复用
> 它把"上一帧的跟踪状态"记在模型里，如果紧接着换一段**不相关**的视频或图片流继续推理，上一段的跟踪状态会被错误带入，导致 ID 错乱、跟踪出错。**切换视频时要么重新 `YOLO(...)` 加载模型，要么不要沿用同一份 persist 状态。**

## 8. 一个完整的视频跟踪示例

包含两个视频顺序处理、整体 FPS 与推理 FPS 分别统计、按 `q` 退出、按右方向键跳过当前视频：

```python
from ultralytics import YOLO
import cv2
import time

# 推理参数
conf = 0.1
iou = 0.7
imgsz = 960

video_paths = [
    r"C:\Users\fanyu\Videos\NVIDIA\Desktop\Desktop 2026.09.21 - 15.54.17.02.mp4",
    r"C:\Users\fanyu\Videos\NVIDIA\Desktop\Desktop 2026.09.21 - 15.51.56.01.mp4",
]

Continue = True
alpha = 0.1          # 整体 FPS 的指数移动平均系数
track_alpha = 0.1    # 推理 FPS 的指数移动平均系数

for video_path in video_paths:
    if not Continue:
        break

    cap = cv2.VideoCapture(video_path, cv2.CAP_FFMPEG)
    if not cap.isOpened():
        print("视频打开失败")
        continue

    frame_count = 0

    # 每个视频单独重新加载模型,避免 persist=True 导致不同视频的跟踪状态相互影响
    model = YOLO(r"D:\python\YOLO\runs\detect\wildlife_yolo26n_train_imgsz960\weights\best.pt")

    fps = 0.0
    track_fps = 0.0
    prev_time = time.perf_counter()          # 使用更高精度的计时器

    while True:
        success, frame = cap.read()
        if not success:
            print("\n==============================\n"
                  "{}推理完成, 共完成{}帧视频"
                  "\n==============================\n"
                  .format(video_path, frame_count))
            break

        frame_count += 1

        # 计算整体 FPS(包括读取、解码、推理、跟踪、绘制、显示等)
        current_time = time.perf_counter()
        delta = current_time - prev_time
        if delta > 0:
            instant_fps = 1.0 / delta
            fps = alpha * instant_fps + (1.0 - alpha) * fps if fps > 0.0 else instant_fps
        prev_time = current_time

        track_prev_time = time.perf_counter()      # 模型推理起始时间
        results = model.track(
            frame,
            persist=True,
            conf=conf,
            iou=iou,
            imgsz=imgsz,
            device=0,
            workers=0,
        )
        track_current_time = time.perf_counter()   # 模型推理结束时间

        # 计算模型推理 FPS
        track_delta = track_current_time - track_prev_time
        if track_delta > 0:
            track_instant_fps = 1.0 / track_delta
            track_fps = (
                track_alpha * track_instant_fps + (1.0 - track_alpha) * track_fps
                if track_fps > 0.0
                else track_instant_fps
            )

        annotated_frame = results[0].plot()
        annotated_frame = cv2.resize(src=annotated_frame, dsize=(0, 0), fx=0.5, fy=0.5)

        cv2.putText(annotated_frame, f"OVERALL FPS: {fps:.1f}", (10, 30),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
        cv2.putText(annotated_frame, f"TRACK FPS: {track_fps:.1f}", (10, 60),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)

        cv2.imshow("YOLO Target Tracking...", annotated_frame)

        key = cv2.waitKey(1)
        if key == ord("q"):
            Continue = False
            break
        if key == 2555904:                     # 右方向键 → ：跳过当前视频
            break

    cap.release()

cv2.destroyAllWindows()
```

> [!tip] 两个容易忽略的细节
> - **每段视频都重新 `YOLO(...)` 加载模型**：这是配合 `persist=True` 的必要操作，用来把上一段的跟踪状态彻底清掉。
> - **`if track_fps > 0.0`**：EMA 的第一个采样点应直接用当前值做种子（否则 `fps` 初值 `0.0` 会把第一帧结果按 `alpha` 缩小，导致起始阶段 FPS 虚低）。整体 FPS 与推理 FPS 两处判断必须保持一致。

## 9. 当前实验记录：模型、分辨率与视频推理

### 9.1 实验对象

本次对比了两个基于 African Wildlife 数据集训练的 YOLO 模型：

- **旧模型**：`D:\python\YOLO\runs\detect\wildlife_yolo26n_b50\weights\best.pt`
- **新模型**：`D:\python\YOLO\runs\detect\wildlife_yolo26n_train_imgsz960\weights\best.pt`
- 新模型在旧模型基础上继续训练时**提高了训练分辨率**，仍采用 `batch=16`、`epochs=50`。

对应的验证结果目录：

- 旧模型：`D:\python\YOLO\runs\val\wildlife_yolo26n_b50c0.001`
- 新模型：`D:\python\YOLO\runs\val\wildlife_yolo26n_train_imgsz960c0.001`

### 9.2 视频 FPS 统计改进

本次视频推理程序已完成以下改进：

- 使用 `time.perf_counter()` 进行高精度计时；
- 每个视频开始处理时重新加载模型；
- 模型加载时间不计入 FPS；
- 每个视频单独重置 FPS 统计，避免上一个视频的统计结果影响下一个视频；
- 修正了 FPS 显示偏低的问题；
- 区分平滑后的整体 FPS 与平滑后的推理 FPS / 跟踪 FPS。

两种 FPS 的含义不同：

| 指标 | 包含的环节 | 反映的问题 |
| --- | --- | --- |
| **整体 FPS（OVERALL FPS）** | 视频读取、解码、模型推理、跟踪、绘制、显示以及可能的写盘 | 更接近**程序最终能否实时播放**的速度 |
| **推理 FPS / 跟踪 FPS** | 主要是模型推理与跟踪本身 | **模型本身快不快**，不代表完整视频链路的实时速度 |

因此两者数值不同是正常现象，不能只看其中一个数值判断程序性能。

### 9.3 `imgsz=960` 下的验证结果

| 模型 | Precision | Recall | mAP50 | mAP50-95 |
| --- | ---: | ---: | ---: | ---: |
| 旧模型 | 0.940 | 0.882 | 0.957 | 0.726 |
| 新模型 | 0.935 | 0.934 | 0.957 | 0.794 |

结论：

- 新模型 Recall 从 `0.882` 提升到 `0.934`，**漏检情况明显改善**；
- 新模型 mAP50-95 从 `0.726` 提升到 `0.794`，**严格 IoU 条件下的定位质量明显提高**；
- 新模型 Precision 从 `0.940` 略降到 `0.935`，说明模型可能引入了少量额外候选框，但不代表整体性能变差；
- mAP50 基本相同，说明在较宽松的 IoU = 0.5 条件下两者总体检测能力接近，而新模型在更严格的定位评价下更有优势。

### 9.4 不同分辨率下的模型选择

经过 `imgsz=640` 和 `imgsz=960` 两次测试，得到以下工程结论：

- 在低分辨率 `imgsz=640` 下，**旧模型表现更佳**；
- 在高分辨率 `imgsz=960` 下，**新模型明显强于旧模型**；
- 如果部署设备性能有限、更重视速度，可优先考虑旧模型和较低分辨率；
- 如果目标较小、更重视召回率和定位质量，且设备能承担更高计算量，可以选择新模型和较高分辨率。

> [!warning] 模型优劣必须和推理分辨率一起讨论
> 不能脱离 `imgsz` 单独比较两个模型——同一个模型在不同分辨率下的相对强弱可能完全相反。

### 9.5 关于训练曲线的理解

新模型的 F1 曲线整体峰值更加平缓，`results.png` 中部分验证指标的波动也比旧模型大。这些现象说明训练过程和不同阈值下的性能分布发生了变化，但**不能仅凭曲线外观判断模型一定变差**。

最终应结合以下内容综合判断：

- 同一验证集；
- 相同推理分辨率；
- Precision、Recall、mAP50、mAP50-95；
- 混淆矩阵；
- 实际视频中的漏检、误检、框抖动和跟踪连续性。

本次在 `imgsz=960` 下的终端验证结果已表明，新模型的 Recall 和 mAP50-95 均优于旧模型，因此"高分辨率下新模型更强"是当前正确结论。相关指标的读法与诊断方法见 [[YOLO第八课 训练结果分析,错误诊断与优化|第八课]]。

## 10. 下一节预告

下一节学习：

1. 读取 `result.boxes.id` 和 `boxes.is_track`；
2. 理解 `track_id` 为什么能在连续帧中表示同一个目标；
3. 分析轨迹断裂和 ID Switch；
4. 比较 `ByteTrack` 与 `BoT-SORT` 的基本思路；
5. 在轨迹基础上实现目标计数和越线计数。

> [!note] 本课小结
> - 视频目标检测 = 对每一帧连续做图片检测；**跟踪**在此基础上用 `track_id` 把跨帧的同一目标串起来
> - `stream=True` 让 `results` 变成生成器，长视频必须用 `for` 迭代，避免把全部帧结果堆在内存里
> - `imgsz` 是精度、显存、速度的三角权衡；模型面对低帧率源时只能"丢帧保最新"或"积压保完整"
> - `vid_stride` 跳帧能提速，但会破坏轨迹连续性，越线计数/高速目标不适合跳帧
> - 检测框抖动的根本改善手段是**跟踪**与坐标时域平滑
> - `predict` 不分配 ID、`track` 分配 `track_id`；手动逐帧送图时必须 `persist=True`
> - `persist=True` 的状态**不能跨不相关视频复用**，切换视频要重新加载模型
> - **整体 FPS** 反映端到端能否实时，**推理 FPS** 只反映模型本身速度，两者要分开看
> - 模型强弱离不开推理分辨率：`imgsz=640` 旧模型更佳，`imgsz=960` 新模型更强（Recall `0.934`、mAP50-95 `0.794`）

---

## 导航

- 上一课：[[YOLO第八课 训练结果分析,错误诊断与优化]]
- 下一课：第十课——性能优化与模型部署（待补充）
- 总览：[[视觉学习/YOLO学习/_MOC|YOLO 学习总览]]
- 进度与练习：[[YOLO学习进度]]
