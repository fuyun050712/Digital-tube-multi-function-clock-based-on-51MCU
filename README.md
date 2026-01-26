## Digital-tube-multi-function-clock-based-on-51MCU

基于51单片机的多功能数码管时钟

芯片：STC89C52RC

项目代码全部集中在main.c文件中，方便直接复制进keil5中。

基本功能：

（1）电子时钟：即可以实现常见的电子时钟功能，并能分别对“时”、“分”、“秒”进行设置包括可以显示。

（2）闹钟功能：系统达到预设的时间后，驱动蜂鸣器发出声音。

（3）具有复位功能：即按下复位键后，显示值为“000000”。

（4）具有秒表功能，时间步长0.01秒。

（5）具有万年历功能，能显示年、月、日

（6）添加温度传感器模块，实时显示现在温度

交互：由五个独立按键控制用K1~K5表示，有五个交互界面，0:时钟界面；1:调时界面；2:闹钟界面；3:日历界面；4:温度界面；5:秒表界面

（1）在时钟界面，按下K1，进入调时界面。此时K1为选择调时的位置（选择时选择的数字会闪烁），K2为数字-1，K3为数字+1，K4为确认并返回时钟界面。

（2）在时钟界面，按下K2，进入闹钟界面。数码管第一位会显示“P”，表示STOP,闹钟关闭，此时K1为选择调时的位置（选择时选择的数字会闪烁），当每个位置都选择过一遍，数码管第一位会显示“E”表示OPEN,闹钟开启。K2为数字-1，K3为数字+1，K4为确认并返回时钟界面。

（3）在时钟界面，按下K3，进入日历界面。此时K1为选择调时的位置（选择时选择的数字会闪烁），K2为数字-1，K3为数字+1，K4为确认并返回时钟界面。

（4）在时钟界面，按下K4，进入温度界面。再按下K4为返回时钟界面。

（5）在时钟界面，按下K5，进入秒表界面。按下K1开始计时/暂停，K2为清零，K4为确认并返回时钟界面。

## 原理图
![原理图](https://github.com/fuyun050712/Digital-tube-multi-function-clock-based-on-51MCU/blob/main/picture/SCH_Schematic1_1-P1_2026-01-26.png)
## PCB
![PCB](https://github.com/fuyun050712/Digital-tube-multi-function-clock-based-on-51MCU/blob/main/picture/PCB_PCB1_2026-01-26.png)

喜欢的话不妨给个⭐

## Star History

[![Star History Chart](https://api.star-history.com/svg?repos=fuyun050712/Digital-tube-multi-function-clock-based-on-51MCU&type=date&legend=top-left)](https://www.star-history.com/#fuyun050712/Digital-tube-multi-function-clock-based-on-51MCU&type=date&legend=top-left)
