#!/bin/bash

AUTOCOMPFILE=$1

# TODO: make this work with multiple files
PLOTDEFFILE=$(${__SCIROOPLOT_BUILD_DIR}/plot-config get plotDefinitions)
if [[ ! -f "$PLOTDEFFILE" ]]; then
  exit
fi

# check if csv file exists and if yes if is newer than the plot definition file
if [[ -f "${AUTOCOMPFILE}" ]]; then
  if [[ ${AUTOCOMPFILE} -nt ${PLOTDEFFILE} ]]; then
    exit
  fi
fi

TMPFILE="${AUTOCOMPFILE}.tmp"

cat $PLOTDEFFILE | grep -o "<PLOT::.*>" | grep -vE '.*TEMPLATES.*' > $TMPFILE

> $AUTOCOMPFILE
while read -r line ; do
  line="${line#<PLOT::}"
  line="${line%>}"
  line="${line/_IN_/,}"
  line="${line/|/,}"
  line="${line//|//}"
  echo $line >> $AUTOCOMPFILE
done < $TMPFILE
rm $TMPFILE
