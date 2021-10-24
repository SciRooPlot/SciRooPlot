#!/bin/bash

AUTOCOMPFILE=$1

# TODO: make this work with multiple files
PLOTDEFFILE=$(${__PLOTTING_BUILD_DIR}/plot-config get plotDefinitions)
if [[ ! -f "$PLOTDEFFILE" ]]; then
  exit
fi

# check if csv file exists and if yes if is newer than the plot definition file
if [[ -f "${AUTOCOMPFILE}" ]]; then
  if [[ ${AUTOCOMPFILE} -nt ${PLOTDEFFILE} ]]; then
    exit
  fi
fi

cat $PLOTDEFFILE | grep -o "<PLOT::.*>" | grep -vE '.*TEMPLATES.*' > tmp

> $AUTOCOMPFILE
while read -r line ; do
  line="${line#<PLOT::}"
  line="${line%>}"
  line="${line/_IN_/,}"
  line="${line/|/,}"
  line="${line//|//}"
  echo $line >> $AUTOCOMPFILE
done < tmp
rm tmp
