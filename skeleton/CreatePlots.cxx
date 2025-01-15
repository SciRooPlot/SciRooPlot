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

  auto [inputFiles, plotDefinitions, outputDir] = PlotManager::ReadConfig("PROJECT_NAME");
  if(inputFiles.empty() || plotDefinitions.empty()){
    ERROR("Configuration could not be loaded properly.");
    return 1;
  }
  plotManager.DumpInputDataFiles(inputFiles);
  plotManager.DumpPlots(plotDefinitions);
  return 0;
}

//****************************************************************************************
/**
 * Defines the input identifiers.
 */
//****************************************************************************************
void DefineInputIdentifiers(PlotManager& plotManager)
{
  plotManager.AddInputDataFiles("inputIdentifier1", {"/path/to/file1.root", "/path/to/file2.root"});
  plotManager.AddInputDataFiles("inputIdentifier2", {"/path/to/file3.root", "/path/to/file4.root", "/path/to/file5.root"});
  /*
  // IMPORTANT: best use absolute paths so the 'plot' command will work from any location on your computer
   // you can also use environment variables:
   if (!gSystem->Getenv("DATASETS_BASE_DIR")) {
     ERROR("Please specify datasets base dir: export DATASETS_BASE_DIR=/path/to/datasets");
   }
   plotManager.AddInputDataFiles("inputIdentifier1", {"${DATASETS_BASE_DIR}/subfolder/file1.root", "${DATASETS_BASE_DIR}/subfolder2/file2.root"});
  */
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
