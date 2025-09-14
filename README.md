# LiveOrigin
    实时显示http请求坐标

    ![Peek 2025-09-14 17-59.gif](./images/where_from.gif)

# 编译

    1. git submodule update --init --recursive
    2. cd service && mkdir -p build && cd build && cmake .. && make
    3. ./LiveOrigin -d lo  -p 8080 -h ip_proxy
        -d:捕获的网络接口号，可通过ip addr查看
        -p:监听-d接口中的指定端口号
        -h:指定被nginx等转发时候的真实ip请求头字段，可忽略