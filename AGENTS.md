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

## Repository organization — owner decision 2026-11-28

Use native GitHub labels for topic, purpose, target scope and state. Use milestones for coherent major deliverables with explicit acceptance criteria, not individual minor steps or artificial deadlines. A PR/issue has one primary milestone; express cross-cutting concerns with labels. Preserve independent risk and validation gates; never treat milestone completion as proof of runtime correctness. Do not close or merge work automatically to improve milestone progress.

### Initial milestone assignment plan (not yet provisioned)

Refresh the actual open PR/issue state and existing milestones first; reuse exact-title milestones and preserve existing assignments unless reviewed. The GitHub connector cannot create milestones; a local agent with authorized GitHub CLI/API access may provision and assign the following, without due dates. Exclude superseded reference-only PRs and deferred items from active delivery milestones. Do not duplicate or reopen merged PRs.

| Milestone | Initial open candidates | Completion gate |
|---|---|---|
| GFX — Renderer Abstraction | PR #94, #96 | Remaining GFX 13a/13b abstraction and resource ownership validated; no unresolved required native escapes. |
| GFX — DX11 Migration | PR #95 | Compatible playable DX11 backend, GFX 13c–13e, game/editor and compatibility gates passed. |
| GFX — SIM/Render Separation | PR #121, #122, #123 | GFX 30–32 presentation snapshot, interpolation and render separation validated; simulation/network/replay determinism preserved. |
| PERF — Engine Optimization | PR #32, #34, #104, #106, #108–#114 | Accepted measured optimization candidates pass independent correctness and performance gates; OPT-D deferred, not required for this milestone. |
| INI — Validation System | Issues #53–#65, #68; PR #76, #78 | IIV/MIV contract, provenance, supported rule coverage and integration fixtures verified. Runtime behavior fixes #66 and optional cache #67 remain separately gated. |
| NET — Network Stability | Issues #1, #3 | Pacing, LAN discovery and slot stability verified with reproducible multiplayer tests. |
| SIM — Determinism | Issues #2, #4, #5 | RNG and CRC diagnostics, faction pool consistency and deterministic regression gates verified. |

Keep #33 (weapon-slot feature), #6 (faction expansion) and #69 (lobby UI) independently tracked until a concrete feature milestone is useful. Keep #71/#75 and OPT-D #37/#51/#52 deferred; #77, #93/#100, #118 and original OPT component references are not extra delivery work. When provisioning, verify every assignment by reading it back and report any failure. This table is a proposed mapping, not a claim of actual milestone creation or assignment.

### GFX issue tracking

Use #124 (Renderer Abstraction), #125 (DX11 Backend), #126 (Performance Optimization, deferred remainder), #127 (Visual Improvements, deferred), #128 (SIM/Render Separation) as umbrella issues. Add #124, #125 and #128 to their respective planned GFX milestones when provisioned; leave #126 and #127 outside active migration milestones until reprioritized. Link PRs without automatic issue-closing keywords; close only when all mandatory acceptance criteria are verified. Preserve DX11-first policy and independent review gates.
