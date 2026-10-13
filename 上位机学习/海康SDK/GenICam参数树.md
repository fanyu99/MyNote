---
title: GenICam 参数树
type: 主题笔记
系列: 上位机学习
created: 2026-10-07
updated: 2026-10-13
tags:
  - 上位机
  - 海康SDK
  - 主题笔记
status: 进行中
---

# GenICam 参数树

> 概述：MVS 相机的参数系统本质是一棵 **GenICam 节点树**——每个可配置项都是一个「节点」，C++ 侧通过节点名（Node Name）读写。本页梳理参数树的组织结构、六种节点类型、参数间的依赖关系、访问权限与推荐配置顺序。

---

## 一、参数树结构

MVS相机的内部还有一套参数系统
```
Camera
├── AcquisitionControl
│   ├── AcquisitionMode
│   ├── TriggerMode
│   ├── TriggerSource
│   └── TriggerSoftware
│
├── AnalogControl
│   ├── Gain
│   ├── GainAuto
│   ├── Gamma
│   └── GammaEnable
│
├── ImageFormatControl
│   ├── Width
│   ├── Height
│   ├── OffsetX
│   ├── OffsetY
│   └── PixelFormat
│
├── TransportLayerControl
│   ├── PacketSize
│   ├── PacketDelay
│   └── ...
│
└── DeviceControl
    ├── DeviceUserID
    ├── DeviceTemperature
    └── ...
```
参数树中的每一个参数,本质上就是一个节点
```
节点名称：ExposureTime
节点类型：Float
当前值：10000.0
最小值：10.0
最大值：1000000.0
步进值：1.0
是否可读：是
是否可写：是
```
### 一个重要的概念：Node Name
C++设置参数时,通常传入的是节点名称
例如

|MVS 界面名称|SDK 节点名称|类型|
|---|---|---|
|宽度|`Width`|Integer|
|高度|`Height`|Integer|
|曝光时间|`ExposureTime`|Float|
|增益|`Gain`|Float|
|自动曝光|`ExposureAuto`|Enumeration|
|触发模式|`TriggerMode`|Enumeration|
|触发源|`TriggerSource`|Enumeration|
|像素格式|`PixelFormat`|Enumeration|
|用户自定义名称|`DeviceUserID`|String|
|软触发命令|`TriggerSoftware`|Command|
|开启 Chunk 数据|`ChunkModeActive`|Boolean|
例如: 
```cpp
MV_CC_SetFloatValue(handle, 
"ExposureTime", // 参数名
 10000.0f);
```
### 六种参数类型
1. Integer: 整数,例如宽度长度等
2. Float: 浮点数.例如ExposureTime,Gain等
```cpp
MVCC_FLOATVALUE stFloatValue = { 0 };

int nRet = MV_CC_GetFloatValue( // 获取浮点数类型
    handle,
    "ExposureTime",
    &stFloatValue
);

if (MV_OK != nRet)
{
    printf("Get ExposureTime failed! Error code: 0x%x\n", nRet);
}
else
{
    printf("Current ExposureTime: %f\n", stFloatValue.fCurValue);
    printf("Min ExposureTime: %f\n", stFloatValue.fMin);
    printf("Max ExposureTime: %f\n", stFloatValue.fMax);
    printf("Increment: %f\n", stFloatValue.fInc);
}
```
3. Enumeration: 枚举,例如:TriggetMode的枚举等
```
TriggerMode:
├── On
└── Off
```
4. Boolean: 布尔值,例如`ChunkModeActive`等
5. String: 字符串,例如`DeviceUserID`等
6. Command: 命令,例如`TriggerSoftware`软件触发
```cpp
int nRet = MV_CC_SetCommandValue(
	handle,
	"TriggerSoftware"
);
if(MV_OK != nRet){
	...
}
```
---

## 二、参数依赖关系
参数之间存在依赖关系,状态关系和前置条件
### 示例 1：自动曝光与手动曝光
如果设置了自动曝光后,想要改为手动曝光,通常需要先关闭自动曝光后,在进行设置手动曝光时间
### 示例 2：宽度、高度与偏移量
ROI参数通常存在这种关系: 
```
OffsetX + Width <= SensorWidth
OffsetY + Height <= SensorHeight
```
因此,在设置宽度和高度等参数时,需要先将偏移量设置为0比较好
---

## 三、参数设置的生命周期

### 1. 采集前的参数
通常在:
```
OpenDevice 之后
StartGrabbing 之前
```
常见参数
- `Width`
- `Height`
- `OffsetX`
- `OffsetY`
- `PixelFormat`
- 部分触发参数
- 部分采集模式参数
```cpp
MV_CC_CreateHandle(&handle, &deviceInfo);
MV_CC_OpenDevice(handle);

// 设置采集前参数
MV_CC_SetIntValueEx(handle, "Width", 1000);
MV_CC_SetIntValueEx(handle, "Height", 800);
MV_CC_SetEnumValue(handle, "PixelFormat", PixelType_Gvsp_Mono8);

MV_CC_StartGrabbing(handle);
```
### 2. 运行时参数
通常为:
- 曝光时间
- 增益
- Gamma
- 部分输出控制参数
---

## 四、参数的访问权限：Access Mode
MVS中可见:
```
当前状态                说明
------------------------------------------------
可读                    可以获取
可写                    可以设置
只读                    只能获取，不能设置
不可用                  当前条件不满足
取流中不可修改           停止取流后才能修改
自动模式下不可修改       关闭自动模式后才能修改
```

---

## 五、推荐的参数配置顺序
```
1. 创建句柄
2. 打开设备
3. 设置采集模式
4. 设置触发模式
5. 设置触发源
6. 设置像素格式
7. 设置 ROI
8. 关闭自动曝光
9. 设置曝光时间
10. 关闭自动增益
11. 设置增益
12. 开始取流
13. 读取并确认参数
```

```cpp
// 1. 打开设备
int nRet = MV_CC_OpenDevice(handle);

if (MV_OK != nRet)
{
    printf("Open device failed!\n");
    return nRet;
}

// 2. 连续采集模式
nRet = MV_CC_SetEnumValue(
    handle,
    "AcquisitionMode",
    MV_ACQ_MODE_CONTINUOUS
);

// 3. 关闭触发，使用连续取流
nRet = MV_CC_SetEnumValue(
    handle,
    "TriggerMode",
    MV_TRIGGER_MODE_OFF
);

// 4. 设置像素格式
nRet = MV_CC_SetEnumValue(
    handle,
    "PixelFormat",
    PixelType_Gvsp_Mono8
);

// 5. 设置 ROI
nRet = MV_CC_SetIntValueEx(
    handle,
    "Width",
    1000
);

nRet = MV_CC_SetIntValueEx(
    handle,
    "Height",
    800
);

// 6. 关闭自动曝光
nRet = MV_CC_SetEnumValue(
    handle,
    "ExposureAuto",
    MV_EXPOSURE_AUTO_OFF
);

// 7. 设置曝光时间
nRet = MV_CC_SetFloatValue(
    handle,
    "ExposureTime",
    10000.0f // 单位us
);

// 8. 关闭自动增益
nRet = MV_CC_SetEnumValue(
    handle,
    "GainAuto",
    MV_GAIN_MODE_OFF
);

// 9. 设置增益
nRet = MV_CC_SetFloatValue(
    handle,
    "Gain",
    5.0f
);
```
设置参数封装
```cpp
int SetFloatParameter(
    void* handle,
    const char* nodeName,
    float value
)
{
    int nRet = MV_CC_SetFloatValue(
        handle,
        nodeName,
        value
    );

    if (MV_OK != nRet)
    {
        printf(
            "Set float parameter failed. "
            "Node=%s, Value=%f, Error=0x%x\n",
            nodeName,
            value,
            nRet
        );
    }

    return nRet;
}

```

> [!note] 重要的结论表
>
>|参数类型|获取函数|设置函数|
|---|---|---|
|Integer|`MV_CC_GetIntValueEx`|`MV_CC_SetIntValueEx`|
|Enumeration|`MV_CC_GetEnumValue`|`MV_CC_SetEnumValue`|
|Boolean|`MV_CC_GetBoolValue`|`MV_CC_SetBoolValue`|
|Float|`MV_CC_GetFloatValue`|`MV_CC_SetFloatValue`|
|String|`MV_CC_GetStringValue`|`MV_CC_SetStringValue`|
|Command|无普通获取|`MV_CC_SetCommandValue`|
---

## 六、三种触发方式

### 1. 连续采集：相机持续拍，程序持续收
参数配置为:
关闭触发模式并持续获取
```
AcquisitionMode = Continuous
TriggerMode = Off
```
伪代码
```cpp
// 设备已经打开，且尚未开始取流

int nRet = MV_CC_SetEnumValue(
    handle,
    "AcquisitionMode",
    MV_ACQ_MODE_CONTINUOUS
);

if (nRet != MV_OK)
{
    printf("Set AcquisitionMode failed: 0x%x\n", nRet);
    return nRet;
}

nRet = MV_CC_SetEnumValue(
    handle,
    "TriggerMode",
    MV_TRIGGER_MODE_OFF
);

if (nRet != MV_OK)
{
    printf("Set TriggerMode failed: 0x%x\n", nRet);
    return nRet;
}
// 开始取流
int nRet = MV_CC_StartGrabbing(handle);
if(nRet != MV_OK){
 printf("StartGrabbing failed: 0x%x\n",nRet);
 return nRet;
}
// 程序收图:
// 一般持续收图使用循环
while (isRunning)
{
    MV_FRAME_OUT frame = { 0 };

    int nRet = MV_CC_GetImageBuffer(
        handle,
        &frame,
        1000
    );

    if (nRet == MV_OK)
    {
        // 在这里处理或复制图像数据

        MV_CC_FreeImageBuffer(handle, &frame);
    }
    else
    {
        // 超时或取图错误：记录日志、检查相机状态
    }
}
```
什么时候适合连续采集?
- 相机需要持续地观察场景
- 目标一直运动,需要连续的检测
- 采集节拍主要由相机帧率决定

### 2. 软触发：程序要求相机拍一帧
通常用于:
- 调试触发流程
- 上位机按钮拍照
- 程序接收通信命令后进行拍照
配置流程:
```
打开相机
    ↓
TriggerMode = On
    ↓
TriggerSource = Software
    ↓
StartGrabbing
    ↓
执行 TriggerSoftware
    ↓
等待并取回图像
```
配置示例:
```cpp
// 设备已经打开，但尚未开始取流

int nRet = MV_CC_SetEnumValue(
    handle,
    "TriggerMode",
    MV_TRIGGER_MODE_ON
);

if (nRet != MV_OK)
{
    printf("Enable TriggerMode failed: 0x%x\n", nRet);
    return nRet;
}

nRet = MV_CC_SetEnumValue(
    handle,
    "TriggerSource",
    MV_TRIGGER_SOURCE_SOFTWARE
);

if (nRet != MV_OK)
{
    printf("Set TriggerSource failed: 0x%x\n", nRet);
    return nRet;
}

nRet = MV_CC_StartGrabbing(handle);

if (nRet != MV_OK)
{
    printf("StartGrabbing failed: 0x%x\n", nRet);
    return nRet;
}
```
发出一次触发再取一帧
```cpp
// 1. 告诉相机采一帧
int nRet = MV_CC_SetCommandValue(
    handle,
    "TriggerSoftware"
);

if (nRet != MV_OK)
{
    printf("TriggerSoftware failed: 0x%x\n", nRet);
    return nRet;
}

// 2. 等待并取回这一帧
MV_FRAME_OUT frame = { 0 };

nRet = MV_CC_GetImageBuffer(
    handle,
    &frame,
    1000 // 等待超时时间1000ms
);

if (nRet == MV_OK)
{
    printf(
        "Triggered image: width=%u, height=%u, frameNum=%u\n",
        frame.stFrameInfo.nWidth,
        frame.stFrameInfo.nHeight,
        frame.stFrameInfo.nFrameNum
    );

    // 在释放缓冲区之前使用图像数据
    ...
	// 释放缓冲区
    MV_CC_FreeImageBuffer(handle, &frame);
}
else
{
    printf("Get triggered image failed or timed out: 0x%x\n", nRet);
}
```
### 3. 硬触发：外部设备让相机拍一帧
通常用于:
- PLC控制
- 光电传感器
- 编码器
- 运动控制卡
软件侧需要配置
```
TriggerMode = On
TriggerSource = Line0 / Line1 / 其他输入源
```
示例
```cpp
// 设备已经打开，尚未开始取流

int nRet = MV_CC_SetEnumValue(
    handle,
    "TriggerMode",
    MV_TRIGGER_MODE_ON
);

if (nRet != MV_OK)
{
    printf("Enable TriggerMode failed: 0x%x\n", nRet);
    return nRet;
}

// 假设 Line0 是该相机实际支持的触发输入源。
// 真实设备上要先检查参数树中是否存在并可用。
nRet = MV_CC_SetEnumValue(
    handle,
    "TriggerSource",
    MV_TRIGGER_SOURCE_LINE0
);

if (nRet != MV_OK)
{
    printf("Set hardware TriggerSource failed: 0x%x\n", nRet);
    return nRet;
}

nRet = MV_CC_StartGrabbing(handle);

if (nRet != MV_OK)
{
    printf("StartGrabbing failed: 0x%x\n", nRet);
    return nRet;
}
// 程序仍然需要持续取图
while (isRunning)
{
    MV_FRAME_OUT frame = { 0 };

    int nRet = MV_CC_GetImageBuffer(
        handle,
        &frame,
        1000
    );

    if (nRet == MV_OK)
    {
        // 收到了外部触发后产生的图像

        MV_CC_FreeImageBuffer(handle, &frame);
    }
    else
    {
        // 在没有触发信号时，等待超时可能是预期行为。
    }
}
```

### 4. 三种触发模式对比

| 项目              | 连续采集      | 软触发        | 硬触发          |
| --------------- | --------- | ---------- | ------------ |
| 谁决定拍摄时机         | 相机自身      | 上位机程序      | 外部电信号        |
| `TriggerMode`   | 通常为 `Off` | `On`       | `On`         |
| `TriggerSource` | 不使用触发源    | `Software` | `Line0` 等输入源 |
| 程序要不要取图         | 要         | 要          | 要            |
| 是否需要外部硬件        | 不需要       | 不需要        | 需要           |
| 你当前能否重点练习       | 可以        | 可以         | 主要学配置与流程     |

---

## 七、AcquisitionMode 与 TriggerMode 的区别
`AcquisitionMode`描述一次采集任务需要采多少张
- `SingleFrame`
- `MultiFrame`
- `Continuous`
`TriggerMode`描述是否通过外部信号进行触发,例如软触发/硬触发,`On`开启,`Off`关闭

```
AcquisitionMode = Continuous // 连续采集
TriggerMode = On // 开启外部触发
TriggerSource = Software // 软触发
```

---

## 相关笔记

- [[上位机学习/海康SDK/入门]] —— 相机初始化与取图整体流程
- [[上位机学习/海康SDK/SDK缓冲区,图像数据生命周期]] —— 取图缓冲区与图像数据生命周期

---

- **知识库总览**：[[上位机学习/_MOC]]
- **仓库主页**：[[README]]

---

*本页随学习进度持续更新。*
