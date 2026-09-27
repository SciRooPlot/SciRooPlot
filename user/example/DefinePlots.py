from SciRooPlot import *
from pathlib import Path

SRC_DIR = str(Path(__file__).resolve().parent) + "/"

# The plots below are organised in groups and subgroups (higgs/diphoton, detector/pid, ...).
# Each plot is produced with `plot <group> <name> [mode]`, where group and name are regular expressions:
#   plot higgs/diphoton massSpectrum      one plot
#   plot higgs .+ pdf                     all plots of the group "higgs" and its subgroups as PDF
#   plot detector '.*(Eff|Res).*'         every plot in "detector" whose name contains Eff or Res

# labels shown on most plots
EXPERIMENT = "Example Experiment"
COLLISIONS = "pp, #sqrt{#it{s}} = 13.6 TeV"


def DefineDataSources(pm: PlotManager):
    """Data sources: a short name for one or more input files, used when adding data to a plot."""
    # pseudo-data shipped with SciRooPlot; replace these with your own files
    inputFolder = "${SCIROOPLOT_DATA_DIR}/ExampleData"

    # a whole file: objects are addressed by their path inside the file, e.g. "diphoton/mgg_data"
    pm.AddDataSource("higgs", inputFolder + "/higgs.root")
    # "file.root:path" makes a directory (or TList) inside the file the entry point
    pm.AddDataSource("data", inputFolder + "/spectra.root:data")
    pm.AddDataSource("mc", inputFolder + "/spectra.root:mc")
    # typical analysis output: one TList per task inside a directory
    for task in ["EventQA", "TriggerQA", "TrackQA", "PIDQA", "CaloQA"]:
        pm.AddDataSource(task, inputFolder + "/AnalysisResults_data.root:" + task + "/output")
        pm.AddDataSource(task + "_MC", inputFolder + "/AnalysisResults_mc.root:" + task + "/output")
    # trees and tables (.csv, .txt, .dat, ...) are turned into histograms or graphs when plotted
    pm.AddDataSource("tracks", inputFolder + "/tracks.root")
    pm.AddDataSource("published", inputFolder + "/ptSpec_published.csv")
    pm.AddDataSource("testbeam", inputFolder + "/testbeam.csv")
    pm.AddDataSource("luminosity", inputFolder + "/luminosity.csv")
    pm.AddDataSource("runQA", inputFolder + "/runQA.csv")
    pm.AddDataSource("systematics", inputFolder + "/systematics.csv")
    pm.AddDataSource("dimuon", inputFolder + "/dimuon.root")
    # a directory: all files in it (and its subdirectories) form one data source
    pm.AddDataSource("jets", inputFolder + "/jets/")
    # several files: they are searched in the given order, the first file containing an object wins
    pm.AddDataSource("heavyion", [inputFolder + "/heavyion.root", inputFolder + "/raa_published.csv"])
    # in-memory objects created in this program are stored with the project
    # mean number of binary nucleon-nucleon collisions per centrality class (illustrative Glauber values)
    pm.AddDataSource("glauber", graph("ncoll", [5.0, 20.0, 40.0], [1500.0, 750.0, 260.0]))

    # more options:
    # pm.AddDataSource("merged", ["/path/to/a.root", "/path/to/b.root"])  # several files, first match wins
    # pm.AddDataSource("folder", "/path/to/folder/")                      # all files in a directory
    # pm.AddDataSource("local", SRC_DIR + "../rel/path/to/file.root")     # relative to this file
    # pm.AddDataSource("numbers", histo("myHist", [1.2, 3.4, 2.2]))       # histograms can be made in-memory, too


def DefineBasePlots(pm: PlotManager):
    """Base plots: layouts that the plots build upon (third argument of the Plot constructor)."""
    pm.AddBasePlot(PlotManager.MakeBasePlot("1d"))
    pm.AddBasePlot(PlotManager.MakeBasePlot("1d_ratio"))  # main pad and a ratio pad
    pm.AddBasePlot(PlotManager.MakeBasePlot("2d"))  # room for the colour scale
    # a base plot can be adapted before adding it, e.g. for all 2d plots with a logarithmic z axis
    plot2dLog = PlotManager.MakeBasePlot("2d")
    plot2dLog.SetName("2d_logz")
    plot2dLog[1]["Z"].SetLog()
    pm.AddBasePlot(plot2dLog)
    # a wide layout, e.g. for trends vs. run number: same height (and therefore text sizes) as "1d"
    wide = PlotManager.MakeBasePlot("1d")
    wide.SetName("wide")
    wide.SetDimensions(1024, 788, True)  # the PDF is at most 1.3 times wider than high
    wide[0].SetMargins(0.04, 0.15, 0.13, 0.03)
    pm.AddBasePlot(wide)
    pm.AddBasePlot(PlotManager.MakeBasePlot("1d_3panels"))  # three panels side by side


def DefineHiggsPlots(pm: PlotManager):
    """Higgs boson: invariant-mass spectra, fits, expected vs. observed, exclusion limits."""
    # data with fit functions, and a ratio pad dividing the data by the background fit ------------
    plot = Plot("massSpectrum", "higgs/diphoton", "1d_ratio")
    plot[1].AddData("diphoton/mgg_data", "higgs", "data")
    plot[1].AddData("diphoton/mgg_sigBkgFit", "higgs", "signal + background fit").SetLine(kRed + 1, kSolid, 3.0)
    plot[1].AddData("diphoton/mgg_bkgFit", "higgs", "background fit").SetLine(kBlue + 1, kDashed, 3.0)
    plot[1].AddText(0.45, 0.9, EXPERIMENT + " // " + COLLISIONS + ", 140 fb^{-1} // H #rightarrow #gamma#gamma")
    plot[1].AddLegend(0.45, 0.62)  # upper left corner of the box in pad coordinates
    plot[2].AddRatio("diphoton/mgg_data", "higgs", "diphoton/mgg_bkgFit", "higgs")  # histogram / function
    plot[2]["Y"].SetTitle("data / bkg.").SetRange(0.9, 1.15)
    pm.AddPlot(plot)

    # normalised shape; <mean> etc. in a label are replaced by properties of the data -------------
    plot = Plot("signalShape", "higgs/diphoton", "1d")
    plot[1].AddData("diphoton/mgg_signalMC", "higgs", "simulation, mean = <mean[.2f]> GeV").Normalize().SetOptions(hist).SetColor(kRed + 1)
    plot[1].AddLegend(0.18, 0.9)
    plot[1]["Y"].SetTitle("normalised counts").SetRange(0.0, 0.05)
    pm.AddPlot(plot)

    # expectation as a filled area, data as points ------------------------------------------------
    plot = Plot("massSpectrum", "higgs/fourLepton", "1d")
    plot[1].AddData("fourLepton/m4l_sigPlusBkg", "higgs", "background + H(125)").SetOptions(hist).SetLine(kRed + 1, kSolid, 3.0)
    plot[1].AddData("fourLepton/m4l_background", "higgs", "background (Z#rightarrow4l, ZZ*, Z+jets)").SetOptions(hist).SetFill(kAzure - 9, 1001).SetLineColor(kAzure - 9)
    plot[1].AddData("fourLepton/m4l_data", "higgs", "data").SetMarker(kBlack, kFullCircle, 1.2)
    plot[1].AddText(0.2, 0.9, EXPERIMENT + " // H #rightarrow ZZ* #rightarrow 4l")
    plot[1].AddLegend(0.2, 0.76)
    plot[1]["Y"].SetRange(0.0, 40.0)
    pm.AddPlot(plot)

    # "Brazil plot": uncertainty bands, a reference line, the first data defines the frame --------
    plot = Plot("brazilPlot", "higgs/limits", "1d")
    plot[1].AddData("limits/band2sigma", "higgs", "expected #pm 2#sigma").SetOptions(band).SetFill(kOrange, 1001).SetLineColor(kOrange)
    plot[1].AddData("limits/band1sigma", "higgs", "expected #pm 1#sigma").SetOptions(band).SetFill(kGreen + 1, 1001).SetLineColor(kGreen + 1)
    plot[1].AddData("limits/expected", "higgs", "expected").SetOptions(line).SetLine(kBlack, kDashed, 3.0)
    plot[1].AddData("limits/observed", "higgs", "observed").SetOptions(points_line).SetMarker(kBlack, kFullCircle, 0.8)
    plot[1].SetRefFunc("1").SetLine(kRed + 1, kSolid, 2.0)  # SM expectation
    # legend entries follow the data by default; here the bands should appear as filled boxes
    legend = plot[1].AddLegend(0.2, 0.9)
    legend.GetEntry(1).SetDrawStyle("F")
    legend.GetEntry(2).SetDrawStyle("F")
    plot[1]["Y"].SetLog().SetRange(0.08, 20.0)
    pm.AddPlot(plot)


def DefineSpectraPlots(pm: PlotManager):
    """Charged-particle spectra: data vs. simulation, a published reference, 2d data and projections."""
    # layouts: properties defined once and reused whenever data is added with them
    dataStyle = Data().SetDataSource("data").SetMarker(kBlack, kFullCircle, 1.0)
    mcStyle = Data().SetDataSource("mc").SetOptions(hist).SetLine(kRed + 1, kSolid, 3.0)

    plot = Plot("ptSpectrum", "spectra", "1d")
    plot[1].AddData("ptSpec", dataStyle, "data")
    plot[1].AddData("ptSpec", mcStyle, "simulation")
    plot[1].AddLegend()
    plot[1].AddText(EXPERIMENT + " // " + COLLISIONS + " // |#eta| < 0.8")
    plot[1]["X"].SetLog()
    plot[1]["Y"].SetLog()
    pm.AddPlot(plot)

    # a copy of the plot above, extended by a ratio pad
    ratioPlot = Plot(plot, "ptSpectrumRatio", "spectra")
    ratioPlot.SetBasePlot("1d_ratio")
    ratioPlot[2].AddRatio("ptSpec", "data", "ptSpec", "mc")
    ratioPlot[2]["Y"].SetTitle("data / sim.").SetRange(0.7, 1.3)
    ratioPlot[0]["X"].SetLog().SetRange(0.15, 15.0)  # pad 0 holds settings shared by all pads
    pm.AddPlot(ratioPlot)

    # table columns become a graph; any C++ expression of the columns can be used -----------------
    plot = Plot("ptSpectrumPublished", "spectra", "1d")
    plot[1].AddData("ptSpec", dataStyle, "this analysis")
    plot[1].AddData("ptSpec_published", "published", "published (stat. #oplus syst.)").Scatter("pt", "value", "pt_err", "sqrt(stat*stat + syst*syst)").SetMarker(kAzure + 2, kOpenSquare, 1.6)
    plot[1].AddLegend()
    plot[1]["X"].SetLog()
    plot[1]["Y"].SetLog()
    pm.AddPlot(plot)

    plot = Plot("multiplicity", "spectra", "1d")
    plot[1].AddData("multDist", dataStyle, "data, #LT#it{N}_{ch}#GT = <mean[.1f]>")
    plot[1].AddData("multDist", mcStyle, "simulation, #LT#it{N}_{ch}#GT = <mean[.1f]>")
    plot[1].AddLegend()
    plot[1]["X"].SetRange(0.0, 60.0)
    plot[1]["Y"].SetLog()
    pm.AddPlot(plot)

    plot = Plot("ptVsMult", "spectra", "2d_logz")
    plot[1].AddData("ptVsMult", "data")
    pm.AddPlot(plot)

    # projections of a 2d histogram in a loop, each divided by the lowest class -------------------
    plot = Plot("ptInMultClasses", "spectra", "1d_ratio")
    classes = [(1, 10), (11, 20), (21, 30), (31, 45), (46, 60)]
    # a colour gradient from blue to red, spread over as many colours as there are classes
    gradient = [(0.1, 0.2, 0.8, 0.0), (0.8, 0.1, 0.1, 1.0)]
    plot[0].SetDefaultColors(gradient, None, len(classes))
    for low, high in classes:
        label = f"{low} #leq #it{{N}}_{{ch}} #leq {high}"
        # ProjectY(from, to, True): project onto y for x in [from, to] (in axis units, not bins)
        plot[1].AddData("ptVsMult", "data", label).ProjectY(low, high, True).Normalize()
        if low > 1:
            # Numer() / Denom() / Both(): the following settings act on numerator, denominator or both
            plot[2].AddRatio("ptVsMult", "data", "ptVsMult", "data").Numer().ProjectY(low, high, True).Denom().ProjectY(1, 10, True).Both().Normalize()
    plot[1].AddLegend()
    plot[1]["Y"].SetLog().SetTitle("normalised counts")
    plot[2]["Y"].SetTitle("ratio to lowest").SetRange(0.0, 6.0)
    pm.AddPlot(plot)

    # Cumulative(False) sums from the right: the probability to find at least N_ch particles ----
    plot = Plot("multiplicityCumulative", "spectra", "1d")
    plot[1].AddData("multDist", dataStyle, "data").Cumulative(False)
    plot[1].AddData("multDist", mcStyle, "simulation").Cumulative(False)
    plot[1].AddLegend()
    plot[1]["X"].SetRange(0.0, 60.0)
    plot[1]["Y"].SetLog().SetTitle("#it{P}(#geq #it{N}_{ch})")
    pm.AddPlot(plot)

    # ProjectX(from, to, True): multiplicity of events with a track in a given pT range ------------
    plot = Plot("multiplicityForHighPt", "spectra", "1d")
    plot[1].AddData("ptVsMult", "data", "#it{p}_{T} < 1 GeV/#it{c}").ProjectX(0.0, 0.99, True).Normalize()
    plot[1].AddData("ptVsMult", "data", "2 < #it{p}_{T} < 5 GeV/#it{c}").ProjectX(2.0, 4.99, True).Normalize()
    plot[1].AddLegend(0.45, 0.9)
    plot[1]["Y"].SetTitle("normalised counts")
    pm.AddPlot(plot)

    # the same 2d histogram shown in 3d, as contours and as candles ------------------------------
    plot3d = Plot("ptVsMultLego", "spectra/2dViews", "2d")
    plot3d[1].AddData("ptVsMult", "data").RebinXY(4, 2).SetOptions(lego)
    plot3d[1].SetView(35.0, 225.0)  # viewing angles theta and phi in degrees
    pm.AddPlot(plot3d)

    plotContours = Plot("ptVsMultContours", "spectra/2dViews", "2d_logz")
    plotContours[1].AddData("ptVsMult", "data").SetOptions(contz).SetContours(12)
    plotContours[1].SetPalette(kViridis)  # any ROOT palette, or a custom gradient like the colours above
    pm.AddPlot(plotContours)

    plotCandles = Plot("ptVsMultCandles", "spectra/2dViews", "1d")
    plotCandles[1].AddData("ptVsMult", "data").RebinX(10).SetOptions(candle2)  # one candle per x bin
    plotCandles[1]["Y"].SetRange(0.0, 3.0)
    pm.AddPlot(plotCandles)


def DefineDetectorPlots(pm: PlotManager):
    """Detector performance: event selection, tracking, particle identification, calorimetry."""
    # ---- events ---------------------------------------------------------------------------------
    # data vs. simulation; Both() normalises numerator and denominator before dividing
    plot = Plot("vertexZ", "detector/events", "1d_ratio")
    plot[1].AddData("hVtxZ", "EventQA", "data").Normalize()
    plot[1].AddData("hVtxZ", "EventQA_MC", "simulation").Normalize().SetOptions(hist).SetLine(kRed + 1, kSolid, 3.0)
    plot[1].AddLegend(0.189, 0.898)
    plot[1]["Y"].SetTitle("normalised counts")
    plot[2].AddRatio("hVtxZ", "EventQA", "hVtxZ", "EventQA_MC").Both().Normalize()
    plot[2]["Y"].SetTitle("data / sim.").SetRange(-1.0, 5.0)
    pm.AddPlot(plot)

    # labelled bins, with the bin contents printed on top
    plot = Plot("eventSelection", "detector/events", "1d")
    plot[1].AddData("hEventSelection", "EventQA").SetOptions("HIST TEXT0").SetTextFormat(".0f").SetFill(kAzure - 9, 1001)
    plot[1]["Y"].SetRange(0.0, 1.2e6)
    pm.AddPlot(plot)

    # ---- tracking -------------------------------------------------------------------------------
    # acceptance map: the hole is a TPC sector that was switched off
    plot = Plot("etaPhi", "detector/tracking", "2d")
    plot[1].AddData("hEtaPhi", "TrackQA")
    plot[1].SetPalette(kViridis)
    pm.AddPlot(plot)

    # efficiency = reconstructed / generated; SetIsCorrelated() gives binomial errors
    plot = Plot("trackingEfficiency", "detector/tracking", "1d")
    plot[1].AddRatio("hPt", "TrackQA_MC", "hPtGen", "TrackQA_MC").SetIsCorrelated()
    plot[1]["X"].SetLog().SetRange(0.1, 10.0)
    plot[1]["Y"].SetTitle("tracking efficiency").SetRange(0.0, 1.0).SetGrid()
    pm.AddPlot(plot)

    plot = Plot("ptResponse", "detector/tracking", "2d_logz")
    plot[1].AddData("hPtRecVsGen", "TrackQA_MC")
    pm.AddPlot(plot)

    # histograms from a tree: variables, binning and selections are chosen at plot time
    plot = Plot("dcaXY", "detector/tracking", "1d")
    plot[1].AddData("tracks", "tracks", "primaries").Project1D(("dcaXY", 200, [-1.0, 1.0])).Filter("isPrimary").SetOptions(hist).SetLine(kBlue + 1, kSolid, 2.0)
    plot[1].AddData("tracks", "tracks", "secondaries").Project1D(("dcaXY", 200, [-1.0, 1.0])).Filter("!isPrimary").SetOptions(hist).SetLine(kRed + 1, kSolid, 2.0)
    plot[1].AddData("tracks", "tracks", "all tracks").Project1D(("dcaXY", 200, [-1.0, 1.0])).SetMarker(kBlack, kFullCircle, 0.6)
    plot[1].AddLegend(0.62, 0.9)
    plot[1]["X"].SetTitle("DCA_{#it{xy}} (cm)")
    plot[1]["Y"].SetLog().SetTitle("tracks")
    pm.AddPlot(plot)

    # Define() adds a derived column that can then be used for projections and filters
    plot = Plot("trackSelection", "detector/tracking", "1d")
    ptBins = [0.1, 0.2, 0.3, 0.5, 0.7, 1.0, 1.5, 2.0, 3.0, 5.0, 10.0]
    plot[1].AddData("tracks", "tracks", "all tracks").Project1D(("pt", ptBins))
    plot[1].AddData("tracks", "tracks", "#it{N}_{cls} > 120, |DCA_{#it{xy}}| < 3#sigma").Define("dcaSigma", "0.002 + 0.003 / pt").Project1D(("pt", ptBins)).Filter("nClustersTPC > 120").Filter("abs(dcaXY) < 3 * dcaSigma")
    plot[1].AddLegend()
    plot[1]["X"].SetLog().SetTitle("#it{p}_{T} (GeV/#it{c})")
    plot[1]["Y"].SetLog().SetTitle("tracks")
    pm.AddPlot(plot)

    # Profile1D(x, y): mean of y in bins of x
    plot = Plot("clustersVsEta", "detector/tracking", "1d")
    plot[1].AddData("tracks", "tracks").Profile1D(("eta", 18, [-0.9, 0.9]), "nClustersTPC")
    plot[1]["X"].SetTitle("#eta")
    plot[1]["Y"].SetTitle("#LT#it{N}_{cls}^{TPC}#GT").SetRange(100.0, 160.0)
    pm.AddPlot(plot)

    # ---- particle identification ----------------------------------------------------------------
    plot = Plot("tpcdEdx", "detector/pid", "2d_logz")
    plot[1].AddData("hTPCdEdxVsP", "PIDQA")
    plot[1]["X"].SetLog()
    pm.AddPlot(plot)

    plot = Plot("tofBeta", "detector/pid", "2d_logz")
    plot[1].AddData("hTOFBetaVsP", "PIDQA")
    plot[1]["X"].SetLog()
    pm.AddPlot(plot)

    # ---- calorimetry ----------------------------------------------------------------------------
    # measured points from a table and a user-defined function
    plot = Plot("energyResolution", "detector/calorimeter", "1d")
    plot[1].AddData("testbeam", "testbeam", "test beam").Scatter("energy", "resolution", "0.", "resolution_err")
    plot[1].AddFunction("sqrt(pow(0.10/sqrt(x), 2) + pow(0.01, 2) + pow(0.05/x, 2))", "design: 10%/#sqrt{#it{E}} #oplus 1% #oplus 5%/#it{E}").SetLine(kRed + 1, kSolid, 2.0)
    # a few numbers typed in directly, e.g. from a logbook
    plot[1].AddPoints([1.0, 5.0, 20.0], [0.135, 0.062, 0.035], "prototype 2022").SetMarker(kAzure + 2, kOpenSquare, 1.6)
    plot[1].AddLegend()
    plot[1]["X"].SetLog().SetTitle("#it{E} (GeV)")
    plot[1]["Y"].SetTitle("#sigma_{#it{E}} / #it{E}").SetRange(0.0, 0.2)
    pm.AddPlot(plot)

    plot = Plot("diphotonMass", "detector/calorimeter", "1d")
    # ScaleX(1000.) converts the x axis from GeV to MeV; Smooth() reduces fluctuations of the simulation
    plot[1].AddData("hInvMassGG", "CaloQA", "data").ScaleX(1000.0)
    plot[1].AddData("hInvMassGG", "CaloQA_MC", "simulation (smoothed)").ScaleX(1000.0).Smooth(2).SetOptions(hist).SetLine(kRed + 1, kSolid, 2.0)
    plot[1].AddLegend(0.494, 0.902)
    plot[1].AddText(0.259, 0.916, "#pi^{0}")  # text at a fixed position (pad coordinates)
    plot[1].AddText(0.72, 0.32, "#eta")
    plot[1]["X"].SetRange(20.0, 750.0).SetTitle("#it{m}_{#gamma#gamma} (MeV/#it{c}^{2})")
    plot[1]["Y"].SetRange(0.0, 15e3)
    pm.AddPlot(plot)

    # ---- trigger and pile-up --------------------------------------------------------------------
    # turn-on curve; Result() switches to settings acting on the ratio itself (here: in %)
    plot = Plot("turnOn", "detector/trigger", "1d")
    plot[1].AddRatio("hLeadingPtTriggered", "TriggerQA", "hLeadingPtAll", "TriggerQA", "data").SetIsCorrelated().Result().Scale(100.0)
    # SetRangeX limits the range in which a data item is drawn
    plot[1].AddFunction("98*0.5*(1+TMath::Erf((x-8)/(sqrt(2)*1.5)))", "erf, #it{p}_{T}^{thr} = 8 GeV/#it{c}").SetRangeX(0.0, 20.0).SetLine(kRed + 1, kSolid, 2.0)
    plot[1].AddLegend(0.553, 0.36)
    plot[1]["Y"].SetTitle("trigger efficiency (%)").SetRange(0.0, 110.0)
    pm.AddPlot(plot)

    # shapes compared at their maxima; the overflow bin shows entries beyond the axis range
    plot = Plot("pileUp", "detector/events", "1d")
    plot[1].AddData("hNPV", "EventQA", "data").NormalizeToMaximum().SetShowOverflowBins().SetScaleMaximum(1.3)
    plot[1].AddData("hNPV", "EventQA_MC", "simulation").NormalizeToMaximum().SetShowOverflowBins().SetOptions(hist).SetLine(kRed + 1, kSolid, 3.0)
    plot[1].AddLegend(0.194, 0.911)
    plot[1]["Y"].SetTitle("normalised to maximum")
    pm.AddPlot(plot)

    # ---- run-by-run quality assurance -----------------------------------------------------------
    # wide custom base plot; AddLine draws a line between two points in axis coordinates
    plot = Plot("meanMultiplicityVsRun", "detector/qa", "wide")
    plot[1].AddData("runQA", "runQA", "run average").Scatter("run", "meanNch", "0.", "meanNch_err")
    plot[1].AddLine((544000.0, 9.6), (544500.0, 9.6), "reference").SetLine(kRed + 1, kDashed, 2.0)
    plot[1].AddLegend(0.718, 0.914)
    plot[1]["X"].SetTitle("run number").SetRange(544000.0, 544500.0).SetNumDivisions(505).SetTickOrientation("+")
    plot[1]["Y"].SetTitle("#LT#it{N}_{ch}#GT").SetRange(8.6, 10.2)
    pm.AddPlot(plot)

    # ---- more from the track tree ---------------------------------------------------------------
    # 2d histogram from a tree; Entries() uses only part of the tree (quick checks)
    plot = Plot("dcaVsPt", "detector/tracking", "2d_logz")
    plot[1].AddData("tracks", "tracks").Project2D(("pt", 50, [0.0, 5.0]), ("dcaXY", 100, [-0.2, 0.2])).Entries(20000)
    plot[1]["X"].SetTitle("#it{p}_{T} (GeV/#it{c})")
    plot[1]["Y"].SetTitle("DCA_{#it{xy}} (cm)").SetTitleOffset(1.1)
    pm.AddPlot(plot)

    # Profile2D(x, y, z): mean of z in bins of x and y
    plot = Plot("meanPtEtaPhi", "detector/tracking", "2d")
    plot[1].AddData("tracks", "tracks").Profile2D(("eta", 18, [-0.9, 0.9]), ("phi", 36, [0.0, 6.2832]), "pt")
    plot[1]["X"].SetTitle("#eta")
    plot[1]["Y"].SetTitle("#varphi (rad)")
    plot[1]["Z"].SetTitle("#LT#it{p}_{T}#GT (GeV/#it{c})").SetRange(0.4, 0.9)
    pm.AddPlot(plot)

    # the second argument of Project1D is a weight: here the summed pT per eta bin
    plot = Plot("ptFlowVsEta", "detector/tracking", "1d")
    plot[1].AddData("tracks", "tracks").Project1D(("eta", 18, [-0.9, 0.9]), "pt")
    plot[1]["X"].SetTitle("#eta")
    plot[1]["Y"].SetTitle("#Sigma #it{p}_{T} (GeV/#it{c})").SetRange(0.0, 2500.0)
    pm.AddPlot(plot)

    # ---- systematic uncertainties ---------------------------------------------------------------
    # several columns of one table; line styles cycle through the pad's default styles
    plot = Plot("systematicsBreakdown", "detector/systematics", "1d")
    plot[1].SetDefaultLineStyles([kDashed, kDotted, kDashDotted, 9])  # 9: long dashes
    for source in ["tracking", "pid", "material", "normalisation"]:
        plot[1].AddData("systematics", "systematics", source).Scatter("pt", source).SetOptions(line)
    plot[1].AddData("systematics", "systematics", "total").Scatter("pt", "total").SetOptions(line).SetLine(kBlack, kSolid, 4.0)
    plot[1].AddLegend(0.25, 0.9).SetNumColumns(2)
    plot[1]["X"].SetLog().SetTitle("#it{p}_{T} (GeV/#it{c})")
    plot[1]["Y"].SetTitle("relative uncertainty (%)").SetRange(0.0, 12.0)
    pm.AddPlot(plot)

    # ---- operation ------------------------------------------------------------------------------
    # time axis: unix time stamps shown as dates
    plot = Plot("integratedLuminosity", "detector/operation", "1d")
    plot[1].AddData("luminosity", "luminosity").Scatter("time", "lumi").SetOptions(points_line)
    plot[1].AddText(0.2, 0.9, EXPERIMENT + " // " + COLLISIONS + " // 2023")
    plot[1]["X"].SetTimeFormat("%b%F1970-01-01 00:00:00").SetNumDivisions(508).SetTitle("")
    plot[1]["Y"].SetTitle("delivered luminosity (fb^{-1})")
    pm.AddPlot(plot)


def DefineCollisionPlots(pm: PlotManager):
    """Collider physics: the dimuon spectrum and jets."""
    # DivideBinWidth() turns counts in logarithmic bins into a density ---------------------------
    plot = Plot("massSpectrum", "collisions/dimuon", "1d")
    plot[1].AddData("mMuMu", "dimuon").DivideBinWidth().SetOptions(hist).SetLine(kAzure + 2, kSolid, 2.0)
    plot[1].AddText(0.249, 0.836, "#omega #phi")
    plot[1].AddText(0.378, 0.91, "J/#psi")
    plot[1].AddText(0.477, 0.82, "#psi'")
    plot[1].AddText(0.613, 0.756, "#Upsilon(1S,2S,3S)")
    plot[1].AddText(0.876, 0.656, "Z")
    plot[1]["X"].SetLog()
    plot[1]["Y"].SetLog().SetTitle("d#it{N}/d#it{m}_{#mu#mu} (GeV^{-1})")
    pm.AddPlot(plot)

    # spectra from a directory data source; a theory curve with a wider range defines the frame -----
    plot = Plot("spectra", "collisions/jets", "1d")
    # ApplyLayout applies only the properties set in the layout; SetLayout would replace all of them
    jetStyle = Data().SetMarkerStyle(kFullSquare).SetMarkerSize(1.2)
    for tag, label in [("R02", "#it{R} = 0.2"), ("R04", "#it{R} = 0.4"), ("R06", "#it{R} = 0.6")]:
        plot[1].AddData("hJetPt_" + tag, "jets", label).ApplyLayout(jetStyle).RebinX(2)
    plot[1].AddData("fNLO_R04", "jets", "NLO, #it{R} = 0.4").SetDefinesFrame().SetLine(kGray + 2, kDashed, 2.0)
    plot[1].AddLegend(00.212, 0.454, "anti-#it{k}_{T} jets, |#eta| < 0.5")  # legend with a title
    plot[1]["X"].SetLog()
    plot[1]["Y"].SetLog()
    pm.AddPlot(plot)

    # a 1d profile drawn on top of the 2d histogram it was made from ------------------------------
    plot = Plot("energyResponse", "collisions/jets", "2d_logz")
    plot[1].AddData("hJESResponse", "jets")
    plot[1].AddData("hJESResponse", "jets", "mean response").ProfileX().SetMarker(kBlack, kFullCircle, 1.0).SetLineColor(kBlack)
    plot[1].AddLegend(0.445, 0.232)
    plot[1]["X"].SetLog()
    pm.AddPlot(plot)


def DefineHeavyIonPlots(pm: PlotManager):
    """Heavy-ion collisions: centrality, nuclear modification factor, flow."""
    centralities = ["0-10", "10-30", "30-50"]
    ncoll = {"0-10": 1500.0, "10-30": 750.0, "30-50": 260.0}

    # vertical lines marking the centrality classes ----------------------------------------------
    plot = Plot("centrality", "heavyion", "1d")
    plot[1].AddData("hV0M", "heavyion")
    for boundary in [2700.0, 6800.0, 14000.0]:
        plot[1].AddLine((boundary, 1.0), (boundary, 3.0e4)).SetLine(kRed + 1, kDashed, 2.0)
    plot[1].AddText(0.64, 0.5, "0-10%")
    plot[1].AddText(0.4, 0.6, "10-30%")
    plot[1].AddText(0.24, 0.7, "30-50%")
    plot[1]["Y"].SetLog()
    pm.AddPlot(plot)

    # the in-memory graph defined with the data sources ------------------------------------------
    plot = Plot("ncoll", "heavyion", "1d")
    plot[1].AddData("ncoll", "glauber").SetMarker(kRed + 1, kFullDiamond, 2.0)
    plot[1]["X"].SetRange(0.0, 60.0).SetTitle("centrality (%)")
    plot[1]["Y"].SetRange(0.0, 1800.0).SetTitle("#LT#it{N}_{coll}#GT")
    pm.AddPlot(plot)

    # R_AA in three panels: Numer().Scale() divides the Pb-Pb yield by <N_coll> -------------------
    plot = Plot("nuclearModification", "heavyion", "1d_3panels")
    for i, cent in enumerate(centralities):
        pad = plot[i + 1]
        label = "Pb-Pb / pp" if i == 0 else None  # one legend entry is enough
        pad.AddRatio("cent_" + cent + "/ptSpec", "heavyion", "ptSpec", "data", label).Numer().Scale(1.0 / ncoll[cent])
        pad.SetRefFunc("1").SetLine(kGray + 1, kDashed, 2.0)
        pad.AddText(0.5, 0.9, cent + "%")
    # the published values in the first panel: systematic uncertainties as boxes, statistical as bars,
    # both from the same table; they get their own (second) legend
    plot[1].AddData("raa_published", "heavyion", "published, syst.").Scatter("pt", "raa", "pt_err", "pt_err", "syst_low", "syst_high").SetOptions(boxes).SetFill(kOrange - 9, 1001).SetLineColor(kOrange + 1).SetLegend(2)
    plot[1].AddData("raa_published", "heavyion", "published, stat.").Scatter("pt", "raa", "0.", "stat").SetMarker(kOrange + 1, kOpenCircle, 1.2).SetLegend(2)
    plot[1].AddLegend(0.2, 0.3)
    plot[1].AddLegend(0.2, 0.78).GetEntry(1).SetDrawStyle("F")
    # pad 3 has no labelled data, so its legend entry 1 belongs to no data: a note on the normalisation
    plot[3].AddLegend(0.1, 0.3).GetEntry(1).SetLabel("norm. unc. 5%").SetDrawStyle("F").SetFillColor(kGray).SetFillStyle(1001)
    plot[0]["X"].SetLog().SetRange(0.15, 10.0)
    plot[0]["Y"].SetRange(0.0, 1.4).SetTitle("#it{R}_{AA}").SetTitleCenter()
    pm.AddPlot(plot)

    # the same, one plot per centrality in the subgroup heavyion/nuclearModification
    for cent in centralities:
        single = Plot("raa_" + cent, "heavyion", "1d")
        single.AppendGroup("nuclearModification")
        single[1].AddRatio("cent_" + cent + "/ptSpec", "heavyion", "ptSpec", "data").Numer().Scale(1.0 / ncoll[cent])
        single[1].SetRefFunc("1").SetLine(kGray + 1, kDashed, 2.0)
        single[1].AddText(0.5, 0.9, "Pb-Pb " + cent + "%")
        single[1]["X"].SetLog().SetRange(0.15, 10.0)
        single[1]["Y"].SetRange(0.0, 1.4).SetTitle("#it{R}_{AA}")
        pm.AddPlot(single)

    # graphs in a loop; a legend in several columns ----------------------------------------------
    plot = Plot("ellipticFlow", "heavyion", "1d")
    for cent in centralities:
        plot[1].AddData("cent_" + cent + "/v2", "heavyion", cent + "%")
    plot[1].AddLegend(0.2, 0.9, "Pb-Pb, #it{v}_{2}{2}").SetNumColumns(3)
    plot[1]["Y"].SetRange(0.0, 0.3)
    pm.AddPlot(plot)

    # projections of a THnSparse with ranges on the other axes -----------------------------------
    plot = Plot("ptInCentralityClasses", "heavyion", "1d")
    # Project([axes to keep], [(axis, from, to), ...], True): the ranges are in axis units;
    # the upper value lies inside the last bin that should be included
    for low, high in [(0.0, 9.9), (40.0, 49.9), (80.0, 99.9)]:
        label = f"{int(low)}-{int(high + 0.1)}%"
        plot[1].AddData("hPtEtaCent", "heavyion", label).Project([0], [(2, low, high)], True).Normalize()
    plot[1].AddLegend()
    plot[1]["Y"].SetLog().SetTitle("normalised counts")
    pm.AddPlot(plot)


def main():
    pm = PlotManager("PROJECT_NAME")
    DefineDataSources(pm)
    DefineBasePlots(pm)
    DefineHiggsPlots(pm)
    DefineSpectraPlots(pm)
    DefineDetectorPlots(pm)
    DefineCollisionPlots(pm)
    DefineHeavyIonPlots(pm)
    # an overview of the default colours with their ROOT indices (`plot overview colors`)
    pm.AddColorOverview("colors", "overview")
    pm.SaveProject()


if __name__ == "__main__":
    main()
