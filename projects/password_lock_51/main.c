/******************************************************************************
 * 智能密码锁系统  (Keil C51)  — v4 最终版
 *
 * 芯片    : STC89C52RC  @ 11.0592 MHz
 *
 *  ╔══════════════════════════════════════════════════════════╗
 *  ║  引脚映射                                               ║
 *  ║  LCD1602 : RS=P2.6, RW=P2.5, EN=P2.7, D0~D7=P0       ║
 *  ║  矩阵键盘: P1.0~P1.3(列), P1.4~P1.7(行)  [JP4]        ║
 *  ║  步进电机: P1.0~P1.3, ULN2003  [JP8]                  ║
 *  ║  蜂鸣器  : P2.5 (与LCD_RW共用, 仅在非LCD操作时发声)   ║
 *  ║  红灯    : P2.1 (D2, 锁定, 共阳:0=亮)                 ║
 *  ║  绿灯    : P3.4 (需外接LED, 高电平亮; 或只看LCD)      ║
 *  ╚══════════════════════════════════════════════════════════╝
 *
 *  ╔══════════════════════════════════════════════════════════╗
 *  ║  启动前跳线帽配置 (务必遵守!)                           ║
 *  ║  段选: ✅ ON    位选: ✅ ON                            ║
 *  ║  JP4(键盘): ✅ ON      JP8(电机): ❌ OFF               ║
 *  ║  ★ JP8 必须先拔掉, 否则按键时电机会乱转!              ║
 *  ╚══════════════════════════════════════════════════════════╝
 *
 *  定时器:
 *   T0 : 蜂鸣器 PWM (模式1)
 *   T1 : 20ms 系统节拍
 *============================================================================*/

#include <reg51.h>
#include <intrins.h>

/* ======================== 硬件引脚 ======================== */

#define LCD_DATA     P0
sbit LCD_RS = P2^6;
sbit LCD_RW = P2^5;
sbit LCD_EN = P2^7;

sbit BEEP      = P2^5;    /* 蜂鸣器=LCD_RW, 共用P2.5 */
sbit LED_RED   = P2^1;    /* D2: 红灯(锁定) */
sbit LED_GREEN = P3^4;    /* 绿灯(解锁), P3.4需外接LED */

sbit BTN_K1 = P3^1;
sbit BTN_K2 = P3^0;

/* ======================== 常量 ======================== */

#define PASS_LEN      4
#define MAX_ATTEMPTS  3
#define LOCKOUT_TICK  1500       /* 30秒 = 1500×20ms */
#define UNLOCK_TICK   250        /* 5秒  = 250×20ms  */
#define PHASE_COUNT   8

#define S_LOCKED      0
#define S_INPUT       1
#define S_VERIFY_OK   2
#define S_UNLOCKED    3
#define S_VERIFY_FAIL 4
#define S_LOCKOUT     5

#define BEEP_KEY      0          /* 1.0kHz */
#define BEEP_OK       1          /* 1.5kHz */
#define BEEP_ERR      2          /* 500Hz  */

#define T1_TH         0xB7
#define T1_TL         0xFF

/* ======================== Flash 数据表 ======================== */

unsigned char code phase_table[PHASE_COUNT] = {
    0x01, 0x03, 0x02, 0x06, 0x04, 0x0C, 0x08, 0x09
};

unsigned char code password[PASS_LEN] = {1, 2, 3, 4};

/*
 * 蜂鸣器频率 → T0 重装值
 * [0] 1.0kHz / [1] 1.5kHz / [2] 500Hz
 */
unsigned char code beep_TH[3] = {0xFE, 0xFE, 0xFC};
unsigned char code beep_TL[3] = {0x33, 0xCD, 0x66};

/* ======================== 全局变量 ======================== */

unsigned char input_buf[PASS_LEN];
unsigned char input_count;
unsigned char state;
unsigned char attempts;
unsigned int  state_timer;
char phase_idx;
unsigned char beep_type;
unsigned int  beep_duration;

/* ======================== 函数声明 ======================== */

void  Delay_ms(unsigned int ms);
void  LCD_WriteCmd(unsigned char cmd);
void  LCD_WriteData(unsigned char dat);
void  LCD_Init(void);
void  LCD_SetPos(unsigned char line, unsigned char col);
void  LCD_WriteStr(unsigned char *str);
void  LCD_ClearLine(unsigned char line);
void  LCD_ShowPrompt(unsigned char *line1, unsigned char *line2);
unsigned char Key_Scan(void);
unsigned char Key_WaitPress(void);
void  Beep_Start(unsigned char type, unsigned int tick);
void  Beep_Stop(void);
void  Motor_StepCW(void);
void  Motor_StepCCW(void);
void  Motor_Run(unsigned int steps, unsigned char direction);
void  Motor_Reset(void);
void  UI_ShowLocked(void);
void  UI_ShowInput(void);
void  UI_ShowCorrect(void);
void  UI_ShowUnlocked(void);
void  UI_ShowError(void);
void  UI_ShowLockout(void);
void  UI_ShowSwitchToMotor(void);
void  UI_ShowSwitchToKeypad(void);

/* ======================== 延时 ======================== */

void Delay_ms(unsigned int ms)
{
    unsigned int i;
    while (ms--) {
        for (i = 0; i < 120; i++) { _nop_(); }
    }
}

/* ====================== LCD1602 驱动 ====================== */

/*
 * LCD 写操作期间必须关蜂鸣器 (T0), 因为蜂鸣器和 LCD_RW 共用 P2.5
 * 如果在 LCD 写数据时 T0 ISR 翻转 P2.5, RW 会变高 → LCD 进入读模式 → 显示异常
 */

void LCD_WriteCmd(unsigned char cmd)
{
    EA = 0;                        /* 关中断, 保护 LCD 时序 */
    LCD_RS = 0;
    LCD_RW = 0;
    LCD_EN = 0;
    LCD_DATA = cmd;
    Delay_ms(1);
    LCD_EN = 1;
    Delay_ms(1);
    LCD_EN = 0;
    LCD_DATA = 0xFF;               /* 释放 P0, 避免数码管乱码 */
    EA = 1;
}

void LCD_WriteData(unsigned char dat)
{
    EA = 0;
    LCD_RS = 1;
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

void LCD_SetPos(unsigned char line, unsigned char col)
{
    LCD_WriteCmd((line == 0 ? 0x80 : 0xC0) + col);
}

void LCD_WriteStr(unsigned char *str)
{
    while (*str) {
        LCD_WriteData(*str++);
    }
}

void LCD_ClearLine(unsigned char line)
{
    unsigned char i;
    LCD_SetPos(line, 0);
    for (i = 0; i < 16; i++) LCD_WriteData(' ');
}

void LCD_ShowPrompt(unsigned char *line1, unsigned char *line2)
{
    LCD_ClearLine(0);
    LCD_ClearLine(1);
    LCD_SetPos(0, 0);
    LCD_WriteStr(line1);
    LCD_SetPos(1, 0);
    LCD_WriteStr(line2);
}

void LCD_Init(void)
{
    Delay_ms(50);
    LCD_WriteCmd(0x38);
    LCD_WriteCmd(0x0C);
    LCD_WriteCmd(0x06);
    LCD_WriteCmd(0x01);
    Delay_ms(5);
}

/* ====================== 矩阵键盘 ====================== */

unsigned char Key_Scan(void)
{
    unsigned char col, row_data, key = 0;

    P1 = 0xF0;
    _nop_(); _nop_();
    if ((P1 & 0xF0) == 0xF0) return 0;

    for (col = 0; col < 4; col++) {
        P1 = (~(1 << col) & 0x0F) | 0xF0;
        _nop_(); _nop_();
        row_data = P1 & 0xF0;
        if (row_data != 0xF0) {
            if      (!(row_data & 0x80)) key = col + 1;
            else if (!(row_data & 0x40)) key = col + 5;
            else if (!(row_data & 0x20)) key = col + 9;
            else if (!(row_data & 0x10)) key = col + 13;
            break;
        }
    }

    P1 = 0xF0;                     /* 释放 P1, 避免电机误动 */
    return key;
}

unsigned char Key_WaitPress(void)
{
    unsigned char raw;

    while (1) {
        raw = Key_Scan();
        if (raw != 0) {
            Delay_ms(20);
            if (Key_Scan() == raw) break;  /* 消抖确认 */
        }
    }

    /* 等按键释放 */
    while (Key_Scan() != 0);
    Delay_ms(20);

    /* 按键释放后才发声, 避免蜂鸣器干扰键盘扫描 */
    Beep_Start(BEEP_KEY, 3);       /* 60ms 按键音 */

    /* 键值映射 */
    if (raw >= 1 && raw <= 9)  return '0' + raw;
    if (raw == 10)             return '0';
    if (raw == 11)             return 'E';
    if (raw == 12)             return 'C';
    return 0;
}

/* ====================== 蜂鸣器 (P2.5, 与LCD_RW共用) ====================== */

void Beep_Start(unsigned char type, unsigned int tick)
{
    beep_type     = type;
    beep_duration = tick;
    TH0 = beep_TH[type];
    TL0 = beep_TL[type];
    TR0 = 1;
    ET0 = 1;
}

void Beep_Stop(void)
{
    ET0 = 0;
    TR0 = 0;
    BEEP = 0;                      /* P2.5=0 → RW=0, 不影响后续LCD操作 */
    beep_duration = 0;
}

/* ====================== 步进电机 (P1.0~P1.3, JP8) ====================== */

void Motor_StepCW(void)
{
    phase_idx++;
    if (phase_idx >= PHASE_COUNT) phase_idx = 0;
    P1 = (P1 & 0xF0) | phase_table[(unsigned char)phase_idx];
}

void Motor_StepCCW(void)
{
    phase_idx--;
    if (phase_idx < 0) phase_idx = PHASE_COUNT - 1;
    P1 = (P1 & 0xF0) | phase_table[(unsigned char)phase_idx];
}

void Motor_Run(unsigned int steps, unsigned char direction)
{
    unsigned int i;
    for (i = 0; i < steps; i++) {
        if (direction) Motor_StepCW();
        else           Motor_StepCCW();
        Delay_ms(2);
    }
}

void Motor_Reset(void)
{
    P1 &= 0xF0;
    phase_idx = 0;
}

/* ====================== UI 显示 ====================== */

void UI_ShowLocked(void)
{
    LCD_ShowPrompt("  >> LOCKED <<  ", "Press any key...");
    LED_RED   = 0;                 /* 红亮 */
    LED_GREEN = 0;                 /* 绿灭 */
}

void UI_ShowInput(void)
{
    unsigned char i;
    LCD_ClearLine(0);
    LCD_ClearLine(1);
    LCD_SetPos(0, 0);
    LCD_WriteStr("Password: ");
    for (i = 0; i < input_count; i++) LCD_WriteData('*');
    LCD_SetPos(1, 0);
    LCD_WriteStr("[E]=OK  [C]=CLR");
}

void UI_ShowCorrect(void)
{
    LCD_ShowPrompt(" >> CORRECT! << ", "Switch JP8 now ");
    LED_RED   = 1;                 /* 红灭 */
    LED_GREEN = 1;                 /* 绿亮 */
    Beep_Start(BEEP_OK, 15);       /* 成功音 300ms */
}

void UI_ShowSwitchToMotor(void)
{
    LCD_ShowPrompt("JP8 ON,JP4 OFF", "Then press [K1]");
}

void UI_ShowUnlocked(void)
{
    LCD_ShowPrompt(" ** WELCOME **  ", "Auto lock in: 5s");
}

void UI_ShowError(void)
{
    LCD_ShowPrompt(" >> WRONG!! <<  ", "Try again...   ");
    LED_RED   = 0;
    LED_GREEN = 0;
    Beep_Start(BEEP_ERR, 25);      /* 错误音 500ms */
}

void UI_ShowLockout(void)
{
    LCD_ShowPrompt(" ** LOCKOUT **  ", "Wait 30 seconds");
    LED_RED   = 0;
    LED_GREEN = 0;
}

void UI_ShowSwitchToKeypad(void)
{
    LCD_ShowPrompt("JP4 ON,JP8 OFF", "Then press [K2]");
}

/* ====================== 中断服务 ====================== */

void timer0_isr(void) interrupt 1
{
    TH0 = beep_TH[beep_type];
    TL0 = beep_TL[beep_type];
    BEEP = !BEEP;                  /* 翻转 P2.5 */
}

void timer1_isr(void) interrupt 3
{
    TH1 = T1_TH;
    TL1 = T1_TL;

    if (beep_duration > 0) {
        beep_duration--;
        if (beep_duration == 0) Beep_Stop();
    }

    if ((state == S_UNLOCKED || state == S_LOCKOUT) && state_timer > 0) {
        state_timer--;
    }
}

/* ====================== 主函数 ====================== */

void main(void)
{
    unsigned char ch, i;
    unsigned int last_timer;

    /* 端口初始化 */
    P0 = 0xFF;
    P1 = 0xF0;                     /* JP4键盘模式, P1低四位=0不驱动电机 */
    P2 = 0xFF;
    P3 = 0xFF;

    /* 变量初始化 */
    state         = S_LOCKED;
    attempts       = 0;
    state_timer    = 0;
    phase_idx      = 0;
    input_count    = 0;
    beep_duration  = 0;
    for (i = 0; i < PASS_LEN; i++) input_buf[i] = 0;

    LED_RED   = 0;
    LED_GREEN = 0;

    /* 定时器 0: 蜂鸣器 */
    TMOD &= 0xF0;
    TMOD |= 0x01;
    ET0 = 0; TR0 = 0;

    /* 定时器 1: 20ms 节拍 */
    TMOD &= 0x0F;
    TMOD |= 0x10;
    TH1 = T1_TH; TL1 = T1_TL;
    ET1 = 1; TR1 = 1;

    /* LCD 初始化 (EA 尚未开启, ISR 不会干扰) */
    LCD_Init();

    EA = 1;                        /* LCD 就绪后才开中断 */

    /* 开机画面 */
    LCD_ShowPrompt("Password Lock v4", "JP8 OFF! JP4 ON");
    Delay_ms(2000);

    /* ══════════════════════════════════════════════════
     *  主循环
     * ══════════════════════════════════════════════════ */
    while (1) {

        switch (state) {

        case S_LOCKED:
            UI_ShowLocked();
            Key_WaitPress();

            input_count = 0;
            for (i = 0; i < PASS_LEN; i++) input_buf[i] = 0;
            state = S_INPUT;
            UI_ShowInput();
            break;

        case S_INPUT:
            ch = Key_WaitPress();

            if (ch == 'C') {
                input_count = 0;
                for (i = 0; i < PASS_LEN; i++) input_buf[i] = 0;
                UI_ShowInput();
            }
            else if (ch == 'E') {
                if (input_count < PASS_LEN) {
                    LCD_SetPos(1, 0);
                    LCD_WriteStr("Need 4 digits! ");
                    Delay_ms(800);
                    UI_ShowInput();
                } else {
                    for (i = 0; i < PASS_LEN; i++)
                        if (input_buf[i] != password[i]) break;

                    if (i == PASS_LEN) {
                        attempts = 0;
                        UI_ShowCorrect();
                        Delay_ms(1200);
                        UI_ShowSwitchToMotor();
                        state = S_VERIFY_OK;

                        while (BTN_K1 != 0);
                        Delay_ms(20);
                        while (BTN_K1 == 0);
                        Delay_ms(20);
                    } else {
                        attempts++;
                        UI_ShowError();
                        Delay_ms(1500);
                        if (attempts >= MAX_ATTEMPTS) {
                            state = S_LOCKOUT;
                            state_timer = LOCKOUT_TICK;
                            last_timer  = 0xFFFF;
                            UI_ShowLockout();
                        } else {
                            state = S_VERIFY_FAIL;
                        }
                    }
                }
            }
            else if (ch >= '0' && ch <= '9') {
                if (input_count < PASS_LEN) {
                    input_buf[input_count] = ch - '0';
                    input_count++;
                    UI_ShowInput();
                }
            }
            break;

        case S_VERIFY_FAIL:
            input_count = 0;
            for (i = 0; i < PASS_LEN; i++) input_buf[i] = 0;
            state = S_INPUT;
            UI_ShowInput();
            LED_RED   = 0;
            LED_GREEN = 0;
            break;

        case S_VERIFY_OK:
            UI_ShowUnlocked();
            Beep_Start(BEEP_OK, 15);
            Delay_ms(300);

            /* 此时用户应已插上 JP8 (电机) */
            Motor_Run(512, 1);
            Motor_Reset();
            Delay_ms(200);

            state = S_UNLOCKED;
            state_timer = UNLOCK_TICK;
            last_timer  = 0xFFFF;   /* 强制首次刷新 */
            LED_RED   = 1;
            LED_GREEN = 1;
            break;

        case S_UNLOCKED:
            /* 实时显示倒计时 */
            if (state_timer != last_timer) {
                last_timer = state_timer;
                LCD_SetPos(1, 0);
                LCD_WriteStr("Auto lock in:   ");
                LCD_SetPos(1, 14);
                if (state_timer >= 50) {
                    LCD_WriteData('0' + (state_timer / 50) % 10);
                    LCD_WriteData('s');
                } else {
                    LCD_WriteData('0' + (state_timer * 20 / 1000) % 10);
                    LCD_WriteData('.');
                    LCD_WriteData('0' + (state_timer * 20 / 100) % 10);
                    LCD_WriteData('s');
                }
            }

            if (state_timer == 0) {
                LCD_ShowPrompt("Auto locking...", "                ");
                Delay_ms(500);

                Motor_Run(512, 0);     /* 反转关锁 */
                Motor_Reset();

                UI_ShowSwitchToKeypad();

                while (BTN_K2 != 0);
                Delay_ms(20);
                while (BTN_K2 == 0);
                Delay_ms(20);

                state = S_LOCKED;
                LED_RED   = 0;
                LED_GREEN = 0;
            }
            break;

        case S_LOCKOUT:
            /* 显示剩余锁定时间 */
            if (state_timer != last_timer) {
                last_timer = state_timer;
                LCD_SetPos(1, 0);
                LCD_WriteStr("Remain:         ");
                LCD_SetPos(1, 8);
                LCD_WriteData('0' + (state_timer / 50) / 10 % 10);
                LCD_WriteData('0' + (state_timer / 50) % 10);
                LCD_WriteStr("s  ");
            }

            if (state_timer == 0) {
                attempts = 0;
                state = S_LOCKED;
                LED_RED   = 0;
                LED_GREEN = 0;
            }
            break;

        } /* end switch */
    } /* end while */
}
