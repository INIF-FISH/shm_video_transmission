# shm_video_demo

基于boost的进程间视频传输Demo，延迟优先，适用于三通道图像

## 注意

【!】调用send函数会更改输入图像大小

【!】大小固定方式为RESIZE

【!】同一Channel的width、height应保持一致，否则程序将报错退出

## 使用

### 依赖

opencv4.5.0+

boost

pthread

### 编译

```
mkdir build
cd build
cmake ..
make
```

### 发送者

```
./sender <channelName> <width> <height> <videoFilePath>
```

### 接收者

```
./receiver <channelName> <width> <height>
```
