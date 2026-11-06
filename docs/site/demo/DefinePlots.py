from SciRooPlot import *

# example data shipped with SciRooPlot
path = "${SCIROOPLOT_DATA_DIR}/ExampleData"
experiment = "Example Experiment"

pm = PlotManager("my-analysis")
pm.AddDataSource("leptons", path + "/higgs.root:fourLepton")
pm.AddDataSource("photons", path + "/higgs.root:diphoton")
pm.AddDataSource("limits", path + "/higgs.root:limits")
for layout in ["1d", "1d_ratio"]:
    pm.AddBasePlot(PlotManager.MakeBasePlot(layout))

# four leptons: expectation as areas, data as points
plot = Plot("massFourLepton", "higgs", "1d")
pad = plot[1]
sig = pad.AddData("m4l_sigPlusBkg", "leptons", "bkg. + H(125)")
sig.SetDrawOption(hist).SetLine(kSolid, 3.0, kRed + 1)
bkg = pad.AddData("m4l_background", "leptons", "background")
bkg.SetDrawOption(hist).SetFillStyle(1001)
bkg.SetColor(kAzure - 9)
data = pad.AddData("m4l_data", "leptons", "data")
data.SetColor(kBlack).SetMarkerStyle(kFullCircle)
pad.AddText(0.06, 0.93, [experiment, "H #rightarrow 4l"])
pad.AddLegend(0.06, 0.75)
pad['Y'].SetRange(0.0, 40.0)
pm.AddPlot(plot)

# two photons: fits, and data divided by the background
plot = Plot("massDiphoton", "higgs", "1d_ratio")
pad = plot[1]
pad.AddData("mgg_data", "photons", "data")
fit = pad.AddData("mgg_sigBkgFit", "photons", "signal + bkg.")
fit.SetLine(kSolid, 3.0, kRed + 1)
fit = pad.AddData("mgg_bkgFit", "photons", "bkg. only")
fit.SetLine(kDashed, 3.0, kBlue + 1)
pad.AddText(0.52, 0.95,
            [experiment, "H #rightarrow #gamma#gamma"])
pad.AddLegend(0.52, 0.74)
plot[2].AddRatio(["mgg_data", "photons"], ["mgg_bkgFit"])
plot[2]['Y'].SetTitle("data / bkg.").SetRange(0.9, 1.15)
pm.AddPlot(plot)

# limits: expected bands, observed points, the SM at 1
plot = Plot("brazilPlot", "higgs", "1d")
pad = plot[1]
two = pad.AddData("band2sigma", "limits", "#pm 2#sigma")
two.SetDrawOption(band).SetFillStyle(1001).SetColor(kOrange)
one = pad.AddData("band1sigma", "limits", "#pm 1#sigma")
one.SetDrawOption(band).SetFillStyle(1001).SetColor(kGreen + 1)
for sigma in [two, one]:  # in the legend as filled boxes
    sigma.GetLegendEntry().SetSymbolElements("F")
exp = pad.AddData("expected", "limits", "expected")
exp.SetDrawOption(line).SetLine(kDashed, 3.0, kBlack)
obs = pad.AddData("observed", "limits", "observed")
obs.SetDrawOption(points_line).SetColor(kBlack)
obs.SetMarkerStyle(kFullCircle).SetMarkerSize(0.8)
pad.SetRefFunc("1").SetLine(kSolid, 3.0, kRed + 1)
pad.AddLegend(0.06, 0.93)
pad['Y'].SetLog().SetRange(0.08, 20.0)
pm.AddPlot(plot)

pm.SaveProject()
