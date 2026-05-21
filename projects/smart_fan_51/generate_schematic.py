#!/usr/bin/env python3
"""
智能风扇 Proteus 仿真接线图 v2
严格按 AT89C52 和 LM016L 等元件的 Proteus 物理引脚编号生成
"""

import svgwrite

W, H = 1250, 880
dwg = svgwrite.Drawing('proteus_schematic.svg', size=(W, H), profile='full')

# Colors
CW  = '#1a237e'
CP  = '#37474f'
CI  = '#263238'
CB  = '#eceff1'
CL  = '#1565c0'
CM  = '#e65100'
CR  = '#c62828'
CG  = '#2e7d32'
C5  = '#4a148c'
C0  = '#333333'
C9  = '#00695c'
CPW = '#0d47a1'

def L(x1,y1,x2,y2,c=CW,w=1.5,dash=None):
    a={'stroke':c,'stroke-width':w,'fill':'none'}
    if dash: a['stroke-dasharray']=dash
    return dwg.line(start=(x1,y1),end=(x2,y2),**a)
def D(x,y,r=3,c=CW):
    return dwg.circle(center=(x,y),r=r,fill=c)
def T(x,y,t,sz=14,anc='middle',c=C0,bold=False):
    wgt='bold' if bold else 'normal'
    return dwg.text(t,insert=(x,y),font_size=sz,font_family='Consolas,sans-serif',
                    text_anchor=anc,fill=c,font_weight=wgt)
def Box(x,y,w,h,rx=4,**kw):
    return dwg.rect((x,y),(w,h),rx=rx,**kw)

# ═══════════════════════════════════════════════════════
#   AT89C52 引脚编号（PDIP-40，Proteus 标准）
#   +----+-----------+    +----+-----------+
#   |  1 | P1.0      |    | 40 | VCC      |
#   |  2 | P1.1      |    | 39 | P0.0     |
#   |  3 | P1.2      |    | 38 | P0.1     |
#   |  4 | P1.3      |    | 37 | P0.2     |
#   |  5 | P1.4      |    | 36 | P0.3     |
#   |  6 | P1.5      |    | 35 | P0.4     |
#   |  7 | P1.6      |    | 34 | P0.5     |
#   |  8 | P1.7      |    | 33 | P0.6     |
#   |  9 | RST       |    | 32 | P0.7     |
#   | 10 | P3.0/RXD  |    | 31 | EA/VPP   |
#   | 11 | P3.1/TXD  |    | 30 | ALE/PROG |
#   | 12 | P3.2/INT0 |    | 29 | PSEN     |
#   | 13 | P3.3/INT1 |    | 28 | P2.7/A15 |
#   | 14 | P3.4/T0   |    | 27 | P2.6/A14 |
#   | 15 | P3.5/T1   |    | 26 | P2.5/A13 |
#   | 16 | P3.6/WR   |    | 25 | P2.4/A12 |
#   | 17 | P3.7/RD   |    | 24 | P2.3/A11 |
#   | 18 | XTAL2     |    | 23 | P2.2/A10 |
#   | 19 | XTAL1     |    | 22 | P2.1/A9  |
#   | 20 | GND       |    | 21 | P2.0/A8  |
#   +----+-----------+    +----+-----------+
# ═══════════════════════════════════════════════════════

# IC position & size
IX, IY, IW, IH = 480, 280, 170, 300

# ── Draw IC body ──
dwg.add(Box(IX,IY,IW,IH, rx=6, fill=CB, stroke=CI, stroke_width=3))
dwg.add(T(IX+IW//2, IY+IH//2-12, 'AT89C52', 22, c=CI, bold=True))
dwg.add(T(IX+IW//2, IY+IH//2+14, 'PDIP-40', 12, c='#546e7a'))
dwg.add(T(IX+IW//2, IY+IH//2+32, '11.0592MHz', 11, c='#546e7a'))

def pin_label_text(n, name, x, y, side='left'):
    dwg.add(T(x-6, y+3, str(n), 9, 'end', '#78909c'))
    dwg.add(T(x-8 if side=='left' else x+8, y+3, name, 10,
              'end' if side=='left' else 'start', CP))

# ── LEFT pins (1-8, 9 RST, 10-13, 18-19) ──
# We'll arrange both P1 ports and other left-side pins
LEFT = [
    (1,  'P1.0',     20),
    (2,  'P1.1',     38),
    (3,  'P1.2',     56),
    (4,  'P1.3',     74),
    (5,  'P1.4',     92),
    (6,  'P1.5',     110),
    (7,  'P1.6',     128),
    (8,  'P1.7',     146),
    (9,  'RST',      178),
    (10, 'P3.0/RXD', 208),
    (11, 'P3.1/TXD', 226),
    (12, 'P3.2/INT0',244),
    (13, 'P3.3/INT1',262),
    # skip 14-17 (right side)
    (18, 'XTAL2',    IY+IH-90),
    (19, 'XTAL1',    IY+IH-72),
]

for pin_no, name, py in LEFT:
    dwg.add(L(IX-25, py, IX, py))
    dwg.add(D(IX, py))
    pin_label_text(pin_no, name, IX-25, py, 'left')

# ── RIGHT pins (21-28, 29 PSEN, 30 ALE, 31 EA, 32-39 P0, 40 VCC) ──
RIGHT = [
    (40, 'VCC',      20),
    (39, 'P0.0/AD0', 44),
    (38, 'P0.1/AD1', 62),
    (37, 'P0.2/AD2', 80),
    (36, 'P0.3/AD3', 98),
    (35, 'P0.4/AD4', 116),
    (34, 'P0.5/AD5', 134),
    (33, 'P0.6/AD6', 152),
    (32, 'P0.7/AD7', 170),
    (31, 'EA/VPP',   198),
    (30, 'ALE/PROG', 220),
    (29, 'PSEN',     238),
    (28, 'P2.7/A15', IY+IH-108),
    (27, 'P2.6/A14', IY+IH-90),
    (26, 'P2.5/A13', IY+IH-72),
    (25, 'P2.4/A12', IY+IH-54),
    (24, 'P2.3/A11', IY+IH-36),
    (23, 'P2.2/A10', IY+IH-18),
]

for pin_no, name, py in RIGHT:
    px = IX+IW
    dwg.add(L(px, py, px+25, py))
    dwg.add(D(px, py))
    pin_label_text(pin_no, name, px+25, py, 'right')

# ── Extra right side: P2.1(p22), P2.0(p21) above IC ──
TOP_R = [
    (22, 'P2.1/A9',  IX+IW-40),
    (21, 'P2.0/A8',  IX+IW-90),
]
for pin_no, name, px in TOP_R:
    py = IY
    dwg.add(L(px, py-18, px, py))
    dwg.add(D(px, py))
    dwg.add(T(px, py-22, f'{pin_no}: {name}', 9, 'middle', CP))

# ── BOTTOM pins (20 GND, 14-17) ──
BOTTOM = [
    (20, 'GND',      IX+30),
    (17, 'P3.7/RD',  IX+70),
    (16, 'P3.6/WR',  IX+100),
    (15, 'P3.5/T1',  IX+130),
    (14, 'P3.4/T0',  IX+160),
]
for pin_no, name, px in BOTTOM:
    py = IY+IH
    dwg.add(L(px, py, px, py+18))
    dwg.add(D(px, py))
    dwg.add(T(px, py+22, f'{pin_no}: {name}', 9, 'middle', CP))

# ═══════════════════════ 1. Crystal circuit (pins 18,19) ═══════════════════════
XTAL1_Y = IY+IH-72  # pin 19
XTAL2_Y = IY+IH-90  # pin 18
CX = 250

dwg.add(L(IX-25, XTAL1_Y, CX, XTAL1_Y))
dwg.add(L(IX-25, XTAL2_Y, CX, XTAL2_Y))
dwg.add(Box(CX-22, XTAL1_Y-4, 44, 34, rx=3, fill='white', stroke=CW, stroke_width=1.5))
dwg.add(T(CX, XTAL1_Y+10, '11.0592MHz', 10, c=CW))
dwg.add(T(CX, XTAL1_Y+22, 'X1 (CRYSTAL)', 8, c=CW))

# C1, C2 30pF
for yy, cap_label in [(XTAL1_Y, 'C1'), (XTAL2_Y, 'C2')]:
    cy = yy + 32
    dwg.add(L(CX, yy+30, CX, cy+10))
    dwg.add(L(CX-12, cy+2, CX+12, cy+2, w=2.5))
    dwg.add(T(CX-6, cy+6, cap_label, 9, 'end', CPW))
    dwg.add(L(CX, cy+2, CX, cy+18))
    gnd_y = cy + 38
    dwg.add(L(CX-40, gnd_y, CX+40, gnd_y, c=C5))
    dwg.add(L(CX, cy+18, CX, gnd_y, c=C5))
    dwg.add(T(CX, gnd_y+12, 'GND', 9, c=C5))

# ═══════════════════════ 2. Reset circuit (pin 9) ═══════════════════════
RST_PY = IY+178  # pin 9
RST_X = IX-25
RST_LX = 220

dwg.add(L(RST_X, RST_PY, RST_LX, RST_PY))

# 10K pull-down to GND (Proteus typical: RST pin 9 -> 10K to GND)
RG_Y = RST_PY + 60
dwg.add(L(RST_LX, RST_PY, RST_LX, RST_PY+15))
dwg.add(L(RST_LX, RST_PY+15, RST_LX, RG_Y))
dwg.add(Box(RST_LX-6, RST_PY+18, 12, 28, rx=2, fill='white', stroke='#4e342e', stroke_width=1.5))
dwg.add(T(RST_LX-12, RST_PY+36, 'R1=10K', 9, 'end', '#4e342e'))
dwg.add(L(RST_LX, RG_Y, RST_LX, RG_Y+15, c=C5))
dwg.add(T(RST_LX, RG_Y+22, 'GND', 9, c=C5))

# 10uF to VCC (RST -> 10uF+ -> 10uF- -> VCC)
RC_X = RST_LX + 90
dwg.add(L(RST_LX, RST_PY+10, RC_X, RST_PY+10))
dwg.add(L(RC_X, RST_PY+10, RC_X, RST_PY-25))
dwg.add(L(RC_X-10, RST_PY-25, RC_X+10, RST_PY-25, w=2.5))
dwg.add(L(RC_X, RST_PY-25, RC_X, RST_PY-40))
dwg.add(T(RC_X+14, RST_PY-22, '+ C2=10uF', 9, 'start', CPW))
dwg.add(L(RC_X, RST_PY-40, RC_X, RST_PY-55, c=C5))
dwg.add(T(RC_X, RST_PY-59, '+5V', 9, c=C5))

# manual reset button (RST -> button -> VCC)
RST_BTN = RC_X
dwg.add(L(RST_LX, RST_PY, RST_BTN, RST_PY))
dwg.add(L(RST_BTN, RST_PY, RST_BTN, RST_PY-55, c=C5))
dwg.add(Box(RST_BTN-10, RST_PY-66, 20, 10, rx=2, fill='white', stroke=CG, stroke_width=2))
dwg.add(T(RST_BTN, RST_PY-56, 'RST', 8, c=CG))

# ═══════════════════════ 3. EA=VCC (pin 31) ═══════════════════════
EA_PY = IY+198  # pin 31
EA_VCC_Y = 90
dwg.add(L(IX+IW+25, EA_PY, IX+150, EA_PY, c=C5))
dwg.add(L(IX+150, EA_PY, IX+150, EA_VCC_Y, c=C5))

# VCC rail for pin 40
VCC_PY = IY+20  # pin 40
dwg.add(L(IX+IW+25, VCC_PY, IX+IW-40, VCC_PY, c=C5))
dwg.add(L(IX+IW-40, VCC_PY, IX+IW-40, EA_VCC_Y, c=C5))
dwg.add(L(IX+IW-40, EA_VCC_Y, IX+150, EA_VCC_Y, c=C5, w=3))
dwg.add(T(IX+150, EA_VCC_Y-8, '+5V 电源总线', 10, c=C5))

# GND rail
GND_PY = IY+IH+18  # pin 20
dwg.add(L(IX+30, GND_PY, IX+30, EA_VCC_Y+55, c=C5))
dwg.add(L(IX+30, EA_VCC_Y+55, IX+150, EA_VCC_Y+55, c=C5, w=3))
dwg.add(T(IX+150, EA_VCC_Y+55-8, 'GND 地线总线', 10, c=C5))

# tie pin 20 to GND
dwg.add(L(IX+30, IY+IH, IX+30, EA_VCC_Y+55, c=C5))

# ═══════════════════════ 4. Buttons (KEY1-KEY4) ═══════════════════════
# KEY1 = P3.1 (pin 11), KEY2 = P3.0 (pin 10), KEY3 = P3.2 (pin 12), KEY4 = P3.3 (pin 13)
BTN_MAP = [
    (11, 'P3.1/TXD', 'KEY1',  226,  '启停'),
    (10, 'P3.0/RXD', 'KEY2',  208,  '风速+'),
    (12, 'P3.2/INT0','KEY3',  244,  '风速-'),
    (13, 'P3.3/INT1','KEY4',  262,  '模式/定时'),
]
BTN_X = 75

for pin_no, pname, kname, py, desc in BTN_MAP:
    bx = BTN_X
    dwg.add(L(IX-25, py, bx, py))

    # 10K pull-up to +5V
    pu_y = py - 48
    dwg.add(L(bx, py, bx, pu_y))
    dwg.add(L(bx, pu_y, bx+28, pu_y, c=C5))
    dwg.add(L(bx+28, pu_y, bx+28, pu_y-15, c=C5))
    dwg.add(Box(bx-4, pu_y-10, 8, 20, rx=1, fill='white', stroke='#4e342e', stroke_width=1.5))
    dwg.add(T(bx+14, pu_y-20, '10K', 8, c='#4e342e'))
    dwg.add(T(bx+28, pu_y-19, '+5V', 7, c=C5))

    # button to GND
    bb = py + 40
    dwg.add(L(bx, py+2, bx, bb))
    dwg.add(Box(bx-8, py+5, 16, 10, rx=2, fill='white', stroke=CG, stroke_width=2))
    dwg.add(T(bx, py+19, kname, 9, c=CG))
    dwg.add(T(bx, py+28, f'({desc})', 7, c='#78909c'))
    dwg.add(L(bx, bb, bx, bb+12, c=C5))
    dwg.add(T(bx, bb+20, 'GND', 7, c=C5))
    dwg.add(T(bx-14, py+3, f'pin{pin_no}', 7, 'end', '#78909c'))

# ═══════════════════════ 5. LCD1602 (LM016L) ═══════════════════════
# LM016L Proteus 引脚编号:
#  1=VSS(GND)  2=VDD(+5V)  3=VEE(对比度,GND)
#  4=RS  5=RW  6=E
#  7=D0  8=D1  9=D2  10=D3  11=D4  12=D5  13=D6  14=D7
LX, LY, LW, LH = 860, 170, 220, 180

dwg.add(Box(LX,LY,LW,LH, rx=4, fill='white', stroke=CL, stroke_width=2.5))
dwg.add(T(LX+LW//2, LY+18, 'LM016L', 14, c=CL, bold=True))
dwg.add(T(LX+LW//2, LY+33, '(LCD1602)', 10, c=CL))
dwg.add(Box(LX+30,LY+45,155,100, rx=2, fill='#e3f2fd', stroke='#90caf9', stroke_width=1))

# LCD pin labels (left edge, going down)
LCD_PINS = [
    (50,  '1:VSS',  '#4e342e'), (65,  '2:VDD',  C5),
    (80,  '3:VEE',  '#4e342e'), (95,  '4:RS',   CL),
    (110, '5:RW',   CL),        (125, '6:E',    CL),
    (140, '7:D0',   CL),        (150, '8:D1',   CL),
    (160, '9:D2',   CL),        (170, '10:D3',  CL),
    (180, '11:D4',  CL),        (190, '12:D5',  CL),
    (200, '13:D6',  CL),        (210, '14:D7',  CL),
]
for py, lbl, col in LCD_PINS:
    dwg.add(L(LX, py, LX-20, py))
    dwg.add(T(LX-22, py+3, lbl, 9, 'end', col))

# Connect power
dwg.add(L(LX-20, 50,  LX-20, EA_VCC_Y+55, c=C5))  # VSS -> GND
dwg.add(L(LX-20, 65,  LX-20+25, 65)); dwg.add(L(LX-20+25, 65, IX+150, EA_VCC_Y, c=C5))  # VDD -> +5V
dwg.add(T(LX-20+12, 68, 'VDD=+5V', 7, c=C5))

# VEE (pin 3) to GND via potentiometer
dwg.add(L(LX-20, 80, LX-20, EA_VCC_Y+55, c=C5))
dwg.add(T(LX-20+10, 80+8, 'VSS,VEE -> GND', 7, c='#4e342e'))

# ── LCD control/data connections from AT89C52 ──
# P2.6 (pin 27) -> RS (pin 4)
P26_Y = IY+IH-90  # pin 27 P2.6
dwg.add(L(IX+IW+25, P26_Y, LX-22, P26_Y))
dwg.add(L(LX-22, P26_Y, LX-22, 95))
dwg.add(L(LX-22, 95, LX, 95))
dwg.add(T(IX+IW+28, P26_Y+3, 'pin27:P2.6->RS', 8, 'start', CL))

# P2.5 (pin 26) -> RW (pin 5)
P25_Y = IY+IH-72  # pin 26 P2.5
dwg.add(L(IX+IW+25, P25_Y, LX-22, P25_Y))
dwg.add(L(LX-22, P25_Y, LX-22, 110))
dwg.add(L(LX-22, 110, LX, 110))
dwg.add(T(IX+IW+28, P25_Y+3, 'pin26:P2.5->RW', 8, 'start', CL))

# P2.7 (pin 28) -> E (pin 6)
P27_Y = IY+IH-108  # pin 28 P2.7
dwg.add(L(IX+IW+25, P27_Y, LX-22, P27_Y))
dwg.add(L(LX-22, P27_Y, LX-22, 125))
dwg.add(L(LX-22, 125, LX, 125))
dwg.add(T(IX+IW+28, P27_Y+3, 'pin28:P2.7->E', 8, 'start', CL))

# P0.0-P0.7 (pins 39-32) -> D0-D7 (pins 7-14)
P0_Ys = [20, 44, 62, 80, 98, 116, 134, 152]  # pin 40 VCC, 39 P0.0, ...
# Actually let's map: P0.0=pin39, P0.1=pin38, ..., P0.7=pin32
P0_MAP = [(39,44),(38,62),(37,80),(36,98),(35,116),(34,134),(33,152),(32,170)]
LCD_DYs = [140,150,160,170,180,190,200,210]  # D0-D7 Y positions

for (pin_no, ic_y), lcd_y in zip(P0_MAP, LCD_DYs):
    dwg.add(L(IX+IW+25, ic_y, LX-22, ic_y, dash='4,3'))
    dwg.add(L(LX-22, ic_y, LX-22, lcd_y))
    dwg.add(L(LX-22, lcd_y, LX, lcd_y))

# data bus label
dwg.add(T((IX+IW+LX)//2, P0_MAP[0][1]-7, 'P0.0~P0.7 (pin39→32) -> D0~D7 (pin7→14)', 9, c='#78909c'))

# ═══════════════════════ 6. Stepper motor ═══════════════════════
# MOTOR-STEPPER in Proteus:
#   Pin 1: A (coil 1)   Pin 2: B (coil 2)
#   Pin 3: C (coil 3)   Pin 4: D (coil 4)
#   Pin 5: COM (+5V)
MX, MY, MW, MH = 790, 600, 140, 100

dwg.add(Box(MX,MY,MW,MH, rx=8, fill='#fff3e0', stroke=CM, stroke_width=2.5))
dwg.add(T(MX+MW//2, MY+20, 'MOTOR-STEPPER', 13, c=CM, bold=True))
dwg.add(T(MX+MW//2, MY+38, '(28BYJ-48)', 11, c=CM))
dwg.add(T(MX+MW//2, MY+60, 'P1.0→A  P1.1→B', 9, c=CM))
dwg.add(T(MX+MW//2, MY+76, 'P1.2→C  P1.3→D', 9, c=CM))
dwg.add(T(MX+MW//2, MY+92, 'COM→+5V', 9, c=C5))

# Motor connection from P1.0-P1.3 (pins 1-4)
P1_MAP = [(1,20,'P1.0','A'),(2,38,'P1.1','B'),
          (3,56,'P1.2','C'),(4,74,'P1.3','D')]
MID = 130
MOTOR_Ys = [MY+10, MY+28, MY+46, MY+64]

for (pin_no, ic_y, pname, mname), my in zip(P1_MAP, MOTOR_Ys):
    dwg.add(L(IX-25, ic_y, MID, ic_y))
    dwg.add(L(MID, ic_y, MID, my+100))
    dwg.add(L(MID, my+100, MX-15, my+100))
    dwg.add(L(MX-15, my+100, MX-15, my))
    dwg.add(L(MX-15, my, MX, my))

dwg.add(T(MID-10, P1_MAP[0][1]+4, f'pin1-4: P1.0~P1.3', 9, 'end', CM))
dwg.add(T(MID+10, P1_MAP[0][1]+14, '→ A,B,C,D', 9, 'start', CM))

# COM -> +5V
dwg.add(L(MX-15, MY+80, MID, MY+80, c=C5))
dwg.add(L(MID, MY+80, MID, EA_VCC_Y, c=C5))
dwg.add(T(MID-10, MY+80+8, 'pin5:COM=+5V', 9, 'end', C5))

# ULN2003 suggestion
dwg.add(Box(50, 620, 180, 30, rx=4, fill='white', stroke=CM, stroke_width=1.5, stroke_dasharray='4,3'))
dwg.add(T(140, 640, '建议通过 ULN2003 驱动电机', 10, c=CM))

# ═══════════════════════ 7. LEDs (D1, D2) ═══════════════════════
# P2.1 (pin 22) -> D1 (power indicator)
# P2.0 (pin 21) -> D2 (mode indicator)
LED_BASE_X = 280
LEDS = [
    (22, IX+IW-40, IY, 'D1', 680, '电源指示', True),
    (21, IX+IW-90, IY, 'D2', 735, '模式指示', False),
]

# Actually P2.0/A8 is pin 21, P2.1/A9 is pin 22
# They're at TOP_R: pin21=IX+IW-90, pin22=IX+IW-40
LEDS = [
    (21, IX+IW-90, 'D1', 680, '电源指示'),
    (22, IX+IW-40, 'D2', 735, '模式指示'),
]

for pin_no, px, dn, led_y, desc in LEDS:
    py = IY
    # drop down from top
    dwg.add(L(px, py-18, px, led_y))
    dwg.add(L(px, led_y, LED_BASE_X, led_y))

    # 220R
    rx_pos = LED_BASE_X - 35
    dwg.add(L(rx_pos-5, led_y, rx_pos-15, led_y))
    dwg.add(Box(rx_pos-8, led_y-8, 16, 16, rx=2, fill='white', stroke='#4e342e', stroke_width=1.5))
    dwg.add(T(rx_pos-6, led_y-12, '220Ω', 8, 'end', '#4e342e'))

    # LED symbol
    lsx = LED_BASE_X + 18
    dwg.add(L(lsx-8, led_y, lsx-2, led_y))
    dwg.add(dwg.polygon([(lsx,led_y-8),(lsx+6,led_y),(lsx,led_y+8)], fill='white', stroke=CR, stroke_width=1.5))
    dwg.add(L(lsx-2, led_y-4, lsx-2, led_y+4, c=CR, w=1.5))
    dwg.add(T(lsx+14, led_y+4, dn, 10, 'start', CR))
    dwg.add(T(lsx+14, led_y-8, desc, 8, 'start', '#78909c'))
    dwg.add(T(px, py-22, f'pin{pin_no}:{dn}', 8, 'middle', CR))

    # GND
    dwg.add(L(lsx+6, led_y, lsx+6, EA_VCC_Y+55, c=C5))
    dwg.add(T(lsx+6, (led_y+EA_VCC_Y+55)//2-4, 'GND', 8, c=C5))

# ═══════════════════════ 8. Buzzer (pin 26 P2.5) ═══════════════════════
# P2.5 (pin 26) -> buzzer +, buzzer - to GND
BX, BY = 600, 710
buz_py = IY+IH-72  # pin 26 P2.5

# Branch off the P2.5 line between IC and LCD
dwg.add(L(IX+70, buz_py, IX+70, BY))
dwg.add(L(IX+70, BY, BX, BY))
dwg.add(Box(BX-18, BY-10, 30, 20, rx=3, fill='#e0f2f1', stroke=C9, stroke_width=2))
dwg.add(T(BX-8, BY+4, 'BUZZER', 8, 'start', C9))
dwg.add(L(BX+12, BY, BX+12, EA_VCC_Y+55, c=C5))
dwg.add(T(BX+12, (BY+EA_VCC_Y+55)//2-4, 'GND', 8, c=C5))
dwg.add(T(IX+70-10, buz_py+3, 'pin26:P2.5', 8, 'end', C9))
dwg.add(T(IX+70-10, buz_py+14, '(LCD_RW共用!)', 7, 'end', '#78909c'))

# ═══════════════════════ 9. Title & Legend ═══════════════════════
dwg.add(T(W//2, 30, '智能风扇系统 — Proteus 仿真接线图 (物理引脚版)', 20, c=CI, bold=True))
dwg.add(T(W//2, 48, '芯片: AT89C52  PDIP-40  @ 11.0592MHz  |  仿真版: main_proteus.c', 12, c='#546e7a'))

# Legend
LYD = 840
for i,(c,t) in enumerate([
    (CW,'信号线'),(C5,'电源/地'),(CG,'按键'),
    (CR,'LED'),(C9,'蜂鸣器'),(CM,'步进电机'),(CL,'LCD')
]):
    x = 30 + i*130
    dwg.add(L(x, LYD, x+20, LYD, w=3, c=c))
    dwg.add(T(x+26, LYD+4, t, 10, 'start', c))

dwg.add(T(W-30, H-20, '注意: 所有引脚编号为 Proteus 中元件的物理引脚号', 9, 'end', '#b0bec5'))

dwg.save()
print("OK: proteus_schematic.svg generated (with physical pin numbers)")
