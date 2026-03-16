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
- export to/ import from CSV
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


