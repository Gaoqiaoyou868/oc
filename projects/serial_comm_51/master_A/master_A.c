#include <reg51.h>
#include <intrins.h>

typedef unsigned int u16;
typedef unsigned char u8;

/* ======================== 引脚定义 ======================== */
sbit KEY1 = P3^2;    // K1 (INT0) 短按=发0x01, 长按=发字符串
sbit KEY2 = P3^3;    // K2 (INT1) 发0x02
sbit SEG_TENS = P1^0;  // 7SEG 十位位选 (共阳: 1=亮)
sbit SEG_UNITS = P1^1; // 7SEG 个位位选 (共阳: 1=亮)

/* ======================== 常量 ======================== */
u8 code seg_CA[10] = {
    0xC0, // 0
    0xF9, // 1
    0xA4, // 2
    0xB0, // 3
    0x99, // 4
    0x92, // 5
    0x82, // 6
    0xF8, // 7
    0x80, // 8
    0x90  // 9
};

/* "Hello 齐继浩 094424128\r\n"  -- GB2312编码 */
u8 code hello_str[] = {
    'H','e','l','l','o',' ',
    0xC6,0xEB,  /* 齐 */
    0xBC,0xCC,  /* 继 */
    0xBA,0xC0,  /* 浩 */
    ' ','0','9','4','4','2','4','1','2','8',
    '\r','\n','\0'
};

/* ======================== 全局变量 ======================== */
u8 disp_val = 0;    // 数码管显示值 (0~99)
u8 rx_byte = 0;     // 串口接收字节
bit rx_flag = 0;    // 收到了新数据

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

void UART_SendString(u8 *str)
{
    while (*str) {
        UART_SendByte(*str++);
    }
}

/* ======================== INT0 — K1 中断 ======================== */
void INT0_ISR(void) interrupt 0
{
    u16 cnt = 0;

    Delay_ms(20);
    if (KEY1 == 0) {
        while (KEY1 == 0 && cnt < 150) {
            Delay_ms(10);
            cnt++;
        }
        if (cnt >= 100) {
            /* 长按 (>=1s) → 发送字符串 (选做1) */
            UART_SendString(hello_str);
        } else {
            /* 短按 → 发 0x01 (必做) */
            UART_SendByte(0x01);
        }
    }
}

/* ======================== INT1 — K2 中断 ======================== */
void INT1_ISR(void) interrupt 2
{
    Delay_ms(20);
    if (KEY2 == 0) {
        UART_SendByte(0x02);
        while (KEY2 == 0);
        Delay_ms(20);
    }
}

/* ======================== Timer0 — 数码管扫描 ======================== */
void Timer0_ISR(void) interrupt 1
{
    static u8 scan = 0;

    TH0 = 0xF8;  TL0 = 0xCD;   /* 2ms @ 11.0592MHz */

    P0 = 0xFF;                  /* 消隐 */
    SEG_TENS = 0;
    SEG_UNITS = 0;

    scan = !scan;
    if (scan) {
        P0 = seg_CA[disp_val / 10];
        SEG_TENS = 1;           /* 十位 */
    } else {
        P0 = seg_CA[disp_val % 10];
        SEG_UNITS = 1;          /* 个位 */
    }
}

/* ======================== UART 中断 ======================== */
void UART_ISR(void) interrupt 4
{
    if (RI) {
        rx_byte = SBUF;
        RI = 0;

        if (rx_byte <= 99) {
            disp_val = rx_byte;
        }
    }
}

/* ======================== 主函数 ======================== */
void main(void)
{
    /* 端口初始化 */
    P0 = 0xFF;
    P1 = 0x00;
    P2 = 0xFF;
    P3 = 0xFF;

    disp_val = 0;

    /* Timer0: 模式1, 16位, 2ms */
    TMOD &= 0xF0;
    TMOD |= 0x01;
    TH0 = 0xF8;  TL0 = 0xCD;
    ET0 = 1;  TR0 = 1;

    /* Timer1: 模式2, 8位自动重装, 9600波特率 @ 11.0592MHz */
    TMOD &= 0x0F;
    TMOD |= 0x20;
    TH1 = 0xFD;  TL1 = 0xFD;
    TR1 = 1;

    /* UART: 方式1, 允许接收 */
    SCON = 0x50;  PCON = 0x00;
    ES = 1;

    /* 外部中断: 下降沿触发 */
    IT0 = 1;  EX0 = 1;   /* K1 */
    IT1 = 1;  EX1 = 1;   /* K2 */

    /* 中断优先级: 按键 > 定时器 > 串口 */
    PX0 = 1;  PX1 = 1;
    PT0 = 0;  PS = 0;

    EA = 1;

    while (1);
}
