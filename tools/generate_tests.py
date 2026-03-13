#!/usr/bin/env python3
"""
Pane — Stage 4 Test Generator

Generates HTML test cases from the pairwise interaction catalog.
Each test exercises a specific layout behavior that the Pane engine
should handle correctly.

Test categories:
  1. Block formatting context (BFC) rules
  2. Inline formatting context
  3. Flexbox layout
  4. Grid layout
  5. Table layout
  6. Box model (margin, padding, border, box-sizing)
  7. Pairwise context transitions (parent→child)
  8. CSS property interactions (display gating, axis, etc.)

Output: tests/ directory with HTML files + expected.json
"""

import json
import os

OUTPUT_DIR = os.path.join(os.path.dirname(os.path.dirname(__file__)), "tests", "generated")


def mkdir_p(path):
    os.makedirs(path, exist_ok=True)


# ── Test Case Structure ─────────────────────────────────────────────

class TestCase:
    def __init__(self, name, category, html, description, expected):
        self.name = name
        self.category = category
        self.html = html
        self.description = description
        self.expected = expected  # dict of element_id → {x, y, width, height}

    def to_dict(self):
        return {
            "name": self.name,
            "category": self.category,
            "description": self.description,
            "expected": self.expected,
        }


# ── Test Generators ─────────────────────────────────────────────────

def gen_block_tests():
    """Block formatting context rules from layout_rules_full.md"""
    tests = []

    # BFC-1: Basic block flow — children stack vertically
    tests.append(TestCase(
        "block_vertical_flow",
        "block",
        '<div id="parent" style="width:200px">'
        '<div id="a" style="height:30px; background:#f00"></div>'
        '<div id="b" style="height:40px; background:#0f0"></div>'
        '<div id="c" style="height:20px; background:#00f"></div>'
        '</div>',
        "Block children stack vertically, each full width",
        {
            "a": {"x": 0, "y": 0, "width": 200, "height": 30},
            "b": {"x": 0, "y": 30, "width": 200, "height": 40},
            "c": {"x": 0, "y": 70, "width": 200, "height": 20},
        }
    ))

    # W-4: Auto margins center horizontally
    tests.append(TestCase(
        "block_auto_margin_center",
        "block",
        '<div id="parent" style="width:400px">'
        '<div id="child" style="width:200px; margin-left:auto; margin-right:auto; height:50px"></div>'
        '</div>',
        "Block with auto left+right margins centers horizontally",
        {
            "child": {"x": 100, "y": 0, "width": 200, "height": 50},
        }
    ))

    # H-2: Auto height wraps children
    tests.append(TestCase(
        "block_auto_height",
        "block",
        '<div id="parent" style="width:300px">'
        '<div id="a" style="height:25px"></div>'
        '<div id="b" style="height:35px"></div>'
        '</div>',
        "Auto-height parent wraps children (25+35=60)",
        {
            "parent": {"width": 300, "height": 60},
        }
    ))

    # Box sizing: border-box (using longhands — shorthand expansion not yet implemented)
    tests.append(TestCase(
        "block_border_box",
        "block",
        '<div id="box" style="width:200px; height:100px; '
        'padding-top:20px; padding-right:20px; padding-bottom:20px; padding-left:20px; '
        'border-top-width:5px; border-right-width:5px; border-bottom-width:5px; border-left-width:5px; '
        'border-top-style:solid; border-right-style:solid; border-bottom-style:solid; border-left-style:solid; '
        'box-sizing:border-box"></div>',
        "border-box: width includes padding+border (content=150x50)",
        {
            "box": {"width": 150, "height": 50},
        }
    ))

    # Nested blocks: width inheritance with border-box
    tests.append(TestCase(
        "block_nested_width",
        "block",
        '<div id="outer" style="width:400px; padding-left:10px; padding-right:10px; box-sizing:border-box">'
        '<div id="inner" style="height:30px"></div>'
        '</div>',
        "border-box: content area = 400 - 10 - 10 = 380, child fills it",
        {
            "inner": {"width": 380, "height": 30},
        }
    ))

    return tests


def gen_margin_tests():
    """Margin collapsing rules"""
    tests = []

    # Margin collapsing: adjacent siblings
    tests.append(TestCase(
        "margin_collapse_siblings",
        "margin",
        '<div style="width:200px">'
        '<div id="a" style="height:20px; margin-bottom:30px"></div>'
        '<div id="b" style="height:20px; margin-top:20px"></div>'
        '</div>',
        "Adjacent sibling margins collapse: max(30,20)=30",
        {
            "a": {"y": 0, "height": 20},
            "b": {"y": 50, "height": 20},  # 20 + max(30,20) = 50
        }
    ))

    # No margin collapse in flex
    tests.append(TestCase(
        "margin_no_collapse_flex",
        "margin",
        '<div style="display:flex; flex-direction:column; width:200px">'
        '<div id="a" style="height:20px; margin-bottom:30px"></div>'
        '<div id="b" style="height:20px; margin-top:20px"></div>'
        '</div>',
        "Flex children: margins do not collapse (30+20=50 gap)",
        {
            "a": {"y": 0, "height": 20},
            "b": {"y": 70, "height": 20},  # 20 + 30 + 20 = 70
        }
    ))

    return tests


def gen_inline_tests():
    """Inline formatting context"""
    tests = []

    # Basic inline flow
    tests.append(TestCase(
        "inline_basic_text",
        "inline",
        '<div id="parent" style="width:200px; font-size:16px">'
        '<span id="a">Hello</span> <span id="b">World</span>'
        '</div>',
        "Inline spans flow horizontally within block container",
        {
            "parent": {"width": 200},
            # Text positions are approximate, just check they render
        }
    ))

    return tests


def gen_flex_tests():
    """Flexbox layout"""
    tests = []

    # Basic row flex
    tests.append(TestCase(
        "flex_row_basic",
        "flex",
        '<div id="container" style="display:flex; width:300px">'
        '<div id="a" style="width:100px; height:50px"></div>'
        '<div id="b" style="width:100px; height:50px"></div>'
        '<div id="c" style="width:100px; height:50px"></div>'
        '</div>',
        "Row flex: 3 items of 100px each fill 300px",
        {
            "a": {"x": 0, "y": 0, "width": 100, "height": 50},
            "b": {"x": 100, "y": 0, "width": 100, "height": 50},
            "c": {"x": 200, "y": 0, "width": 100, "height": 50},
        }
    ))

    # Flex-grow
    tests.append(TestCase(
        "flex_grow",
        "flex",
        '<div id="container" style="display:flex; width:400px">'
        '<div id="a" style="flex-grow:1; height:40px"></div>'
        '<div id="b" style="flex-grow:1; height:40px"></div>'
        '</div>',
        "Two flex-grow:1 items split space equally (200px each)",
        {
            "a": {"x": 0, "width": 200, "height": 40},
            "b": {"x": 200, "width": 200, "height": 40},
        }
    ))

    # Flex-direction: column
    tests.append(TestCase(
        "flex_column",
        "flex",
        '<div id="container" style="display:flex; flex-direction:column; width:200px; height:300px">'
        '<div id="a" style="flex-grow:1"></div>'
        '<div id="b" style="flex-grow:2"></div>'
        '</div>',
        "Column flex: items split height by grow ratio (1:2 = 100:200)",
        {
            "a": {"x": 0, "y": 0, "height": 100},
            "b": {"x": 0, "y": 100, "height": 200},
        }
    ))

    # Justify-content: center
    tests.append(TestCase(
        "flex_justify_center",
        "flex",
        '<div id="container" style="display:flex; justify-content:center; width:400px">'
        '<div id="a" style="width:100px; height:40px"></div>'
        '<div id="b" style="width:100px; height:40px"></div>'
        '</div>',
        "justify-content:center → 100px gap on each side",
        {
            "a": {"x": 100, "width": 100, "height": 40},
            "b": {"x": 200, "width": 100, "height": 40},
        }
    ))

    # Justify-content: space-between
    tests.append(TestCase(
        "flex_justify_space_between",
        "flex",
        '<div id="container" style="display:flex; justify-content:space-between; width:400px">'
        '<div id="a" style="width:100px; height:30px"></div>'
        '<div id="b" style="width:100px; height:30px"></div>'
        '</div>',
        "space-between: first at 0, last at end",
        {
            "a": {"x": 0, "width": 100},
            "b": {"x": 300, "width": 100},
        }
    ))

    # Align-items: center
    tests.append(TestCase(
        "flex_align_center",
        "flex",
        '<div id="container" style="display:flex; align-items:center; width:300px; height:100px">'
        '<div id="a" style="width:100px; height:40px"></div>'
        '</div>',
        "align-items:center → vertically centered (y=30)",
        {
            "a": {"y": 30, "height": 40},
        }
    ))

    return tests


def gen_grid_tests():
    """Grid layout"""
    tests = []

    # Basic 2x2 grid
    tests.append(TestCase(
        "grid_2x2",
        "grid",
        '<div id="container" style="display:grid; grid-template-columns:100px 100px; width:200px">'
        '<div id="a" style="height:30px"></div>'
        '<div id="b" style="height:30px"></div>'
        '<div id="c" style="height:30px"></div>'
        '<div id="d" style="height:30px"></div>'
        '</div>',
        "2x2 grid: 100px columns, auto-placed",
        {
            "a": {"x": 0, "y": 0, "width": 100, "height": 30},
            "b": {"x": 100, "y": 0, "width": 100, "height": 30},
            "c": {"x": 0, "y": 30, "width": 100, "height": 30},
            "d": {"x": 100, "y": 30, "width": 100, "height": 30},
        }
    ))

    # Fr units
    tests.append(TestCase(
        "grid_fr_units",
        "grid",
        '<div id="container" style="display:grid; grid-template-columns:1fr 2fr; width:300px">'
        '<div id="a" style="height:40px"></div>'
        '<div id="b" style="height:40px"></div>'
        '</div>',
        "1fr 2fr on 300px → 100px 200px columns",
        {
            "a": {"x": 0, "width": 100, "height": 40},
            "b": {"x": 100, "width": 200, "height": 40},
        }
    ))

    # Gap
    tests.append(TestCase(
        "grid_gap",
        "grid",
        '<div id="container" style="display:grid; grid-template-columns:1fr 1fr; gap:10px; width:210px">'
        '<div id="a" style="height:20px"></div>'
        '<div id="b" style="height:20px"></div>'
        '<div id="c" style="height:20px"></div>'
        '<div id="d" style="height:20px"></div>'
        '</div>',
        "2x2 grid with 10px gap: cols=100px each, rows 10px apart",
        {
            "a": {"x": 0, "y": 0, "width": 100},
            "b": {"x": 110, "y": 0, "width": 100},
            "c": {"x": 0, "y": 30},
            "d": {"x": 110, "y": 30},
        }
    ))

    # Explicit placement
    tests.append(TestCase(
        "grid_explicit_placement",
        "grid",
        '<div id="container" style="display:grid; grid-template-columns:100px 100px 100px; width:300px">'
        '<div id="a" style="grid-column-start:2; grid-column-end:4; height:30px"></div>'
        '<div id="b" style="height:30px"></div>'
        '</div>',
        "Item A spans columns 2-3, item B auto-placed in col 1",
        {
            "a": {"x": 100, "width": 200, "height": 30},
            "b": {"x": 0, "y": 0, "width": 100, "height": 30},
        }
    ))

    # Mixed fixed and fr
    tests.append(TestCase(
        "grid_mixed_tracks",
        "grid",
        '<div id="container" style="display:grid; grid-template-columns:100px 1fr; width:400px">'
        '<div id="a" style="height:25px"></div>'
        '<div id="b" style="height:25px"></div>'
        '</div>',
        "100px fixed + 1fr(300px) columns",
        {
            "a": {"x": 0, "width": 100},
            "b": {"x": 100, "width": 300},
        }
    ))

    return tests


def gen_table_tests():
    """Table layout"""
    tests = []

    tests.append(TestCase(
        "table_basic",
        "table",
        '<table id="t" style="width:400px">'
        '<tr id="r1"><td id="c1">A</td><td id="c2">B</td></tr>'
        '<tr id="r2"><td id="c3">C</td><td id="c4">D</td></tr>'
        '</table>',
        "Basic 2x2 table: cells split width equally",
        {
            "t": {"width": 400},
        }
    ))

    return tests


def gen_context_transition_tests():
    """Pairwise context transitions: parent→child formatting context changes"""
    tests = []

    # Block parent → flex child
    tests.append(TestCase(
        "ctx_block_flex_child",
        "context",
        '<div id="parent" style="width:400px">'
        '<div id="flex" style="display:flex">'
        '<div id="a" style="width:100px; height:30px"></div>'
        '<div id="b" style="width:100px; height:30px"></div>'
        '</div></div>',
        "Flex container inside block: flex items flow horizontally",
        {
            "a": {"x": 0, "y": 0, "width": 100, "height": 30},
            "b": {"x": 100, "y": 0, "width": 100, "height": 30},
        }
    ))

    # Block parent → grid child
    tests.append(TestCase(
        "ctx_block_grid_child",
        "context",
        '<div id="parent" style="width:400px">'
        '<div id="grid" style="display:grid; grid-template-columns:1fr 1fr">'
        '<div id="a" style="height:30px"></div>'
        '<div id="b" style="height:30px"></div>'
        '</div></div>',
        "Grid container inside block: items placed in 2-col grid",
        {
            "a": {"x": 0, "width": 200, "height": 30},
            "b": {"x": 200, "width": 200, "height": 30},
        }
    ))

    # Flex parent → block child
    tests.append(TestCase(
        "ctx_flex_block_child",
        "context",
        '<div style="display:flex; width:300px">'
        '<div id="block" style="width:150px">'
        '<div id="inner" style="height:20px"></div>'
        '</div></div>',
        "Block inside flex item: inner block fills parent width",
        {
            "block": {"width": 150},
            "inner": {"width": 150, "height": 20},
        }
    ))

    # Grid parent → flex child
    tests.append(TestCase(
        "ctx_grid_flex_child",
        "context",
        '<div style="display:grid; grid-template-columns:200px 200px; width:400px">'
        '<div id="flex" style="display:flex">'
        '<div id="a" style="flex-grow:1; height:25px"></div>'
        '<div id="b" style="flex-grow:1; height:25px"></div>'
        '</div></div>',
        "Flex container inside grid cell: items split cell width",
        {
            "a": {"width": 100, "height": 25},
            "b": {"width": 100, "height": 25},
        }
    ))

    # Deeply nested contexts
    tests.append(TestCase(
        "ctx_deep_nesting",
        "context",
        '<div style="width:400px">'
        '<div style="display:flex">'
        '<div style="flex-grow:1">'
        '<div id="grid" style="display:grid; grid-template-columns:1fr 1fr">'
        '<div id="a" style="height:20px"></div>'
        '<div id="b" style="height:20px"></div>'
        '</div></div></div></div>',
        "Grid inside flex inside block: context propagates correctly",
        {
            "a": {"width": 200, "height": 20},
            "b": {"width": 200, "height": 20},
        }
    ))

    return tests


def gen_display_gating_tests():
    """CSS property interactions: display-gated properties"""
    tests = []

    # Flex properties ignored on non-flex container
    tests.append(TestCase(
        "display_gate_flex_on_block",
        "css_interaction",
        '<div id="parent" style="width:200px; justify-content:center">'
        '<div id="child" style="width:100px; height:30px"></div>'
        '</div>',
        "justify-content ignored on block container — child at x=0",
        {
            "child": {"x": 0, "width": 100, "height": 30},
        }
    ))

    # Grid properties ignored on non-grid container
    tests.append(TestCase(
        "display_gate_grid_on_block",
        "css_interaction",
        '<div id="parent" style="width:200px; grid-template-columns:100px 100px">'
        '<div id="child" style="height:30px"></div>'
        '</div>',
        "grid-template-columns ignored on block container — child full width",
        {
            "child": {"width": 200, "height": 30},
        }
    ))

    # Display: none hides element
    tests.append(TestCase(
        "display_none",
        "css_interaction",
        '<div style="width:200px">'
        '<div id="a" style="height:20px"></div>'
        '<div style="display:none; height:100px"></div>'
        '<div id="b" style="height:20px"></div>'
        '</div>',
        "display:none element takes no space — b follows a directly",
        {
            "a": {"y": 0, "height": 20},
            "b": {"y": 20, "height": 20},
        }
    ))

    return tests


def gen_box_model_tests():
    """Box model: padding, border, margin interactions"""
    tests = []

    # Padding adds to content area
    tests.append(TestCase(
        "box_padding",
        "box_model",
        '<div id="box" style="width:200px; height:100px; padding:10px"></div>',
        "Padding: content 200x100, total 220x120",
        {
            "box": {"width": 200, "height": 100},  # content area
        }
    ))

    # Percentage width
    tests.append(TestCase(
        "box_percentage_width",
        "box_model",
        '<div style="width:400px">'
        '<div id="child" style="width:50%; height:30px"></div>'
        '</div>',
        "50% width of 400px parent = 200px",
        {
            "child": {"width": 200, "height": 30},
        }
    ))

    # Min-width constraint
    tests.append(TestCase(
        "box_min_width",
        "box_model",
        '<div style="width:400px">'
        '<div id="child" style="width:100px; min-width:200px; height:30px"></div>'
        '</div>',
        "min-width:200px overrides width:100px",
        {
            "child": {"width": 200, "height": 30},
        }
    ))

    # Max-width constraint
    tests.append(TestCase(
        "box_max_width",
        "box_model",
        '<div style="width:400px">'
        '<div id="child" style="width:300px; max-width:200px; height:30px"></div>'
        '</div>',
        "max-width:200px overrides width:300px",
        {
            "child": {"width": 200, "height": 30},
        }
    ))

    return tests


def gen_html_nesting_tests():
    """HTML parent→child nesting pair tests from the catalog"""
    tests = []

    # Heading inside paragraph: auto-closes <p>
    tests.append(TestCase(
        "nest_p_h1",
        "nesting",
        '<div style="width:400px">'
        '<p id="para">text</p>'
        '<h1 id="heading">Title</h1>'
        '</div>',
        "h1 after p: both are block children of div",
        {
            "heading": {"width": 400},
        }
    ))

    # Table with thead/tbody/tfoot
    tests.append(TestCase(
        "nest_table_sections",
        "nesting",
        '<table id="t" style="width:300px">'
        '<thead><tr><td id="h1">H</td></tr></thead>'
        '<tbody><tr><td id="b1">B</td></tr></tbody>'
        '</table>',
        "Table with thead/tbody: rows laid out correctly",
        {
            "t": {"width": 300},
        }
    ))

    # List items
    tests.append(TestCase(
        "nest_ul_li",
        "nesting",
        '<ul id="list" style="width:300px; padding:0; margin:0">'
        '<li id="item1" style="height:20px">A</li>'
        '<li id="item2" style="height:20px">B</li>'
        '</ul>',
        "List items stack vertically",
        {
            "item1": {"y": 0, "height": 20},
            "item2": {"y": 20, "height": 20},
        }
    ))

    return tests


# ── Main ─────────────────────────────────────────────────────────────

def main():
    mkdir_p(OUTPUT_DIR)

    all_tests = []
    generators = [
        gen_block_tests,
        gen_margin_tests,
        gen_inline_tests,
        gen_flex_tests,
        gen_grid_tests,
        gen_table_tests,
        gen_context_transition_tests,
        gen_display_gating_tests,
        gen_box_model_tests,
        gen_html_nesting_tests,
    ]

    for gen in generators:
        tests = gen()
        all_tests.extend(tests)
        for t in tests:
            html_path = os.path.join(OUTPUT_DIR, f"{t.name}.html")
            # Wrap in a full document with viewport
            full_html = (
                f'<!DOCTYPE html>\n'
                f'<html><head><style>\n'
                f'  * {{ margin: 0; padding: 0; }}\n'
                f'  body {{ font-size: 16px; }}\n'
                f'</style></head><body>\n'
                f'{t.html}\n'
                f'</body></html>\n'
            )
            with open(html_path, 'w') as f:
                f.write(full_html)

    # Write manifest
    manifest = {
        "test_count": len(all_tests),
        "categories": {},
        "tests": [],
    }
    for t in all_tests:
        manifest["tests"].append(t.to_dict())
        cat = t.category
        if cat not in manifest["categories"]:
            manifest["categories"][cat] = 0
        manifest["categories"][cat] += 1

    manifest_path = os.path.join(OUTPUT_DIR, "manifest.json")
    with open(manifest_path, 'w') as f:
        json.dump(manifest, f, indent=2)

    print(f"Generated {len(all_tests)} test cases in {OUTPUT_DIR}/")
    for cat, count in sorted(manifest["categories"].items()):
        print(f"  {cat}: {count}")


if __name__ == "__main__":
    main()
