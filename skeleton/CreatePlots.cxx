#include "PlotManager.h"
#include "Logging.h"

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
  plotManager.AddInputDataFiles("inputIdentifier1", {"${HOME}/path/to/file1.root", "${HOME}/path/to/file2.root"});
  plotManager.AddInputDataFiles("inputIdentifier2", {"${HOME}/subdir/file3.root", "${HOME}/file4.root", "${HOME}/file5.root"});
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
  { // -----------------------------------------------------------------------
    string plotName = "plot1";
    Plot plot(plotName, "test", "1d");
    plot[1].AddData("myData1", "inputIdentifier1", "title");
    plot[1].AddData("myData2", "inputIdentifier2", "title2");
    plot[1].AddLegend();
    plotManager.AddPlot(plot);
  } // -----------------------------------------------------------------------
}
