package: SciRooPlot
version: "v1.2.1"
requires:
  - ROOT
  - fmt
  - boost
build_requires:
  - "GCC-Toolchain:(?!osx)"
  - CMake
source: https://github.com/SciRooPlot/SciRooPlot.git
---
#!/bin/bash -ex
cmake $SOURCEDIR -DCMAKE_INSTALL_PREFIX=$INSTALLROOT       \
                 -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE      \
                 -G Ninja

cmake --build . -- ${JOBS:+-j$JOBS} install

[[ -d $INSTALLROOT/lib64 ]] && [[ ! -d $INSTALLROOT/lib ]] && ln -sf lib64 $INSTALLROOT/lib

#ModuleFile
MODULEDIR="${INSTALLROOT}/etc/modulefiles"
MODULEFILE="${MODULEDIR}/${PKGNAME}"
mkdir -p ${MODULEDIR}

alibuild-generate-module --bin --lib --cmake > "${MODULEFILE}"

cat >> "${MODULEFILE}" <<EoF
prepend-path ROOT_INCLUDE_PATH \${PKG_ROOT}/include
source-sh bash "\${PKG_ROOT}/share/scirooplot/scirooplot-env.sh"
puts stderr {For SciRooPlot tab completion: source \$SCIROOPLOT_ENV}
EoF
