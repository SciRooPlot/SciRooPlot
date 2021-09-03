#!/bin/bash

if [ ! -d "${__PLOTTING_CONFIG_DIR}" ]; then
  exit
fi

# TODO: make this work with multiple files
PLOTDEFFILE="${__PLOTTING_CONFIG_DIR}/plotDefinitions.XML"
if [[ ! -f "$PLOTDEFFILE" ]]; then
  exit
fi

AUTOCOMPFILE=plots.csv
# check if csv file exists and if yes if is newer than the plot definition file
if [[ -f "$AUTOCOMPFILE" ]]; then
  if [[ $AUTOCOMPFILE -nt $PLOTDEFFILE ]]; then
    exit
  fi
fi

cat $PLOTDEFFILE | grep -o "<PLOT::.*>" | grep -vE '.*TEMPLATES.*' > tmp

> $AUTOCOMPFILE
while read -r line ; do
  line="${line#<PLOT::}"
  line="${line%>}"
  line="${line/_IN_/,}"
  line="${line/:/,}"
  line="${line//|//}"
  echo $line >> $AUTOCOMPFILE
done < tmp
rm tmp
