import matplotlib.pyplot as plt
import matplotlib.font_manager as fm
import matplotlib.patches as mpatches
from matplotlib.patches import FancyBboxPatch
import os

# Find a Chinese font
font_paths = [
    r'C:\Windows\Fonts\msyh.ttc',       # Microsoft YaHei
    r'C:\Windows\Fonts\simhei.ttf',      # SimHei
    r'C:\Windows\Fonts\msyhbd.ttc',      # Microsoft YaHei Bold
    r'C:\Windows\Fonts\SIMKAI.TTF',      # KaiTi
    r'C:\Windows\Fonts\SURSONG.TTF',     # SimSun
]
chinese_font = None
for fp in font_paths:
    if os.path.exists(fp):
        chinese_font = fm.FontProperties(fname=fp)
        print(f'Using font: {fp}')
        break

if chinese_font is None:
    # Find any available Chinese font
    all_fonts = fm.findSystemFonts()
    for f in all_fonts:
        try:
            fp = fm.FontProperties(fname=f)
            if fp.get_name() and any(ord(c) > 0x4E00 for c in fp.get_name()):
                chinese_font = fp
                print(f'Using font: {f}')
                break
        except:
            pass

if chinese_font is None:
    print('WARNING: No Chinese font found!')

chinese_font_name = chinese_font.get_name() if chinese_font else 'DejaVu Sans'
plt.rcParams['font.family'] = chinese_font_name
plt.rcParams['font.sans-serif'] = [chinese_font_name]

output_dir = r'C:\Users\17537\Desktop\oc\数据库'


def draw_er_diagram(entities, relationships, title, filename):
    fig, ax = plt.subplots(figsize=(16, 11))
    ax.set_xlim(0, 16)
    ax.set_ylim(0, 11)
    ax.axis('off')
    ax.set_title(title, fontsize=16, fontweight='bold', pad=20,
                 fontproperties=chinese_font)

    entity_pos = {e['name']: (e['x'], e['y']) for e in entities}

    # Draw relationship diamonds and lines
    for r in relationships:
        if r['from'] in entity_pos and r['to'] in entity_pos:
            x1, y1 = entity_pos[r['from']]
            x2, y2 = entity_pos[r['to']]
            # Line from entity1 to relationship
            ax.plot([x1, r['x']], [y1, r['y']], 'k-', lw=1.5, zorder=2)
            # Line from relationship to entity2
            ax.plot([r['x'], x2], [r['y'], y2], 'k-', lw=1.5, zorder=2)

    for r in relationships:
        diamond = plt.Polygon(
            [[r['x'], r['y'] + 0.45],
             [r['x'] + 0.6, r['y']],
             [r['x'], r['y'] - 0.45],
             [r['x'] - 0.6, r['y']]],
            facecolor='#FFB6C1', edgecolor='#8B0000', lw=2, zorder=3
        )
        ax.add_patch(diamond)
        ax.text(r['x'], r['y'], r['name'],
                fontsize=10, ha='center', va='center',
                fontweight='bold', color='darkred', zorder=4,
                fontproperties=chinese_font)

    # Draw entity boxes
    for e in entities:
        n_attrs = len(e['attrs'])
        box_h = 1.0 + n_attrs * 0.28
        box_w = 2.6
        x, y = e['x'], e['y']

        rect = FancyBboxPatch(
            (x - box_w / 2, y - box_h / 2), box_w, box_h,
            boxstyle="round,pad=0.1",
            facecolor='#B0E0E6', edgecolor='#2F4F4F', lw=2, zorder=2
        )
        ax.add_patch(rect)

        # Entity name
        ax.text(x, y + box_h / 2 - 0.2, e['name'],
                fontsize=12, ha='center', va='top',
                fontweight='bold', color='#2F4F4F', zorder=4,
                fontproperties=chinese_font)

        # Underline
        ax.plot([x - box_w / 2 + 0.2, x + box_w / 2 - 0.2],
                [y + box_h / 2 - 0.4, y + box_h / 2 - 0.4],
                'k-', lw=1, zorder=4)

        # Attributes
        for i, attr in enumerate(e['attrs']):
            is_pk = attr.startswith('*')
            attr_text = attr[1:] if is_pk else attr
            y_pos = y + box_h / 2 - 0.55 - i * 0.28
            if is_pk:
                ax.text(x, y_pos, attr_text, fontsize=8.5, ha='center', va='center',
                        fontweight='bold', zorder=4, fontproperties=chinese_font)
                ax.plot([x - len(attr_text) * 0.1, x + len(attr_text) * 0.1],
                        [y_pos - 0.06, y_pos - 0.06], 'k-', lw=1, zorder=4)
            else:
                ax.text(x, y_pos, attr_text, fontsize=8.5, ha='center', va='center',
                        zorder=4, fontproperties=chinese_font)

    # Draw cardinality labels near entities
    for r in relationships:
        if r['from'] in entity_pos and r['to'] in entity_pos and r.get('type'):
            x1, y1 = entity_pos[r['from']]
            x2, y2 = entity_pos[r['to']]
            parts = r['type'].split(':')
            if len(parts) == 2:
                card1, card2 = parts[0].strip(), parts[1].strip()
                # Near from entity
                dx1, dy1 = x1 - r['x'], y1 - r['y']
                dist1 = (dx1 ** 2 + dy1 ** 2) ** 0.5
                if dist1 > 0:
                    nx1, ny1 = dx1 / dist1 * 0.8, dy1 / dist1 * 0.8
                    ax.text(r['x'] + nx1, r['y'] + ny1, card1,
                            fontsize=9, ha='center', va='center',
                            bbox=dict(boxstyle='round,pad=0.15', facecolor='white',
                                      edgecolor='gray', alpha=0.9),
                            fontproperties=chinese_font)
                # Near to entity
                dx2, dy2 = x2 - r['x'], y2 - r['y']
                dist2 = (dx2 ** 2 + dy2 ** 2) ** 0.5
                if dist2 > 0:
                    nx2, ny2 = dx2 / dist2 * 0.8, dy2 / dist2 * 0.8
                    ax.text(r['x'] + nx2, r['y'] + ny2, card2,
                            fontsize=9, ha='center', va='center',
                            bbox=dict(boxstyle='round,pad=0.15', facecolor='white',
                                      edgecolor='gray', alpha=0.9),
                            fontproperties=chinese_font)

    plt.tight_layout()
    path = os.path.join(output_dir, filename)
    plt.savefig(path, dpi=150, bbox_inches='tight', facecolor='white')
    plt.close()
    print(f'Saved: {path}')


# =====================================================
# Global E-R Diagram
# =====================================================
entities_global = [
    {'name': '球员', 'attrs': ['*球员ID', '姓名', '国籍', '位置', '出生日期'], 'x': 3, 'y': 8.5},
    {'name': '球队', 'attrs': ['*球队ID', '球队名称', '所在城市', '主场球场'], 'x': 3, 'y': 3.5},
    {'name': '赛季', 'attrs': ['*赛季ID', '年份'], 'x': 13, 'y': 8.5},
    {'name': '比赛', 'attrs': ['*比赛ID', '比赛场次', '比赛日期'], 'x': 13, 'y': 3.5},
    {'name': '球员表现', 'attrs': ['*表现ID', '出场分钟', '进球', '助攻', '射门', '成功传球', '抢断', '评分'], 'x': 8, 'y': 1.2},
]

relationships_global = [
    {'name': '属于', 'x': 3, 'y': 6, 'from': '球员', 'to': '球队', 'type': 'N : 1'},
    {'name': '包含', 'x': 13, 'y': 6, 'from': '赛季', 'to': '比赛', 'type': '1 : N'},
    {'name': '主场', 'x': 6.5, 'y': 3.5, 'from': '球队', 'to': '比赛', 'type': '1 : N'},
    {'name': '客场', 'x': 9.5, 'y': 4.5, 'from': '球队', 'to': '比赛', 'type': '1 : N'},
    {'name': '获得', 'x': 5.5, 'y': 4.5, 'from': '球员', 'to': '球员表现', 'type': '1 : N'},
    {'name': '产生', 'x': 10.5, 'y': 2, 'from': '比赛', 'to': '球员表现', 'type': '1 : N'},
]

draw_er_diagram(entities_global, relationships_global,
                '全局E-R图 — 英超球员2019-2023赛季表现数据管理系统',
                '全局ER图.png')

# =====================================================
# Local E-R Diagram 1: 球员与球队管理
# =====================================================
entities_local1 = [
    {'name': '球员', 'attrs': ['*球员ID', '姓名', '国籍', '位置', '出生日期'], 'x': 5, 'y': 7},
    {'name': '球队', 'attrs': ['*球队ID', '球队名称', '所在城市', '主场球场'], 'x': 11, 'y': 7},
]
relationships_local1 = [
    {'name': '属于', 'x': 8, 'y': 7, 'from': '球员', 'to': '球队', 'type': 'N : 1'},
]
draw_er_diagram(entities_local1, relationships_local1,
                '局部E-R图1 — 球员与球队管理模块',
                '局部ER图1_球员与球队.png')

# =====================================================
# Local E-R Diagram 2: 赛季与比赛管理
# =====================================================
entities_local2 = [
    {'name': '赛季', 'attrs': ['*赛季ID', '年份'], 'x': 4, 'y': 8},
    {'name': '比赛', 'attrs': ['*比赛ID', '比赛场次', '比赛日期'], 'x': 12, 'y': 8},
    {'name': '球队', 'attrs': ['*球队ID', '球队名称', '所在城市', '主场球场'], 'x': 8, 'y': 2.5},
]
relationships_local2 = [
    {'name': '包含', 'x': 8, 'y': 8, 'from': '赛季', 'to': '比赛', 'type': '1 : N'},
    {'name': '主场', 'x': 10, 'y': 5, 'from': '球队', 'to': '比赛', 'type': '1 : N'},
    {'name': '客场', 'x': 6, 'y': 5, 'from': '球队', 'to': '比赛', 'type': '1 : N'},
]
draw_er_diagram(entities_local2, relationships_local2,
                '局部E-R图2 — 赛季与比赛管理模块',
                '局部ER图2_赛季与比赛.png')

# =====================================================
# Local E-R Diagram 3: 比赛表现管理
# =====================================================
entities_local3 = [
    {'name': '球员', 'attrs': ['*球员ID', '姓名', '国籍', '位置', '出生日期'], 'x': 3, 'y': 8},
    {'name': '比赛', 'attrs': ['*比赛ID', '比赛场次', '比赛日期'], 'x': 13, 'y': 8},
    {'name': '球员表现', 'attrs': ['*表现ID', '出场分钟', '进球', '助攻', '射门', '成功传球', '抢断', '评分'], 'x': 8, 'y': 2.5},
]
relationships_local3 = [
    {'name': '获得', 'x': 5, 'y': 5, 'from': '球员', 'to': '球员表现', 'type': '1 : N'},
    {'name': '产生', 'x': 11, 'y': 5, 'from': '比赛', 'to': '球员表现', 'type': '1 : N'},
]
draw_er_diagram(entities_local3, relationships_local3,
                '局部E-R图3 — 比赛表现管理模块',
                '局部ER图3_比赛表现.png')

print('\nAll E-R diagrams generated successfully!')
