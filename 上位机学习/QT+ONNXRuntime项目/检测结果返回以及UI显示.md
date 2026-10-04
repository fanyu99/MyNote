>[!根据检测作业(Job)进行分类]
>1. Job含单图片/单视频Task
>2. Job多图片/多视频/多图片+视频Task

# 检测结果返回
## 1.不保存文件 + 单图片/视频
直接返回检测结果图片并交由QML进行显示
## 2.保存文件 + 单图片/视频
保存检测结果文件(图片/视频)到指定路径后,在显示器中显示检测图片
## 3.不保存文件 + 批量任务
仅保留缓冲区的最后n个结果并显示
## 4.保存文件 + 批量任务
保存到指定路径后,在UI的文件结果浏览面板进行选择文件并预览文件

# UI显示

## 1.Job进度条(非摄像头检测)
使用以下公式进行计算:
```C++
// jobProgress 为整体的作业进度
// taskCompleted 为作业中的整体完成的任务数
// taskUnitCompleted 为任务中的完成子部分,例如视频完成了多少帧
// taskUnitTotal 为任务中待总完成总数,例如视频总共多少帧
jobProgress = (taskCompleted + taskUnitCompleted / taskUnitTotal) / taskTotal;
```

