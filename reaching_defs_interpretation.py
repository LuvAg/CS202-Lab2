"""
Generate interpretation-only reports:
For each C file, identify variables that may have multiple reaching definitions at the same program point
and write a small text file `interpretation_<program>.txt` containing only that section.

Usage:
  python reaching_defs_interpretation.py --all
  python reaching_defs_interpretation.py file1.c file2.c
"""
import sys, glob, os
import cfg_builder as cb
import reaching_defs as rd
from collections import defaultdict


def analyze_and_write(fn):
    base = os.path.splitext(os.path.basename(fn))[0]
    lines = cb.read_c_code(fn)
    leaders = cb.find_leaders(lines)
    blocks = cb.form_basic_blocks(lines, leaders)
    cfg = cb.build_cfg(blocks)

    defs, defs_by_block = rd.find_definitions_in_blocks(blocks)
    defs_map = {d['id']: d for d in defs}
    gen, kill = rd.build_gen_kill(blocks, defs, defs_by_block)
    history, IN, OUT = rd.iterative_reaching(cfg, blocks, gen, kill)

    # find multi-definition sites
    multi_sites = []
    for b in blocks.keys():
        in_b = IN.get(b, set())
        out_b = OUT.get(b, set())
        var_in = defaultdict(list)
        var_out = defaultdict(list)
        for did in in_b:
            var_in[defs_map[did]['var']].append(did)
        for did in out_b:
            var_out[defs_map[did]['var']].append(did)
        for var, ids in var_in.items():
            if len(ids) > 1:
                multi_sites.append((b, 'IN', var, ids))
        for var, ids in var_out.items():
            if len(ids) > 1:
                multi_sites.append((b, 'OUT', var, ids))

    outname = f"interpretation_{base}.txt"
    with open(outname, 'w') as f:
        f.write(f"Interpretation of reaching definitions for {fn}\n\n")
        if not multi_sites:
            f.write('None detected: at each program point there is at most one reaching definition per variable.\n')
        else:
            for b, where, var, ids in multi_sites:
                f.write(f"Block {b} {where}: variable '{var}' may be defined by {', '.join(ids)}\n")
    print(f"Wrote {outname}")


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('Usage: python reaching_defs_interpretation.py --all  OR  python reaching_defs_interpretation.py file1.c file2.c')
        sys.exit(1)
    args = sys.argv[1:]
    if args[0] == '--all':
        files = sorted(glob.glob('*.c'))
    else:
        files = args
    for fn in files:
        if not os.path.isfile(fn):
            print(f"Skipping {fn}: not found")
            continue
        analyze_and_write(fn)
