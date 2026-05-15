   #include <reg51.h>

// --- 引脚定义（与原代码完全一致）---
// 南北方向灯 (P1.3 - P1.5)
sbit NS_RED    = P1^3;
sbit NS_YELLOW = P1^5;
sbit NS_GREEN  = P1^4;
// 东西方向灯 (P1.0 - P1.2)
sbit EW_RED    = P1^0;
sbit EW_YELLOW = P1^2;
sbit EW_GREEN  = P1^1;

sbit EMERGENCY_KEY = P3^2;

// 数码管位选 (高电平有效)
sbit DIG_NS10 = P2^0;
sbit DIG_NS1  = P2^1;
sbit DIG_EW10 = P2^2;
sbit DIG_EW1  = P2^3;

// 共阳极段码表 (0~9, 全灭)
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

volatile unsigned char disp_buf[4] = {10,10,10,10};

// --- 状态定义（与原代码一致）---
#define STATE_NS_GO    0    // 南北通行
#define STATE_NS_WARN  1    // 南北缓冲（黄灯）
#define STATE_EW_GO    2    // 东西通行
#define STATE_EW_WARN  3    // 东西缓冲（黄灯）

unsigned char current_state = STATE_NS_GO;

// --- 已修改为演示用时间参数 ---
#define NS_GREEN_TIME 10    // 南北绿灯纯通行时间（原20秒→现10秒）
#define EW_GREEN_TIME  5    // 东西绿灯纯通行时间（原15秒→现5秒）
#define YELLOW_TIME    3    // 黄灯缓冲时间（保持3秒不变）

// 全局统一倒计时（两个方向显示数值完全相同）
unsigned char countdown = NS_GREEN_TIME + YELLOW_TIME;  // 初始：南北绿+黄总时间13秒

// 紧急控制相关变量
volatile bit emergency_mode = 0;
bit saved_once = 0;
unsigned char state_backup;      // 备份进入紧急前的状态
unsigned char countdown_backup;  // 备份进入紧急前的倒计时

// 时基标志
volatile bit flag_1s = 0;
volatile bit flag_500ms = 0;
volatile bit flag_update_lights = 0;

void delay_ms(unsigned int ms) {
    unsigned int i, j;
    for(i=0; i<ms; i++)
        for(j=0; j<120; j++);
}

// 更新信号灯状态
void update_lights() {
    // 先全灭所有灯
    NS_RED = 1; NS_YELLOW = 1; NS_GREEN = 1;
    EW_RED = 1; EW_YELLOW = 1; EW_GREEN = 1;

    switch(current_state) {
        case STATE_NS_GO:    // 南北通行：南北绿，东西红
            NS_GREEN = 0;
            EW_RED = 0;
            break;
        case STATE_NS_WARN:  // 南北缓冲：南北黄闪烁，东西红
            EW_RED = 0;
            NS_YELLOW = flag_500ms ? 0 : 1;  // 500ms交替闪烁
            break;
        case STATE_EW_GO:    // 东西通行：南北红，东西绿
            NS_RED = 0;
            EW_GREEN = 0;
            break;
        case STATE_EW_WARN:  // 东西缓冲：南北红，东西黄闪烁
            NS_RED = 0;
            EW_YELLOW = flag_500ms ? 0 : 1;  // 500ms交替闪烁
            break;
    }
}

// 更新数码管显示缓冲区（严格遵循表格要求）
void update_display() {
    // 南北方向：显示距离变红的时间（绿灯+黄灯总剩余/黄灯剩余）
    disp_buf[0] = countdown / 10;
    disp_buf[1] = countdown % 10;
    // 东西方向：显示距离变绿的时间（红灯剩余）
    disp_buf[2] = countdown / 10;
    disp_buf[3] = countdown % 10;
}

// 定时器0初始化（1ms中断，11.0592MHz晶振）
void timer0_init() {
    TMOD &= 0xF0;
    TMOD |= 0x01;
    TH0 = 0xFC;
    TL0 = 0x18;
    ET0 = 1;
    TR0 = 1;
}

// 外部中断0初始化（下降沿触发紧急模式）
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
        // ==================== 紧急模式处理 ====================
        if(emergency_mode) {
            if(!saved_once) {
                state_backup = current_state;
                countdown_backup = countdown;
                saved_once = 1;
            }
            
            // 强制全红灯
            NS_RED = 0; NS_YELLOW = 1; NS_GREEN = 1;
            EW_RED = 0; EW_YELLOW = 1; EW_GREEN = 1;
            
            // 数码管显示00
            disp_buf[0] = 0;
            disp_buf[1] = 0;
            disp_buf[2] = 0;
            disp_buf[3] = 0;

            // 检测按键释放退出
            if(EMERGENCY_KEY == 1) {
                delay_ms(20);
                if(EMERGENCY_KEY == 1) {
                    emergency_mode = 0;
                }
            }
        }
        // ==================== 正常模式处理 ====================
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

                // 状态切换逻辑
                switch(current_state) {
                    case STATE_NS_GO:
                        if(countdown == YELLOW_TIME) {
                            current_state = STATE_NS_WARN;
                        }
                        else if(countdown == 0) {
                            current_state = STATE_EW_GO;
                            countdown = EW_GREEN_TIME + YELLOW_TIME;  // 东西绿+黄总时间8秒
                        }
                        break;

                    case STATE_NS_WARN:
                        if(countdown == 0) {
                            current_state = STATE_EW_GO;
                            countdown = EW_GREEN_TIME + YELLOW_TIME;
                        }
                        break;

                    case STATE_EW_GO:
                        if(countdown == YELLOW_TIME) {
                            current_state = STATE_EW_WARN;
                        }
                        else if(countdown == 0) {
                            current_state = STATE_NS_GO;
                            countdown = NS_GREEN_TIME + YELLOW_TIME;  // 南北绿+黄总时间13秒
                        }
                        break;

                    case STATE_EW_WARN:
                        if(countdown == 0) {
                            current_state = STATE_NS_GO;
                            countdown = NS_GREEN_TIME + YELLOW_TIME;
                        }
                        break;
                }

                update_display();
            }
        }
    }
}

// 定时器0中断服务程序
void timer0_ISR() interrupt 1 {
    static unsigned char scan_pos = 0;
    static unsigned int ms_count = 0;
    static unsigned int half_count = 0;

    TH0 = 0xFC;
    TL0 = 0x18;

    // 数码管动态扫描
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

// 外部中断0服务程序
void ext0_ISR() interrupt 0 {
    emergency_mode = ~emergency_mode;
}