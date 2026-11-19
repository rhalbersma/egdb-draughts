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
| `CORES` | the logical cores | Snakemake's `--cores`: jobs side by side |
| `MEM_MB` | 75 % of physical memory | the memory budget of the jobs together, in MiB |
| `SNAKEMAKE_EXECUTABLE` | found on the `PATH` | Snakemake 8 |

Sizes are MiB (2²⁰ bytes) throughout, as Snakemake's `mem_mb` and the engines' `plan`; the configure step prints them
scaled to MiB, GiB or TiB, e.g. "needs 96.6 GiB (91.2 GiB of files, ...)".

**A build fails when the machine provides less than it requires:**

* **disk**, at configure time, exactly. Every file's size follows from its slot count, a product of four binomials, so
  no engine is needed. The need is the raw size: all files uncompressed, plus the temporary copies while the largest
  material is written.
* **memory**, when Snakemake reads the workflow, before any job runs. The engine's `plan` gives every material's need
  per rung; if one needs more than `MEM_MB` even at its lowest rung, the workflow stops and names the largest. Give
  more memory, a finer granularity, or `config mem_check=0` to run over the budget anyway. This check lives in the
  Snakefiles (`check_budget` in `src/snakemake_plan.py`), so it also applies without CMake.

Snakemake runs from the build directory (its `.snakemake` metadata goes there). `snakemake --profile
<build>/profile --config metric=dtz` does the same as `cmake --build <build> --target dtz`. A second `cmake --build`
finds every file up to date and does nothing.

The engine's header-only dependencies (xstd-bits, xstd-ints, xstd-misc) are fetched from GitHub; to use local
checkouts, pass `-DFETCHCONTENT_SOURCE_DIR_XSTD-BITS=<path>` and likewise for the other two.
