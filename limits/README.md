# EMC limit files

Load them with **Corrections & limits → Limits → ...** in polarSA. They are drawn as dashed lines on the
Spectrum and Max Emissions graphs, and as dashed circles on the radiation pattern (the limit value at the
selected frequency, interpolated on sloped segments).

Limits are only compared to the measurement when the units match: the shipped files are in **dBµV/m**, so a
setup calibration converting dBm to dBµV/m must be loaded (see `../calibration/README.md`). Without it the
lines are still drawn on the spectrum graphs, but no margin is computed.

## Shipped files

| File | Regulation | Distance |
|---|---|---|
| `EN55032_ClassB_3m.csv` | EN 55032 / CISPR 32 Class B (residential) | 3 m (below 1 GHz: 10 m limits + 10 dB) |
| `EN55032_ClassB_10m.csv` | EN 55032 / CISPR 32 Class B, 30 MHz – 1 GHz | 10 m |
| `EN55032_ClassA_3m.csv` | EN 55032 / CISPR 32 Class A (non-residential) | 3 m (below 1 GHz: 10 m limits + 10 dB) |
| `EN55032_ClassA_10m.csv` | EN 55032 / CISPR 32 Class A, 30 MHz – 1 GHz | 10 m |
| `FCC_Part15_109_ClassB_3m.csv` | FCC 47 CFR 15.109(a) Class B | 3 m |
| `FCC_Part15_109_ClassA_10m.csv` | FCC 47 CFR 15.109(b) Class A | 10 m |
| `FCC_Part15_109_ClassA_3m_extrapolated.csv` | FCC 15.109(b) Class A, extrapolated to 3 m (+10.5 dB) | 3 m (not a published table) |
| `FCC_Part15_209_3m.csv` | FCC 47 CFR 15.209 general limits (intentional radiators, out-of-band) | 3 m |

Each file holds several limit lines (quasi-peak below 1 GHz, average and peak above 1 GHz). Above 1 GHz the
lines stop at 6 GHz, the top of the tinySA Ultra range. polarSA measures with the tinySA's peak detector: a
peak reading below a quasi-peak or average limit is conservative, a peak reading above it is not necessarily a
failure.

These values are given for **pre-compliance** only. Check them against the edition of the standard that
applies to your product (limits, detectors, frequency ranges, measurement distance, product-specific rules).

## File format

```
# name,EN 55032 / CISPR 32 Class B radiated, 3 m     <- shown in the panel and the report
# unit,dBuV/m                                        <- unit of the levels
# description,free text
limit,start_MHz,stop_MHz,start_level,stop_level      <- header
Quasi-peak,30,230,40,40
Quasi-peak,230,1000,47,47
Average,1000,3000,50,50
```

- One row per straight segment. Rows with the same `limit` name form one line (one color, one legend entry).
- Frequencies are in the unit of the header: `start_Hz`, `start_kHz`, `start_MHz` or `start_GHz`.
- `stop_level` may differ from `start_level` for a sloped segment (interpolated linearly in frequency), or be
  left empty for a flat one. Example: `Sloped example,150,500,50,40`.
- At a frequency where two segments meet (e.g. 230 MHz above), the lower (stricter) value is used.
- `,` `;` or tab separators are accepted; with `;` or tab, `,` is accepted as decimal mark.
