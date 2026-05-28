/******************************************************************************
 * AD/DA转换实验 — DA部分：TLC5615 波形发生器
 *
 * 芯片    : AT89C51 (Proteus仿真)  @ 12 MHz
 * 编译环境: Keil uVision 5
 * 仿真软件: Proteus 8.x
 *
 * ╔══════════════════════════════════════════════════════════════╗
 * ║  引脚映射                                                   ║
 * ║  TLC5615                                                    ║
 * ║    DIN (串行数据输入)  → P1.0                               ║
 * ║    SCLK (串行时钟输入) → P1.1                               ║
 * ║    CS (片选, 低有效)    → P1.2                              ║
 * ║    OUT (模拟电压输出)   → 示波器通道                         ║
 * ║    REFIN (参考电压输入) → +5V                               ║
 * ╚══════════════════════════════════════════════════════════════╝
 *
 * TLC5615 通信协议:
 *   TLC5615 是 10 位串行 DAC, 数据格式为 12 位:
 *   高 2 位填充 0 (无关位), 低 10 位为 DAC 数据 (D9~D0, MSB 先发)
 *   时序: CS=0 → SCLK 上升沿锁存 DIN → 12 位发完后 CS=1 更新输出
 *   输出: Vout = 2 x REFIN x (DAC_VALUE / 1024)
 *         当 REFIN=+5V 时, Vout 范围 0~10V
 *
 * 功能:
 *   自动循环输出四种波形 (每种约 3 秒后切换):
 *   ① 锯齿波 — 0→1023 循环递增
 *   ② 三角波 — 0→1023→0 循环
 *   ③ 方波   — 0→1023 交替
 *   ④ 正弦波 — 128 点查表输出
 *
 * 接线说明 (Proteus):
 *   AT89C51:
 *     P1.0 → TLC5615.DIN
 *     P1.1 → TLC5615.SCLK
 *     P1.2 → TLC5615.CS
 *   TLC5615:
 *     OUT → 示波器 A 通道
 *     REFIN → +5V
 *     AGND → GND (仿真中可以不接)
 *     DOUT → 悬空
 *   晶振: 12MHz
 ******************************************************************************/

#include <reg51.h>
#include <intrins.h>

typedef unsigned int  u16;
typedef unsigned char u8;

/* ======================== TLC5615 引脚 ======================== */
sbit TLC_DIN  = P1^0;    // 串行数据输入
sbit TLC_SCLK = P1^1;    // 串行时钟
sbit TLC_CS   = P1^2;    // 片选 (低有效, 上升沿更新输出)

/* ======================== 波形枚举 ======================== */
#define WAVE_SAWTOOTH  0   // 锯齿波
#define WAVE_TRIANGLE  1   // 三角波
#define WAVE_SQUARE    2   // 方波
#define WAVE_SINE      3   // 正弦波
#define WAVE_COUNT     4   // 波形总数

/* ======================== 正弦波查找表 ======================== */
/*
 * 128 点, 10 位精度 (0x000~0x3FF), 覆盖完整周期
 * 预计算: value[i] = (u16)(512 + 511 * sin(2*PI*i/128))
 */
code u16 sin_table[128] = {
    0x200, 0x219, 0x232, 0x24A, 0x263, 0x27C, 0x294, 0x2AC,
    0x2C3, 0x2DA, 0x2F0, 0x306, 0x31B, 0x330, 0x344, 0x357,
    0x369, 0x37A, 0x38B, 0x39A, 0x3A8, 0x3B6, 0x3C2, 0x3CD,
    0x3D8, 0x3E1, 0x3E8, 0x3EF, 0x3F5, 0x3F9, 0x3FC, 0x3FE,
    0x3FF, 0x3FE, 0x3FC, 0x3F9, 0x3F5, 0x3EF, 0x3E8, 0x3E1,
    0x3D8, 0x3CD, 0x3C2, 0x3B6, 0x3A8, 0x39A, 0x38B, 0x37A,
    0x369, 0x357, 0x344, 0x330, 0x31B, 0x306, 0x2F0, 0x2DA,
    0x2C3, 0x2AC, 0x294, 0x27C, 0x263, 0x24A, 0x232, 0x219,
    0x200, 0x1E6, 0x1CD, 0x1B5, 0x19C, 0x183, 0x16B, 0x153,
    0x13C, 0x125, 0x10F, 0x0F9, 0x0E4, 0x0CF, 0x0BB, 0x0A8,
    0x096, 0x085, 0x074, 0x065, 0x057, 0x049, 0x03D, 0x032,
    0x027, 0x01E, 0x017, 0x010, 0x00A, 0x006, 0x003, 0x001,
    0x001, 0x001, 0x003, 0x006, 0x00A, 0x010, 0x017, 0x01E,
    0x027, 0x032, 0x03D, 0x049, 0x057, 0x065, 0x074, 0x085,
    0x096, 0x0A8, 0x0BB, 0x0CF, 0x0E4, 0x0F9, 0x10F, 0x125,
    0x13C, 0x153, 0x16B, 0x183, 0x19C, 0x1B5, 0x1CD, 0x1E6,
};

/* ======================== 延时函数 ======================== */

/*******************************************************************************
 * 函数名  : Delay_10us
 * 功能    : 约 10us 延时 (@12MHz, 仿真用)
 * 参数    : ten_us = 倍数 (1 ≈ 10us)
 * 返回值  : 无
 ******************************************************************************/
void Delay_10us(u16 ten_us)
{
    while (ten_us--);
}

/*******************************************************************************
 * 函数名  : Delay_ms
 * 功能    : 毫秒级延时 (@12MHz, 仿真用)
 * 参数    : ms = 毫秒数
 * 返回值  : 无
 ******************************************************************************/
void Delay_ms(u16 ms)
{
    u16 i, j;
    for (i = ms; i > 0; i--)
        for (j = 130; j > 0; j--);
}

/* ======================== TLC5615 驱动 ======================== */

/*******************************************************************************
 * 函数名  : TLC5615_Write
 * 功能    : 向 TLC5615 写入 10 位 DAC 数据
 * 参数    : dat = 10 位 DAC 值 (0~1023)
 * 返回值  : 无
 * 说明    : 发送 12 位串行数据帧: 2 位无关位(0) + 10 位数据(MSB 先发)
 *           时钟约 500kHz (1MHz 高电平时间 + 1MHz 低电平时间)
 ******************************************************************************/
void TLC5615_Write(u16 dat)
{
    u8 i;

    dat &= 0x03FF;        // 屏蔽高 6 位, 只保留低 10 位 DAC 数据

    TLC_CS = 0;           // CS=0, 使能串行输入
    TLC_SCLK = 0;         // SCLK 初始为低电平

    /* 发送 2 位无关位 (填充 0) */
    TLC_DIN = 0;
    TLC_SCLK = 1;         // SCLK 上升沿, TLC5615 锁存 DIN
    _nop_();              // 保持高电平
    TLC_SCLK = 0;         // SCLK 拉低, 准备下一位

    TLC_DIN = 0;
    TLC_SCLK = 1;
    _nop_();
    TLC_SCLK = 0;

    /* 发送 10 位 DAC 数据 (D9~D0, MSB First) */
    for (i = 0; i < 10; i++) {
        if (dat & 0x0200)        // 判断最高位 (bit 9)
            TLC_DIN = 1;
        else
            TLC_DIN = 0;

        TLC_SCLK = 1;            // 上升沿锁存数据位
        _nop_();                 // 保持高电平 (>50ns 即可, 此处约 1us)
        TLC_SCLK = 0;            // 拉低准备下一位

        dat <<= 1;               // 数据左移, 下一位进入最高位位置
    }

    TLC_CS = 1;           // CS=1 (上升沿), 将数据从输入寄存器更新到 DAC 寄存器
}

/* ======================== 主函数 ======================== */

/*******************************************************************************
 * 函数名  : main
 * 功能    : 程序入口 — 自动循环输出四种波形
 * 参数    : 无
 * 返回值  : 无
 *
 * 说明    : 主循环不断生成当前波形的下一个采样点值,
 *           写入 TLC5615 后延时一小段时间控制波形频率.
 *           每完成约 150 个完整波形周期后自动切换到下一种波形.
 *
 *           波形频率 (12MHz):
 *             锯齿波/三角波/方波: ~7Hz
 *             正弦波: ~55Hz
 ******************************************************************************/
void main(void)
{
    u8  wave_type = WAVE_SAWTOOTH;
    u16 step      = 0;
    u16 val       = 0;
    u16 cycle_cnt = 0;

    P1 = 0xFF;
    P3 = 0xFF;
    TLC_CS = 1;
    Delay_ms(10);

    while (1) {
        switch (wave_type) {

        case WAVE_SAWTOOTH:
            val = step;
            step++;
            if (step >= 1024) { step = 0; cycle_cnt++; }
            break;

        case WAVE_TRIANGLE:
            if (step < 512)
                val = step << 1;
            else
                val = (1023 - step) << 1;
            step++;
            if (step >= 1024) { step = 0; cycle_cnt++; }
            break;

        case WAVE_SQUARE:
            val = (step < 512) ? 0 : 1023;
            step++;
            if (step >= 1024) { step = 0; cycle_cnt++; }
            break;

        case WAVE_SINE:
            val = sin_table[step];
            step++;
            if (step >= 128) { step = 0; cycle_cnt++; }
            break;
        }

        TLC5615_Write(val);

        Delay_10us(13);

        if (cycle_cnt >= 150) {
            cycle_cnt = 0;
            wave_type++;
            if (wave_type >= WAVE_COUNT)
                wave_type = WAVE_SAWTOOTH;
            step = 0;
        }
    }
}

    /* ===== 主循环 ===== */
    while (1) {
        /* 根据当前波形类型计算下一个输出值 */
        switch (wave_type) {

        case WAVE_SAWTOOTH:        /* ── 锯齿波: 0→1023 线性递增 ── */
            val = step;            // step 0~1023 直接作为 DAC 值
            step++;
            if (step >= 1024) {    // 超过最大值, 完成一个周期
                step = 0;
                cycle_cnt++;
            }
            break;

        case WAVE_TRIANGLE:        /* ── 三角波: 0→1023→0 对称 ── */
            if (step < 512)
                val = step << 1;             // 上升段: step*2, 0→1022
            else
                val = (1023 - step) << 1;    // 下降段: (1023-step)*2, 1022→0
            step++;
            if (step >= 1024) {    // 1024 步完成一个完整周期
                step = 0;
                cycle_cnt++;
            }
            break;

        case WAVE_SQUARE:          /* ── 方波: 高电平 1023 / 低电平 0 ── */
            val = (step < 512) ? 0 : 1023;   // 前半周期低电平, 后半高电平
            step++;
            if (step >= 1024) {
                step = 0;
                cycle_cnt++;
            }
            break;

        case WAVE_SINE:            /* ── 正弦波: 128 点查表 ── */
            val = sin_table[step];           // 从查找表中读取预计算值
            step++;
            if (step >= 128) {     // 128 步完成一个完整周期
                step = 0;
                cycle_cnt++;
            }
            break;
        }

        /* 将当前值写入 TLC5615, 输出模拟电压 */
        TLC5615_Write(val);

        /* 步进间隔延时: 控制波形频率 (~130us per step) */
        Delay_10us(13);            // 约 130us 延时 (@12MHz)

        /*
         * 波形自动切换: 每完成约 150 个周期切换一次.
         * 60Hz × 150 周期 ≈ 2.5 秒, 示波器有足够时间观察波形.
         */
        if (cycle_cnt >= 150) {
            cycle_cnt = 0;                     // 周期计数器归零
            wave_type++;                       // 切换到下一种波形
            if (wave_type >= WAVE_COUNT)
                wave_type = WAVE_SAWTOOTH;     // 如果已经是最后一种, 回到第一种
            step = 0;                          // 新波形步进位置重置
        }
    }
}
