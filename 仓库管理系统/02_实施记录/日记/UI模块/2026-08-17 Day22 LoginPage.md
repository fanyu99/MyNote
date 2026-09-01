# 完成了loginpage的基础功能:
1. 使用authservice通过用户名进行获取用户信息并emit登录信号
2. 登录成功信号传递登录用户信息,交给后续页面进行获取
3. 添加loginconfig字段以及相关配置,从ini文件中获取配置
4. UI方面添加userNameEdit,passwordEdit,loginBtn,并连接对应信号槽
5. 完成真实的登录页面测试