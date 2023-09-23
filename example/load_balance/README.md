# 负载均衡测试
利用 nginx 的负载均衡使水平扩展变得更加容易, 可以由一台机器专门负责负载均衡转发, 其他 nginx 机器就负责处理转发过来的请求, 每台机器上支持的服务可以根据需要配置

为了方便演示, 下面用一台机器上的多个虚拟 server 来模拟多台机器负载均衡的场景, 监听端口 `39000` 的 server 负责转发请求到后端 server, 后端 server 有两个, 分别监听 `39001` 端口和 `39002` 端口

## 配置 nginx.conf 文件
```nginx
http {
    # 加入服务处理模块的配置
    include server.conf;

    server {
        listen 39000;
        server_name localhost;

        # 转发请求到后端
        location /service_test {
            proxy_pass http://ngx_backend;
        }
    }

    # 使用轮询负载均衡策略转发请求
    upstream ngx_backend {
        server localhost:39001;
        server localhost:39002;
    }

    # 业务处理服务器 1
    server {
        listen 39001;
        server_name localhost;

        location /service_test {
            service_mode on;
        }
    }

    # 业务处理服务器 2
    server {
        listen 39002;
        server_name localhost;

        location /service_test {
            service_mode on;
        }
    }
}
```

## 配置服务处理模块配置文件 server.conf

```nginx
server {
    libload_balance.so      srv_test1 srv_test2;
}

# 假设你编译的 so 所在路径为 /root/ngx_http_service_module/example/load_balance/build
module_path {
    /root/ngx_http_service_module/example/load_balance/build  libload_balance.so;
}
```

## 测试
* 向本机的 `39000` 端口发送服务请求即可
```bash
curl -X POST "http://localhost:39000/service_test" \
-H 'Content-Type: application/json' \
-H 'Service-Name: srv_test1'

# 返回报文如下
# {
#         "data": "test func one"
# }
```