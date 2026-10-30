# StreamDemo
StreamDemo 是一个基于QT框架实现流媒体服务的推流端和拉流端的协同系统。需要结合轻量级流媒体服务器mediamtx使用。
核心功能包括：局域网UDP广播设备发现、TCP私有协议命令交互（ACC_ON/ACC_OFF/LOCK）、RTSP视频推流与拉流、OpenGL加速YUV渲染、硬件编码录像、碰撞检测触发事件录像及分段落盘。
## mediamtx
<img width="1105" height="195" alt="image" src="https://github.com/user-attachments/assets/24045e5f-b53d-44a7-beea-afa7b55d5345" />
mediamtx支持的推流流媒体协议：RTSP、RTMP、HLS、SRT
mediamtx支持的拉流流媒体协议：RTSP、RTMP、HLS、SRT、WebRTC、MoQ
## 效果图
​​<img width="460" height="335" alt="image" src="https://github.com/user-attachments/assets/dfc0424a-6888-4542-8d6d-ef3b38406270" />
<img width="456" height="335" alt="image" src="https://github.com/user-attachments/assets/20e6928b-0514-4f01-84f1-f666ed000d2a" />
<img width="980" height="655" alt="image" src="https://github.com/user-attachments/assets/699ff584-100f-4af4-ac7e-e1edff63c6ec" />
## 说明
两个应用都只支持RTSP和RTMP，先开启mediamtx流媒体服务器，然后打开推流端点击【开始推流】，然后打开拉流端，点击【连接】按钮，即可看到流媒体视频。
## 篇外
功能详细介绍可查阅
https://blog.csdn.net/liangyuna8787/article/details/169463601?sharetype=blogdetail&sharerId=169463601&sharerefer=PC&sharesource=liangyuna8787&spm=1011.2480.3001.8118
