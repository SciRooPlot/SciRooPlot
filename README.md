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

You can find a commented example on how to use the framework in tut/Example.cxx
The framework ships with a simple command-line program that lets you easily generate plots from the plot definitions created by the user.
If you source the .plottingrc in your .bashrc this program can be used from everywhere.
For usage details and available options see plot --help.

To generate the documenation run:

    cd PlottingFramework/doc
    doxygen doxygen.conf
    open html/index.html


DESIGN
------

The PlotManager is the pivot point of the plotting framework.
There you define which input files should be loaded and where the output plots should be stored.

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
By default plots are created as pdf in your output folder, but you can also choose png or macro to create root macros or "interactive" if you only want to look at the plot and not create a file. Using the keyword "file" it is also possible to write the plots to a root file.


Regard this as an early beta version...

Any suggestions, bug reports and user feedback is very welcome.
