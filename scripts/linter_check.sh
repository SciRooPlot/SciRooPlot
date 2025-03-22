LINTER_CMD="cpplint --filter=-whitespace/indent_namespace,-build/c++17,-whitespace/line_length,-whitespace/braces,-readability/todo"

${LINTER_CMD} ../include/*.h
${LINTER_CMD} ../src/*.cxx
${LINTER_CMD} ../app/*.cxx
${LINTER_CMD} ../user/*.cxx
