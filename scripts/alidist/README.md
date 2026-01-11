## Installation with aliBuild
Put `scirooplot.sh` into the `alidist` folder and install via
```bash
aliBuild build SciRooPlot
```
After entering the environment via
```bash
alienv enter SciRooPlot/latest
```
source the environment script to enable all features of SciRooPlot:
```bash
source "${SCIROOPLOT_ENV}"
```

To conveniently auto-load the env script, you may also put the following lines into your .bashrc, .zshrc or .bash_aliases
```bash
if [[ "${ALIENVLVL:-0}" == 1 ]]; then
  [[ "${LOADEDMODULES:-}" == *SciRooPlot* ]] && source "${SCIROOPLOT_ENV}"
fi
```
and then enter the environment via
```bash
alienv enter --shellrc SciRooPlot/latest
```
