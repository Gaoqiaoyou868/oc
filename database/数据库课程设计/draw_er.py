import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import FancyBboxPatch
import os

output_dir = r'C:\Users\17537\Desktop\oc\数据库'

# =====================================================
# Helper: draw a clean E-R diagram
# =====================================================
def draw_er_diagram(entities, relationships, title, filename):
    """
    entities: list of dicts { 'name': str, 'attrs': [str], 'x': float, 'y': float }
    relationships: list of dicts { 'name': str, 'x': float, 'y': float, 'from': str, 'to': str, 'type': str }
    """
    fig, ax = plt.subplots(figsize=(14, 10))
    ax.set_xlim(0, 14)
    ax.set_ylim(0, 10)
    ax.axis('off')
    ax.set_title(title, fontsize=16, fontweight='bold', pad=20)

    entity_pos = {e['name']: (e['x'], e['y']) for e in entities}

    # Draw relationships first (lines)
    for r in relationships:
        if r['from'] in entity_pos and r['to'] in entity_pos:
            x1, y1 = entity_pos[r['from']]
            x2, y2 = entity_pos[r['to']]
            ax.plot([x1, r['x']], [y1, r['y']], 'k-', lw=1.5)
            ax.plot([r['x'], x2], [r['y'], y2], 'k-', lw=1.5)
            # Draw relationship type label
            mx, my = (x1 + x2) / 2, (y1 + y2) / 2
            # Offset the label
            if r['type']:
                ax.text(mx, my - 0.3, r['type'], fontsize=9, ha='center', va='top',
                        bbox=dict(boxstyle='round,pad=0.2', facecolor='lightyellow', edgecolor='none'))

    # Draw relationship diamonds
    for r in relationships:
        d = mpatches.FancyBboxPatch(
            (r['x'] - 0.5, r['y'] - 0.25), 1.0, 0.5,
            boxstyle="darrow",  # diamond-like
            facecolor='lightcoral', edgecolor='darkred', lw=2
        )
        ax.add_patch(d)
        # Use a rotated rectangle to look like diamond
        diamond = plt.Polygon(
            [[r['x'], r['y'] + 0.35],
             [r['x'] + 0.5, r['y']],
             [r['x'], r['y'] - 0.35],
             [r['x'] - 0.5, r['y']]],
            facecolor='#FFB6C1', edgecolor='#8B0000', lw=2, zorder=3
        )
        ax.add_patch(diamond)
        ax.text(r['x'], r['y'], r['name'], fontsize=9, ha='center', va='center',
                fontweight='bold', color='darkred', zorder=4)

    # Draw entity rectangles
    for e in entities:
        # Calculate box size based on number of attributes
        n_attrs = len(e['attrs'])
        box_h = 1.0 + n_attrs * 0.3
        box_w = 2.4
        x, y = e['x'], e['y']

        # Entity box
        rect = FancyBboxPatch(
            (x - box_w / 2, y - box_h / 2), box_w, box_h,
            boxstyle="round,pad=0.1",
            facecolor='#B0E0E6', edgecolor='#2F4F4F', lw=2, zorder=2
        )
        ax.add_patch(rect)

        # Entity name (top of box, bold)
        ax.text(x, y + box_h / 2 - 0.25, e['name'],
                fontsize=11, ha='center', va='top', fontweight='bold', color='#2F4F4F', zorder=4)

        # Underline
        ax.plot([x - box_w / 2 + 0.2, x + box_w / 2 - 0.2],
                [y + box_h / 2 - 0.4, y + box_h / 2 - 0.4],
                'k-', lw=1, zorder=4)

        # Attributes
        for i, attr in enumerate(e['attrs']):
            is_pk = attr.startswith('*')
            attr_text = attr[1:] if is_pk else attr
            y_pos = y + box_h / 2 - 0.6 - i * 0.3
            # Underline PK
            if is_pk:
                ax.text(x, y_pos, attr_text, fontsize=8, ha='center', va='center',
                        fontweight='bold', zorder=4)
                ax.plot([x - len(attr_text) * 0.12, x + len(attr_text) * 0.12],
                        [y_pos - 0.08, y_pos - 0.08], 'k-', lw=1, zorder=4)
            else:
                ax.text(x, y_pos, attr_text, fontsize=8, ha='center', va='center', zorder=4)

            # Draw line from attribute to entity
            ax.plot([x, x], [y + box_h / 2 - 0.45, y + box_h / 2 - 0.55],
                    'w', lw=0)  # invisible

    # Relationship lines between entities
    for r in relationships:
        if r['from'] in entity_pos and r['to'] in entity_pos:
            x1, y1 = entity_pos[r['from']]
            x2, y2 = entity_pos[r['to']]
            # Redraw on top
            ax.plot([x1, r['x']], [y1, r['y']], 'k-', lw=1.5, zorder=2)
            ax.plot([r['x'], x2], [r['y'], y2], 'k-', lw=1.5, zorder=2)

            # Cardinality labels
            if r['type']:
                parts = r['type'].split(':')
                if len(parts) == 2:
                    card1, card2 = parts
                    # Position near each entity
                    dx1, dy1 = x1 - r['x'], y1 - r['y']
                    dist1 = (dx1 ** 2 + dy1 ** 2) ** 0.5
                    if dist1 > 0:
                        nx1, ny1 = dx1 / dist1, dy1 / dist1
                        ax.text(r['x'] + nx1 * 0.8, r['y'] + ny1 * 0.8,
                                card1.strip(), fontsize=9, ha='center', va='center',
                                bbox=dict(boxstyle='round,pad=0.1', facecolor='white', edgecolor='none', alpha=0.8))

                    dx2, dy2 = x2 - r['x'], y2 - r['y']
                    dist2 = (dx2 ** 2 + dy2 ** 2) ** 0.5
                    if dist2 > 0:
                        nx2, ny2 = dx2 / dist2, dy2 / dist2
                        ax.text(r['x'] + nx2 * 0.8, r['y'] + ny2 * 0.8,
                                card2.strip(), fontsize=9, ha='center', va='center',
                                bbox=dict(boxstyle='round,pad=0.1', facecolor='white', edgecolor='none', alpha=0.8))

    plt.tight_layout()
    path = os.path.join(output_dir, filename)
    plt.savefig(path, dpi=150, bbox_inches='tight', facecolor='white')
    plt.close()
    print(f'Saved: {path}')


# =====================================================
# Global E-R Diagram
# =====================================================
entities_global = [
    {
        'name': '球员',
        'attrs': ['*球员ID', '姓名', '国籍', '位置', '出生日期'],
        'x': 3, 'y': 7
    },
    {
        'name': '球队',
        'attrs': ['*球队ID', '球队名称', '所在城市', '主场球场'],
        'x': 3, 'y': 3
    },
    {
        'name': '赛季',
        'attrs': ['*赛季ID', '年份'],
        'x': 11, 'y': 7
    },
    {
        'name': '比赛',
        'attrs': ['*比赛ID', '比赛场次', '比赛日期', '主队ID', '客队ID'],
        'x': 11, 'y': 3
    },
    {
        'name': '球员表现',
        'attrs': ['*表现ID', '出场分钟', '进球', '助攻', '射门', '成功传球', '抢断', '评分'],
        'x': 7, 'y': 1
    },
]

relationships_global = [
    {'name': '属于', 'x': 3, 'y': 5, 'from': '球员', 'to': '球队', 'type': 'N:1'},
    {'name': '包含', 'x': 11, 'y': 5, 'from': '赛季', 'to': '比赛', 'type': '1 : N'},
    {'name': '主场', 'x': 6, 'y': 3.5, 'from': '球队', 'to': '比赛', 'type': '1 : N'},
    {'name': '客场', 'x': 8, 'y': 2.5, 'from': '球队', 'to': '比赛', 'type': '1 : N'},
    {'name': '获得', 'x': 5, 'y': 3.5, 'from': '球员', 'to': '球员表现', 'type': '1 : N'},
    {'name': '产生', 'x': 9, 'y': 1.5, 'from': '比赛', 'to': '球员表现', 'type': '1 : N'},
]

draw_er_diagram(entities_global, relationships_global,
                '全局E-R图 - 英超球员2019-2023赛季表现数据管理系统',
                '全局ER图.png')

# =====================================================
# Local E-R Diagram 1: 球员与球队管理
# =====================================================
entities_local1 = [
    {
        'name': '球员',
        'attrs': ['*球员ID', '姓名', '国籍', '位置', '出生日期'],
        'x': 4, 'y': 6
    },
    {
        'name': '球队',
        'attrs': ['*球队ID', '球队名称', '所在城市', '主场球场'],
        'x': 9, 'y': 6
    },
]

relationships_local1 = [
    {'name': '属于', 'x': 6.5, 'y': 6, 'from': '球员', 'to': '球队', 'type': 'N:1'},
]

draw_er_diagram(entities_local1, relationships_local1,
                '局部E-R图1 - 球员与球队管理模块',
                '局部ER图1_球员与球队.png')

# =====================================================
# Local E-R Diagram 2: 赛季与比赛管理
# =====================================================
entities_local2 = [
    {
        'name': '赛季',
        'attrs': ['*赛季ID', '年份'],
        'x': 3, 'y': 6
    },
    {
        'name': '比赛',
        'attrs': ['*比赛ID', '比赛场次', '比赛日期', '主队ID', '客队ID'],
        'x': 10, 'y': 6
    },
    {
        'name': '球队',
        'attrs': ['*球队ID', '球队名称', '所在城市', '主场球场'],
        'x': 10, 'y': 2
    },
]

relationships_local2 = [
    {'name': '包含', 'x': 6.5, 'y': 6, 'from': '赛季', 'to': '比赛', 'type': '1 : N'},
    {'name': '主场', 'x': 9, 'y': 4, 'from': '球队', 'to': '比赛', 'type': '1 : N'},
    {'name': '客场', 'x': 11.5, 'y': 4, 'from': '球队', 'to': '比赛', 'type': '1 : N'},
]

draw_er_diagram(entities_local2, relationships_local2,
                '局部E-R图2 - 赛季与比赛管理模块',
                '局部ER图2_赛季与比赛.png')

# =====================================================
# Local E-R Diagram 3: 比赛表现管理
# =====================================================
entities_local3 = [
    {
        'name': '球员',
        'attrs': ['*球员ID', '姓名', '国籍', '位置', '出生日期'],
        'x': 3, 'y': 7
    },
    {
        'name': '比赛',
        'attrs': ['*比赛ID', '比赛场次', '比赛日期', '主队ID', '客队ID'],
        'x': 10, 'y': 7
    },
    {
        'name': '球员表现',
        'attrs': ['*表现ID', '出场分钟', '进球', '助攻', '射门', '成功传球', '抢断', '评分'],
        'x': 6.5, 'y': 3
    },
]

relationships_local3 = [
    {'name': '获得', 'x': 4.5, 'y': 5, 'from': '球员', 'to': '球员表现', 'type': '1 : N'},
    {'name': '产生', 'x': 8.5, 'y': 5, 'from': '比赛', 'to': '球员表现', 'type': '1 : N'},
]

draw_er_diagram(entities_local3, relationships_local3,
                '局部E-R图3 - 比赛表现管理模块',
                '局部ER图3_比赛表现.png')

print('\nAll E-R diagrams generated successfully!')
