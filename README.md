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


Any contributions, suggestions or bug reports are very welcome.


Example1
--------
```cpp
//============================================================================
// Example 1:
// Create a simple plot from multiple input root files.
//============================================================================
{
  // create plotting environment that handles IO plot creation
  PlotManager plotManager;

  // first tell the manager where to look for the data you want to use in your plots
  // the first argument gives these input files a unique identifier you can then use when creating the plots
  plotManager.AddInputDataFiles("inputGroupA", {"path/to/file/a.root", "path/to/file/a2.root"});
  plotManager.AddInputDataFiles("inputGroupB", {"path/to/file/b.root", "path/to/file/b2.root", "path/to/file/b3.root:my/sub/list/or/dir/path"});

  // N.B.:
  // you can save these settings to a file via:
  plotManager.DumpInputDataFiles("path/to/inputFilesConfig.XML");
  // this way you can also modify this list of input files manually in the
  // xml file and then read it into your program via
  plotManager.LoadInputDataFiles("path/to/inputFilesConfig.XML");


  // now that we know where to look for the data, we can start creating plot(s).
  // the plot will be handed over to the manager after it was created and defined
  // therefore it makes sense to put this part in a scope {}
  // this way you you can re-use the symbol 'plot' for the next plots

  { // -----------------------------------------------------------------------
    // create the Plot object
    Plot plot("myPlot1", "myPlotGroup"); // plots are classified in user defined groups
    // optionally you can also define figure categories within the figureGroup
    plot.SetFigureCategory("QA");
     // you always have to specify height and width of the plot:
    plot.SetDimensions(710, 710);
    plot.SetTransparent(); // for all other settings see inc/Plot.h

    // a plot consists of multiple pads, which you can access via the [] operator (counting starts at '1'):
    plot[1].SetPosition(0., 0., 1., 1.); // specifies a pad that expands over the whole plot
    plot[1].SetFillFrame(kRed+2);
    // ... see definition of Pad class in inc/Plot.h for the list of all accessors

    // you can also define default settings for ALL pads of the plot by using the padID=0, e.g.:
    plot[0].SetMargins(0.07, 0.14, 0.12, 0.07);
    plot[0].SetDefaultMarkerSize(1.2).SetDefaultLineWidth(2.);

    // now set text properties, that will be used everywhere in the pad if not overridden for a specific lable/title/etc..
    plot[1].SetDefaultTextFont(43);
    plot[1].SetDefaultTextSize(24);

    // finally we can start adding data to the plot
    plot[1].AddData({"histName1", "inputGroupA"} ,"myLable");
    // this will search for a hist/graph/fuc called histName1 in all files specified in "inputGroupA"
    // the algorithm by default recursively traverses the whole directory or list substructure of the files and returns the first match
    // in case of multiple data with the same name, living in different subfolders/lists within the file you can do
    plot[1].AddData({"folder1/histName2", "inputGroupA"} ,"myLable2");
    plot[1].AddData({"folder2/histName2", "inputGroupA"} ,"myLable3");

    // it is possible to specify in the lables that you want to include some meta info of the data that is drawn, e.g.:
    plot[1].AddData({"histName1", "inputGroupA"} ,"myLable avg = <mean>");
    // possible options are: <name>, <title>, <entries>, <integral>, <maximum>, <minimum>, <mean>
    // you can use the standard printf style to specify how these numbers shall be formatted:
    plot[1].AddData({"histName1", "inputGroupA"} ,"myLable avg = <mean[.2f]>");
    plot[1].AddData({"histName2", "inputGroupA"} ,"myLable sum = <integral[.2e]>");
    // or let the framework decide the best way to do it
    plot[1].AddData({"histName2", "inputGroupA"} ,"myLable sum = <integral[.2]>");

    
    // now lets add another piece of input data from the second group (this time without adding a lable to the legend)
    plot[1].AddData({"histName3", "inputGroupB"});

    // you can also simply add ratios of two input data
    plot[1].AddRatio({"histName3", "inputGroupB"}, {"histName1", "inputGroupA"}, "ratioLable");


    // to mdify how the data is displayed we can apply the settings via:
    plot[1].AddData({"histName4", "inputGroupB"}).SetOptions("HIST C").SetLine(kGreen+2, kSolid, 3.);
    // instead of directly using the ROOT drawing option string ("HIST C") you can
    // use pre-defined human readible options like curve, points, points_line, etc (you can find all available options in PlottingFramework.h):
    plot[1].AddData({"graphName1", "inputGroupA"}).SetOptions(points).SetMarker(kRed, kFullCircle, 1.);
    // all root layout settings can be applied in this manner (see definition of Data class in inc/Plot.h for the list of all accessors)

    // now add a legend containing the lables that were specified when adding the data
    plot[1].AddLegend(0.5, 0.8);
    // if you leave out the xy pad coordinates, the framework will try to auto-place the legend without overlapping your data or the axes
    plot[1].AddLegend();


    // to modify the axis titles, offsets, ranges, etc of a pad, just do the following
    plot[1]["Y"].SetTitle("my y axis tilte with meaning");
    plot[1]["X"].SetRange(1.4, 2.8).SetTitle("what i am showing on x axis").SetLog();
    plot[1]["X"].SetMaxRangeX(5.);
    // a full list of axis property accessors can be found in the Axis class definition in inc/Plot.h


    // finally after plot definition is done we can add it to the manager
    // at this point the plot object we were modifying is moved to the manager
    // and the plot object remaining in the current scope will be empty
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------

  // we could now add plenty of more plots to the manager....
  { // -----------------------------------------------------------------------
    //Plot plot("myPlot2", "myPlotGroup:myCategory");
    // ...
    // ...
    // plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------

  // it is also possible to construct a plot based on a copy of another
  { // -----------------------------------------------------------------------
    Plot plot("myPlot", "myPlotGroup2");
    // ...
    plot[1].AddData({"folder1/histName2", "inputGroupA"} ,"myLable2");
    //...

    // now I want to have exact same plot, but with some additional data points
    // this can be helpful to avoid useless code duplication
    Plot plot2(plot, "myPlotGroup2");
    plot2[1].AddData({"graphName1", "inputGroupB"} ,"more data");

    // now add both to the manager
    plotManager.AddPlot(plot);
    plotManager.AddPlot(plot2);
  } // -----------------------------------------------------------------------


  // to save the plots to disk as pdf you first have to tell the manager where to put them
  plotManager.SetOutputDirectory("path/to/my/OutputFolder");
  // the manager will automatically create subfolders for the figureGroups and categories
  // to generate all stored plots as pdf you can call
  plotManager.CreatePlots();

  // for a specific figureGroup, figureCategory and or plotName just specify them:
  plotManager.CreatePlots("myPlotGroup", "myCategory");
  // or
  plotManager.CreatePlots("myPlotGroup", "", {"myPlot1", "myPlot2"});
  // in case you want the pdf files to contain the figureGroup and category in the file name, you can use
  plotManager.SetUseUniquePlotNames();

  // in "interactive" mode a root canvas window will pop up
  // and you can scroll throught the plots by double clicking on the right resp. left side of the plot
  plotManager.CreatePlots("", "", {}, "interactive");

  // you can also save the plots as a root macro (.C):
  plotManager.CreatePlots("myPlotGroup", "", {"myPlot1", "myPlot2"}, "macro");

  // after you specifying a file name you can also save the plots to a .root file
  plotManager.SetOutputFileName("ResultPlots.root");
  plotManager.CreatePlots("myPlotGroup", "", {"myPlot1", "myPlot2"}, "file");
}
```
