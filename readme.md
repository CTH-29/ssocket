# 简化的socket接口

## 简述

ssocket(simplied socket interface) 用来简化linux系统里socket的接口

单文件（ssocket.c/ssocket.h）即可使用

## TODO

只支持ipv4的tcp协议

## 使用

具体函数使用详见main.c

## 测试

```bash
mkdir build
cd build
cmake ..
make
run ./ssocket
```

注意修改ssocket_create函数的参数url

## license

MIT
