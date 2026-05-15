#include <reg51.h>

// ===== 引脚定义（按你的要求） =====
// 南北方向灯
sbit NS_RED    = P1^3;
sbit NS_YELLOW = P1^5;
sbit NS_GREEN  = P1^4;
// 东西方向灯
sbit EW_RED    = P1^0;
sbit EW_YELLOW = P1^2;
sbit EW_GREEN  = P1^1;

sbit EMERGENCY_KEY = P3^2;  // 紧急按键 (INT0)

// 数码管位选（共阳极，高有效）
sbit DIG_EW_TENS = P2^0;   // 东西十位
sbit DIG_EW_ONES = P2^1;   // 东西个位
sbit DIG_NS_TENS = P2^2;   // 南北十位
sbit DIG_NS_ONES = P2^3;   // 南北个位

// ===== 全局变量与状态定义 =====
#define STATE_NS_GO     0   // 南北通行
#define STATE_NS_WARN   1   // 南北缓冲（黄灯闪）
#define STATE_EW_GO     2   // 东西通行
#define STATE_EW_WARN   3   // 东西缓冲（黄灯闪）

unsigned char code seg_table[] = {  // 共阳极段码（0~9）
    0xC0, 0xF9, 0xA4, 0xB0, 0x99,
    0x92, 0x82, 0xF8, 0x80, 0x90
};

unsigned char disp_buf[4];         // 显示缓冲区：0-EW十位,1-EW个位,2-NS十位,3-NS个位
unsigned char state;               // 当前状态
unsigned char time_left;           // 当前状态剩余秒数

bit emergency_mode;                // 紧急模式标志
unsigned char saved_state;         // 保存的状态
unsigned char saved_time_left;     // 保存的剩余秒数
bit emergency_triggered;           // 中断触发标志

bit yellow_blink;                  // 黄灯闪烁翻转位（0.5s 翻转一次）

// ===== 函数声明 =====
void UpdateDisplayBuffer(void);
void UpdateLights(void);
void Delay10ms(void);

// ===== 定时器0初始化（1ms中断） =====
void Timer0_Init(void) {
    TMOD &= 0xF0;     // 不影响定时器1
    TMOD |= 0x01;     // 定时器0，模式1（16位）
    TH0 = (65536 - 1000) / 256;  // 1ms @12MHz
    TL0 = (65536 - 1000) % 256;
    ET0 = 1;          // 允许定时器0中断
    TR0 = 1;          // 启动定时器0
}

// ===== 外部中断0初始化（下降沿触发） =====
void ExtInt0_Init(void) {
    IT0 = 1;          // 下降沿触发
    EX0 = 1;          // 允许外部中断0
}

// ===== 主函数 =====
void main(void) {
    // 初始化状态
    state = STATE_NS_GO;
    time_left = 20;
    emergency_mode = 0;
    emergency_triggered = 0;
    yellow_blink = 0;
    
    Timer0_Init();
    ExtInt0_Init();
    EA = 1;           // 开总中断
    
    while (1) {
        // 处理紧急按键触发
        if (emergency_triggered) {
            Delay10ms();                    // 去抖
            if (EMERGENCY_KEY == 0) {       // 按键确实按下
                if (!emergency_mode) {
                    // 保存当前运行环境
                    saved_state = state;
                    saved_time_left = time_left;
                    emergency_mode = 1;     // 进入紧急模式
                }
            }
            emergency_triggered = 0;
        }
        
        // 紧急模式下检测按键释放以恢复正常运行
        if (emergency_mode) {
            if (EMERGENCY_KEY == 1) {       // 检测到按键松开
                Delay10ms();                // 去抖
                if (EMERGENCY_KEY == 1) {
                    emergency_mode = 0;     // 退出紧急模式
                    // 恢复现场
                    state = saved_state;
                    time_left = saved_time_left;
                    UpdateDisplayBuffer();  // 刷新显示缓冲
                }
            }
        }
    }
}

// ===== 定时器0中断服务（1ms） =====
void Timer0_ISR() interrupt 1 {
    static unsigned int ms_count = 0;
    static unsigned int blink_count = 0;
    static unsigned char scan_index = 0;
    
    TH0 = (65536 - 1000) / 256;  // 重装初值
    TL0 = (65536 - 1000) % 256;
    
    // ---- 动态扫描显示 ----
    // 先关闭所有位选
    DIG_EW_TENS = 0;
    DIG_EW_ONES = 0;
    DIG_NS_TENS = 0;
    DIG_NS_ONES = 0;
    // 输出段码并开启对应位选
    switch (scan_index) {
        case 0: P0 = seg_table[disp_buf[0]]; DIG_EW_TENS = 1; break;
        case 1: P0 = seg_table[disp_buf[1]]; DIG_EW_ONES = 1; break;
        case 2: P0 = seg_table[disp_buf[2]]; DIG_NS_TENS = 1; break;
        case 3: P0 = seg_table[disp_buf[3]]; DIG_NS_ONES = 1; break;
    }
    scan_index = (scan_index + 1) & 0x03;   // 0~3循环
    
    // ---- 非紧急模式下的计时与状态控制 ----
    if (!emergency_mode) {
        // 黄灯闪烁计数（0.5s翻转一次）
        blink_count++;
        if (blink_count >= 500) {
            blink_count = 0;
            yellow_blink = ~yellow_blink;
        }
        
        // 1秒计时
        ms_count++;
        if (ms_count >= 1000) {
            ms_count = 0;
            // 倒计时递减
            if (time_left > 0) {
                time_left--;
            }
            // 状态切换
            if (time_left == 0) {
                state = (state + 1) % 4;
                switch (state) {
                    case STATE_NS_GO:   time_left = 20; break;
                    case STATE_NS_WARN: time_left = 3;  break;
                    case STATE_EW_GO:   time_left = 15; break;
                    case STATE_EW_WARN: time_left = 3;  break;
                }
            }
            UpdateDisplayBuffer(); // 更新显示缓冲区
        }
        UpdateLights();            // 根据当前状态设置LED
    } else {
        // 紧急模式：全红灯，冻结倒计时显示（缓冲区保持不变）
        NS_RED = 0;    // 点亮
        EW_RED = 0;
        NS_YELLOW = 1;
        NS_GREEN = 1;
        EW_YELLOW = 1;
        EW_GREEN = 1;
    }
}

// ===== 更新显示缓冲区（根据状态计算东西、南北显示的秒数） =====
void UpdateDisplayBuffer(void) {
    unsigned char ns_disp, ew_disp;
    
    switch (state) {
        case STATE_NS_GO:   // 南北通行，东西红灯
            ns_disp = time_left;
            ew_disp = time_left + 3;  // 东西红灯还将持续当前态+下一缓冲态
            break;
        case STATE_NS_WARN: // 南北黄闪，东西红灯
            ns_disp = time_left;
            ew_disp = time_left;
            break;
        case STATE_EW_GO:   // 东西通行，南北红灯
            ew_disp = time_left;
            ns_disp = time_left + 3;
            break;
        case STATE_EW_WARN: // 东西黄闪，南北红灯
            ew_disp = time_left;
            ns_disp = time_left;
            break;
    }
    // 拆分十位、个位
    disp_buf[0] = ew_disp / 10;  // 东西十位
    disp_buf[1] = ew_disp % 10;  // 东西个位
    disp_buf[2] = ns_disp / 10;  // 南北十位
    disp_buf[3] = ns_disp % 10;  // 南北个位
}

// ===== 根据状态设置交通灯（黄灯闪烁控制） =====
void UpdateLights(void) {
    // 先全部熄灭（共阳极，输出1为灭）
    NS_RED = 1; NS_YELLOW = 1; NS_GREEN = 1;
    EW_RED = 1; EW_YELLOW = 1; EW_GREEN = 1;
    
    switch (state) {
        case STATE_NS_GO:   // 南北绿，东西红
            NS_GREEN = 0;
            EW_RED = 0;
            break;
        case STATE_NS_WARN: // 南北黄闪，东西红
            EW_RED = 0;
            NS_YELLOW = yellow_blink ? 0 : 1;  // 闪烁
            break;
        case STATE_EW_GO:   // 东西绿，南北红
            EW_GREEN = 0;
            NS_RED = 0;
            break;
        case STATE_EW_WARN: // 东西黄闪，南北红
            NS_RED = 0;
            EW_YELLOW = yellow_blink ? 0 : 1;
            break;
    }
}

// ===== 外部中断0服务 =====
void ExtInt0_ISR() interrupt 0 {
    if (!emergency_mode) {
        emergency_triggered = 1;   // 设置标志，由主循环处理
    }
}

// ===== 简单延时函数（约10ms，用于按键消抖） =====
void Delay10ms(void) {
    unsigned int i, j;
    for (i = 0; i < 10; i++)
        for (j = 0; j < 120; j++);
}