#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""The longest win on every board of the 3-1 table, with the analyses along its main line, stored in longest31.json.gz for
the site (building the 3-1 databases of all 131 boards and colourings takes about half an hour). Resumable: boards
already in the file are skipped.

usage: longest31.py [path/to/egdb_fast]
"""

import gzip
import importlib.util
import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / 'common'))
from sitekit import CachedEngine, FastEngine   # noqa: E402

EXE = sys.argv[1] if len(sys.argv) > 1 else str(HERE.parent / 'build' / 'egdb_fast')
OUT = HERE / 'longest31.json.gz'

spec = importlib.util.spec_from_file_location('site31', HERE / 'site.py')
site = importlib.util.module_from_spec(spec)
spec.loader.exec_module(site)

data = json.loads(gzip.decompress(OUT.read_bytes())) if OUT.exists() else {}
for h, cells in site.RESULTS_31.items():
    for d, cell in enumerate(cells):
        parts = cell.split('/') if cell else []
        for k, part in enumerate(parts):
            w, n = h + d, int(re.sub(r'\D', '', part))
            corners = 'unplayable' if len(parts) == 2 and k == 0 else 'playable'
            key = f'{w}x{h} {corners}'
            if key in data or n == 0:          # 0: no position is won (3×3 unplayable corners)
                continue
            e = FastEngine(EXE, '--width', w, '--height', h, '--corners', corners, '--white', 3, '--black', 1)
            assert e.longest and e.analyse(e.longest)[0] == f'W{n}', (key, part, e.longest)
            data[key] = CachedEngine.entry(e, n)
            e.p.terminate()
            OUT.write_bytes(gzip.compress(json.dumps(data, separators=(',', ':'), sort_keys=True).encode(), mtime=0))
            print(key, f'W{n}', len(data), flush=True)
