# polarSA

![alt text](https://github.com/bsan4/polarSA/blob/b2d26cbe5efdd30f7a57d8f71b692da966fb4316/rsc/polarsa.svg)

**polarSA** is a measurement and visualization tool for RF engineers working with the **tinySA Ultra spectrum analyzer** and a **rotating DUT platform** to characterize antenna radiation patterns and/or intentionnal and unintentional radiations.

The software records spectra while the DUT rotates and produces **polar radiation diagrams**, **maximum-hold emission spectra**, and other useful visualizations for RF analysis and pre-compliance work.

The goal of polarSA is :
 - to provide antenna radiation pattern automated measurement for hardware debugging
 - to provide functionality similar to what EMC labs do during compliance testing, but with **portable and low-cost equipment** (for pre-compliance or EMC fixes)

![alt text](https://github.com/bsan4/polarSA/blob/b2d26cbe5efdd30f7a57d8f71b692da966fb4316/rsc/example_radiation.png)

---

# Features

### Polar Radiation Pattern Measurement
Measure antenna radiation patterns by rotating the DUT and capturing the signal level over angle.

Features:

- Polar radiation diagrams
- Frequency selection cursor
- Real-time measurement display

---

### Max-Hold Emission Spectrum

During rotation, polarSA records (amongs other data) the **maximum power measured at each frequency over all angles**.

This produces a spectrum representing **worst-case emissions**, similar to what EMC labs measure when rotating a DUT during compliance testing.

Benefits:

- More useful for EMC analysis than a single orientation spectrum
- Helps identify worst-case emission frequencies
- Fast overview of DUT emissions

Note that the radiation data can be browsed after the record using the frequency cursor, allowing analysis of the worst angles as well

QPD approximation (non-CISPR, but close enough) planned 2026-Q2 using tinySA Ultra time-domain, single-frequency mesurements and computed charge/discharge.

---

### Spectrum vs Angle Data Capture

polarSA stores the **entire spectrum for every angular position**.

This enables:

- Dynamic radiation pattern extraction
- Post-processing
- Measurement replay
- Cursor-based pattern visualization

Example workflow:

1. Run a scan while the DUT rotates 360°
2. Store all spectra
3. Select a frequency
4. Display the radiation pattern for that frequency

---

### Capture Modes

- **Single**: one sweep at the current rotator position.
- **Loop Single**: continuous sweeps until **Stop**; spectrum analyzer settings changed meanwhile are applied on the fly.
- **Sequence**: one sweep per angle from start to end angle.
- **Loop Sequence**: repeats the sequence until **Stop**, alternating the rotation direction (start→end, end→start...) so the turntable never travels back across the whole range. Each pass replaces the live data.

Capture buttons are disabled while a capture runs; **Stop** is only enabled then.

---

### User Interface

- **Menus**: **File** (Import Trace(s), Export Trace, Export Quasi-Peak, Export PDF Report, Import / Export Config, Import Calibration, Import Limits, Quit), **Edit** (Clear Imports, Clear Graphs: removes the live capture and keeps the imports, Clear Quasi-Peak, Clear Calibration, Clear Limits) and **Help** (About).
- **Tabs**: Spectrum (last sweep), Radiation Pattern (polar plot at the frequency cursor), Max Emissions (max hold over all angles). Under the Max Emissions graph, the **Autoscale on update** boxes (X, Y, XY) rescale the axes on every update, and the **Rescale now** buttons rescale once; Y fits the data inside the displayed frequency range, so you can zoom in frequency and then rescale Y. Uncheck autoscale to keep a zoom while data keeps updating.
- **Right panel**: spectrum analyzer and level settings, rotator angles, capture buttons and the Traces list. The splitter between the graphs and the panel can be dragged.
- **Status bar**: the config, calibration and limits in use (hover for details).

---

### Quasi-Peak Measurements

Normal captures use the tinySA's fast raw sweep (peak). For emissions close to a limit, **Capture → Quasi-Peak...** measures a narrow frequency window with the tinySA's quasi-peak detector, the way EMC labs re-measure suspect peaks:

- The window is set by center / span or start / stop, with the number of points (450 max) and the **sweeps per angle**. The center defaults to the frequency selected with the cursor (e.g. after **Peak**). RBW, attenuation and LNA come from the Spectrum Analyzer panel (CISPR 16 uses 120 kHz from 30 MHz to 1 GHz; the closest tinySA RBW is 100 kHz).
- The window is measured at every rotator angle (Rotator start / end / step). At each angle the detector is restarted from a raw sweep, then the quasi-peak sweeps are run and the last one is kept.
- Each window is added to a separate **Quasi-peak** trace (Traces list): successive windows build a sparse spectrum, and a window overlapping a measured one replaces its points. It is drawn with markers on Max Emissions, shows on the radiation pattern at its frequencies, is compared to the limits and appears in reports. A stopped window is discarded, so every angle keeps the same frequency points.
- **File → Export Quasi-Peak...** saves it as a regular polarSA CSV (sparse frequency columns), which can be imported again; **Edit → Clear Quasi-Peak** removes it (**Clear Imports** keeps it).

The tinySA's quasi-peak (trace calculation `calc quasi`) charges instantly and discharges slowly over successive sweeps (decay of 20 sweeps by default, set on the device): it approximates, but is not, a CISPR 16 quasi-peak detector. More sweeps per angle bring the value closer to the settled one. It is reached over USB with the `scan` command (the raw `scanraw` sweep bypasses the trace calculation), so it is slower than a normal capture.

---

### CSV Export / Import and Pattern Comparison

**File → Export Trace...** first asks for measurement notes (DUT, antenna, matching, polarization, operator, free comments; remembered between exports), then saves the last sequence capture (every spectrum at every angle) to a CSV file:

```
# polarSA measurement export
# dut,Sensor board rev B        <- user notes
# matching,pi 2.2nH / 1.5pF / NC
# center_MHz,100.0000            <- capture settings (informative)
angle_deg/freq_Hz,95000000,...   <- frequencies in Hz
0,-82.50,...                     <- one row per angle, power in dBm
10,-80.13,...
```

**File → Import Trace(s)...** loads one or several of these files and overlays their radiation patterns on the polar plot, at the frequency selected by the cursor (nearest frequency bin; a file whose range does not include that frequency is not drawn). The frequency cursor spans the frequencies of all traces (live capture and imports), so every trace stays reachable even when they were measured in different bands. **Center** jumps to the middle of the live capture (or of the first visible import), **Peak** to the frequency of maximum power over all visible traces and angles. Colors are assigned in import order: red, yellow, cyan, then a set of distinct colors (green, orange, blue, magenta...), then random. In the **Traces** list, the checkbox shows/hides a trace (the live capture included). Hovering a file shows its notes and settings. **Edit → Clear Imports** removes all imported files.

**Normalized** shows every trace relative to its own maximum (0 dB).

Files using `;` or tab separators with a `,` decimal mark (e.g. re-saved by a French-locale spreadsheet) are accepted on import. Export, import, report and config file dialogs each remember their last folder.

---

### Trace Analysis and Adjustments

Double-click a trace (live capture or import) to open its dialog:

- **Analysis** at the selected frequency: maximum and its angle, minimum, max − min, average (power average over the angles), −3 dB beamwidth around the maximum (interpolated; "omnidirectional" when the pattern never drops 3 dB), front-to-back ratio (maximum vs the value 180° away), angular coverage. Imports are also compared to the live capture (difference of max and average).
- **Rotation**: rotates the whole pattern around the center, e.g. to align two antennas measured with a different orientation. **Max → 0°** aligns the maximum with 0°.
- **Gain offset** (dB): added to every value, e.g. to compensate a cable, an attenuator or a different reference level.
- **Color**, source file, frequency range and the file's **metadata**; **Remove trace** for imports.

Changes are previewed live; Cancel restores the previous settings. Adjustments are display settings: they are shown in the Traces list and the report, and **Export Trace** applies the live capture's adjustments to the exported data (recorded in its metadata).

---

### PDF Report

**File → Export PDF Report...** asks for notes (same fields as the export), then creates an A4 report: header with logo, date and config file name, notes and capture settings, radiation pattern at the selected frequency with an analysis table of the visible traces, Max Emissions plot, and the details / metadata of every trace. Plots are rendered with light colors for printing.

---

### Session Config Files

**File → Export Config... / Import Config...** stores the settings (no measurement data) in a JSON file: spectrum analyzer (center, span, RBW, attenuation, LNA), rotator (start, end, step), display (normalized) and the notes used to pre-fill the export / report dialogs. Configs also store the levels settings (external gain, unit, antenna factor) and the calibration / limits files in use. The config name is shown in the status bar and the window title (`*` when settings were changed since), and is written in the CSV metadata and the PDF report.

---

### Levels, Calibration and EMC Limits

Displayed level = tinySA level (dBm) − external gain + conversion. These are display corrections, applied to every trace (live and imported) and to Peak, analysis, limits and reports; data files always keep the tinySA levels in dBm, with the settings recorded in their metadata, so a measurement can be re-read later with other settings.

- **Levels → Ext. gain (dB)**: gain between the antenna and the tinySA (`+20` for a 20 dB preamplifier, `-6` for an attenuator or cable loss). Changing it updates the graphs at once.
- **Levels → Unit**: coarse conversion, flat in frequency, when no calibration file is loaded: dBm (none), dBµV (+107 dB, 50 Ω), dBµV/m (+107 dB + a flat antenna factor, **AF (dB/m)**), or dB (relative, label only).
- **File → Import Calibration...** (optional): a setup calibration file, correction in dB vs frequency, typically `107 + antenna factor + cable loss` to read field strength in dBµV/m. It replaces the unit conversion while loaded. Without it, the report and CSV metadata state `calibration: none`. Format and formula: [`calibration/README.md`](calibration/README.md); the template in `calibration/` converts dBm to dBµV.
- **File → Import Limits...**: an EMC limits file with one or several limit lines (e.g. quasi-peak, average, peak), each made of straight segments that may be sloped. They are drawn as dashed lines (with a legend) on the Spectrum and Max Emissions graphs, and as dashed circles on the radiation pattern: the limit value at the selected frequency, interpolated on sloped segments. The Max Emissions title shows the worst margin of the visible traces, and the PDF report gets a limit-check table (worst margin per trace and per limit). Limits are only compared when their unit matches the levels' unit: dBµV/m for the shipped files, from a calibration file or from the coarse dBµV/m unit with an antenna factor.

The `limits/` folder ships ready-to-use files for EN 55032 / CISPR 32 (Class A and B, 3 m and 10 m) and FCC Part 15.109 (Class A and B) / 15.209: see [`limits/README.md`](limits/README.md) for the list, the file format and how to write your own. They are for pre-compliance; check them against the edition of the standard that applies. The application looks for `limits/` and `calibration/` next to its executable (copy them there when deploying), then in the source tree.

The **Max Emissions** tab shows the max hold (over all angles) of the live capture and of every visible import, in the trace colors, with the worst margin to the limits.

Files exported by builds of polarSA where the external gain was removed from the data at capture time (`data_unit,dBm (analyzer input level minus external gain)`) are converted back to tinySA levels on import.

---

### tinySA Integration

polarSA communicates directly with **tinySA Ultra** over USB.

Capabilities:

- Configure sweep parameters
- Read spectrum data
- Continuous acquisition
- Synchronization with the rotating platform

---

# Hardware Setup

Typical measurement setup:
- TinySA Ultra connected through USB, equipped with an antenna and optionnaly an amplifier
- a custom rotating plate/turntable connected through USB (currently, a simple servomotor controlled by an arduino)
- a DUT radiating from the rotating plate

---

# Applications

polarSA is useful for:

- antenna radiation pattern measurements
- RF design validation
- EMC debugging
- EMI pre-compliance investigations
- directional antenna tuning
- RF leakage analysis

---

# Typical Workflow

1. Connect tinySA Ultra to the computer
2. Place the DUT on the rotating platform
3. Configure frequency span and sweep parameters
4. Start acquisition
5. Rotate the DUT through 360°
6. View:

   - radiation patterns
   - emission spectrum
   - worst-case spectrum

---

# Future Features

Planned improvements include:
- QPD approximation
- antenna comparison tools
- 3D radiation pattern visualization
- More scanning axes (currently, only 360° around Z axis supported)

---

# Requirements

- **tinySA Ultra**
- Windows / Linux computer
- USB connection
- rotating DUT platform (github repo for a custom tool coming soon)

---

# Philosophy

polarSA is designed around three principles:

- **engineering-grade measurements**
- **simple hardware**
- **fast visualization**

The goal is to provide useful RF measurement tools with modest but capable EMC lab equipment.

---

# License
GPLV3

# About
Designed and maintained by Quantor Innovation https://quantor.fr/ 

