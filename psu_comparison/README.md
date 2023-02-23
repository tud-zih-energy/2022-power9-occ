# PSU Comparison
This directory contains data and scripts for comparing OCC-reported values to other sources.

## Experiment
- basic setup: run many different types of workload on many different levels
- measured: power consumption as reported by OCC, and as reported by PSU

Uses p9_longrun configuration from roco2.
Commits used:

- roco2: 189965c533abd94dbabe944de50f865da2ca53a9
- scorep ibmpowernv plugin: 3fd626813a68c8d87f04ce12cc3373176fecb613

Each configuration (workload, number of active threads) has its sensor readouts summarized and stored in `data/it01_run03.tsv`.

## Files
- raw results
  - `data/run03.tar.xz`: raw trace
  - `data/it01_run03.tsv`: *phase profile*, summarize each metric for each configuration (workload, number of active threads)
    - remove first and last 10% of configuration duration, compute mean across remaining samples
- PSUs input vs OCC bulk for plotting: `plots/phase_profile`
  - `prepare.R`: extract values from phase profile, dump samples and fit
  - `input_vs_occ_reported_sum.dat`: coordinates of samples
  - `input_vs_occ_reported_sum_fit.dat`: coordinates of fit, used to draw line
- PSUs input vs OCC bulk for efficiency approximation:
  - `plots/fits/compute_errors.R`: compute errors for different regressions for PSU efficiency (linear, quadratic)
- compare OCC-provided sum vs recalculated OCC sum: `plots/occ_sums`
  - `prepare.R`: extract data from phase profile, recalculate sums
  - `occ_sums.dat`: samples with OCC-reported and recalculated sum
    - for each sample includes the experiment identifier (`kernel`) and if that kernel uses memory (`uses_mem`)
- sankey plots/power distribution scheme: `plots/domains`
  - `flake.lock`, `flake.nix`: nix files to provide environment where the ggsankey R module is available
  - `sankey.R`: plot itself, automatically picks the configuration with the highest power total power consumption
  - `domains.tex`: built sankey plot as tikz/latex source (for standalone PDF)
- other files
  - `README.md`: this file
