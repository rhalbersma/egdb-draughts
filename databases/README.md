# A CMake shell on top of Snakemake

`CMakeLists.txt` here configures and builds the 6×6 endgame databases of [`thompson86`](../thompson86/) or
[`syzygy`](../syzygy/) with CMake's two steps. Snakemake still schedules the builds; CMake checks the machine, compiles
the engine and writes the Snakemake profile (synthesis §7.3, "choosing by machine").

```bash
cmake -S databases -B build/syzygy-6 -DBUILDER=syzygy -DPIECES=6
```

```bash
cmake --build build/syzygy-6
```

```bash
cmake --build build/syzygy-6 --target dtm
```

| step | what it does |
|---|---|
| configure | reads the machine (physical and logical cores, total and available memory, free disk of `DATA_DIR` and whether it is rotational), computes the disk the build needs, **fails if the disk is too small**, creates the tree `<DATA_DIR>/<metric>/6x6/<np>/<wp>-<bp>/`, and writes `machine.yaml`, the Snakemake profile `profile/config.yaml` and the workflow's values `egdb.yaml` |
| `--build` (target `databases`, the default) | compiles the engine, then runs Snakemake for every metric, one after another, each on the whole machine |
| `--target dtm` (or `dtc`, `dtz`) | one metric |
| `--target plan` | Snakemake's dry run: the memory plan and the jobs that would run |
| `--target machine` | prints `machine.yaml` |

**Options** (`-D<name>=<value>`):

| option | default | |
|---|---|---|
| `BUILDER` | `syzygy` | `thompson86` (one-sided, half results) or `syzygy` (two-sided, full results) |
| `PIECES` | 6 | at most this many pieces |
| `METRICS` | `dtm;dtc;dtz` | each into `<DATA_DIR>/<metric>` |
| `GRANULARITY` | `material` | `material`, `tempi` or `configuration` |
| `POLICY` | `seq` | every pass of a build sequential or parallel (`par`) |
| `SINK` | `auto` | `auto`, `groups` or `array` |
| `STYLE` | `86` | thompson86 only: `86` or `96` |
| `ROUNDS` | `frontier` | syzygy only: `frontier` or `scan` |
| `DATA_DIR` | `<build>/data` | where the databases go |
| `DISK_MBPS` | 150 for an HDD, 500 otherwise | the disk's sequential speed, for the I/O time |
| `DISK_TBW` | none | an SSD's rated endurance in TB written, to put the build's writes against |
| `CORES` | the logical cores | Snakemake's `--cores`: jobs side by side |
| `MEM_MB` | 75 % of physical memory | the memory budget of the jobs together, in MiB |
| `SNAKEMAKE_EXECUTABLE` | found on the `PATH` | Snakemake 8 |

Inputs and checks are in MiB (2²⁰ bytes), as Snakemake's `mem_mb` and the engines' `plan`. The configure step reports
sizes as they are sold: memory in binary units, MiB or GiB (a "16 GB" module is 16 GiB; `free -h`), disk in decimal
ones, MB, GB or TB (a "1 TB" disk is 10¹² bytes; `df -H`), e.g. "memory: 15.7 GiB" and "needs 103.8 GB".

**Besides space, configure reports the I/O** (synthesis §7.4): the writes (every file once, sequentially; up to the
depth sink's group-order files on top), the reads (each build's inputs through memory maps, at best each input once,
at worst once per build that reads it, plus the checks), the disk time at `DISK_MBPS`, and with `DISK_TBW` the writes
against the SSD's endurance. For `syzygy` to 6 pieces, all metrics: "writes 404 MB (all sequential; up to 1.1 GB with
group files), reads 590 MB to 1.5 GB; at 150 MB/s: 6 to 17 s of disk time". The volumes are those of rungs 0–2, where
the builders run; at the lower rungs the per-level streams and random accesses of §7.4 come on top.

**A build fails when the machine provides less than it requires:**

* **disk**, at configure time, exactly. Every file's size follows from its slot count, a product of four binomials, so
  no engine is needed. Disk is cumulative: the need is every file of every metric (the databases are permanent, raw:
  uncompressed), plus the scratch of the `CORES` largest materials written side by side (their files under temporary
  names and the depth sink's group-order files).
* **memory**, when Snakemake reads the workflow, before any job runs. Memory is scratch only: a job's working set and
  the touched pages of the finished files it maps, freed when the job ends; Snakemake keeps the claims of the jobs side
  by side within `MEM_MB`. The engine's `plan` gives every material's need
  per rung; if one needs more than `MEM_MB` even at its lowest rung, the workflow stops and names the largest. Give
  more memory, a finer granularity, or `config mem_check=0` to run over the budget anyway. This check lives in the
  Snakefiles (`check_budget` in `src/snakemake_plan.py`), so it also applies without CMake.

Snakemake runs from the build directory (its `.snakemake` metadata goes there). `snakemake --profile
<build>/profile --config metric=dtz` does the same as `cmake --build <build> --target dtz`. A second `cmake --build`
finds every file up to date and does nothing.

The engine's header-only dependencies (xstd-bits, xstd-ints, xstd-misc) are fetched from GitHub; to use local
checkouts, pass `-DFETCHCONTENT_SOURCE_DIR_XSTD-BITS=<path>` and likewise for the other two.

## What a RAM budget does to the DAG

`src/egdb_layout.py` answers "with this much RAM per group, what do the groups and their DAG look like?", for any
board (6×6, 8×8, 10×10) and without building anything:

```bash
python3 src/egdb_layout.py --board 8x8 --pieces 10 --exact --ram 900MB --cost gilbert2005 --materials
```

Per material it takes the coarsest granularity that fits, refined only where needed: the whole material; else its tempi
cells, packed with the other cells of the same total tempo up to the budget (they are independent); a cell that still
does not fit becomes packs of its configurations. It reports the groups per granularity, what does not fit, the floor
(1 bit per position of the largest configuration of men with its king placements; kings-only materials by their
4-fold symmetry), and the DAG: levels (the critical path in groups) and width (groups that could run side by side).

Ed Gilbert's 2005 build of the 10-piece English checkers database (2 GB machines, a 900 MB threshold per subdivision,
2 bits per position) for comparison:

| exactly 10 pieces, 2 bits per position | Gilbert 2005 | `egdb_layout.py --ram 900MB` |
|---|---|---|
| groups | 64,369 subdivisions (three leading-rank tiers) | 16,506 (packed tempi cells and configurations) |
| buildable at once | "about 50 at a time on average, above 100 in the middle months" | mean 49, max 274 |
| all-kings 5v5 | "almost 4 GB of buffer", halved by one diagonal to fit 2 GB | 16.3 G positions, 3.8 GiB; ÷2 1.9 GiB, ÷4 969 MiB |
