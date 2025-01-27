LINTER_CMD="cpplint --filter=-whitespace/line_length,-whitespace/braces,-readability/todo"

${LINTER_CMD} ../include/*.h
${LINTER_CMD} ../src/*.cxx
${LINTER_CMD} ../app/*.cxx
${LINTER_CMD} ../user/*.cxx
