# stream_save_mp4
基于ffmpeg,将h264视频以内存方式存成mp4.用一个h264码流文件分段读到内存, 然后ffmpeg库从内存中不断取数据,存mp4.支持设置总共存多少帧.
编译环境  vs2017 x64 Relase
