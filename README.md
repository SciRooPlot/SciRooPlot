# SciRooPlot
SciRooPlot is a [ROOT](https://github.com/root-project/root)-based plotting framework available in both C++ and Python. It simplifies the creation and organization of plots for scientific data analysis.

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
- are looking for a way to organize and keep track of the many plots that arise in the course of their work
- appreciate useful additional plotting functionality that enhances productivity

SciRooPlot aims to simplify common plotting tasks and reduce the amount of plotting-specific implementation work required from analyzers.

Contributions to SciRooPlot are very welcome. This includes bug reports, feature
ideas, suggestions, documentation improvements, and code contributions.
Please feel free to open an issue to discuss an idea or submit a pull request.

## 🛠️ Installation
SciRooPlot works on both Linux and macOS.
It can be installed either via the package managers homebrew and conda, or using the SciRooPlot installer.
The latter requires that [ROOT](https://github.com/root-project/root) and
[Boost](https://www.boost.org/) are installed on your system.

After installation, source the provided SciRooPlot environment script to enable all features.
Once loaded, the `srp` and `plot` commands are available in your terminal.

For convenience, you can add the source command to your shell startup file (for example `~/.bashrc` or `~/.zshrc`) to load SciRooPlot automatically in new shell sessions.

### Homebrew
```bash
brew tap SciRooPlot/scirooplot
brew install SciRooPlot
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

## 📖 Documentation

- User manual: [Cpp interface](https://scirooplot.github.io/SciRooPlot/slides/SciRooPlot_UserManual_cpp.pdf), [Python interface](https://scirooplot.github.io/SciRooPlot/slides/SciRooPlot_UserManual_py.pdf)
- Complementary documentation: [Webpage](https://SciRooPlot.github.io/SciRooPlot/)
