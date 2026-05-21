const docx = require("docx");
const fs = require("fs");

const { Document, Packer, Paragraph, TextRun, Table, TableRow, TableCell,
        WidthType, AlignmentType, HeadingLevel, BorderStyle,
        ShadingType, PageBreak, TableOfContents } = docx;

// Helper: create a table cell
function cell(text, opts = {}) {
    const runs = [];
    if (typeof text === "string") {
        runs.push(new TextRun({ text, bold: opts.bold, font: { name: "微软雅黑" }, size: opts.size || 20 }));
    } else {
        runs.push(text);
    }
    return new TableCell({
        children: [new Paragraph({ children: runs, alignment: opts.align || AlignmentType.LEFT })],
        width: opts.width ? { size: opts.width, type: WidthType.PERCENTAGE } : undefined,
        shading: opts.shading ? { fill: opts.shading, type: ShadingType.CLEAR } : undefined,
        verticalAlign: "center",
    });
}

function row(cells, header) {
    return new TableRow({
        children: cells.map((c, i) => {
            const isHeader = header || (i === 0);
            return cell(c, { bold: isHeader, shading: isHeader ? "D9E2F3" : undefined });
        }),
    });
}

function heading(text, level) {
    return new Paragraph({
        children: [new TextRun({ text, bold: true, size: level === 1 ? 32 : level === 2 ? 26 : 22, font: { name: "微软雅黑" } })],
        heading: level === 1 ? HeadingLevel.HEADING_1 : level === 2 ? HeadingLevel.HEADING_2 : HeadingLevel.HEADING_3,
        spacing: { before: level === 1 ? 400 : 240, after: 200 },
    });
}

function para(text, opts = {}) {
    const runs = [];
    if (typeof text === "string") {
        runs.push(new TextRun({ text, font: { name: "微软雅黑" }, size: opts.size || 20, bold: opts.bold }));
    } else if (Array.isArray(text)) {
        text.forEach(t => {
            if (typeof t === "string") runs.push(new TextRun({ text: t, font: { name: "微软雅黑" }, size: opts.size || 20 }));
            else runs.push(new TextRun({ ...t, font: { name: t.font || "微软雅黑" }, size: t.size || opts.size || 20 }));
        });
    }
    return new Paragraph({
        children: runs,
        spacing: { after: opts.after !== undefined ? opts.after : 120 },
        alignment: opts.align || AlignmentType.LEFT,
        indent: opts.indent ? { left: opts.indent } : undefined,
    });
}

function spacer(h) {
    return new Paragraph({ spacing: { after: h || 60 }, children: [] });
}

// ──────────────────────────────────────────────────
// Build document content
// ──────────────────────────────────────────────────

const doc = new Document({
    creator: "OpenCode",
    title: "智能风扇系统 Proteus 仿真 — 接线文档",
    description: "智能风扇 Proteus 仿真完整接线指南",
    styles: {
        default: {
            document: {
                run: { font: "微软雅黑", size: 20 },
                paragraph: { spacing: { after: 120 } },
            },
        },
    },
    sections: [
        // ════════════════ COVER ════════════════
        {
            properties: { page: { margin: { top: 1440, bottom: 1440, left: 1440, right: 1440 } } },
            children: [
                spacer(600),
                new Paragraph({
                    children: [new TextRun({ text: "智能风扇系统", bold: true, size: 52, font: { name: "微软雅黑" } })],
                    alignment: AlignmentType.CENTER,
                    spacing: { after: 120 },
                }),
                new Paragraph({
                    children: [new TextRun({ text: "Proteus 仿真接线文档", bold: true, size: 36, font: { name: "微软雅黑" }, color: "2E75B6" })],
                    alignment: AlignmentType.CENTER,
                    spacing: { after: 400 },
                }),
                new Paragraph({
                    children: [new TextRun({ text: "芯片: AT89C52  @  11.0592MHz", size: 24, font: { name: "微软雅黑" } })],
                    alignment: AlignmentType.CENTER,
                }),
                spacer(60),
                new Paragraph({
                    children: [new TextRun({ text: "开发环境: Keil uVision 5  +  Proteus 8.x", size: 22, font: { name: "微软雅黑" } })],
                    alignment: AlignmentType.CENTER,
                }),
                spacer(60),
                new Paragraph({
                    children: [new TextRun({ text: "固件: main_proteus.c  (Proteus 仿真版 v1.0)", size: 22, font: { name: "微软雅黑" } })],
                    alignment: AlignmentType.CENTER,
                }),
                spacer(200),
                new Paragraph({
                    children: [new TextRun({ text: "—— 基于普中 A2 开发板原理图 ——", size: 20, font: { name: "微软雅黑" }, italics: true, color: "666666" })],
                    alignment: AlignmentType.CENTER,
                }),
                new Paragraph({ children: [], spacing: { after: 100 } }),
            ],
        },

        // ════════════════ PAGE 2: TOC ════════════════
        {
            properties: { page: { margin: { top: 1440, bottom: 1440, left: 1440, right: 1440 } } },
            children: [
                heading("目录", 1),
                para("1. 系统概述"),
                para("2. 元器件清单（BOM）"),
                para("3. AT89C52 最小系统"),
                para("4. 接线总表（按引脚编号）"),
                para("5. LCD1602 接线"),
                para("6. 步进电机接线"),
                para("7. 按键接线"),
                para("8. LED 与蜂鸣器接线"),
                para("9. 仿真运行步骤"),
                para("10. 常见问题排查"),
                { break: new PageBreak() },
            ],
        },

        // ════════════════ 1. 系统概述 ════════════════
        {
            properties: { page: { margin: { top: 1440, bottom: 1440, left: 1440, right: 1440 } } },
            children: [
                heading("1. 系统概述", 1),
                para("本系统使用 AT89C52 单片机控制一个智能风扇的 Proteus 仿真。主要功能包括："),
                para("• 5 档风速调节（1 档微风 ~ 5 档强风），基于步进电机转速控制"),
                para("• 3 种风类模式：常风 / 自然风（忽大忽小）/ 睡眠风（每 10 分钟降一档）"),
                para("• 定时关机：长按 KEY4 设置 30 分钟定时，再次长按取消"),
                para("• LCD1602 实时显示风速档位、风类模式、运行状态、定时时间"),
                para("• 4 个独立按键控制 + LED 状态指示 + 蜂鸣器操作提示音"),
                spacer(80),
                para([
                    { text: "重要提醒：", bold: true, color: "FF0000" },
                    { text: "P2.5 同时连接 LCD_RW 和蜂鸣器，代码中 LCD 写操作时已关总中断(EA=0)保护，接线时无需额外处理。" },
                ]),
                { break: new PageBreak() },
            ],
        },

        // ════════════════ 2. BOM ════════════════
        {
            properties: { page: { margin: { top: 1440, bottom: 1440, left: 1440, right: 1440 } } },
            children: [
                heading("2. 元器件清单（BOM）", 1),
                spacer(),
                new Table({
                    width: { size: 100, type: WidthType.PERCENTAGE },
                    rows: [
                        row(["序号", "Proteus 元件名", "数量", "功能说明"], true),
                        row(["1", "AT89C52", "× 1", "主控芯片（与 STC89C52RC 引脚兼容）"]),
                        row(["2", "LM016L", "× 1", "LCD1602 液晶显示屏"]),
                        row(["3", "MOTOR-STEPPER（4脚型号）", "× 1", "步进电机（28BYJ-48 模型）"]),
                        row(["4", "BUTTON", "× 4", "独立按键（K1~K4）"]),
                        row(["5", "LED-RED / LED-GREEN", "× 2", "指示灯 D1（电源）、D2（模式）"]),
                        row(["6", "BUZZER（ACTIVE）", "× 1", "有源蜂鸣器，操作提示音"]),
                        row(["7", "RES（220Ω）", "× 2", "LED 限流电阻"]),
                        row(["8", "RES（10KΩ）", "× 4", "按键上拉电阻"]),
                        row(["9", "RES（10KΩ）", "× 1", "复位电路上拉"]),
                        row(["10", "CAP（30pF）", "× 2", "晶振负载电容"]),
                        row(["11", "CAP-ELEC（10μF）", "× 1", "复位电路电容"]),
                        row(["12", "CRYSTAL（11.0592MHz）", "× 1", "主晶振"]),
                        row(["13", "POWER（+5V VCC）", "× 1", "正电源终端"]),
                        row(["14", "GROUND", "× 若干", "地线终端"]),
                    ].map(r => r),
                }),
                { break: new PageBreak() },
            ],
        },

        // ════════════════ 3. 最小系统 ════════════════
        {
            properties: { page: { margin: { top: 1440, bottom: 1440, left: 1440, right: 1440 } } },
            children: [
                heading("3. AT89C52 最小系统", 1),
                heading("3.1 晶振电路", 2),
                new Table({
                    width: { size: 100, type: WidthType.PERCENTAGE },
                    rows: [
                        row(["AT89C52 引脚", "连接目标", "说明"], true),
                        row(["XTAL1（pin19）", "11.0592MHz 晶振一端 + 30pF 电容 → GND", "晶振输入"]),
                        row(["XTAL2（pin18）", "11.0592MHz 晶振另一端 + 30pF 电容 → GND", "晶振输出"]),
                    ].map(r => r),
                }),
                spacer(),
                heading("3.2 复位电路", 2),
                new Table({
                    width: { size: 100, type: WidthType.PERCENTAGE },
                    rows: [
                        row(["AT89C52 引脚", "连接目标", "说明"], true),
                        row(["RST（pin9）", "10KΩ → GND  +  10μF → VCC  +  按键 → VCC", "上电自动复位 + 手动复位"]),
                    ].map(r => r),
                }),
                spacer(),
                heading("3.3 电源", 2),
                new Table({
                    width: { size: 100, type: WidthType.PERCENTAGE },
                    rows: [
                        row(["AT89C52 引脚", "连接目标", "说明"], true),
                        row(["VCC（pin40）", "+5V", "主电源"]),
                        row(["GND（pin20）", "GND", "地"]),
                        row(["EA/Vpp（pin31）", "VCC（+5V）", "使能内部程序存储器"]),
                    ].map(r => r),
                }),
                { break: new PageBreak() },
            ],
        },

        // ════════════════ 4. 接线总表 ════════════════
        {
            properties: { page: { margin: { top: 1440, bottom: 1440, left: 1440, right: 1440 } } },
            children: [
                heading("4. 接线总表（按引脚编号）", 1),
                para("以下是 AT89C52 全部 40 个引脚的完整接线表，按引脚号排列："),
                spacer(),
                new Table({
                    width: { size: 100, type: WidthType.PERCENTAGE },
                    rows: [
                        row(["引脚号", "引脚名", "连接目标", "功能"], true),
                        row(["1", "P1.0 (T2)", "步进电机 A 相 或 ULN2003 IN1", "电机相位 A"]),
                        row(["2", "P1.1 (T2EX)", "步进电机 B 相 或 ULN2003 IN2", "电机相位 B"]),
                        row(["3", "P1.2", "步进电机 C 相 或 ULN2003 IN3", "电机相位 C"]),
                        row(["4", "P1.3", "步进电机 D 相 或 ULN2003 IN4", "电机相位 D"]),
                        row(["5", "P1.4", "不接（悬空）", ""]),
                        row(["6", "P1.5", "不接（悬空）", ""]),
                        row(["7", "P1.6", "不接（悬空）", ""]),
                        row(["8", "P1.7", "不接（悬空）", ""]),
                        row(["9", "RST", "10KΩ→GND + 10μF→VCC", "复位"]),
                        row(["10", "P3.0 (RXD)", "KEY2 — 10KΩ→VCC, 按键→GND", "风速 +1"]),
                        row(["11", "P3.1 (TXD)", "KEY1 — 10KΩ→VCC, 按键→GND", "启动/停止"]),
                        row(["12", "P3.2 (INT0)", "KEY3 — 10KΩ→VCC, 按键→GND", "风速 -1"]),
                        row(["13", "P3.3 (INT1)", "KEY4 — 10KΩ→VCC, 按键→GND", "模式/定时"]),
                        row(["14", "P3.4 (T0)", "不接（悬空）", ""]),
                        row(["15", "P3.5 (T1)", "不接（悬空）", ""]),
                        row(["16", "P3.6 (WR)", "不接（悬空）", ""]),
                        row(["17", "P3.7 (RD)", "不接（悬空）", ""]),
                        row(["18", "XTAL2", "11.0592MHz 晶振 + 30pF→GND", "晶振输出"]),
                        row(["19", "XTAL1", "11.0592MHz 晶振 + 30pF→GND", "晶振输入"]),
                        row(["20", "GND", "GND", "地"]),
                        row(["21", "P2.0 (A8)", "D1(LED) — 220Ω→GND", "电源指示灯（低电平亮）"]),
                        row(["22", "P2.1 (A9)", "D2(LED) — 220Ω→GND", "模式指示灯（低电平亮）"]),
                        row(["23", "P2.2 (A10)", "不接（悬空）", ""]),
                        row(["24", "P2.3 (A11)", "不接（悬空）", ""]),
                        row(["25", "P2.4 (A12)", "不接（悬空）", ""]),
                        row(["26", "P2.5 (A13)", "LCD1602 RW + 蜂鸣器 +极", "共用引脚"]),
                        row(["27", "P2.6 (A14)", "LCD1602 RS", "寄存器选择"]),
                        row(["28", "P2.7 (A15)", "LCD1602 E", "使能信号"]),
                        row(["29", "PSEN", "不接（悬空）", ""]),
                        row(["30", "ALE/PROG", "不接（悬空）", ""]),
                        row(["31", "EA/Vpp", "VCC（+5V）", "使能内部 ROM"]),
                        row(["32", "P0.7 (AD7)", "LCD1602 D7", "数据总线 bit7"]),
                        row(["33", "P0.6 (AD6)", "LCD1602 D6", "数据总线 bit6"]),
                        row(["34", "P0.5 (AD5)", "LCD1602 D5", "数据总线 bit5"]),
                        row(["35", "P0.4 (AD4)", "LCD1602 D4", "数据总线 bit4"]),
                        row(["36", "P0.3 (AD3)", "LCD1602 D3", "数据总线 bit3"]),
                        row(["37", "P0.2 (AD2)", "LCD1602 D2", "数据总线 bit2"]),
                        row(["38", "P0.1 (AD1)", "LCD1602 D1", "数据总线 bit1"]),
                        row(["39", "P0.0 (AD0)", "LCD1602 D0", "数据总线 bit0"]),
                        row(["40", "VCC", "+5V", "主电源"]),
                    ].map(r => r),
                }),
                { break: new PageBreak() },
            ],
        },

        // ════════════════ 5. LCD1602 ════════════════
        {
            properties: { page: { margin: { top: 1440, bottom: 1440, left: 1440, right: 1440 } } },
            children: [
                heading("5. LCD1602 接线", 1),
                para("LCD1602 在 Proteus 中选用 LM016L 模型，使用 8 位数据接口。"),
                spacer(),
                new Table({
                    width: { size: 100, type: WidthType.PERCENTAGE },
                    rows: [
                        row(["LCD 引脚", "LCD 功能", "连接目标"], true),
                        row(["1 (VSS)", "电源地", "GND"]),
                        row(["2 (VDD)", "电源正", "+5V"]),
                        row(["3 (VEE)", "对比度调节", "GND（Proteus 仿真直接接地）"]),
                        row(["4 (RS)", "寄存器选择", "AT89C52 P2.6（pin27）"]),
                        row(["5 (RW)", "读写选择", "AT89C52 P2.5（pin26）—— 与蜂鸣器共用"]),
                        row(["6 (E)", "使能信号", "AT89C52 P2.7（pin28）"]),
                        row(["7 (D0)", "数据 bit0", "AT89C52 P0.0（pin39）"]),
                        row(["8 (D1)", "数据 bit1", "AT89C52 P0.1（pin38）"]),
                        row(["9 (D2)", "数据 bit2", "AT89C52 P0.2（pin37）"]),
                        row(["10 (D3)", "数据 bit3", "AT89C52 P0.3（pin36）"]),
                        row(["11 (D4)", "数据 bit4", "AT89C52 P0.4（pin35）"]),
                        row(["12 (D5)", "数据 bit5", "AT89C52 P0.5（pin34）"]),
                        row(["13 (D6)", "数据 bit6", "AT89C52 P0.6（pin33）"]),
                        row(["14 (D7)", "数据 bit7", "AT89C52 P0.7（pin32）"]),
                        row(["15 (BLA)", "背光正极", "+5V（Proteus 可选）"]),
                        row(["16 (BLK)", "背光负极", "GND（Proteus 可选）"]),
                    ].map(r => r),
                }),
                spacer(80),
                para([
                    { text: "⚠ 关键提醒：", bold: true, color: "FF0000" },
                    { text: "P2.5（pin26）同时控制 LCD_RW 和蜂鸣器。代码中 LCD 写操作时通过 EA=0 关闭总中断，防止蜂鸣器 ISR 翻转 P2.5 导致 LCD 误入读模式。此保护由代码完成，接线无额外要求。" },
                ]),
                { break: new PageBreak() },
            ],
        },

        // ════════════════ 6. 步进电机 ════════════════
        {
            properties: { page: { margin: { top: 1440, bottom: 1440, left: 1440, right: 1440 } } },
            children: [
                heading("6. 步进电机接线", 1),
                para("步进电机选用 MOTOR-STEPPER（4 脚型号）。4 个引脚对应电机的 A/B/C/D 四相。在 Proteus 中可直接连接，也可以加 ULN2003A 驱动芯片。"),
                spacer(),
                heading("6.1 方式一：直接驱动（简单仿真）", 2),
                new Table({
                    width: { size: 100, type: WidthType.PERCENTAGE },
                    rows: [
                        row(["AT89C52", "MOTOR-STEPPER", "说明"], true),
                        row(["P1.0（pin1）", "角1（A 相）", "步进电机相位 A"]),
                        row(["P1.1（pin2）", "角2（B 相）", "步进电机相位 B"]),
                        row(["P1.2（pin3）", "角3（C 相）", "步进电机相位 C"]),
                        row(["P1.3（pin4）", "角4（D 相）", "步进电机相位 D"]),
                    ].map(r => r),
                }),
                spacer(),
                heading("6.2 方式二：通过 ULN2003 驱动（推荐，与实物一致）", 2),
                new Table({
                    width: { size: 100, type: WidthType.PERCENTAGE },
                    rows: [
                        row(["AT89C52", "ULN2003A", "MOTOR-STEPPER", "说明"], true),
                        row(["P1.0（pin1）", "IN1（pin1）", "", "ULN2003 输入 1"]),
                        row(["P1.1（pin2）", "IN2（pin2）", "", "ULN2003 输入 2"]),
                        row(["P1.2（pin3）", "IN3（pin3）", "", "ULN2003 输入 3"]),
                        row(["P1.3（pin4）", "IN4（pin4）", "", "ULN2003 输入 4"]),
                        row(["", "OUT1（pin16）", "角1（A 相）", "ULN2003 输出 1 → 电机 A"]),
                        row(["", "OUT2（pin15）", "角2（B 相）", "ULN2003 输出 2 → 电机 B"]),
                        row(["", "OUT3（pin14）", "角3（C 相）", "ULN2003 输出 3 → 电机 C"]),
                        row(["", "OUT4（pin13）", "角4（D 相）", "ULN2003 输出 4 → 电机 D"]),
                        row(["", "COM（pin9）", "", "+5V（续流保护）"]),
                        row(["", "GND（pin8）", "", "GND"]),
                    ].map(r => r),
                }),
                spacer(),
                para("步进电机驱动时序（8 拍半步模式）："),
                para("A → AB → B → BC → C → CD → D → DA  → (循环)"),
                para([
                    { text: "注意：", bold: true },
                    { text: "如果电机转向反了，任意交换相邻两根线（如 A↔B）即可。" },
                ]),
                { break: new PageBreak() },
            ],
        },

        // ════════════════ 7. 按键 ════════════════
        {
            properties: { page: { margin: { top: 1440, bottom: 1440, left: 1440, right: 1440 } } },
            children: [
                heading("7. 按键接线", 1),
                para("每个按键都是独立式按键，低电平有效。Proteus 中直接使用 BUTTON 组件，一端接地，一端接单片机引脚。"),
                spacer(),
                new Table({
                    width: { size: 100, type: WidthType.PERCENTAGE },
                    rows: [
                        row(["按键", "AT89C52", "上拉电阻", "功能"], true),
                        row(["KEY1", "P3.1（pin11）", "10KΩ → VCC", "启动 / 停止风扇"]),
                        row(["KEY2", "P3.0（pin10）", "10KΩ → VCC", "风速 +1 档"]),
                        row(["KEY3", "P3.2（pin12）", "10KΩ → VCC", "风速 -1 档"]),
                        row(["KEY4", "P3.3（pin13）", "10KΩ → VCC", "短按→切换模式 / 长按≥2秒→设置/取消定时"]),
                    ].map(r => r),
                }),
                spacer(),
                para([{ text: "接线说明：", bold: true }, { text: "每个按键的'引脚'端接单片机引脚，'另一端接地'GND。10KΩ 上拉电阻从单片机引脚接到 +5V，确保按键未按下时引脚为高电平。" }]),
                { break: new PageBreak() },
            ],
        },

        // ════════════════ 8. LED & 蜂鸣器 ════════════════
        {
            properties: { page: { margin: { top: 1440, bottom: 1440, left: 1440, right: 1440 } } },
            children: [
                heading("8. LED 与蜂鸣器接线", 1),
                heading("8.1 LED 指示灯", 2),
                para("两个 LED 均为共阳接法（低电平点亮），串联 220Ω 限流电阻。"),
                new Table({
                    width: { size: 100, type: WidthType.PERCENTAGE },
                    rows: [
                        row(["LED", "AT89C52", "串联电阻", "功能"], true),
                        row(["D1（LED-RED）", "P2.0（pin21）", "220Ω → GND", "电源指示灯（亮=风扇运行）"]),
                        row(["D2（LED-GREEN）", "P2.1（pin22）", "220Ω → GND", "模式指示灯（常风常亮/自然风闪烁/睡眠风渐弱）"]),
                    ].map(r => r),
                }),
                spacer(),
                heading("8.2 蜂鸣器", 2),
                new Table({
                    width: { size: 100, type: WidthType.PERCENTAGE },
                    rows: [
                        row(["蜂鸣器引脚", "连接目标", "说明"], true),
                        row(["+极", "AT89C52 P2.5（pin26）", "共用于 LCD_RW，代码中关中断保护"]),
                        row(["-极", "GND", "地"]),
                    ].map(r => r),
                }),
                spacer(),
                para([{ text: "⚠ 注意：", bold: true, color: "FF0000" }, { text: "蜂鸣器 P2.5 与 LCD_RW 共用同一个引脚！这在实物板上也是如此（见普中A2原理图）。代码在每次 LCD 写操作时关中断(EA=0)，写完后恢复(EA=1)。Beep_Tick 函数在蜂鸣器发声后强制将 P2.5 拉低（RW=0 写模式），确保不影响后续 LCD 操作。" }]),
                { break: new PageBreak() },
            ],
        },

        // ════════════════ 9. 仿真运行步骤 ════════════════
        {
            properties: { page: { margin: { top: 1440, bottom: 1440, left: 1440, right: 1440 } } },
            children: [
                heading("9. 仿真运行步骤", 1),
                para("步骤 1：在 Proteus 中放置所有元器件（见第 2 节 BOM 列表）。"),
                para("步骤 2：按照第 4~8 节的接线表完成全部连线。"),
                para('步骤 3：双击 AT89C52，在 Program File 中加载 main_proteus.hex（需先在 Keil 中编译生成）。'),
                para("步骤 4：设置晶振频率为 11.0592MHz（双击 AT89C52 → Clock Frequency → 11.0592MHz）。"),
                para('步骤 5：点击 Proteus 左下角的"运行"按钮（▶）启动仿真。'),
                para('步骤 6：观察 LCD 显示 "Smart Fan v1.0"，1.5 秒后显示初始状态。'),
                para("步骤 7：按 KEY1 启动风扇 → 步进电机开始旋转 → LCD 显示档位和模式。"),
                spacer(),
                para([
                    { text: "功能测试表：", bold: true },
                ]),
                spacer(),
                new Table({
                    width: { size: 100, type: WidthType.PERCENTAGE },
                    rows: [
                        row(["操作", "预期结果"], true),
                        row(["按 KEY1", "风扇启/停切换，电机转/停，D1 亮/灭"]),
                        row(["按 KEY2", "风速 +1 档，电机加速"]),
                        row(["按 KEY3", "风速 -1 档，电机减速"]),
                        row(["短按 KEY4", "模式切换：NORMAL→NATURAL→SLEEP→NORMAL"]),
                        row(["长按 KEY4（≥2秒）", "定时 30 分钟/取消定时（双声提示）"]),
                        row(["风扇关闭", "LCD 显示 FAN STOPPED"]),
                        row(["自然风模式", "D2 闪烁，风速周期性变化"]),
                        row(["睡眠风模式", "D2 渐弱闪烁，每 10 分钟降一档"]),
                    ].map(r => r),
                }),
                { break: new PageBreak() },
            ],
        },

        // ════════════════ 10. 常见问题 ════════════════
        {
            properties: { page: { margin: { top: 1440, bottom: 1440, left: 1440, right: 1440 } } },
            children: [
                heading("10. 常见问题排查", 1),

                heading("电机不转", 2),
                para("• 检查 P1.0~P1.3 到电机 4 个角的接线是否正确。"),
                para("• 检查 KEY1 是否已按下（P3.1 为低电平），风扇已启动。"),
                para("• 检查电机是否选对了 4 脚 MOTOR-STEPPER 型号。"),
                para("• 检查 AT89C52 晶振频率是否设为 11.0592MHz。"),

                heading("LCD 不显示或显示乱码", 2),
                para("• 检查 P0 口到 LCD D0~D7 的 8 根线是否一一对应。"),
                para("• 检查 RS（P2.6）、RW（P2.5）、E（P2.7）接线。"),
                para("• Proteus 中 LCD 的 VEE（pin3）必须接地或接电位器。"),
                para("• 确认 Keil 编译时芯片选的是 AT89C52，晶振 11.0592MHz。"),

                heading("按键不响应", 2),
                para("• 检查每个按键是否一端接 GND、另一端接对应 P3 引脚。"),
                para("• 检查 10KΩ 上拉电阻是否从 P3 引脚接到 +5V。"),
                para("• Proteus 中 BUTTON 的默认属性就是按下接通、松开断开。"),

                heading("蜂鸣器不响", 2),
                para("• 检查 BUZZER 正极接 P2.5，负极接 GND。"),
                para("• 使用 BUZZER（ACTIVE）模型，不要用 BUZZER（DEVICE）。"),

                heading("编译错误/无法生成 HEX", 2),
                para('• Keil 中需勾选 Output -> Create HEX File。'),
                para("• 芯片选择 AT89C52（与 Proteus 一致）。"),
                para("• 晶振频率设为 11.0592MHz。"),

                spacer(200),
                para("—— 文档结束 ——", { align: AlignmentType.CENTER, bold: true }),
            ],
        },
    ],
});

// ══════════════════════════════════════════
// Generate
// ══════════════════════════════════════════
const outPath = "C:\\Users\\17537\\Desktop\\oc\\projects\\smart_fan_51\\智能风扇_Proteus仿真接线文档.docx";
Packer.toBuffer(doc).then(buffer => {
    fs.writeFileSync(outPath, buffer);
    console.log("OK: " + outPath);
});
