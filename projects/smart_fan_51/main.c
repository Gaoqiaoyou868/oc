/******************************************************************************
 * 红外遥控智能风扇系统  (Keil C51)  — v1.0
 *
 * 芯片    : STC89C52RC  @ 11.0592 MHz
 * 编译环境: Keil uVision 5
 *
 *  ╔══════════════════════════════════════════════════════════╗
 *  ║  引脚映射 (普中A2实验板)                                 ║
 *  ║  LCD1602 : RS=P2.6, RW=P2.5, EN=P2.7, D0~D7=P0       ║
 *  ║  直流电机: P2.0 → L298.IN1, P2.4 → L298.IN2          ║
 *  ║            P3.1 → L298.IN3 (预留未用)                 ║
 *  ║            (L298 ENA→+5V, VS→+12V, VSS→+5V)           ║
 *  ║  红外接收: P3.2 (外部中断0, HS0038)                     ║
 *  ║  蜂鸣器  : P2.5 (与LCD_RW共用, LCD操作时需关中断)      ║
 *  ║  LED(D1) : P3.0 (电源指示灯, 共阳:0=亮)               ║
 *  ║  LED(D2) : P2.1 (模式指示灯)                           ║
 *  ║  按键KEY1: P1.7 (备用: 启动/停止)                       ║
 *  ║  按键KEY2: P1.6 (备用: 加速)                            ║
 *  ║  按键KEY3: P1.5 (备用: 减速)                            ║
 *  ║  按键KEY4: P1.4 (备用: 切换模式/长按设置定时)           ║
 *  ╚══════════════════════════════════════════════════════════╝
 *
 *  ╔══════════════════════════════════════════════════════════╗
 *  ║  启动前跳线帽配置 (务必遵守!)                           ║
 *  ║  段选: ON   位选: ON                                   ║
 *  ║  JP4(矩阵键盘): OFF    JP8(电机+按键): ON              ║
 *  ╚══════════════════════════════════════════════════════════╝
 *
 *  功能说明:
 *  1. 红外遥控: 电源键启停, 模式键切换风类, +/-调速, 数字键1~5直选档位
 *  2. 5档风速: 1档(微风) ~ 5档(强风), 直流电机 PWM 调速
 *  3. 3种风类: 常风(恒定转速) / 自然风(忽大忽小) / 睡眠风(逐渐降速)
 *  4. 定时关机: 遥控器设置0~60分钟定时, 支持5分钟步进调节
 *  5. 按键备用: KEY1启停, KEY2切换模式, 遥控失灵时也能操作
 *  6. LCD1602: 实时显示风速档位/风类模式/运行状态/定时剩余时间
 *
 *  定时器:
 *   T0 : 未使用
 *   T1 : 50ms 系统节拍 (定时器计数 / 自然风换档 / 睡眠风降档 / 倒计时)
 ******************************************************************************/

#include <reg51.h>
#include <intrins.h>

/* ======================== 类型定义 ======================== */
/* 为方便移植, 定义常用类型别名 */
typedef unsigned int  u16;
typedef unsigned char u8;

/* ======================== LCD1602 引脚 ======================== */
/*
 * LCD1602 使用 8 位数据接口, 数据线接 P0 口
 * RS (Register Select): 0=写命令, 1=写数据
 * RW (Read/Write)     : 0=写, 1=读 (本系统始终为写)
 * EN (Enable)         : 高脉冲将数据写入 LCD
 *
 * 注意: P2.5 同时连接蜂鸣器和 LCD_RW
 * 写 LCD 时必须关总中断(EA=0), 防止定时器 ISR 翻转 P2.5 导致 LCD 误入读模式
 */
#define LCD_DATA  P0
sbit LCD_RS = P2^6;
sbit LCD_RW = P2^5;
sbit LCD_EN = P2^7;

/* ======================== 直流电机引脚 (L298 驱动) ======================== */
/*
 * L298 双 H 桥驱动直流电机 (PWM 调速)
 * P2.0 → L298.IN1 (正转控制)
 * P2.4 → L298.IN2 (反转/刹车控制)
 * P3.1 → L298.IN3 (连接但本工程未用, 留给 B 桥)
 *
 * L298 接线:
 *   ENA → +5V (常使能, 由 IN1/IN2 控制方向)
 *   VS  → +12V (电机供电)
 *   VSS → +5V (逻辑供电)
 *   SENSE_A → GND
 *   IN4/ENB/OUT3/OUT4 → 悬空 (只用 A 桥)
 *
 * L298 A桥真值表 (ENA=1):
 *   IN1=1,IN2=0 → 电机正转
 *   IN1=0,IN2=1 → 电机反转
 *   IN1=0,IN2=0 → 电机停止 (自由停车)
 *   IN1=1,IN2=1 → 电机刹车
 *
 * 调速方式: 软件 PWM, 通过占空比控制转速
 */
sbit MOTOR_IN1 = P2^0;
sbit MOTOR_IN2 = P2^4;

/* ======================== 红外接收引脚 ======================== */
/*
 * HS0038 红外接收头, 输出脚接 P3.2 (外部中断0)
 * 收到 38KHz 红外载波时输出低电平, 无载波时输出高电平
 * 解码 NEC 协议: 引导码(9ms低+4.5ms高) + 地址码 + 地址反码 + 数据码 + 数据反码
 *
 * 注意: P3.2 独立用于红外, 按键已移至 P1.4~P1.7, 无冲突
 */
sbit IRED = P3^2;

/* ======================== 蜂鸣器 & LED 引脚 ======================== */
/*
 * 蜂鸣器: P2.5 (有源蜂鸣器, 拉低即发声, 与 LCD_RW 共用)
 * D1 LED: P3.0 (共阳, 低电平点亮, 用作电源指示灯)
 * D2 LED: P2.1 (共阳, 低电平点亮, 用作模式指示灯)
 */
sbit BEEP    = P2^5;
sbit LED_D1  = P3^0;
sbit LED_D2  = P2^1;

/* ======================== 独立按键引脚 (P1 高四位) ======================== */
/*
 * 4 个独立按键, 低电平有效, 外接 10KΩ 上拉到 VCC
 * 普中A2 上 JP8 接出 P1 高四位, 可直接插按键模块
 *
 * P1.7 → KEY1 (启动/停止)
 * P1.6 → KEY2 (风速 +1)
 * P1.5 → KEY3 (风速 -1)
 * P1.4 → KEY4 (短按切换模式 / 长按设置定时)
 *
 * Key_Scan() 返回值: 1=KEY1, 2=KEY2, 3=KEY3, 4=KEY4
 */
sbit KEY1 = P1^7;
sbit KEY2 = P1^6;
sbit KEY3 = P1^5;
sbit KEY4 = P1^4;

/* ======================== 常量定义 ======================== */

/* --- 直流电机 PWM 占空比表 --- */
/*
 * 软件 PWM 调速, 固定周期 32ms
 * pwm_on_time[i] 表示每个周期内的导通时间 (ms)
 * 值越大 → 占空比越高 → 转速越快
 *
 * 写入方式: MOTOR_IN1=1, MOTOR_IN2=0 → 电机正转
 *          MOTOR_IN1=0, MOTOR_IN2=0 → 电机停止
 *
 * 1档(微风):   1ms/8ms  ≈  12% 占空比 → 慢速
 * 5档(强风):   7ms/8ms  ≈  88% 占空比 → 快速
 */
u8 code pwm_on_time[5] = {1, 2, 4, 6, 7};
#define PWM_CYCLE  8   // PWM 周期 (ms) 125Hz

/* --- 风速档位对应的 PWM 占空比 --- */
/*
 * pwm_on_time[i] 表示第 (i+1) 档的导通时间 (ms)
 * 值越大 = 占空比越高 = 转速越快
 *
 * 档位与转速 (125Hz PWM):
 *   1档(微风):  1ms/8ms =  12% → 慢速
 *   2档        :  2ms/8ms =  25%
 *   3档(中速)  :  4ms/8ms =  50%
 *   4档        :  6ms/8ms =  75%
 *   5档(强风)  :  7ms/8ms =  88% → 快速
 */

/* --- 风类模式 --- */
#define MODE_NORMAL  0   // 常风模式: 恒定风速
#define MODE_NATURAL 1   // 自然风模式: 风速忽大忽小, 模拟自然风
#define MODE_SLEEP   2   // 睡眠风模式: 每10分钟自动降一档

/* --- 风扇状态 --- */
#define FAN_OFF  0       // 风扇关闭
#define FAN_ON   1       // 风扇运行

/* --- 系统节拍 --- */
/*
 * T1 每 50ms 产生一次中断
 * 50ms = 12 × (65536 - 定时器重装值) / 11.0592MHz
 * 重装值计算: 65536 - (50000 × 11.0592 / 12) = 65536 - 46080 = 19456 = 0x4C00
 * 实际重装值: TH1=0x4C, TL1=0x00
 */
#define T1_TH  0x4C
#define T1_TL  0x00

/* --- 定时器计数换算 --- */
/*
 * 20 个 50ms 节拍 = 1 秒
 * 1200 个 50ms 节拍 = 60 秒 = 1 分钟
 */
#define TICKS_PER_SEC   20      // 20 × 50ms = 1秒
#define TICKS_PER_MIN   1200    // 1200 × 50ms = 60秒

/* --- 红外遥控 NEC 协议键码 (普中配套 21 键遥控器) --- */
/*
 * 以下键码是 ired_data[2] (数据码) 的值
 * 如遥控器批次不同, 实际键码可能有差异, 请用串口打印验证
 */
#define IR_POWER   0x45   // 电源键 → 启动/停止
#define IR_MODE    0x46   // 模式键 → 切换风类
#define IR_MUTE    0x47   // 静音键 → (未使用)
#define IR_PLAY    0x44   // 播放键 → 进入定时设置
#define IR_PREV    0x40   // 快退键 → 定时-5分钟
#define IR_NEXT    0x43   // 快进键 → 定时+5分钟
#define IR_EQ      0x07   // EQ 键 → (未使用)
#define IR_MINUS   0x15   // 减键  → 风速-1
#define IR_PLUS    0x09   // 加键  → 风速+1
#define IR_0       0x16   // 数字0 → 取消定时
#define IR_1       0x0C   // 数字1 → 风速1档
#define IR_2       0x18   // 数字2 → 风速2档
#define IR_3       0x5E   // 数字3 → 风速3档
#define IR_4       0x08   // 数字4 → 风速4档
#define IR_5       0x1C   // 数字5 → 风速5档

/* ======================== 全局变量 ======================== */

/* --- 风扇状态 --- */
u8  fan_state;          // 当前开关状态: FAN_OFF / FAN_ON
u8  fan_mode;           // 当前风类模式: MODE_NORMAL / MODE_NATURAL / MODE_SLEEP
u8  speed_level;        // 当前风速档位: 1~5
u8  effective_speed;    // 实际 PWM 占空比索引: 0~4 (受自然风/睡眠风影响)
bit sleep_decremented;   // 睡眠风标志: 1=ISR已降过档(防竞态), 0=尚未降档

/* --- 系统节拍 --- */
u16 sys_tick;           // 系统总节拍计数 (每 50ms 加 1, 不停累加)
u16 natural_tick;       // 自然风换档计时 (每 3 秒换一次)
u16 sleep_tick;         // 睡眠风降档计时 (每 10 分钟降一档)
u16 timer_tick;         // 定时关机倒计时 (剩余 50ms 节拍数)

/* --- 红外接收 --- */
u8  ired_data[4];       // 红外接收缓冲区: [0]地址码 [1]地址反码 [2]数据码 [3]数据反码
bit ired_ready;         // 红外数据就绪标志: 1=有新数据待处理

/* --- 自然风伪随机种子 --- */
u8  natural_rand;       // 用于自然风随机风速偏移的简单种子

/* --- LCD 刷新控制 --- */
u16 lcd_refresh_tick;   // LCD 刷新倒计时节拍数

/* ======================== 函数声明 ======================== */

/* 延时 */
void Delay_ms(u16 ms);
void Delay_10us(u16 ten_us);

/* LCD1602 */
void LCD_WriteCmd(u8 cmd);
void LCD_WriteData(u8 dat);
void LCD_SetPos(u8 line, u8 col);
void LCD_WriteStr(u8 *str);
void LCD_ClearLine(u8 line);
void LCD_Init(void);

/* 直流电机 */
void Motor_Forward(void);
void Motor_Stop(void);

/* 红外遥控 */
void IR_Init(void);
void IR_Process(void);

/* 按键 */
u8   Key_Scan(void);

/* 蜂鸣器 */
void Beep_Tick(void);

/* 显示更新 */
void LCD_UpdateAll(void);

/* 系统控制 */
void Fan_Start(void);
void Fan_Stop(void);
void Fan_SpeedUp(void);
void Fan_SpeedDown(void);
void Fan_ModeSwitch(void);
void Fan_UpdateEffectiveSpeed(void);

/* ======================== 延时函数 ======================== */

/*******************************************************************************
 * 函数名  : Delay_10us
 * 功能    : 约 10us 延时 (@11.0592MHz)
 * 参数    : ten_us = 延时倍数, 1 约等于 10us
 * 返回值  : 无
 * 说明    : 用于红外解码中的精确时序等待
 *******************************************************************************/
void Delay_10us(u16 ten_us)
{
    while (ten_us--);
}

/*******************************************************************************
 * 函数名  : Delay_ms
 * 功能    : 毫秒级延时 (@11.0592MHz)
 * 参数    : ms = 延时的毫秒数
 * 返回值  : 无
 * 说明    : 软件循环延时, 约 1ms 一个循环
 *           注意: 延时期间不能响应中断(除非中断打断)
 *           仅在 LCD 初始化等非时序敏感的场合使用
 *******************************************************************************/
void Delay_ms(u16 ms)
{
    u16 i, j;
    for (i = ms; i > 0; i--)
        for (j = 120; j > 0; j--);
}

/* ======================== LCD1602 驱动 ======================== */

/*******************************************************************************
 * 函数名  : LCD_WriteCmd
 * 功能    : 向 LCD1602 写入一条指令
 * 参数    : cmd = 指令字节 (如 0x01=清屏, 0x38=8位模式, 0x0C=显示开)
 * 返回值  : 无
 * 说明    : ★关键: 写入前必须 EA=0 关总中断!
 *           因为蜂鸣器和 LCD_RW 共用 P2.5,
 *           若在 LCD 写过程中定时器 ISR 翻转 P2.5,
 *           LCD 会误认为 RW=1 (读模式), 导致显示异常.
 *           写入完毕后 EA=1 恢复中断.
 *
 *           写时序: RS=0, RW=0 → 数据放 P0 → EN 拉高 → 延时 → EN 拉低
 *******************************************************************************/
void LCD_WriteCmd(u8 cmd)
{
    EA = 0;                      // ★关总中断, 保护 LCD 写时序
    LCD_RS = 0;                  // RS=0: 选择指令寄存器
    LCD_RW = 0;                  // RW=0: 写模式
    LCD_EN = 0;                  // EN 先拉低
    LCD_DATA = cmd;              // 指令字节放到数据总线
    Delay_ms(1);                 // 等待数据稳定
    LCD_EN = 1;                  // EN 上升沿: LCD 锁存数据
    Delay_ms(1);                 // 保持高电平
    LCD_EN = 0;                  // EN 下降沿: LCD 执行指令
    LCD_DATA = 0xFF;             // ★释放 P0 口, 防止数码管段选乱码
    EA = 1;                      // ★恢复总中断
}

/*******************************************************************************
 * 函数名  : LCD_WriteData
 * 功能    : 向 LCD1602 写入一个字符数据
 * 参数    : dat = 待显示的 ASCII 字符
 * 返回值  : 无
 * 说明    : 时序与 LCD_WriteCmd 相同, 唯一区别是 RS=1 (选择数据寄存器)
 *******************************************************************************/
void LCD_WriteData(u8 dat)
{
    EA = 0;
    LCD_RS = 1;                  // RS=1: 选择数据寄存器
    LCD_RW = 0;
    LCD_EN = 0;
    LCD_DATA = dat;
    Delay_ms(1);
    LCD_EN = 1;
    Delay_ms(1);
    LCD_EN = 0;
    LCD_DATA = 0xFF;
    EA = 1;
}

/*******************************************************************************
 * 函数名  : LCD_SetPos
 * 功能    : 设置 LCD1602 光标位置
 * 参数    : line = 行号 (0=第一行, 1=第二行)
 *           col  = 列号 (0~15)
 * 返回值  : 无
 * 说明    : LCD1602 的 DDRAM 地址:
 *           第一行: 0x80 + col
 *           第二行: 0xC0 + col
 *******************************************************************************/
void LCD_SetPos(u8 line, u8 col)
{
    LCD_WriteCmd((line == 0 ? 0x80 : 0xC0) + col);
}

/*******************************************************************************
 * 函数名  : LCD_WriteStr
 * 功能    : 在当前位置连续写入一个字符串
 * 参数    : str = 指向字符串首地址的指针 (必须以 '\0' 结尾)
 * 返回值  : 无
 * 说明    : 逐字符调用 LCD_WriteData, 直到遇到字符串结束符
 *******************************************************************************/
void LCD_WriteStr(u8 *str)
{
    while (*str) {
        LCD_WriteData(*str++);
    }
}

/*******************************************************************************
 * 函数名  : LCD_ClearLine
 * 功能    : 清除 LCD1602 指定行的显示内容 (用空格填充)
 * 参数    : line = 行号 (0 或 1)
 * 返回值  : 无
 *******************************************************************************/
void LCD_ClearLine(u8 line)
{
    u8 i;
    LCD_SetPos(line, 0);
    for (i = 0; i < 16; i++) {
        LCD_WriteData(' ');
    }
}

/*******************************************************************************
 * 函数名  : LCD_Init
 * 功能    : LCD1602 初始化
 * 参数    : 无
 * 返回值  : 无
 * 说明    : 上电后必须调用此函数配置 LCD 工作模式
 *           0x38: 8位数据接口, 2行显示, 5×7点阵
 *           0x0C: 显示开, 光标关, 光标不闪烁
 *           0x06: 写入新数据后光标右移, 屏幕不移动
 *           0x01: 清屏
 *
 *           注意: LCD 上电后需要至少 15ms 稳定时间
 *           VDD 升至 4.5V 后还需额外等待, 因此在开头加了 50ms 延时
 *******************************************************************************/
void LCD_Init(void)
{
    Delay_ms(50);                // 等待 LCD 上电稳定
    LCD_WriteCmd(0x38);          // 8位接口, 2行, 5×7点阵
    LCD_WriteCmd(0x0C);          // 显示开, 无光标
    LCD_WriteCmd(0x06);          // 写入后地址自动+1
    LCD_WriteCmd(0x01);          // 清屏
    Delay_ms(5);                 // 清屏需要较长执行时间
}

/* ======================== 直流电机驱动 ======================== */

/*******************************************************************************
 * 函数名  : Motor_Forward
 * 功能    : 直流电机正转 (L298 IN1=1, IN2=0)
 * 参数    : 无
 * 返回值  : 无
 * 说明    : L298 OUT1=H, OUT2=L → 电机正转
 *******************************************************************************/
void Motor_Forward(void)
{
    MOTOR_IN1 = 1;
    MOTOR_IN2 = 0;
}

/*******************************************************************************
 * 函数名  : Motor_Stop
 * 功能    : 直流电机停止 (L298 IN1=0, IN2=0)
 * 参数    : 无
 * 返回值  : 无
 * 说明    : L298 OUT1=L, OUT2=L → 电机自由停车
 *******************************************************************************/
void Motor_Stop(void)
{
    MOTOR_IN1 = 0;
    MOTOR_IN2 = 0;
}

/* ======================== 红外遥控 NEC 协议解码 ======================== */

/*******************************************************************************
 * 函数名  : IR_Init
 * 功能    : 红外接收模块初始化
 * 参数    : 无
 * 返回值  : 无
 * 说明    : 配置外部中断 0 (P3.2) 为下降沿触发
 *           IT0=1: 下降沿触发 (红外接收头收到载波时输出低电平)
 *           EX0=1: 使能外部中断 0
 *
 *           NEC 协议要点:
 *           - 引导码: 9ms 低电平 + 4.5ms 高电平
 *           - 逻辑 "0": 560us 低 + 560us 高
 *           - 逻辑 "1": 560us 低 + 1680us 高
 *           - 数据格式: 地址码(8bit) + 地址反码(8bit) + 数据码(8bit) + 数据反码(8bit)
 *           - 反码用于校验: 数据码 = ~数据反码 才是有效帧
 *
 *           注意: 接收头对信号做了反相处理
 *           发射端发 38KHz 载波 → 接收端输出低电平
 *           发射端无载波       → 接收端输出高电平
 *******************************************************************************/
void IR_Init(void)
{
    IT0 = 1;                     // 外部中断 0 下降沿触发
    EX0 = 1;                     // 使能外部中断 0
    IRED = 1;                    // 初始化引脚为高电平
}

/*******************************************************************************
 * 函数名  : IR_ISR
 * 功能    : 外部中断 0 中断服务程序 — 红外遥控 NEC 协议解码
 * 参数    : 无 (中断函数)
 * 返回值  : 无
 * 说明    : ★这是整个红外遥控的核心, 流程如下:
 *
 *           [1] 等待引导码 9ms 低电平结束 (超时 10ms 退出)
 *           [2] 等待引导码 4.5ms 高电平结束 (超时 5ms 退出)
 *           [3] 循环 4 次, 每次读取一个字节:
 *               - 内层循环 8 次, 每次读 1 bit
 *               - 等待 560us 低电平结束 (每个 bit 的低电平)
 *               - 测量后续高电平持续时间:
 *                 - 若约 560us  → bit = 0
 *                 - 若约 1680us → bit = 1
 *               - 从 LSB (最低位) 开始组装字节
 *           [4] 校验: 数据码反码 = ~数据码 (即 ired_data[2] == ~ired_data[3])
 *           [5] 校验通过 → 置位 ired_ready, 通知主循环有新数据
 *
 *           超时保护: 每个等待循环都有最大计数限制,
 *           超过限制则直接退出, 防止干扰信号导致死循环
 *******************************************************************************/
void IR_ISR(void) interrupt 0
{
    u8  ired_high_time = 0;      // 高电平持续时间计数 (单位: 0.1ms)
    u16 time_cnt       = 0;      // 通用超时计数器
    u8  i, j;                    // 循环变量

    /* --- 阶段 1: 等待 9ms 引导码低电平 --- */
    /* 红外接收头输出低电平时, 表示正在接收 38KHz 载波 */
    if (IRED == 0) {
        time_cnt = 1000;         // 最大等待 1000 × 10us = 10ms
        while ((!IRED) && time_cnt) {
            Delay_10us(1);       // 每次延时约 10us
            time_cnt--;
            if (time_cnt == 0) return;  // 超时: 非有效信号, 退出
        }

        /* --- 阶段 2: 等待 4.5ms 引导码高电平 --- */
        if (IRED) {              // 确认低电平结束
            time_cnt = 500;      // 最大等待 500 × 10us = 5ms
            while (IRED && time_cnt) {
                Delay_10us(1);
                time_cnt--;
                if (time_cnt == 0) return;  // 超时退出
            }

            /* --- 阶段 3: 接收 4 字节数据 --- */
            for (i = 0; i < 4; i++) {
                for (j = 0; j < 8; j++) {

                    /* 等待当前 bit 的低电平 (560us) 结束 */
                    time_cnt = 600;      // 最大等待 6ms
                    while ((IRED == 0) && time_cnt) {
                        Delay_10us(1);
                        time_cnt--;
                        if (time_cnt == 0) return;
                    }

                    /* 测量后续高电平宽度 */
                    time_cnt = 20;       // 最大等待 2ms
                    while (IRED) {
                        Delay_10us(10);  // 每次延时约 0.1ms
                        ired_high_time++;
                        if (ired_high_time > 20) return;  // 高电平超过 2ms 退出
                    }

                    /* 判断 bit 值 */
                    ired_data[i] >>= 1;  // 先右移 (先收到的 bit 是 LSB)
                    if (ired_high_time >= 8) {
                        /* 高电平 > 0.8ms → bit = 1 (标准为 1.68ms) */
                        ired_data[i] |= 0x80;  // 最高位置 1
                    }
                    /* 否则 bit = 0 (标准为 0.56ms), 数据位已由右移清零 */

                    ired_high_time = 0;   // 清零, 准备下一次测量
                }
            }
        }

        /* --- 阶段 4: 校验数据 --- */
        /*
         * NEC 协议帧格式: [地址码][地址反码][数据码][数据反码]
         * 需要同时校验地址和数据:
         *   地址码 == ~地址反码  → 确认是本遥控器的信号
         *   数据码 == ~数据反码  → 确认数据完整无误
         *
         * 双重校验防止其他遥控器的信号被误识别
         */
        if (ired_data[0] != (u8)(~ired_data[1]) ||
            ired_data[2] != (u8)(~ired_data[3])) {
            /* 校验失败, 清空缓冲区, 丢弃此帧 */
            for (i = 0; i < 4; i++) ired_data[i] = 0;
            return;
        }

        /* 校验通过, 标志数据就绪 */
        ired_ready = 1;
    }
}

/*******************************************************************************
 * 函数名  : IR_Process
 * 功能    : 处理红外遥控器按下的按键
 * 参数    : 无
 * 返回值  : 无
 * 说明    : 主循环中反复调用, 检测到 ired_ready=1 时处理对应键码
 *
 *           键码映射 (ired_data[2] 值):
 *           ┌────────┬──────────┐
 *           │ 遥控键 │ 功能     │
 *           ├────────┼──────────┤
 *           │ POWER  │ 启动/停止│
 *           │ MODE   │ 切换风类 │
 *           │ VOL+   │ 风速 +1  │
 *           │ VOL-   │ 风速 -1  │
 *           │ 1~5    │ 直选档位 │
 *           │ PLAY   │ 定时设置 │
 *           │ PREV   │ 定时 -5  │
 *           │ NEXT   │ 定时 +5  │
 *           │ 0      │ 取消定时 │
 *           └────────┴──────────┘
 *******************************************************************************/
void IR_Process(void)
{
    u8 key_code;

    if (!ired_ready) return;     // 无新数据, 直接返回

    key_code = ired_data[2];    // 取出数据码 (按键编码)
    ired_ready = 0;              // 清除就绪标志

    switch (key_code) {

    case IR_POWER:               // 电源键: 启动 / 停止
        if (fan_state == FAN_OFF)
            Fan_Start();
        else
            Fan_Stop();
        break;

    case IR_MODE:                // 模式键: 切换风类
        Fan_ModeSwitch();
        break;

    case IR_PLUS:                // 加键: 风速 +1
        Fan_SpeedUp();
        break;

    case IR_MINUS:               // 减键: 风速 -1
        Fan_SpeedDown();
        break;

    case IR_1:                   // 数字键 1 → 1 档
        speed_level = 1;
        Fan_UpdateEffectiveSpeed();  // ★更新实际转速
        break;

    case IR_2:                   // 数字键 2 → 2 档
        speed_level = 2;
        Fan_UpdateEffectiveSpeed();
        break;

    case IR_3:                   // 数字键 3 → 3 档
        speed_level = 3;
        Fan_UpdateEffectiveSpeed();
        break;

    case IR_4:                   // 数字键 4 → 4 档
        speed_level = 4;
        Fan_UpdateEffectiveSpeed();
        break;

    case IR_5:                   // 数字键 5 → 5 档
        speed_level = 5;
        Fan_UpdateEffectiveSpeed();
        break;

    case IR_PLAY:                // 播放键: 进入定时设置模式
        /* 默认从 30 分钟开始, 每按一次 PREV/NEXT 增减 5 分钟 */
        if (timer_tick == 0) {
            timer_tick = 30 * TICKS_PER_MIN;  // 默认 30 分钟
        } else {
            /* 已有时钟在跑, 再按一次就取消 */
            timer_tick = 0;
        }
        break;

    case IR_PREV:                // 快退键: 定时 -5 分钟
        if (timer_tick >= 5 * TICKS_PER_MIN)
            timer_tick -= 5 * TICKS_PER_MIN;
        else
            timer_tick = 0;
        break;

    case IR_NEXT:                // 快进键: 定时 +5 分钟
        if (timer_tick < 60 * TICKS_PER_MIN)
            timer_tick += 5 * TICKS_PER_MIN;
        break;

    case IR_0:                   // 数字键 0: 取消定时
        timer_tick = 0;
        break;

    default:
        ired_ready = 0;          // 未知按键, 清标志, 不做反馈
        return;
    }

    /* 有效按键: 蜂鸣一声 + 立即刷新 LCD */
    Beep_Tick();
    lcd_refresh_tick = 1;
}

/* ======================== 独立按键扫描 ======================== */

/*******************************************************************************
 * 函数名  : Key_Scan
 * 功能    : 扫描 4 个独立按键 (KEY1~KEY4), 带消抖
 * 参数    : 无
 * 返回值  : 0=无按键, 1=KEY1, 2=KEY2, 3=KEY3, 4=KEY4
 * 说明    : 按下→消抖→确认→等待释放→返回
 *           优先级: KEY1 > KEY2 > KEY3 > KEY4
 *******************************************************************************/
u8 Key_Scan(void)
{
    if (KEY1 == 0) {
        Delay_ms(20);
        if (KEY1 == 0) {
            while (KEY1 == 0);
            Delay_ms(20);
            return 1;
        }
    }
    if (KEY2 == 0) {
        Delay_ms(20);
        if (KEY2 == 0) {
            while (KEY2 == 0);
            Delay_ms(20);
            return 2;
        }
    }
    if (KEY3 == 0) {
        Delay_ms(20);
        if (KEY3 == 0) {
            while (KEY3 == 0);
            Delay_ms(20);
            return 3;
        }
    }
    if (KEY4 == 0) {
        Delay_ms(20);
        if (KEY4 == 0) {
            while (KEY4 == 0);
            Delay_ms(20);
            return 4;
        }
    }
    return 0;
}

/* ======================== 蜂鸣器控制 ======================== */

/*******************************************************************************
 * 函数名  : Beep_Tick
 * 功能    : 短促提示音 (无源蜂鸣器, 2kHz 方波驱动 100ms)
 * 参数    : 无
 * 返回值  : 无
 * 说明    : 无源蜂鸣器需方波驱动: P2.5 以 2kHz 翻转 100ms
 *           Delay_10us(25) ≈ 0.25ms → 半周期 0.25ms → 频率 2kHz
 *           200 次翻转 × 0.5ms = 100ms 时长
 *
 *           结束时 BEEP=0 (LCD_RW=0 写模式)
 *           ★在按键/遥控操作后调用, 必须在 LCD 操作间隙调用
 *******************************************************************************/
void Beep_Tick(void)
{
    u8 i;
    for (i = 0; i < 200; i++) {
        BEEP = !BEEP;
        Delay_10us(25);          // ~0.25ms → 2kHz
    }
    BEEP = 0;                    // 200次翻转(偶数)回到0 → RW=0 写模式
}

/* ======================== 风扇系统控制逻辑 ======================== */

/*******************************************************************************
 * 函数名  : Fan_Start
 * 功能    : 启动风扇
 * 参数    : 无
 * 返回值  : 无
 * 说明    : 将风扇状态设为 ON, 点亮 D1 电源指示灯
 *           重置自然风计时和睡眠风计时
 *******************************************************************************/
void Fan_Start(void)
{
    fan_state          = FAN_ON;
    LED_D1             = 0;           // D1 亮 (共阳, 低电平亮)
    natural_tick       = 0;           // 重置自然风计时
    sleep_tick         = 0;           // 重置睡眠风计时
    natural_rand       = 1;           // 初始化随机种子
    sleep_decremented  = 0;           // ★重置睡眠风降档标志
    Fan_UpdateEffectiveSpeed();       // 计算初始有效速度
}

/*******************************************************************************
 * 函数名  : Fan_Stop
 * 功能    : 停止风扇
 * 参数    : 无
 * 返回值  : 无
 * 说明    : 关闭风扇, 熄灭所有 LED, 停止电机
 *******************************************************************************/
void Fan_Stop(void)
{
    fan_state = FAN_OFF;
    LED_D1    = 1;               // D1 灭 (共阳, 高电平灭)
    LED_D2    = 1;               // D2 灭
    Motor_Stop();
}

/*******************************************************************************
 * 函数名  : Fan_SpeedUp
 * 功能    : 风速升一档 (最大 5 档)
 * 参数    : 无
 * 返回值  : 无
 *******************************************************************************/
void Fan_SpeedUp(void)
{
    if (speed_level < 5) {
        speed_level++;
        Fan_UpdateEffectiveSpeed();  // ★必须更新实际转速, 否则电机速度不会变
    }
}

/*******************************************************************************
 * 函数名  : Fan_SpeedDown
 * 功能    : 风速降一档 (最小 1 档)
 * 参数    : 无
 * 返回值  : 无
 *******************************************************************************/
void Fan_SpeedDown(void)
{
    if (speed_level > 1) {
        speed_level--;
        Fan_UpdateEffectiveSpeed();  // ★必须更新实际转速
    }
}

/*******************************************************************************
 * 函数名  : Fan_ModeSwitch
 * 功能    : 切换风类模式: 常风 → 自然风 → 睡眠风 → 常风 (循环)
 * 参数    : 无
 * 返回值  : 无
 *******************************************************************************/
void Fan_ModeSwitch(void)
{
    fan_mode++;
    if (fan_mode > MODE_SLEEP) fan_mode = MODE_NORMAL;
    natural_tick       = 0;      // 重置自然风计时
    sleep_tick         = 0;      // 重置睡眠风计时
    sleep_decremented  = 0;      // ★重置睡眠风降档标志
    Fan_UpdateEffectiveSpeed();
}

/*******************************************************************************
 * 函数名  : Fan_UpdateEffectiveSpeed
 * 功能    : 根据当前模式和风速档位, 计算实际生效的电机延时索引
 * 参数    : 无
 * 返回值  : 无
 * 说明    : effective_speed 是 pwm_on_time[] 数组的索引 (0~4)
 *           常风模式: 直接使用用户设定档位
 *           自然风模式: 由 natural_rand 决定偏移量
 *           睡眠风模式: 使用当前降速后的值
 *******************************************************************************/
void Fan_UpdateEffectiveSpeed(void)
{
    u8 base;

    switch (fan_mode) {

    case MODE_NORMAL:
        /* 常风: 直接映射用户档位 */
        effective_speed = speed_level - 1;
        break;

    case MODE_NATURAL:
        /* 自然风: 在基础档位附近波动 ±2 档, 模拟自然风忽大忽小 */
        base = speed_level - 1;  // 转为 0-based 索引
        /* 用简单算法产生 -2 ~ +2 的偏移 */
        if (natural_rand < 51) {
            effective_speed = base;           // 40% 概率: 不变
        } else if (natural_rand < 77) {
            effective_speed = (base >= 1) ? base - 1 : 0;      // 20%: -1档
        } else if (natural_rand < 103) {
            effective_speed = (base < 4) ? base + 1 : 4;       // 20%: +1档
        } else if (natural_rand < 116) {
            effective_speed = (base >= 2) ? base - 2 : 0;      // 10%: -2档
        } else {
            effective_speed = (base < 3) ? base + 2 : 4;       // 10%: +2档
        }
        /* 边界夹紧到 0~4 之间 */
        if (effective_speed > 4) effective_speed = 4;
        break;

    case MODE_SLEEP:
        /* 睡眠风: 初始转速同常风, 由定时器 ISR 逐步降低 */
        /* ★使用 sleep_decremented 标志而非 sleep_tick==0 判断,
         *   防止 ISR 刚降完档(此时 sleep_tick==0)但 Fan_UpdateEffectiveSpeed
         *   被主循环调用时又重置为初始转速的竞态问题 */
        if (!sleep_decremented && sleep_tick == 0) {
            effective_speed = speed_level - 1;
        }
        /* 若 sleep_decremented==1, 说明 ISR 已修改过 effective_speed, 保持 */
        break;
    }
}

/* ======================== LCD 显示更新 ======================== */

/*******************************************************************************
 * 函数名  : LCD_UpdateAll
 * 功能    : 刷新 LCD1602 两行显示内容
 * 参数    : 无
 * 返回值  : 无
 * 说明    : 第一行: 风速档位 + 风类模式 + 运行状态
 *           第二行: 定时剩余时间
 *
 *           显示格式示例:
 *           第一行: "S:3 NORMAL  ON "  (Speed:3, Normal模式, 运行中)
 *           第一行: "S:5 NATURAL ON "  (Speed:5, 自然风模式, 运行中)
 *           第一行: "S:1 SLEEP   ON "  (Speed:1, 睡眠风模式, 运行中)
 *           第一行: "  FAN STOPPED  "  (风扇已关闭)
 *
 *           第二行: "Timer:30m      "  (定时剩余 30 分钟)
 *           第二行: "Timer:OFF      "  (未设置定时)
 *
 *           注意: 此函数在 LCD 写数据时会关总中断,
 *           因此调用前应确保不处于时间敏感的代码段中
 *******************************************************************************/
void LCD_UpdateAll(void)
{
    u8 min;
    u8  *mode_str;

    /* ===== 第一行: 风速 + 模式 + 状态 ===== */
    LCD_SetPos(0, 0);

    if (fan_state == FAN_OFF) {
        /* 风扇关闭时的显示 */
        LCD_WriteStr("  FAN STOPPED  ");
    } else {
        /* 风扇运行时显示: S:档位 + 模式名 + ON */
        LCD_WriteStr("S:");
        LCD_WriteData('0' + speed_level);  // 当前风速档位 (1~5)

        LCD_WriteStr(" ");

        /* 模式名称映射 */
        switch (fan_mode) {
            case MODE_NORMAL:  mode_str = "NORMAL ";  break;
            case MODE_NATURAL: mode_str = "NATURAL";  break;
            case MODE_SLEEP:   mode_str = "SLEEP  ";  break;
            default:           mode_str = "NORMAL ";  break;
        }
        LCD_WriteStr(mode_str);

        LCD_WriteStr(" ON ");
    }

    /* ===== 第二行: 定时信息 ===== */
    LCD_SetPos(1, 0);
    LCD_WriteStr("Timer:");

    if (timer_tick == 0) {
        LCD_WriteStr("OFF     ");
    } else {
        /* 将剩余节拍数转换为分钟数显示 */
        min = timer_tick / TICKS_PER_MIN;
        if (min >= 10) {
            LCD_WriteData('0' + (min / 10) % 10);
        } else {
            LCD_WriteData('0');
        }
        LCD_WriteData('0' + min % 10);
        LCD_WriteStr("m     ");
    }
}

/* ======================== 定时器 1 中断服务程序 ======================== */

/*******************************************************************************
 * 函数名  : Timer1_ISR
 * 功能    : 定时器 1 中断服务程序 — 50ms 系统节拍
 * 参数    : 无 (中断函数)
 * 返回值  : 无
 * 说明    : ★这是整个系统的时间基准, 每 50ms 触发一次
 *
 *           处理以下任务:
 *           1. 系统总节拍计数 sys_tick (不停累加, 可溢出)
 *           2. 定时关机倒计时: timer_tick > 0 时递减
 *           3. 自然风模式: 每 3 秒重新生成随机风速偏移
 *           4. 睡眠风模式: 每 10 分钟降低一档风速
 *           5. LCD 刷新周期控制: 每 0.5 秒刷新一次
 *
 *           定时器重装值:
 *           50ms = 12 × (65536 - 重装值) / 11059200
 *           重装值 = 65536 - 46080 = 19456 = 0x4C00
 *
 *           ★中断中只做轻量级的计数和标志操作,
 *           耗时操作(如 LCD 刷新)放在主循环中根据标志执行
 *******************************************************************************/
void Timer1_ISR(void) interrupt 3
{
    /* 重装定时器初值 (模式 1 不会自动重装, 需手动设置) */
    TH1 = T1_TH;
    TL1 = T1_TL;

    /* (1) 系统总节拍计数 */
    sys_tick++;

    /* (2) 定时关机倒计时 */
    if (timer_tick > 0) {
        timer_tick--;
        if (timer_tick == 0) {
            /* 定时时间到, 自动关闭风扇 */
            Fan_Stop();
        }
    }

    /* (3) 自然风模式: 每 3 秒换一次风速 */
    if (fan_state == FAN_ON && fan_mode == MODE_NATURAL) {
        natural_tick++;
        if (natural_tick >= 3 * TICKS_PER_SEC) {  // 3 秒到
            natural_tick = 0;
            /* 简单伪随机: 用系统节拍的低位生成种子 */
            natural_rand = (u8)(sys_tick & 0x7F);  // 0~127
            Fan_UpdateEffectiveSpeed();
        }
    }

    /* (4) 睡眠风模式: 每 10 分钟降一档风速 */
    if (fan_state == FAN_ON && fan_mode == MODE_SLEEP) {
        sleep_tick++;
        /* 10 分钟 = 10 × 60 × 20 = 12000 个节拍 */
        if (sleep_tick >= 10 * TICKS_PER_MIN) {
            sleep_tick = 0;
            if (effective_speed > 0) {
                /* 尚未降到最低档, 继续降一档 (减小占空比) */
                effective_speed--;
                sleep_decremented = 1;  // ★标记: ISR 已修改转速, 防止主循环重置
            }
        }
    }

    /* (5) LCD 刷新周期: 每 0.5 秒刷新一次显示屏 */
    if (lcd_refresh_tick > 0) {
        lcd_refresh_tick--;
    }
}

/* ======================== 主函数 ======================== */

/*******************************************************************************
 * 函数名  : main
 * 功能    : 程序入口, 系统初始化与主循环
 * 参数    : 无
 * 返回值  : 无
 *
 * 说明    : 初始化顺序:
 *           [1] 端口初始化 (P0~P3 设为高电平 / 输入模式)
 *           [2] 变量初始化 (风扇状态 / 速度 / 模式等)
 *           [3] 定时器 1 初始化 (50ms 系统节拍)
 *           [4] LCD1602 初始化
 *           [5] 红外接收初始化
 *           [6] 开总中断 (EA=1)
 *           [7] 进入主循环
 *
 *           主循环流程:
 *           while(1) {
 *               (a) 检测并处理红外遥控按键
 *               (b) 检测并处理独立按键 (KEY1~KEY4)
 *               (c) 若风扇运行, 直流电机 PWM 驱动
 *               (d) 若 LCD 刷新时间到, 更新显示
 *           }
 *******************************************************************************/
void main(void)
{
    u8  key_val;

    /* ===== [1] 端口初始化 ===== */
    P0 = 0xFF;                   // P0 全高 (释放数据总线, 防止数码管乱码)
    P1 = 0xF0;                   // P1 高 4 位=1 (按键上拉输入), 低 4 位=0
    P2 = 0xFF;                   // P2 LCD/蜂鸣器就绪, 电机引脚初始高
    P3 = 0xFF;                   // P3 全高 (D1灭, 红外就绪, IN3 高)
    MOTOR_IN1 = 0;               // L298.IN1=0, 确保启动时电机停止
    MOTOR_IN2 = 0;               // L298.IN2=0

    /* ===== [2] 变量初始化 ===== */
    fan_state        = FAN_OFF;    // 初始状态: 风扇关闭
    fan_mode         = MODE_NORMAL;// 初始模式: 常风
    speed_level      = 1;          // 默认 1 档 (微风)
    effective_speed  = 0;          // 对应 pwm_on_time[0] = 1ms
    sys_tick         = 0;
    natural_tick     = 0;
    sleep_tick       = 0;
    timer_tick       = 0;
    ired_ready       = 0;
    natural_rand     = 1;
    lcd_refresh_tick = 0;

    LED_D1 = 1;                  // D1(P3.0) 初始灭
    LED_D2 = 1;                  // D2(P2.1) 初始灭

    /* ===== [3] 定时器 1 初始化 (50ms 节拍) ===== */
    /*
     * TMOD 配置:
     * 定时器 1 使用模式 1 (16 位定时器)
     * GATE=0 (软件启动), C/T=0 (定时模式)
     * TMOD = (TMOD & 0x0F) | 0x10
     *
     * 定时器 0 不使用, 保持默认
     */
    TMOD &= 0x0F;                // 清除 T1 配置位
    TMOD |= 0x10;                // T1=模式1 (16位定时器)
    TH1 = T1_TH;                 // 装载初值高字节
    TL1 = T1_TL;                 // 装载初值低字节
    ET1 = 1;                     // 使能 T1 中断
    TR1 = 1;                     // 启动 T1

    /* ===== [4] LCD1602 初始化 ===== */
    /*
     * 注意: LCD_Init 中包含延时, 此时总中断尚未开启,
     * 定时器 ISR 不会干扰 LCD 初始化过程
     */
    LCD_Init();

    /* ===== [5] 红外接收初始化 ===== */
    IR_Init();

    /* ===== [6] 开启总中断 ===== */
    EA = 1;                      // ★必须在所有初始化完成后才开中断

    /* ===== [7] 开机画面 ===== */
    LCD_SetPos(0, 0);
    LCD_WriteStr("Smart Fan v1.0 ");
    LCD_SetPos(1, 0);
    LCD_WriteStr("IR Remote Fan  ");
    Delay_ms(1500);              // 开机画面显示 1.5 秒

    LCD_ClearLine(0);
    LCD_ClearLine(1);
    LCD_UpdateAll();             // 显示初始状态
    lcd_refresh_tick = 10;       // 首次 0.5 秒后刷新

    /* ══════════════════════════════════════════════════
     *  主循环
     * ══════════════════════════════════════════════════ */
    while (1) {

        /* --- (a) 处理红外遥控输入 --- */
        /*
         * IR_Process() 检测 ired_ready 标志,
         * 若有新的红外数据帧, 解析并执行对应操作
         */
        IR_Process();

        /* --- (b) 处理独立按键 --- */
        key_val = Key_Scan();
        switch (key_val) {
        case 1:  /* KEY1: 启动/停止 */
            if (fan_state == FAN_OFF)
                Fan_Start();
            else
                Fan_Stop();
            Beep_Tick();
            lcd_refresh_tick = 1;
            break;
        case 2:  /* KEY2: 风速 +1 */
            Fan_SpeedUp();
            Beep_Tick();
            lcd_refresh_tick = 1;
            break;
        case 3:  /* KEY3: 风速 -1 */
            Fan_SpeedDown();
            Beep_Tick();
            lcd_refresh_tick = 1;
            break;
        default:
            break;
        }

        /* --- (b2) KEY4 独立处理: 短按切换模式 / 长按设置定时 --- */
        {
            static u16  key4_down_start = 0;
            static bit  key4_was_down  = 0;
            static bit  key4_long_done = 0;

            if (KEY4 == 0) {
                if (!key4_was_down) {
                    key4_down_start = sys_tick;
                    key4_long_done  = 0;
                    key4_was_down   = 1;
                }
                if (!key4_long_done &&
                    (u16)(sys_tick - key4_down_start) >= 40) {
                    key4_long_done = 1;
                    Fan_SetTimer();
                    Beep_Tick();
                    Beep_Tick();
                    lcd_refresh_tick = 1;
                    while (KEY4 == 0);
                    key4_was_down = 0;
                }
            } else {
                if (key4_was_down && !key4_long_done) {
                    Fan_ModeSwitch();
                    Beep_Tick();
                    lcd_refresh_tick = 1;
                }
                key4_was_down = 0;
            }
        }

        /* --- (c) 风扇运行时驱动直流电机 (PWM) --- */
        if (fan_state == FAN_ON) {
            /*
             * 软件 PWM 调速: 固定周期 8ms (125Hz)
             * 导通时间 = pwm_on_time[effective_speed]
             * 占空比越高 → 转速越快
             *
             *   1档: 1ms/8ms = 12% → 慢速
             *   5档: 7ms/8ms = 88% → 快速
             *
             * 阻塞期间定时器 ISR 仍在运行:
             * - 系统节拍正常累加
             * - 自然风/睡眠风定时换档正常
             * - 红外遥控由 INT0 中断处理, 不受影响
             */
            Motor_Forward();
            Delay_ms(pwm_on_time[effective_speed]);
            Motor_Stop();
            Delay_ms(PWM_CYCLE - pwm_on_time[effective_speed]);

            /* 睡眠风: D2 每 0.5s 闪烁一次 (用 sys_tick 节拍) */
            if (fan_mode == MODE_SLEEP) {
                LED_D2 = (sys_tick / 10) % 2 ? 0 : 1;
            }

            /* 自然风: D2 用 natural_tick 控制闪烁节奏, 每秒亮1秒/周期3秒 */
            if (fan_mode == MODE_NATURAL) {
                LED_D2 = (natural_tick < TICKS_PER_SEC) ? 0 : 1;
            }

        } else {
            /* 风扇关闭时加一个小延时, 防止主循环空转占满 CPU */
            Delay_ms(10);
        }

        /* --- (d) LCD 定时刷新 --- */
        /*
         * 由定时器 ISR 每 50ms 递减 lcd_refresh_tick,
         * 当减到 0 时刷新 LCD 显示
         * (LCD 刷新太频繁会闪烁, 0.5 秒一次刚好)
         */
        if (lcd_refresh_tick == 0) {
            LCD_UpdateAll();
            lcd_refresh_tick = 10;   // 10 × 50ms = 0.5 秒后再次刷新
        }

        /*
         * --- 常风模式指示灯 ---
         * 常风模式: D2 常亮 (表示稳定运行)
         */
        if (fan_state == FAN_ON && fan_mode == MODE_NORMAL) {
            LED_D2 = 0;          // D2 亮
        }

    } /* end while(1) */
}
