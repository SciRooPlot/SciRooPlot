DEPENDENCIES
------------
- boost >= 1.68
- ROOT  >= 6.16

USE FRAMEWORK
-------------
cd PlottingFramework
mkdir build
cd build
cmake ../

The user entry point is the CreatePlots.cxx file.
You can also define your own MyPlotting.cxx and adjust the add_executable() function in CMakeLists.txt accordingly


DESIGN
------

The PlotManager is the central point for the plotting.
There you define which input files should be loaded and where the output plot should be stored.
The basic idea is that there are plot designs that can be specified and then have a unique name.
Currently I implemented "default" and "default ratio". This can be extended or changed in PlotManager code.
In the final version this will be more flexible and more user frienldy to change.

First you need to define the output folder where any output plots will end up via SetOutputDirectory().

The input root files containing the histograms and graphs you want to plot will be known to the manager via a unique inputIdentifier string. This identifier can contain a group of input files.
File paths can be added via AddInputDataFiles(). Once loaded you can also dump these file path to a config file via DumpInputDataFiles(). This config file can be extended outside of the program and again loaded via LoadInputDataFiles().

Once input and output is defined in the manager, you can start adding plot definitions to the manager:
The best way is to create a scope via {} and create there a Plot object.
You can change this plot object with a bunch of self explanatory accessors.
Then you need to add this plot to the manager via AddPlot()
This adding will move the settings of your plot to the manager and you will be left with an empty plot.
After adding your plots to the manager you can eigher generate your plots via CreatePlots()
Or you can dump the plot definitions to an external config file that can also be modified outside of the program via DumpPlots()
These definitions you can read into the manager again via LoadPlots().
Alternatively you can also load and generate only specific plots defined in the config file via CreatePlotsFromFile()
By default plots are created as pdf in your output folder, but you can also choose "bitmap" for pngs "macro" to create root macros or "interactive" if you only want to look at the plot and not create a file.


The framework is still contains some unfinished or not very general pragmatic implementations, so please regard this as a very early beta version.. 
Any suggestions, bug reports and user feedback is very welcome.
