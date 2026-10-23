---
id: net-dns-request-diagnosis
title: DNS、TCP 与面向服务的排障
description: 每个命令只证明它实际测试的那一层。
course: networks
order: 2
date: 2026-10-23
tags: [计算机网络, 后端, 面试]
---

# DNS、TCP 与面向服务的排障

## 名称解析不总从根开始

DNS 是分布式命名系统，A/AAAA 记录提供地址，CNAME 提供别名。客户端可查浏览器/系统缓存，再询问递归解析器；解析器根据缓存或委派查权威数据，必要时才访问根和顶级域。TTL 影响缓存期限，也存在负缓存，修改记录不代表所有客户端立即更新。

同一域名不同 IP 可能来自 CDN、地理调度或不同解析器，不足以单独证明欺骗。传统 DNS 常用 UDP，也必须考虑 TCP，DoH/DoT 为查询通道提供加密；域名解析正确不等于网站身份已经认证。

## TCP 是字节流，不是业务成功证明

典型新连接为 SYN、SYN+ACK、ACK，交换初始序列号并同步连接状态；TCP 负责有序字节流与重传等，不提供网站身份认证或业务授权。一次发送的边界不被 TCP 保留，应用需明确长度、分隔符等消息格式。

访问 HTTPS API 的简化路径：URL→DNS→路由→TCP→TLS→HTTP→Nginx→Spring MVC→数据库。复用连接、DNS 缓存、HTTP/3 都会改变实际步骤。TCP 建连成功但后端数据库不可用，接口仍可能 500；Nginx 502 可能发生在连接上游或协议处理阶段，需查对应日志。

## 实际排查：网页打开但 ping 不通

ping 使用 ICMP 回显，不是 TCP/UDP，不使用端口；服务可屏蔽 ICMP 而继续提供 HTTPS。HTTP/SOCKS 代理也通常不会转发普通 ping。域名找不到是解析阶段失败；已解析后超时只表示未收到 ICMP 应答，不能直接断定服务器离线。

```powershell
nslookup example.com
Test-NetConnection example.com -Port 443
curl.exe -I https://example.com
tracert example.com
```

每条分别检查 DNS、TCP 443、某次 HTTP HEAD 响应以及诊断路径。HEAD 被拒绝不等于 GET 失败，需根据接口选择方法；tracert 的星号也可能是中间设备不应答。curl 还会受环境变量与代理配置影响，比较前记录网络路径与协议版本。

## 服务器侧怎样继续

Linux 查看监听 `ss -lntp`、对应进程和应用日志，容器需进入正确网络命名空间。SSH 是管理服务器的加密协议，与向用户提供的 HTTP 不同，SSH 正常不能证明 443 正常。公网反向代理到 127.0.0.1:8080 时，内网 HTTP 链路和公网 TLS 分开检查。

开发中以同一个请求 ID 串联代理、控制器、数据库日志，记录 DNS/建连/TLS/首字节/总时长可缩小范围。先复现真实接口、相同方法、必要的测试认证与请求体，再用低层测试解释问题；不要把健康检查成功当所有业务接口成功。

## 拓展：观测结果的范围

抓包、日志和命令输出都是特定时间、路径、权限下的观测。一次 curl 成功不代表全天无故障，单次 ping 的 RTT 不等同用户接口响应时间。排障记录至少写明目的主机、端口、代理、时间与错误层级，避免更换多个条件后无法判断哪个因素起效。

## 参考资料

- [RFC 1034 DNS 概念与委派](https://www.rfc-editor.org/rfc/rfc1034)
- [RFC 9293 TCP 状态与字节流](https://www.rfc-editor.org/rfc/rfc9293)
- [RFC 9110 HTTP 方法语义](https://www.rfc-editor.org/rfc/rfc9110)
- [curl 官方手册](https://curl.se/docs/manpage.html)

## 模拟面试问题

1. 基础理解：DNS 缓存命中时是否还要询问根服务器？
2. 实际应用：ping 失败、TCP 443 成功分别说明什么？
3. 实际应用：TCP 建连成功但 API 500 应从哪些证据继续排查？
4. 深入追问：TCP 字节流为什么需要应用自行定义消息边界？
5. 深入追问：怎样设计排障对照以避免代理路径干扰结论？
