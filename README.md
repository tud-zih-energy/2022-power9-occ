# POWER9 IBM OCC Artifacts
This repository contains artifacts for reproducing [Evaluating the Energy Measurements of the IBM POWER9 On-Chip Controller](https://doi.org/10.1145/3578244.3583729).

Hannes Tröpgen, Mario Bielert, and Thomas Ilsche. 2023. Evaluating the Energy Measurements of the IBM POWER9 On-Chip Controller. In Proceedings of the 2023 ACM/SPEC International Conference on Performance Engineering (ICPE ’23), April 15–19, 2023, Coimbra, Portugal. ACM, New York, NY, USA, 10 pages. https://doi.org/10.1145/3578244.3583729

To carry out the experiments shown here you need:

- [roco2](https://github.com/tud-zih-energy/roco2/) (GPL-3.0)
- [Score-P IBM PowerNV Plugin](https://github.com/score-p/scorep_plugin_ibmpowernv) (BSD-3-Clause)

both available under open-source licenses.

The subdirectories contain:

- `green500`: data on Green 500
  - used for Fig. 1
- `psu_comparison`: comparing the OCC-reported sensor values to PSU-reported sensors
  - used for Fig. 3, Fig. 4, Fig. 5
- `sampling_frequency_external_interface`: measure overhead, update rate of interface (OCC, hwmon)
  - used for Fig. 2
- `sampling_frequency_internal_accumulator`: infer internal sampling rate through aliasing
  - used for Fig. 7, Fig. 8, Tab. 3

## License
All data in this repository is dual-licensed, i.e., available under

- [Creative Commons Attribution 4.0 International](./LICENSE.CC-BY-4.0) (`CC-BY-4.0`) **or**
- [BSD 3-Clause "New" or "Revised" License](./LICENSE.BSD-3-Clause) (`BSD-3-Clause`)
