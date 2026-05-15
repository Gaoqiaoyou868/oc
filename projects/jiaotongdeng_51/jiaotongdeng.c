 #include <reg51.h>

// --- 引脚定义 ---
// 南北方向 (P1.3 - P1.5)
sbit NS_RED    = P1^3;
sbit NS_YELLOW = P1^5;
sbit NS_GREEN  = P1^4;
// 东西方向 (P1.0 - P1.2)
sbit EW_RED    = P1^0;
sbit EW_YELLOW = P1^2;
sbit EW_GREEN  = P1^1;

sbit EMERGENCY_KEY = P3^2;

// 数码管位选 (高电平有效)
sbit DIG_NS10 = P2^0;
sbit DIG_NS1  = P2^1;
sbit DIG_EW10 = P2^2;
sbit DIG_EW1  = P2^3;

// 数码管段码表 (0~9, 全灭)
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

volatile unsigned char disp_buf[4] = {0,0,0,0};

// 状态定义
#define STATE_NS_GO    0    // 南北通行
#define STATE_NS_WARN  1    // 南北缓冲
#define STATE_EW_GO    2    // 东西通行
#define STATE_EW_WARN  3    // 东西缓冲

unsigned char current_state = STATE_NS_GO;

// 定时参数
#define NS_GREEN  20
#define NS_YELLOW  3
#define EW_GREEN  15
#define EW_YELLOW  3
#define NS_TOTAL  (NS_GREEN + NS_YELLOW)  // 23
#define EW_TOTAL  (EW_GREEN + EW_YELLOW)  // 18

// 单个倒计时变量，两个数码管显示相同值
unsigned char countdown = NS_TOTAL;

// 紧急模式
volatile bit emergency_mode = 0;
bit saved_once = 0;
unsigned char state_backup;
unsigned char countdown_backup;

// ʱ����־
volatile bit flag_1s = 0;
volatile bit flag_500ms = 0;
volatile bit flag_update_lights = 0;

void delay_ms(unsigned int ms) {
    unsigned int i, j;
    for(i=0; i<ms; i++)
        for(j=0; j<120; j++);
}

// �����źŵƣ����ݵ�ǰ״̬��500ms��˸��־��
void update_lights() {
    NS_RED = 1; NS_YELLOW = 1; NS_GREEN = 1;
    EW_RED = 1; EW_YELLOW = 1; EW_GREEN = 1;

    switch(current_state) {
        case STATE_NS_GO:
            NS_GREEN = 0;
            EW_RED = 0;
            break;
        case STATE_NS_WARN:
            EW_RED = 0;
            NS_YELLOW = flag_500ms ? 0 : 1;  // 0.5s����
            break;
        case STATE_EW_GO:
            NS_RED = 0;
            EW_GREEN = 0;
            break;
        case STATE_EW_WARN:
            NS_RED = 0;
            EW_YELLOW = flag_500ms ? 0 : 1;
            break;
    }
}

// 更新数码管显示，两个数码管显示相同的倒计数值
void update_display() {
    disp_buf[0] = countdown / 10;
    disp_buf[1] = countdown % 10;
    disp_buf[2] = countdown / 10;
    disp_buf[3] = countdown % 10;
}

void timer0_init() {
    TMOD &= 0xF0;
    TMOD |= 0x01;
    TH0 = 0xFC;
    TL0 = 0x18;
    ET0 = 1;
    TR0 = 1;
}

void ext0_init() {
    IT0 = 1;
    EX0 = 1;
}

void main() {
    timer0_init();
    ext0_init();
    EA = 1;

    update_lights();
    update_display();

    while(1) {
        // ---------- 紧急模式 ----------
        if(emergency_mode) {
            if(!saved_once) {
                state_backup = current_state;
                countdown_backup = countdown;
                saved_once = 1;
            }
            // 全红
            NS_RED = 0; NS_YELLOW = 1; NS_GREEN = 1;
            EW_RED = 0; EW_YELLOW = 1; EW_GREEN = 1;
            // 显示00
            disp_buf[0] = 0; disp_buf[1] = 0;
            disp_buf[2] = 0; disp_buf[3] = 0;

            if(EMERGENCY_KEY == 1) {
                delay_ms(20);
                if(EMERGENCY_KEY == 1)
                    emergency_mode = 0;
            }
        }
        // ---------- 正常模式 ----------
        else {
            if(saved_once) {
                current_state = state_backup;
                countdown = countdown_backup;
                saved_once = 0;
                update_lights();
                update_display();
            }

            if(flag_update_lights) {
                flag_update_lights = 0;
                update_lights();
            }

            if(flag_1s) {
                flag_1s = 0;
                if(countdown > 0) countdown--;

                switch(current_state) {
                    case STATE_NS_GO:   // 南北通行：绿灯，倒计时到达黄灯时间时切换
                        if(countdown == NS_YELLOW) {
                            current_state = STATE_NS_WARN;
                        }
                        break;
                    case STATE_NS_WARN: // 南北缓冲：黄灯闪烁
                        if(countdown == 0) {
                            current_state = STATE_EW_GO;
                            countdown = EW_TOTAL;  // 设为18
                        }
                        break;
                    case STATE_EW_GO:   // 东西通行：绿灯
                        if(countdown == EW_YELLOW) {
                            current_state = STATE_EW_WARN;
                        }
                        break;
                    case STATE_EW_WARN: // 东西缓冲：黄灯闪烁
                        if(countdown == 0) {
                            current_state = STATE_NS_GO;
                            countdown = NS_TOTAL;  // 设为23
                        }
                        break;
                }
                update_display();
            }
        }
    }
}

void timer0_ISR() interrupt 1 {
    static unsigned char scan_pos = 0;
    static unsigned int ms_count = 0;
    static unsigned int half_count = 0;

    TH0 = 0xFC;
    TL0 = 0x18;

    P2 = 0x00;
    P0 = seg_table[disp_buf[scan_pos]];
    switch(scan_pos) {
        case 0: DIG_NS10 = 1; break;
        case 1: DIG_NS1  = 1; break;
        case 2: DIG_EW10 = 1; break;
        case 3: DIG_EW1  = 1; break;
    }
    scan_pos = (scan_pos + 1) & 0x03;

    if(++ms_count >= 1000) {
        ms_count = 0;
        flag_1s = 1;
    }
    if(++half_count >= 500) {
        half_count = 0;
        flag_500ms = ~flag_500ms;
        flag_update_lights = 1;
    }
}

void ext0_ISR() interrupt 0 {
    if(emergency_mode) {
        emergency_mode = 0;
    } else {
        emergency_mode = 1;
    }
}