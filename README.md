# SciRooPlot
SciRooPlot is a [ROOT](https://github.com/root-project/root)-based C++ and Python framework to define, organize and generate plots.

<p align="center">
  <b><a href="https://scirooplot.github.io/SciRooPlot/">Website</a> · <a href="https://scirooplot.github.io/SciRooPlot/#tour">Guided tour</a> · <a href="https://scirooplot.github.io/SciRooPlot/#examples">Examples</a></b>
</p>

<p align="center">
  <a href="https://scirooplot.github.io/SciRooPlot/#examples">
    <img src="https://scirooplot.github.io/SciRooPlot/img/ex/higgs/limits/brazilPlot.png" width="32%" alt="exclusion limits with uncertainty bands">
    <img src="https://scirooplot.github.io/SciRooPlot/img/ex/heavyion/correlations/ridge.png" width="32%" alt="two-particle correlation in 3D">
    <img src="https://scirooplot.github.io/SciRooPlot/img/ex/animations/higgsDiscovery.gif" width="32%" alt="an animated plot">
  </a>
</p>

## 🌟 Highlights

- define publication-ready plots with only a few lines of code, in C++ or Python with the same interface
- read data from ROOT files and their folders and lists, trees, RNTuples, CSV tables or numpy arrays and pandas DataFrames
- centrally organize and manage all of your plots in projects and groups
- show any plot with one command: `plot <group> <name>`, with tab completion for groups and names
- write the same plots as pdf, png, svg, ROOT macros or animated gifs

## 🎯 Target group
This software is made for scientists, who:
- are tired of the ever-increasing amount of single-purpose plotting macros that accumulate over the years
- simply want to draw their data without having to deal with technical details and complexities of ROOT
- miss an intuitive and straightforward interface to define a plot
- are looking for a way to organize and keep track of the many plots that arise in the course of their work
- appreciate useful additional plotting functionality that enhances productivity

SciRooPlot aims to simplify common plotting tasks and reduce the amount of plotting-specific implementation work required from analyzers.
No ROOT knowledge is needed to start: Python users can plot their numpy and pandas data directly.

## 🛠️ Installation
SciRooPlot works on both Linux and macOS.
It can be installed either via the package managers homebrew and conda, or using the SciRooPlot installer.
The latter requires that [ROOT](https://github.com/root-project/root) (6.36.04 or newer),
[Boost](https://www.boost.org/), CMake (3.27 or newer) and a C++17 compiler are installed on your system.
Plotting programs written in C++ also need CMake and a C++17 compiler; Python programs need nothing else.

After installation, source the provided SciRooPlot environment script to enable all features.
Once loaded, the `srp` and `plot` commands are available in your terminal.

For convenience, you can add the source command to your shell startup file (for example `~/.bashrc` or `~/.zshrc`) to load SciRooPlot automatically in new shell sessions.

### Homebrew
```bash
brew tap SciRooPlot/scirooplot
brew install scirooplot
source "$(brew --prefix scirooplot)/share/scirooplot/env.sh"
```

### Conda
```bash
conda install conda-forge::scirooplot
source "${CONDA_PREFIX}/share/scirooplot/env.sh"
```

### Installer
```bash
git clone https://github.com/SciRooPlot/SciRooPlot.git
cd SciRooPlot
./scripts/install.sh
source ./install/share/scirooplot/env.sh
```
In your shell startup file you have to use the absolute path to the env script, which the installer will print at the end.
You can also choose a custom install location via `--prefix <my/install/directory>`.

### AI agents
If you work with a coding agent, import [scirooplot.skill](https://scirooplot.github.io/SciRooPlot/skill/scirooplot.skill) into it.
The file describes the workflow, the API and common pitfalls; the same content is indexed for web-based agents in [llms.txt](https://scirooplot.github.io/SciRooPlot/llms.txt).

## 🚀 Quick start
```bash
srp example-py examples    # a project with many commented example plots and their data (or example-cpp)
plot higgs .+              # every plot of the group higgs, browsed with s (next) and a (back)
plot higgs .+ pdf          # the same plots as pdf files
```
`srp init-py <project>` (or `init-cpp`) starts your own project from a minimal program, `DefinePlots.py`.
Change it and run `plot` again: the program is re-run (and rebuilt in C++) by itself.

## 📖 Documentation

Start at the **[SciRooPlot website](https://scirooplot.github.io/SciRooPlot/)**: a guided tour for newcomers, the full reference for experienced users and example plots with their code.

- User manual (slides): [C++ interface](https://scirooplot.github.io/SciRooPlot/slides/SciRooPlot_UserManual_cpp.pdf) · [Python interface](https://scirooplot.github.io/SciRooPlot/slides/SciRooPlot_UserManual_py.pdf)
- Updating existing code to a new version: [interface changes](https://scirooplot.github.io/SciRooPlot/#updating)

## 🤝 Contributing
Contributions to SciRooPlot are very welcome. This includes bug reports, feature
ideas, suggestions, documentation improvements, and code contributions.
Please feel free to open an issue to discuss an idea or submit a pull request.
The people behind SciRooPlot are listed in [CONTRIBUTORS.md](CONTRIBUTORS.md).

SciRooPlot is free software under the [GNU General Public License v3](LICENSE).
