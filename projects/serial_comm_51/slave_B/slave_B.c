#include <reg51.h>
#include <intrins.h>

typedef unsigned int u16;
typedef unsigned char u8;

/* ======================== 引脚定义 ======================== */
sbit KEY3 = P3^2;    // K3 (INT0) 按键计数并发送
sbit LED1 = P2^6;    // LED1 (低电平亮)
sbit LED2 = P2^7;    // LED2 (低电平亮)

/* ======================== 全局变量 ======================== */
u8 key_count = 0;     // K3 按键次数 (0~99)
u8 rx_buf[4];         // 字符串命令缓存
u8 rx_idx = 0;        // 缓存索引

/* ======================== 延时 ======================== */
void Delay_ms(u16 ms)
{
    u16 i, j;
    for (i = ms; i > 0; i--)
        for (j = 120; j > 0; j--);
}

/* ======================== 串口发送 ======================== */
void UART_SendByte(u8 dat)
{
    SBUF = dat;
    while (!TI);
    TI = 0;
}

/* ======================== INT0 — K3 中断 ======================== */
void INT0_ISR(void) interrupt 0
{
    Delay_ms(20);
    if (KEY3 == 0) {
        key_count++;
        if (key_count > 99) key_count = 0;
        UART_SendByte(key_count);
        while (KEY3 == 0);
        Delay_ms(20);
    }
}

/* ======================== UART 中断 ======================== */
void UART_ISR(void) interrupt 4
{
    u8 ch;

    if (RI) {
        ch = SBUF;
        RI = 0;

        /* ---- 字节命令 (必做) ---- */
        if (ch == 0x01) {
            LED1 = 0;  LED2 = 1;   /* LED1亮, LED2灭 */
            rx_idx = 0;
            return;
        }
        if (ch == 0x02) {
            LED1 = 1;  LED2 = 1;   /* 全灭 */
            rx_idx = 0;
            return;
        }

        /* ---- 字符串命令 (选做2) ---- */
        if (ch >= '0' && ch <= '9') {
            if (rx_idx < 3) {
                rx_buf[rx_idx++] = ch;
            }
        } else {
            rx_idx = 0;   /* 非数字字符 → 复位 */
            return;
        }

        if (rx_idx == 2) {
            if (rx_buf[0] == '1' && rx_buf[1] == '1') {
                LED1 = 0;            /* "11" → LED1开 */
            } else if (rx_buf[0] == '1' && rx_buf[1] == '0') {
                LED1 = 1;            /* "10" → LED1关 */
            } else if (rx_buf[0] == '2' && rx_buf[1] == '1') {
                LED2 = 0;            /* "21" → LED2开 */
            } else if (rx_buf[0] == '2' && rx_buf[1] == '0') {
                LED2 = 1;            /* "20" → LED2关 */
            }
            rx_idx = 0;
        }

        if (rx_idx >= 3) rx_idx = 0;
    }
}

/* ======================== 主函数 ======================== */
void main(void)
{
    /* 端口初始化 */
    P2 = 0xFF;     /* LED 初始灭 */
    key_count = 0;
    rx_idx = 0;

    /* Timer1: 模式2, 8位自动重装, 9600波特率 @ 11.0592MHz */
    TMOD &= 0x0F;
    TMOD |= 0x20;
    TH1 = 0xFD;  TL1 = 0xFD;
    TR1 = 1;

    /* UART: 方式1, 允许接收 */
    SCON = 0x50;  PCON = 0x00;
    ES = 1;

    /* 外部中断: 下降沿触发 */
    IT0 = 1;  EX0 = 1;   /* K3 */

    /* 中断优先级: 按键 > 串口 */
    PX0 = 1;
    PS = 0;

    EA = 1;

    while (1);
}
