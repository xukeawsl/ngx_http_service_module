# Nginx 服务处理模块

[![License](https://img.shields.io/npm/l/mithril.svg)](https://github.com/xukeawsl/ngx_http_service_module/blob/master/LICENSE)

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

## 配置指令

### 1. module_path (只能在 http 块配置)
* 此指令配置动态库所属路径, 如果未配置则安装默认搜索路径搜索
* 第一个参数为路径, 后面跟动态库文件名(全名)
```
module_path {
    /path/to/dir1    module1.so;
    /path/to/dir2    module2.so  module3.so;
}
```

### 2. module_dependency (只能在 http 块配置)
* 此配置设置动态库之间的依赖关系
* 第一个参数是被依赖的动态库, 后面跟它依赖的动态库
```
module_dependency {
    module1.so     module2.so;
    module3.so     module2.so  libjsoncpp.so;
}
```

### 3. service (只能在 http 块配置)
* 此配置设置模块提供的服务
* 第一个参数是动态库文件名, 后面为动态库提供的服务名(大小写敏感)
* 服务名需要全局唯一
```
service {
    module1.so    srv_echo  srv_datetime;
    module2.so    srv_sayHello;
    module3.so    srv_getSum;
}
```

### 4. service_mode (http、server、location 块均可配置)
* 用于启用模块功能, 未启用的部分不进行服务处理
* 默认不启用, 以下示例在指定服务器上全局启用, 但是对于 `/test` 的路由请求不处理
```
http {

    server {
        sevice_mode on;

        location /test {
            service_mode off;
        }
    }
}
```

## 服务接口开发
* 用户需要在项目中包含 `ngx_http_service_interface.h` 接口文件
```c
// 接口形如下面的函数

#include "ngx_http_service_interface.h"
#include "cJSON.h"

void srv_sayHello(const ngx_request_t *rqst, ngx_request_t *resp)
{
    // do something...
}
```

* 请求/响应结构
```c
// 请求结构只包含一个 const char * 指针指向 json 请求字符串
struct ngx_json_request_s {
    const char *data;
};

// 响应结构包含一个 char * 指针指向 json 响应字符串和一个回调函数用于释放响应字符串的内存
struct ngx_json_response_s {
    char *data;
    void (*release)(void *);
};
```