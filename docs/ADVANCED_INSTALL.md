# SciRooPlot
⬅️ [back to main page](https://SciRooPlot.github.io/SciRooPlot/)

## Advanced installation instructions
SciRooPlot requires ROOT 6.36.04 or newer. In case your package manager does not provide it, you can build ROOT from source.

### Building ROOT from source:
```bash
git clone --branch v6-40-04 --depth 1 https://github.com/root-project/root.git root_src
cmake -S root_src -B root_build -DCMAKE_INSTALL_PREFIX=$HOME/root
cmake --build root_build --parallel
cmake --install root_build
source $HOME/root/bin/thisroot.sh
```
Sourcing `thisroot.sh` (e.g. in your `.bashrc` or `.zshrc`) is enough for SciRooPlot to find this ROOT installation.
Alternatively, you can pass its location directly to SciRooPlot via `-DROOT_DIR=$HOME/root/cmake`.

The C++ standard does not need to be chosen: SciRooPlot and your plotting projects are compiled with the C++ standard ROOT was built with (C++17 or newer).

### fmt and pybind11:
fmt and pybind11 are fetched automatically in case they are not installed or too old.
To use the fetched fmt even though a suitable version is installed, pass `-DCMAKE_DISABLE_FIND_PACKAGE_fmt=ON`.
