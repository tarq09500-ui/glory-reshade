"""يعدّل نسخة ReShade: الاسم الظاهر -> Glory، ويضيف تبويب Background في شريط التبويبات."""
import re, shutil, sys, os

root = sys.argv[1]
src = os.path.join(root, "source")
gui_path = os.path.join(src, "runtime_gui.cpp")
hpp_path = os.path.join(src, "runtime.hpp")
shutil.copy("glory_theme.hpp", os.path.join(src, "glory_theme.hpp"))

text = open(gui_path, encoding="utf-8-sig").read()
lines = text.split("\n")
SKIP = (".fx", ".ini", ".dll", ".log", "ReShade_", "ReShade::", "http", "\\", "reshade-")

def rename_literals(line):
    if line.lstrip().startswith("#"):
        return line
    def fix(m):
        body = m.group(1)
        if "ReShade" not in body or any(s in body for s in SKIP):
            return m.group(0)
        return '"' + body.replace("ReShade", "Glory") + '"'
    return re.sub(r'"([^"\n]*)"', fix, line)

n = 0
for i, l in enumerate(lines):
    r = rename_literals(l)
    if r != l:
        lines[i] = r; n += 1
print(f"[glory] renamed text on {n} lines")

last_inc = max(i for i, l in enumerate(lines[:200]) if l.startswith("#include"))
lines.insert(last_inc + 1, '#include "glory_theme.hpp"')

def find(pattern, start=0):
    for i in range(start, len(lines)):
        if pattern in lines[i]:
            return i
    return -1

def insert_before(idx, code):
    indent = re.match(r"\s*", lines[idx]).group(0)
    lines.insert(idx, indent + code)

full = "\n".join(lines)
show_var = "_show_overlay" if "_show_overlay" in full else ("show_overlay" if "show_overlay" in full else "true")
dev_expr = "_device" if "_device" in full else "get_device()"
print(f"[glory] show var = {show_var}, device = {dev_expr}")

nf = find("ImGui::NewFrame();")
if nf < 0:
    sys.exit("ANCHOR NOT FOUND: ImGui::NewFrame();")
insert_before(nf, "glory::apply_style();")
rd = find("ImGui::Render();", nf)
if rd < 0:
    sys.exit("ANCHOR NOT FOUND: ImGui::Render();")

# تبويب Background
tab_ok = False
hpp = open(hpp_path, encoding="utf-8-sig").read()
m = re.search(r"^([ \t]*)void draw_gui_about\(\);", hpp, re.M)
about = find("&runtime::draw_gui_about")
if m and about >= 0:
    hpp = hpp[:m.end()] + "\n" + m.group(1) + "void draw_gui_background();" + hpp[m.end():]
    open(hpp_path, "w", encoding="utf-8").write(hpp)
    about = find("&runtime::draw_gui_about")
    indent = re.match(r"\s*", lines[about]).group(0)
    lines.insert(about, indent + '{ "Background###background", &runtime::draw_gui_background },')
    lines.append("")
    lines.append("void reshade::runtime::draw_gui_background()")
    lines.append("{")
    lines.append("\tglory::draw_controls();")
    lines.append("}")
    tab_ok = True
    print("[glory] Background tab added")
else:
    print("[glory] WARNING: tab anchors not found -> using floating 'Glory Theme' window instead")

rd = find("ImGui::Render();", find("glory::apply_style();"))
insert_before(rd, f"glory::frame({show_var}, {dev_expr}, {'false' if tab_ok else 'true'});")
open(gui_path, "w", encoding="utf-8").write("\n".join(lines))
print("[glory] patched OK")
