# Example data

Synthetic but realistic measurements, to try polarSA without hardware. Every file says so in its metadata.

| File | Content |
|---|---|
| `session_antenna_868MHz.json` | Config: 868 MHz, 2 MHz span, 10 kHz RBW, 5° steps |
| `antenna_revA_unmatched.csv` | 868.1 MHz LoRa carrier, PCB antenna before matching: two lobes, deep nulls |
| `antenna_revB_matched.csv` | Same board after matching: about 5 dB more |
| `antenna_whip_reference.csv` | Quarter-wave whip reference: nearly omnidirectional |
| `session_emc_30-1000MHz.json` | Config: 30 MHz - 1 GHz, 850 kHz RBW, 15° steps, with the calibration and the EN 55032 Class B 3 m limits |
| `calibration_example_3m.csv` | **Fictitious** setup calibration (107 + antenna factor + cable loss): replace it with your own |
| `emc_scan_peak.csv` | Sensor board emissions: 48 MHz clock harmonics with their own directivity, switching-supply noise |
| `emc_scan_quasi_peak.csv` | Quasi-peak re-measurement around 144 and 240 MHz: the 144 MHz harmonic fails in peak, passes in quasi-peak |
| `example_report.pdf` | PDF report of the EMC example |

Import a config with **File → Import Config...**, then the measurements with **File → Import Trace(s)...**.
