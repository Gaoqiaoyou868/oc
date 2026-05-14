#include <reg51.h>

// --- 引脚定义 ---
// 南北方向灯 (P1.3 - P1.5)
sbit NS_RED    = P1^3;
sbit NS_YELLOW = P1^5;
sbit NS_GREEN  = P1^4;
// 东西方向灯 (P1.0 - P1.2)
sbit EW_RED    = P1^0;
sbit EW_YELLOW = P1^2;
sbit EW_GREEN  = P1^1;

sbit EMERGENCY_KEY = P3^2;   // 紧急按键

// 数码管位选 (高电平有效)
sbit DIG_NS10 = P2^0;
sbit DIG_NS1  = P2^1;
sbit DIG_EW10 = P2^2;
sbit DIG_EW1  = P2^3;

// 共阳极段码表 (0~9 和 全灭，P0口段选低有效)
unsigned char code seg_table[] = {
    0xC0, // 0
    0xF9, // 1
    0xA4, // 2
    0xB0, // 3
    0x99, // 4
    0x92, // 5
    0x82, // 6
    0xF8, // 7
    0x80, // 8
    0x90, // 9
    0xFF  // 全灭
};

// 显示缓冲区 (0~9 或 10)
volatile unsigned char disp_buf[4] = {10, 10, 10, 10};

// 状态定义
#define STATE_NS_GO    0    // 南北通行
#define STATE_NS_WARN  1    // 南北缓冲
#define STATE_EW_GO    2    // 东西通行
#define STATE_EW_WARN  3    // 东西缓冲

unsigned char current_state = STATE_NS_GO;
unsigned char ns_count = 20;     // 南北倒计时
unsigned char ew_count = 0;      // 东西倒计时

// 紧急控制
volatile bit emergency_mode = 0;
bit saved_once = 0;
unsigned char state_backup;
unsigned char ns_backup, ew_backup;

// 时基标志
volatile bit flag_1s = 0;
volatile bit flag_500ms = 0;
volatile bit flag_update_lights = 0;   // 新增：触发信号灯更新

// 延时函数 (消抖用)
void delay_ms(unsigned int ms) {
    unsigned int i, j;
    for(i = 0; i < ms; i++)
        for(j = 0; j < 120; j++);
}

// 更新信号灯状态
void update_lights() {
    // 先全灭 (共阳：1=灭)
    NS_RED = 1; NS_YELLOW = 1; NS_GREEN = 1;
    EW_RED = 1; EW_YELLOW = 1; EW_GREEN = 1;

    switch(current_state) {
        case STATE_NS_GO:      // 南北绿，东西红
            NS_GREEN = 0;
            EW_RED = 0;
            break;
        case STATE_NS_WARN:    // 南北黄闪烁，东西红
            EW_RED = 0;
            NS_YELLOW = flag_500ms ? 0 : 1;  // 根据500ms标志亮灭
            break;
        case STATE_EW_GO:      // 南北红，东西绿
            NS_RED = 0;
            EW_GREEN = 0;
            break;
        case STATE_EW_WARN:    // 南北红，东西黄闪烁
            NS_RED = 0;
            EW_YELLOW = flag_500ms ? 0 : 1;
            break;
    }
}

// 更新数码管显示内容
void update_display(unsigned char ns, unsigned char ew) {
    if(ns > 0) {
        disp_buf[0] = ns / 10;
        disp_buf[1] = ns % 10;
    } else {
        disp_buf[0] = 10;
        disp_buf[1] = 10;
    }
    if(ew > 0) {
        disp_buf[2] = ew / 10;
        disp_buf[3] = ew % 10;
    } else {
        disp_buf[2] = 10;
        disp_buf[3] = 10;
    }
}

// 定时器0初始化 (1ms)
void timer0_init() {
    TMOD &= 0xF0;
    TMOD |= 0x01;
    TH0 = 0xFC;    // 12MHz, 1ms
    TL0 = 0x18;
    ET0 = 1;
    TR0 = 1;
}

// 外部中断0初始化 (下降沿触发)
void ext0_init() {
    IT0 = 1;
    EX0 = 1;
}

// 主函数
void main() {
    timer0_init();
    ext0_init();
    EA = 1;

    update_lights();
    update_display(ns_count, ew_count);

    while(1) {
        // ---------- 紧急模式 ----------
        if(emergency_mode) {
            if(!saved_once) {          // 首次进入，保存现场
                state_backup = current_state;
                ns_backup = ns_count;
                ew_backup = ew_count;
                saved_once = 1;
            }
            // 强制全红灯
            NS_RED = 0; NS_YELLOW = 1; NS_GREEN = 1;
            EW_RED = 0; EW_YELLOW = 1; EW_GREEN = 1;
            // 显示保持中断前的时间
            update_display(ns_backup, ew_backup);

            // 松开恢复 (电平检测)
            if(EMERGENCY_KEY == 1) {
                delay_ms(20);
                if(EMERGENCY_KEY == 1)
                    emergency_mode = 0;
            }
        }
        // ---------- 正常模式 ----------
        else {
            if(saved_once) {           // 刚从紧急模式退出，恢复现场
                current_state = state_backup;
                ns_count = ns_backup;
                ew_count = ew_backup;
                saved_once = 0;
                update_lights();
                update_display(ns_count, ew_count);
            }

            // 每0.5秒更新一次信号灯 (保证黄灯闪烁频率)
            if(flag_update_lights) {
                flag_update_lights = 0;
                update_lights();
            }

            // 每秒状态机倒计时
            if(flag_1s) {
                flag_1s = 0;
                switch(current_state) {
                    case STATE_NS_GO:
                        if(ns_count > 0) ns_count--;
                        if(ns_count == 0) {
                            current_state = STATE_NS_WARN;
                            ns_count = 3;
                            ew_count = 0;
                        }
                        break;
                    case STATE_NS_WARN:
                        if(ns_count > 0) ns_count--;
                        if(ns_count == 0) {
                            current_state = STATE_EW_GO;
                            ns_count = 0;
                            ew_count = 15;
                        }
                        break;
                    case STATE_EW_GO:
                        if(ew_count > 0) ew_count--;
                        if(ew_count == 0) {
                            current_state = STATE_EW_WARN;
                            ns_count = 0;
                            ew_count = 3;
                        }
                        break;
                    case STATE_EW_WARN:
                        if(ew_count > 0) ew_count--;
                        if(ew_count == 0) {
                            current_state = STATE_NS_GO;
                            ns_count = 20;
                            ew_count = 0;
                        }
                        break;
                }
                update_display(ns_count, ew_count);
            }
        }
    }
}

// 定时器0中断服务程序 (1ms)
void timer0_ISR() interrupt 1 {
    static unsigned char scan_pos = 0;
    static unsigned int ms_count = 0;
    static unsigned int half_count = 0;

    TH0 = 0xFC;
    TL0 = 0x18;

    // --- 数码管动态扫描 ---
    P2 = 0x00;                         // 先关所有位选
    P0 = seg_table[disp_buf[scan_pos]]; // 送段码
    switch(scan_pos) {
        case 0: DIG_NS10 = 1; break;
        case 1: DIG_NS1  = 1; break;
        case 2: DIG_EW10 = 1; break;
        case 3: DIG_EW1  = 1; break;
    }
    scan_pos = (scan_pos + 1) & 0x03;   // 0~3循环

    // --- 时基产生 ---
    if(++ms_count >= 1000) {
        ms_count = 0;
        flag_1s = 1;
    }
    if(++half_count >= 500) {
        half_count = 0;
        flag_500ms = ~flag_500ms;
        flag_update_lights = 1;          // 通知主程序刷新信号灯
    }
}

// 外部中断0服务程序 (紧急按键)
void ext0_ISR() interrupt 0 {
    if(emergency_mode) {
        emergency_mode = 0;   // 若已在紧急模式，再次按下则退出
    } else {
        emergency_mode = 1;   // 进入紧急模式
    }
}