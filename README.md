# SciRooPlot
SciRooPlot is a [ROOT](https://github.com/root-project/root)-based plotting framework available in both C++ and Python. It facilitates creating and organizing plots arising in scientific data analyses.

## 🌟 Highlights

- define publication-ready plots with only a few lines of code
- intuitively handle data from multiple input files and substructures therein
- centrally organize and manage all of your plots
- quickly access your plots through a command-line application

## 🎯 Target group
This software is made for scientists working with ROOT data, who:
- are tired of the ever-increasing amount of single-purpose plotting macros that accumulate over the years
- simply want to draw their data without having to deal with technical details and complexities of ROOT
- miss an intuitive and straightforward interface to define a plot
- are looking for a lasting solution to organize and keep track of the many plots that arise in the course of their work
- appreciate useful additional plotting functionality that enhances productivity

SciRooPlot aims to address these day-to-day plotting requirements such that analyzers can focus on their science instead of wasting time with plotting technicalities.

Ideas, suggestions, and code contributions to this open-source project are very welcome.

## 🛠️ Installation
SciRooPlot works on Linux and macOS. It requires a working installation of [ROOT](https://github.com/root-project/root) and [Boost](https://www.boost.org/).
Python bindings are built automatically if a compatible environment is detected.

```bash
git clone https://github.com/SciRooPlot/SciRooPlot.git
cd SciRooPlot
./scripts/install.sh
```
The installer will print a command required to load the SciRooPlot environment.
Add this command to your shell startup script (for example `~/.bashrc` or `~/.zshrc`) to load SciRooPlot automatically.
After setup, the commands `srp` and `plot` are available from your terminal.

## 📖 Documentation

- Introductory slides: [link](https://cernbox.cern.ch/s/SBOWwlYkPUhd1KB)
- Full documentation: [link](https://SciRooPlot.github.io/SciRooPlot/)
