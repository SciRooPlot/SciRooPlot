#include "PlotManager.h"
#include "Logging.h"
#include <string>

using std::string;
using namespace SciRooPlot;
void DefineInputIdentifiers(PlotManager& plotManager);
void DefinePlotTemplates(PlotManager& plotManager);
void DefinePlots(PlotManager& plotManager);

int main(int argc, char* argv[])
{
  PlotManager plotManager;
  DefineInputIdentifiers(plotManager);
  DefinePlotTemplates(plotManager);
  DefinePlots(plotManager);
  plotManager.SaveProject("PROJECT_NAME");
  return 0;
}

//****************************************************************************************
/**
 * Defines the input identifiers.
 */
//****************************************************************************************
void DefineInputIdentifiers(PlotManager& plotManager)
{
  const string inputFolder = "${SCIROOPLOT_DATA_DIR}/TestFiles";
  plotManager.AddInputDataFiles("measurements", {inputFolder + "/file1.root", inputFolder + "/file2.root"});
  plotManager.AddInputDataFiles("theory", {inputFolder + "/subfolder/file3.root"});
  plotManager.AddInputDataFiles("pseudodata", {inputFolder + "/subfolder/file3.root:pseudodata"});
}

//****************************************************************************************
/**
 * Defines the plot templates.
 */
//****************************************************************************************
void DefinePlotTemplates(PlotManager& plotManager)
{
  {
    auto plotTemplate = PlotManager::GetPlotTemplate("1d");
    plotManager.AddPlotTemplate(plotTemplate);
  }
  {
    auto plotTemplate = PlotManager::GetPlotTemplate("1d_ratio");
    plotManager.AddPlotTemplate(plotTemplate);
  }
  {
    auto plotTemplate = PlotManager::GetPlotTemplate("2d");
    plotManager.AddPlotTemplate(plotTemplate);
  }
}

//****************************************************************************************
/**
 * Defines the plots.
 */
//****************************************************************************************
void DefinePlots(PlotManager& plotManager)
{
  {  // -----------------------------------------------------------------------
    Plot plot("plot1", "dummyFigures", "1d");
    plot[1].AddData("hist3", "measurements", "gaus");
    plot[1].AddData("hist1", "measurements", "anothergaus");
    plot[1].AddData("hist7", "theory", "prediction");
    plot[1].AddLegend();
    plotManager.AddPlot(plot);
  }  // -----------------------------------------------------------------------
  {  // -----------------------------------------------------------------------
    Plot plot("plot2", "dummyFigures", "1d");
    plot[1].AddData("dir2/subdir2/hist3", "measurements");
    plot[1].AddData("list1/hist5", "measurements");
    plotManager.AddPlot(plot);
  }  // -----------------------------------------------------------------------
  {  // -----------------------------------------------------------------------
    Plot plot("plot3", "dummyFigures", "1d_ratio");
    plot[1].AddData("dir2/subdir1/mygausrand", "measurements", "gaus random");
    plot[1].AddData("dir2/subdir1/mygaus", "measurements", "gaus function");
    plot[2].AddRatio("dir2/subdir1/mygausrand", "measurements", "dir2/subdir1/mygaus", "measurements");
    plot[2]['Y'].SetTitle("rand / func");
    plot[2]['X'].SetTitle("my X title");
    plot[1]['Y'].SetRange(0.04, 0.45);
    plot[0]['X'].SetRange(-2, 2);
    plotManager.AddPlot(plot);
  }  // -----------------------------------------------------------------------
  {  // -----------------------------------------------------------------------
    Plot plot("invMassDist", "examples", "1d");
    plot[1].AddData("invMassDist", "pseudodata").SetScaleFactor(0.00001).SetColor(kBlue + 2);
    plotManager.AddPlot(plot);
  }  // -----------------------------------------------------------------------
  {  // -----------------------------------------------------------------------
    Plot plot("multDist", "examples", "1d");
    plot[1].AddData("multDist", "pseudodata");
    plot[1].AddText("My Experiment // pp collisions, X TeV");
    plot[1]['Y'].SetLog();
    plotManager.AddPlot(plot);
  }  // -----------------------------------------------------------------------
  {  // -----------------------------------------------------------------------
    Plot plot("ptSpec", "examples", "1d");
    plot[1].AddData("ptSpec", "pseudodata").SetColor(kRed + 2);
    plot[1]['X'].SetLog();
    plot[1]['Y'].SetLog();
    plotManager.AddPlot(plot);
  }  // -----------------------------------------------------------------------
}
