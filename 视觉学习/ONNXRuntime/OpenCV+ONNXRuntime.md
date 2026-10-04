# 1. `cv::Mat`内存布局
`Mat`由两部分组成: `header` + `data`
`header`存储`rows`,`cols`,`step`,`flags`,`type`
`data`为指向矩阵数据区的指针
>[!重点1]
> `step[0]`不一定等于`cols * channels()`;
>图像做了`ROI`或者由内存补齐时,`step` > `cols * channels()`
>一般使用`isContinuous()`判断内存是否连续
>`clone()`或`reshape`会强制内存连续


>[!重点2]
>`type()`返回不同类型,例如`CV_8UC3`表示uint8 * 3channels
>`cvtColor(BGR2RGB)`,`resize()`返回新的`Mat`;
>`Mat a = b;`仅浅拷贝,共享数据
# 2. NCHW(ONNX) -> HWC(OpenCV)
- pytorch中训练使用的NCHW,但是OpenCV使用的是HWC;
ONNX的输入`shape`是固定的`(1,3,640,640)`,即`(batch,channels,H,W)`
	其一维下标公式为:
	```
	input[c][y][x] -> flat[c*640*640 + y*640+x]
	```
- YOLOv8输出`(1,84,8400)`
	第i个候选框的第k个值:
	```
	v(k,i) = out[k*8400 + i]; // 注意是k*8400+i
	```
>[!索引公式(核心)]
>```
>HWC : idx(y,x,c) = (y*W + x)*C+c;
>CHW : idx(c,y,x) = (c*H*W+y)*W+x;
>NCHW : idx(n,c,y,x) = ((n*C+c)*H+y)*W+x
>```
>通用的stride版(应对非连续矩阵/有padding的张量)
>```
>HWC : idx = y*strideH+x*strideW+c*strideC 
>// OpenCV中strideH = mat.step[0],strideW = C,strideC = 1
>NCHW : idx = n*sn + c*sc + y*sy + x*sx 
>```

## HWC -> CHW/NCHW
HWC排列:**以像素作为区分**
```
R  G  B
1,2,3, // 像素0
4,5,6, // 像素1
7,8,9, // 像素2
10,11,12, // 像素3
行优先,先遍历完一行所有像素再换行,W*C是一行的总元素数(W个像素,每个像素C个通道)
因此: 线性索引公式: index(y,x,c) = y*(W*C)+x*C+c
```
CHW排列:**以通道作为区分**
```
R: 1,4,7,10, // R通道
G: 2,5,8,11, // G通道
B: 3,6,9,12 // B通道
先遍历一个通道的所有像素,H*W是一个通道的总像素数,通道顺序为RGB
线性索引公式: index(c,y,x) = c*(H*W)+y*W+x
```
转换参照形式:
```C++
void hwc_to_chw(const float* src, float* dst, int H, int W, int C) {
    const size_t HW = H * W; // 计算像素总数
    for (size_t i = 0; i < HW; ++i) { // 对HWC的每个像素进行遍历
		// 由上方的索引公式可得:
		// index_CHW[c*H*W+y*H+x] = index_HWC[y*W*C+x*C+c]
		// 注意到这里的i是从0遍历到H*W,所以最终:i = y*H+x
		// 这里是从OpenCV的BGR格式到RGB格式
		dst[0*HW + i] = src[i*3+2]; //R 
		dst[1*HW + i] = src[i*3+1]; //G 
		dst[2*HW + i] = src[i*3+0]; //B
    }
}
```
OpenCV中将HWC快捷转换为CHW的API(生成的`cv::Mat`可以转换为`Ort::Value`后直接送入ONNXRuntime推理):
```C++
cv::Mat cv::dnn::blobFromImage(
    InputArray image,          // 输入图像（单张）
    double scalefactor = 1.0,  // 像素值缩放因子（乘）
    const Size& size = Size(), // 输出空间尺寸 (width, height)，默认原图大小
    const Scalar& mean = Scalar(), // 减去均值（顺序取决于 swapRB）
    bool swapRB = false,       // 是否交换 R 和 B 通道
    bool crop = false,         // 是否中心裁剪（配合 size 使用）
    int ddepth = CV_32F        // 输出数据类型，通常 CV_32F
);
// 返回值为(1,C,H,W)的float32数组
```
### NCHW ->HWC
```C++
// tensor : const float*
// shape(1,C,H,W)是归一化后的结果
// 输出 : CV_8UC3,按照RGB排布
cv::Mat out(H,W,CV_8UC3);
const int plane = H*W;
for(int y=0;y<H;++y){
	uchar* row = out.ptr<uchar>(y);
	for(int x=0;x<W;++x){
		const int i = y*W+x; // i像素,原理同上方
		float r = tensor[0*plane+i];
		float g= tensor[1*plane+i];
		float b = tensor[2*plane+i];
		// 最后获取OpenCV的BGR格式
		row[x*3 + 0] = saturate_cast<uchar>(b * 255.0f);  // 反归一化
        row[x*3 + 1] = saturate_cast<uchar>(g * 255.0f);
        row[x*3 + 2] = saturate_cast<uchar>(r * 255.0f);
	}
}
```


# 3. ONNX Runtime C++ API
## 流程骨架
```C++
Ort::Env env(ORT_LOGGIN_LEVEL_WARNING,"det");
Ort::SessionOptions opts; 
Ort::Session session(env,modelPath.c_str(),opts;

Ort::AllocatorWithDefaultOptions alloc;
auto inName = session.GetInputNameAllocated(0,alloc);
auto outName = session.GetOutputNameAllocated(0,alloc);

Ort::MemoryInfo info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator,OrtMemTypeDefault);
Ort::Value inTensor = Ort::Value::CreateTensor<float>( info, buf.data(),buf.size(),inputShape.data(),inputShape.size()
);
auto outs = session.Run(
	Ort::RunOptions{nullptr},
	inName.get(), &inTensor,1,
	outName.get(),1
);
const float* p = outs[0].GetTensorData<float>();
```

# 4. letterbox 
传入图像时,不能直接resize为640 * 640,直接拉伸会使物体的长宽比更改,***而卷积和锚框是在"物体形状不变形"的前提下进行的***,而letterbox保持长宽比,边缘补灰
>[!两个个必须和训练段对齐的细节]
>通道顺序: `cv::imread`给的是BGR,训练时PIL等读的是RGB,必须使用`cvtColor(BGR2RGB)`,
>padding值: Ultralytics使用的是114而不是0


# 5.输出张量解读 - (1,84,8400)到底装了什么
- 1代表batch
- 8400表示三个检测尺度的网格数之和 :`80*80 + 40*40 + 20*20 = 8400`
- 84 = 4(cx,cy,w,h)+80(COCO类别分), 注意不是固定的84,由标签类别数决定
如果导出时选了`nms = True`,输出会变成已经NMS的(1,300,6)等张量而非(1,84,8400)

# 6. 坐标映射回原图
模型使用的是`640*640`的letterbox图,最重要在原图上画框等
```C++
float scale = 640*640 / imageH / imageW; // 缩放比例
float x1o = (x1-padX) / scale;
float y1o = (y1-padY) / scale;
// 最后clamp到[0,w0-1]/[0,h0-1]
```
注意: 这里letterbox是由原图先乘scale后再加pad,逆操作相反

# 7. 手写IoU+NMS后处理
```
// 交并比
IoU(A,B) = inter(A,B) / (area(A)+area(B)-inter(A,B))

// NMS贪心流程
同类别中按照score分数降序排序
while 候选集非空:
	取出score最高的框m,加入到keep保留
	删除候选框中所有IoU(m,b) > iouThresh 的框b(删除重叠度高的框,仅保留置信度高且独立度高的框) 
```
>[!重点]
>一定要按类别分别做NMS(class-wise NMS)
>阈值iouThresh表示重不重复,高于其的框删除;
>阈值confThresh表示可不可信,高于其的框才保留

# 8. `Detector`封装与延迟测量
```C++
class Detector{
public:
	// 配置
	struct Config{float confThresh = 0.25f;float iouThresh = 0.45f;int inputSize = 640;};
	// 置信框(OpenCV格式: x1,y1,x2,y2)
	struct Box{float x1,y1,x2,y2,score;int label;};
	explicit Detector(const std::string&modelPath,Config config = {});
	Detector(const Detector&) = delete; // Ort::Session不可拷贝
	Detector& operator = (const Detector&) = delete; 
	
	std::vector<Box>detected(const cv::Mat&bgr); 
	
private:
	Ort::Env env;
	Ort::Session session;
	Config config;
	std::vector<float>inputBuf; // 缓冲区,避免每次都要进行分配内存	
};
```


```
flowchart TD
    S1[Mat 内存布局] --> S2[NCHW 一维索引]
    S2 --> S3[ORT API 绑 tensor]
    S2 --> S4[letterbox/归一化]
    S4 --> S6[坐标逆映射]
    S5[输出张量解读] --> S6
    S6 --> S7[IoU/NMS]
    S3 --> S5
    S7 --> S8[Detector 封装]
    A3[A轨 RAII/智能指针] -.支撑.-> S8
    A4[A轨 move-only] -.支撑.-> S8
    M1[M1 训练一致性] -.约束.-> S4
    S8 --> M3[相机送图进 Detector]
    S8 --> M6[Qt 推理线程]
```
