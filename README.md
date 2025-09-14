# LiveOrigin
    实时显示http请求来源坐标

![Peek 2025-09-14 17-59.gif](https://raw.githubusercontent.com/houxinlin/live-origin/refs/heads/main/service/images/where_from.gif)
    
# 编译

    1. git submodule update --init --recursive
    2. cd service && mkdir -p build && cd build && cmake .. && make
    3. ./LiveOrigin -d lo  -p 8080 -h ip_proxy
        -d:捕获的网络接口号，可通过ip addr查看，默认lo
        -p:监听-d接口中的指定端口号，默认8080
        -h:指定被nginx等转发时候的真实ip请求头字段，可忽略，默认ip_proxy
        -m:使用本地数据库还是在线服务获取经纬度,默认net，可指定db模式
