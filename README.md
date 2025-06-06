# VFD 变频器
中/快走丝线切割专用变频器，控制3相交流异步电机

## 基础配置
1. 主控芯片 **STM32G431RBT6**(128k flash and 32k ram)
2. 操作系统 **threadx 6.4.1**


## 基础框架
1. 采用事件驱动的微服务架构，每个业务均为一个服务(单独线程)，其他业务采用事件驱动的方式通信
2. 事件驱动链(待补充)






# NOTES
1. 第一次移植使用threadx，每个业务逻辑务必单独测试OK，否则逻辑太多，如果有BUG，不熟悉系统很难查找

2. 基于内存块，内存池，和消息队列实现的通信代码现场写的，没有经过实际项目验证，只是简单测试了下，是否可靠需要验证

3. 软件版本命名方式 **VA.B.C_250213**，其中：
    - A为主版本号，只在功能或者框架大变动时更新
    - B为次版本号，功能升级时更新
    - C为修订号，解决BUG时更新
    - 最后6位为固件编译打包时间

# 交流异步电机转速和电源频率的换算
1. n = 60f/p
2. **n**为转速，单位为rpm，**f** 为交流电源频率，单位为Hz，p为电机极对数

# 输入输出统计
1. 开关量输入信号，17个输入信号
    - 左右行程，超程(有效极性拨码开关控制，一般为常开NPN开关，有效极性为0)
    - 结束(有效极性拨码开关控制，一般为常开NPN开关，有效极性为0)
    - SP0~SP2(0~7速度调节)
    - 调试状态(0进入调试模式，无视断丝检测和加工结束，1无效)
    - 4键模式
        - 开运丝---常态为1(常开开关)，有效为0
        - 关运丝---常态为0(常闭开关)，有效为1
        - 开水泵---常态为1(常开开关)，有效为0
        - 关水泵---常态为0(常闭开关)，有效为1
    - 点动模式
        - 2个开关控制4个功能，触发一次为开运丝(开水泵)，再触发一次为1关运丝(关水泵)

2. 拨码开关信号，用于确定左右行程和超程的极性，4个

3. 断丝检测信号

4. 开关量输出信号，2个继电器输出信号
    - 开关高频
    - 水泵继电器输出220

5. ADC采样信号
    - 三相电流采样
    - 母线电压采样



# 本次修改需要测试的内容
1. 验证电机发热到底是刹车方式不对还是电压不对，修改刹车逻辑
2. 低频力矩提升
3. 变频关高频
4. 母线电压采样，三相电流采样(低端采样)
5. 分辨率提升
6. 优化SPWM波形

# 下一步计划

1. eeprom参数校验
2. 自动省电
3. 蓝牙调试
4. 手控盒逻辑
