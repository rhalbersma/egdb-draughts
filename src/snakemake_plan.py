#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""The plan of a per-material build, shared by the Snakefiles of thompson86 and syzygy (synthesis section 7.3).

Each engine reports, per material, its sizes and the memory a build needs at each rung of the memory ladder (its
`plan` command, estimated from its own data structures and calibrated against measured peaks). From that:

  - the memory budget: `--resources mem_mb=N` when given (Snakemake then never runs jobs whose claims exceed it
    together), else `config mem_mb`, else 75 % of the physical memory (the rest for the page cache of finished files);
  - a job's claim: its rung-0 need if that fits the budget, else the whole budget, so that a big material waits for
    the whole machine instead of squeezing other jobs, and the engine takes the highest rung that fits its claim;
  - a job's priority: the critical path, its own work plus the longest chain of jobs that need it (0 to 1000), so that
    the long chains of the DAG start first.
"""

import functools
import subprocess


def memory_budget_mb(workflow, config):
    if 'mem_mb' in workflow.global_resources:
        return int(workflow.global_resources['mem_mb'])
    if 'mem_mb' in config:
        return int(config['mem_mb'])
    with open('/proc/meminfo') as f:
        total_kb = next(int(line.split()[1]) for line in f if line.startswith('MemTotal:'))
    return int(total_kb / 1024 * 0.75)


def load_plan(command):
    """material name -> slots, positions, groups, max_group, rung0, rung1 (MB), from the engine's `plan` command"""
    out = subprocess.run(command, capture_output=True, text=True, check=True).stdout
    plan = {}
    for line in out.splitlines():
        if not line or line.startswith('#'):
            continue
        f = line.split('\t')
        plan[f[0]] = dict(slots=int(f[1]), positions=int(f[2]), groups=int(f[3]), max_group=int(f[4]),
                          rung0=int(f[5]), rung1=int(f[6]))
    return plan


def claim_mb(p, budget):
    """rung 0 if it fits the budget, else the whole budget (the engine then takes a lower rung if it has one)"""
    return p['rung0'] if p['rung0'] <= budget else budget


def priorities(plan, deps):
    """critical path per material: its positions plus the longest chain of materials that need it, scaled to 0..1000;
    deps: material name -> the names of the materials it needs"""
    users = {m: [] for m in plan}
    for m, ds in deps.items():
        for d in ds:
            if d in users and m in plan:
                users[d].append(m)

    @functools.lru_cache(maxsize=None)
    def rank(m):
        return plan[m]['positions'] + max((rank(u) for u in users[m]), default=0)

    top = max((rank(m) for m in plan), default=1) or 1
    return {m: int(1000 * rank(m) / top) for m in plan}


def report(plan, budget, claim):
    """one line for the log: how many materials fit at rung 0, how many need a lower rung, the largest needs"""
    fit = sum(p['rung0'] <= budget for p in plan.values())
    lower = sum(p['rung0'] > budget >= p['rung1'] for p in plan.values())
    over = sum(p['rung1'] > budget for p in plan.values())
    big = max(plan, key=lambda m: plan[m]['rung0'])
    return (f'plan: {len(plan)} materials, budget {budget} MB: {fit} at rung 0, {lower} at rung 1, {over} over the budget '
            f'even at rung 1; largest {big} (rung 0 {plan[big]["rung0"]} MB, rung 1 {plan[big]["rung1"]} MB, claim '
            f'{claim(plan[big])} MB)')
