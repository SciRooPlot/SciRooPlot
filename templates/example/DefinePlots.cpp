#include "SciRooPlot/Logger.h"
#include "SciRooPlot/PlotManager.h"

#include <TGraph.h>
#include <TGraphErrors.h>
#include <TMath.h>
#include <TString.h>

#include <cmath>
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
void DefineMedicalPlots(PlotManager& pm);
void DefineAtomicPlots(PlotManager& pm);
void DefineAcceleratorPlots(PlotManager& pm);
void DefineNuclearPlots(PlotManager& pm);
void DefineSearchPlots(PlotManager& pm);
void DefineNeutronStarPlots(PlotManager& pm);
void DefineBiophysicsPlots(PlotManager& pm);
void DefineElectronicsPlots(PlotManager& pm);
void DefineTheoryPlots(PlotManager& pm);
void DefineOtherSciencePlots(PlotManager& pm);
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
  DefineMedicalPlots(pm);
  DefineAtomicPlots(pm);
  DefineAcceleratorPlots(pm);
  DefineNuclearPlots(pm);
  DefineSearchPlots(pm);
  DefineNeutronStarPlots(pm);
  DefineBiophysicsPlots(pm);
  DefineElectronicsPlots(pm);
  DefineTheoryPlots(pm);
  DefineOtherSciencePlots(pm);
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
  pm.AddDataSource("medical", inputFolder + "/medical.root");
  pm.AddDataSource("atomic", inputFolder + "/atomic.root");
  pm.AddDataSource("accelerator", inputFolder + "/accelerator.root");
  pm.AddDataSource("co2", inputFolder + "/co2.csv");
  pm.AddDataSource("stars", inputFolder + "/stars.csv");
  pm.AddDataSource("spikes", inputFolder + "/spikes.csv");
  pm.AddDataSource("nuclear", inputFolder + "/nuclear.root");
  pm.AddDataSource("neutrino", inputFolder + "/neutrino.root");
  pm.AddDataSource("searches", inputFolder + "/searches.root");
  pm.AddDataSource("neutronstars", inputFolder + "/neutronstars.root");
  pm.AddDataSource("biophysics", inputFolder + "/biophysics.root");
  pm.AddDataSource("theory", inputFolder + "/theory.root");
  pm.AddDataSource("astronomy", inputFolder + "/astronomy.root");
  pm.AddDataSource("trigger", inputFolder + "/trigger.root");
  pm.AddDataSource("timeline", inputFolder + "/timeline.root");
  pm.AddDataSource("tpc", inputFolder + "/tpc.root");
  pm.AddDataSource("temperature", inputFolder + "/temperature.csv");
  pm.AddDataSource("warming", inputFolder + "/warming.csv");
  pm.AddDataSource("climate", inputFolder + "/climate.root");
  pm.AddDataSource("membrane", inputFolder + "/membrane.csv");
  pm.AddDataSource("tuning", inputFolder + "/tuning.csv");
  pm.AddDataSource("clustering", inputFolder + "/clustering.root");
  pm.AddDataSource("mdn", inputFolder + "/mdn.root");
  pm.AddDataSource("electronics", inputFolder + "/electronics.root");
  // a directory: all files in it (and its subdirectories) form one data source
  pm.AddDataSource("jets", inputFolder + "/jets/");
  // several files: they are searched in the given order, the first file containing an object wins
  pm.AddDataSource("heavyion", {inputFolder + "/heavyion.root", inputFolder + "/raa_published.csv"});
  // in-memory objects created in this program are stored with the project
  // mean numbers of participating nucleons and of binary nucleon-nucleon collisions in Pb-Pb collisions per centrality class
  // (illustrative Glauber values; x errors: half the width of the class), two graphs in one data source
  const vector<double> centre = {2.5, 7.5, 15.0, 25.0, 35.0, 45.0, 55.0, 65.0, 75.0, 85.0};
  const vector<double> halfWidth = {2.5, 2.5, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0};
  const vector<double> nPart = {383.0, 331.0, 262.0, 188.0, 131.0, 87.0, 54.0, 31.0, 16.0, 7.0};
  const vector<double> nColl = {1762.0, 1380.0, 978.0, 600.0, 346.0, 186.0, 91.0, 40.0, 15.0, 5.0};
  TGraphErrors npart(static_cast<int>(centre.size()), centre.data(), nPart.data(), halfWidth.data(), nullptr);
  TGraphErrors ncoll(static_cast<int>(centre.size()), centre.data(), nColl.data(), halfWidth.data(), nullptr);
  npart.SetName("npart");
  ncoll.SetName("ncoll");
  pm.AddDataSource("glauber", vector<TObject*>{&npart, &ncoll});

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
 * The name describes the layout: <type>[_wide|_tall][_<columns>x<rows>][_ratio][_pull][_gap], with type 1d (plain
 * panels) or 2d (panels with colour scale), wide or tall panels, a grid of panels sharing their axes, a ratio and/or
 * pull panel below each panel, or gaps between independent panels. Pads are numbered row by row.
 * The code of PlotManager::MakeBasePlot shows how they are made with PanelLayout.h, as a start for your own.
 */
//****************************************************************************************
void DefineBasePlots(PlotManager& pm)
{
  for (const string& name : {"1d", "1d_ratio", "1d_wide", "1d_2x1", "1d_2x1_ratio", "1d_2x2", "1d_3x1", "1d_2x1_gap", "1d_3x1_gap", "1d_3x2_gap", "1d_wide_1x2", "2d", "2d_2x1", "2d_2x1_gap", "2d_3x1", "2d_2x2"}) {
    pm.AddBasePlot(PlotManager::MakeBasePlot(name));
  }
  // a base plot can be adapted before adding it, e.g. for all 2d plots with a logarithmic z axis
  Plot logz = PlotManager::MakeBasePlot("2d");
  logz.SetName("2d_logz");
  logz[1]['Z'].SetLog();
  pm.AddBasePlot(logz);
  // and other proportions: a frame twice as wide as high for maps of the whole sphere, with small margins (a map has no axes)
  Plot worldMap = PlotManager::MakeBasePlot("2d_wide", PanelLayout().SetMargins(32.0, 32.0, 32.0, 32.0), BasePlotShape().SetAspectRatio(2.0));
  worldMap.SetName("2d_map");
  pm.AddBasePlot(worldMap);
  // the same proportions with the margins of the shipped base plots, for a map with longitude and latitude as its axes
  Plot lonLatMap = PlotManager::MakeBasePlot("2d_wide", PanelLayout(), BasePlotShape().SetAspectRatio(2.0));
  lonLatMap.SetName("2d_lonlat");
  pm.AddBasePlot(lonLatMap);
  // a map of a region in longitude and latitude, true to scale at 52 degrees north: a degree of longitude is cos(52 deg) = 0.62 of
  // a degree of latitude, so 45 x 38 degrees of Europe need a frame 1.37 times as high as wide
  Plot regionMap = PlotManager::MakeBasePlot("2d_tall", PanelLayout(), BasePlotShape().SetAspectRatio(1.37));
  regionMap.SetName("2d_europe");
  pm.AddBasePlot(regionMap);
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
    plot[1].AddData("diphoton/mgg_sigBkgFit", "higgs", "fit: signal + bkg.").SetLine(kSolid, 3.0, kRed + 1);
    plot[1].AddData("diphoton/mgg_bkgFit", "higgs", "fit: bkg. only").SetLine(kDashed, 3.0, kBlue + 1);
    plot[1].AddText(0.57, 0.9, {kExperiment, kCollisions, "140 fb^{-1}, H #rightarrow #gamma#gamma"});  // clear of the signal window
    plot[1].AddLegend(0.57, 0.62);                                                                      // upper left corner of the box in pad coordinates
    plot[2].AddRatio({"diphoton/mgg_data", "higgs"}, {"diphoton/mgg_bkgFit"});                          // histogram / function
    plot[2]['Y'].SetTitle("data / bkg.").SetRange(0.9, 1.15);
    for (uint8_t pad : {1, 2}) {  // the signal window as a shape behind the data: not data, so no legend row and not in the axis ranges
      plot[pad].AddRegionX(120.0, 130.0).SetBehindData();
    }
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // normalised shape; <mean> etc. in a label are replaced by properties of the data -----
    Plot plot("signalShape", "higgs/diphoton", "1d");
    plot[1].AddData("diphoton/mgg_signalMC", "higgs", "simulation, mean = <mean[.2f]> GeV").Normalize().SetDrawOption(hist).SetColor(kRed + 1);
    plot[1].AddLegend(0.18, 0.9);
    plot[1]['Y'].SetTitle("normalised counts").SetRange(0.0, 0.05);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // expectation as a filled area, data as points ----------------------------------------
    Plot plot("massSpectrum", "higgs/fourLepton", "1d");
    plot[1].AddData("fourLepton/m4l_sigPlusBkg", "higgs", "background + H(125)").SetDrawOption(hist).SetLine(kSolid, 3.0, kRed + 1);
    plot[1].AddData("fourLepton/m4l_background", "higgs", "background (Z#rightarrow4l, ZZ*, Z+jets)").SetDrawOption(hist).SetFill(1001, kAzure - 9).SetLineColor(kAzure - 9);
    plot[1].AddData("fourLepton/m4l_data", "higgs", "data").SetMarker(kFullCircle, 1.2, kBlack);
    plot[1].AddText(0.2, 0.9, {kExperiment, "H #rightarrow ZZ* #rightarrow 4l"});
    plot[1].AddLegend(0.2, 0.76);
    plot[1]['Y'].SetRange(0.0, 40.0);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // "Brazil plot": uncertainty bands, a reference line, the first data defines the frame
    Plot plot("brazilPlot", "higgs/limits", "1d");
    // legend entries follow the data by default; here the bands should appear as filled boxes
    plot[1].AddData("limits/band2sigma", "higgs", "expected #pm 2#sigma").SetDrawOption(band).SetFill(1001, kOrange).SetLineColor(kOrange).SetLegendEntry(LegendEntry().SetSymbolElements("F"));
    plot[1].AddData("limits/band1sigma", "higgs", "expected #pm 1#sigma").SetDrawOption(band).SetFill(1001, kGreen + 1).SetLineColor(kGreen + 1).SetLegendEntry(LegendEntry().SetSymbolElements("F"));
    plot[1].AddData("limits/expected", "higgs", "expected").SetDrawOption(line).SetLine(kDashed, 3.0, kBlack);
    plot[1].AddData("limits/observed", "higgs", "observed").SetDrawOption(points_line).SetMarker(kFullCircle, 0.8, kBlack);
    plot[1].SetRefFunc("1").SetLine(kSolid, 2.0, kRed + 1);  // SM expectation
    plot[1].AddLegend(0.2, 0.9);
    plot[1]['Y'].SetLog().SetRange(0.08, 20.0);
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
  const Data measured = Data().SetDataSource("data").SetMarker(kFullCircle, 1.0, kBlack);
  const Data simulated = Data().SetDataSource("mc").SetDrawOption(hist_open).SetLine(kSolid, 3.0, kRed + 1);

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
    ratioPlot[0]['X'].SetLog().SetRange(0.15, 15.0);  // pad 0 holds settings shared by all pads
    pm.AddPlot(ratioPlot);
  }  // -----------------------------------------------------------------------------------
  {  // table columns become a graph; any C++ expression of the columns can be used ---------
    Plot plot("ptSpectrumPublished", "spectra", "1d");
    plot[1].AddData(measured("ptSpec"), "this analysis");
    plot[1].AddData("ptSpec_published", "published", "published (stat. #oplus syst.)").Scatter("pt", "value", "pt_err", "sqrt(stat*stat + syst*syst)").SetMarker(kOpenSquare, 1.6, kAzure + 2);
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
    plot[1]['X'].SetRange(0.0, 60.0);
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
    const vector<std::tuple<float, float, float, float>> gradient = {{0.1, 0.2, 0.8, 0.0}, {0.8, 0.1, 0.1, 1.0}};
    plot[0].SetDefaultColors(gradient, {}, classes.size());
    for (const auto& [low, high] : classes) {
      const string label = std::to_string(static_cast<int>(low)) + " #leq #it{N}_{ch} #leq " + std::to_string(static_cast<int>(high));
      // ProjectY(from, to, true): project onto y for x in [from, to] (in axis units, not bins)
      plot[1].AddData("ptVsMult", "data", label).ProjectY(low, high, true).Normalize();
      if (low > 1) {
        // Numer() / Denom() / Both(): the following settings act on numerator, denominator or both
        plot[2].AddRatio({"ptVsMult", "data"}, {"ptVsMult"}).Numer().ProjectY(low, high, true).Denom().ProjectY(1, 10, true).Both().Normalize();
      }
    }
    plot[1].AddLegend();
    plot[1]['Y'].SetLog().SetTitle("normalised counts");
    plot[2]['Y'].SetTitle("ratio to lowest").SetRange(0.0, 6.0);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // Cumulative(false) sums from the right: the probability to find at least N_ch particles
    Plot plot("multiplicityCumulative", "spectra", "1d");
    plot[1].AddData(measured("multDist"), "data").Cumulative(false);
    plot[1].AddData(simulated("multDist"), "simulation").Cumulative(false);
    plot[1].AddLegend();
    plot[1]['X'].SetRange(0.0, 60.0);
    plot[1]['Y'].SetLog().SetTitle("#it{P}(#geq #it{N}_{ch})");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // ProjectX(from, to, true): multiplicity of events with a track in a given pT range
    Plot plot("multiplicityForHighPt", "spectra", "1d");
    plot[1].AddData("ptVsMult", "data", "#it{p}_{T} < 1 GeV/#it{c}").ProjectX(0.0, 1.0, true).Normalize();
    plot[1].AddData("ptVsMult", "data", "2 < #it{p}_{T} < 5 GeV/#it{c}").ProjectX(2.0, 5.0, true).Normalize();
    plot[1].AddLegend(0.45, 0.9);
    plot[1]['Y'].SetTitle("normalised counts");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the same 2d histogram shown in 3d, as contours and as candles --------------------
    Plot plot3d("ptVsMultLego", "spectra/2dViews", "1d_3dview");
    plot3d[1].AddData("ptVsMult", "data").RebinXY(4, 2).Scale(1e-3).SetDrawOption(lego).SetLineWidth(1.0);
    plot3d[1].SetView(35.0, 225.0);      // viewing angles theta and phi in degrees
    plot3d[1]['X'].SetTitleOffset(1.8);  // in 3D views the axis titles need more room from the labels
    plot3d[1]['Y'].SetTitleOffset(1.8).SetNumDivisions(505);
    plot3d[1]['Z'].SetTitle("tracks (10^{3})").SetTitleOffset(1.5);
    pm.AddPlot(plot3d);

    Plot plotContours("ptVsMultContours", "spectra/2dViews", "2d_logz");
    plotContours[1].AddData("ptVsMult", "data").SetDrawOption(cont_fillz).SetContours(12);
    plotContours[1].SetPalette(kViridis);  // any ROOT palette, or a custom gradient like the colours above
    pm.AddPlot(plotContours);

    Plot plotCandles("ptVsMultCandles", "spectra/2dViews", "1d");
    plotCandles[1].AddData("ptVsMult", "data").RebinX(10).SetDrawOption(candle);  // one candle per x bin
    plotCandles[1]['Y'].SetRange(0.0, 3.0);
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
    plot[1].AddData("hVtxZ", "EventQA_MC", "simulation").Normalize().SetDrawOption(hist).SetLine(kSolid, 3.0, kRed + 1);
    plot[1].AddLegend(0.189, 0.898);
    // the first data defines the frame: leave room for the simulation, which peaks higher
    plot[1]['Y'].SetTitle("normalised counts").SetMaxRange(0.036);
    plot[2].AddRatio({"hVtxZ", "EventQA"}, {"hVtxZ", "EventQA_MC"}).Both().Normalize();
    plot[2]['Y'].SetTitle("data / sim.").SetRange(-1.0, 5.0);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // labelled bins, with the bin contents printed on top -------------------------------
    Plot plot("eventSelection", "detector/events", "1d_wide");
    plot[1].AddData("hEventSelection", "EventQA").SetDrawOption("HIST TEXT0").SetTextFormat(".0f").SetFill(1001, kAzure - 9);
    plot[1]['Y'].SetRange(0.0, 1.2e6);
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
  {  // efficiency = reconstructed / generated; SetCorrelated() gives binomial errors -----
    Plot plot("trackingEfficiency", "detector/tracking", "1d");
    plot[1].AddRatio({"hPt", "TrackQA_MC"}, {"hPtGen"}).SetCorrelated();
    plot[1]['X'].SetLog().SetRange(0.1, 10.0);
    plot[1]['Y'].SetTitle("tracking efficiency").SetRange(0.0, 1.0).SetGrid();
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // ---------------------------------------------------------------------------------------
    Plot plot("ptResponse", "detector/tracking", "2d_logz");
    plot[1].AddData("hPtRecVsGen", "TrackQA_MC");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // histograms from a tree: variables, binning and selections are chosen at plot time --
    Plot plot("dcaXY", "detector/tracking", "1d");
    plot[1].AddData("tracks", "tracks", "primaries").Project1D({"dcaXY", 200, {-1.0, 1.0}}).Filter("isPrimary").SetDrawOption(hist).SetLine(kSolid, 2.0, kBlue + 1);
    plot[1].AddData("tracks", "tracks", "secondaries").Project1D({"dcaXY", 200, {-1.0, 1.0}}).Filter("!isPrimary").SetDrawOption(hist).SetLine(kSolid, 2.0, kRed + 1);
    plot[1].AddData("tracks", "tracks", "all tracks").Project1D({"dcaXY", 200, {-1.0, 1.0}}).SetMarker(kFullCircle, 0.6, kBlack);
    plot[1].AddLegend(0.62, 0.9);
    plot[1]['X'].SetTitle("DCA_{#it{xy}} (cm)");
    plot[1]['Y'].SetLog().SetTitle("tracks");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // Define() adds a derived column that can then be used for projections and filters ---
    Plot plot("trackSelection", "detector/tracking", "1d");
    const vector<double> ptBins = {0.1, 0.2, 0.3, 0.5, 0.7, 1.0, 1.5, 2.0, 3.0, 5.0, 10.0};
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
    plot[1]['Y'].SetTitle("#LT#it{N}_{cls}^{TPC}#GT").SetRange(100.0, 160.0);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // two different quantities side by side, each with its own palette and colour scale (_gap: the panels share no axis)
    Plot plot("trackingQAMaps", "detector/tracking", "2d_2x1_gap");
    plot[1].AddData("tracks", "tracks").Project2D({"eta", 18, {-0.9, 0.9}}, {"phi", 36, {0.0, 2.0 * TMath::Pi()}});
    plot[2].AddData("tracks", "tracks").Profile2D({"eta", 18, {-0.9, 0.9}}, {"phi", 36, {0.0, 2.0 * TMath::Pi()}}, "nClustersTPC");
    plot[1].SetPalette(kViridis);
    plot[2].SetPalette(kCividis);
    plot[1]['Z'].SetTitle("tracks");
    plot[2]['Z'].SetTitle("#LT#it{N}_{cls}^{TPC}#GT");
    plot[0]['X'].SetTitle("#eta");
    plot[0]['Y'].SetTitle("#varphi (rad)");
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
    plot[1].AddFunction("sqrt(pow(0.10/sqrt(x), 2) + pow(0.01, 2) + pow(0.05/x, 2))", "design: 10%/#sqrt{#it{E}} #oplus 1% #oplus 5%/#it{E}").SetLine(kSolid, 2.0, kRed + 1);
    // a few numbers typed in directly, e.g. from a logbook
    plot[1].AddPoints({1.0, 5.0, 20.0}, {0.135, 0.062, 0.035}, "prototype 2022").SetMarker(kOpenSquare, 1.6, kAzure + 2);
    plot[1].AddLegend();
    plot[1]['X'].SetLog().SetTitle("#it{E} (GeV)");
    plot[1]['Y'].SetTitle("#sigma_{#it{E}} / #it{E}").SetRange(0.0, 0.2);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // ---------------------------------------------------------------------------------------
    Plot plot("diphotonMass", "detector/calorimeter", "1d");
    // ScaleX(1000.0) converts the x axis from GeV to MeV; Smooth() reduces fluctuations of the simulation
    plot[1].AddData("hInvMassGG", "CaloQA", "data").ScaleX(1000.0);
    plot[1].AddData("hInvMassGG", "CaloQA_MC", "simulation (smoothed)").ScaleX(1000.0).Smooth(2).SetDrawOption(hist).SetLine(kSolid, 2.0, kRed + 1);
    plot[1].AddLegend(0.494, 0.902);
    plot[1].AddText(0.28, 0.93, "#pi^{0}");  // text at a fixed position (pad coordinates)
    plot[1].AddText(0.72, 0.32, "#eta");
    plot[1]['X'].SetRange(20.0, 750.0).SetTitle("#it{m}_{#gamma#gamma} (MeV/#it{c}^{2})");
    plot[1]['Y'].SetRange(0.0, 15e3);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the pi0 peak in six bins of pair pT, each panel with its own axes (_gap) ----------------
    // ProjectX(from, to, true) takes the mass of the pairs with pT in [from, to] (axis units); the background from mixed
    // events is scaled to the same-event pairs in the normalisation region: Normalize(false, 190.0, 240.0) normalises
    // both to their integral there (shown as a line)
    Plot plot("pi0PeakVsPt", "detector/calorimeter", "1d_3x2_gap");
    const vector<std::pair<double, double>> ptBins = {{1.0, 1.5}, {1.5, 2.0}, {2.0, 3.0}, {3.0, 4.0}, {4.0, 6.0}, {6.0, 10.0}};
    for (size_t i = 0; i < ptBins.size(); ++i) {
      const auto& [low, high] = ptBins[i];
      plot[i + 1].AddData("hInvMassGGVsPtMixed", "CaloQA", "mixed event").ProjectX(low, high, true).ScaleX(1000.0).Normalize(false, 190.0, 240.0).SetDrawOption(hist).SetFill(1001, kAzure - 9).SetLineColor(kAzure - 9);
      plot[i + 1].AddData("hInvMassGGVsPt", "CaloQA", "same event").ProjectX(low, high, true).ScaleX(1000.0).Normalize(false, 190.0, 240.0).SetMarker(kFullCircle, 1.0, kBlack).SetDefinesFrame();
      plot[i + 1].AddPoints({190.0, 240.0}, {0.05, 0.05}, "norm. region").SetDrawOption(line).SetLine(kSolid, 5.0, kRed + 1);  // 20 bins: 1/20 each
      plot[i + 1].AddText(0.6, 0.9, Form("%g < #it{p}_{T} < %g GeV/#it{c}", low, high));
    }
    plot[1].AddLegend(0.6, 0.8);  // upper left corner, in pad coordinates like the texts
    plot[0]['X'].SetRange(60.0, 240.0).SetTitle("#it{m}_{#gamma#gamma} (MeV/#it{c}^{2})");
    plot[0]['Y'].SetTitle("pairs (norm. region = 1)").SetMinRange(0.0);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------

  // --------------------------------------------------------------------------------------
  // trigger and pile-up
  // --------------------------------------------------------------------------------------
  {  // turn-on curve; Result() switches to settings acting on the ratio itself (here: in %)
    Plot plot("turnOn", "detector/trigger", "1d");
    plot[1].AddRatio({"hLeadingPtTriggered", "TriggerQA"}, {"hLeadingPtAll"}, "data").SetCorrelated().Result().Scale(100.0);
    // SetRangeX limits the range in which a data item is drawn
    plot[1].AddFunction("98*0.5*(1+TMath::Erf((x-8)/(sqrt(2)*1.5)))", "erf, #it{p}_{T}^{thr} = 8 GeV/#it{c}").SetRangeX(0.0, 20.0).SetLine(kSolid, 2.0, kRed + 1);
    plot[1].AddLegend(0.553, 0.36);
    plot[1]['Y'].SetTitle("trigger efficiency (%)").SetRange(0.0, 110.0);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // TEfficiency objects keep passed and total counts; drawn, their errors are binomial intervals (Clopper-Pearson)
    Plot plot("muonEfficiency", "detector/trigger", "1d");
    plot[1].AddData("eta_2023", "trigger", "2023").SetMarkerStyle(kOpenCircle).SetMarkerSize(1.2).SetColor(kRed + 1);
    plot[1].AddData("eta_2024", "trigger", "2024 (repaired)").SetMarkerStyle(kFullCircle).SetColor(kBlack);
    plot[1].AddText(0.18, 0.38, {"muon trigger", "#it{p}_{T} > 25 GeV/#it{c}"});
    plot[1].AddLegend(0.62, 0.38);
    plot[1]['Y'].SetRange(0.0, 1.1);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // a 2d TEfficiency is drawn as a map of the efficiency: holes where the detector stands on its feet
    Plot plot("muonEfficiencyMap", "detector/trigger", "2d");
    plot[1].AddData("etaPhi_2023", "trigger").SetDrawOption(colz);
    plot[1].SetPalette(kCividis);
    plot[1]['Z'].SetRange(0.0, 1.0);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // shapes compared at their maxima; the overflow bin shows entries beyond the axis range
    Plot plot("pileUp", "detector/events", "1d");
    plot[1].AddData("hNPV", "EventQA", "data").NormalizeToMaximum().SetShowOverflowBins();
    plot[1].AddData("hNPV", "EventQA_MC", "simulation").NormalizeToMaximum().SetShowOverflowBins().SetDrawOption(hist).SetLine(kSolid, 3.0, kRed + 1);
    plot[1].AddLegend(0.194, 0.911);
    plot[1]['Y'].SetTitle("normalised to maximum").SetRoomAboveMax(0.25);  // a quarter of the frame free above the data for the legend
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------

  // --------------------------------------------------------------------------------------
  // run-by-run quality assurance
  // --------------------------------------------------------------------------------------
  {  // wide custom base plot; a line with a legend row is data (points drawn as a line), one without would be a shape (AddHorizontalLine)
    Plot plot("meanMultiplicityVsRun", "detector/qa", "1d_wide");
    plot[1].AddData("runQA", "runQA", "run average").Scatter("run", "meanNch", "0.", "meanNch_err");
    plot[1].AddPoints({544000.0, 544400.0}, {9.6, 9.6}, "reference").SetDrawOption(line).SetLine(kDashed, 2.0, kRed + 1);
    plot[1].AddLegend(0.718, 0.914);
    plot[1]['X'].SetTitle("run number").SetRange(544000.0, 544400.0).SetNumDivisions(504).SetTickOrientation("+");
    plot[1]['Y'].SetTitle("#LT#it{N}_{ch}#GT").SetRange(8.7, 10.2);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------

  // --------------------------------------------------------------------------------------
  // more from the track tree
  // --------------------------------------------------------------------------------------
  {  // 2d histogram from a tree; Entries() uses only part of the tree (quick checks)
    Plot plot("dcaVsPt", "detector/tracking", "2d_logz");
    plot[1].AddData("tracks", "tracks").Project2D({"pt", 50, {0.0, 5.0}}, {"dcaXY", 100, {-0.2, 0.2}}).Entries(20000);
    plot[1]['X'].SetTitle("#it{p}_{T} (GeV/#it{c})");
    plot[1]['Y'].SetTitle("DCA_{#it{xy}} (cm)").SetTitleOffset(1.1);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // Profile2D(x, y, z): mean of z in bins of x and y -----------------------------------
    Plot plot("meanPtEtaPhi", "detector/tracking", "2d");
    plot[1].AddData("tracks", "tracks").Profile2D({"eta", 18, {-0.9, 0.9}}, {"phi", 36, {0.0, 6.2832}}, "pt");
    plot[1]['X'].SetTitle("#eta");
    plot[1]['Y'].SetTitle("#varphi (rad)");
    plot[1]['Z'].SetTitle("#LT#it{p}_{T}#GT (GeV/#it{c})").SetRange(0.4, 0.9);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the second argument of Project1D is a weight: here the summed pT per eta bin ---------
    Plot plot("ptFlowVsEta", "detector/tracking", "1d");
    plot[1].AddData("tracks", "tracks").Project1D({"eta", 18, {-0.9, 0.9}}, "pt");
    plot[1]['X'].SetTitle("#eta");
    plot[1]['Y'].SetTitle("#Sigma #it{p}_{T} (GeV/#it{c})").SetRange(0.0, 2500.0);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------

  // --------------------------------------------------------------------------------------
  // systematic uncertainties
  // --------------------------------------------------------------------------------------
  {  // several columns of one table; line styles cycle through the pad's default styles
    // hit density on four barrel layers of a tracker (radii invented), from the tracks crossing them (z = r sinh(eta)), in hits per
    // cm^2: the weight is one over the area of a bin; one colour scale for all layers shows how the density falls with the radius,
    // a scale per layer would make them look alike; one module of the third layer is off
    Plot plot("hitDensity", "detector/tracking", "2d_2x2");  // pads 1, 2, 4, 5; pad 3 is the common scale
    const vector<std::pair<uint8_t, double>> layers = {{1, 30.0}, {2, 45.0}, {4, 60.0}, {5, 80.0}};
    for (const auto& [pad, radius] : layers) {
      auto& data = plot[pad].AddData("tracks", "tracks").Define("z", Form("%g * sinh(eta)", radius)).Define("w", Form("1.0 / (%g * 0.08727 * 2.5)", radius));
      if (radius == 60.0) data.Filter("phi < 2.1 || phi > 2.6");
      data.Project2D({"z", 64, {-80.0, 80.0}}, {"phi", 72, {0.0, 6.2832}}, "w");
      plot[pad].AddText(top_left, Form("#it{r} = %g cm", radius)).SetTextColor(kWhite);
    }
    plot[0].SetPalette(kViridis);
    plot[0].SetFrameFill(1001, kBlack);
    plot[0]['X'].SetTitle("#it{z} (cm)");
    plot[0]['Y'].SetTitle("#varphi (rad)");
    plot[0]['Z'].SetTitle("hits per cm^{2}");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {
    Plot plot("systematicsBreakdown", "detector/systematics", "1d");
    plot[1].SetDefaultLineStyles({kDashed, kDotted, kDashDotted, 9});  // 9: long dashes
    for (const string& source : {"tracking", "pid", "material", "normalisation"}) {
      plot[1].AddData("systematics", "systematics", source).Scatter("pt", source).SetDrawOption(line);
    }
    plot[1].AddData("systematics", "systematics", "total").Scatter("pt", "total").SetDrawOption(line).SetLine(kSolid, 4.0, kBlack);
    plot[1].AddLegend(0.25, 0.9).SetNumColumns(2);
    plot[1]['X'].SetLog().SetTitle("#it{p}_{T} (GeV/#it{c})");
    plot[1]['Y'].SetTitle("relative uncertainty (%)").SetRange(0.0, 12.0);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the same, one source per panel against the total: all four panels share one y range (the rows of a grid only share theirs),
     // so the dominant source stands out instead of every panel being zoomed to its own range
    Plot plot("systematicsPerSource", "detector/systematics", "1d_2x2");
    plot.ShareAxis('Y', {1, 2, 3, 4});
    uint8_t pad = 1;
    for (const string& source : {"tracking", "pid", "material", "normalisation"}) {
      plot[pad].AddData("systematics", "systematics", "this source").Scatter("pt", source).SetDrawOption(line).SetLine(kSolid, 4.0, kBlue + 1);
      plot[pad].AddData("systematics", "systematics", "total").Scatter("pt", "total").SetDrawOption(line).SetLine(kDashed, 2.0, kGray + 2);
      plot[pad].AddText(top_left, source);
      ++pad;
    }
    plot[2].AddLegend(top_right);
    plot[0]['X'].SetLog().SetTitle("#it{p}_{T} (GeV/#it{c})");
    plot[0]['Y'].SetTitle("relative uncertainty (%)").SetRoomAboveMax(0.3);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------

  // --------------------------------------------------------------------------------------
  // operation
  // --------------------------------------------------------------------------------------
  {  // time axis: unix time stamps shown as dates ------------------------------------------
    Plot plot("integratedLuminosity", "detector/operation", "1d");
    plot[1].AddData("luminosity", "luminosity").Scatter("time", "lumi").SetDrawOption(points_line);
    plot[1].AddText(0.2, 0.9, {kExperiment, kCollisions, "2023"});
    plot[1].AddImage(0.21, 0.7, "${SCIROOPLOT_DATA_DIR}/ExampleData/logo.png").SetWidth(0.3);  // an image (e.g. a logo): upper left corner and width
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
    plot[1].AddData("mMuMu", "dimuon").DivideBinWidth().SetDrawOption(hist).SetLine(kSolid, 2.0, kAzure + 2);
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
    plot[1].AddData("fNLO_R04", "jets", "NLO, #it{R} = 0.4").SetDefinesFrame().SetLine(kDashed, 2.0, kGray + 2);
    plot[1].AddLegend(0.212, 0.454, "anti-#it{k}_{T} jets, |#eta| < 0.5");  // legend with a title
    // the theory curve defining the frame has no axis titles
    plot[1]['X'].SetLog().SetRange(10.0, 1000.0).SetTitle("#it{p}_{T}^{jet} (GeV/#it{c})");
    plot[1]['Y'].SetLog().SetTitle("d#it{N}/d#it{p}_{T} (GeV/#it{c})^{-1}");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // a 1d profile drawn on top of the 2d histogram it was made from ---------------------
    Plot plot("energyResponse", "collisions/jets", "2d_logz");
    plot[1].AddData("hJESResponse", "jets");
    plot[1].AddData("hJESResponse", "jets", "mean response").ProfileX().SetMarker(kFullCircle, 1.0, kBlack).SetLineColor(kBlack);
    plot[1].AddLegend(0.445, 0.232);
    plot[1]['X'].SetLog();
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // Armenteros-Podolanski plot of V0 candidates from a tree: each two-body decay is an arc, texts in axis units
    Plot plot("armenteros", "collisions/decays", "2d_logz");
    plot[1].AddData("v0s", "decays").Project2D({"alpha", 200, {-1.0, 1.0}}, {"qT", 125, {0.0, 0.25}});
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
  const std::map<string, double> ncoll = {{"0-10", 1500.0}, {"10-30", 750.0}, {"30-50", 260.0}};

  {  // vertical lines marking the centrality classes (shapes: over the height of the frame, not data)
    Plot plot("centrality", "heavyion", "1d");
    plot[1].AddData("hV0M", "heavyion");
    for (const double boundary : {2700.0, 6800.0, 14000.0}) {
      plot[1].AddVerticalLine(boundary).SetLine(kDashed, 2.0, kRed + 1);
    }
    plot[1].AddText(0.7, 0.5, "0-10%");
    plot[1].AddText(0.43, 0.6, "10-30%");
    plot[1].AddText(0.255, 0.7, "30-50%");
    plot[1]['Y'].SetLog();
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the in-memory graphs defined with the data sources: N_coll grows faster than N_part towards central collisions, so hard
    // processes (scaling with N_coll) gain on soft ones (scaling with N_part); the classes of the R_AA plots below shaded (labels: centrality in %)
    Plot plot("ncoll", "heavyion", "1d");
    for (const auto& [low, high] : vector<std::pair<double, double>>{{0.0, 10.0}, {30.0, 50.0}}) {  // every other class, so that the three can be told apart
      plot[1].AddRegionX(low, high).SetFill(1001, kGray, 0.5).SetBehindData();
    }
    for (const auto& [x, cent] : vector<std::pair<double, string>>{{1.0, "0-10"}, {15.0, "10-30"}, {35.0, "30-50"}}) {
      plot[1].AddText(x, 4.0, cent).SetUserCoordinates();
    }
    plot[1].AddData("ncoll", "glauber", "#LT#it{N}_{coll}#GT").SetDrawOption(points_xerr).SetMarker(kFullCircle, 1.4, kRed + 1).SetLineColor(kRed + 1);
    plot[1].AddData("npart", "glauber", "#LT#it{N}_{part}#GT").SetDrawOption(points_xerr).SetMarker(kFullSquare, 1.3, kAzure + 2).SetLineColor(kAzure + 2);
    plot[1].AddText(0.55, 0.92, {"Pb-Pb, #sqrt{#it{s}_{NN}} = 5.02 TeV", "Glauber model"});
    plot[1].AddLegend(0.72, 0.78);
    plot[1]['X'].SetRange(0.0, 90.0).SetTitle("centrality (%)");
    plot[1]['Y'].SetLog().SetRange(2.0, 8000.0).SetTitle("nucleons or collisions");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // R_AA in three panels: Numer().Scale() divides the Pb-Pb yield by <N_coll> ----------
    Plot plot("nuclearModification", "heavyion", "1d_3x1");
    for (size_t i = 0; i < centralities.size(); ++i) {
      const auto& cent = centralities[i];
      auto& panel = plot[i + 1];
      const std::optional<string> label = (i == 0) ? std::optional<string>("Pb-Pb / pp") : std::nullopt;  // one legend entry is enough
      panel.AddRatio({"cent_" + cent + "/ptSpec", "heavyion"}, {"ptSpec", "data"}, label).Numer().Scale(1.0 / ncoll.at(cent));
      panel.SetRefFunc("1").SetLine(kDashed, 2.0, kGray + 1);
      panel.AddText(0.5, 0.9, cent + "%");
    }
    // the published values in the first panel: systematic uncertainties as boxes, statistical as bars,
    // both from the same table; they get their own (second) legend
    plot[1].AddData("raa_published", "heavyion", "published, syst.").Scatter("pt", "raa", "pt_err", "pt_err", "syst_low", "syst_high").SetDrawOption(boxes).SetFill(1001, kOrange - 9).SetLineColor(kOrange + 1).SetLegend(2).SetLegendEntry(LegendEntry().SetSymbolElements("F"));
    plot[1].AddData("raa_published", "heavyion", "published, stat.").Scatter("pt", "raa", "0.", "stat").SetMarker(kOpenCircle, 1.2, kOrange + 1).SetLegend(2);
    plot[1].AddLegend(0.2, 0.3);
    plot[1].AddLegend(0.2, 0.78);
    // a legend row that belongs to no data: a note on the normalisation
    plot[3].AddLegend(0.1, 0.3).AddEntry("norm. unc. 5%").SetSymbolElements("F").SetFill(1001, kGray);
    plot[0]['X'].SetLog().SetRange(0.15, 10.0);
    plot[0]['Y'].SetRange(0.0, 1.4).SetTitle("#it{R}_{AA}").SetTitleCenter();
    pm.AddPlot(plot);

    // the same, one plot per centrality in the subgroup heavyion/raaPerCentrality
    for (const auto& cent : centralities) {
      Plot single("raa_" + cent, "heavyion", "1d");
      single.AppendGroup("raaPerCentrality");
      single[1].AddRatio({"cent_" + cent + "/ptSpec", "heavyion"}, {"ptSpec", "data"}).Numer().Scale(1.0 / ncoll.at(cent));
      single[1].SetRefFunc("1").SetLine(kDashed, 2.0, kGray + 1);
      single[1].AddText(0.5, 0.9, "Pb-Pb " + cent + "%");
      single[1]['X'].SetLog().SetRange(0.15, 10.0);
      single[1]['Y'].SetRange(0.0, 1.4).SetTitle("#it{R}_{AA}");
      pm.AddPlot(single);
    }
  }  // -----------------------------------------------------------------------------------
  {  // graphs in a loop; a legend in several columns --------------------------------------
    Plot plot("ellipticFlow", "heavyion", "1d");
    for (const auto& cent : centralities) {
      plot[1].AddData("cent_" + cent + "/v2", "heavyion", cent + "%");
    }
    plot[1].AddLegend(0.2, 0.9, "Pb-Pb, #it{v}_{2}{2}").SetNumColumns(3);
    plot[1]['Y'].SetRange(0.0, 0.3);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // projections of a THnSparse with ranges on the other axes ---------------------------
    Plot plot("ptInCentralityClasses", "heavyion", "1d");
    // Project({axes to keep}, {{axis, from, to}, ...}, true): the ranges are in axis units;
    // a bin is included if the range overlaps it, so from 0 to 10 takes exactly the bins between these edges
    for (const auto& [low, high] : vector<std::pair<double, double>>{{0.0, 10.0}, {40.0, 50.0}, {80.0, 100.0}}) {
      const string label = std::to_string(static_cast<int>(low)) + "-" + std::to_string(static_cast<int>(high)) + "%";
      plot[1].AddData("hPtEtaCent", "heavyion", label).Project({0}, {{2, low, high}}, true).Normalize();
    }
    plot[1].AddLegend();
    plot[1]['X'].SetRange(0.0, 5.5);
    plot[1]['Y'].SetLog().SetTitle("normalised counts");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // two centrality classes side by side, each with the R_AA below: the panels of a row share the y axis,
     // so their ranges are set on plot[0]; pads are numbered row by row (1, 2 above, 3, 4 below) -----
    Plot plot("spectraAndRaa", "heavyion", "1d_2x1_ratio");
    for (size_t i = 0; i < 2; ++i) {
      const string cent = centralities[2 * i];  // 0-10 and 30-50
      plot[i + 1].AddData("cent_" + cent + "/ptSpec", "heavyion", "Pb-Pb " + cent + "%");
      plot[i + 1].AddData("ptSpec", "data", "pp #times #LT#it{N}_{coll}#GT").Scale(ncoll.at(cent)).SetDrawOption(hist).SetLine(kDashed, 3.0, kGray + 2);
      plot[i + 1].AddLegend();
      plot[i + 3].AddRatio({"cent_" + cent + "/ptSpec", "heavyion"}, {"ptSpec", "data"}).Numer().Scale(1.0 / ncoll.at(cent));
    }
    plot[0]['X'].SetLog().SetRange(0.15, 10.0);
    plot[1]['Y'].SetLog().SetRange(1e-3, 1e6).SetTitle("d#it{N}/d#it{p}_{T} (GeV/#it{c})^{-1}");
    plot[2]['Y'].SetLog().SetRange(1e-3, 1e6);
    plot[3]['Y'].SetRange(0.0, 1.4).SetTitle("#it{R}_{AA}");
    plot[4]['Y'].SetRange(0.0, 1.4);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // four centrality classes in a grid with shared axes -----------------------------------
    Plot plot("ptInCentralityGrid", "heavyion", "1d_2x2");
    const vector<std::pair<double, double>> classes = {{0.0, 10.0}, {10.0, 30.0}, {30.0, 50.0}, {50.0, 80.0}};
    for (size_t i = 0; i < classes.size(); ++i) {
      const auto& [low, high] = classes[i];
      const string label = std::to_string(static_cast<int>(low)) + "-" + std::to_string(static_cast<int>(high)) + "%";
      plot[i + 1].AddData("hPtEtaCent", "heavyion").Project({0}, {{2, low, high}}, true).Normalize();
      plot[i + 1].AddText(0.6, 0.85, label);
    }
    plot[0]['Y'].SetLog().SetRange(1e-6, 1.0).SetTitle("normalised counts");
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
    plot[0]['Y'].SetRange(0.0, 0.3).SetTitle("#it{v}_{2}");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  const vector<std::tuple<string, string, int, int>> hadrons = {{"pi", "#pi^{#pm}", kRed + 1, kFullCircle}, {"K", "K^{#pm}", kGreen + 2, kFullSquare}, {"p", "p+#bar{p}", kAzure + 2, kFullTriangleUp}};
  {  // identified particles: lighter ones flow more at low pT (mass ordering), baryons more at intermediate pT
    Plot plot("flowIdentified", "heavyion", "1d");
    for (const auto& [name, label, color, marker] : hadrons) {
      plot[1].AddData("flow/v2_" + name, "heavyion", label).SetMarker(marker, 1.4, color).SetLineColor(color);
    }
    plot[1].AddLegend(0.2, 0.9, "Pb-Pb 30-40%, #it{v}_{2}{SP}");
    plot[1]['X'].SetRange(0.0, 6.0).SetTitle("#it{p}_{T} (GeV/#it{c})");
    plot[1]['Y'].SetRange(0.0, 0.32).SetTitle("#it{v}_{2}");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the same per constituent quark against the kinetic energy per quark: one common curve
    Plot plot("quarkScaling", "heavyion", "1d");
    for (const auto& [name, label, color, marker] : hadrons) {
      plot[1].AddData("flow/v2ncq_" + name, "heavyion", label).SetMarker(marker, 1.4, color).SetLineColor(color);
    }
    plot[1].AddLegend(0.2, 0.9, "Pb-Pb 30-40%");
    plot[1]['X'].SetRange(0.0, 2.8).SetTitle("(#it{m}_{T} #minus #it{m}) / #it{n}_{q} (GeV/#it{c}^{2})");
    plot[1]['Y'].SetRange(0.0, 0.12).SetTitle("#it{v}_{2} / #it{n}_{q}");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // flow harmonics against centrality, the class widths as x errors
    Plot plot("flowHarmonics", "heavyion", "1d");
    for (const auto& [n, color] : vector<std::pair<int, int>>{{2, kRed + 1}, {3, kAzure + 2}, {4, kGreen + 2}}) {
      const string harmonic = std::to_string(n);
      plot[1].AddData("flow/v" + harmonic, "heavyion", "#it{v}_{" + harmonic + "}{2}").SetDrawOption(points_xerr).SetColor(color);
    }
    plot[1].AddLegend(0.2, 0.9, "Pb-Pb, |#Delta#eta| > 1");
    plot[1]['X'].SetRange(0.0, 70.0).SetTitle("centrality (%)");
    plot[1]['Y'].SetRange(0.0, 0.14).SetTitle("#it{v}_{#it{n}}");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the yield around the second-order event plane in polar coordinates (angle phi - Psi2, radius pT): each pT ring relative
    // to its mean, so the elliptic modulation of the flow shows how it grows with pT
    Plot plot("azimuthalMap", "heavyion", "2d");
    plot[1].AddData("flow/azimuthalMap", "heavyion").SetDrawOption(colz_polar);
    plot[1].AddText(0.02, 0.985, {"Pb-Pb, 30-40%", "angle: #varphi - #Psi_{2}"});  // in the corner beside the disc
    plot[1]['Z'].SetRange(0.5, 1.5).SetTitle("yield / mean of the ring");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // mean pT against the multiplicity for ten collision systems: all start alike, pp rises steeply, p-Pb follows and bends away,
    // A-A rises in a step and then slowly; the systematic uncertainties as bands behind the points, one colour per system for both;
    // A-A and p-Pb in the first legend, pp in the second (SetLegend), with larger markers than in the plot
    Plot plot("meanPtVsNch", "heavyion", "1d");
    const vector<std::tuple<string, string, string>> systems = {
      {"PbPb_5TeV", "Pb-Pb, 5.02 TeV", "#2b7bab"}, {"PbPb_2.76TeV", "Pb-Pb, 2.76 TeV", "#4fb3f0"}, {"XeXe_5.44TeV", "Xe-Xe, 5.44 TeV", "#1a1f8c"}, {"pPb_8.16TeV", "p-Pb, 8.16 TeV", "#7e2fd0"}, {"pPb_5TeV", "p-Pb, 5.02 TeV", "#cc1f6a"}, {"pp_13TeV", "pp, 13 TeV", "#d81b1b"}, {"pp_8TeV", "pp, 8 TeV", "#ff7a00"}, {"pp_7TeV", "pp, 7 TeV", "#f5b400"}, {"pp_5TeV", "pp, 5.02 TeV", "#1f9e89"}, {"pp_2.76TeV", "pp, 2.76 TeV", "#1d6b1d"}};
    for (const auto& [system, label, hexColor] : systems) {
      plot[1].AddData("meanPt/" + system + "_syst", "heavyion").SetDrawOption(band).SetFill(1001, hexColor, 0.3).SetLine(kSolid, 0.0, hexColor);
    }
    for (const auto& [system, label, hexColor] : systems) {
      plot[1].AddData("meanPt/" + system, "heavyion", label).SetMarker(kFullCircle, 0.6, hexColor).SetLineColor(hexColor).SetLegend(system.rfind("pp", 0) == 0 ? 2 : 1);
    }
    plot[1].AddText(0.18, 0.93, {string(kExperiment) + ", charged particles", "|#eta| < 0.8, 0.15 GeV/#it{c} < #it{p}_{T} < 10 GeV/#it{c}"});
    plot[1].AddLegend(0.18, 0.8).SetDefaultMarkerSize(1.2);
    plot[1].AddLegend(0.65, 0.45).SetDefaultMarkerSize(1.2);
    plot[1]['X'].SetLog().SetRange(0.6, 4000.0).SetTitle("#it{N}_{ch}");
    plot[1]['Y'].SetRange(0.38, 1.08).SetTitle("#LT#it{p}_{T}#GT (GeV/#it{c})");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {                                                            // two-particle correlations in high-multiplicity events: the jet peak cut off by the z range to show the ridge
    Plot plot("ridge", "heavyion/correlations", "1d_3dview");  // the base plot of the lego animation below
    plot[1].AddData("correlations/ridge", "heavyion").SetDrawOption(surf_col).SetLineWidth(1.0);
    plot[1].SetView(40.0, 135.0);
    plot[1].SetPalette(kViridis);
    plot[1]['Z'].SetRange(0.985, 1.03).SetTitleOffset(2.1);  // the jet peak reaches 1.15; the title needs room from the long labels
    plot[1]['X'].SetTitleOffset(1.8);                        // in 3D views the axis titles need more room from the labels
    plot[1]['Y'].SetTitleOffset(1.8);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {                                                                       // the ridge emerging with multiplicity: three classes on one colour scale, the jet peak cut off by the common z range, so the
                                                                          // long-range structure at small Delta phi (and the away side at pi) can be compared between the classes
    Plot plot("ridgeVsMultiplicity", "heavyion/correlations", "2d_3x1");  // pads 1-3, pad 4 is the common scale
    uint8_t pad = 1;
    for (const auto& [name, label] : vector<std::pair<string, string>>{{"ridge_lowMult", "low multiplicity"}, {"ridge_midMult", "intermediate"}, {"ridge", "high multiplicity"}}) {
      plot[pad].AddData("correlations/" + name, "heavyion");
      plot[pad].AddText(top_left, label).SetTextColor(kWhite);
      ++pad;
    }
    plot[0].SetPalette(kViridis);
    plot[0]['Z'].SetRange(0.985, 1.02);  // the jet peak reaches 1.15
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the long-range part projected on Delta phi, with the Fourier fit stored with the data
    Plot plot("longRange", "heavyion/correlations", "1d");
    plot[1].AddData("correlations/ridge", "heavyion", "2 < #Delta#eta < 4").ProjectX(2.0, 4.0, true).SetColor(kBlack);
    plot[1].AddData("correlations/fourier_longRange", "heavyion", "<title>").SetLine(kSolid, 2.0, kRed + 1);
    plot[1].AddLegend(0.2, 0.92);
    plot[1]['Y'].SetTitle("pairs per trigger (a.u.)").SetRange(7.92, 8.2);  // room for the legend
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // femtoscopy: the width of the Bose-Einstein peak measures the size of the source, larger for central collisions
    Plot plot("femtoscopy", "heavyion/correlations", "1d");
    for (const auto& [centrality, color] : vector<std::pair<string, int>>{{"0-5", kRed + 1}, {"20-30", kGreen + 2}, {"50-60", kAzure + 2}}) {
      plot[1].AddData("femtoscopy/cf_" + centrality, "heavyion", centrality + "%").SetColor(color);
      plot[1].AddData("femtoscopy/fit_" + centrality, "heavyion", "<title>").SetLine(kSolid, 2.0, color);
    }
    plot[1].AddLegend(0.32, 0.92, "#pi^{#pm}#pi^{#pm}, Pb-Pb").SetNumColumns(2);
    plot[1]['X'].SetRange(0.0, 0.2).SetTitle("#it{q}_{inv} (GeV/#it{c})");
    plot[1]['Y'].SetRange(0.85, 1.65).SetTitle("#it{C}(#it{q}_{inv})");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the same for direct photons: the Bose-Einstein enhancement is a few 10^-3, as most photons come from pi0 decays, whose
    // two photons make the peak at the pi0 mass; the full range, and the small q_inv with the fit, each panel with its own axes
    Plot plot("photonHBT", "heavyion/correlations", "1d_2x1_gap");
    for (const uint8_t pad : {1, 2}) {
      plot[pad].AddHorizontalLine(1.0).SetLine(kDashed, 1.0, kGray + 1).SetBehindData();
      plot[pad].AddData("femtoscopy/photons_cf", "heavyion", "same / mixed events").SetMarker(kFullCircle, 0.8, kBlack);
      plot[pad].AddData("femtoscopy/photons_fit", "heavyion", "<title>").SetLine(kSolid, 2.0, kRed + 1);
    }
    plot[1].AddText(0.13, 1.13, "#pi^{0} #rightarrow #gamma#gamma").SetUserCoordinates();
    plot[1].AddText(0.2, 0.92, {"#gamma#gamma, Pb-Pb 0-10%", "0.1 < #it{k}_{T} < 0.3 GeV/#it{c}"});
    plot[1]['X'].SetRange(0.0, 0.2);
    plot[1]['Y'].SetRange(0.98, 1.16);
    plot[2].AddLegend(0.2, 0.92);
    plot[2]['X'].SetRange(0.0, 0.08);
    plot[2]['Y'].SetRange(0.99, 1.025).SetNumDivisions(504);  // room for the legend; labels with few digits leave room for the title
    plot[0]['X'].SetTitle("#it{q}_{inv} (GeV/#it{c})");
    plot[0]['Y'].SetTitle("#it{C}(#it{q}_{inv})").SetTitleCenter();  // at the end of the axis it would run off the top of the pad
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
    plot[1].AddData("hBDT_sig_train", "ml", "signal, train").Normalize().SetDrawOption(hist).SetFill(1001, kAzure - 9).SetLineColor(kAzure + 2);
    plot[1].AddData("hBDT_bkg_train", "ml", "background, train").Normalize().SetDrawOption(hist).SetFill(3004, kRed + 1).SetLineColor(kRed + 1);
    plot[1].AddData("hBDT_sig_test", "ml", "signal, test").Normalize().SetColor(kAzure + 2).SetMarkerStyle(kFullCircle);
    plot[1].AddData("hBDT_bkg_test", "ml", "background, test").Normalize().SetColor(kRed + 1).SetMarkerStyle(kFullSquare);
    plot[1].AddLegend(top_left).SetNumColumns(2);
    plot[1]['Y'].SetLog().SetRange(1e-4, 1.0).SetTitle("normalised counts");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // efficiency of a cut on the score: Normalize().Cumulative(false) sums from the right, i.e. the fraction above the cut
    Plot plot("efficiencyVsCut", "ml", "1d");
    plot[1].AddData("hBDT_sig_test", "ml", "signal").Normalize().Cumulative(false).SetDrawOption(line).SetLine(kSolid, 3.0, kAzure + 2);
    plot[1].AddData("hBDT_bkg_test", "ml", "background").Normalize().Cumulative(false).SetDrawOption(line).SetLine(kSolid, 3.0, kRed + 1);
    plot[1].AddLegend(bottom_left);
    plot[1]['X'].SetTitle("cut on the BDT score");
    plot[1]['Y'].SetLog().SetRange(1e-3, 2.0).SetTitle("efficiency (score > cut)");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // ROC curves of two classifiers; <title> puts the title of each graph (with its AUC) into the label
    Plot plot("rocCurve", "ml", "1d");
    plot[1].AddData("roc_BDT", "ml", "<title>").SetDrawOption(line).SetLine(kSolid, 3.0, kAzure + 2);
    plot[1].AddData("roc_DNN", "ml", "<title>").SetDrawOption(line).SetLine(kSolid, 3.0, kRed + 1);
    plot[1].AddFunction("1 - x", "random guess").SetLine(kDashed, 2.0, kGray + 1);
    plot[1].AddLegend(bottom_left);
    plot[1]['X'].SetRange(0.0, 1.0);
    plot[1]['Y'].SetRange(0.0, 1.05);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // feature importance as horizontal bars, the labels of the bins on the y axis
    Plot plot("featureImportance", "ml", "1d");
    plot[1].AddData("hFeatureImportance", "ml").SetDrawOption(hbars).SetFill(1001, kAzure - 9).SetLineColor(kAzure + 2);
    plot[1].SetLeftMargin(0.2);  // room for the bin labels
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // unsupervised learning: a Gaussian mixture separates three groups of points; the cluster map behind them in pale colours,
    // its contours are the learned borders, the ellipses mark 2 sigma of each fitted component
    Plot plot("clustering", "ml", "1d");  // no colour scale: the colours are the clusters
    const vector<color_t> clusterColors = {kAzure + 2, kOrange + 7, kGreen + 2};
    plot[1].SetPalette({{clusterColors[0], 0.0}, {clusterColors[1], 0.5}, {clusterColors[2], 1.0}}, 0.15, 3);  // one pale colour per cluster
    plot[1].AddData("clusterMap", "clustering").SetDrawOption(col);
    for (size_t k = 1; k <= clusterColors.size(); ++k) {
      const string cluster = std::to_string(k);
      plot[1].AddData("points", "clustering", "cluster " + cluster).Filter("cluster == " + cluster).Scatter("x", "y").SetMarker(kFullCircle, 0.7, clusterColors[k - 1]);
      plot[1].AddData("ellipse_" + cluster, "clustering").SetDrawOption(line).SetLine(kDashed, 2.0, clusterColors[k - 1]);
    }
    plot[1].AddData("clusterMap", "clustering", "borders").SetDrawOption(cont).SetContours({1.5, 2.5}).SetLine(kSolid, 2.0, kBlack);
    plot[1].AddData("centres", "clustering", "centres").SetMarker(kFullStar, 2.5, kBlack);
    plot[1].AddLegend(0.18, 0.92).SetNumColumns(2);
    plot[1]['X'].SetRange(-4.0, 4.0).SetTitle("feature 1");
    plot[1]['Y'].SetRange(-4.0, 6.0).SetTitle("feature 2");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  // the components of the mixture density network below: name and colour
  const vector<std::pair<string, color_t>> components = {{"core", kAzure + 2}, {"tail 1", kViolet + 1}, {"tail 2", kOrange + 7}, {"tail 3", kGreen + 2}};
  {  // a mixture density network for the electron pT resolution: bremsstrahlung in the material gives a tail to negative values, the
    // same at all pT; the core widens at high pT. The network predicts four Gaussians per pT_gen (the core and three for the tail):
    // mean +- sigma, the weights below
    Plot plot("mixtureComponents", "ml", "1d_wide_1x2");
    for (size_t k = 1; k <= components.size(); ++k) {
      const auto& [label, color] = components[k - 1];
      const string component = std::to_string(k);
      plot[1].AddData("component_" + component, "mdn", label + " (#mu #pm #sigma)").SetDrawOption(band).SetFill(1001, color, 0.4).SetLineColor(color).SetLegendEntry(LegendEntry().SetSymbolElements("F"));
      plot[1].AddData("component_" + component, "mdn").SetDrawOption(line).SetLine(kSolid, 2.0, color);  // the mean
      plot[2].AddData("weight_" + component, "mdn").SetDrawOption(points_line).SetMarkerStyle(kFullCircle).SetMarkerSize(0.8).SetColor(color);
    }
    plot[1].AddRegionX(2.0, 2.5).SetFill(1001, kGray, 0.5).SetBehindData();  // the slice of the plot mixtureSlice
    plot[1].AddLegend(0.12, 0.93).SetNumColumns(4);
    plot[1]['Y'].SetRange(-0.7, 0.2).SetTitle("(#it{p}_{T}^{rec} #minus #it{p}_{T}^{gen}) / #it{p}_{T}^{gen}");
    plot[2]['Y'].SetRange(0.0, 1.0).SetTitle("weight");
    plot[0]['X'].SetLog().SetRange(0.5, 50.0).SetTitle("#it{p}_{T}^{gen} (GeV/#it{c})");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // at 2 < pT_gen < 2.5 GeV/c: the predicted density, its weighted components and the simulated electrons
    Plot plot("mixtureSlice", "ml", "1d");
    plot[1].AddData("electrons", "mdn", "simulated electrons").Filter("ptGen > 2.0 && ptGen < 2.5").Project1D({"deltaPt", 115, {-1.0, 0.15}}).Normalize(true).SetMarker(kFullCircle, 0.9, kBlack);
    for (size_t k = 1; k <= components.size(); ++k) {
      const auto& [label, color] = components[k - 1];
      plot[1].AddData("slice_component_" + std::to_string(k), "mdn", label + ", weight <title>").SetDrawOption(area).SetFill(1001, color, 0.3).SetLineColor(color);
    }
    plot[1].AddData("slice_total", "mdn", "mixture").SetDrawOption(line).SetLine(kSolid, 3.0, kRed + 1);
    plot[1].AddText(0.2, 0.92, {"electrons, |#eta| < 0.9", "2 < #it{p}_{T}^{gen} < 2.5 GeV/#it{c}"});
    plot[1].AddLegend(0.2, 0.78);
    plot[1]['X'].SetRange(-1.0, 0.15).SetTitle("(#it{p}_{T}^{rec} #minus #it{p}_{T}^{gen}) / #it{p}_{T}^{gen}");
    plot[1]['Y'].SetLog().SetRange(2.0e-3, 2000.0).SetTitle("probability density");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
}

//**************************************************************************************************
// Medical physics: depth-dose curves of ion beams in water and the dose of a treatment plan
//**************************************************************************************************
void DefineMedicalPlots(PlotManager& pm)
{
  {  // the Bragg peak moves deeper with the energy of the protons: one colour per energy from a gradient
    Plot plot("braggPeaks", "medical", "1d");
    const vector<int> energies = {100, 150, 200};
    plot[0].SetDefaultColors({{0.1, 0.3, 0.8, 0.0}, {0.8, 0.1, 0.1, 1.0}}, {}, energies.size());
    for (const int energy : energies) {
      plot[1].AddData("depthDose/proton_" + std::to_string(energy) + "MeV", "medical", std::to_string(energy) + " MeV").SetDrawOption(curve);
    }
    plot[1].AddLegend(0.38, 0.93, "protons in water");
    plot[1]['Y'].SetRange(0.0, 150.0);  // room for the legend above the peaks
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // protons and carbon ions of the same range: the carbon peak is sharper, its fragments deposit dose behind it
    Plot plot("protonVsCarbon", "medical", "1d");
    plot[1].AddData("depthDose/proton_150MeV", "medical", "protons, 150 MeV").SetDrawOption(curve).SetColor(kAzure + 2);
    plot[1].AddData("depthDose/carbon_280MeVu", "medical", "^{12}C, 280 MeV/#it{u}").SetDrawOption(curve).SetColor(kRed + 1);
    plot[1].AddLegend(0.2, 0.85);
    plot[1]['Y'].SetLog().SetRange(1.0, 300.0);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // dose map of a two-field plan with isodose lines on top: the same histogram drawn twice
    Plot plot("treatmentPlan", "medical", "2d");
    plot[1].AddData("plan/doseMap", "medical").SetDrawOption(colz);
    plot[1].AddData("plan/doseMap", "medical").SetDrawOption(cont).SetContours({20.0, 50.0, 80.0, 95.0}).SetLine(kSolid, 2.0, kBlack);
    plot[1].SetPalette(kTemperatureMap);
    plot[1].AddText(0.15, 0.3, "isodose lines 20, 50, 80, 95%");
    plot[1]['Y'].SetRange(-75.0, 100.0);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
}

//**************************************************************************************************
// Atomic physics: laser spectroscopy, strong-field ionisation, hydrogen orbitals and the photoelectric effect
//**************************************************************************************************
void DefineAtomicPlots(PlotManager& pm)
{
  {  // the fit stored with the data, and the data divided by it below
    Plot plot("hyperfineScan", "atomic", "1d_ratio");
    plot[1].AddData("hScan", "atomic", "data");
    plot[1].AddData("fitScan", "atomic", "fit: 4 Voigt profiles").SetLine(kSolid, 3.0, kRed + 1);
    plot[1].AddLegend(0.2, 0.85);
    plot[1].AddText(0.2, 0.7, {"laser spectroscopy", "hyperfine structure of ^{A}X"});
    plot[1]['Y'].SetRange(0.0, 850.0);  // room for the texts above the lines
    plot[2].AddRatio({"hScan", "atomic"}, {"fitScan"});
    plot[2]['Y'].SetTitle("data / fit").SetRange(0.5, 1.5);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // Doppler-free spectroscopy: the pump burns narrow dips into the Doppler profile, the ratio shows them alone
    Plot plot("saturatedAbsorption", "atomic", "1d_ratio");
    plot[1].AddData("laser/probeOnly", "atomic", "probe only").SetDrawOption(line).SetLine(kSolid, 2.0, kGray + 2);
    plot[1].AddData("laser/withPump", "atomic", "probe with pump").SetDrawOption(line).SetLine(kSolid, 2.0, kRed + 1);
    plot[1].AddLegend(0.2, 0.3, "^{87}Rb D_{2}, #it{F} = 2 #rightarrow #it{F}'");
    plot[1]['Y'].SetTitle("probe transmission");
    plot[2].AddRatio({"laser/withPump", "atomic"}, {"laser/probeOnly"}).SetDrawOption(line).SetLine(kSolid, 2.0, kRed + 1);
    // the lines (F') and the strongest crossover, halfway between F' = 2 and 3, centred above their peaks
    for (const auto& [x, label] : vector<std::pair<double, string>>{{-95.0, "1"}, {-22.0, "2"}, {115.0, "c"}, {245.0, "3"}}) {
      plot[2].AddText(x, 1.43, label).SetUserCoordinates();
    }
    plot[2].AddText(0.2, 0.85, "c: crossover 2-3");
    plot[2]['Y'].SetTitle("ratio").SetRange(0.97, 1.45);
    plot[0]['X'].SetRange(-700.0, 700.0).SetTitle("detuning from #it{F}' = 2 (MHz)");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // Rabi oscillations: measured points and the expectation in the same colour, one colour per detuning
    Plot plot("rabiOscillations", "atomic", "1d");
    for (const auto& [detuning, color] : vector<std::pair<int, int>>{{0, kAzure + 2}, {1, kRed + 1}, {2, kGreen + 2}}) {
      const string tag = std::to_string(detuning) + "MHz";
      plot[1].AddData("laser/rabi_" + tag, "atomic", "#Delta/2#pi = " + std::to_string(detuning) + " MHz").SetColor(color);
      plot[1].AddData("laser/theory_" + tag, "atomic").SetLine(kSolid, 2.0, color);  // no label: no legend entry
    }
    plot[1].AddLegend(0.65, 0.92, "#Omega/2#pi = 1 MHz");
    plot[1]['X'].SetTitle("pulse duration (#mus)");
    plot[1]['Y'].SetTitle("excited-state population").SetRange(0.0, 1.3);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // photoelectron spectra of xenon at three peak intensities: the ATI peaks move down with the ponderomotive energy
    // and low orders close, the Freeman resonances (Rydberg states shifted into resonance during the pulse) stay
    Plot plot("freemanResonances", "atomic", "1d");
    const vector<int> intensities = {1, 2, 4};
    plot[0].SetDefaultColors({{0.1, 0.3, 0.8, 0.0}, {0.8, 0.1, 0.1, 1.0}}, {}, intensities.size());
    for (const int intensity : intensities) {
      const string i = std::to_string(intensity);
      plot[1].AddData("strongField/xenon_" + i + "e13", "atomic", i + " #times 10^{13} W/cm^{2}").SetDrawOption(hist);
    }
    plot[1].AddText(0.6, 3.e5, "4f").SetUserCoordinates();  // the first-order Freeman peaks of two states
    plot[1].AddText(0.93, 3.e5, "5f").SetUserCoordinates();
    plot[1].AddLegend(0.5, 0.92, "Xe, 800 nm");
    plot[1]['X'].SetRange(0.0, 5.0);
    plot[1]['Y'].SetLog().SetRange(1.0, 1.e7);  // room for the legend above the peaks
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // above-threshold ionisation at high intensity: direct electrons up to 2 U_p, rescattered ones up to 10 U_p
    Plot plot("atiPlateau", "atomic", "1d");
    plot[1].AddData("strongField/argon_1e14", "atomic", "Ar, 800 nm, 10^{14} W/cm^{2}").SetDrawOption(hist).SetColor(kAzure + 2);
    plot[1].AddPoints({11.9, 11.9}, {1.0, 1.e6}, "2#it{U}_{p}").SetDrawOption(line).SetLine(kDashed, 2.0, kGray + 2);
    plot[1].AddPoints({59.7, 59.7}, {1.0, 1.e6}, "10#it{U}_{p}").SetDrawOption(line).SetLine(kDashed, 2.0, kRed + 1);
    plot[1].AddText(1.0, 1.e7, "direct").SetUserCoordinates();
    plot[1].AddText(28.0, 5.e3, "rescattered").SetUserCoordinates();
    plot[1].AddLegend(0.45, 0.9);
    plot[1]['Y'].SetLog().SetRange(1.0, 1.e8);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // a velocity-map image: the momentum slice of the photoelectrons, the laser polarised along z
    Plot plot("vmiImage", "atomic", "2d_logz");
    plot[1].AddData("strongField/vmi", "atomic").Project2D({"px", 140, {-0.7, 0.7}}, {"pz", 140, {-0.7, 0.7}});
    plot[1]['X'].SetTitle("#it{p}_{#it{x}} (a.u.)");
    plot[1]['Y'].SetTitle("#it{p}_{#it{z}} (a.u.)");
    plot[1]['Z'].SetTitle("electrons");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the angular distribution of two ATI rings from the same image: energy and angle defined from the momenta, a ring
    // selected by its energy; the Legendre fits (their beta parameters in the titles) in the colour of their ring
    Plot plot("angularDistributions", "atomic", "1d");
    const vector<std::tuple<string, string, string, int>> rings = {{"1st ATI ring", "energy > 0.8 && energy < 1.2", "fitRing1", kAzure + 2},
                                                                   {"2nd ATI ring", "energy > 2.3 && energy < 2.8", "fitRing2", kRed + 1}};
    for (const auto& [label, ring, fit, color] : rings) {
      plot[1].AddData("strongField/vmi", "atomic", label).Define("energy", "13.6055 * (px * px + pz * pz)").Define("cosTheta", "pz / sqrt(px * px + pz * pz)").Project1D({"cosTheta", 40, {-1.0, 1.0}}).Filter(ring).SetColor(color);
      plot[1].AddData("strongField/" + fit, "atomic", "<title>").SetLine(kSolid, 2.0, color);
    }
    plot[1].AddLegend(0.2, 0.92);
    plot[1]['X'].SetTitle("cos #theta");
    plot[1]['Y'].SetTitle("electrons").SetRange(0.0, 4500.0);  // room for the legend
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // photoelectron angular distributions, the light polarised along the angle 0: the cos^2 dipole of He 1s (beta = 2) and the
    // rounder Ar 3p; the inputs are polar graphs, drawn on a polar grid in place of the axes (x the angle, y the radius)
    Plot plot("photoelectronAngles", "atomic", "1d");
    for (const auto& [name, label, color] : vector<std::tuple<string, string, int>>{{"he1s", "He 1s, #beta = 2", kAzure + 2}, {"ar3p", "Ar 3p, #beta = 0.88", kRed + 1}}) {
      plot[1].AddData("angular/" + name + "_model", "atomic").SetDrawOption(line).SetLine(kSolid, 2.0, color);
      plot[1].AddData("angular/" + name, "atomic", label).SetDrawOption(points).SetColor(color).SetDefinesFrame();
    }
    plot[1].AddLegend(0.02, 0.985);  // in the corner above the grid, which fills the pad
    plot[1].AddText(bottom_left, "1 + #beta #it{P}_{2}(cos #theta)");
    plot[1]['Y'].SetRange(0.0, 3.3);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // four orbitals of hydrogen in a grid of maps sharing their axes and one colour scale (pad 3 is the scale)
    Plot plot("orbitals", "atomic", "2d_2x2");
    const vector<std::pair<string, string>> orbitals = {{"psi_2pz", "2p_{z}"}, {"psi_3pz", "3p_{z}"}, {"psi_3dz2", "3d_{z^{2}}"}, {"psi_3dxz", "3d_{xz}"}};
    const vector<uint8_t> pads = {1, 2, 4, 5};
    for (size_t i = 0; i < orbitals.size(); ++i) {
      const uint8_t pad = pads[i];
      plot[pad].AddData("orbitals/" + orbitals[i].first, "atomic").SetDrawOption(colz);
      plot[pad].AddText(0.15, 0.88, orbitals[i].second).SetTextColor(kWhite);
    }
    plot[0].SetPalette(kViridis);
    plot[0]['Z'].SetLog().SetRange(0.005, 6.0);  // the densities of the shells differ by an order of magnitude
    plot[0].SetFrameFill(1001, kBlack);          // behind the cells below the range
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // radial probability densities in independent panels, one per shell; one colour per angular momentum
    Plot plot("radialDensities", "atomic", "1d_3x1_gap");
    const std::map<char, int> colors = {{'s', kBlack}, {'p', kAzure + 2}, {'d', kRed + 1}};
    for (const uint8_t n : {1, 2, 3}) {
      for (const char orbital : string("spd").substr(0, n)) {
        const string state = std::to_string(n) + orbital;
        plot[n].AddData("orbitals/radial_" + state, "atomic", state).SetLine(kSolid, 3.0, colors.at(orbital));
      }
      plot[n].AddLegend(0.6, 0.9, (n == 1) ? std::optional<string>("hydrogen") : std::nullopt);
      plot[n]['X'].SetRange(0.0, 10.0 * n).SetTitle("#it{r} (#it{a}_{0})");
      plot[n]['Y'].SetTitle("#it{r}^{2}|#it{R}_{#it{nl}}|^{2} (#it{a}_{0}^{-1})").SetRange(0.0, 0.65 / n);
    }
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // Millikan's measurement of Planck's constant: the slope of the stopping voltage against the frequency is h/e
    Plot plot("millikan", "atomic", "1d");
    plot[1].AddData("photoeffect/stoppingVoltage", "atomic", "Hg lines, Cs cathode").SetColor(kBlack);
    plot[1].AddData("photoeffect/fitStopping", "atomic", "<title>").SetLine(kSolid, 2.0, kRed + 1);
    plot[1].AddHorizontalLine(0.0).SetLine(kDashed, 1.0, kGray + 1);
    plot[1].AddLegend(0.2, 0.9);
    plot[1]['X'].SetRange(4.0, 9.0).SetTitle("frequency #nu (10^{14} Hz)");
    plot[1]['Y'].SetRange(-0.4, 2.0).SetTitle("stopping voltage (V)");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // photocurrent against the voltage: each colour of light sets in at its own stopping voltage
    Plot plot("photocurrent", "atomic", "1d");
    for (const auto& [wavelength, color] : vector<std::pair<int, int>>{{365, kViolet + 1}, {436, kAzure + 2}, {546, kGreen + 2}}) {
      const string w = std::to_string(wavelength);
      plot[1].AddData("photoeffect/current_" + w + "nm", "atomic", w + " nm").SetColor(color);
    }
    plot[1].AddLegend(0.2, 0.9, "Hg lamp");
    plot[1]['X'].SetTitle("voltage #it{U} (V)");
    plot[1]['Y'].SetTitle("photocurrent (nA)").SetRange(0.0, 15.0);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // how photons are absorbed in lead: photoeffect with its absorption edges, Compton scattering, pair production
    Plot plot("photonAttenuation", "atomic", "1d");
    plot[1].AddData("photoeffect/lead_total", "atomic", "total").SetDrawOption(line).SetLine(kSolid, 3.0, kBlack);
    plot[1].AddData("photoeffect/lead_photo", "atomic", "photoeffect").SetDrawOption(line).SetLine(kSolid, 2.0, kRed + 1);
    plot[1].AddData("photoeffect/lead_compton", "atomic", "Compton scattering").SetDrawOption(line).SetLine(kSolid, 2.0, kAzure + 2);
    plot[1].AddData("photoeffect/lead_pair", "atomic", "pair production").SetDrawOption(line).SetLine(kSolid, 2.0, kGreen + 2);
    plot[1].AddText(0.075, 18.0, "K edge").SetUserCoordinates();
    plot[1].AddText(0.0115, 350.0, "L edges").SetUserCoordinates();
    plot[1].AddLegend(0.55, 0.9, "lead");
    plot[1]['X'].SetLog().SetRange(0.01, 100.0).SetTitle("photon energy (MeV)");
    plot[1]['Y'].SetLog().SetRange(1.e-3, 1.e3).SetTitle("#mu/#rho (cm^{2}/g)");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
}

//**************************************************************************************************
// Accelerator physics: the profile of a beam and its phase space
//**************************************************************************************************
void DefineAcceleratorPlots(PlotManager& pm)
{
  {  // a beam profile with its projections in panels of their own: an own layout, the projections sharing the axes
    // of the map, and the colour scale of the map (pad 3) in a cell of its own beyond the projection on the right;
    // PanelLayout arranges the panels and gives them the look of the shipped base plots
    Plot profileLayout("profileWithProjections", PanelLayout({{Panel(1.0, 0.4), Empty().Span(2)},
                                                              {Panel(), Panel(0.4, 1.0), ZAxis(3)}}));
    pm.AddBasePlot(profileLayout);
    Plot plot("beamProfile", "accelerator", "profileWithProjections");  // pads: 1 above, 2 the empty cell, 3 the map, 4 right, 5 its colour scale
    plot[3].AddData("hBeamSpot", "accelerator").SetDrawOption(colz);
    plot[3].SetPalette({{"#e8f2ff", 0.0}, {kAzure - 9, 0.35}, {kAzure + 2, 0.7}, {kAzure + 3, 1.0}});  // pale to dark azure, as the projections
    plot[1].AddData("hBeamSpot", "accelerator").ProjectX().NormalizeToMaximum().SetDrawOption(hist).SetFill(1001, kAzure - 9).SetLineWidth(0.0);
    plot[4].AddData("hBeamSpot", "accelerator").ProjectY().NormalizeToMaximum().SetDrawOption(hbars).SetFill(1001, kAzure - 9);
    plot[2].AddText(0.05, 0.8, {"screen 3", "#sigma_{#it{x}} = <stddev[.1f]:1@1> mm", "#sigma_{#it{y}} = <stddev[.1f]:1@4> mm"});
    plot[1]['Y'].SetRange(0.0, 1.1).SetTitle("");
    plot[4]['Y'].SetTitle("");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the particles of a bunch in horizontal phase space, with the rms ellipse (the emittance in its title)
    Plot plot("phaseSpace", "accelerator", "2d_logz");
    plot[1].AddData("beam", "accelerator").Project2D({"x", 80, {-8.0, 8.0}}, {"xp", 88, {-2.2, 2.2}});
    plot[1].AddData("rmsEllipse", "accelerator", "rms ellipse, #varepsilon = <title> mm mrad").SetDrawOption(line).SetLine(kSolid, 3.0, kRed + 1);
    plot[1].AddLegend(0.15, 0.92);
    plot[1]['X'].SetTitle("#it{x} (mm)");
    plot[1]['Y'].SetTitle("#it{x}' (mrad)");
    plot[1]['Z'].SetTitle("particles");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the beta functions along a FODO lattice: the beam is widest in x at the focusing, in y at the defocusing quadrupoles
    Plot plot("betaFunctions", "accelerator", "1d_wide");
    plot[1].AddData("lattice/betaX", "accelerator", "#beta_{#it{x}}").SetDrawOption(line).SetLine(kSolid, 3.0, kAzure + 2);
    plot[1].AddData("lattice/betaY", "accelerator", "#beta_{#it{y}}").SetDrawOption(line).SetLine(kSolid, 3.0, kRed + 1);
    for (const double s : {0.0, 10.0, 20.0, 30.0, 40.0, 50.0, 60.0}) {  // the quadrupoles: shapes behind the data, no legend rows
      plot[1].AddLine({s, 0.0}, {s, 40.0}).SetLine((std::fmod(s, 20.0) == 0.0) ? kSolid : kDashed, 1.0, kGray + 1).SetBehindData();
    }
    plot[1].AddText(0.13, 0.92, {"FODO lattice, 90#circ phase advance per cell", "quadrupoles focusing in #it{x} (solid) and #it{y} (dashed)"});
    plot[1].AddLegend(0.75, 0.92).SetNumColumns(2);
    plot[1]['X'].SetRange(0.0, 60.0).SetTitle("#it{s} (m)");
    plot[1]['Y'].SetRange(0.0, 55.0).SetTitle("#beta (m)");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
}

//**************************************************************************************************
// Nuclear and neutrino physics: beta decay spectra, the neutrino mass at the tritium endpoint, reactor neutrino oscillations
//**************************************************************************************************
void DefineNuclearPlots(PlotManager& pm)
{
  {  // the tritium beta spectrum
    Plot plot("tritiumSpectrum", "nuclear", "1d");
    plot[1].AddData("betaDecay/tritium", "nuclear", "^{3}H #rightarrow ^{3}He e^{#minus} #bar{#nu}_{e}").SetDrawOption(hist).SetColor(kAzure + 2);
    plot[1].AddLegend(0.45, 0.9);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // Kurie plot: a straight line for an allowed decay, crossing zero at the endpoint (from the fit, in its title)
    Plot plot("kuriePlot", "nuclear", "1d");
    plot[1].AddData("betaDecay/kurie", "nuclear", "tritium").SetColor(kBlack);
    plot[1].AddData("betaDecay/kurieFit", "nuclear", "<title>").SetLine(kSolid, 2.0, kRed + 1);
    plot[1].AddLegend(0.45, 0.9);
    plot[1]['X'].SetRange(0.0, 19.5).SetTitle("electron kinetic energy (keV)");
    plot[1]['Y'].SetRange(0.0, 1.2).SetTitle("#sqrt{#it{N} / (#it{F} #it{p} #it{E})} (a.u.)");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the last eV of the spectrum: a neutrino mass moves the endpoint and bends the spectrum before it
    Plot plot("tritiumEndpoint", "nuclear", "1d");
    const vector<std::pair<string, string>> masses = {{"0eV", "0"}, {"05eV", "0.5"}, {"1eV", "1"}};
    plot[0].SetDefaultColors({{0.1, 0.3, 0.8, 0.0}, {0.8, 0.1, 0.1, 1.0}}, {}, masses.size());
    for (const auto& [name, mass] : masses) {
      plot[1].AddData("betaDecay/endpoint_" + name, "nuclear", "#it{m}_{#nu} = " + mass + " eV").SetLineWidth(3.0);  // colour from the gradient
    }
    plot[1].AddLegend(0.55, 0.9);
    plot[1]['X'].SetRange(-3.0, 0.5).SetTitle("#it{E} #minus #it{E}_{0} (eV)");
    plot[1]['Y'].SetRange(0.0, 10.0).SetTitle("rate (a.u.)");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // electrons and positrons from the same nucleus: the Coulomb field slows one and pushes the other
    Plot plot("copperBeta", "nuclear", "1d");
    plot[1].AddData("betaDecay/cu64_betaMinus", "nuclear", "#beta^{#minus}: ^{64}Cu #rightarrow ^{64}Zn").SetDrawOption(line).SetLine(kSolid, 3.0, kAzure + 2);
    plot[1].AddData("betaDecay/cu64_betaPlus", "nuclear", "#beta^{+}: ^{64}Cu #rightarrow ^{64}Ni").SetDrawOption(line).SetLine(kSolid, 3.0, kRed + 1);
    plot[1].AddLegend(0.5, 0.9);
    plot[1]['X'].SetRange(0.0, 700.0).SetTitle("kinetic energy (keV)");
    plot[1]['Y'].SetRange(0.0, 3.5).SetTitle("d#it{N}/d#it{T} (MeV^{#minus1})");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // reactor antineutrinos disappear and come back with L/E: the oscillation, washed out by the spread of distances
    Plot plot("reactorOscillation", "neutrino", "1d");
    plot[1].AddData("survival", "neutrino", "data #minus background").SetDrawOption(points_xerr).SetColor(kBlack);
    plot[1].AddData("expectation", "neutrino", "oscillation, best fit").SetDrawOption(line).SetLine(kSolid, 2.0, kRed + 1);
    plot[1].AddPoints({15.0, 105.0}, {1.0, 1.0}, "no oscillation").SetDrawOption(line).SetLine(kDashed, 2.0, kGray + 2);
    plot[1].AddLegend(0.45, 0.92);
    plot[1]['X'].SetRange(15.0, 105.0).SetTitle("#it{L}_{0} / #it{E}_{#bar{#nu}_{e}} (km/MeV)");
    plot[1]['Y'].SetRange(0.0, 1.4).SetTitle("survival probability");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the allowed region of the oscillation parameters: Delta chi2 as colour, its 1, 2, 3 sigma contours on top
    Plot plot("oscillationParameters", "neutrino", "2d");
    plot[1].AddData("deltaChi2", "neutrino").SetDrawOption(colz);
    plot[1].AddData("deltaChi2", "neutrino").SetDrawOption(cont).SetContours({2.30, 6.18, 11.83}).SetLine(kSolid, 2.0, kWhite);
    plot[1].AddData("bestFit", "neutrino").SetMarker(kFullStar, 2.5, kWhite);
    plot[1].AddText(0.15, 0.92, "contours: 1#sigma, 2#sigma, 3#sigma");
    plot[1].SetPalette(kDarkBodyRadiator);  // dark at the minimum, fading to white where the parameters are excluded
    plot[1]['X'].SetRange(0.6, 1.0);
    plot[1]['Y'].SetRange(6.7, 8.5);
    plot[1]['Z'].SetRange(0.0, 30.0);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // neutrino-nucleon cross-sections from quasi-elastic scattering to deep-inelastic scattering, with measurements
    Plot plot("crossSections", "neutrino", "1d");
    for (const auto& [name, label, color] : vector<std::tuple<string, string, int>>{{"qe", "quasi-elastic", kAzure + 2}, {"res", "resonance production", kGreen + 2}, {"dis", "deep-inelastic", kOrange + 7}}) {
      plot[1].AddData("crossSection/" + name, "neutrino", label).SetDrawOption(line).SetLine(kSolid, 2.0, color);
    }
    plot[1].AddData("crossSection/total", "neutrino", "total").SetDrawOption(line).SetLine(kSolid, 3.0, kBlack);
    for (const auto& [name, label, marker, color] :
         vector<std::tuple<string, string, int, int>>{{"data_A", "experiment A", kFullCircle, kBlack}, {"data_B", "experiment B", kOpenSquare, kViolet + 1}, {"data_C", "experiment C", kFullTriangleUp, kGray + 2}}) {
      plot[1].AddData("crossSection/" + name, "neutrino", label).SetMarkerStyle(marker).SetMarkerSize(1.3).SetColor(color);
    }
    plot[1].AddText(0.2, 0.92, "#nu_{#mu} #it{N} #rightarrow #mu^{#minus} #it{X}");
    plot[1].AddLegend(0.2, 0.86).SetNumColumns(2);
    plot[1]['X'].SetLog().SetRange(0.1, 100.0).SetTitle("#it{E}_{#nu} (GeV)");
    plot[1]['Y'].SetRange(0.0, 2.0).SetTitle("#sigma / #it{E}_{#nu} (10^{#minus38} cm^{2}/GeV)");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
}

//**************************************************************************************************
// Searches: exclusion limits on dark matter and on top squarks
//**************************************************************************************************
void DefineSearchPlots(PlotManager& pm)
{
  {  // limits of three experiments, the expectation of the newest with its bands, the neutrino fog beneath
    Plot plot("darkMatterLimits", "searches/darkMatter", "1d");
    plot[1].AddData("darkMatter/neutrinoFog", "searches", "neutrino fog").SetDrawOption(band).SetFill(1001, kGray).SetLineColor(kGray).SetLegendEntry(LegendEntry().SetSymbolElements("F"));
    plot[1].AddData("darkMatter/expected_A_2sigma", "searches", "expected A #pm 2#sigma").SetDrawOption(band).SetFill(1001, kOrange).SetLineColor(kOrange).SetLegendEntry(LegendEntry().SetSymbolElements("F"));
    plot[1].AddData("darkMatter/expected_A_1sigma", "searches", "expected A #pm 1#sigma").SetDrawOption(band).SetFill(1001, kGreen + 1).SetLineColor(kGreen + 1).SetLegendEntry(LegendEntry().SetSymbolElements("F"));
    for (const auto& [name, label, color] : vector<std::tuple<string, string, int>>{{"limit_A", "experiment A (2025)", kBlack}, {"limit_B", "experiment B (2020)", kAzure + 2}, {"limit_C", "experiment C, low threshold", kRed + 1}}) {
      plot[1].AddData("darkMatter/" + name, "searches", label).SetDrawOption(line).SetLine(kSolid, 3.0, color);
    }
    plot[1].AddLegend(0.38, 0.92, "90% CL upper limits");
    plot[1]['X'].SetLog().SetRange(1.0, 1.e4).SetTitle("WIMP mass (GeV/#it{c}^{2})");
    plot[1]['Y'].SetLog().SetRange(1.e-50, 1.e-38).SetTitle("#sigma_{SI} (cm^{2})");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the mass plane of a top-squark search: the upper limit as colour, the excluded region as contours
    Plot plot("stopExclusion", "searches/stop", "2d_logz");
    plot[1].AddData("stop/upperLimit", "searches").SetDrawOption(colz);
    plot[1].AddData("stop/expected", "searches", "expected #pm 1#sigma_{exp}").SetDrawOption(line).SetLine(kDashed, 3.0, kRed + 1);
    for (const string variation : {"expected_up", "expected_down"}) {
      plot[1].AddData("stop/" + variation, "searches").SetDrawOption(line).SetLine(kDashed, 1.0, kRed + 1);
    }
    plot[1].AddData("stop/observed", "searches", "observed #pm 1#sigma_{theory}").SetDrawOption(line).SetLine(kSolid, 803.0, kBlack).SetFill(3354, kBlack);  // 803: hatches of width 8 on the excluded side
    for (const string variation : {"observed_theoryUp", "observed_theoryDown"}) {
      plot[1].AddData("stop/" + variation, "searches").SetDrawOption(line).SetLine(kSolid, 1.0, kBlack);
    }
    plot[1].AddPoints({200.0, 993.0}, {7.0, 800.0}, "#it{m}(#tilde{t}) = #it{m}(t) + #it{m}(#tilde{#chi}^{0}_{1})").SetDrawOption(line).SetLine(kDotted, 2.0, kGray + 2);
    plot[1].AddText(0.17, 0.95, "#tilde{t} #tilde{t}*, #tilde{t} #rightarrow t #tilde{#chi}^{0}_{1}");
    plot[1].AddLegend(0.17, 0.89).SetNumColumns(2);
    plot[1]['Y'].SetRoomAboveMax(0.25).SetNoExponent().SetTitleOffset(1.2);  // a quarter of the frame free above the data, for the legend; labels with four digits
    plot[1]['Z'].SetTitle("95% CL upper limit on #sigma (pb)").SetRange(3.e-3, 0.1);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // a bump hunt: the dijet mass spectrum with a smooth fit, below it the significance of each bin's deviation
    Plot plot("dijetMass", "searches/resonance", "1d_ratio");
    plot[1].AddData("dijet/spectrum", "searches", "data").SetMarker(kFullCircle, 0.9, kBlack);
    plot[1].AddData("dijet/fit", "searches", "<title>").SetLine(kSolid, 2.0, kRed + 1);
    plot[2].AddData("dijet/significance", "searches").SetRangeX(1.5, 6.5).SetDrawOption(hist).SetFill(1001, kRed - 9).SetLineColor(kRed + 1);  // empty bins above
    // what a signal would look like: excited quarks of two masses on top of the fit, and below the significance each would give
    for (const auto& [mass, color] : vector<std::pair<int, int>>{{3, kAzure + 2}, {5, kGreen + 2}}) {
      plot[1].AddData(Form("dijet/sigPlusBkg_%dTeV", mass), "searches", Form("fit + excited quark, %d TeV", mass)).SetDrawOption(line).SetLine(kSolid, 3.0, color);
      plot[2].AddData(Form("dijet/significance_%dTeV", mass), "searches").SetRangeX(0.79 * mass, 1.21 * mass).SetDrawOption(hist).SetLine(kSolid, 3.0, color);  // 3 sigma
    }
    plot[1].AddText(0.45, 0.9, {kExperiment, string(kCollisions) + ", 140 fb^{-1}", "|#it{y}*| < 0.6"});
    plot[1].AddLegend(0.45, 0.68);
    plot[1]['Y'].SetLog().SetRange(0.2, 1.e7);
    plot[2].SetRefFunc("0");
    plot[2]['Y'].SetRange(-3.5, 4.5).SetTitle("signif.");
    plot[0]['X'].SetRange(1.5, 7.0);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
}

//**************************************************************************************************
// Neutron stars: equations of state, masses and radii, and a merger seen in gravitational waves and light
//**************************************************************************************************
void DefineNeutronStarPlots(PlotManager& pm)
{
  const vector<std::pair<string, int>> eos = {{"soft", kAzure + 2}, {"medium", kGreen + 2}, {"stiff", kRed + 1}};
  {  // three equations of state, pressure against energy density
    Plot plot("equationOfState", "neutronStars", "1d");
    for (const auto& [name, color] : eos) {
      plot[1].AddData("structure/eos_" + name, "neutronstars", name).SetDrawOption(line).SetLine(kSolid, 3.0, color);
    }
    plot[1].AddLegend(0.2, 0.9, "polytropes");
    plot[1]['X'].SetLog().SetRange(60.0, 2000.0).SetTitle("energy density #varepsilon (MeV/fm^{3})");
    plot[1]['Y'].SetLog().SetTitle("pressure #it{p} (MeV/fm^{3})");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the mass-radius relations they give, against measurements: a heavy pulsar rules out the soft one
    Plot plot("massRadius", "neutronStars", "1d");
    plot[1].AddData("structure/pulsar_J0740", "neutronstars", "J0740 mass").SetDrawOption(band).SetFill(1001, kGray).SetLineColor(kGray).SetLegendEntry(LegendEntry().SetSymbolElements("F"));
    for (const auto& [name, color] : eos) {
      plot[1].AddData("structure/massRadius_" + name, "neutronstars", name).SetDrawOption(line).SetLine(kSolid, 3.0, color);
    }
    plot[1].AddData("structure/nicer_J0030", "neutronstars", "NICER J0030").SetDrawOption(line).SetLine(kSolid, 2.0, kViolet + 1);
    plot[1].AddData("structure/nicer_J0740", "neutronstars", "NICER J0740").SetDrawOption(line).SetLine(kSolid, 2.0, kOrange + 7);
    plot[1].AddLegend(0.18, 0.92).SetNumColumns(2);
    plot[1]['X'].SetRange(8.0, 16.0).SetTitle("radius (km)");
    plot[1]['Y'].SetRange(0.0, 3.6).SetTitle("mass (#it{M}_{#odot})");  // room for the legend
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the chirp of the last tenth of a second before the merger: detector data and the template found in it
    Plot plot("gwStrain", "neutronStars/merger", "1d_wide");
    plot[1].AddData("merger/strain", "neutronstars", "data (whitened)").SetDrawOption(line).SetLine(kSolid, 1.0, kGray + 1);
    plot[1].AddData("merger/template", "neutronstars", "template, #it{M}_{c} = 1.19 #it{M}_{#odot}").SetDrawOption(line).SetLine(kSolid, 2.0, kRed + 1);
    plot[1].AddLegend(0.15, 0.92);
    plot[1]['X'].SetRange(-0.1, 0.01).SetTitle("time from merger (s)");
    plot[1]['Y'].SetRange(-2.5, 3.5).SetTitle("strain (a.u.)");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the same as a time-frequency map: the frequency sweeps up to the merger
    Plot plot("gwSpectrogram", "neutronStars/merger", "2d");
    plot[1].AddData("merger/spectrogram", "neutronstars").SetDrawOption(colz);
    plot[1].SetPalette(kViridis);
    plot[1]['Y'].SetLog();
    plot[1]['Z'].SetRange(0.0, 25.0);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the kilonova: the light of the radioactive ejecta, a fast blue and a slow red part
    Plot plot("kilonova", "neutronStars/merger", "1d");
    plot[1].AddData("merger/bolometric", "neutronstars", "bolometric").SetMarker(kFullCircle, 1.2, kBlack).SetLineColor(kBlack);
    plot[1].AddData("merger/model_blue", "neutronstars", "lanthanide-poor ejecta").SetLine(kSolid, 2.0, kAzure + 2);
    plot[1].AddData("merger/model_red", "neutronstars", "lanthanide-rich ejecta").SetLine(kSolid, 2.0, kRed + 1);
    plot[1].AddData("merger/model_total", "neutronstars", "sum").SetLine(kSolid, 3.0, kGray + 2);
    plot[1].AddLegend(0.45, 0.92);
    plot[1]['X'].SetLog().SetRange(0.2, 20.0).SetTitle("time after merger (days)");
    plot[1]['Y'].SetLog().SetRange(0.1, 1000.0).SetTitle("luminosity (10^{40} erg/s)");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
}

//**************************************************************************************************
// Biophysics: protein structure, single-molecule mechanics and spectroscopy, enzyme kinetics, ion channels
//**************************************************************************************************
void DefineBiophysicsPlots(PlotManager& pm)
{
  {  // the Ramachandran plot: where the backbone angles of proteins cluster
    Plot plot("ramachandran", "biophysics", "2d_logz");
    plot[1].AddData("dihedrals", "biophysics").Project2D({"phi", 120, {-180.0, 180.0}}, {"psi", 120, {-180.0, 180.0}});
    plot[1].AddText(-95.0, -15.0, "#alpha_{R}").SetUserCoordinates().SetTextColor(kWhite);
    plot[1].AddText(-150.0, 175.0, "#beta").SetUserCoordinates().SetTextColor(kWhite);
    plot[1].AddText(80.0, 30.0, "#alpha_{L}").SetUserCoordinates();
    plot[1].SetPalette(kViridis);
    plot[1]['X'].SetTitle("#varphi (deg)");
    plot[1]['Y'].SetTitle("#psi (deg)");
    plot[1]['Z'].SetTitle("residues");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // stretching a DNA molecule: entropic elasticity, then the overstretching plateau at 65 pN
    Plot plot("dnaStretching", "biophysics", "1d");
    plot[1].AddData("forceExtension", "biophysics", "#lambda-DNA, optical tweezers").SetColor(kBlack);
    plot[1].AddData("wormLikeChain", "biophysics", "worm-like chain, #it{P} = 50 nm").SetDrawOption(line).SetLine(kSolid, 2.0, kRed + 1);
    plot[1].AddPoints({16.4, 16.4}, {0.0, 100.0}, "contour length").SetDrawOption(line).SetLine(kDashed, 2.0, kGray + 2);
    plot[1].AddLegend(0.2, 0.92);
    plot[1]['X'].SetRange(0.0, 32.0).SetTitle("extension (#mum)");
    plot[1]['Y'].SetRange(0.0, 120.0).SetTitle("force (pN)");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // single-molecule FRET: folded and unfolded molecules as two peaks, the fractions from the fit
    Plot plot("fretEfficiency", "biophysics", "1d");
    plot[1].AddData("fretEfficiency", "biophysics", "molecules").SetDrawOption(hist).SetLine(kSolid, 2.0, kGray + 2);
    // the two states of the fit as filled areas under the curve of the whole fit
    plot[1].AddData("fretUnfolded", "biophysics", "unfolded, <title>").SetDrawOption(area).SetFill(1001, kAzure + 2, 0.35).SetLineColor(kAzure + 2).SetLegendEntry(LegendEntry().SetSymbolElements("F"));
    plot[1].AddData("fretFolded", "biophysics", "folded, <title>").SetDrawOption(area).SetFill(1001, kRed + 1, 0.35).SetLineColor(kRed + 1).SetLegendEntry(LegendEntry().SetSymbolElements("F"));
    plot[1].AddData("fretFit", "biophysics", "fit, with donor-only peak").SetLine(kSolid, 2.0, kBlack);
    plot[1].AddLegend(0.2, 0.92);
    plot[1]['Y'].SetRange(0.0, 1300.0);  // room for the legend
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // enzyme kinetics: a competitive inhibitor raises K_M and leaves the maximal rate
    Plot plot("michaelisMenten", "biophysics", "1d");
    for (const auto& [name, label, color] : vector<std::tuple<string, string, int>>{{"noInhibitor", "without inhibitor", kAzure + 2}, {"inhibitor", "with inhibitor", kRed + 1}}) {
      plot[1].AddData("rate_" + name, "biophysics", label).SetColor(color);
      plot[1].AddData("fit_" + name, "biophysics", "<title>").SetLine(kSolid, 2.0, color);
    }
    plot[1].AddLegend(0.2, 0.92);
    plot[1]['X'].SetLog().SetRange(0.5, 1000.0).SetTitle("substrate concentration (#muM)");
    plot[1]['Y'].SetRange(0.0, 15.0).SetTitle("initial rate (#muM/s)");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // a single ion channel opening and closing
    Plot plot("ionChannel", "biophysics", "1d_wide");
    plot[1].AddData("current", "biophysics").SetDrawOption(line).SetLine(kSolid, 1.0, kAzure + 2);
    plot[1].AddText(0.15, 0.3, "open");
    plot[1].AddText(0.15, 0.85, "closed");
    plot[1]['X'].SetRange(0.0, 300.0).SetTitle("time (ms)");
    plot[1]['Y'].SetRange(-7.5, 2.5).SetTitle("current (pA)");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // its open times in logarithmic bins: one exponential is one peak at its time constant
    Plot plot("dwellTimes", "biophysics", "1d");
    plot[1].AddData("openDwellTimes", "biophysics", "open times").SetColor(kAzure + 2);
    plot[1].AddData("fit_openDwellTimes", "biophysics", "<title>").SetLine(kSolid, 2.0, kRed + 1);
    plot[1].AddLegend(0.2, 0.92);
    plot[1]['X'].SetLog();
    plot[1]['Y'].SetRange(0.0, 400.0);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
}

//**************************************************************************************************
// Electronics: threshold scans and tuning of a pixel chip, pulse shapes and frequency response of its front end, eye diagrams of its data link
//**************************************************************************************************
void DefineElectronicsPlots(PlotManager& pm)
{
  {  // threshold scans: the hit efficiency while the injected charge rises, one colour per pixel for the points and their erf fit
    Plot plot("sCurves", "electronics/pixelChip", "1d");
    for (const auto& [pixel, color] : vector<std::pair<string, int>>{{"A", kAzure + 2}, {"B", kBlack}, {"C", kRed + 1}}) {
      plot[1].AddData("pixelChip/scan_" + pixel, "electronics", "pixel " + pixel + ": <title>").SetColor(color);
      plot[1].AddData("pixelChip/fit_" + pixel, "electronics").SetLine(kSolid, 2.0, color);  // no label: no legend row
    }
    plot[1].AddLegend(0.2, 0.92, "threshold and noise, 50 injections per charge");
    plot[1]['X'].SetRange(0.0, 2000.0).SetNumDivisions(505).SetTitle("injected charge (e^{#minus})");
    plot[1]['Y'].SetRange(0.0, 1.45).SetTitle("hit efficiency");  // room for the legend above the plateau
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the thresholds of all pixels before and after tuning the trim DAC of each pixel; masked pixels left out
    Plot plot("thresholdTuning", "electronics/pixelChip", "1d");
    plot[1].AddData("pixelChip/pixels", "electronics", "before tuning").Filter("!masked").Project1D({"thresholdUntuned", 140, {400.0, 1800.0}}).SetDrawOption(hist).SetFill(1001, kGray).SetLineColor(kGray + 1);
    plot[1].AddData("pixelChip/pixels", "electronics", "after tuning").Filter("!masked").Project1D({"threshold", 140, {400.0, 1800.0}}).SetDrawOption(hist).SetLine(kSolid, 2.0, kAzure + 2);
    plot[1].AddData("pixelChip/fitTuned", "electronics", "<title>").SetLine(kDashed, 2.0, kRed + 1);
    plot[1].AddVerticalLine(1000.0).SetLine(kDotted, 2.0, kGray + 2).SetBehindData();
    plot[1].AddText(0.2, 0.92, {"128 #times 64 pixels", "target 1000 e^{#minus}"});
    plot[1].AddLegend(0.58, 0.92);
    plot[1]['X'].SetTitle("threshold (e^{#minus})");
    plot[1]['Y'].SetLog().SetRange(0.5, 2.0e4).SetTitle("pixels per 10 e^{#minus}");  // room for the legend above the peak
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the same as maps of the matrix sharing one colour scale: a weight puts the threshold into the cell of its pixel; the
    // gradient over the rows (the supply voltage drops with the distance from the periphery) is tuned away
    Plot plot("thresholdMaps", "electronics/pixelChip", "2d_2x1");
    uint8_t pad = 1;
    for (const auto& [column, label] : vector<std::pair<string, string>>{{"thresholdUntuned", "before tuning"}, {"threshold", "after tuning"}}) {
      plot[pad].AddData("pixelChip/pixels", "electronics").Filter("!masked").Project2D({"col", 128, {-0.5, 127.5}}, {"row", 64, {-0.5, 63.5}}, column);
      plot[pad].AddText(0.15, 0.9, label).SetTextColor(kWhite);
      ++pad;
    }
    plot[2].AddText(86.0, 8.0, {"masked", "double column"}).SetUserCoordinates().SetTextColor(kWhite);
    plot[2].AddArrow({100.0, 12.0}, {89.0, 24.0}).SetLine(kSolid, 2.0, kWhite).SetFill(1001, 10).SetArrowSize(0.02).SetOnTop();  // 10: white (0 leaves the head open)
    plot[0].SetPalette(kViridis);
    plot[0]['X'].SetTitle("column");
    plot[0]['Y'].SetTitle("row");
    plot[0]['Z'].SetRange(600.0, 1500.0).SetNoExponent().SetTitle("threshold (e^{#minus})");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the shaper output for three charges: the larger pulse crosses the threshold earlier (time walk) and stays above it
    // longer (time over threshold); arrows and lines are shapes: no data, no legend rows
    Plot plot("pulseShapes", "electronics/frontEnd", "1d");
    for (const auto& [charge, color] : vector<std::pair<int, int>>{{2, kAzure + 2}, {5, kGreen + 2}, {10, kRed + 1}}) {
      plot[1].AddData(Form("frontEnd/pulse_%dke", charge), "electronics", Form("%d000 e^{#minus}", charge)).SetDrawOption(line).SetLine(kSolid, 2.0, color);
    }
    plot[1].AddHorizontalLine(25.0).SetLine(kDashed, 2.0, kGray + 2).SetBehindData();
    plot[1].AddText(172.0, 52.0, "threshold").SetUserCoordinates();
    // where the smallest and the largest pulse cross the threshold
    plot[1].AddLine({19.0, 25.0}, {19.0, 60.0}).SetLine(kDotted, 2.0, kAzure + 2);
    plot[1].AddLine({6.6, 60.0}, {6.6, -30.0}).SetLine(kDotted, 2.0, kRed + 1);
    plot[1].AddLine({168.2, 25.0}, {168.2, -30.0}).SetLine(kDotted, 2.0, kRed + 1);
    plot[1].AddArrow({6.6, 55.0}, {19.0, 55.0}).SetArrowHead("<|>").SetLine(kSolid, 2.0, kBlack).SetFill(1001, kBlack).SetArrowSize(0.012);
    plot[1].AddText(22.0, 70.0, "time walk").SetUserCoordinates();
    plot[1].AddArrow({6.6, -20.0}, {168.2, -20.0}).SetArrowHead("<|>").SetLine(kSolid, 2.0, kRed + 1).SetFill(1001, kRed + 1).SetArrowSize(0.012);
    plot[1].AddText(50.0, -28.0, "time over threshold").SetUserCoordinates();
    plot[1].AddLegend(0.6, 0.92, "injected charge");
    plot[1]['X'].SetRange(-10.0, 220.0).SetTitle("time (ns)");
    plot[1]['Y'].SetRange(-60.0, 300.0).SetTitle("shaper output (mV)");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // a Bode plot: gain and phase of the shaper at two peaking times, in two panels sharing the logarithmic frequency axis; the
    // measurement leaves the ideal CR-RC^2 response at high frequencies, where the preamplifier has its pole
    Plot plot("bodePlot", "electronics/frontEnd", "1d_wide_1x2");
    for (const auto& [peaking, color] : vector<std::pair<int, int>>{{25, kAzure + 2}, {100, kRed + 1}}) {
      plot[1].AddData(Form("frontEnd/gain_%dns", peaking), "electronics", Form("peaking time %d ns", peaking)).SetMarker(kFullCircle, 0.8, color);
      plot[1].AddData(Form("frontEnd/gainModel_%dns", peaking), "electronics").SetDrawOption(line).SetLine(kSolid, 2.0, color);
      plot[2].AddData(Form("frontEnd/phase_%dns", peaking), "electronics").SetMarker(kFullCircle, 0.8, color);
      plot[2].AddData(Form("frontEnd/phaseModel_%dns", peaking), "electronics").SetDrawOption(line).SetLine(kSolid, 2.0, color);
    }
    plot[1].AddHorizontalLine(-3.0).SetLine(kDashed, 1.0, kGray + 2).SetBehindData();
    plot[1].AddLegend(0.33, 0.33, "points: measured, lines: ideal CR-RC^{2}");
    plot[1]['Y'].SetRange(-95.0, 10.0).SetTitle("gain (dB)");
    plot[2]['Y'].SetRange(-290.0, 120.0).SetNumDivisions(505).SetTitle("phase (deg)");
    plot[0]['X'].SetLog().SetRange(0.01, 1000.0).SetTitle("frequency (MHz)");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // an eye diagram: the waveform of a serial link cut into pieces of two bits, laid on top of each other; the receiver needs
    // the eye open around the mask (a polygon in the units of the axes)
    Plot plot("eyeDiagram", "electronics/link", "2d_logz");
    plot[1].AddData("link/eye_3m", "electronics").SetDrawOption(colz);
    plot[1].AddPolygon({{0.3, 0.0}, {0.4, 80.0}, {0.6, 80.0}, {0.7, 0.0}, {0.6, -80.0}, {0.4, -80.0}}).SetFill(3354, kRed - 4).SetLine(kSolid, 3.0, kRed - 4);
    plot[1].AddText(0.15, 0.92, "2.56 Gb/s, PRBS7, 3 m cable").SetTextColor(kWhite);
    plot[1].SetFrameFill(1001, kBlack);  // dark like the screen of an oscilloscope
    plot[1].SetPalette(kBird);
    plot[1]['X'].SetNumDivisions(505);
    plot[1]['Z'].SetRange(1.0, 2.0e3);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the radiation pattern of an array of four patch antennas, fed in phase and steered by 30 deg with phase steps: polar graphs
    // drawn on a polar grid in place of the axes (x the angle, the front of the array up, y the field strength)
    Plot plot("antennaPattern", "electronics/antenna", "1d");
    for (const auto& [name, label, color] : vector<std::tuple<string, string, int>>{{"broadside", "in phase", kAzure + 2}, {"steered30", "steered by 30#circ", kRed + 1}}) {
      plot[1].AddData("antenna/" + name + "_model", "electronics").SetDrawOption(line).SetLine(kSolid, 2.0, color);
      plot[1].AddData("antenna/" + name, "electronics", label).SetDrawOption(points).SetMarker(kFullCircle, 0.7, color).SetLineColor(color).SetDefinesFrame();
    }
    plot[1].AddLegend(0.02, 0.985).SetDefaultMarkerSize(1.2);  // in the corner above the grid, which fills the pad
    plot[1].AddText(bottom_left, {"4 patches, #lambda/2 apart", "relative field strength"});
    plot[1]['Y'].SetRange(0.0, 1.1);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
}

//**************************************************************************************************
// Theory: QCD (running coupling, parton distributions, lattice thermodynamics), chaos, critical phenomena
//**************************************************************************************************
void DefineTheoryPlots(PlotManager& pm)
{
  {  // asymptotic freedom: the strong coupling falls with the scale
    Plot plot("runningCoupling", "theory", "1d");
    plot[1].AddData("alphaS", "theory", "2-loop QCD, #alpha_{s}(#it{m}_{Z}) = 0.118").SetDrawOption(band).SetFill(1001, kOrange - 9).SetLineColor(kOrange + 1).SetLegendEntry(LegendEntry().SetSymbolElements("F"));
    plot[1].AddData("alphaS_measurements", "theory", "measurements").SetColor(kBlack);
    plot[1].AddLegend(0.3, 0.92);
    plot[1]['X'].SetLog().SetRange(1.0, 3000.0).SetTitle("#it{Q} (GeV)");
    plot[1]['Y'].SetRange(0.05, 0.45).SetTitle("#alpha_{s}(#it{Q})");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // what is inside a proton: parton distributions with their uncertainty bands
    Plot plot("partonDistributions", "theory", "1d");
    for (const auto& [name, label, color] : vector<std::tuple<string, string, int>>{{"uv", "#it{x}#it{u}_{v}", kRed + 1}, {"dv", "#it{x}#it{d}_{v}", kAzure + 2}, {"g", "#it{x}#it{g} / 10", kGreen + 2}, {"sea", "#it{x}#it{S} / 10", kViolet + 1}}) {
      plot[1].AddData("pdf_" + name, "theory", label).SetDrawOption(band).SetFill(1001, color, 0.4).SetLineColor(color).SetLegendEntry(LegendEntry().SetSymbolElements("F"));
    }
    plot[1].AddLegend(0.6, 0.92, "#it{Q}^{2} = 10 GeV^{2}");
    plot[1]['X'].SetLog().SetRange(1e-4, 1.0).SetTitle("#it{x}");
    plot[1]['Y'].SetRange(0.0, 1.2).SetTitle("#it{x}#it{f}(#it{x}, #it{Q}^{2})");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // QCD matter from the lattice: the crossover from hadrons to quarks and gluons
    Plot plot("latticeQCD", "theory", "1d");
    plot[1].AddData("crossover", "theory", "#it{T}_{c}").SetDrawOption(boxes_nomarkers).SetFill(1001, kGray).SetLineColor(kGray).SetLegendEntry(LegendEntry().SetSymbolElements("F"));
    for (const auto& [name, label, color] : vector<std::tuple<string, string, int>>{{"energy", "#varepsilon/#it{T}^{4}", kRed + 1}, {"entropy", "3#it{s}/4#it{T}^{3}", kAzure + 2}, {"3p", "3#it{p}/#it{T}^{4}", kGreen + 2}}) {
      plot[1].AddData("lattice_" + name, "theory", label).SetDrawOption(band).SetFill(1001, color, 0.6).SetLineColor(color).SetLegendEntry(LegendEntry().SetSymbolElements("F"));
    }
    plot[1].AddPoints({130.0, 410.0}, {15.62, 15.62}, "Stefan-Boltzmann limit").SetDrawOption(line).SetLine(kDashed, 2.0, kBlack);
    plot[1].AddLegend(0.5, 0.45);
    plot[1]['X'].SetRange(130.0, 410.0).SetTitle("#it{T} (MeV)");
    plot[1]['Y'].SetRange(0.0, 17.0);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the logistic map: period doubling into chaos, with windows of order
    Plot plot("bifurcation", "theory", "2d_logz");
    plot[1].AddData("bifurcation", "theory").SetDrawOption(col);
    plot[1].SetPalette(kInvertedDarkBodyRadiator);  // light where rarely visited, dark on the stable orbits
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // its Lyapunov exponent: positive where nearby orbits separate
    Plot plot("lyapunov", "theory", "1d_wide");
    plot[1].AddData("lyapunov", "theory").SetDrawOption(line).SetLine(kSolid, 1.0, kAzure + 2);
    plot[1].SetRefFunc("0").SetLine(kDashed, 2.0, kGray + 1);
    plot[1]['X'].SetRange(2.8, 4.0).SetTitle("#it{r}");
    plot[1]['Y'].SetRange(-2.5, 1.0).SetTitle("Lyapunov exponent #lambda");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the Mandelbrot set, black inside: around it the number of iterations until the orbit escapes, on a log scale
    Plot plot("mandelbrot", "theory", "2d_logz");
    plot[1].AddData("mandelbrot", "theory").SetContours(99);
    plot[1].SetFrameFill(1001, kBlack);  // the empty cells (the set itself) show the frame's fill
    plot[1].SetPalette(kSunset);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  const vector<int> sizes = {8, 16, 32, 64};
  {  // the 2D Ising model: Binder cumulants of all lattice sizes cross at the critical temperature
    Plot plot("binderCumulant", "theory", "1d");
    plot[0].SetDefaultColors({{0.1, 0.3, 0.8, 0.0}, {0.8, 0.1, 0.1, 1.0}}, {}, sizes.size());
    for (const int size : sizes) {
      plot[1].AddData("binder_L" + std::to_string(size), "theory", "#it{L} = " + std::to_string(size)).SetDrawOption(points_line);
    }
    plot[1].AddPoints({2.269, 2.269}, {0.0, 0.7}, "#it{T}_{c} (Onsager)").SetDrawOption(line).SetLine(kDashed, 2.0, kGray + 2);
    plot[1].AddLegend(0.18, 0.5);
    plot[1]['X'].SetTitle("#it{T} (#it{J}/#it{k}_{B})");
    plot[1]['Y'].SetRange(0.0, 0.9).SetTitle("#it{U}_{4} = 1 #minus #LT#it{m}^{4}#GT / 3#LT#it{m}^{2}#GT^{2}");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the magnetisation of finite lattices approaches the exact solution
    Plot plot("isingMagnetisation", "theory", "1d");
    plot[0].SetDefaultColors({{0.1, 0.3, 0.8, 0.0}, {0.8, 0.1, 0.1, 1.0}}, {}, sizes.size() + 1);
    for (const int size : sizes) {
      plot[1].AddData("magnetisation_L" + std::to_string(size), "theory", "#it{L} = " + std::to_string(size));
    }
    plot[1].AddData("onsager", "theory", "Onsager, #it{L} #rightarrow #infty").SetLine(kSolid, 3.0, kBlack);
    plot[1].AddLegend(0.6, 0.92);
    plot[1]['X'].SetRange(1.5, 3.5).SetTitle("#it{T} (#it{J}/#it{k}_{B})");
    plot[1]['Y'].SetRange(0.0, 1.2).SetTitle("#LT|#it{m}|#GT");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
}

//**************************************************************************************************
// Beyond physics: tables (CSV) from climate science, astronomy and neuroscience
//**************************************************************************************************
void DefineOtherSciencePlots(PlotManager& pm)
{
  {  // monthly CO2 with its seasonal cycle, and the trend without it
    Plot plot("co2", "climate", "1d_wide");
    plot[1].AddData("co2", "co2", "monthly mean").Scatter("year", "co2").SetDrawOption(line).SetLine(kSolid, 2.0, kAzure + 2);
    plot[1].AddData("co2", "co2", "seasonal cycle removed").Scatter("year", "deseasonalized").SetDrawOption(line).SetLine(kSolid, 3.0, kBlack);
    plot[1].AddLegend(0.15, 0.85);
    plot[1]['X'].SetRange(1960.0, 2030.0).SetTitle("year");
    plot[1]['Y'].SetTitle("CO_{2} (ppm)");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // yearly global temperature anomalies as bars, warm years red and cold years blue: Filter selects the rows of a table
    Plot plot("temperatureAnomaly", "climate", "1d_wide");
    const Data::data_dim_t years{"year", 146, {1879.5, 2025.5}};
    plot[1].AddData("temperature", "temperature").Filter("anomaly >= 0").Project1D(years, "anomaly").SetDrawOption(bars).SetFill(1001, kRed - 4).SetLineColor(kRed - 4);
    plot[1].AddData("temperature", "temperature").Filter("anomaly < 0").Project1D(years, "anomaly").SetDrawOption(bars).SetFill(1001, kAzure - 4).SetLineColor(kAzure - 4);
    plot[1].AddData("temperature", "temperature", "11-year mean").Scatter("year", "smoothed").SetDrawOption(line).SetLine(kSolid, 3.0, kBlack);
    plot[1].AddLegend(0.15, 0.85);
    plot[1]['X'].SetRange(1879.5, 2025.5).SetTitle("year");
    plot[1]['Y'].SetRange(-0.7, 1.6).SetTitle("anomaly to 1951-1980 (K)");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the anomaly by latitude and year from a table with one row per cell: the weight of Project2D puts it into the cell
    Plot plot("warmingByLatitude", "climate", "2d");
    plot[1].AddData("warming", "warming").Project2D({"year", 76, {1949.5, 2025.5}}, {"latitude", 36, {-90.0, 90.0}}, "anomaly");
    plot[1].SetPalette(kTemperatureMap);
    plot[1]['X'].SetTitle("year");
    plot[1]['Y'].SetTitle("latitude (deg)").SetNumDivisions(506);
    plot[1]['Z'].SetRange(-3.0, 3.0).SetTitle("anomaly to 1960-1970 (K)");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  // anomalies on a diverging scale: blue below, white at zero, red above (a gradient of rgb colours and positions on the scale)
  const vector<std::pair<color_t, float_t>> anomalyColors = {{"#2166ac", 0.0}, {"#67a9cf", 0.3}, {"#f7f7f7", 0.5}, {"#ef8a62", 0.7}, {"#b2182b", 0.85}, {"#67001f", 1.0}};
  {  // the warming of the last decade on a world map: a 2d histogram in degrees drawn with ROOT's Mollweide projection (equal area),
    // Z adds the colour scale; the axes would show the projected coordinates, which are not longitude and latitude inside the map
    Plot plot("warmingMap", "climate", "2d_map");
    plot[1].AddData("warmingMap", "climate").SetDrawOption("MOLLWEIDE Z");
    plot[1].SetPalette(anomalyColors, {}, 60);
    plot[1].AddText(0.03, 0.95, "2015-2024");  // in the corner beside the map
    // no axes: without ticks and in white, as ROOT draws transparent lines opaque in png
    plot[1]['X'].SetTitle("").SetLabelSize(0.0).SetTickLength(0.0).SetAxisColor(kWhite);
    plot[1]['Y'].SetTitle("").SetLabelSize(0.0).SetTickLength(0.0).SetAxisColor(kWhite);
    plot[1]['Z'].SetRange(-3.0, 3.0);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the same map unprojected: drawn as a plain colour map, longitude and latitude are the axes
    Plot plot("warmingMapLonLat", "climate", "2d_lonlat");
    plot[1].AddData("warmingMap", "climate").SetDrawOption(colz);
    plot[1].SetPalette(anomalyColors, {}, 60);
    plot[1]['X'].SetNumDivisions(-6);  // every 60 degrees (negative: exactly these divisions)
    plot[1]['Y'].SetNumDivisions(-6);  // every 30 degrees
    plot[1]['Z'].SetRange(-3.0, 3.0);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the Arctic, where the warming is strongest: a range on the pad cuts the projection to a part of the sphere
    Plot plot("arcticWarming", "climate", "2d_map");
    plot[1].AddData("warmingMap", "climate").SetDrawOption("MOLLWEIDE Z");
    plot[1].SetPalette(anomalyColors, {}, 60);
    plot[1].AddText(0.03, 0.95, "north of 45#circ, 2015-2024");
    plot[1]['X'].SetTitle("").SetLabelSize(0.0).SetTickLength(0.0).SetAxisColor(kWhite);
    plot[1]['Y'].SetTitle("").SetLabelSize(0.0).SetTickLength(0.0).SetAxisColor(kWhite).SetRange(45.0, 90.0);
    plot[1]['Z'].SetRange(-3.0, 3.0);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // regions with their own shapes (countries, districts, detector cells) are a TH2Poly: here lightning over Europe on hexagonal cells of land
    Plot plot("lightningEurope", "climate", "2d_europe");
    plot[1].AddData("lightningEurope", "climate").SetDrawOption(colz);
    plot[1]['Z'].SetLog().SetRange(0.05, 10.0);
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // a Hertzsprung-Russell diagram: the density of 15000 stars of a catalogue
    Plot plot("hertzsprungRussell", "astronomy", "2d_logz");
    plot[1].AddData("stars", "stars").Project2D({"bv", 100, {-0.4, 2.0}}, {"logL", 100, {-4.0, 4.0}});
    // texts placed in the units of the axes
    plot[1].AddText(0.1, 3.3, "main sequence").SetUserCoordinates();
    plot[1].AddText(1.1, 3.3, "red giants").SetUserCoordinates();
    plot[1].AddText(0.65, -3.25, "white dwarfs").SetUserCoordinates();
    plot[1]['X'].SetTitle("colour index #it{B}#minus#it{V} (mag)");
    plot[1]['Y'].SetTitle("log_{10}(#it{L} / #it{L}_{#odot})");
    plot[1]['Z'].SetTitle("stars");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the rotation curve of a spiral galaxy: stars and gas alone cannot hold it flat, a dark matter halo does
    Plot plot("rotationCurve", "astronomy", "1d");
    plot[1].AddData("rotation/measured", "astronomy", "measured (H I, H#alpha)").SetMarker(kFullCircle, 1.1, kBlack);
    plot[1].AddData("rotation/total", "astronomy", "total").SetDrawOption(line).SetLine(kSolid, 3.0, kBlack);
    for (const auto& [name, label, color] : vector<std::tuple<string, string, int>>{{"bulge", "bulge", kOrange + 7}, {"disk", "stellar disk", kAzure + 2}, {"halo", "dark matter halo", kGray + 2}}) {
      plot[1].AddData("rotation/" + name, "astronomy", label).SetDrawOption(line).SetLine(kSolid, 2.0, color);
    }
    plot[1].AddLegend(0.2, 0.92).SetNumColumns(2);
    plot[1]['X'].SetRange(0.0, 30.0).SetTitle("radius (kpc)");
    plot[1]['Y'].SetRange(0.0, 300.0).SetTitle("rotation velocity (km/s)");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the temperature power spectrum of the cosmic microwave background: the acoustic peaks of the early universe
    Plot plot("cmbPowerSpectrum", "astronomy", "1d_wide");
    plot[1].AddData("cmb/measured", "astronomy", "measured").SetDrawOption(points_xerr).SetMarkerStyle(kFullCircle).SetMarkerSize(0.8).SetColor(kRed + 1);
    plot[1].AddData("cmb/model", "astronomy", "#LambdaCDM best fit").SetDrawOption(line).SetLine(kSolid, 2.0, kBlack);
    plot[1].AddLegend(0.7, 0.9);
    plot[1]['X'].SetRange(0.0, 2500.0).SetTitle("multipole #it{l}");
    plot[1]['Y'].SetRange(0.0, 6900.0).SetNoExponent().SetTitle("#it{l}(#it{l}+1)#it{C}_{#it{l}} / 2#pi (#muK^{2})");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the gamma-ray sky in galactic coordinates as all-sky maps show it: an Aitoff projection with the centre of the Milky Way in
    // the middle, the disk along the equator, the Fermi bubbles above and below the centre
    Plot plot("gammaRaySky", "astronomy", "2d_map");
    plot[1].AddData("sky/gammaRays", "astronomy").SetDrawOption("AITOFF Z");
    plot[1].AddText(0.03, 0.95, "#it{E} > 1 GeV");
    // no axes: without ticks and in white, as ROOT draws transparent lines opaque in png
    plot[1]['X'].SetTitle("").SetLabelSize(0.0).SetTickLength(0.0).SetAxisColor(kWhite);
    plot[1]['Y'].SetTitle("").SetLabelSize(0.0).SetTickLength(0.0).SetAxisColor(kWhite);
    plot[1]['Z'].SetLog();
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // a spike raster of 40 trials and the firing rate below it: the neuron answers the stimulus at 0 ms
    Plot plot("spikeRaster", "neuroscience", "1d_wide_1x2");
    plot[1].AddData("spikes", "spikes").Scatter("time", "trial").SetDrawOption(points).SetMarker(kFullDotLarge, 1.0, kBlack);
    plot[1]['Y'].SetRange(0.0, 41.0).SetTitle("trial");
    // 40 trials and 50 ms bins: the counts times 1 / (40 x 0.05 s) are spikes per second
    plot[2].AddData("spikes", "spikes").Project1D({"time", 30, {-500.0, 1000.0}}).Scale(0.5).SetDrawOption(hist).SetFill(1001, kAzure - 9).SetLineColor(kAzure + 2);
    plot[2]['Y'].SetTitle("rate (spikes/s)");
    plot[0]['X'].SetRange(-500.0, 1000.0).SetTitle("time after stimulus (ms)");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // a train of action potentials in the Hodgkin-Huxley model while a current is injected, the current below
    Plot plot("actionPotentials", "neuroscience", "1d_wide_1x2");
    plot[1].AddData("membrane", "membrane").Scatter("time", "voltage").SetDrawOption(line).SetLine(kSolid, 2.0, kAzure + 2);
    plot[1]['Y'].SetRange(-85.0, 55.0).SetTitle("membrane voltage (mV)");
    plot[2].AddData("membrane", "membrane").Scatter("time", "current").SetDrawOption(line).SetLine(kSolid, 2.0, kRed + 1);
    plot[2]['Y'].SetRange(-2.0, 13.0).SetTitle("#it{I} (#muA/cm^{2})");
    plot[0]['X'].SetRange(0.0, 100.0).SetTitle("time (ms)");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
  {  // the orientation tuning curve of a visual cortex neuron, with the von Mises function it was drawn from
    Plot plot("orientationTuning", "neuroscience", "1d");
    plot[1].AddData("tuning", "tuning", "20 trials per orientation").Scatter("orientation", "rate", "0", "rate_err").SetMarker(kFullCircle, 1.2, kBlack);
    plot[1].AddFunction("2 + 38*exp(2.5*(cos(2*(x - 60)*TMath::DegToRad()) - 1))", "von Mises, peak at 60#circ").SetRangeX(0.0, 180.0).SetLine(kSolid, 2.0, kRed + 1);
    plot[1].AddLegend(0.45, 0.9);
    plot[1]['X'].SetRange(0.0, 180.0).SetNumDivisions(504).SetTitle("grating orientation (deg)");
    plot[1]['Y'].SetRange(0.0, 55.0).SetTitle("firing rate (spikes/s)");
    pm.AddPlot(plot);
  }  // -----------------------------------------------------------------------------------
}

//**************************************************************************************************
// Animations: one plot per frame, in a group of its own; mode gif writes all plots it is given as one animated gif
//**************************************************************************************************
void DefineAnimations(PlotManager& pm)
{
  {  // a camera flight around a lego plot: the same plot seen from 36 angles ---------------
    // plot animations/legoRotation '.+' gif+8++ makes them one gif, 8 centiseconds per frame, looping endlessly
    // the axes change sides while the view turns, so the base plot leaves the same room on both sides
    auto view3d = PlotManager::MakeBasePlot("1d", PanelLayout().SetMargins(60.0, 150.0, 190.0, 190.0));
    view3d.SetName("1d_3dview");
    pm.AddBasePlot(view3d);
    for (int phi = 5; phi < 360; phi += 10) {  // never looking exactly along an axis, where its labels would pile up
      Plot plot("phi" + std::to_string(phi), "animations/legoRotation", "1d_3dview");
      plot[1].AddData("ptVsMult", "data").RebinXY(4, 2).SetDrawOption(lego_col).SetLineWidth(1.0);
      plot[1].SetView(35.0, phi);
      plot[1]['X'].SetTitleOffset(1.8);
      plot[1]['Y'].SetTitleOffset(1.8).SetNumDivisions(505);
      plot[1]['Z'].SetLog().SetTitleOffset(1.5);
      pm.AddPlot(plot);
    }
  }  // -----------------------------------------------------------------------------------
  {  // a TPC read out continuously, in end view and side view: tracks appear with their collision and disappear as their
    // electrons drift to the endcaps (2.58 cm/us, the drifted position from a Define); Profile2D colours the cells by the mean age
    // of their clusters, so that each collision has its own colour
    // plot animations/tpcDrift '.+' gif+20++ makes them one gif, 0.2 s per frame (10 us of drift), looping endlessly
    for (int time = 0; time < 200; time += 10) {
      Plot plot(Form("t%03dus", time), "animations/tpcDrift", "2d_2x1");
      uint8_t pad = 1;
      for (const auto& [horizontal, title] : vector<std::pair<string, string>>{{"x", "#it{x} (cm)"}, {"zNow", "#it{z} (cm)"}}) {
        plot[pad].AddData("clusters", "tpc").Define("age", Form("%d - t0", time)).Define("zNow", "z + (z > 0 ? 2.58 : -2.58) * age").Filter("age >= 0 && abs(zNow) < 250").Profile2D({horizontal, 250, {-250.0, 250.0}}, {"y", 250, {-250.0, 250.0}}, "age");
        plot[pad].SetFrameFill(1001, kBlack);
        plot[pad]['X'].SetTitle(title);
        ++pad;
      }
      for (const double r : {84.85, 246.4}) {  // the readout chambers: 18 sectors of 20 deg between these radii
        plot[1].AddEllipse({0.0, 0.0}, r, r).SetLine(kSolid, 1.0, kGray + 1).SetFillStyle(0).SetBehindData();
      }
      for (int sector = 0; sector < 18; ++sector) {
        const double angle = sector * 20.0 * TMath::DegToRad();
        plot[1].AddLine({84.85 * std::cos(angle), 84.85 * std::sin(angle)}, {246.4 * std::cos(angle), 246.4 * std::sin(angle)}).SetLine(kSolid, 1.0, kGray + 1).SetBehindData();
      }
      plot[2].AddVerticalLine(0.0).SetLine(kDashed, 1.0, kGray + 1).SetBehindData();  // the central electrode: the drift starts here
      plot[1].AddText(0.15, 0.92, "end view").SetTextColor(kWhite);
      plot[2].AddText(0.15, 0.92, {"side view", Form("#it{t} = %d #mus", time)}).SetTextColor(kWhite);
      plot[0].SetPalette(kRainBow);
      plot[0]['Y'].SetTitle("#it{y} (cm)");
      plot[0]['Z'].SetRange(0.0, 97.0).SetTitle("time since the collision (#mus)");  // fixed: the frames differ only in the data
      pm.AddPlot(plot);
    }
  }  // -----------------------------------------------------------------------------------
  {  // the same in one sector: sector 0 (0 to 20 deg, z > 0) in its own coordinates, across the sector and along the radius,
    // binned in the pad rows of its chambers (variable bin edges, the gaps between the chambers empty) and in 6 mm across;
    // the inner chamber and the three stacks of the outer chamber are black polygons behind the data, the rest of the frame stays clear
    // plot animations/tpcSector '.+' gif+20++ makes them one gif, 0.2 s per frame (10 us of drift), looping endlessly
    pm.AddBasePlot(PlotManager::MakeBasePlot("2d_tall"));
    const double tan10 = std::tan(10.0 * TMath::DegToRad());                                                                             // half the opening angle of a sector
    const vector<std::tuple<double, int, double>> chambers = {{84.85, 63, 0.75}, {134.7, 34, 1.0}, {170.8, 30, 1.2}, {208.9, 25, 1.5}};  // first radius (cm), pad rows, row length (cm)
    vector<double> rowEdges;
    for (const auto& [start, nRows, length] : chambers) {
      for (int row = 0; row <= nRows; ++row) {
        rowEdges.push_back(start + row * length);
      }
    }
    for (int time = 0; time < 200; time += 10) {
      Plot plot(Form("t%03dus", time), "animations/tpcSector", "2d_tall");
      auto& clusters = plot[1].AddData("clusters", "tpc").Define("age", Form("%d - t0", time)).Define("zNow", "z + 2.58 * age").Filter("age >= 0 && z > 0 && zNow < 250");
      clusters.Define("phi", "atan2(y, x)").Filter("phi >= 0 && phi < 20 * TMath::DegToRad()");  // in the sector
      clusters.Define("radial", "hypot(x, y) * cos(phi - 10 * TMath::DegToRad())").Define("across", "hypot(x, y) * sin(phi - 10 * TMath::DegToRad())");
      clusters.Profile2D({"across", 150, {-45.0, 45.0}}, {"radial", rowEdges}, "age");
      for (const auto& [start, nRows, length] : chambers) {
        const double low = start, high = start + nRows * length;
        plot[1].AddPolygon({{-tan10 * low, low}, {tan10 * low, low}, {tan10 * high, high}, {-tan10 * high, high}}).SetFill(1001, kBlack).SetLineColor(kBlack).SetBehindData();
      }
      plot[1].AddText(0.13, 0.3, {"TPC", "sector 0", Form("#it{t} = %d #mus", time)});
      plot[1].SetPalette(kRainBow);
      plot[1]['X'].SetTitle("across the sector (cm)");
      plot[1]['Y'].SetRange(80.0, 250.0).SetTitle("radius (cm)");
      plot[1]['Z'].SetRange(0.0, 97.0).SetTitle("time since the collision (#mus)");
      pm.AddPlot(plot);
    }
  }  // -----------------------------------------------------------------------------------
  {  // the spectrum class by class, against all classes together: it gets harder with the multiplicity
    // plot animations/multiplicityClasses '.+' gif+60++ makes them one gif, 0.6 s per frame, looping endlessly
    for (int low = 1; low < 60; low += 4) {
      const int high = low + 3;
      Plot plot("nch" + std::to_string(low), "animations/multiplicityClasses", "1d_ratio");
      const string label = std::to_string(low) + " #leq #it{N}_{ch} #leq " + std::to_string(high);
      plot[1].AddData("ptVsMult", "data", label).ProjectY(low, high, true).Normalize().SetColor(kRed + 1);
      plot[1].AddData("ptVsMult", "data", "all #it{N}_{ch}").ProjectY(1, 60, true).Normalize().SetDrawOption(hist_open).SetLine(kSolid, 3.0, kBlack);
      plot[1].AddLegend(0.5, 0.88);
      plot[1].AddText(0.2, 0.3, {kExperiment, kCollisions});
      plot[1]['Y'].SetLog().SetRange(1e-5, 1.0).SetTitle("normalised counts");
      plot[2].AddRatio({"ptVsMult", "data"}, {"ptVsMult"}).Numer().ProjectY(low, high, true).Denom().ProjectY(1, 60, true).Both().Normalize().SetColor(kRed + 1);
      plot[2]['Y'].SetRange(0.0, 4.0).SetTitle("class / all");
      plot[0]['X'].SetRange(0.0, 4.0);  // fixed ranges: the frames differ only in the data
      pm.AddPlot(plot);
    }
  }  // -----------------------------------------------------------------------------------
  {  // the Higgs peak growing out of the background while the data come in: each frame takes the events up to a luminosity
    // plot animations/higgsDiscovery '.+' gif+40++ makes them one gif, 0.4 s per frame, looping endlessly
    for (int lumi = 10; lumi < 150; lumi += 10) {
      Plot plot(Form("lumi%03d", lumi), "animations/higgsDiscovery", "1d");
      plot[1].AddData("diphoton", "timeline", "data").Filter(Form("lumi < %d", lumi)).Project1D({"mass", 60, {100.0, 160.0}}).SetMarker(kFullCircle, 1.0, kBlack);
      const string background = Form("%g * exp(-(x - 100) / 22)", lumi / 140.0 * 5350.0);  // the shape the background was drawn from, per GeV
      plot[1].AddFunction(background + Form(" + %g * TMath::Gaus(x, 125, 1.7, true)", lumi / 140.0 * 1400.0), "signal + background").SetLine(kSolid, 3.0, kRed + 1);
      plot[1].AddFunction(background, "background").SetLine(kDashed, 3.0, kAzure + 2);
      plot[1].AddText(0.45, 0.9, {kExperiment, Form("#sqrt{#it{s}} = 13.6 TeV, %d fb^{-1}", lumi), "H #rightarrow #gamma#gamma"});
      plot[1].AddLegend(0.45, 0.68);
      plot[1]['X'].SetTitle("#it{m}_{#gamma#gamma} (GeV)");
      plot[1]['Y'].SetRange(0.0, 6500.0).SetTitle("events / GeV");
      pm.AddPlot(plot);
    }
  }  // -----------------------------------------------------------------------------------
  {  // the eye of a serial link closing with the length of the cable: the cable smears each bit into the next ones
    // plot animations/eyeClosing '.+' gif+50++ makes them one gif, 0.5 s per frame, looping endlessly
    for (int length = 1; length <= 10; ++length) {
      Plot plot(Form("cable%02dm", length), "animations/eyeClosing", "2d_logz");
      plot[1].AddData(Form("link/eye_%dm", length), "electronics").SetDrawOption(colz);
      plot[1].AddPolygon({{0.3, 0.0}, {0.4, 80.0}, {0.6, 80.0}, {0.7, 0.0}, {0.6, -80.0}, {0.4, -80.0}}).SetFill(3354, kRed - 4).SetLine(kSolid, 3.0, kRed - 4);
      plot[1].AddText(0.15, 0.92, Form("2.56 Gb/s, PRBS7, %d m cable", length)).SetTextColor(kWhite);
      plot[1].SetFrameFill(1001, kBlack);
      plot[1].SetPalette(kBird);
      plot[1]['X'].SetNumDivisions(505);
      plot[1]['Z'].SetRange(1.0, 2.0e3);  // fixed: the frames differ only in the data
      pm.AddPlot(plot);
    }
  }  // -----------------------------------------------------------------------------------
}
