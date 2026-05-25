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
│   │   ├── Keil源码/               #     所有 Keil C 源文件 + 项目文件
│   │   │   ├── jiaotongdeng.c      ─ Proteus仿真版
│   │   │   ├── jiaotongdeng_board.c─ 实物版（普中A2）
│   │   │   ├── doubao.c            ─ 备选版（豆包生成）
│   │   │   ├── CZ2.c               ─ 备选版
│   │   │   └── 提示词.txt
│   │   ├── Proteus仿真/             #     仿真文件 + 原理图 PDF
│   │   │   ├── 交通灯.pdsprj
│   │   │   ├── 交通灯实验_proteus原理图.PDF
│   │   │   └── 交通灯实验_proteus原理图_1.PDF
│   │   └── 实验文档/               #     实验指导书
│   │       └── 实验二 交通灯实验.doc
│   ├── liushuideng_51/             #   流水灯 Proteus 仿真
│   │   └── liushuideng.pdsprj
│   ├── stepper_motor_51/           #   步进电机实验（已清理，尚未开始）
│   └── password_lock_51/           #   密码锁（已放弃）v4最终版
│         ├── main.c
│         ├── test_lcd.c
│         ├── test_lcd2.c
│         ├── test_lcd3.c
│         └── ...Keil 项目文件
├── reference/                      # 参考材料
│   ├── 51单片机资料/               #   手册/原理图/官方62个实验
│   └── study/
│       └── 学习笔记/               #   步进电机/红外/LCD等学习练习
├── database/                       # 数据库课程设计（英超球员 2019-2023）
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

## 文件清理规则

- 每完成一个项目或一个阶段性任务后，**必须**立即整理文件
- 清理规则：
  - 删除编译/构建产物（如 .obj, .lst, .lnp, .M51, .plg, .bak 等）
  - **⚠ .hex 文件绝对不能删除**（烧录文件，必须保留）
  - 删除临时调试脚本、中间截图、OCR输出、日志文件
  - 删除空目录
  - 保留：源代码、项目配置文件、最终文档/报告、最终的截图文件夹
- 清理后必须 `git add .` + `git commit`，提交信息标注"清理"字样

## git 自动提交规则

- 每次我改完代码后，**必须**自动执行 `git add .` + `git commit`，不需要用户提醒
- 提交信息必须用中文，简要说明本次改动内容
- 第一次提交时已设置好本仓库的用户信息，无需再配置

## git 推送（GitHub）

- 远程仓库: `origin` → `https://github.com/Gaoqiaoyou868/oc.git`
- git 已配置全局代理 `socks5://127.0.0.1:10808`（V2Ray），开 VPN 后终端自动走代理
- 每次用户开 VPN 时，运行 `git push` 即可推送到 GitHub
- 如果由我提交了代码且检测到网络可达，我会自动执行 `git push`