#include "PlotManager.h"
#include "Logging.h"
#include <string>

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
  const string inputFolder = "${__SCIROOPLOT_BUILD_DIR}/TestFiles";
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
  {  // -----------------------------------------------------------------------
    Plot plotTemplate("myOwnLayout", "PLOT_TEMPLATES");
    plotTemplate.SetDimensions(710, 710, true);
    plotTemplate.SetTransparent();
    plotTemplate[0].SetFrameFill(10, 1001);
    plotTemplate[0].SetDefaultMarkerColors({kRed, kBlue, kOrange});
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
    plotTemplate[1].SetFrameBorderColor(kRed);
    plotTemplate[1].SetFrameBorderWidth(5.);
    plotTemplate[1]['X'].SetColor(kBlue);
    plotTemplate[1]['Y'].SetColor(kBlue);
    plotManager.AddPlotTemplate(plotTemplate);
  }  // -----------------------------------------------------------------------
}

//****************************************************************************************
/**
 * Defines the plots.
 */
//****************************************************************************************
void DefinePlots(PlotManager& plotManager)
{
  {  // -----------------------------------------------------------------------
    string plotName = "plot1";
    Plot plot(plotName, "dummyFigures", "1d");
    plot[1].AddData("hist3", "measurements", "gaus");
    plot[1].AddData("hist1", "measurements", "anothergaus");
    plot[1].AddData("hist7", "theory", "prediction");
    plot[1].AddLegend();
    plotManager.AddPlot(plot);
  }  // -----------------------------------------------------------------------
  {  // -----------------------------------------------------------------------
    string plotName = "plot2";
    Plot plot(plotName, "dummyFigures", "myOwnLayout");
    plot[1].AddData("dir2/subdir2/hist3", "measurements");
    plot[1].AddData("list1/hist5", "measurements");
    plotManager.AddPlot(plot);
  }  // -----------------------------------------------------------------------
  {  // -----------------------------------------------------------------------
    string plotName = "invMassDist";
    Plot plot(plotName, "examples", "1d");
    plot[1].AddData("invMassDist", "pseudodata").SetScaleFactor(0.00001).SetColor(kBlue + 2);
    plotManager.AddPlot(plot);
  }  // -----------------------------------------------------------------------
  {  // -----------------------------------------------------------------------
    string plotName = "multDist";
    Plot plot(plotName, "examples", "1d");
    plot[1].AddData("multDist", "pseudodata");
    plot[1].AddText("My Experiment // pp collisions, X TeV");
    plot[1]['Y'].SetLog();
    plotManager.AddPlot(plot);
  }  // -----------------------------------------------------------------------
  {  // -----------------------------------------------------------------------
    string plotName = "ptSpec";
    Plot plot(plotName, "examples", "1d");
    plot[1].AddData("ptSpec", "pseudodata").SetColor(kRed + 2);
    plot[1]['X'].SetLog();
    plot[1]['Y'].SetLog();
    plotManager.AddPlot(plot);
  }  // -----------------------------------------------------------------------
}
