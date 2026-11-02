---
title: LoginPage
type: 通用笔记
topic: 项目实战
status: 已完成
created: 2026-08-18
tags:
  - wms
  - qt
  - mysql
  - 实施记录
series: WMS 实施记录
canvas: 实施记录/白板/UI模块/Day22 LoginPage基础功能.canvas
---

# LoginPage

> 用 AuthService 按用户名取用户信息并发出登录信号，接入 ini 配置与登录页控件。

1. 使用authservice通过用户名进行获取用户信息并emit登录信号
2. 登录成功信号传递登录用户信息,交给后续页面进行获取
3. 添加loginconfig字段以及相关配置,从ini文件中获取配置
4. UI方面添加userNameEdit,passwordEdit,loginBtn,并连接对应信号槽
5. 完成真实的登录页面测试