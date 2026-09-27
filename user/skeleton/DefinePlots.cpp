#include "SciRooPlot/Logging.h"
#include "SciRooPlot/PlotManager.h"

#include <filesystem>
#include <string>
#include <vector>
#define SRC_DIR std::filesystem::absolute(__FILE__).parent_path().string() + "/"

using std::string;
using std::vector;
using namespace SciRooPlot;
using Data = Plot::Pad::Data;

// Plots are created with `plot <group> <name> [mode]`, e.g. `plot myGroup myFirstPlot pdf`.
// A project with many commented examples (and example data) is created with `srp example-cpp <project>`.

void DefineDataSources(PlotManager& pm)
{
  // pm.AddDataSource("myData", "/path/to/file.root");                     // whole file
  // pm.AddDataSource("myList", "/path/to/file.root:dir/list");            // directory or TList inside the file
  // pm.AddDataSource("merged", {"/path/to/a.root", "/path/to/b.root"});   // several files, first match wins
  // pm.AddDataSource("table", "/path/to/table.csv");                      // trees and tables work the same way
  // pm.AddDataSource("local", SRC_DIR + "../rel/path/to/file.root");      // relative to this file
}

void DefineBasePlots(PlotManager& pm)
{
  pm.AddBasePlot(PlotManager::MakeBasePlot("1d"));
  pm.AddBasePlot(PlotManager::MakeBasePlot("1d_ratio"));
  pm.AddBasePlot(PlotManager::MakeBasePlot("2d"));
}

void DefinePlots(PlotManager& pm)
{
  {  // -----------------------------------------------------------------------------------
    Plot plot("myFirstPlot", "myGroup", "1d");
    plot[1].AddFunction("sin(x)/x", "sin(#it{x})/#it{x}");
    // plot[1].AddData("histName", "myData", "label").SetColor(kRed + 1);
    // plot[1].AddData("dir/histName", "myData").Normalize().SetOptions(hist);
    // plot[1].AddData("treeName", "myData").Project1D({"pt", 100, {0., 10.}}).Filter("abs(eta) < 0.8");
    // plot[1].AddRatio("numerator", "myData", "denominator", "myData").SetIsCorrelated();
    plot[1].AddLegend(0.6, 0.9);  // upper left corner in pad coordinates, or AddLegend() to place it automatically
    // plot[1].AddText("My Experiment // 13.6 TeV");
    plot[1]['X'].SetTitle("#it{x}").SetRange(0.1, 20.);
    plot[1]['Y'].SetRange(-0.3, 1.1);  // or e.g. .SetLog()
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
}

int main(int argc, char* argv[])
{
  PlotManager pm("PROJECT_NAME");
  DefineDataSources(pm);
  DefineBasePlots(pm);
  DefinePlots(pm);
  pm.SaveProject();
  return 0;
}
