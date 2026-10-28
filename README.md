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

### CSV Export / Import and Pattern Comparison

**Export...** first asks for measurement notes (DUT, antenna, matching, polarization, operator, free comments; remembered between exports), then saves the last sequence capture (every spectrum at every angle) to a CSV file:

```
# polarSA measurement export
# dut,Sensor board rev B        <- user notes
# matching,pi 2.2nH / 1.5pF / NC
# center_MHz,100.0000            <- capture settings (informative)
angle_deg/freq_Hz,95000000,...   <- frequencies in Hz
0,-82.50,...                     <- one row per angle, power in dBm
10,-80.13,...
```

**Import...** loads one or several of these files and overlays their radiation patterns on the polar plot, at the frequency selected by the cursor (nearest frequency bin; a file whose range does not include that frequency is not drawn). The frequency cursor spans the frequencies of all traces (live capture and imports), so every trace stays reachable even when they were measured in different bands. **Center** jumps to the middle of the live capture (or of the first visible import), **Peak** to the frequency of maximum power over all visible traces and angles. Colors are assigned in import order: red, yellow, cyan, then a set of distinct colors (green, orange, blue, magenta...), then random. In the **Traces** list, the checkbox shows/hides a trace (the live capture included) and a double-click opens a color picker. Hovering a file shows its notes and settings. **Clear Imports** removes all imported files.

The radiation pattern and Max Emissions graphs are separated by a splitter: drag it to resize, drag it to an edge to hide a graph, or double-click it to hide/show Max Emissions.

Files using `;` or tab separators with a `,` decimal mark (e.g. re-saved by a French-locale spreadsheet) are accepted on import.

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

