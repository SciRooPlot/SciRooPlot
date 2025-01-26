# SciRooPlot
⬅️ [back to main page](https://SciRooPlot.github.io/SciRooPlot/)

## Advanced installation instructions
In case the dependencies ROOT and fmt are not installable with a package manager or you encounter some compatibility issues, the following tips may help you compile them manually.

### Building ROOT from source:
One reason you might need to compile ROOT from source is that SciRooPlot depends on cxx17 and ROOT has some compatibility issues when software using a different cxx standard is linked to it (see [here](https://root.cern/manual/integrate_root_into_my_cmake_project/)
).
This problem will result in errors such as:
```
error: conflicting declaration of template ‘template<class _CharT, class _Traits> using basic_string_view = std::experimental::__ROOT::basic_string_view<_CharT, _Traits>’
using basic_string_view = ::std::experimental::basic_string_view<_CharT,_Traits>;
```
To solve this you can compile ROOT with the cxx17 standard:
```bash
git clone https://github.com/root-project/root.git
cd root && mkdir mybuild && cd mybuild
cmake -DCMAKE_CXX_STANDARD=17 .. && make
```
For SciRooPlot to find this ROOT installation add the location to the `CMAKE_PREFIX_PATH`:
```bash
export CMAKE_PREFIX_PATH=/path/to/root/mybuild:$CMAKE_PREFIX_PATH
```
Alternatively, you can pass this information directly to SciRooPlot via the compiler flag:
`-DROOTSYS=/path/to/root/mybuild`

### Building fmt from source:

One reason you might need to compile fmt from source is that cmake's `find_package(fmt)` picks up a different 
fmt that comes with the GNU Utilities on common Linux distributions.
To avoid this you can compile your own fmt version:
```bash
git clone https://github.com/fmtlib/fmt.git
cd fmt && mkdir build && cd build
cmake -DBUILD_SHARED_LIBS=TRUE .. && make
```
For SciRooPlot to find this fmt installation add the location to the `CMAKE_PREFIX_PATH`:
```bash
export CMAKE_PREFIX_PATH=/path/to/fmt/build:$CMAKE_PREFIX_PATH
```
Alternatively, you can pass this information directly to SciRooPlot via the compiler flag:
`-DFMT_ROOT=/path/to/fmt/build`
