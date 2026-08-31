#!/bin/bash
LINTER_CMD="cpplint --filter=-whitespace/indent_namespace,-build/c++17,-whitespace/line_length,-whitespace/braces,-readability/todo"

${LINTER_CMD} ../include/SciRooPlot/*.h
${LINTER_CMD} ../src/*.cpp
${LINTER_CMD} ../app/*.cpp
${LINTER_CMD} ../user/*.cpp
