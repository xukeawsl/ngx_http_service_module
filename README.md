# Nginx 服务处理模块


## 先决条件
* Linux 平台
* Nginx 版本不低于 `1.9.11`
* 已安装 Nginx 编译所需工具

## 模块特性
* 服务请求/响应报文均为 `json` 格式, 默认将 `cJSON` 库编译进去了, 业务开发人员可以使用其进行报文解析/打包, 当然也可以使用其他的 `json` 解析库
* 通过实现特定接口, 将服务编译成动态库加载到 Nginx 中

## 优点
* **高性能**, 借助 Nginx 的多进程模型来处理业务, 同时业务开发人员还能根据具体场景在实现的接口中使用多线程进一步提升性能
* **健壮性强**, 当服务崩溃时, Nginx 会重新拉起工作进程, 保证业务不停止处理
* **服务平滑重启/升级**, 借助 Nginx 的平滑重启/升级功能, 可以优雅地进行服务重启/升级


## 构建

### 1. 下载 Nginx 源码
```bash
# 这里以 nginx-1.24.0 为例, 也可以根据自己情况下载其他版本的 nginx 源码
wget http://nginx.org/download/nginx-1.24.0.tar.gz
```

### 2. 解压并进入 Nginx 源码目录
```bash
tar -zxvf nginx-1.24.0.tar.gz
cd nginx-1.24.0
```

### 3. 拉取本项目的代码
```bash
# 这里拉取到 nginx 源码的目录下, 也可以根据情况拉取到其他目录
git clone https://github.com/xukeawsl/ngx_http_service_module.git
```

### 4. 将模块添加到 nginx 中并安装
```bash
# 如果你将项目代码放在其他路径, 则需要修改这里的模块路径
./configure --add-module=ngx_http_service_module
make
make install
```

