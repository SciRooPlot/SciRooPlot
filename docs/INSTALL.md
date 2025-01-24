Prerequisites
-------------


Using SciRooPlot
--------------------

To disable console colours in the logging output, use `cmake -D DISABLE_COLORS=ON ..`, or `cmake -D DARK_COLORS=ON ..` for a darker colour scheme.
The location of a dependency can be specified via the cmake arguments `ROOTSYS`, `FMT_ROOT` and `BOOST_ROOT`, respectively (e.g. `cmake -DROOTSYS=/path/to/software/root/mybuild ..`).




### Important note for linux users (fmt)
fmt can come shipped together with GNU Utilities on common linux distros. However, it is better to build fmt from source (see [here](https://fmt.dev/latest/usage.html)) with the BUILD_SHARED_LIBS flag set to
TRUE.

### Important note for linux users (gcc compiler):
Since this project relies on cxx17 and ROOT has some compatibility issue when software using a different cxx standard is linked to it (see [here](https://root.cern/manual/integrate_root_into_my_cmake_project/)
), it is necessary that your ROOT installation was also compiled with cxx17.\
You will spot this problem when you try to compile the framework and see errors such as\
`error: conflicting declaration of template ‘template<class _CharT, class _Traits> using basic_string_view = std::experimental::__ROOT::basic_string_view<_CharT, _Traits>’
using basic_string_view = ::std::experimental::basic_string_view<_CharT,_Traits>;`\
This is not an issue for mac users, who usually work with the Clang compiler.\
In order to work around this issue, simply compile the ROOT software as follows:
```console
cd /path/to/software
git clone https://github.com/root-project/root
cd root
mkdir mybuild
cd mybuild
cmake -D CMAKE_CXX_STANDARD=17 ..
make
```


# Building prerequisites from source

SciRooPlot depends on ROOT, fmt and boost which have to be installed first
the following steps illustrate how to do this on pcikf5 assuming you build all software from source code

# 0. create a directory where all the sofware should be installed
```
cd /data/$USER
mkdir software
```
# 1. compile ROOT from source:
# [alternatively you can skip building the code and just put
#  export CMAKE_PREFIX_PATH=/data/mkrueger/software/root/mybuild:$CMAKE_PREFIX_PATH 
#  in your .bashrc to use my installation]
# SciRooPlot relies on cxx17 and thus also root needs to be compiled with cxx17
# ----------------------
# use compiler that supports cxx17 (needs to be enabled only once per session, so below you can skip it)
scl enable gcc-toolset-13 bash
cd /data/$USER/software
git clone https://github.com/root-project/root.git
cd root
mkdir mybuild
cd mybuild
cmake -DCMAKE_CXX_STANDARD=17 -DPython3_FIND_STRATEGY=VERSION ..
make -j 120
(maybe the make command has to be run multiple times since parralel build of root sometimes interfers with itself)
# now put the following line in your .bashrc for the software to be found
export CMAKE_PREFIX_PATH=/data/$USER/software/root/mybuild:$CMAKE_PREFIX_PATH
# ----------------------

# 2. compile fmt from source:
# [alternatively you can skip building the code and just put
#  export CMAKE_PREFIX_PATH=/data/mkrueger/software/fmt/build:$CMAKE_PREFIX_PATH 
#  in your .bashrc to use my installation]
# ----------------------
# scl enable gcc-toolset-13 bash
cd /data/$USER/software
git clone https://github.com/fmtlib/fmt.git
cd fmt
mkdir build
cd build
cmake -DBUILD_SHARED_LIBS=TRUE ..
make
# now put the following line in your .bashrc for the software to be found
export CMAKE_PREFIX_PATH=/data/$USER/software/fmt/build:$CMAKE_PREFIX_PATH
# ----------------------

# 3. install SciRooPlot:
# ----------------------
# scl enable gcc-toolset-13 bash
# [alternatively you can skip building the code and just put
#  source /data/mkrueger/software/SciRooPlot/.plotrc 
#  in your .bashrc to use my installation]
cd /data/$USER/software
git clone https://github.com/SciRooPlot/SciRooPlot.git
cd SciRooPlot
mkdir build
cd build
cmake ..
make
# now put the following line in your .bashrc for the software to be found
source /data/$USER/software/SciRooPlot/.plotrc
# (also run this command manually or log in again for it to take effekt)
# ----------------------

# 4. start using SciRooPlot
# ----------------------
# scl enable gcc-toolset-13 bash
# now cd to any location on the comupter and run
plot-init myPlots
cd myPlots
mkdir build
cd build
cmake ..
make
# ----------------------
# now you are good to go!
# you can use the plotting app from everywhere
# try 
plot test plot1

# idea is that you look into CreatePlots.cxx, and define your plots there
# those will be saved to a file and the plotting app then loads these definitions and creates the plots
# for description of usage see:
https://github.com/SciRooPlot/SciRooPlot
or for a real-life example
https://gitlab.cern.ch/mkruger/MultDepSpecPlots







