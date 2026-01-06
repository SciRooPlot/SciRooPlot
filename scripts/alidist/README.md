## Installation with aliBuild
Put `scirooplot.sh` into the `alidist` folder and install via
```bash
aliBuild build SciRooPlot
```
For convenience, put the following lines into your .bashrc, .zshrc or .bash_aliases
```bash
if [[ $ALIENVLVL == 1 ]] then;
  [[ "${LOADEDMODULES}" == *SciRooPlot* ]] && source ${SCIROOPLOT_ENV}
fi
```
Then enter the environment via
```bash
alienv enter --shellrc SciRooPlot/latest
```
