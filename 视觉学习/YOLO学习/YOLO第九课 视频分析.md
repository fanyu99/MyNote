---
title: YOLO 第九课：视频推理与目标跟踪
type: 课程笔记
课程: YOLO 目标检测
课次: 9
created: 2026-09-21
updated: 2026-09-22
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
> 把图片推理扩展到视频：理解**逐帧检测**的完整流水线，掌握 `stream=True`、`imgsz`、`vid_stride` 等视频推理参数，学会用 `model.track()` + `persist=True` 做目标跟踪，能区分**整体 FPS** 与**推理 FPS**，并认识**轨迹断裂 / ID Switch**、两种内置跟踪器（`ByteTrack` / `BoT-SORT`）以及**越线计数**的实现要点。
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

## 9. 轨迹断裂

轨迹断裂指：同一个真实目标在视频中**没有保持同一个 ID**，或者目标暂时消失后重新出现时被分配了新的 ID。

常见原因：

- 目标被其他目标完全遮挡；
- 目标离开画面后重新进入；
- 当前帧检测置信度过低；
- 两个同类目标严重重叠；
- 视频跳帧过多；
- 摄像机快速移动；
- 视频画面模糊；
- 目标外观非常相似；
- `conf` 设置太高；
- 检测框位置抖动严重；
- 跟踪缓冲时间不足。

> [!tip] 轨迹断裂的直接后果
> 目标计数、越线统计、停留时间都建立在 `track_id` 的连续性上。一次断裂就可能把一个目标算成两个，所以这类任务的精度**首先取决于跟踪是否稳定**。

## 10. ID Switch

ID Switch 比普通的轨迹断裂**更严重**：轨迹断裂是"目标暂时丢失、换了新号"，ID Switch 则是两个目标把身份**互相认错**。

现象：

```
真实目标 A：红色标记
真实目标 B：蓝色标记

原本识别为：
A → ID 1
B → ID 2

遮挡后误判为：
A → ID 2
B → ID 1
```

ID Switch 会直接影响：
- 目标计数；
- 每个目标的运动轨迹；
- 进入 / 离开统计；
- 停留时间统计；
- 速度估计；
- 行为分析。

> [!warning] 为什么 ID Switch 比轨迹断裂更致命
> 轨迹断裂只是"多出几个 ID"，而 ID Switch 是**两条轨迹被整体交换**：计数、轨迹、速度全部跟着错，事后还不容易从结果里察觉。

## 11. `tracker`：ByteTrack 与 BoT-SORT

YOLO 跟踪时 `tracker` 参数用于指定跟踪器配置，Ultralytics 内置 `bytetrack.yaml` 与 `botsort.yaml` 两种。

### 11.1 ByteTrack 基本思路

不只使用高置信度检测框，还会尝试利用**低置信度检测框**来恢复轨迹：有些低置信度框可能确实是真实目标，ByteTrack 会拿它们与已有轨迹再匹配一次，从而减少目标暂时消失导致的轨迹断裂。

```python
results = model.track(
    frame,
    persist=True,
    tracker="bytetrack.yaml"
)
```

> [!important] ByteTrack 一句话理解
> 先用高置信度框做正常匹配，**再用低置信度框去补救已有轨迹**。

### 11.2 BoT-SORT 基本思路

BoT-SORT 在目标运动信息的基础上，还可以结合：

- 摄像机运动补偿；
- 目标外观特征；
- ReID；
- 卡尔曼滤波；
- 检测框匹配。

它更适合：

- 摄像机移动；
- 目标较多；
- 目标相互遮挡；
- 多个目标外观相似；
- 更关注 ID 稳定性的场景。

```python
results = model.track(
    frame,
    persist=True,
    tracker="botsort.yaml"
)
```

> [!important] BoT-SORT 一句话理解
> BoT-SORT 是带**摄像机运动补偿**、可选 **ReID 外观特征**的跟踪方案，一般用于摄像机移动等场景。

### 11.3 两者区别

| 对比项 | ByteTrack | BoT-SORT     |
| ------- | --------- | ------------ |
| 速度      | 快         | 较慢           |
| 结构      | 相对简单      | 结构更加复杂       |
| 量级      | 轻量级       | 较为重          |
| 一般用途    | 作为轻量级基线   | 对移动相机等场景更有帮助 |

## 12. 如何实现越线计数

假设视频中存在一条水平线：

```python
line_y = 400
```

记录每个目标**上一帧和当前帧的中心点**，例如：

```
上一帧 y = 380 < 400
当前帧 y = 420 > 400
```

说明该目标向下越线。

> [!important] 越线计数不能只看坐标
> 还必须保存**已经计数过的 ID**，并判断当前目标是否已经在其中；否则同一个目标在阈值线附近反复抖动会被重复计数：
>
> ```python
> # 只在第一次越线时计数
> if track_id not in counted_ids:
>     count += 1
>     counted_ids.add(track_id)
> ```

### 12.1 越线计数常见问题

#### 目标检测框抖动导致越线

框在阈值线附近上下抖动时，同一目标会反复"越过"又"退回"，造成**重复计数**。对策是给越线判定加**滞回**（例如上一帧在线上、当前帧要超出线下一段距离才判为越线），或先对框坐标做时域平滑，见上文第 6 节对检测框抖动的讨论。

#### 线设置得太靠近目标

目标稍微晃动就可能会触发计数。可行的解决方法为：

- 将线放在目标的必经路径上；
- 不要放在目标容易停留的位置；
- 尽量让目标有明显的运动方向。

#### 目标发生轨迹断裂

如果同一只动物在越线前后发生轨迹断裂：

```
越线前：ID 1
越线后：ID 3
```

就会把它判定为两个目标。因此，越线计数高度依赖：

- 跟踪器的稳定性；
- 检测质量；
- 遮挡处理能力；
- `track_id` 的连续性。

#### 跳帧可能跳过越线瞬间

如果在跳帧间隔中目标"越线后又回线"，会被误判为未越线。因此：

> [!warning] 越线计数不能过度跳帧
> 越线计数、快速目标分析和精确轨迹任务，**不能过度跳帧**。跳帧直接降低了时间采样率，一旦越线正好发生在被跳过的帧上就会被漏掉（见上文第 5 节）。

## 本课练习

1. 用 `model.track()` + `persist=True` 跑一段视频，逐帧打印 `track_id`、类别与中心点坐标；
2. 人为把 `conf` 调高，观察轨迹断裂与 ID Switch 是否变多；
3. 分别用 `bytetrack.yaml` 与 `botsort.yaml` 跑同一段视频，比较 ID 稳定性与处理速度；
4. 在轨迹基础上实现越线计数，验证"已计数 ID 去重"能否避免同一目标被重复计数。

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
> - **轨迹断裂**是同一目标没保住同一 ID；**ID Switch** 是两个目标互相认错 ID，后者更严重，两者都会直接毁掉计数与统计
> - 跟踪器二选一：**ByteTrack** 轻量快、用低置信度框补救轨迹；**BoT-SORT** 带相机运动补偿与 ReID，ID 更稳但更慢
> - **越线计数** = 中心点跨线 + 已计数 ID 去重；线别贴目标、别过度跳帧，抖动要靠滞回或时域平滑抑制

---

## 导航

- 上一课：[[YOLO第八课 训练结果分析,错误诊断与优化]]
- 下一课：第十课——性能优化与模型部署（待补充）
- 总览：[[视觉学习/YOLO学习/_MOC|YOLO 学习总览]]
- 进度与练习：[[YOLO学习进度]]
