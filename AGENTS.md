# Agent instructions for this fork

Read [CONTRIBUTING.md](CONTRIBUTING.md) and [AI_POLICY.md](AI_POLICY.md).
The following project-owner-approved planning rule applies to graphics (GFX)
work in this fork. It does not replace upstream contribution policies.

## GFX priority: DX11 first

See [the GFX roadmap](docs/gfx-roadmap.md) for stage dependencies and review gates.

Decision recorded 2026-11-27. Until the project has a stable, validated DX11
renderer, prioritize the GFX 13 roadmap (renderer abstraction, resource
abstraction, and DX11 backend) over non-urgent DX8-specific optimizations.

Before proposing or implementing a GFX task, check current main, existing
issues/PRs, dependencies, and available profiling results.

- Implement tasks required for the DX11 transition first.
- Fix critical crashes, correctness defects, and severe visual or performance
  regressions when they cannot reasonably wait.
- Consider optimizations that benefit both backends individually, based on
  evidence, migration relevance, complexity, and regression risk.
- Defer non-urgent optimizations that are specific to DX8, are likely to be
  replaced during migration, or lack a demonstrated performance benefit.
- Preserve existing diagnostic instrumentation and baseline measurements.
  Keep existing diagnostic evidence. Do not resume PR #71 or #75 until a critical defect, severe regression, or demonstrated migration dependency requires it, unless the owner explicitly reprioritizes them.
- Keep deferred tasks in the roadmap; do not treat deferral as cancellation.
  Re-profile representative workloads once DX11 is stable, compare against
  DX8 baselines, and reassess the necessity, scope, benefit and risk before
  implementation. The owner can explicitly reprioritize an individual task.

### Current roadmap decisions

| Stage | Decision |
| --- | --- |
| GFX 09b.1 (PR #75) | Deferred until critical need; preserve open PR and baseline. |
| GFX 09b.2–09b.6 | Defer shadow geometry batching, cross-stencil-pass reuse, binding/draw-call optimizations and their comparative optimization testing until DX11, unless an urgent need or migration dependency is demonstrated. |
| GFX 10a (PR #71) | Deferred until critical need; preserve open PR and existing diagnostic work. |
| GFX 10b–10c | Defer debris optimization unless urgent or required for migration. |
| GFX 11d | Defer further particle worker optimization until evidence supports it. |
| GFX 12c | Defer static shadow parallelization until new measurements justify it. |
| GFX 13a–13e | Primary implementation priority: renderer interfaces, resource abstraction, DX11 device and compatible backend. |

Evidence: [PR #30](https://github.com/prnnsn/GeneralsGameCode/pull/30),
[PR #70](https://github.com/prnnsn/GeneralsGameCode/pull/70),
[PR #75](https://github.com/prnnsn/GeneralsGameCode/pull/75).
The recorded DX8 profiling motivates future optimization but is not proof
that the same bottlenecks or savings will apply to DX11.

### Review and compatibility

Keep graphics migration separate from gameplay simulation. Preserve legacy
visual behavior during the initial backend transition, original simulation
tick behavior, deterministic RNG/CRC, network/replay compatibility, and
existing INI/assets and World Builder compatibility. Do not claim compatibility
or speedups without the corresponding tests. Keep changes focused and
independently reviewable, report actual test results and limitations, and do
not merge changes without owner authorization.
