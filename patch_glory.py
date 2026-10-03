"""يعدّل نسخة ReShade المنسوخة: يغيّر الاسم الظاهر في القائمة إلى Glory ويضيف قائمة Glory Theme."""
import re, shutil, sys, os

root = sys.argv[1]
gui = os.path.join(root, "source", "runtime_gui.cpp")
shutil.copy("glory_theme.hpp", os.path.join(root, "source", "glory_theme.hpp"))

lines = open(gui, encoding="utf-8-sig").read().split("\n")
SKIP = (".fx", ".ini", ".dll", "ReShade_", "ReShade::", "http", "\\")

def rename_literals(line):
    if line.lstrip().startswith("#"):
        return line
    def fix(m):
        body = m.group(1)
        if "ReShade" not in body or any(s in body for s in SKIP):
            return m.group(0)
        return '"' + body.replace("ReShade", "Glory") + '"'
    return re.sub(r'"([^"\n]*)"', fix, line)

renamed = 0
for i, l in enumerate(lines):
    n = rename_literals(l)
    if n != l:
        renamed += 1
        lines[i] = n
print(f"renamed {renamed} lines")

# include بعد آخر #include في أول الملف
last_inc = max(i for i, l in enumerate(lines[:150]) if l.startswith("#include"))
lines.insert(last_inc + 1, '#include "glory_theme.hpp"')

def insert_before(pattern, code, start=0):
    for i in range(start, len(lines)):
        if pattern in lines[i]:
            indent = re.match(r"\s*", lines[i]).group(0)
            lines.insert(i, indent + code)
            return i
    sys.exit(f"ANCHOR NOT FOUND: {pattern}")

nf = insert_before("ImGui::NewFrame();", "glory::apply_style();")
insert_before("ImGui::Render();", "glory::draw_window(_show_overlay, _device);", nf)
open(gui, "w", encoding="utf-8").write("\n".join(lines))
print("patched OK")
