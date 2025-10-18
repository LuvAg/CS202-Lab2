"""
Generate per-block reaching-definitions summary as CSV.
Outputs: reaching_defs_summary_<program>.csv with columns:
  Basic-Block, gen[B], kill[B], in[B], out[B]

Usage:
  python reaching_defs_csv.py <file.c> [more.c ...]
  python reaching_defs_csv.py --all

This script reuses the analysis helpers in `reaching_defs.py` and `cfg_builder.py`.
"""
import sys, glob, os, csv
import cfg_builder as cb
import reaching_defs as rd


def set_to_str(s):
    # join defs in sorted order with semicolon for compact CSV cell
    return ';'.join(sorted(s)) if s else ''


def process_file(fn):
    base = os.path.splitext(os.path.basename(fn))[0]
    # build blocks and cfg
    lines = cb.read_c_code(fn)
    leaders = cb.find_leaders(lines)
    blocks = cb.form_basic_blocks(lines, leaders)
    cfg = cb.build_cfg(blocks)

    # find defs and compute gen/kill
    defs, defs_by_block = rd.find_definitions_in_blocks(blocks)
    gen, kill = rd.build_gen_kill(blocks, defs, defs_by_block)

    # iterative reaching
    history, IN, OUT = rd.iterative_reaching(cfg, blocks, gen, kill)

    csv_name = f"reaching_defs_summary_{base}.csv"
    with open(csv_name, 'w', newline='') as cf:
        w = csv.writer(cf)
        w.writerow(['Basic-Block', 'gen[B]', 'kill[B]', 'in[B]', 'out[B]'])
        for b in blocks.keys():
            w.writerow([b, set_to_str(gen.get(b, set())), set_to_str(kill.get(b, set())), set_to_str(IN.get(b, set())), set_to_str(OUT.get(b, set()))])

    print(f"Wrote {csv_name}")
    return csv_name


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('Usage: python reaching_defs_csv.py <file.c> [more.c ...]  OR  python reaching_defs_csv.py --all')
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
        process_file(fn)
