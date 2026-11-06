#!/usr/bin/env python3
"""Static syntax/safety gate for standalone HTML breadboard."""
from pathlib import Path
import subprocess
import tempfile

src = Path(__file__).with_name("editor.html").read_text(encoding="utf-8")
assert src.count("<script>") == 1 and src.count("</script>") == 1
js = src.split("<script>", 1)[1].split("</script>", 1)[0]
for required in ("function validate()", "function netlist()", "function download(", "function seed(", "export-spice", "export-bom", "export-wires", "save-project"):
    assert required in src, f"Missing editor feature: {required}"
with tempfile.TemporaryDirectory() as root:
    path=Path(root)/"editor.js"
    path.write_text(js, encoding="utf-8")
    subprocess.run(["node", "--check", str(path)], check=True)
print("PASS: HTML electrical editor JS syntax and essential controls")
