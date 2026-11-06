#include "SciRooPlot/PlotManager.h"

#include <string>

using namespace SciRooPlot;

int main()
{
  // example data shipped with SciRooPlot
  std::string path = "${SCIROOPLOT_DATA_DIR}/ExampleData";
  std::string experiment = "Example Experiment";

  PlotManager pm("my-analysis");
  pm.AddDataSource("leptons", path + "/higgs.root:fourLepton");
  pm.AddDataSource("photons", path + "/higgs.root:diphoton");
  pm.AddDataSource("limits", path + "/higgs.root:limits");
  for (const auto* layout : {"1d", "1d_ratio"}) {
    pm.AddBasePlot(PlotManager::MakeBasePlot(layout));
  }

  {  // four leptons: expectation as areas, data as points
    Plot plot("massFourLepton", "higgs", "1d");
    auto& pad = plot[1];
    pad.AddData("m4l_sigPlusBkg", "leptons", "bkg. + H(125)")
      .SetDrawOption(hist)
      .SetLine(kSolid, 3.0, kRed + 1);
    pad.AddData("m4l_background", "leptons", "background")
      .SetDrawOption(hist)
      .SetFillStyle(1001)
      .SetColor(kAzure - 9);
    pad.AddData("m4l_data", "leptons", "data")
      .SetColor(kBlack)
      .SetMarkerStyle(kFullCircle);
    pad.AddText(0.06, 0.93, {experiment, "H #rightarrow 4l"});
    pad.AddLegend(0.06, 0.75);
    pad['Y'].SetRange(0.0, 40.0);
    pm.AddPlot(plot);
  }

  {  // two photons: fits, and data divided by the background
    Plot plot("massDiphoton", "higgs", "1d_ratio");
    auto& pad = plot[1];
    pad.AddData("mgg_data", "photons", "data");
    pad.AddData("mgg_sigBkgFit", "photons", "signal + bkg.")
      .SetLine(kSolid, 3.0, kRed + 1);
    pad.AddData("mgg_bkgFit", "photons", "bkg. only")
      .SetLine(kDashed, 3.0, kBlue + 1);
    pad.AddText(0.52, 0.95,
                {experiment, "H #rightarrow #gamma#gamma"});
    pad.AddLegend(0.52, 0.74);
    plot[2].AddRatio({"mgg_data", "photons"}, {"mgg_bkgFit"});
    plot[2]['Y'].SetTitle("data / bkg.").SetRange(0.9, 1.15);
    pm.AddPlot(plot);
  }

  {  // limits: expected bands, observed points, the SM at 1
    Plot plot("brazilPlot", "higgs", "1d");
    auto& pad = plot[1];
    pad.AddData("band2sigma", "limits", "#pm 2#sigma")
      .SetDrawOption(band)
      .SetFillStyle(1001)
      .SetColor(kOrange)
      .GetLegendEntry()
      .SetSymbolElements("F");  // a box in the legend
    pad.AddData("band1sigma", "limits", "#pm 1#sigma")
      .SetDrawOption(band)
      .SetFillStyle(1001)
      .SetColor(kGreen + 1)
      .GetLegendEntry()
      .SetSymbolElements("F");
    pad.AddData("expected", "limits", "expected")
      .SetDrawOption(line)
      .SetLine(kDashed, 3.0, kBlack);
    pad.AddData("observed", "limits", "observed")
      .SetDrawOption(points_line)
      .SetColor(kBlack)
      .SetMarkerStyle(kFullCircle)
      .SetMarkerSize(0.8);
    pad.SetRefFunc("1").SetLine(kSolid, 3.0, kRed + 1);
    pad.AddLegend(0.06, 0.93);
    pad['Y'].SetLog().SetRange(0.08, 20.0);
    pm.AddPlot(plot);
  }

  pm.SaveProject();
  return 0;
}
