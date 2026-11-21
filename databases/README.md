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
| `--target calibrate` | an instrumented build of every metric to `CALIBRATE_PIECES` (default `PIECES` − 1), as a profile-guided build: per material its positions, levels, frontier and seconds per pass, collected into `calibration.tsv` |
| `--target estimate` | space, I/O and time of the configured build on this machine, extrapolated from the calibration, with the RAM, the cores and the disk (`src/egdb_layout.py`) |
| `--target bench` | the quick alternative: the rules' move generation, branching, captures and promotions |

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
  uncompressed), plus the scratch of the `CORES` largest materials written side by side (the depth sink's group-order
  files; a file under its temporary name becomes the final one by a rename).
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

With `--cores N` (and `--rate`, positions per second per core) it also refines for parallelism: below the RAM limit
it halves the group limit until a list schedule on N cores reaches `--target` utilization (default 90 %), and reports
the makespan and critical path per step. Gilbert's 2009 8-piece draughts build (4×4, 5×3, 3×5; 8 GB; 10 cores; his
197,529 positions per second):

```bash
python3 src/egdb_layout.py --board 10x10 --pieces 8 --split 4-4,5-3,3-5 --ram 8GB --cores 10 --rate 197529 --kings-symmetry 2
```

| group limit | groups | makespan | utilization | critical path |
|---|---|---|---|---|
| 32 G (the coarsest that fits 8 GB) | 2,154 | 149 days | 65 % | 149 days |
| 16 G | 2,738 | 111 days | 87 % | 100 days |
| 8 G | 4,136 | 101 days | 96 % | 59 days |

The total work, 964 days on one core, matches his 2.65 CPU-years of building.

## Space, I/O and time for a machine

With `--builder` (`thompson86`, `syzygy` or `wdl`) the same command estimates the build on a machine: RAM per group
(`--ram`), cores (`--cores`) and their clock (`--ghz`), and the disk (`--disk`, `--disk-mbps`, `--tbw`):

* **space**: the files (depth bytes per metric, the outcomes) and the scratch of the jobs side by side (the depth
  sink's group-order files), against the disk; `--index overlapping` for the index the builders use today,
  `disjoint` (the default) for one slot per legal position;
* **I/O**: the writes (each file once, sequentially), the reads (each build's inputs, best and worst), and for groups
  that do not fit the RAM at the builder's cost the per-level streams of rung 3 (`--levels` per group); the disk time
  at `--disk-mbps`, and the writes against `--tbw`;
* **time**: the list schedule on `--cores`, at the builders' 6×6 cost per position and side (thompson86 0.35 µs,
  syzygy 0.68 µs, WDL 0.52 µs at 2.1 GHz) scaled by `--ghz`, or `--us` measured on the target board; with the disk
  time overlapped or added.

On 6×6 the estimate matches the builds: syzygy, three metrics, 404 MB of files with `--index overlapping` (the
configure step's figure), about 10 s per metric on 4 cores (measured: 11.8 s for dtm). For larger boards the 6×6 cost
is a floor (longer rays, flying kings, inputs out of the page cache): Gilbert's 2009 build ran at 5.06 µs per position
on 2.4 GHz cores, and with that `--us` the estimate gives about 114 days on 10 cores, for his 2.65 CPU-years.

```bash
python3 src/egdb_layout.py --board 8x8 --pieces 10 --ram 16GiB --builder wdl --cores 8 --ghz 4 --disk 4TB --disk-mbps 3000 --tbw 2400
```

**The cost per position depends on the game**, so the estimate measures it: `<engine> bench n` samples every material
and times the move generator, with the branching factor and the shares of captures and promotions (each a lookup into
another material's files). The measured builds fix the ratio of build cost to move generation (thompson86 1.23, syzygy
2.39 move generations per position and side), so a bench of other rules gives their cost. Same engine, same board, same
machine:

| 6×6, ≤ 6 pieces | international draughts | English checkers (`-DEGDB_6X6_CHECKERS=1`) |
|---|---|---|
| one move generation | 0.284 µs | 0.153 µs |
| moves per position | 4.07 | 3.44 |
| captures, promotions among them | 24.5 %, 5.3 % | 15.2 %, 8.4 % |
| thompson86, three metrics, 4 cores | 24 s | 14 s (a prediction: the unmoves still assume flying kings) |

`cmake --build <build> --target estimate` runs the bench and the estimate for the configured build.

**Every build is a calibration run.** As a profile-guided build, both engines measure themselves (a few popcounts per
round and timers around the passes) and end each material's log with a `stats` line: positions, groups, levels,
frontier positions summed over the rounds, seconds in pass 0, the rounds and the write. `src/egdb_calibrate.py`
collects a build's lines; `egdb_layout.py --calibration` fits per metric the cost per position against the pieces,
extrapolates it geometrically from the last two, and schedules each material at its own cost. Calibrated on the build
to 5 pieces, thompson86 dtm, predicted for the 6-piece materials against the build to 6 pieces: 0.40 against 0.419 µs
per position and side, about 36 against 40.5 s of CPU. The number of levels (passes) is an outcome of the build, the
longest win, and is not predicted: the frontier builds do not depend on it, and what costs a pass per level (rung 3)
is given as a range, from the calibration build's levels to what the byte holds.

## Machine files

A machine is a small file of `key: value` lines, so the estimate can be rerun for another machine, or for this one
with an upgrade: options given on the command line override the file.

| key | meaning |
|---|---|
| `name` | shown in the report |
| `cores`, `ghz` | jobs side by side and their clock |
| `ram`, `ram_per_group` | the machine's memory, and what one group may use (default 75 % of `ram`) |
| `disk`, `disk_kind`, `disk_mbps`, `tbw` | capacity, `hdd`/`ssd`/`nvme` (default speed 150/500/3000 MB/s), sequential speed, endurance in TB written |
| `us` | microseconds per position and side, when measured on that machine (Gilbert 2009: 5.06) |

`databases/machines/` holds Thompson's 1986 and 1996 machines, Gilbert's 2005 and 2009 build machines, and a desktop and
a workstation of 2026; the configure step writes this machine's as `<build>/machine.yaml`, which the `estimate` target
uses.

```bash
python3 src/egdb_layout.py --board 8x8 --pieces 10 --builder wdl --machine databases/machines/desktop-2026.yaml --cores 32
```

| 8×8 checkers ≤ 10 pieces, WDL | time | |
|---|---|---|
| `desktop-2026` (16 cores, 64 GiB, 4 TB NVMe) | 7.5 days | 10.3 TB of files (raw) do not fit 4 TB |
| `--ram 192GiB` | 7.5 days | RAM did not bind |
| `--cores 32` | 3.8 days | |
| `--disk 24TB` | 7.5 days | the files fit |
| `workstation-2026` (64 cores, 512 GiB, 32 TB) | 2.6 days | 34,605 groups keep 64 cores 91 % busy |

More RAM only raises the limit a group may have; the refinement for the cores (`--cores`) cuts the groups as fine as
the schedule needs, so an upgrade never makes the estimate worse.
