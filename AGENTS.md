# AGENTS.md

## 目录结构

```
oc/
├── .opencode/                      # OpenCode 配置 + skills
├── projects/                       # 51单片机项目
│   ├── smart_fan_51/               #   红外遥控智能风扇（期末项目）v1.0
│   │   ├── main.c                  ─ 硬件版（红外+按键+LCD+步进电机）
│   │   ├── main_proteus.c          ─ Proteus仿真版（纯按键）
│   │   ├── 使用手册与电路说明.md     ─ 完整文档
│   │   └── SmartFan_backup/        ─ 早期版本存档
│   ├── jiaotongdeng_51/            #   交通灯实验（实验二）
│   │   ├── jiaotongdeng.c          ─ Proteus仿真程序
│   │   ├── 交通灯实验_proteus原理图.PDF
│   │   ├── 实验二 交通灯实验.doc
│   │   └── 提示词.txt
│   ├── stepper_motor_51/           #   步进电机实验（已清理，尚未开始）
│   └── password_lock_51/           #   密码锁（已放弃）v4最终版
│         ├── main.c
│         ├── test_lcd.c
│         ├── test_lcd2.c
│         └── test_lcd3.c
├── reference/                      # 参考材料
│   ├── 51单片机资料/               #   手册/原理图/官方62个实验
│   └── study/
│       └── 学习笔记/               #   步进电机/红外/LCD等学习练习
├── database/                       # 数据库课程设计（英超球员 2019-2023）
│   └── 数据库课程设计/
├── scripts/                        # 共享工具脚本
│   └── extract_pdf.py
└── AGENTS.md                       # 本文件
```

## 硬件平台

- 主控: STC89C52RC @ 11.0592MHz (PDIP-40)
- 开发板: 普中A2（A2/A3/A4/A5/A6/A7 全系列引脚兼容）
- 仿真: Proteus 8.x（仿真实物差异注意晶振: 仿真12MHz vs 实物11.0592MHz）
- IDE: Keil uVision 5

## 关键规则

- 代码注释全部中文（老师会提问）
- LCD写操作必须 `EA=0` → 写 → `EA=1`（P2.5蜂鸣器与LCD_RW共用）
- 写完LCD后 `P0=0xFF` 释放总线
- 步进电机写P1时 `P1 = (P1 & 0xF0) | phase` 保护高4位
- 初始化顺序: 端口→变量→定时器→外设→中断→EA=1
- 仿真版(Proteus 12MHz)与实物版(11.0592MHz)定时初值不同

## 类型定义（所有项目统一）

```c
typedef unsigned int u16;
typedef unsigned char u8;
```