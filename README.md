## 参考muduo的c++ reactor模式网络库

### 期待

* 使用该库实现http server
* 使用该库实现rpc框架

### 亮点

* 完整的单元测试覆盖
* 增加代码注释
* 代码量略微减少
* 其中日志库我单独写了一个库Asynlog在另外一个repo里面

### 学习过程中收获

* 学习使用非阻塞IO以及epoll(其他IO多路复用函数也可以)实现reactor模式
* 学习c++的线程池写法，避免创建过多线程对OS的调度产生过大压力
* 使用eventfd来实现线程的异步唤醒
* 尽量使用智能指针以及RAII机制来避免内存泄露
* 避免使用继承，使用std::function作为签名，传递函数时使用std::bind
* 使用std::shared_ptr\<void\> 来替代void \*，防止内存泄漏
* 使用idle fd来避免当文件描述符不够用时，accept出现busy loop的情况

### Environment

* OS: Ubuntu 16.04
* Compiler: g++ 5.4.0

### BUILD

> mkdir build && cd build/
>
> cmake .. && make