# SEMG通信协议

标签（空格分隔）： SEMG

---
## 1. 引言
**Notes**
  - 协议中采用小端模式
  - 表格中的字段默认是1个字节

| 时间        |版本   |  内容  | 作者|
| :--------:   | :-----:  | :----:  | :-----: |
| 2014-6-xx    | 1.0     |         | 林上耀 |
| 2015-5-14  | 2.0      |        | 林上耀 |

## 2. 协议过程
### 2.1 MCU（Branch)和Linux(root)通信协议
#### 2.1.1 Channel数据包
Channel通道数据包(203Byte)

| 0x11        |Channel_num   |  State  | Data|
| :--------:   | :-----:  | :----:  | :-----: |

- 0x11: Channel数据包头
- Channel_Num: 通道编号,0-127共128通道
- State: 状态字节(留作扩展)
- Data 通信包中数据字段,200Byte，共100个AD采样值
    其中

| DataH  | DataL | ... | DataH | DataL|
| :-----: | :-----: | :----: | :-----: |:-----: |

####2.1.2 握手过程
Branch同root握手包
*Branch握手应答包*

| 0xB6 | Branch_Num | State |
|------|-----|------|

- 0xB6: 握手应答包头
- Branch_Num: Branch编号,0-7
- State: 状态标志,保留
root->branch
*root握手请求包*

| 0xA5 | Branch_Num |  Cmd  |
| --- | -------     |-----  |

- 0xB6: 握手应答包头
- Branch_Num: Branch编号,0-7
- State: 状态标志,保留

#### 2.1.3 数据通信
Branch和root数据通信过程
*Branch->root(共3258 `Bytes)*

| 0xB7 | Branch_Num | DataLenH | DataLenL | FrameNumberH | FrameNumberL | WaitTime | stateH | stateL| Data | 0xED |
| ---  |------      |------|---|-----------|-------------|----------------|------- |--------|-------|-----|  -----|

- 0xB7: Branch数据应答头
- Branch_Num: Branch编号,0-7共8个，每个Branch携带(8/16 Channel)
- DataLenH: 状态字节(留作扩展)
- DataLenL: 通信数据
- FrameNumberH: Frame Number高8
- FrameNumberL: Frame Number低8位
- WaitTime: 从准备发送到实际发送的FrameNumber之差,即等了多少ms
- StateH: 状态标志高8位[7] - x
                [6] - x
                [5] - x
        [4]
                [3:1] - Buffer Status(empty, write, full or overflow)
                [0] - Buffer Label(left buffer or right buffer
- StateL: 状态标志低8位 [7-0] - Overflow Count
- Data: Branch数据,共n(16)个通道，每个通道数据包的格式

| 通道0 数据包 | 通道1 数据包 | ... | 通道n-1 数据包 |
|-----        | -----------| ---- | -----------  |

- 0xED: 包尾

### 2.2 root和PC通信协议
root和PC采用socket进行通信，同样需要握手和数据通信。
#### 2.2.1 握手(ShakeHand)
root->PC
*root握手应答包*

| 'h' | 协议版本H | 协议版本L | 总通道数 | AD采样率 |
|---- |----------| ---------|---------|---------|

 - 'h': PC握手请求包
 - Branch_Num: Branch编号,0-7共8个，每个Branch携带(8/16 Channel)
 - 协议版本H: 状态字节(留作扩展)
 - 协议版本L: 通信数据
 - 总通道数: 时间戳高8位
 - AD采样率: 时间戳低8位
 - StateH: 状态标志高8位
 - StateL: 状态标志低8位
 - Data: Branch数据
 - 0xED: 包尾

##### PC->>root
*PC握手请求包*

|'H'|
|---|

- 'H': 握手请求包头

#### 2.2.2 数据包(Data)
*PC数据请求包*

|'D'|
|---|

* 'D': 数据请求包头
*root数据应答包*

| d'   | LenH | LenL | TimestampH | TimestampL | Data | 0xED |
|---   | ----| ------|------------|-------------|-----|------|

- 'd': 数据应答头
- LenH: 本帧字节长度高8位
- LenL: 本帧字节长度低8位
- TimestampH: 时间戳高8位
- TimestampL: 时间戳低8位
- Data: 数据包正文
            共n(128)个通道，每个通道数据包的格式
**参照**

|通道0 数据包|通道1 数据包|...|通道n-1 数据包|
|----------| ------ | -----| ------------|

#### 2.2.3 控制包(Control)

##### PC->root
*PC控制请求包*

| 'C'(0x43) | CMD | Data | CheckCode |0xED |
| ---- | ---------| ---- | ----------|-----|

- 'd': 数据应答
- LenH: 本帧字节长度高8位
- LenL: 本帧字节长度低8位
- TimestampH: 时间戳高8位
- TimestampL: 时间戳低8位
- Data: 数据包正文
            共n(128)个通道，每个通道数据包的格式
**参照**

|通道0 数据包|通道1 数据包|...|通道n-1 数据包|
|---------- | -------   |---|----------- |

##### PC->root
*root控制应答包*

|'C'(0x63)|ACK_CMD|Data|0xED|
| ------- | ------ | ---|----|