#include "PlotManager.h"
#include "Logging.h"
#include <string>
#include <filesystem>
#define SRC_DIR std::filesystem::absolute(__FILE__).parent_path().string() + "/"

using std::string;
using std::vector;
using namespace SciRooPlot;
using Plot::Pad::Data;

void DefineDatasets(PlotManager& pm);
void DefineBasePlots(PlotManager& pm);
void DefinePlots(PlotManager& pm);

int main(int argc, char* argv[])
{
  PlotManager pm;
  DefineDatasets(pm);
  DefineBasePlots(pm);
  DefinePlots(pm);
  pm.SaveProject("PROJECT_NAME");
  return 0;
}

//****************************************************************************************
/**
 * Defines the input identifiers.
 */
//****************************************************************************************
void DefineDatasets(PlotManager& pm)
{
  const string inputFolder = "${SCIROOPLOT_DATA_DIR}/TestFiles";
  pm.AddDataset("measurements", {inputFolder + "/file1.root", inputFolder + "/file2.root"});
  pm.AddDataset("theory", inputFolder + "/subfolder/file3.root");
  pm.AddDataset("pseudodata", inputFolder + "/subfolder/file3.root:pseudodata");
  // NB.: path relative to this file:
  // pm.AddDataset("test", SRC_DIR + "../rel/path/to/file.root");
}

//****************************************************************************************
/**
 * Defines the base plots.
 */
//****************************************************************************************
void DefineBasePlots(PlotManager& pm)
{
  {
    Plot plot = PlotManager::MakeBasePlot("1d");
    pm.AddBasePlot(plot);
  }
  {
    Plot plot = PlotManager::MakeBasePlot("1d_ratio");
    pm.AddBasePlot(plot);
  }
  {
    Plot plot = PlotManager::MakeBasePlot("2d");
    pm.AddBasePlot(plot);
  }
}

//****************************************************************************************
/**
 * Defines the plots.
 */
//****************************************************************************************
void DefinePlots(PlotManager& pm)
{
  // logging helpers (INFO, DEBUG, WARNING, ERROR, PRINT) can be used as follows:
  INFO("Welcome to SciRooPlot. Working in directory {}", SRC_DIR);
  {  // -----------------------------------------------------------------------
    Plot plot("plot1", "dummyFigures", "1d");
    plot[1].AddData("hist3", "measurements", "gaus");
    plot[1].AddData("hist1", "measurements", "anothergaus");
    plot[1].AddData("hist7", "theory", "prediction");
    plot[1].AddLegend();
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------
  {  // -----------------------------------------------------------------------
    Plot plot("plot2", "dummyFigures", "1d");
    plot[1].AddData("dir2/subdir2/hist3", "measurements");
    plot[1].AddData("list1/hist5", "measurements");
    pm.AddPlot(plot);
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
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------
  {  // -----------------------------------------------------------------------
    Plot plot("invMassDist", "examples", "1d");
    plot[1].AddData("invMassDist", "pseudodata").Scale(0.00001).SetColor(kBlue + 2);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------
  {  // -----------------------------------------------------------------------
    Plot plot("multDist", "examples", "1d");
    plot[1].AddData("multDist", "pseudodata");
    plot[1].AddText("My Experiment // pp collisions, X TeV");
    plot[1]['Y'].SetLog();
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------
  {  // -----------------------------------------------------------------------
    Plot plot("ptSpec", "examples", "1d");
    plot[1].AddData("ptSpec", "pseudodata").SetColor(kRed + 2);
    plot[1]['X'].SetLog();
    plot[1]['Y'].SetLog();
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------
}
