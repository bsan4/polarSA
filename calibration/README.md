# Setup calibration files

A setup calibration turns the level measured by the tinySA (dBm) into the quantity you want to read, usually
the field strength at the antenna in **dBµV/m**, so it can be compared to EMC limits. Load it with
**Corrections & limits → Calibration → ...** in polarSA.

It is optional: without it, levels are shown in dBm at the analyzer input. Either way, the report and the
exported CSV files state which calibration was active ("none" when there is none).

## What is applied

```
level shown = level measured by the tinySA (dBm) - external gain (dB) + correction(f) (dB)
```

- **External gain** (spin box above the calibration): gain between the antenna and the tinySA, constant in
  frequency: `+20` for a 20 dB preamplifier, `-6` for a 6 dB attenuator. It is removed from every sweep when it
  is captured, and recorded in the capture metadata (`ext_gain_dB`).
- **Correction(f)**: this file, interpolated linearly in frequency (edge values are used outside the table).
  It is applied for display, analysis and reports only: exported CSV data stay in dBm, so a measurement can be
  re-read later with another calibration.

For field strength in dBµV/m (50 Ω system):

```
correction(f) = 107 + AF(f) + cable loss(f)
```

with `107` = dBm → dBµV in 50 Ω, `AF` the antenna factor in dB/m (from the antenna calibration sheet), and the
cable loss in dB (positive number). A frequency-dependent preamplifier can be included here too
(`- preamp gain(f)`), keeping the external gain at 0.

## File format

```
# unit,dBuV/m                                  <- unit of the corrected levels (default: dB)
# description,LPDA XYZ + 3 m RG58, 2026-11     <- free text, shown in the panel and the report
frequency_MHz,correction_dB                    <- header: frequency_Hz / _kHz / _MHz / _GHz
30,125.2
100,118.6
300,121.9
1000,131.0
```

`TEMPLATE_calibration.csv` is a neutral starting point (0 dB everywhere, unit dBm): copy it, fill in your
setup's values and set the unit.
