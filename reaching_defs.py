"""
Reaching Definitions Analysis tool
- Uses functions from cfg_builder.py to build basic blocks and CFG
- Identifies definition statements (simple assignment patterns)
- Assigns unique IDs D1, D2, ... to each definition
- Computes gen[B], kill[B], in[B], out[B] using iterative dataflow
- Prints iteration tables and saves results to 'reaching_defs_<program>.txt'

Usage:
    python reaching_defs.py <file.c> [more.c ...]
    python reaching_defs.py --all    # runs on all .c files in the current directory

"""
import sys, os, glob
import re
from collections import defaultdict, OrderedDict

# Import helpers from cfg_builder (must be in same directory)
import cfg_builder as cb

ASSIGN_RE = re.compile(r"([A-Za-z_]\w*)\s*(?:\+=|-=|\*=|/=|%=|<<=|>>=|&=|\|=|\^=|=)(?!=)")


def find_definitions_in_blocks(blocks):
    """Scan blocks and return ordered list of definitions and mapping.
    Returns:
      defs: list of dicts with keys: id, var, block, stmt_index, code
      defs_by_block: dict block -> list of def ids in order
    """
    defs = []
    defs_by_block = defaultdict(list)
    counter = 1
    for bname, stmts in blocks.items():
        for si, stmt in enumerate(stmts):
            # skip preprocessor and empty lines
            s = stmt.strip()
            if not s or s.startswith('#'):
                continue
            m = ASSIGN_RE.search(s)
            if m:
                var = m.group(1)
                did = f"D{counter}"
                defs.append({'id': did, 'var': var, 'block': bname, 'stmt_index': si, 'code': s})
                defs_by_block[bname].append(did)
                counter += 1
    return defs, defs_by_block


def build_gen_kill(blocks, defs, defs_by_block):
    """Compute gen and kill sets per block.
    gen[B] = last definitions in B for each variable (i.e., those reaching block end)
    kill[B] = all other defs in the program that define variables that B defines
    """
    # map def id -> var
    def_var = {d['id']: d['var'] for d in defs}

    # group defs by var globally
    defs_by_var = defaultdict(list)
    for d in defs:
        defs_by_var[d['var']].append(d['id'])

    gen = {}
    kill = {}

    for bname, stmts in blocks.items():
        # for each variable defined in block, only the last def survives to the end
        last_def_for_var = {}
        # iterate defs in this block in order
        for did in defs_by_block.get(bname, []):
            var = def_var[did]
            last_def_for_var[var] = did
        gen_set = set(last_def_for_var.values())
        gen[bname] = gen_set

        # kill: all defs (global) of these variables except those generated in this block
        killed = set()
        for var, all_defs_for_var in defs_by_var.items():
            if var in last_def_for_var:
                for od in all_defs_for_var:
                    if od not in gen_set:
                        killed.add(od)
        kill[bname] = killed

    return gen, kill


def iterative_reaching(cfg, blocks, gen, kill):
    """Compute IN and OUT sets iteratively. Return history and final IN/OUT.
    history: list of dicts mapping block->(INset, OUTset) for each iteration.
    """
    blocks_list = list(blocks.keys())
    IN = {b: set() for b in blocks_list}
    OUT = {b: set() for b in blocks_list}

    history = []
    changed = True
    iteration = 0
    while changed:
        iteration += 1
        changed = False
        snapshot = {}
        for b in blocks_list:
            # in[B] = union of out[p] for predecessors p of B
            preds = list(cfg.predecessors(b)) if hasattr(cfg, 'predecessors') else []
            in_b = set()
            for p in preds:
                in_b |= OUT.get(p, set())
            out_b = gen.get(b, set()) | (in_b - kill.get(b, set()))
            # record whether changed
            if in_b != IN[b] or out_b != OUT[b]:
                changed = True
            IN[b] = in_b
            OUT[b] = out_b
            snapshot[b] = (set(in_b), set(out_b))
        history.append(snapshot)
        # safety: avoid infinite loop (shouldn't happen) but cap iterations
        if iteration > 500:
            print("Iteration cap reached; aborting")
            break
    return history, IN, OUT


def format_set(s):
    return '{' + ', '.join(sorted(s)) + '}' if s else '{}'


def analyze_file(filename, out_dir=None):
    print(f"Processing {filename}...")
    lines = cb.read_c_code(filename)
    leaders = cb.find_leaders(lines)
    blocks = cb.form_basic_blocks(lines, leaders)
    cfg = cb.build_cfg(blocks)

    defs, defs_by_block = find_definitions_in_blocks(blocks)
    # Ordered mapping id -> dict
    defs_map = {d['id']: d for d in defs}

    gen, kill = build_gen_kill(blocks, defs, defs_by_block)
    history, IN, OUT = iterative_reaching(cfg, blocks, gen, kill)

    # prepare output
    base = os.path.splitext(os.path.basename(filename))[0]
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)
        outpath = os.path.join(out_dir, f"reaching_defs_{base}.txt")
    else:
        outpath = f"reaching_defs_{base}.txt"

    with open(outpath, 'w') as f:
        f.write(f"Reaching Definitions Analysis for {filename}\n\n")
        f.write("All definitions (ID -> var, block, code):\n")
        for d in defs:
            f.write(f"  {d['id']}: var={d['var']}, block={d['block']}, code='{d['code']}'\n")
        f.write('\n')

        f.write("gen[B] and kill[B]:\n")
        for b in blocks.keys():
            f.write(f"  {b}: gen={format_set(gen.get(b, set()))}, kill={format_set(kill.get(b, set()))}\n")
        f.write('\n')

        # tabular iterations
        f.write("Iterative computation (per iteration):\n")
        for it, snap in enumerate(history, start=1):
            f.write(f"\nIteration {it}:\n")
            f.write(f"{'Block':<12}{'IN':>18}{'OUT':>20}\n")
            for b in blocks.keys():
                in_b, out_b = snap[b]
                f.write(f"{b:<12}{format_set(in_b):>18}{format_set(out_b):>20}\n")

        f.write('\nFinal IN/OUT sets:\n')
        for b in blocks.keys():
            f.write(f"  {b}: IN={format_set(IN[b])}, OUT={format_set(OUT[b])}\n")
        f.write('\nSummary table (per block):\n')
        f.write(f"{'Basic-Block':<12}{'gen[B]':>20}{'kill[B]':>20}{'in[B]':>20}{'out[B]':>20}\n")
        for b in blocks.keys():
            f.write(f"{b:<12}{format_set(gen.get(b, set())):>20}{format_set(kill.get(b, set())):>20}{format_set(IN.get(b, set())):>20}{format_set(OUT.get(b, set())):>20}\n")

        # Interpretation: variables that may have multiple reaching definitions at same program point
        f.write('\nInterpretation of Results:\n')
        f.write('Based on IN and OUT sets, the following variables may have multiple possible reaching definitions at the same program point:\n')
        # For each block and each variable, see if IN or OUT contains more than one definition for same var
        var_defs = defaultdict(list)  # var -> list of def ids
        for d in defs:
            var_defs[d['var']].append(d['id'])

        multi_sites = []
        for b in blocks.keys():
            # check IN[b]
            in_b = IN.get(b, set())
            out_b = OUT.get(b, set())
            # map var -> defs in sets
            var_in = defaultdict(list)
            var_out = defaultdict(list)
            for did in in_b:
                var = defs_map[did]['var']
                var_in[var].append(did)
            for did in out_b:
                var = defs_map[did]['var']
                var_out[var].append(did)

            for var, ids in var_in.items():
                if len(ids) > 1:
                    multi_sites.append((b, 'IN', var, ids))
            for var, ids in var_out.items():
                if len(ids) > 1:
                    multi_sites.append((b, 'OUT', var, ids))

        if not multi_sites:
            f.write('  None detected: at each program point there is at most one reaching definition per variable.\n')
        else:
            for b, where, var, ids in multi_sites:
                f.write(f"  Block {b} {where}: variable '{var}' may be defined by {ids}\n")

    print(f"Wrote results to {outpath}")
    return {
        'filename': filename,
        'defs': defs_map,
        'gen': gen,
        'kill': kill,
        'IN': IN,
        'OUT': OUT,
        'history': history,
        'outpath': outpath
    }


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('Usage: python reaching_defs.py <file.c> [more.c ...]  or  python reaching_defs.py --all')
        sys.exit(1)
    args = sys.argv[1:]
    if args[0] == '--all':
        files = sorted(glob.glob('*.c'))
    else:
        files = args

    results = []
    for fn in files:
        if not os.path.isfile(fn):
            print(f"Skipping {fn}: not found")
            continue
        res = analyze_file(fn, out_dir=None)
        results.append(res)

    # summary print
    print('\nSummary:')
    for r in results:
        N = len(r['gen'])  # approximate: number of blocks
        total_defs = len(r['defs'])
        print(f"{r['filename']}: defs={total_defs}, blocks={len(r['gen'])}, output={r['outpath']}")
