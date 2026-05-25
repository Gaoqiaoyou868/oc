/******************************************************************************
 * 智能风扇系统 — Proteus 仿真版  (Keil C51)  — v1.0
 *
 * 芯片    : AT89C52 (Proteus 中仿真)  @ 11.0592 MHz
 * 编译环境: Keil uVision 5
 * 仿真软件: Proteus 8.x
 *
 *  ╔══════════════════════════════════════════════════════════╗
 *  ║  引脚映射 (Proteus 仿真)                                 ║
 *  ║  LCD1602 : RS=P2.6, RW=P2.5, EN=P2.7, D0~D7=P0       ║
 *  ║  步进电机: P1.0~P1.3 → ULN2003D → MOTOR-STEPPER   ║
 *  ║  按键KEY1: P1.7 (启动/停止)                             ║
 *  ║  按键KEY2: P1.6 (风速 +1)                               ║
 *  ║  按键KEY3: P1.5 (风速 -1)                               ║
 *  ║  按键KEY4: P1.4 (短按=切换模式, 长按2秒=循环切换定时    ║
 *  ║                   15分→30分→45分→60分→关闭)            ║
 *  ║  LED(D1) : P3.0 (电源指示灯, 共阳:0=亮)               ║
 *  ║  LED(D2) : P2.1 (模式指示灯)                           ║
 *  ║  蜂鸣器  : P2.5 → SPEAKER (无源, 2kHz方波提示音)       ║
 *  ╚══════════════════════════════════════════════════════════╝
 *
 *  ╔══════════════════════════════════════════════════════════╗
 *  ║  Proteus 仿真接线说明                                    ║
 *  ║  ── LCD1602 ──                                          ║
 *  ║    LM016L 元件, D0~D7 接 P0                             ║
 *  ║    RS=P2.6, RW=P2.5, EN=P2.7                            ║
 *  ║    VDD→+5V, VSS→GND, VEE→10KΩ电位器中点(调对比度)      ║
 *  ║                                                         ║
 *  ║  ── 步进电机 + ULN2003D 驱动 ──                         ║
 *  ║    元件: ULN2003A (Darlington 驱动阵列)                  ║
 *  ║    P1.0 → ULN2003.IN1 → OUT1 → 步进电机 A 相           ║
 *  ║    P1.1 → ULN2003.IN2 → OUT2 → 步进电机 B 相           ║
 *  ║    P1.2 → ULN2003.IN3 → OUT3 → 步进电机 C 相           ║
 *  ║    P1.3 → ULN2003.IN4 → OUT4 → 步进电机 D 相           ║
 *  ║    ULN2003.COM → +5V (续流二极管公共端)                 ║
 *  ║    ULN2003.GND → GND                                    ║
 *  ║    8 拍半步序列: A→AB→B→BC→C→CD→D→DA                 ║
 *  ║                                                         ║
 *  ║  ── 按键 ──                                              ║
 *  ║    4个 BUTTON 元件, 接 P1.7/P1.6/P1.5/P1.4             ║
 *  ║    一端接单片机引脚, 另一端接地                           ║
 *  ║    每个按键引脚需外接 10KΩ 上拉电阻到 +5V               ║
 *  ║                                                         ║
 *  ║  ── 指示灯 ──                                            ║
 *  ║    D1(P3.0) 和 D2(P2.1), 串 220Ω 限流电阻到 +5V         ║
 *  ║    共阳接法: 引脚=0 时亮  
                              ║
 *  ║                                                         ║
 *  ║  ── 蜂鸣器 ──                                            ║
 *  ║    SPEAKER 无源, P2.5 → 1KΩ → NPN基极                  ║
 *  ║    NPN发射极→GND, NPN集电极→SPEAKER(-), SPEAKER(+)→+5V  ║
 *  ║    (P2.5 与 LCD RW 共用, 写 LCD 时必须关中断)           ║
 *  ║                                                         ║
 *  ║  ── 晶振 ──                                              ║
 *  ║    11.0592MHz + 两个 30pF 电容接地                      ║
 *  ║                                                         ║
 *  ║  ── 复位 ──                                              ║
 *  ║    10KΩ 上拉到 VCC + 10uF 电容到 GND + 复位按键到 VCC  ║
 *  ╚══════════════════════════════════════════════════════════╝
 *
 *  功能说明:
 *  1. 按键控制: KEY1启停, KEY2加速, KEY3减速, KEY4切换模式/设置定时
 *  2. 5档风速: 1档(微风) ~ 5档(强风), 步进电机转速随之变化
 *  3. 3种风类: 常风/自然风(忽大忽小)/睡眠风(每10分钟降一档)
 *  4. 定时关机: KEY4 长按 2 秒, 循环切换 15分→30分→45分→60分→关闭
 *  5. LCD1602: 实时显示风速档位/风类模式/运行状态/定时剩余时间
 ******************************************************************************/

#include <reg51.h>
#include <intrins.h>

/* ======================== 类型定义 ======================== */
typedef unsigned int  u16;
typedef unsigned char u8;

/* ======================== LCD1602 引脚 ======================== */
/*
 * LCD1602 使用 8 位数据接口
 * RS=P2.6: 寄存器选择 (0=命令, 1=数据)
 * RW=P2.5: 读写选择 (0=写, 1=读) — 本系统始终为写
 * EN=P2.7: 使能信号 (高脉冲写入)
 * D0~D7=P0: 8 位数据总线
 *
 * 警告: P2.5 同时连接蜂鸣器, LCD 写操作时必须关中断
 */
#define LCD_DATA  P0
sbit LCD_RS = P2^6;
sbit LCD_RW = P2^5;
sbit LCD_EN = P2^7;

/* ======================== 步进电机 (ULN2003D 驱动) ======================== */
/*
 * ULN2003D Darlington 阵列驱动 28BYJ-48 步进电机
 * P1.0 → ULN2003.IN1 → OUT1 → 电机 A 相
 * P1.1 → ULN2003.IN2 → OUT2 → 电机 B 相
 * P1.2 → ULN2003.IN3 → OUT3 → 电机 C 相
 * P1.3 → ULN2003.IN4 → OUT4 → 电机 D 相
 *
 * 8 拍半步序列: A → AB → B → BC → C → CD → D → DA
 * 写 P1 时用 & 0xF0 保护高4位 (独立按键)
 */
u8 code step_phase[8] = {0x01, 0x03, 0x02, 0x06, 0x04, 0x0C, 0x08, 0x09};

/* ======================== 按键引脚 (P1 高四位) ======================== */
/*
 * 4 个独立按键, 低电平有效 (按下时引脚经按键接地)
 * 每个按键需外接 10KΩ 上拉电阻到 +5V
 *
 * 使用 P1 高四位 (P1.4~P1.7) 作为按键输入:
 *   P1.7 → KEY1  (启动/停止)
 *   P1.6 → KEY2  (风速 +1)
 *   P1.5 → KEY3  (风速 -1)
 *   P1.4 → KEY4  (短按切换模式 / 长按设置定时)
 *
 * 按键扫描函数 Key_Scan() 返回 1/2/3/4 对应 KEY1~KEY4
 *
 * Proteus 接线:
 *   每个 BUTTON 元件: 一端接单片机引脚, 另一端接地
 *   引脚处外接 10KΩ 上拉电阻到 +5V
 */
sbit KEY1 = P1^7;    // 启动 / 停止
sbit KEY2 = P1^6;    // 风速 +1
sbit KEY3 = P1^5;    // 风速 -1
sbit KEY4 = P1^4;    // 短按=切换模式, 长按2秒=设置/取消定时

/* ======================== LED 和蜂鸣器引脚 ======================== */
/*
 * LED: 共阳接法 (低电平点亮)
 * D1(P3.0): 电源指示灯
 * D2(P2.1): 模式指示灯 (自然风/睡眠风时闪烁)
 *
 * 蜂鸣器: P2.5 (无源蜂鸣器, 2kHz方波驱动, 与 LCD_RW 共用)
 */
sbit LED_D1 = P3^0;
sbit LED_D2 = P2^1;
sbit BEEP   = P2^5;

/* ======================== 常量定义 ======================== */

/* --- 步进电机速率累积表 --- */
/*
 * step_rate[i] 表示第 (i+1) 档每 50ms 节拍的累积步进量
 * 原理: ISR 每 50ms 将 step_acc += step_rate[档位]
 *       当 step_acc >= 50 时, 步进一次, step_acc -= 50
 *
 * 计算: 2500 / 步间延时(ms) = rate
 *   1档(微风): 2500/8  = 312  → 约 8ms/步
 *   2档       : 2500/6  = 416  → 约 6ms/步
 *   3档(中速) : 2500/4  = 625  → 约 4ms/步
 *   4档       : 2500/2  = 1250 → 约 2ms/步
 *   5档(强风) : 2500/1  = 2500 → 约 1ms/步
 */
u16 code step_rate[5] = {312, 416, 625, 1250, 2500};

/* --- 风类模式 --- */
#define MODE_NORMAL  0   // 常风: 恒定转速
#define MODE_NATURAL 1   // 自然风: 转速忽大忽小
#define MODE_SLEEP   2   // 睡眠风: 逐渐降速

/* --- 风扇状态 --- */
#define FAN_OFF  0
#define FAN_ON   1

/* --- 系统节拍 (50ms) --- */
/*
 * 定时器 1 工作在模式 1 (16位), 50ms 周期
 *
 * 重装值计算:
 *   机器周期 = 12 / 11.0592MHz ≈ 1.085us
 *   计数值   = 50ms / 1.085us ≈ 46080
 *   重装值   = 65536 - 46080 = 19456 = 0x4C00
 */
#define T1_TH  0x4C
#define T1_TL  0x00

/* --- 定时换算 --- */
#define TICKS_PER_SEC   20      // 1秒 = 20 × 50ms
#define TICKS_PER_MIN   1200    // 1分钟 = 60秒 × 20

/* ======================== 全局变量 ======================== */

/* 风扇状态 */
u8  fan_state;          // FAN_OFF / FAN_ON
u8  fan_mode;           // MODE_NORMAL / MODE_NATURAL / MODE_SLEEP
u8  speed_level;        // 用户设定风速: 1~5
u8  effective_speed;    // 实际步进延时索引: 0~4
bit sleep_decremented;   // 睡眠风标志: 1=ISR已降过档, 0=尚未
/* 系统计时 */
u16 sys_tick;           // 系统 50ms 节拍累加器 (可溢出)
u16 natural_tick;       // 自然风换档倒计时
u16 sleep_tick;         // 睡眠风降档倒计时
u16 timer_tick;         // 定时关机倒计时 (单位: 50ms节拍)

/* 步进电机速率累积器 (ISR 每50ms加 step_rate) */
volatile u16 step_acc;

/* 自然风随机种子 */
u8  natural_rand;

/* LCD 刷新控制 */
u16 lcd_refresh_tick;

/* ======================== 函数声明 ======================== */

void Delay_ms(u16 ms);
void Delay_10us(u16 ten_us);
void LCD_WriteCmd(u8 cmd);
void LCD_WriteData(u8 dat);
void LCD_SetPos(u8 line, u8 col);
void LCD_WriteStr(u8 *str);
void LCD_ClearLine(u8 line);
void LCD_Init(void);
void Motor_Forward(void);
void Motor_Stop(void);
u8   Key_Scan(void);
void Beep_Tick(void);
void LCD_UpdateAll(void);
void Fan_Start(void);
void Fan_Stop(void);
void Fan_SpeedUp(void);
void Fan_SpeedDown(void);
void Fan_ModeSwitch(void);
void Fan_UpdateEffectiveSpeed(void);
void Fan_SetTimer(void);

/* ======================== 延时函数 ======================== */

/*******************************************************************************
 * 函数名  : Delay_10us
 * 功能    : 约 10us 延时 (@11.0592MHz)
 * 参数    : ten_us = 倍数 (1 ≈ 10us)
 * 返回值  : 无
 *******************************************************************************/
void Delay_10us(u16 ten_us)
{
    while (ten_us--);
}

/*******************************************************************************
 * 函数名  : Delay_ms
 * 功能    : 毫秒级延时 (@11.0592MHz)
 * 参数    : ms = 毫秒数
 * 返回值  : 无
 * 说明    : 软件循环延时, 约 1ms/循环
 *           注意: 延时期间不能做其他事情
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
 * 功能    : 向 LCD1602 写一条指令
 * 参数    : cmd = 指令码 (如 0x01=清屏, 0x38=8位模式)
 * 返回值  : 无
 * 说明    : ★关键! 写 LCD 前必须 EA=0
 *           因为蜂鸣器与 LCD_RW 共用 P2.5,
 *           若中断触发时正好在写 LCD, P2.5 被翻转会导致 LCD 显示异常
 *
 *           写时序 (8080 并行接口):
 *           RS=0(命令), RW=0(写) → 数据放 P0 → EN上升沿锁存 → EN下降沿执行
 *
 *           写完后 P0=0xFF 释放总线, 避免影响其他共用 P0 的设备
 *******************************************************************************/
void LCD_WriteCmd(u8 cmd)
{
    EA = 0;                      // ★关中断! 防止蜂鸣器 ISR 干扰
    LCD_RS = 0;                  // 选择指令寄存器
    LCD_RW = 0;                  // 选择写模式
    LCD_EN = 0;
    LCD_DATA = cmd;              // 指令放到 P0 总线
    Delay_ms(1);                 // 等待数据稳定
    LCD_EN = 1;                  // EN 拉高 → LCD 锁存数据
    Delay_ms(1);
    LCD_EN = 0;                  // EN 拉低 → LCD 执行指令
    LCD_DATA = 0xFF;             // ★释放 P0 总线
    EA = 1;                      // ★恢复中断
}

/*******************************************************************************
 * 函数名  : LCD_WriteData
 * 功能    : 向 LCD1602 写一个字符的 ASCII 码
 * 参数    : dat = 要显示的字符 (ASCII)
 * 返回值  : 无
 * 说明    : 与写命令的唯一区别: RS=1 (选择数据寄存器)
 *******************************************************************************/
void LCD_WriteData(u8 dat)
{
    EA = 0;
    LCD_RS = 1;                  // 选择数据寄存器
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
 * 功能    : 设置 LCD 光标位置
 * 参数    : line=行号(0/1), col=列号(0~15)
 * 返回值  : 无
 * 说明    : DDRAM 地址: 第一行 0x80, 第二行 0xC0
 *******************************************************************************/
void LCD_SetPos(u8 line, u8 col)
{
    LCD_WriteCmd((line == 0 ? 0x80 : 0xC0) + col);
}

/*******************************************************************************
 * 函数名  : LCD_WriteStr
 * 功能    : 在当前位置显示一个字符串
 * 参数    : str = 字符串指针 (以 '\0' 结尾)
 * 返回值  : 无
 *******************************************************************************/
void LCD_WriteStr(u8 *str)
{
    while (*str) {
        LCD_WriteData(*str++);
    }
}

/*******************************************************************************
 * 函数名  : LCD_ClearLine
 * 功能    : 清除指定行 (用空格覆盖)
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
 * 功能    : LCD1602 上电初始化
 * 参数    : 无
 * 返回值  : 无
 * 说明    : 配置 LCD 为 8位/2行/5×7点阵 模式
 *           0x38: 8位数据, 2行显示, 5×7字符
 *           0x0C: 开显示, 关光标, 不闪烁
 *           0x06: 写字符后光标右移, 屏不移动
 *           0x01: 清屏
 *
 *           上电后 LCD 需要 15ms 以上的稳定时间,
 *           开头 50ms 延时确保这一点
 *******************************************************************************/
void LCD_Init(void)
{
    Delay_ms(50);                // 等待 LCD 电源稳定
    LCD_WriteCmd(0x38);          // 功能设置: 8位/2行/5×7
    LCD_WriteCmd(0x0C);          // 显示控制: 开显示/无光标
    LCD_WriteCmd(0x06);          // 输入模式: 地址自增
    LCD_WriteCmd(0x01);          // 清屏
    Delay_ms(5);                 // 清屏指令耗时较长
}

/* ======================== 步进电机驱动 ======================== */

/*******************************************************************************
 * 函数名  : Motor_Forward
 * 功能    : 步进电机前进一步 (顺时针)
 * 参数    : 无
 * 返回值  : 无
 * 说明    : 按 8 拍半步序列 A→AB→B→BC→C→CD→D→DA 递进
 *           P1 写操作保护高4位不变 (独立按键)
 *******************************************************************************/
void Motor_Forward(void)
{
    static u8 step_idx = 0;
    P1 = (P1 & 0xF0) | step_phase[step_idx];
    step_idx = (step_idx + 1) & 0x07;
}

/*******************************************************************************
 * 函数名  : Motor_Stop
 * 功能    : 步进电机停止 (所有相断电)
 * 参数    : 无
 * 返回值  : 无
 * 说明    : P1 低4位清 0 → ULN2003 所有输入低 → 电机无励磁
 *******************************************************************************/
void Motor_Stop(void)
{
    P1 &= 0xF0;   // 仅清低4位, 高4位按键不变
}

/* ======================== 独立按键扫描 ======================== */

/*******************************************************************************
 * 函数名  : Key_Scan
 * 功能    : 扫描 4 个按键, 检测短按并消抖
 * 参数    : 无
 * 返回值  : 0=无按键, 1=KEY1, 2=KEY2, 3=KEY3, 4=KEY4
 * 说明    : 扫描流程:
 *           ① 检测哪个引脚被拉低
 *           ② 延时 20ms 消抖
 *           ③ 再次确认 (防止外界干扰)
 *           ④ 等待按键释放
 *           ⑤ 再次消抖
 *
 *           注意: 一次只返回一个按键,
 *           若有多个键同时按下, 按 KEY1 > KEY2 > KEY3 > KEY4 优先级
 *******************************************************************************/
u8 Key_Scan(void)
{
    if (KEY1 == 0) {
        Delay_ms(20);            // 消抖
        if (KEY1 == 0) {
            while (KEY1 == 0);   // 等待释放
            Delay_ms(20);        // 释放消抖
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
    /*
     * 注意: KEY4 不在此函数中检测!
     * KEY4 需要同时支持短按(切换模式)和长按(设置定时),
     * 所以 KEY4 的处理逻辑放到了主循环中,
     * 使用独立的按键状态机实现
     */
    return 0;
}

/*
 * KEY4 的处理不在独立函数中, 而是在主循环中通过状态机实现
 * 详见主循环中 KEY4 处理部分的详细注释
 */

/* ======================== 蜂鸣器 ======================== */

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

/* ======================== 风扇系统控制 ======================== */

/*******************************************************************************
 * 函数名  : Fan_Start
 * 功能    : 启动风扇
 * 参数    : 无
 * 返回值  : 无
 * 说明    : 设置状态为 ON, 点亮 D1, 初始化各计时器,
 *           计算首次有效转速
 *******************************************************************************/
void Fan_Start(void)
{
    fan_state         = FAN_ON;
    LED_D1            = 0;           // D1 亮 (共阳低电平点亮)
    step_acc          = 0;           // 步进累积器归零
    natural_tick      = 0;
    sleep_tick        = 0;
    natural_rand      = 1;
    sleep_decremented = 0;           // ★重置睡眠风降档标志
    Fan_UpdateEffectiveSpeed();
}

/*******************************************************************************
 * 函数名  : Fan_Stop
 * 功能    : 停止风扇
 * 参数    : 无
 * 返回值  : 无
 *******************************************************************************/
void Fan_Stop(void)
{
    fan_state = FAN_OFF;
    LED_D1    = 1;
    LED_D2    = 1;
    Motor_Stop();
}

/*******************************************************************************
 * 函数名  : Fan_SpeedUp
 * 功能    : 风速 +1 档 (最高 5 档)
 * 参数    : 无
 * 返回值  : 无
 *******************************************************************************/
void Fan_SpeedUp(void)
{
    if (speed_level < 5) {
        speed_level++;
        Fan_UpdateEffectiveSpeed();
    }
}

/*******************************************************************************
 * 函数名  : Fan_SpeedDown
 * 功能    : 风速 -1 档 (最低 1 档)
 * 参数    : 无
 * 返回值  : 无
 *******************************************************************************/
void Fan_SpeedDown(void)
{
    if (speed_level > 1) {
        speed_level--;
        Fan_UpdateEffectiveSpeed();
    }
}

/*******************************************************************************
 * 函数名  : Fan_ModeSwitch
 * 功能    : 切换风类模式 (常风→自然风→睡眠风→常风...)
 * 参数    : 无
 * 返回值  : 无
 *******************************************************************************/
void Fan_ModeSwitch(void)
{
    fan_mode++;
    if (fan_mode > MODE_SLEEP) {
        fan_mode = MODE_NORMAL;
    }
    natural_tick      = 0;
    sleep_tick        = 0;
    sleep_decremented = 0;       // ★重置睡眠风降档标志
    Fan_UpdateEffectiveSpeed();
}

/*******************************************************************************
 * 函数名  : Fan_SetTimer
 * 功能    : 设置或取消定时关机 (循环切换定时时长)
 * 参数    : 无
 * 返回值  : 无
 * 说明    : 每次调用循环切换: 15分 → 30分 → 45分 → 60分 → 关闭
 *           用 timer_tick 当前值判断下一个档位
 *******************************************************************************/
void Fan_SetTimer(void)
{
    if (timer_tick == 0) {
        timer_tick = 15 * TICKS_PER_MIN;  // 无定时 → 15分钟
    } else if (timer_tick == 15 * TICKS_PER_MIN) {
        timer_tick = 30 * TICKS_PER_MIN;  // 15→30分钟
    } else if (timer_tick == 30 * TICKS_PER_MIN) {
        timer_tick = 45 * TICKS_PER_MIN;  // 30→45分钟
    } else if (timer_tick == 45 * TICKS_PER_MIN) {
        timer_tick = 60 * TICKS_PER_MIN;  // 45→60分钟
    } else {
        timer_tick = 0;                   // 其他情况 → 取消定时
    }
}

/*******************************************************************************
 * 函数名  : Fan_UpdateEffectiveSpeed
 * 功能    : 根据用户设定风速和当前风类模式, 计算电机实际转速索引
 * 参数    : 无
 * 返回值  : 无
 *
 * 说明    : 这是风速控制的核心函数
 *
 *           常风模式: 直接使用用户设定
 *             effective_speed = speed_level - 1
 *
 *           自然风模式: 在基础档位附近 ±2 波动
 *             用伪随机算法模拟自然风忽大忽小的效果
 *             风速分布: 40% 基础档, 20% -1, 20% +1, 10% -2, 10% +2
 *
 *           睡眠风模式: 初始同常风, 由定时器 ISR 逐步降低
 *             (只在 sleep_tick==0 时初始赋值)
 *******************************************************************************/
void Fan_UpdateEffectiveSpeed(void)
{
    u8 base = speed_level - 1;   // 转为 0-based 索引

    switch (fan_mode) {

    case MODE_NORMAL:
        effective_speed = base;
        break;

    case MODE_NATURAL:
        /* 用 natural_rand 低 7 位 (0~127) 决定偏移量 */
        if (natural_rand < 51) {
            effective_speed = base;                       // 40%: 不变
        } else if (natural_rand < 77) {
            effective_speed = (base >= 1) ? base - 1 : 0;  // 20%: -1档
        } else if (natural_rand < 103) {
            effective_speed = (base < 4) ? base + 1 : 4;    // 20%: +1档
        } else if (natural_rand < 116) {
            effective_speed = (base >= 2) ? base - 2 : 0;   // 10%: -2档
        } else {
            effective_speed = (base < 3) ? base + 2 : 4;    // 10%: +2档
        }
        /* 边界保护 */
        if (effective_speed > 4) effective_speed = 4;
        break;

    case MODE_SLEEP:
        /* ★使用 sleep_decremented 标志防止竞态:
         *   ISR 刚降完档后 sleep_tick==0, 若此时主循环调用
         *   Fan_UpdateEffectiveSpeed, 不应重置转速 */
        if (!sleep_decremented && sleep_tick == 0) {
            effective_speed = base;
        }
        break;
    }
}

/* ======================== LCD 显示更新 ======================== */

/*******************************************************************************
 * 函数名  : LCD_UpdateAll
 * 功能    : 刷新 LCD1602 全部显示内容
 * 参数    : 无
 * 返回值  : 无
 *
 * 说明    : 显示格式:
 *
 *           风扇关闭时:
 *           ┌────────────────┐
 *           │  FAN STOPPED  │  ← 第 1 行
 *           │ Timer:OFF      │  ← 第 2 行
 *           └────────────────┘
 *
 *           风扇运行时:
 *           ┌────────────────┐
 *           │S:3 NORMAL  ON  │  ← S=Speed, NORMAL=常风, ON=运行
 *           │Timer:30m       │  ← 定时剩余 30 分钟
 *           └────────────────┘
 *
 *           模式缩写: NORMAL=常风, NATURAL=自然风, SLEEP=睡眠风
 *******************************************************************************/
void LCD_UpdateAll(void)
{
    u8  min;
    u8  *mode_str;

    /* --- 第 1 行: 风速 + 模式 + 状态 --- */
    LCD_SetPos(0, 0);

    if (fan_state == FAN_OFF) {
        LCD_WriteStr("  FAN STOPPED  ");
    } else {
        LCD_WriteStr("S:");           // Speed
        LCD_WriteData('0' + speed_level);
        LCD_WriteData(' ');

        switch (fan_mode) {
            case MODE_NORMAL:  mode_str = "NORMAL ";  break;
            case MODE_NATURAL: mode_str = "NATURAL";  break;
            case MODE_SLEEP:   mode_str = "SLEEP  ";  break;
            default:           mode_str = "NORMAL ";  break;
        }
        LCD_WriteStr(mode_str);
        LCD_WriteStr(" ON ");
    }

    /* --- 第 2 行: 定时信息 --- */
    LCD_SetPos(1, 0);
    LCD_WriteStr("Timer:");

    if (timer_tick == 0) {
        LCD_WriteStr("OFF     ");
    } else {
        min = timer_tick / TICKS_PER_MIN;
        if (min >= 10) LCD_WriteData('0' + (min / 10) % 10);
        else           LCD_WriteData('0');
        LCD_WriteData('0' + (min % 10));
        LCD_WriteStr("m     ");
    }
}

/* ======================== 定时器 1 中断 — 系统节拍 ======================== */

/*******************************************************************************
 * 函数名  : Timer1_ISR
 * 功能    : 定时器 1 中断 (每 50ms 触发一次)
 * 参数    : 无 (中断)
 * 返回值  : 无
 *
 * 说明    : 这是整个系统的时间"心跳", 负责:
 *           [1] sys_tick 累加 (系统总时间)
 *           [2] 定时关机倒计时 (timer_tick 递减, 到零自动关机)
 *           [3] 自然风模式每 3 秒重新计算风速
 *           [4] 睡眠风模式每 10 分钟降一档
 *           [5] LCD 刷新周期控制
 *
 *           ★中断中只做计数和标志更新,
 *           不操作 LCD、不调用耗时函数
 *******************************************************************************/
void Timer1_ISR(void) interrupt 3
{
    /* 重装定时初值 (模式 1 需手动重装) */
    TH1 = T1_TH;
    TL1 = T1_TL;

    /* [1] 系统总节拍 */
    sys_tick++;

    /* 步进电机速率累积 */
    if (fan_state == FAN_ON) {
        step_acc += step_rate[effective_speed];
    }

    /* [2] 定时关机倒计时 */
    if (timer_tick > 0) {
        timer_tick--;
        if (timer_tick == 0) {
            Fan_Stop();          // 定时到, 自动关风扇
        }
    }

    /* [3] 自然风: 每 3 秒换一次风速 */
    if (fan_state == FAN_ON && fan_mode == MODE_NATURAL) {
        natural_tick++;
        if (natural_tick >= 3 * TICKS_PER_SEC) {  // 3 秒
            natural_tick = 0;
            /* 用系统节拍低位生成 0~127 伪随机值 */
            natural_rand = (u8)(sys_tick & 0x7F);
            Fan_UpdateEffectiveSpeed();
        }
    }

    /* [4] 睡眠风: 每 10 分钟降一档 */
    if (fan_state == FAN_ON && fan_mode == MODE_SLEEP) {
        sleep_tick++;
        /* 10 分钟 = 10 × 60 × 20 = 12000 节拍 */
        if (sleep_tick >= 10 * TICKS_PER_MIN) {
            sleep_tick = 0;
            /* effective_speed 越小 → PWM 占空比越低 → 转速越慢 */
            if (effective_speed > 0) {
                effective_speed--;       // 降一档 (减小占空比)
                sleep_decremented = 1;   // ★标记: ISR已修改转速
            }
        }
    }

    /* [5] LCD 刷新周期: 每 0.5 秒允许一次刷新 */
    if (lcd_refresh_tick > 0) {
        lcd_refresh_tick--;
    }
}

/* ======================== 主函数 ======================== */

/*******************************************************************************
 * 函数名  : main
 * 功能    : 程序入口 — 初始化 + 主循环
 * 参数    : 无
 * 返回值  : 无
 *
 * 说明    : 初始化顺序很重要, 不要随意调整:
 *          ① 端口初始化 → ② 变量初始化 → ③ 定时器初始化
 *          → ④ LCD 初始化 → ⑤ 开总中断 → ⑥ 显示开机画面
 *          → ⑦ 进入主循环
 *
 *           主循环每轮做以下工作:
 *          (a) 检测按键 → 执行对应操作
 *          (b) 检测 KEY4 长按 → 设置/取消定时
 *          (c) 若风扇运行 → 步进电机驱动
 *          (d) 若 LCD 刷新时间到 → 更新显示
 *******************************************************************************/
void main(void)
{
    u8  key_val;

    /* ===== ① 端口初始化 ===== */
    P0 = 0xFF;                   // P0 释放 (LCD 数据总线)
    P1 = 0xF0;                   // P1 高4位=1 (按键输入), 低4位=0 (步进电机相)
    P2 = 0xFF;                   // P2 LCD/蜂鸣器就绪
    P3 = 0xFF;                   // P3 全高 (D1灭, 红外未用)

    /* ===== ② 变量初始化 ===== */
    fan_state        = FAN_OFF;
    fan_mode         = MODE_NORMAL;
    speed_level      = 1;        // 默认 1 档 (微风)
    effective_speed  = 0;
    step_acc         = 0;
    sys_tick         = 0;
    natural_tick     = 0;
    sleep_tick       = 0;
    timer_tick       = 0;
    natural_rand     = 1;
    lcd_refresh_tick = 0;

    LED_D1 = 1;                  // D1(P3.0) 初始灭
    LED_D2 = 1;

    /* ===== ③ 定时器 1 初始化 ===== */
    /*
     * TMOD 寄存器:
     *   bit7~4: T1 模式 (GATE=0, C/T=0, M1M0=01=模式1)
     *   bit3~0: T0 模式 (保持默认)
     *
     * T1 模式 1: 16 位定时/计数器
     * 定时时间 = (65536 - 初值) × 12 / 晶振频率
     * 50ms = (65536 - 19456) × 12 / 11059200
     * 初值 = 19456 = 0x4C00
     */
    TMOD &= 0x0F;                // 清零 T1 配置
    TMOD |= 0x10;                // T1 = 模式 1 (16位)
    TH1 = T1_TH;
    TL1 = T1_TL;
    ET1 = 1;                     // 允许 T1 中断
    TR1 = 1;                     // 启动 T1

    /* ===== ④ LCD 初始化 ===== */
    LCD_Init();

    /* ===== ⑤ 开总中断 ===== */
    EA = 1;                      // ★最后才开中断, 防止中断干扰初始化

    /* ===== ⑥ 开机画面 ===== */
    LCD_SetPos(0, 0);
    LCD_WriteStr("Smart Fan v1.0 ");
    LCD_SetPos(1, 0);
    LCD_WriteStr("Proteus  Sim.  ");
    Delay_ms(1500);

    LCD_ClearLine(0);
    LCD_ClearLine(1);
    LCD_UpdateAll();
    lcd_refresh_tick = 10;       // 0.5 秒后首次刷新

    /* ═══════════════════════════════════════════════════
     *  ⑦ 主循环
     * ═══════════════════════════════════════════════════ */
    while (1) {

        /* ----- (a) 扫描按键短按 ----- */
        key_val = Key_Scan();

        switch (key_val) {

        case 1:  /* KEY1: 启动 / 停止 */
            if (fan_state == FAN_OFF)
                Fan_Start();
            else
                Fan_Stop();
            Beep_Tick();
            lcd_refresh_tick = 1;    // 立即刷新
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

        /* ----- (b) KEY4 处理: 短按切换模式 / 长按设置定时 ----- */
        /*
         * KEY4 使用独立的状态机, 不经过 Key_Scan(),
         * 因为需要区分短按和长按:
         *
         * - 短按 (<2秒): 切换风类模式 (常风→自然风→睡眠风)
         * - 长按 (≥2秒): 设置/取消定时关机
         *
         * 状态机使用 sys_tick 计时, 不受主循环速度影响:
         *   ① KEY4 按下 → 记录按下时刻 key4_down_start
         *   ② KEY4 持续按下 → 检查 sys_tick - key4_down_start ≥ 40 (2秒)
         *      → 是: 触发长按动作, 等待释放后返回
         *   ③ KEY4 释放且未达 2 秒 → 触发短按动作
         *
         * key4_was_down: 记录上一轮 KEY4 的状态, 用于检测边沿
         * key4_down_start: KEY4 按下时的 sys_tick 快照
         * key4_long_done: 防止长按触发后释放时又触发短按
         */
        {
            static u16  key4_down_start = 0;  // 按下时刻的 sys_tick
            static bit  key4_was_down  = 0;   // 上一轮是否按下
            static bit  key4_long_done = 0;   // 本次长按是否已处理

            if (KEY4 == 0) {                   // KEY4 当前被按下
                if (!key4_was_down) {          // 刚按下 (下降沿)
                    key4_down_start = sys_tick;
                    key4_long_done  = 0;
                    key4_was_down   = 1;
                }
                /* 检查是否达到长按阈值 (2秒 = 40节拍) */
                if (!key4_long_done &&
                    (u16)(sys_tick - key4_down_start) >= 40) {
                    key4_long_done = 1;
                    Fan_SetTimer();            // ★长按: 设置/取消定时
                    Beep_Tick();
                    Beep_Tick();               // 两声提示表示长按
                    lcd_refresh_tick = 1;
                    while (KEY4 == 0);         // 等待释放
                    key4_was_down = 0;
                }
            } else {                           // KEY4 当前已释放
                if (key4_was_down && !key4_long_done) {
                    /* 释放时未达到长按阈值 → 短按 */
                    Fan_ModeSwitch();          // ★短按: 切换模式
                    Beep_Tick();               // 一声提示表示短按
                    lcd_refresh_tick = 1;
                }
                key4_was_down = 0;
            }
        }

        /* ----- (c) 风扇运行时驱动步进电机 (非阻塞, 速率累积) ----- */
        if (fan_state == FAN_ON) {
            /* 累积到 50 (即 50ms) 步进一次 */
            while (step_acc >= 50) {
                step_acc -= 50;
                Motor_Forward();
            }

            /* 睡眠风模式: D2 每 0.5s 闪烁一次 (用 sys_tick 节拍) */
            if (fan_mode == MODE_SLEEP) {
                LED_D2 = (sys_tick / 10) % 2 ? 0 : 1;
            }

            /* 自然风模式: D2 随风速变化闪烁, 每秒亮1秒/周期3秒 */
            if (fan_mode == MODE_NATURAL) {
                LED_D2 = (natural_tick < TICKS_PER_SEC) ? 0 : 1;
            }

        } else {
            Motor_Stop();
        }

        /* ----- (d) LCD 定时刷新 (每 0.5 秒) ----- */
        if (lcd_refresh_tick == 0) {
            LCD_UpdateAll();
            lcd_refresh_tick = 10;    // 10 × 50ms = 0.5 秒
        }

        /* 常风模式下 D2 常亮 */
        if (fan_state == FAN_ON && fan_mode == MODE_NORMAL) {
            LED_D2 = 0;
        }

    } /* end while(1) */
}
