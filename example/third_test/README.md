# 演示在服务中使用三方库

以 C++ 常用的 json 解析库 `jsoncpp` 为例

## 安装 jsoncpp 库
* Ubuntu
```bash
apt-get install libjsoncpp-dev
```

* CentOS7
```bash
yum -y install epel-release
yum -y install jsoncpp-devel.x86_64
```

## 编译服务动态库
```bash
mkdir build && cd build
cmake ..
make
```

## 配置 nginx.conf 文件
* 配置文件修改完后, 先执行 `nginx -t` 测试文件配置是否有问题, 没有报错即可启动或通过 `nginx -s reload` 重启
```nginx
http {
    # 配置服务列表
    service {
        libuse_third.so     srv_jsoncpp;
    }

    # so 文件路径(根据自己实际的路径配置)
    module_path {
        /root/ngx_http_service_module/example/third_test/build  libuse_third.so;
    }

    # so 依赖关系(使用 jsoncpp 需要链接 libjsoncpp.so 库)
    # 由于 libjson.so 在动态库的搜索路径下, 可以不用为其配置路径
    module_dependency {
        libuse_third.so     libjsoncpp.so;
    }

    server {
        
        # 此路由设置为服务处理模式, 发送给它的 POST 请求会被直接处理
        location /service_test {
            service_mode on;
        }
    }
}
```

## 测试
```bash
curl -X POST "http://localhost/service_test" \
-H 'Content-Type: application/json' \
-H 'Service-Name: srv_jsoncpp'

# 返回报文如下
# {
#    "data" : "use jsoncpp"
# }
```