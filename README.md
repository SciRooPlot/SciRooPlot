Rationale
---------
This program aims to simplify day-to-day scientific plotting by adding a level of abstraction on top of the mighty [ROOT data analysis framework](https://github.com/root-project/root).
It provides an intuitive interface for defining plots and makes it very easy to work with data stored in many different input files.
An interactive command-line plotting app allows to quickly find and create specific plots from the pool of plot definitions.

Everybody is welcome to use this software and contribute new features.

Prerequisites
-------------
The PlottingFramework depends on the following external software which needs to be installed on your system:

- cmake >= 3.15
- fmt   >= 6.1.2
- boost >= 1.65
- ROOT  >= 6.16

### Important note for linux users (gcc compiler):
Since this project relies on cxx17 and ROOT has some compatibility issue when software using a different cxx standard is linked to it (see [here](https://root.cern/manual/integrate_root_into_my_cmake_project/)
), it is necessary that your ROOT installation was also compiled with cxx17.\
You will spot this problem when you try to compile the framework and see errors such as\
`error: conflicting declaration of template ‘template<class _CharT, class _Traits> using basic_string_view = std::experimental::__ROOT::basic_string_view<_CharT, _Traits>’
using basic_string_view = ::std::experimental::basic_string_view<_CharT,_Traits>;`\
This is not an issue for mac users, who usually work with the Clang compiler and might also not be a problem when using the latest gcc version.\
In order to work around this issue, simply compile the ROOT software as follows:
```console
cd ~
git clone https://github.com/root-project/root
cd root
mkdir mybuild
cd mybuild
cmake -DCMAKE_CXX_STANDARD=17 ..
make
```
In case you do not want to install ROOT into `~/root/mybuild` you will need to adapt line `set(ROOTSYS ~/root/mybuild)` in the `CMakeLists.txt` file of this project accordingly.

Using the Framework
--------------------
In order to use the framework, first you need to compile the library in the following way:

    cd PlottingFramework
    mkdir build
    cd build
    cmake ..
    make

To disable console colours in the logging output, use `cmake -D DISABLE_COLORS=ON ..`, or for a darker colour scheme `cmake -D DARK_COLORS=ON ..` instead.
In case you have doxygen installed, you can also generate a documentation from the comments in the code via:

    cd PlottingFramework/doc
    doxygen doxygen.conf
    open html/namespace_plotting_framework.html

After building the library, you can write your own individual plotting programs and link them against the PlottingFramework.
As a starting point you can copy the `example` folder to some location on your computer and follow the instructions provided in `CreatePlots.cxx`.
This example should work out of the box, but it requires to know where the PlottingFramework library is located.
The easiest way to define this is by putting the following line in your .bashrc or .bash_aliases:
```
    source /path/to/PlottingFramework/.plotrc
```

The PlotManager is the pivot point of the PlottingFramework. In your application you will have to
```cpp
    #include "PlotManager.h"
```
which will provide all the user interfaces you need.
In the following you can find some examples illustrating how to use the framework.

Example 1
---------
```cpp
//============================================================================
// Example 1:
// Create a simple plot from multiple input root files.
//============================================================================

#include "PlotManager.h"
using namespace PlottingFramework;

// the plot manager is the central entity that handles and creates your plots
PlotManager plotManager;

// first you need to define the data sources that should be available for your plots
plotManager.AddInputDataFiles("inputIdentifierA", {"/path/to/file/a.root", "/path/to/file/a2.root"});
// instead of adding the whole root file, you can also specify a sub-list or sub-folder within the file:
plotManager.AddInputDataFiles("inputIdentifierB", {"/path/to/file/b.root:sub/list/or/dir"});
// the paths may contain any environment variable defined in your shell:
plotManager.AddInputDataFiles("inputIdentifierC", {"${HOME}/myRootFiles/b2.root"});
// and it is possible to add all root files within a directory (including sub-directories):
plotManager.AddInputDataFiles("inputIdentifierD", {"/path/to/folder/with/rootfiles/"});
// please note that multiple root files grouped under one inputIdentifier will be treated as one big input file and are traversed in alphabetical order

// N.B.:
// you can save these settings to a file via:
plotManager.DumpInputDataFiles("/path/to/inputFiles.XML");
// this way you can also modify this list of input files manually in the
// xml file and then read it into your program via
plotManager.LoadInputDataFiles("/path/to/inputFiles.XML");

// now that we know where to look for the data, we can start creating plots
// each plot will be handed over to the manager after it was defined
{ // -----------------------------------------------------------------------
  // create the Plot object
  Plot plot("myPlot1", "myFigureGroup"); // plots are organized in user defined figure groups
  // optionally you can also define figure categories (and subcategories) within a figure group
  plot.SetFigureCategory("QA-Plots/closureTests");
   // you always have to specify height and width of the plot:
  plot.SetDimensions(710, 710);
  plot.SetTransparent(); // for all other settings see the class definition in inc/Plot.h

  // a plot consists of multiple pads, which you can access via the [] operator (counting starts at '1'):
  plot[1].SetPosition(0., 0., 1., 1.); // specifies a pad that expands over the whole plot
  plot[1].SetFillFrame(kRed+2);
  // ... see definition of Pad class in inc/Plot.h for the list of all accessors

  // you can also define default settings for ALL pads of the plot by using the padID=0, e.g.:
  plot[0].SetMargins(0.07, 0.14, 0.12, 0.07);
  plot[0].SetDefaultMarkerSize(1.2).SetDefaultLineWidth(2.);
  // as you can see here the user interface is designed such that each setter returns
  // a reference to the object that is currently being modified. This allows you to concatenate
  // all the settings belonging to a pad. This design pattern is consistently used also for all
  // the other user definable objects (Data, Axis, Legends, etc.).

  // now set text properties, that will be used everywhere in the pad if not overridden for a specific label/title/etc..
  plot[1].SetDefaultTextFont(43);
  plot[1].SetDefaultTextSize(24);

  // finally we can start adding data to the plot
  plot[1].AddData("histName1", "inputIdentifierA", "myLabel");
  // this will search for a histogram, graph or function called "histName1" in all files specified for "inputIdentifierA"
  // the algorithm by default recursively traverses the whole directory or list substructure of the files and returns the first match
  // in case of multiple data with the same name, living in different subfolders/lists within the file you can do
  plot[1].AddData("folder1/histName2", "inputIdentifierA", "myLabel2");
  plot[1].AddData("folder2/histName2", "inputIdentifierA", "myLabel3");

  // by default the global coordinate system of the plot is defined by the first data that is added (equivalent to how ROOT does it)
  // however, every so often we want to plot first some data with a small range and on top of it some data with a larger range
  // while allowing the plot to show the full range of the second data
  // this can be achieved by explicitly stating that the second data should be the one that defines the axis frame of the plot:
  plot[1].AddData("data2", "inputIdentifierA").SetDefinesFrame();

  // it is possible to specify in the labels that you want to include some meta info of the data that is drawn, e.g.:
  plot[1].AddData("histName1", "inputIdentifierA", "mylabel avg = <mean>");
  // possible options are: <name>, <title>, <entries>, <integral>, <maximum>, <minimum>, <mean>
  // you can use the standard printf style to specify how these numbers shall be formatted:
  plot[1].AddData("histName1", "inputIdentifierA", "mylabel avg = <mean[.2f]>");
  plot[1].AddData("histName2", "inputIdentifierA", "mylabel sum = <integral[.2e]>");

  // now lets add another piece of input data from the second group (this time without adding a label to the legend)
  plot[1].AddData("histName3", "inputIdentifierB");

  // you can also simply add the ratio of two input data
  plot[1].AddRatio("histName3", "inputIdentifierB", "histName1", "inputIdentifierA", "ratioLabel");
  // for incompatible data (e.g. different number of bins or bin limits) the framework will try to use splines to interpolate the data points before dividing
  // in case numerator and denominator are sub-samples of one another bayesian error propagation can be applied:
  plot[1].AddRatio("histName3", "inputIdentifierB", "histName1", "inputIdentifierA", "ratioLabel").SetIsCorrelated();
  

  // to modify how the data is displayed we can apply the settings via:
  plot[1].AddData("histName4", "inputIdentifierB").SetOptions("HIST C").SetLine(kGreen+2, kSolid, 3.);
  // instead of directly using the ROOT drawing option string ("HIST C") you can
  // use pre-defined human readable options like curve, points, points_line, etc
  // (you can find all available options in inc/PlotPainter.h):
  plot[1].AddData("graphName1", "inputIdentifierA").SetOptions(points).SetMarker(kRed, kFullCircle, 1.);
  // all root layout settings can be applied in this manner
  // (see definition of Data class in inc/Plot.h for the list of all accessors)

  // now add a legend containing the labels that were specified when adding the data
  plot[1].AddLegend(0.5, 0.8);
  // if you leave out the xy pad coordinates, the framework will try to auto-place the legend without overlapping your data or the axes
  plot[1].AddLegend();
  
  // it is possible to add multiple legends to the plot (as done above) which are then identified by the order they were added
  // in order to express that the label for a specific data should be put in the second legend you can do the following:
  plot[1].AddData("function1", "inputIdentifierA", "my label in second legend").SetLegendID(2);

  // the styles and colors of the respective legend entries are by default based on the data that is represented
  // you can however override this and specify different default settings for the legend
  plot[1].GetLegend(2).SetDefaultLineWidth(5.);
  // modifications can also be done per entry:
  plot[1].GetLegend(2).GetEntry(1).SetLineColor(kRed);
  // you can also add and modify legends without any automatically generated labels:
  plot[1].AddLegend();
  plot[1].GetLegend(3).GetEntry(1).SetLabel("my custom lable").SetDrawStyle("L").SetLineColor(kRed);
  plot[1].GetLegend(3).GetEntry(1).SetLabel("my custom lable 2").SetDrawStyle("EP").SetLineColor(kBlue).SetMarkerColor(kRed);
  // (again please have a look at inc/Plot.h for the list of possibilities)

  // similar to the legend boxes you can add a text box to your plot
  plot[1].AddText("my important description spanning // over two lines");
  // or
  plot[1].AddText(0.2, 0.9, "this is // where I // want my text");
  // by default " // " is used as a delimiter between multiple lines

  // pro tip: to determine the perfect position of the legend or text,
  // you can place it at the desired position in interactive mode (see below)
  // and double-click on it to print out its current coordinates
    
  // to modify the axis titles, offsets, ranges, etc of a pad, just do the following
  plot[1]['Y'].SetTitle("my y axis title with meaning");
  plot[1]['X'].SetRange(1.4, 2.8).SetTitle("what I am showing on x axis").SetLog();
  // or if you want to modify synchronously the axes of multiple plots (e.g. for 2-panel ratios) you can apply the settings to the default pad
  plot[0]['X'].SetRange(1.4, 2.8);
  // a full list of axis property accessors can be found in the Axis class definition in inc/Plot.h

  // note that you can define the visible range of the data independent of the pad axis range
  plot[1].AddData("hist", "inputIdentifierA", "hist with reduced range").SetRangeX(1.5, 2.7);

  // finally, after plot definition is done we can add it to the manager
  // at this point the plot object we were modifying is moved to the manager
  // and the plot object remaining in the current scope will be empty
  plotManager.AddPlot(plot);
} // -----------------------------------------------------------------------

// we could now add plenty of more plots to the manager....
{ // -----------------------------------------------------------------------
  //Plot plot("myPlot2", "myFigureGroup/myCategory");
  // ...
  // ...
  // plotManager.AddPlot(plot);
} // -----------------------------------------------------------------------

// it is also possible to construct a plot based on a copy of another
{ // -----------------------------------------------------------------------
  Plot plot("myPlot", "myFigureGroup2/my/figure/sub/category");
  // ...
  plot[1].AddData("folder1/histName2", "inputIdentifierA", "myLabel2");
  //...

  // now we want to have exact same plot, but with some additional data points
  // this can be helpful to avoid code duplication
  Plot plot2(plot, "myPlot2", "myFigureGroup2");
  plot2[1].AddData("graphName1", "inputIdentifierB", "more data");

  // now add both to the manager
  plotManager.AddPlot(plot);
  plotManager.AddPlot(plot2);
} // -----------------------------------------------------------------------

// in case you are dealing with {2,3,n}-dimensional input data, it is also possible
// to book only projections of the data for your plot
{ // -----------------------------------------------------------------------
  Plot plot("myProjectionsIllustration", "myFigureGroup2");

  // projection of 2d data on X axis
  plot[1].AddData("my2dhist1", "inputIdentifierA").SetProjectionX();
  // projection of 2d data on X axis but with restricted Y range:
  plot[1].AddData("my2dhist2", "inputIdentifierA").SetProjectionX(yBin, yBin);
  // the same works also in user coordinates:
  plot[1].AddData("my2dhist3", "inputIdentifierA").SetProjectionX(yValue, yValue, true);

  // for arbitrary-dimensional data (also the 2d histograms above) you can use the more generic function
  plot[1].AddData("myNdhist3", "inputIdentifierA").SetProjection({3}, { {0, 5, 10},  {1, 90, 100} });
  // here the first argument is a vector of dimensions you want to project on
  // for instance {2,0} would produce a projection of your n-dimensional input histogram where
  // the third original axis will be the new x-axis and the first original axis will be the y axis of your plotted data
  // with the second argument of the SetProjection() function you can specify the ranges of the other axes prior to the projection
  // this means in the above example the axis 0 of the input data is restricted to bin 5 to 10 and axis 1 is restricted to bin 90 to 100
  // before the histogram is projected on the axis 3
  // also here one can provide user coordinates instead of bins by setting the third argument of the function to 'true'

  // the same can be done also with the numerator and denominator of ratios
  // have a look at inc/Plot.h for the details

  plotManager.AddPlot(plot);
} // -----------------------------------------------------------------------

// to save the plots to disk as pdf you first have to tell the manager where to put them
plotManager.SetOutputDirectory("/path/to/OutputFolder");
// the manager will automatically create subfolders for the figure groups and categories
// to generate all stored plots as pdf you can call
plotManager.CreatePlots();

// for a specific figure group, category and / or plot just specify them like this:
plotManager.CreatePlots("myFigureGroup", "myFigureCategory");
plotManager.CreatePlots("myFigureGroup", "", {"myPlot1", "myPlot2"});
// in case you want the pdf files to contain the figureGroup and category in the file name, you can use
plotManager.SetUseUniquePlotNames();

// in "interactive" mode a root canvas window will pop up
// and you can scroll through the plots by double clicking on the right resp. left side of the plot
// or by pressing the keys 'a' (left) and 's' (right) while the mouse pointer resides on the canvas
plotManager.CreatePlots("", "", {}, "interactive");

// you can also save the plots as a root macro (.C):
plotManager.CreatePlots("myFigureGroup", "", {"myPlot1", "myPlot2"}, "macro");

// after specifying a file name you can also save the plots to a .root file
plotManager.SetOutputFileName("ResultPlots.root");
plotManager.CreatePlots("myFigureGroup", "", {"myPlot1", "myPlot2"}, "file");

```
Example 2
---------
```cpp
//============================================================================
// Example 2:
// Create plots based on a template and save plot definitions to file.
//============================================================================

#include "PlotManager.h"
using namespace PlottingFramework;

PlotManager plotManager;

// assuming you saved the list of input root files from example 1 to a xml file via
// plotManager.DumpInputDataFiles("/path/to/inputFiles.XML");
// we can now load this directly into the manager via:
plotManager.LoadInputDataFiles("/path/to/inputFiles.XML");

// as you know from your own plotting experience
// there are plenty of settings that have to be applied to generate a beautiful plot
// most of these settings e.g. the number and alignment of pads, background colors, transparency, etc..
// stay the same for all (or a group of) your plots and you do not want to
// verbosely specify them again and again for every one of your plots
// therefore the framework provides the option to specify plot templates
// these are just ordinary plots, but you can use their settings as a baseline
// and the properties specified for your final plot will be applied on top of these

// for instance the following will give you some nice default plots:
vector<int16_t> goodColors = {kBlack, kBlue + 1, kRed + 1, kYellow + 1,
                              kMagenta - 4, kGreen + 3, kOrange + 1,
                              kViolet - 3, kCyan + 2, kPink + 3, kTeal - 7,
                              kMagenta + 1, kPink + 8, kCyan - 6,
                              kMagenta, kRed + 2, kGreen + 2,
                              kOrange + 2, kMagenta + 2, kYellow + 3,
                              kGray + 2, kBlue + 2, kYellow + 2,
                              kRed, kBlue, kMagenta + 3,
                              kGreen + 4, 28, 8, 15, 17, 12};
{ // -----------------------------------------------------------------------
  Plot plotTemplate("1d", "PLOT_TEMPLATES");
  plotTemplate.SetDimensions(710, 710, true);
  plotTemplate.SetTransparent();
  plotTemplate[0].SetFrameFill(10, 1001);
  plotTemplate[0].SetDefaultMarkerColors(goodColors);
  plotTemplate[0].SetDefaultLineColors(goodColors);
  plotTemplate[0].SetDefaultFillColors(goodColors);
  plotTemplate[0].SetDefaultFillStyles({0});
  plotTemplate[0].SetDefaultMarkerStyles({kFullCircle});
  plotTemplate[0].SetDefaultLineStyles({kSolid});
  plotTemplate[0].SetDefaultTextFont(43);
  plotTemplate[0].SetDefaultTextSize(24);
  plotTemplate[0].SetDefaultMarkerSize(1.);
  plotTemplate[0].SetDefaultLineWidth(3.);
  plotTemplate[0].SetDefaultDrawingOptionGraph(points);
  plotTemplate[0].SetTransparent();
  plotTemplate[0].SetMargins(0.07, 0.14, 0.12, 0.07);
  plotTemplate[0]['X'].SetTitleSize(28).SetTitleOffset(1.1).SetOppositeTicks().SetMaxDigits(3).SetNoExponent();
  plotTemplate[0]['Y'].SetTitleSize(28).SetTitleOffset(1.5).SetOppositeTicks().SetMaxDigits(3);
  plotTemplate[1].SetPosition(0., 0., 1., 1.);
  plotManager.AddPlotTemplate(plotTemplate);
} // -----------------------------------------------------------------------
{ // -----------------------------------------------------------------------
  Plot plotTemplate("1d_ratio", "PLOT_TEMPLATES");
  plotTemplate.SetDimensions(710, 710, true);
  plotTemplate.SetTransparent();
  plotTemplate[0].SetFrameFill(10, 1001);
  plotTemplate[0].SetDefaultLineColors(goodColors);
  plotTemplate[0].SetDefaultMarkerColors(goodColors);
  plotTemplate[0].SetDefaultFillColors(goodColors);
  plotTemplate[0].SetDefaultFillStyles({0});
  plotTemplate[0].SetDefaultMarkerStyles({kFullCircle});
  plotTemplate[0].SetDefaultLineStyles({kSolid});
  plotTemplate[0].SetDefaultMarkerSize(1.);
  plotTemplate[0].SetDefaultLineWidth(3.);
  plotTemplate[0].SetDefaultDrawingOptionGraph(points);
  plotTemplate[0].SetDefaultTextFont(43);
  plotTemplate[0].SetDefaultTextSize(24);
  plotTemplate[0].SetTransparent();
  plotTemplate[0]['X'].SetTitleSize(28).SetTitleOffset(1.1).SetOppositeTicks().SetMaxDigits(3);
  plotTemplate[0]['Y'].SetTitleSize(28).SetTitleOffset(1.5).SetOppositeTicks().SetMaxDigits(3);
  plotTemplate[1]['X'].SetTitleSize(0.).SetLabelSize(0.);
  plotTemplate[1].SetPosition(0., 0.28, 1., 1.);
  plotTemplate[1].SetMargins(0.05, 0.0, 0.14, 0.05);
  plotTemplate[2].SetPosition(0., 0., 1., 0.28);
  plotTemplate[2].SetMargins(0.015, 0.4, 0.14, 0.05);
  plotTemplate[2].SetRefFunc("1");
  plotTemplate[2]['X'].SetTickLength(0.06).SetNoExponent();
  plotTemplate[2]['Y'].SetNumDivisions(305).SetTitleCenter();
  plotManager.AddPlotTemplate(plotTemplate);
} // -----------------------------------------------------------------------
{ // -----------------------------------------------------------------------
  Plot plotTemplate("2d", "PLOT_TEMPLATES");
  plotTemplate.SetDimensions(710, 710, true);
  plotTemplate.SetTransparent();
  plotTemplate[0].SetFrameFill(10, 1001);
  plotTemplate[0].SetDefaultDrawingOptionHist2d(colz);
  plotTemplate[0].SetDefaultTextFont(43);
  plotTemplate[0].SetDefaultTextSize(24);
  plotTemplate[0].SetTransparent();
  plotTemplate[0].SetMargins(0.07, 0.14, 0.12, 0.18);
  plotTemplate[0]['X'].SetTitleSize(28).SetTitleOffset(1.2).SetMaxDigits(3);
  plotTemplate[0]['Y'].SetTitleSize(28).SetTitleOffset(1.2).SetMaxDigits(3);
  plotTemplate[0]['Z'].SetTitleSize(28).SetTitleOffset(1.6).SetMaxDigits(3);
  plotTemplate[0].SetRedrawAxes();
  plotTemplate[1].SetPosition(0., 0., 1., 1.);
  plotManager.AddPlotTemplate(plotTemplate);
} // -----------------------------------------------------------------------
// as you have seen, these template plots shall live in a figure group called "PLOT_TEMPLATES"
// and have to be added to the manager via plotManager.AddPlotTemplate(plotTemplate);

// once this is done, you can define plots based on these templates
// by specifying their name as the third argument in the new plots' constructor:
{ // -----------------------------------------------------------------------
  Plot plot("test1d_ratio", "myFigureGroup", "1d_ratio");

  plot[1].AddData("graph2", "inputIdentifierA", "5 TeV #Delta = #sqrt{s}")
  .SetMarker(kRed, kFullCircle, 1.2).SetMaxRangeX(40);

  plot[1].AddData("func3", "inputIdentifierB", "ratio")
  .SetMarker(kBlack, kFullCircle, 1.2).SetMaxRangeX(70);

  plot[2].AddRatio("invMassSpec", "inputIdentifierA", "momentUnfolded1", "pp_5TeV", "ratio")
  .SetMarker(kRed, kFullCircle, 1.2)
  .SetMaxRangeX(60);

  plot[1].AddLegend(0.,0.9);
  plot[2].AddLegend(0.,0.9);

  plot[0]['X'].SetRange(20, 70).SetTickOrientation("+-");

  plotManager.AddPlot(plot);
} // -----------------------------------------------------------------------
// as you can see the plot definition becomes simpler now since we
// do not have to specify again all of the layout overhead and can focus on the content

// for re-occurring data it can make sense to define a bunch of properties only once
// and then just re-use this layout for different plots:
using DataLayout = Plot::Pad::Data;
DataLayout pp_5TeV = DataLayout().SetOptions(curve).SetLineStyle(9).SetColor(kGreen+3);

// or if you want to logically group multiple layouts put them in a map:
enum layoutID : int { data, mc, };
using data_layout_t = map<layoutID, DataLayout>;

data_layout_t pp_7TeV
{
  {data, DataLayout()
    .SetInputID("pp_7TeV")
    .SetOptions(points)
    .SetMarkerStyle(kFullCircle)
    .SetColor(kBlue+2)
  },
  {mc, DataLayout()
    .SetInputID("pp_7TeV_MC")
    .SetOptions(curve)
    .SetLineStyle(kSolid)
    .SetColor(kRed+1)
  },
};

{ // -----------------------------------------------------------------------
  Plot plot("test1d", "myFigureGroup", "1d");
  plot[1].AddData("graph2", "inputIdentifierA").SetLayout(pp_5TeV);

  // if your data layout also contains an input identifier
  pp_5TeV.SetInputID("inputIdentifierA");
  // you can add the data and layout in the following way:
  plot[1].AddData("graph2", pp_5TeV);
  plot[1].AddData("graph2", pp_7TeV[data]);
  plot[1].AddData("graph2", pp_7TeV[mc]);

  // and this can be done in a similar manner also for ratios:
  plot[1].AddRatio("graph2", pp_5TeV, "graph3", "inputIdentifierA");

  plot[1].AddLegend(0.,0.9);
  plotManager.AddPlot(plot);
} // -----------------------------------------------------------------------

{ // -----------------------------------------------------------------------
  Plot plot("testColorGradients", "myFigureGroup", "1d");

  // some of the data properties (e.g. styles, colors and line widths) we may not want to specify for each of the data separately
  // therefore, as you have already seen in the definitions of the plot templates above, it is possible to define some default settings per pad
  // for instance you can define a list of colors that should be used for drawing the data:
  vector<int16_t> goodColors = {kBlack, kBlue+1, kRed+1, kYellow+1};
  plot[1].SetDefaultMarkerColors(goodColors);
  // this way the first data will be black, the second blue, etc.
  // if you add more data to the pad than the length of the defined color vector, it will continue with the first color in the vector

  // analogous implementations are available for marker, line or fill styles
  plot[1].SetDefaultMarkerStyles({kOpenCircle, kOpenCross});
  plot[1].SetDefaultLineStyles({kDashed, kSolid});

  // when drawing a lot of data to the plot, the color vector might become a bit lengthly
  // in these cases we often want a continuous color spectrum between some color endpoints
  // the following example shows how to generate a continuous rainbow color range that spans from blue, cyan, green and yellow to red
  vector<tuple<float_t, float_t, float_t, float_t>> rainbowColors =
  { {0., 0., 1., 0.},   // blue
    {0., 1., 1., 0.25}, // cyan
    {0., 1., 0., 0.50}, // green
    {1., 1., 0., 0.75}, // yellow
    {1., 0., 0., 1.}    // red
  };
  // here the first three numbers in the tuple correspond to rgb values (between 0. and 1.) of a color endpoint
  // while the last number specifies the 'location' of the respective point in the color range to be defined (again between 0. and 1.)
  plot[1].SetDefaultMarkerColors(rainbowColors);
  plot[1].SetDefaultLineColors(rainbowColors);

  plot[1].AddData("hist1", "inputIdentifierA");
  plot[1].AddData("hist2", "inputIdentifierA");
  plot[1].AddData("hist3", "inputIdentifierA");
  plot[1].AddData("hist4", "inputIdentifierA");
  plot[1].AddData("hist5", "inputIdentifierA");

  plotManager.AddPlot(plot);
} // -----------------------------------------------------------------------


// instead of creating the plots directly as we have done in the previous example,
// we can also save the plot definitions into another xml file via:
plotManager.DumpPlots("/path/to/plotDefinitions.XML");

// this file can then be read in by another one of your programs via
plotManager.ExtractPlotsFromFile("/path/to/plotDefinitions.XML");
// this extracts all plots defined in the file

// in case you want to load only certain plots:
plotManager.ExtractPlotsFromFile("/path/to/plotDefinitions.XML", "load", "(invMass|ptSpec)");
plotManager.ExtractPlotsFromFile("/path/to/plotDefinitions.XML", "load", "projBin_[1,5,7]");
plotManager.ExtractPlotsFromFile("/path/to/plotDefinitions.XML", "load", "projBin_[1-9]+");
plotManager.ExtractPlotsFromFile("/path/to/plotDefinitions.XML", "load", "peak_.*");

// ... or from specific figure groups:
plotManager.ExtractPlotsFromFile("/path/to/plotDefinitions.XML", "load", ".*" "figureGroup[1,2]", "myCategory");

// the plot name, figure group or category can be regular expressions

// as an alternative to the option "load" you can also use the modi find|interactive|pdf|eps|png|macro|file
// just give it a try and see what happens...

// the main reasoning behind the "dump-to-file" feature is that you can then
// make use of the builtin command-line plotting tool, which can conveniently
// create specific plots you defined and saved to file (see next section)
```

Using the App
-------------
The PlottingFramework ships with a builtin plotting application that enables you to quickly create plots from previously saved plot definitions.
To use the application, first source the `.plotrc` script in this repository.
In order to find the necessary files for creating your plots, the app needs to be configured according to your needs.
This configuration is done in the following way:
```
plot-config add <configName> inputFiles </path/to/inputFiles.XML>
plot-config add <configName> plotDefinitions </path/to/plotDefFile.XML>
plot-config add <configName> executable </path/to/executable>
plot-config add <configName> outputDir </path/to/output/dir>
```
Here `<configName>` refers to an arbitrary name you give this group of settings.
The first two settings are mandatory, while the rest is optional.
It is possible to add multiple configurations and switch between them via
```
plot-config switch <configName>
```
This can be useful for instance if you have multiple independent plotting programs which create different plots and need a separate set of input files.
All settings are saved in `~/.plotconfig.xml` (any other file path and name can be specified by setting the environment variable `__PLOTTING_CONFIG_FILE`).

Once this is set up, you can easily create plots via the command line from everywhere on your computer by typing
```
plot <figureGroup> <plotName> <mode>
```
Regular expressions for `<figureGroup>` and `<plotName>` are supported.
This means you can create multiple plots according to naming patterns. For example `plot paperPlots .+` would generate all plots defined within the figure group called `paperPlots` and `plot test "plot[1,2]"` would create `plot1` and `plot2` from the figure group `test`. Note that in the second example quotes are needed, since your shell will try to interprete some special characters (in this case `[]`) before they are passed to the plotting app.
To select only a sub-category within the figure group, use `<figureGroup/some/category>`.
By default, the optional `mode` argument is set to `interactive` and you can leave it out in the command.
Possible alternatives are: `find`, `pdf`, `eps`, `svg`, `png`, `gif`, `macro`, `file`.

For bash and zsh this program provides an auto-completion feature, this means you can tab through the available commands, figure groups and plot names.
Your `executable` (which creates the plot definitions) specified in the configuration
will automatically be rebuilt and (if needed) executed before the app generates the requested plot(s).
