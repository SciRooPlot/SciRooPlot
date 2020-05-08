DEPENDENCIES
------------
- c++   >= 17
- cmake >= 3.15
- fmt   >= 6.1.2
- boost >= 1.72
- ROOT  >= 6.16

USE FRAMEWORK
-------------
cd PlottingFramework

mkdir build

cd build

cmake ../

make

The user needs to write an application that defines and saves the required plots (for inspiration see to app/MultDepSpec/).
This app can be added in the CMakeList.txt file (similar to the apps "plot" and "create" already specified there).
The framework ships with a simple command-line program that lets you easily generate plots from the files created by the user app.
For usage details and available options see ./plot --help.

To generate the documenation run:

cd PlottingFramework/doc

doxygen doxygen.conf

open html/index.html


DESIGN
------

The PlotManager is the pivot point of the plotting framework.
There you define which input files should be loaded and where the output plots should be stored.
The basic idea is that there are plot designs that can be specified and then have a unique name inside the framework.
Currently the designs "default" and "default ratio" are implemented. This can be extended or changed in PlotManager code.
In the final version this will be more flexible and more user frienldy to change via configure files.

First you need to define via SetOutputDirectory() the output folder where any output plots will be saved.

The input .root (or .csv) files containing the histograms and graphs you intend to plot will be known to the manager via a unique inputIdentifier string. This identifier can contain a group of input files.
File paths can be added via AddInputDataFiles(). Once loaded you can also dump these file paths to a config file via DumpInputDataFiles(). This config file can be extended outside of the program and again loaded via LoadInputDataFiles().

Once input and output are known to the manager, you can start adding plot definitions:
The best way to do this is to create a scope via curly brackets {} and create a Plot object inside.
You can change this plot object with a variaty of self explanatory accessors (see doxygen doc).
Then you need to add this new plot to the PlotManager via AddPlot().
This will move the settings of your plot to the manager and you will be left with an empty plot.
After adding all plots to the manager you can dump the plot definitions to an external config file (that can in principle also be modified by hand outside of the program) via DumpPlots().
These plot definitions you can again read into the manager via LoadPlots().
Alternatively you can also load and generate only specific plots defined in the config file via ExtractPlotsFromFile()
By default plots are created as pdf in your output folder, but you can also choose "bitmap" for pngs "macro" to create root macros or "interactive" if you only want to look at the plot and not create a file. Using the keyword "file" it is also possible to write the plots to a root file.


The framework still contains some unfinished pragmatic implementations that are currently beeing generalized.
Regard this as an early beta version...

Any suggestions, bug reports and user feedback is very welcome.
