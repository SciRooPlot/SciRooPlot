#include "SciRooPlot/Logger.h"
#include "SciRooPlot/PlotManager.h"

#include <TGraph.h>
#include <TString.h>

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#define SRC_DIR std::filesystem::absolute(__FILE__).parent_path().string() + "/"

using std::string;
using std::vector;
using namespace SciRooPlot;
using Data = Plot::Pad::Data;
using LegendEntry = Plot::Pad::LegendEntry;
using StatsBox = Plot::Pad::StatsBox;

void DefineDataSources(PlotManager& pm);
void DefineBasePlots(PlotManager& pm);
void DefineHiggsPlots(PlotManager& pm);
void DefineSpectraPlots(PlotManager& pm);
void DefineDetectorPlots(PlotManager& pm);
void DefineCollisionPlots(PlotManager& pm);
void DefineHeavyIonPlots(PlotManager& pm);
void DefineMLPlots(PlotManager& pm);
void DefineAnimations(PlotManager& pm);

// The plots below are organised in groups and subgroups (higgs/diphoton, detector/pid, ...).
// Each plot is produced with `plot <group> <name> [mode]`, where group and name are regular expressions:
//   plot higgs/diphoton massSpectrum      one plot
//   plot higgs .+ pdf                     all plots of the group "higgs" and its subgroups as PDF
//   plot detector '.*(Eff|Res).*'         every plot in "detector" whose name contains Eff or Res
int main(int argc, char* argv[])
{
  PlotManager pm("PROJECT_NAME");
  // own messages in the style of SciRooPlot (Logger.h): logger::print (always shown),
  // logger::info, logger::log, logger::debug, logger::warning, logger::error (shown depending on `srp verbosity`)
  logger::info("Defining the example plots in {}", SRC_DIR);
  DefineDataSources(pm);
  DefineBasePlots(pm);
  DefineHiggsPlots(pm);
  DefineSpectraPlots(pm);
  DefineDetectorPlots(pm);
  DefineCollisionPlots(pm);
  DefineHeavyIonPlots(pm);
  DefineMLPlots(pm);
  DefineAnimations(pm);
  // the default colours of the base plots with their names and indices (`plot overview colors`; `srp colorwheel` shows all)
  pm.AddColorOverview("colors", "overview");
  pm.SaveProject();
  return 0;
}

// labels shown on most plots
const char kExperiment[] = "Example Experiment";
const char kCollisions[] = "pp, #sqrt{#it{s}} = 13.6 TeV";

//****************************************************************************************
/**
 * Data sources: a short name for one or more input files, used when adding data to a plot.
 */
//****************************************************************************************
void DefineDataSources(PlotManager& pm)
{
  // pseudo-data shipped with SciRooPlot; replace these with your own files
  const string inputFolder = "${SCIROOPLOT_DATA_DIR}/ExampleData";

  // a whole file: objects are addressed by their path inside the file, e.g. "diphoton/mgg_data"
  pm.AddDataSource("higgs", inputFolder + "/higgs.root");
  // "file.root:path" makes a directory (or TList) inside the file the entry point
  pm.AddDataSource("data", inputFolder + "/spectra.root:data");
  pm.AddDataSource("mc", inputFolder + "/spectra.root:mc");
  // typical analysis output: one TList per task inside a directory
  for (const string& task : {"EventQA", "TriggerQA", "TrackQA", "PIDQA", "CaloQA"}) {
    pm.AddDataSource(task, inputFolder + "/AnalysisResults_data.root:" + task + "/output");
    pm.AddDataSource(task + "_MC", inputFolder + "/AnalysisResults_mc.root:" + task + "/output");
  }
  // trees and tables (.csv, .txt, .dat, ...) are turned into histograms or graphs when plotted
  pm.AddDataSource("tracks", inputFolder + "/tracks.root");
  pm.AddDataSource("published", inputFolder + "/ptSpec_published.csv");
  pm.AddDataSource("testbeam", inputFolder + "/testbeam.csv");
  pm.AddDataSource("luminosity", inputFolder + "/luminosity.csv");
  pm.AddDataSource("runQA", inputFolder + "/runQA.csv");
  pm.AddDataSource("systematics", inputFolder + "/systematics.csv");
  pm.AddDataSource("dimuon", inputFolder + "/dimuon.root");
  pm.AddDataSource("decays", inputFolder + "/decays.root");
  pm.AddDataSource("ml", inputFolder + "/ml.root");
  // a directory: all files in it (and its subdirectories) form one data source
  pm.AddDataSource("jets", inputFolder + "/jets/");
  // several files: they are searched in the given order, the first file containing an object wins
  pm.AddDataSource("heavyion", {inputFolder + "/heavyion.root", inputFolder + "/raa_published.csv"});
  // in-memory objects created in this program are stored with the project
  // mean number of binary nucleon-nucleon collisions per centrality class (illustrative Glauber values)
  TGraph ncoll(3);
  ncoll.SetName("ncoll");
  ncoll.SetPoint(0, 5., 1500.);
  ncoll.SetPoint(1, 20., 750.);
  ncoll.SetPoint(2, 40., 260.);
  pm.AddDataSource("glauber", &ncoll);

  // more options:
  // pm.AddDataSource("merged", {"/path/to/a.root", "/path/to/b.root"});  // several files, first match wins
  // pm.AddDataSource("folder", "/path/to/folder/");                       // all files in a directory
  // pm.AddDataSource("local", SRC_DIR + "../rel/path/to/file.root");      // relative to this file
}

//****************************************************************************************
/**
 * Base plots: layouts that the plots build upon (third argument of the Plot constructor).
 * SciRooPlot provides base plots with consistent layout among them: text sizes, tick lengths, distances, line
 * widths and marker sizes are the same in every panel, so that plots of different layouts look alike.
 * The name describes the layout: <type>[_wide|_tall][_<columns>x<rows>][_ratio][_gap], with type 1d (plain
 * panels) or 2d (panels with colour scale), wide or tall panels, a grid of panels sharing their axes, a ratio
 * panel below each panel, or gaps between independent panels. Pads are numbered row by row.
 * The code of PlotManager::MakeBasePlot shows how they are made with PanelLayout.h, as a start for your own.
 */
//****************************************************************************************
void DefineBasePlots(PlotManager& pm)
{
  for (const string& name : {"1d", "1d_ratio", "1d_wide", "1d_2x1", "1d_2x1_ratio", "1d_2x2", "1d_3x1", "1d_3x2_gap", "2d", "2d_2x1"}) {
    pm.AddBasePlot(PlotManager::MakeBasePlot(name));
  }
  // a base plot can be adapted before adding it, e.g. for all 2d plots with a logarithmic z axis
  Plot logz = PlotManager::MakeBasePlot("2d");
  logz.SetName("2d_logz");
  logz[1]['Z'].SetLog();
  pm.AddBasePlot(logz);
}

//****************************************************************************************
/**
 * Higgs boson: invariant-mass spectra, fits, expected vs. observed, exclusion limits.
 */
//****************************************************************************************
void DefineHiggsPlots(PlotManager& pm)
{
  {  // data with fit functions, and a ratio pad dividing the data by the background fit ----
    Plot plot("massSpectrum", "higgs/diphoton", "1d_ratio");
    plot[1].AddData("diphoton/mgg_data", "higgs", "data");
    plot[1].AddData("diphoton/mgg_sigBkgFit", "higgs", "signal + background fit").SetLine(kRed + 1, kSolid, 3.);
    plot[1].AddData("diphoton/mgg_bkgFit", "higgs", "background fit").SetLine(kBlue + 1, kDashed, 3.);
    plot[1].AddText(0.45, 0.9, {kExperiment, string(kCollisions) + ", 140 fb^{-1}", "H #rightarrow #gamma#gamma"});
    plot[1].AddLegend(0.45, 0.62);                                                       // upper left corner of the box in pad coordinates
    plot[2].AddRatio({"diphoton/mgg_data", "higgs"}, {"diphoton/mgg_bkgFit", "higgs"});  // histogram / function
    plot[2]['Y'].SetTitle("data / bkg.").SetRange(0.9, 1.15);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // normalised shape; <mean> etc. in a label are replaced by properties of the data -----
    Plot plot("signalShape", "higgs/diphoton", "1d");
    plot[1].AddData("diphoton/mgg_signalMC", "higgs", "simulation, mean = <mean[.2f]> GeV").Normalize().SetDrawOption(hist).SetColor(kRed + 1);
    plot[1].AddLegend(0.18, 0.9);
    plot[1]['Y'].SetTitle("normalised counts").SetRange(0., 0.05);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // expectation as a filled area, data as points ----------------------------------------
    Plot plot("massSpectrum", "higgs/fourLepton", "1d");
    plot[1].AddData("fourLepton/m4l_sigPlusBkg", "higgs", "background + H(125)").SetDrawOption(hist).SetLine(kRed + 1, kSolid, 3.);
    plot[1].AddData("fourLepton/m4l_background", "higgs", "background (Z#rightarrow4l, ZZ*, Z+jets)").SetDrawOption(hist).SetFill(kAzure - 9, 1001).SetLineColor(kAzure - 9);
    plot[1].AddData("fourLepton/m4l_data", "higgs", "data").SetMarker(kBlack, kFullCircle, 1.2);
    plot[1].AddText(0.2, 0.9, {kExperiment, "H #rightarrow ZZ* #rightarrow 4l"});
    plot[1].AddLegend(0.2, 0.76);
    plot[1]['Y'].SetRange(0., 40.);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // "Brazil plot": uncertainty bands, a reference line, the first data defines the frame
    Plot plot("brazilPlot", "higgs/limits", "1d");
    // legend entries follow the data by default; here the bands should appear as filled boxes
    plot[1].AddData("limits/band2sigma", "higgs", "expected #pm 2#sigma").SetDrawOption(band).SetFill(kOrange, 1001).SetLineColor(kOrange).SetLegendEntry(LegendEntry().SetSymbolElements("F"));
    plot[1].AddData("limits/band1sigma", "higgs", "expected #pm 1#sigma").SetDrawOption(band).SetFill(kGreen + 1, 1001).SetLineColor(kGreen + 1).SetLegendEntry(LegendEntry().SetSymbolElements("F"));
    plot[1].AddData("limits/expected", "higgs", "expected").SetDrawOption(line).SetLine(kBlack, kDashed, 3.);
    plot[1].AddData("limits/observed", "higgs", "observed").SetDrawOption(points_line).SetMarker(kBlack, kFullCircle, 0.8);
    plot[1].SetRefFunc("1").SetLine(kRed + 1, kSolid, 2.);  // SM expectation
    plot[1].AddLegend(0.2, 0.9);
    plot[1]['Y'].SetLog().SetRange(0.08, 20.);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
}

//****************************************************************************************
/**
 * Charged-particle spectra: data vs. simulation, a published reference, 2d data and projections.
 */
//****************************************************************************************
void DefineSpectraPlots(PlotManager& pm)
{
  // templates: how each kind of data looks, said once; measured("ptSpec") is the data "ptSpec" of the source "data" in this look,
  // and changing a template here changes every plot that uses it
  const Data measured = Data().SetDataSource("data").SetMarker(kBlack, kFullCircle, 1.);
  const Data simulated = Data().SetDataSource("mc").SetDrawOption(hist_open).SetLine(kRed + 1, kSolid, 3.);

  {  // ---------------------------------------------------------------------------------------
    Plot plot("ptSpectrum", "spectra", "1d");
    plot[1].AddData(measured("ptSpec"), "data");
    plot[1].AddData(simulated("ptSpec"), "simulation");
    plot[1].AddLegend();
    plot[1].AddText({kExperiment, kCollisions, "|#eta| < 0.8"});
    plot[1]['X'].SetLog();
    plot[1]['Y'].SetLog();
    pm.AddPlot(plot);

    // a copy of the plot above, extended by a ratio pad
    Plot ratioPlot(plot, "ptSpectrumRatio", "spectra");
    ratioPlot.SetBasePlot("1d_ratio");
    ratioPlot[2].AddRatio({"ptSpec", "data"}, {"ptSpec", "mc"});
    ratioPlot[2]['Y'].SetTitle("data / sim.").SetRange(0.7, 1.3);
    ratioPlot[0]['X'].SetLog().SetRange(0.15, 15.);  // pad 0 holds settings shared by all pads
    pm.AddPlot(ratioPlot);
  }  // -----------------------------------------------------------------------------------
  {  // table columns become a graph; any C++ expression of the columns can be used ---------
    Plot plot("ptSpectrumPublished", "spectra", "1d");
    plot[1].AddData(measured("ptSpec"), "this analysis");
    plot[1].AddData("ptSpec_published", "published", "published (stat. #oplus syst.)").Scatter("pt", "value", "pt_err", "sqrt(stat*stat + syst*syst)").SetMarker(kAzure + 2, kOpenSquare, 1.6);
    plot[1].AddLegend();
    plot[1]['X'].SetLog();
    plot[1]['Y'].SetLog();
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // ---------------------------------------------------------------------------------------
    Plot plot("multiplicity", "spectra", "1d");
    plot[1].AddData(measured("multDist"), "data, #LT#it{N}_{ch}#GT = <mean[.1f]>");
    plot[1].AddData(simulated("multDist"), "simulation, #LT#it{N}_{ch}#GT = <mean[.1f]>");
    plot[1].AddLegend();
    plot[1]['X'].SetRange(0., 60.);
    plot[1]['Y'].SetLog();
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // ---------------------------------------------------------------------------------------
    Plot plot("ptVsMult", "spectra", "2d_logz");
    plot[1].AddData("ptVsMult", "data");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // projections of a 2d histogram in a loop, each divided by the lowest class ----------
    Plot plot("ptInMultClasses", "spectra", "1d_ratio");
    const vector<std::pair<double, double>> classes = {{1, 10}, {11, 20}, {21, 30}, {31, 45}, {46, 60}};
    // a colour gradient from blue to red, spread over as many colours as there are classes
    const vector<std::tuple<float, float, float, float>> gradient = {{0.1, 0.2, 0.8, 0.}, {0.8, 0.1, 0.1, 1.}};
    plot[0].SetDefaultColors(gradient, {}, classes.size());
    for (const auto& [low, high] : classes) {
      const string label = std::to_string(static_cast<int>(low)) + " #leq #it{N}_{ch} #leq " + std::to_string(static_cast<int>(high));
      // ProjectY(from, to, true): project onto y for x in [from, to] (in axis units, not bins)
      plot[1].AddData("ptVsMult", "data", label).ProjectY(low, high, true).Normalize();
      if (low > 1) {
        // Numer() / Denom() / Both(): the following settings act on numerator, denominator or both
        plot[2].AddRatio({"ptVsMult", "data"}, {"ptVsMult", "data"}).Numer().ProjectY(low, high, true).Denom().ProjectY(1, 10, true).Both().Normalize();
      }
    }
    plot[1].AddLegend();
    plot[1]['Y'].SetLog().SetTitle("normalised counts");
    plot[2]['Y'].SetTitle("ratio to lowest").SetRange(0., 6.);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // Cumulative(false) sums from the right: the probability to find at least N_ch particles
    Plot plot("multiplicityCumulative", "spectra", "1d");
    plot[1].AddData(measured("multDist"), "data").Cumulative(false);
    plot[1].AddData(simulated("multDist"), "simulation").Cumulative(false);
    plot[1].AddLegend();
    plot[1]['X'].SetRange(0., 60.);
    plot[1]['Y'].SetLog().SetTitle("#it{P}(#geq #it{N}_{ch})");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // ProjectX(from, to, true): multiplicity of events with a track in a given pT range
    Plot plot("multiplicityForHighPt", "spectra", "1d");
    plot[1].AddData("ptVsMult", "data", "#it{p}_{T} < 1 GeV/#it{c}").ProjectX(0., 1., true).Normalize();
    plot[1].AddData("ptVsMult", "data", "2 < #it{p}_{T} < 5 GeV/#it{c}").ProjectX(2., 5., true).Normalize();
    plot[1].AddLegend(0.45, 0.9);
    plot[1]['Y'].SetTitle("normalised counts");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the same 2d histogram shown in 3d, as contours and as candles --------------------
    Plot plot3d("ptVsMultLego", "spectra/2dViews", "2d");
    plot3d[1].AddData("ptVsMult", "data").RebinXY(4, 2).SetDrawOption(lego);
    plot3d[1].SetView(35., 225.);  // viewing angles theta and phi in degrees
    pm.AddPlot(plot3d);

    Plot plotContours("ptVsMultContours", "spectra/2dViews", "2d_logz");
    plotContours[1].AddData("ptVsMult", "data").SetDrawOption(cont_fillz).SetContours(12);
    plotContours[1].SetPalette(kViridis);  // any ROOT palette, or a custom gradient like the colours above
    pm.AddPlot(plotContours);

    Plot plotCandles("ptVsMultCandles", "spectra/2dViews", "1d");
    plotCandles[1].AddData("ptVsMult", "data").RebinX(10).SetDrawOption(candle);  // one candle per x bin
    plotCandles[1]['Y'].SetRange(0., 3.);
    pm.AddPlot(plotCandles);
  }  // -----------------------------------------------------------------------------------
}

//****************************************************************************************
/**
 * Detector performance: event selection, tracking, particle identification, calorimetry.
 */
//****************************************************************************************
void DefineDetectorPlots(PlotManager& pm)
{
  // --------------------------------------------------------------------------------------
  // events
  // --------------------------------------------------------------------------------------
  {  // data vs. simulation; Both() normalises numerator and denominator before dividing
    Plot plot("vertexZ", "detector/events", "1d_ratio");
    plot[1].AddData("hVtxZ", "EventQA", "data").Normalize();
    plot[1].AddData("hVtxZ", "EventQA_MC", "simulation").Normalize().SetDrawOption(hist).SetLine(kRed + 1, kSolid, 3.);
    plot[1].AddLegend(0.189, 0.898);
    // the first data defines the frame: leave room for the simulation, which peaks higher
    plot[1]['Y'].SetTitle("normalised counts").SetMaxRange(0.036);
    plot[2].AddRatio({"hVtxZ", "EventQA"}, {"hVtxZ", "EventQA_MC"}).Both().Normalize();
    plot[2]['Y'].SetTitle("data / sim.").SetRange(-1., 5.);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // labelled bins, with the bin contents printed on top -------------------------------
    Plot plot("eventSelection", "detector/events", "1d");
    plot[1].AddData("hEventSelection", "EventQA").SetDrawOption("HIST TEXT0").SetTextFormat(".0f").SetFill(kAzure - 9, 1001);
    plot[1]['Y'].SetRange(0., 1.2e6);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  // --------------------------------------------------------------------------------------
  // tracking
  // --------------------------------------------------------------------------------------
  {  // acceptance map: the hole is a TPC sector that was switched off ---------------------
    Plot plot("etaPhi", "detector/tracking", "2d");
    plot[1].AddData("hEtaPhi", "TrackQA");
    plot[1].SetPalette(kViridis);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // two colour maps side by side: data and simulation -----------------------------------
    Plot plot("etaPhiDataVsSim", "detector/tracking", "2d_2x1");
    plot[1].AddData("hEtaPhi", "TrackQA");
    plot[1].AddText(0.15, 0.9, "data");
    plot[2].AddData("hEtaPhi", "TrackQA_MC");
    plot[2].AddText(0.15, 0.9, "simulation");
    plot[0].SetPalette(kViridis);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // efficiency = reconstructed / generated; SetIsCorrelated() gives binomial errors -----
    Plot plot("trackingEfficiency", "detector/tracking", "1d");
    plot[1].AddRatio({"hPt", "TrackQA_MC"}, {"hPtGen", "TrackQA_MC"}).SetIsCorrelated();
    plot[1]['X'].SetLog().SetRange(0.1, 10.);
    plot[1]['Y'].SetTitle("tracking efficiency").SetRange(0., 1.).SetGrid();
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // ---------------------------------------------------------------------------------------
    Plot plot("ptResponse", "detector/tracking", "2d_logz");
    plot[1].AddData("hPtRecVsGen", "TrackQA_MC");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // histograms from a tree: variables, binning and selections are chosen at plot time --
    Plot plot("dcaXY", "detector/tracking", "1d");
    plot[1].AddData("tracks", "tracks", "primaries").Project1D({"dcaXY", 200, {-1., 1.}}).Filter("isPrimary").SetDrawOption(hist).SetLine(kBlue + 1, kSolid, 2.);
    plot[1].AddData("tracks", "tracks", "secondaries").Project1D({"dcaXY", 200, {-1., 1.}}).Filter("!isPrimary").SetDrawOption(hist).SetLine(kRed + 1, kSolid, 2.);
    plot[1].AddData("tracks", "tracks", "all tracks").Project1D({"dcaXY", 200, {-1., 1.}}).SetMarker(kBlack, kFullCircle, 0.6);
    plot[1].AddLegend(0.62, 0.9);
    plot[1]['X'].SetTitle("DCA_{#it{xy}} (cm)");
    plot[1]['Y'].SetLog().SetTitle("tracks");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // Define() adds a derived column that can then be used for projections and filters ---
    Plot plot("trackSelection", "detector/tracking", "1d");
    const vector<double> ptBins = {0.1, 0.2, 0.3, 0.5, 0.7, 1., 1.5, 2., 3., 5., 10.};
    plot[1].AddData("tracks", "tracks", "all tracks").Project1D({"pt", ptBins});
    plot[1].AddData("tracks", "tracks", "#it{N}_{cls} > 120, |DCA_{#it{xy}}| < 3#sigma").Define("dcaSigma", "0.002 + 0.003 / pt").Project1D({"pt", ptBins}).Filter("nClustersTPC > 120").Filter("abs(dcaXY) < 3 * dcaSigma");
    plot[1].AddLegend();
    plot[1]['X'].SetLog().SetTitle("#it{p}_{T} (GeV/#it{c})");
    plot[1]['Y'].SetLog().SetTitle("tracks");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // Profile1D(x, y): mean of y in bins of x --------------------------------------------
    Plot plot("clustersVsEta", "detector/tracking", "1d");
    plot[1].AddData("tracks", "tracks").Profile1D({"eta", 18, {-0.9, 0.9}}, "nClustersTPC");
    plot[1]['X'].SetTitle("#eta");
    plot[1]['Y'].SetTitle("#LT#it{N}_{cls}^{TPC}#GT").SetRange(100., 160.);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------

  // --------------------------------------------------------------------------------------
  // particle identification
  // --------------------------------------------------------------------------------------
  {  // ---------------------------------------------------------------------------------------
    Plot plot("tpcdEdx", "detector/pid", "2d_logz");
    plot[1].AddData("hTPCdEdxVsP", "PIDQA");
    plot[1]['X'].SetLog();
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // ---------------------------------------------------------------------------------------
    Plot plot("tofBeta", "detector/pid", "2d_logz");
    plot[1].AddData("hTOFBetaVsP", "PIDQA");
    plot[1]['X'].SetLog();
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------

  // --------------------------------------------------------------------------------------
  // calorimetry
  // --------------------------------------------------------------------------------------
  {  // measured points from a table and a user-defined function ---------------------------
    Plot plot("energyResolution", "detector/calorimeter", "1d");
    plot[1].AddData("testbeam", "testbeam", "test beam").Scatter("energy", "resolution", "0.", "resolution_err");
    plot[1].AddFunction("sqrt(pow(0.10/sqrt(x), 2) + pow(0.01, 2) + pow(0.05/x, 2))", "design: 10%/#sqrt{#it{E}} #oplus 1% #oplus 5%/#it{E}").SetLine(kRed + 1, kSolid, 2.);
    // a few numbers typed in directly, e.g. from a logbook
    plot[1].AddPoints({1., 5., 20.}, {0.135, 0.062, 0.035}, "prototype 2022").SetMarker(kAzure + 2, kOpenSquare, 1.6);
    plot[1].AddLegend();
    plot[1]['X'].SetLog().SetTitle("#it{E} (GeV)");
    plot[1]['Y'].SetTitle("#sigma_{#it{E}} / #it{E}").SetRange(0., 0.2);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // ---------------------------------------------------------------------------------------
    Plot plot("diphotonMass", "detector/calorimeter", "1d");
    // ScaleX(1000.) converts the x axis from GeV to MeV; Smooth() reduces fluctuations of the simulation
    plot[1].AddData("hInvMassGG", "CaloQA", "data").ScaleX(1000.);
    plot[1].AddData("hInvMassGG", "CaloQA_MC", "simulation (smoothed)").ScaleX(1000.).Smooth(2).SetDrawOption(hist).SetLine(kRed + 1, kSolid, 2.);
    plot[1].AddLegend(0.494, 0.902);
    plot[1].AddText(0.259, 0.916, "#pi^{0}");  // text at a fixed position (pad coordinates)
    plot[1].AddText(0.72, 0.32, "#eta");
    plot[1]['X'].SetRange(20., 750.).SetTitle("#it{m}_{#gamma#gamma} (MeV/#it{c}^{2})");
    plot[1]['Y'].SetRange(0., 15e3);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the pi0 peak in six bins of pair pT, each panel with its own axes (_gap) ----------------
    // ProjectX(from, to, true) takes the mass of the pairs with pT in [from, to] (axis units); the background from mixed
    // events is scaled to the same-event pairs in the normalisation region: Normalize(false, 190., 240.) normalises
    // both to their integral there (shown as a line)
    Plot plot("pi0PeakVsPt", "detector/calorimeter", "1d_3x2_gap");
    const vector<std::pair<double, double>> ptBins = {{1., 1.5}, {1.5, 2.}, {2., 3.}, {3., 4.}, {4., 6.}, {6., 10.}};
    for (size_t i = 0; i < ptBins.size(); ++i) {
      const auto& [low, high] = ptBins[i];
      plot[i + 1].AddData("hInvMassGGVsPtMixed", "CaloQA", "mixed event").ProjectX(low, high, true).ScaleX(1000.).Normalize(false, 190., 240.).SetDrawOption(hist).SetFill(kAzure - 9, 1001).SetLineColor(kAzure - 9);
      plot[i + 1].AddData("hInvMassGGVsPt", "CaloQA", "same event").ProjectX(low, high, true).ScaleX(1000.).Normalize(false, 190., 240.).SetMarker(kBlack, kFullCircle, 1.).SetDefinesFrame();
      plot[i + 1].AddLine({190., 0.05}, {240., 0.05}, "norm. region").SetLine(kRed + 1, kSolid, 5.);  // 20 bins: 1/20 each
      plot[i + 1].AddText(0.6, 0.9, Form("%g < #it{p}_{T} < %g GeV/#it{c}", low, high));
    }
    plot[1].AddLegend(0.6, 0.8);  // upper left corner, in pad coordinates like the texts
    plot[0]['X'].SetRange(60., 240.).SetTitle("#it{m}_{#gamma#gamma} (MeV/#it{c}^{2})");
    plot[0]['Y'].SetTitle("pairs (norm. region = 1)").SetMinRange(0.);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------

  // --------------------------------------------------------------------------------------
  // trigger and pile-up
  // --------------------------------------------------------------------------------------
  {  // turn-on curve; Result() switches to settings acting on the ratio itself (here: in %)
    Plot plot("turnOn", "detector/trigger", "1d");
    plot[1].AddRatio({"hLeadingPtTriggered", "TriggerQA"}, {"hLeadingPtAll", "TriggerQA"}, "data").SetIsCorrelated().Result().Scale(100.);
    // SetRangeX limits the range in which a data item is drawn
    plot[1].AddFunction("98*0.5*(1+TMath::Erf((x-8)/(sqrt(2)*1.5)))", "erf, #it{p}_{T}^{thr} = 8 GeV/#it{c}").SetRangeX(0., 20.).SetLine(kRed + 1, kSolid, 2.);
    plot[1].AddLegend(0.553, 0.36);
    plot[1]['Y'].SetTitle("trigger efficiency (%)").SetRange(0., 110.);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // shapes compared at their maxima; the overflow bin shows entries beyond the axis range
    Plot plot("pileUp", "detector/events", "1d");
    plot[1].AddData("hNPV", "EventQA", "data").NormalizeToMaximum().SetShowOverflowBins().SetScaleMaximum(1.3);
    plot[1].AddData("hNPV", "EventQA_MC", "simulation").NormalizeToMaximum().SetShowOverflowBins().SetDrawOption(hist).SetLine(kRed + 1, kSolid, 3.);
    plot[1].AddLegend(0.194, 0.911);
    plot[1]['Y'].SetTitle("normalised to maximum");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------

  // --------------------------------------------------------------------------------------
  // run-by-run quality assurance
  // --------------------------------------------------------------------------------------
  {  // wide custom base plot; AddLine draws a line between two points in axis coordinates
    Plot plot("meanMultiplicityVsRun", "detector/qa", "1d_wide");
    plot[1].AddData("runQA", "runQA", "run average").Scatter("run", "meanNch", "0.", "meanNch_err");
    plot[1].AddLine({544000., 9.6}, {544500., 9.6}, "reference").SetLine(kRed + 1, kDashed, 2.);
    plot[1].AddLegend(0.718, 0.914);
    plot[1]['X'].SetTitle("run number").SetRange(544000., 544500.).SetNumDivisions(505).SetTickOrientation("+");
    plot[1]['Y'].SetTitle("#LT#it{N}_{ch}#GT").SetRange(8.6, 10.2);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------

  // --------------------------------------------------------------------------------------
  // more from the track tree
  // --------------------------------------------------------------------------------------
  {  // 2d histogram from a tree; Entries() uses only part of the tree (quick checks)
    Plot plot("dcaVsPt", "detector/tracking", "2d_logz");
    plot[1].AddData("tracks", "tracks").Project2D({"pt", 50, {0., 5.}}, {"dcaXY", 100, {-0.2, 0.2}}).Entries(20000);
    plot[1]['X'].SetTitle("#it{p}_{T} (GeV/#it{c})");
    plot[1]['Y'].SetTitle("DCA_{#it{xy}} (cm)").SetTitleOffset(1.1);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // Profile2D(x, y, z): mean of z in bins of x and y -----------------------------------
    Plot plot("meanPtEtaPhi", "detector/tracking", "2d");
    plot[1].AddData("tracks", "tracks").Profile2D({"eta", 18, {-0.9, 0.9}}, {"phi", 36, {0., 6.2832}}, "pt");
    plot[1]['X'].SetTitle("#eta");
    plot[1]['Y'].SetTitle("#varphi (rad)");
    plot[1]['Z'].SetTitle("#LT#it{p}_{T}#GT (GeV/#it{c})").SetRange(0.4, 0.9);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the second argument of Project1D is a weight: here the summed pT per eta bin ---------
    Plot plot("ptFlowVsEta", "detector/tracking", "1d");
    plot[1].AddData("tracks", "tracks").Project1D({"eta", 18, {-0.9, 0.9}}, "pt");
    plot[1]['X'].SetTitle("#eta");
    plot[1]['Y'].SetTitle("#Sigma #it{p}_{T} (GeV/#it{c})").SetRange(0., 2500.);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------

  // --------------------------------------------------------------------------------------
  // systematic uncertainties
  // --------------------------------------------------------------------------------------
  {  // several columns of one table; line styles cycle through the pad's default styles
    Plot plot("systematicsBreakdown", "detector/systematics", "1d");
    plot[1].SetDefaultLineStyles({kDashed, kDotted, kDashDotted, 9});  // 9: long dashes
    for (const string& source : {"tracking", "pid", "material", "normalisation"}) {
      plot[1].AddData("systematics", "systematics", source).Scatter("pt", source).SetDrawOption(line);
    }
    plot[1].AddData("systematics", "systematics", "total").Scatter("pt", "total").SetDrawOption(line).SetLine(kBlack, kSolid, 4.);
    plot[1].AddLegend(0.25, 0.9).SetNumColumns(2);
    plot[1]['X'].SetLog().SetTitle("#it{p}_{T} (GeV/#it{c})");
    plot[1]['Y'].SetTitle("relative uncertainty (%)").SetRange(0., 12.);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------

  // --------------------------------------------------------------------------------------
  // operation
  // --------------------------------------------------------------------------------------
  {  // time axis: unix time stamps shown as dates ------------------------------------------
    Plot plot("integratedLuminosity", "detector/operation", "1d");
    plot[1].AddData("luminosity", "luminosity").Scatter("time", "lumi").SetDrawOption(points_line);
    plot[1].AddText(0.2, 0.9, {kExperiment, kCollisions, "2023"});
    plot[1]['X'].SetTimeFormat("%b%F1970-01-01 00:00:00").SetNumDivisions(508).SetTitle("");
    plot[1]['Y'].SetTitle("delivered luminosity (fb^{-1})");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
}

//****************************************************************************************
/**
 * Collider physics: the dimuon spectrum and jets.
 */
//****************************************************************************************
void DefineCollisionPlots(PlotManager& pm)
{
  {  // DivideBinWidth() turns counts in logarithmic bins into a density ---------------------
    Plot plot("massSpectrum", "collisions/dimuon", "1d");
    plot[1].AddData("mMuMu", "dimuon").DivideBinWidth().SetDrawOption(hist).SetLine(kAzure + 2, kSolid, 2.);
    plot[1].AddText(0.249, 0.836, "#omega #phi");
    plot[1].AddText(0.378, 0.91, "J/#psi");
    plot[1].AddText(0.477, 0.82, "#psi'");
    plot[1].AddText(0.613, 0.756, "#Upsilon(1S,2S,3S)");
    plot[1].AddText(0.876, 0.656, "Z");
    plot[1]['X'].SetLog();
    plot[1]['Y'].SetLog().SetTitle("d#it{N}/d#it{m}_{#mu#mu} (GeV^{-1})");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // spectra from a directory data source; a theory curve with a wider range defines the frame
    Plot plot("spectra", "collisions/jets", "1d");
    // ApplyLayout applies only the properties set in the layout; SetLayout would replace all of them
    const Data jetStyle = Data().SetMarkerStyle(kFullSquare).SetMarkerSize(1.2);
    for (const auto& [tag, label] : vector<std::pair<string, string>>{{"R02", "#it{R} = 0.2"}, {"R04", "#it{R} = 0.4"}, {"R06", "#it{R} = 0.6"}}) {
      plot[1].AddData("hJetPt_" + tag, "jets", label).ApplyLayout(jetStyle).RebinX(2);
    }
    plot[1].AddData("fNLO_R04", "jets", "NLO, #it{R} = 0.4").SetDefinesFrame().SetLine(kGray + 2, kDashed, 2.);
    plot[1].AddLegend(0.212, 0.454, "anti-#it{k}_{T} jets, |#eta| < 0.5");  // legend with a title
    plot[1]['X'].SetLog();
    plot[1]['Y'].SetLog();
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // a 1d profile drawn on top of the 2d histogram it was made from ---------------------
    Plot plot("energyResponse", "collisions/jets", "2d_logz");
    plot[1].AddData("hJESResponse", "jets");
    plot[1].AddData("hJESResponse", "jets", "mean response").ProfileX().SetMarker(kBlack, kFullCircle, 1.).SetLineColor(kBlack);
    plot[1].AddLegend(0.445, 0.232);
    plot[1]['X'].SetLog();
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // Armenteros-Podolanski plot of V0 candidates from a tree: each two-body decay is an arc, texts in axis units
    Plot plot("armenteros", "collisions/decays", "2d_logz");
    plot[1].AddData("v0s", "decays").Project2D({"alpha", 200, {-1., 1.}}, {"qT", 125, {0., 0.25}});
    plot[1].AddText(-0.1, 0.235, "K^{0}_{S}").SetUserCoordinates();
    plot[1].AddText(0.64, 0.05, "#Lambda").SetUserCoordinates();
    plot[1].AddText(-0.78, 0.05, "#bar{#Lambda}").SetUserCoordinates();
    plot[1].AddText(0.9, 0.025, "#gamma").SetUserCoordinates();
    plot[1]['X'].SetTitle("#it{#alpha} = (#it{p}_{L}^{+} #minus #it{p}_{L}^{#minus}) / (#it{p}_{L}^{+} + #it{p}_{L}^{#minus})");
    plot[1]['Y'].SetTitle("#it{q}_{T} (GeV/#it{c})");
    plot[1]['Z'].SetTitle("candidates");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // Dalitz plot of D0 -> K0S pi+ pi-: the K*(892) and rho(770) resonances are bands -----
    Plot plot("dalitz", "collisions/decays", "2d");
    plot[1].AddData("d0ToK0sPiPi", "decays").Project2D({"m2KsPip", 70, {0.3, 3.1}}, {"m2KsPim", 70, {0.3, 3.1}});
    plot[1].AddText(top_right, "D^{0} #rightarrow K^{0}_{S}#pi^{+}#pi^{#minus}");
    plot[1]['X'].SetTitle("#it{m}^{2}(K^{0}_{S}#pi^{+}) (GeV^{2}/#it{c}^{4})");
    plot[1]['Y'].SetTitle("#it{m}^{2}(K^{0}_{S}#pi^{#minus}) (GeV^{2}/#it{c}^{4})");
    plot[1]['Z'].SetTitle("candidates");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
}

//****************************************************************************************
/**
 * Heavy-ion collisions: centrality, nuclear modification factor, flow.
 */
//****************************************************************************************
void DefineHeavyIonPlots(PlotManager& pm)
{
  const vector<string> centralities = {"0-10", "10-30", "30-50"};
  const std::map<string, double> ncoll = {{"0-10", 1500.}, {"10-30", 750.}, {"30-50", 260.}};

  {  // vertical lines marking the centrality classes -------------------------------------
    Plot plot("centrality", "heavyion", "1d");
    plot[1].AddData("hV0M", "heavyion");
    for (const double boundary : {2700., 6800., 14000.}) {
      plot[1].AddLine({boundary, 1.}, {boundary, 3.e4}).SetLine(kRed + 1, kDashed, 2.);
    }
    plot[1].AddText(0.64, 0.5, "0-10%");
    plot[1].AddText(0.4, 0.6, "10-30%");
    plot[1].AddText(0.24, 0.7, "30-50%");
    plot[1]['Y'].SetLog();
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the in-memory graph defined with the data sources ---------------------------------
    Plot plot("ncoll", "heavyion", "1d");
    plot[1].AddData("ncoll", "glauber").SetMarker(kRed + 1, kFullDiamond, 2.);
    plot[1]['X'].SetRange(0., 60.).SetTitle("centrality (%)");
    plot[1]['Y'].SetRange(0., 1800.).SetTitle("#LT#it{N}_{coll}#GT");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // R_AA in three panels: Numer().Scale() divides the Pb-Pb yield by <N_coll> ----------
    Plot plot("nuclearModification", "heavyion", "1d_3x1");
    for (size_t i = 0; i < centralities.size(); ++i) {
      const auto& cent = centralities[i];
      auto& pad = plot[i + 1];
      const std::optional<string> label = (i == 0) ? std::optional<string>("Pb-Pb / pp") : std::nullopt;  // one legend entry is enough
      pad.AddRatio({"cent_" + cent + "/ptSpec", "heavyion"}, {"ptSpec", "data"}, label).Numer().Scale(1. / ncoll.at(cent));
      pad.SetRefFunc("1").SetLine(kGray + 1, kDashed, 2.);
      pad.AddText(0.5, 0.9, cent + "%");
    }
    // the published values in the first panel: systematic uncertainties as boxes, statistical as bars,
    // both from the same table; they get their own (second) legend
    plot[1].AddData("raa_published", "heavyion", "published, syst.").Scatter("pt", "raa", "pt_err", "pt_err", "syst_low", "syst_high").SetDrawOption(boxes).SetFill(kOrange - 9, 1001).SetLineColor(kOrange + 1).SetLegend(2).SetLegendEntry(LegendEntry().SetSymbolElements("F"));
    plot[1].AddData("raa_published", "heavyion", "published, stat.").Scatter("pt", "raa", "0.", "stat").SetMarker(kOrange + 1, kOpenCircle, 1.2).SetLegend(2);
    plot[1].AddLegend(0.2, 0.3);
    plot[1].AddLegend(0.2, 0.78);
    // a legend row that belongs to no data: a note on the normalisation
    plot[3].AddLegend(0.1, 0.3).AddEntry("norm. unc. 5%").SetSymbolElements("F").SetFill(kGray, 1001);
    plot[0]['X'].SetLog().SetRange(0.15, 10.);
    plot[0]['Y'].SetRange(0., 1.4).SetTitle("#it{R}_{AA}").SetTitleCenter();
    pm.AddPlot(plot);

    // the same, one plot per centrality in the subgroup heavyion/nuclearModification
    for (const auto& cent : centralities) {
      Plot single("raa_" + cent, "heavyion", "1d");
      single.AppendGroup("nuclearModification");
      single[1].AddRatio({"cent_" + cent + "/ptSpec", "heavyion"}, {"ptSpec", "data"}).Numer().Scale(1. / ncoll.at(cent));
      single[1].SetRefFunc("1").SetLine(kGray + 1, kDashed, 2.);
      single[1].AddText(0.5, 0.9, "Pb-Pb " + cent + "%");
      single[1]['X'].SetLog().SetRange(0.15, 10.);
      single[1]['Y'].SetRange(0., 1.4).SetTitle("#it{R}_{AA}");
      pm.AddPlot(single);
    }
  }  // -----------------------------------------------------------------------------------
  {  // graphs in a loop; a legend in several columns --------------------------------------
    Plot plot("ellipticFlow", "heavyion", "1d");
    for (const auto& cent : centralities) {
      plot[1].AddData("cent_" + cent + "/v2", "heavyion", cent + "%");
    }
    plot[1].AddLegend(0.2, 0.9, "Pb-Pb, #it{v}_{2}{2}").SetNumColumns(3);
    plot[1]['Y'].SetRange(0., 0.3);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // projections of a THnSparse with ranges on the other axes ---------------------------
    Plot plot("ptInCentralityClasses", "heavyion", "1d");
    // Project({axes to keep}, {{axis, from, to}, ...}, true): the ranges are in axis units;
    // a bin is included if the range overlaps it, so from 0 to 10 takes exactly the bins between these edges
    for (const auto& [low, high] : vector<std::pair<double, double>>{{0., 10.}, {40., 50.}, {80., 100.}}) {
      const string label = std::to_string(static_cast<int>(low)) + "-" + std::to_string(static_cast<int>(high)) + "%";
      plot[1].AddData("hPtEtaCent", "heavyion", label).Project({0}, {{2, low, high}}, true).Normalize();
    }
    plot[1].AddLegend();
    plot[1]['Y'].SetLog().SetTitle("normalised counts");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // two centrality classes side by side, each with the R_AA below: the panels of a row share the y axis,
     // so their ranges are set on plot[0]; pads are numbered row by row (1, 2 above, 3, 4 below) -----
    Plot plot("spectraAndRaa", "heavyion", "1d_2x1_ratio");
    for (size_t i = 0; i < 2; ++i) {
      const string cent = centralities[2 * i];  // 0-10 and 30-50
      plot[i + 1].AddData("cent_" + cent + "/ptSpec", "heavyion", "Pb-Pb " + cent + "%");
      plot[i + 1].AddData("ptSpec", "data", "pp #times #LT#it{N}_{coll}#GT").Scale(ncoll.at(cent)).SetDrawOption(hist).SetLine(kGray + 2, kDashed, 3.);
      plot[i + 1].AddLegend();
      plot[i + 3].AddRatio({"cent_" + cent + "/ptSpec", "heavyion"}, {"ptSpec", "data"}).Numer().Scale(1. / ncoll.at(cent));
    }
    plot[0]['X'].SetLog().SetRange(0.15, 10.);
    plot[1]['Y'].SetLog().SetRange(1e-3, 1e6).SetTitle("d#it{N}/d#it{p}_{T} (GeV/#it{c})^{-1}");
    plot[2]['Y'].SetLog().SetRange(1e-3, 1e6);
    plot[3]['Y'].SetRange(0., 1.4).SetTitle("#it{R}_{AA}");
    plot[4]['Y'].SetRange(0., 1.4);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // four centrality classes in a grid with shared axes -----------------------------------
    Plot plot("ptInCentralityGrid", "heavyion", "1d_2x2");
    const vector<std::pair<double, double>> classes = {{0., 10.}, {10., 30.}, {30., 50.}, {50., 80.}};
    for (size_t i = 0; i < classes.size(); ++i) {
      const auto& [low, high] = classes[i];
      const string label = std::to_string(static_cast<int>(low)) + "-" + std::to_string(static_cast<int>(high)) + "%";
      plot[i + 1].AddData("hPtEtaCent", "heavyion").Project({0}, {{2, low, high}}, true).Normalize();
      plot[i + 1].AddText(0.6, 0.85, label);
    }
    plot[0]['Y'].SetLog().SetRange(1e-6, 1.).SetTitle("normalised counts");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // two panels side by side, sharing the y axis ------------------------------------------
    Plot plot("flowCentralVsPeripheral", "heavyion", "1d_2x1");
    for (size_t i = 0; i < 2; ++i) {
      const string cent = centralities[2 * i];  // 0-10 and 30-50
      plot[i + 1].AddData("cent_" + cent + "/v2", "heavyion");
      plot[i + 1].AddText(0.2, 0.85, "Pb-Pb " + cent + "%");
    }
    plot[1].AddText(0.2, 0.75, {kExperiment, "#it{v}_{2}{2}"});
    plot[0]['Y'].SetRange(0., 0.3).SetTitle("#it{v}_{2}");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
}

//**************************************************************************************************
// Machine learning: classifier scores, cut efficiencies, ROC curves, feature importance
//**************************************************************************************************
void DefineMLPlots(PlotManager& pm)
{
  {  // score distributions, training sample filled, test sample as points: does the classifier overtrain?
    Plot plot("scoreDistribution", "ml", "1d");
    plot[1].AddData("hBDT_sig_train", "ml", "signal, train").Normalize().SetDrawOption(hist).SetFill(kAzure - 9, 1001).SetLineColor(kAzure + 2);
    plot[1].AddData("hBDT_bkg_train", "ml", "background, train").Normalize().SetDrawOption(hist).SetFill(kRed + 1, 3004).SetLineColor(kRed + 1);
    plot[1].AddData("hBDT_sig_test", "ml", "signal, test").Normalize().SetColor(kAzure + 2).SetMarkerStyle(kFullCircle);
    plot[1].AddData("hBDT_bkg_test", "ml", "background, test").Normalize().SetColor(kRed + 1).SetMarkerStyle(kFullSquare);
    plot[1].AddLegend(top_left).SetNumColumns(2);
    plot[1]['Y'].SetLog().SetRange(1e-4, 1.).SetTitle("normalised counts");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // efficiency of a cut on the score: Normalize().Cumulative(false) sums from the right, i.e. the fraction above the cut
    Plot plot("efficiencyVsCut", "ml", "1d");
    plot[1].AddData("hBDT_sig_test", "ml", "signal").Normalize().Cumulative(false).SetDrawOption(line).SetLine(kAzure + 2, kSolid, 3.);
    plot[1].AddData("hBDT_bkg_test", "ml", "background").Normalize().Cumulative(false).SetDrawOption(line).SetLine(kRed + 1, kSolid, 3.);
    plot[1].AddLegend(bottom_left);
    plot[1]['X'].SetTitle("cut on the BDT score");
    plot[1]['Y'].SetLog().SetRange(1e-3, 2.).SetTitle("efficiency (score > cut)");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // ROC curves of two classifiers; <title> puts the title of each graph (with its AUC) into the label
    Plot plot("rocCurve", "ml", "1d");
    plot[1].AddData("roc_BDT", "ml", "<title>").SetDrawOption(line).SetLine(kAzure + 2, kSolid, 3.);
    plot[1].AddData("roc_DNN", "ml", "<title>").SetDrawOption(line).SetLine(kRed + 1, kSolid, 3.);
    plot[1].AddFunction("1 - x", "random guess").SetLine(kGray + 1, kDashed, 2.);
    plot[1].AddLegend(bottom_left);
    plot[1]['X'].SetRange(0., 1.);
    plot[1]['Y'].SetRange(0., 1.05);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // feature importance as horizontal bars, the labels of the bins on the y axis
    Plot plot("featureImportance", "ml", "1d");
    plot[1].AddData("hFeatureImportance", "ml").SetDrawOption(hbars).SetFill(kAzure - 9, 1001).SetLineColor(kAzure + 2);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
}

//**************************************************************************************************
// Animations: one plot per frame, in a group of its own; mode gif writes all plots it is given as one animated gif
//**************************************************************************************************
void DefineAnimations(PlotManager& pm)
{
  {  // a camera flight around a lego plot: the same plot seen from 36 angles ---------------
    // plot animations/legoRotation '.+' gif+8 makes them one gif, 8 centiseconds per frame
    // the axes change sides while the view turns, so the base plot leaves the same room on both sides
    auto view3d = PlotManager::MakeBasePlot("1d", PanelLayout().SetMargins(60., 150., 190., 190.));
    view3d.SetName("1d_3dview");
    pm.AddBasePlot(view3d);
    for (int phi = 5; phi < 360; phi += 10) {  // never looking exactly along an axis, where its labels would pile up
      Plot plot("phi" + std::to_string(phi), "animations/legoRotation", "1d_3dview");
      plot[1].AddData("ptVsMult", "data").RebinXY(4, 2).SetDrawOption(lego_col);
      plot[1].SetView(35., phi);
      plot[1]['Z'].SetLog();
      pm.AddPlot(plot);
    }
  }  // -----------------------------------------------------------------------------------
  {  // the spectrum class by class, against all classes together: it gets harder with the multiplicity
    // plot animations/multiplicityClasses '.+' gif+60 makes them one gif, 0.6 s per frame
    for (int low = 1; low < 60; low += 4) {
      const int high = low + 3;
      Plot plot("nch" + std::to_string(low), "animations/multiplicityClasses", "1d_ratio");
      const string label = std::to_string(low) + " #leq #it{N}_{ch} #leq " + std::to_string(high);
      plot[1].AddData("ptVsMult", "data", label).ProjectY(low, high, true).Normalize().SetColor(kRed + 1);
      plot[1].AddData("ptVsMult", "data", "all #it{N}_{ch}").ProjectY(1, 60, true).Normalize().SetDrawOption(hist_open).SetLine(kBlack, kSolid, 3.);
      plot[1].AddLegend(0.5, 0.88);
      plot[1].AddText(0.2, 0.3, {kExperiment, kCollisions});
      plot[1]['Y'].SetLog().SetRange(1e-5, 1.).SetTitle("normalised counts");
      plot[2].AddRatio({"ptVsMult", "data"}, {"ptVsMult", "data"}).Numer().ProjectY(low, high, true).Denom().ProjectY(1, 60, true).Both().Normalize().SetColor(kRed + 1);
      plot[2]['Y'].SetRange(0., 4.).SetTitle("class / all");
      plot[0]['X'].SetRange(0., 4.);  // fixed ranges: the frames differ only in the data
      pm.AddPlot(plot);
    }
  }  // -----------------------------------------------------------------------------------
}
