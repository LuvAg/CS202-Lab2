"""
CFG Builder for C Programs (for Amalthea Lab 7)
-----------------------------------------------
✅ Identifies Leaders based on rules:
  1. First instruction is a leader.
  2. Any target of a branch/jump/loop is a leader.
  3. Any instruction immediately after a branch/jump/loop is a leader.

✅ Builds Basic Blocks (maximal sequence of straight-line code)
✅ Builds Control Flow Graph (CFG)
✅ Exports .dot and .png diagrams using Graphviz
"""

import sys, os, re, glob, csv
import networkx as nx
import shutil

# If Graphviz is commonly installed in a default location, prefer that directory
# at the front of PATH so the graphviz Python package can find the `dot` binary.
default_gv_paths = [r"C:\Program Files\Graphviz\bin", r"C:\Program Files (x86)\Graphviz\bin"]
for p in default_gv_paths:
    if os.path.isdir(p) and p not in os.environ.get("PATH", ""):
        os.environ["PATH"] = p + os.pathsep + os.environ.get("PATH", "")

# Import Digraph after possibly updating PATH
from graphviz import Digraph

# Check for `dot` availability so we can provide a helpful message instead of raising.
if shutil.which("dot") is None:
    DOT_AVAILABLE = False
    sys.stderr.write(
        "Warning: 'dot' executable not found on PATH. Install Graphviz (https://graphviz.org/download/) or add its bin directory to PATH.\n"
    )
    sys.stderr.write("PowerShell example: winget install --id Graphviz.Graphviz -e ; then restart your shell.\n")
    sys.stderr.write("Verify with: where.exe dot  and  dot -V\n")
else:
    DOT_AVAILABLE = True

# -------- Helper functions -------- #

def read_c_code(filename):
    """Read C code and remove comments for cleaner parsing"""
    with open(filename, 'r') as f:
        code = f.read()
    code = re.sub(r'//.*', '', code)  # remove single-line comments
    code = re.sub(r'/\*.*?\*/', '', code, flags=re.DOTALL)  # remove block comments
    lines = [line.strip() for line in code.split('\n') if line.strip()]
    return lines

def find_leaders(lines):
    """Apply leader rules and return list of leader indices"""
    n = len(lines)
    leaders = set([0])  # rule 1: first instruction
    branch_keywords = ['if', 'else if', 'else', 'for', 'while', 'switch', 'case', 'default']

    for i, line in enumerate(lines):
        # Detect branch/jump/loop
        if any(line.startswith(k) for k in branch_keywords):
            leaders.add(i)  # rule 2: target of branch/loop
            if i + 1 < n:
                leaders.add(i + 1)  # rule 3: next instruction
        elif re.match(r'.*\breturn\b', line):
            if i + 1 < n:
                leaders.add(i + 1)

    leaders = sorted(list(leaders))
    return leaders

def form_basic_blocks(lines, leaders):
    """Group instructions into basic blocks"""
    blocks = {}
    for i, leader in enumerate(leaders):
        start = leader
        end = leaders[i + 1] if i + 1 < len(leaders) else len(lines)
        block_lines = lines[start:end]
        blocks[f"B{i}"] = block_lines
    return blocks

def build_cfg(blocks):
    """Construct the CFG as a directed graph"""
    cfg = nx.DiGraph()
    block_names = list(blocks.keys())

    for i, bname in enumerate(block_names):
        cfg.add_node(bname)

    # Add edges according to simplified control flow rules
    for i, (bname, code) in enumerate(blocks.items()):
        last = code[-1] if code else ''
        next_block = block_names[i + 1] if i + 1 < len(block_names) else None

        # Sequential flow
        if next_block and not any(k in last for k in ['return', 'break']):
            cfg.add_edge(bname, next_block)

        # If-else structure
        if 'if' in last or last.startswith('if'):
            if i + 1 < len(block_names):
                cfg.add_edge(bname, block_names[i + 1], label="true")
            if i + 2 < len(block_names):
                cfg.add_edge(bname, block_names[i + 2], label="false")

        # Loops: while / for
        if last.startswith('while') or last.startswith('for'):
            if i + 1 < len(block_names):
                cfg.add_edge(bname, block_names[i + 1], label="loop-body")
                cfg.add_edge(block_names[i + 1], bname, label="back-edge")

        # Switch / case
        if last.startswith('switch'):
            for j in range(i + 1, len(block_names)):
                if any('case' in l or 'default' in l for l in blocks[block_names[j]]):
                    cfg.add_edge(bname, block_names[j])

    return cfg

def write_dot(blocks, cfg, filename):
    """Write .dot file and render .png"""
    dot = Digraph(comment=f'CFG for {filename}', format='png')
    for bname, stmts in blocks.items():
        label = f"{bname}:\\n" + "\\n".join(stmts)
        dot.node(bname, label=label, shape='box')

    for u, v, data in cfg.edges(data=True):
        label = data.get('label', '')
        dot.edge(u, v, label=label)

    base = os.path.splitext(filename)[0]
    dot_path = f"{base}.dot"
    png_path = f"{base}.png"
    dot.save(dot_path)
    if DOT_AVAILABLE:
        try:
            dot.render(base, view=False)
            print(f"✅ Generated {dot_path} and {png_path}")
        except Exception as e:
            print(f"Warning: failed to render PNG using Graphviz: {e}")
            print(f"The .dot file was saved as {dot_path}. You can render it manually once Graphviz is installed:")
            print(f"  dot -Tpng {dot_path} -o {png_path}")
    else:
        print(f"Warning: 'dot' not found on PATH. Saved DOT only: {dot_path}")
        print(f"Install Graphviz and run: dot -Tpng {dot_path} -o {png_path} to produce the PNG")

def print_summary(blocks, cfg):
    print("\n=== BASIC BLOCKS ===")
    for bname, stmts in blocks.items():
        print(f"\n{bname}:")
        for s in stmts:
            print("   ", s)

    print("\n=== CFG Edges ===")
    for u, v, d in cfg.edges(data=True):
        label = d.get('label', '')
        print(f"{u} -> {v} {label}")

    N = cfg.number_of_nodes()
    E = cfg.number_of_edges()
    CC = E - N + 2
    print(f"\nNodes (N): {N}, Edges (E): {E}, Cyclomatic Complexity (CC): {CC}")


def compute_metrics(blocks, cfg):
    """Return metrics tuple (N, E, CC) for the given CFG."""
    N = cfg.number_of_nodes()
    E = cfg.number_of_edges()
    CC = E - N + 2
    return N, E, CC

# -------- MAIN DRIVER -------- #

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python cfg_builder.py <filename.c> [more_files.c]   or   python cfg_builder.py --all")
        sys.exit(1)

    args = sys.argv[1:]
    if args[0] == '--all':
        files = sorted([p for p in glob.glob('*.c') if os.path.isfile(p)])
        if not files:
            print("No .c files found in current directory.")
            sys.exit(1)
    else:
        files = args

    metrics = []
    for filename in files:
        if not os.path.isfile(filename):
            print(f"Skipping '{filename}': file not found.")
            continue

        print(f"\n--- Processing: {filename} ---")
        lines = read_c_code(filename)
        leaders = find_leaders(lines)
        blocks = form_basic_blocks(lines, leaders)
        cfg = build_cfg(blocks)
        print_summary(blocks, cfg)
        write_dot(blocks, cfg, filename)
        N, E, CC = compute_metrics(blocks, cfg)
        metrics.append((filename, N, E, CC))

    if metrics:
        print("\n=== METRICS TABLE ===")
        print(f"{'Program':<30}{'N':>8}{'E':>8}{'CC':>8}")
        for fn, n, e, cc in metrics:
            print(f"{fn:<30}{n:>8}{e:>8}{cc:>8}")

        # save CSV
        csv_path = 'metrics.csv'
        try:
            with open(csv_path, 'w', newline='') as cf:
                w = csv.writer(cf)
                w.writerow(['Program', 'N', 'E', 'CC'])
                for fn, n, e, cc in metrics:
                    w.writerow([fn, n, e, cc])
            print(f"Saved metrics CSV to {csv_path}")
        except Exception as ex:
            print(f"Failed to write metrics CSV: {ex}")
