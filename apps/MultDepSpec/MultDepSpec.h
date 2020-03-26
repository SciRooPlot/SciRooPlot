
#ifndef MultDepSpec_h
#define MultDepSpec_h

#include "PlottingFramework.h"
#include "PlotManager.h"
#include "Plot.h"

namespace MultDepSpec{

const string gConfigFolder = "~/Desktop/PlottingFramework/apps/MultDepSpec/config/";
const string gInputFilesConfig = gConfigFolder + "inputFiles.XML";
const string gPlotDefConfig = gConfigFolder + "plotDefinitions.XML";
const string gDatasetFolder = "~/Desktop/AliMultDepSpec/Datasets/";

using PlottingFramework::PlotManager;
using PlottingFramework::Plot;

// Functions
void DefineTestPlots(PlotManager& plotEnv);
void DefineDatasetPlots(string& dataSet, PlotManager& plotEnv);
void DefineSystemPlots(PlotManager& plotEnv);
void DefineEnergyPlots(PlotManager& plotEnv);
void DefinePublicationPlots(PlotManager& plotEnv);

// Helpers
string GetPtString(int pTbin);


//---- Lable definitions -----------------------------------------------------
const string newLine = " // ";
const string alice = "";
const string aliceWIP = "#bf{ALICE work in progress}";
const string alicePrel = "#bf{ALICE Preliminary}";
const string chargedParticles = "charged particles";
const string erg5TeV_NN = "#sqrt{#it{s}_{NN}} = 5.02 TeV";
const string erg544TeV_NN = "#sqrt{#it{s}_{NN}} = 5.44 TeV";
const string erg2TeV = "#sqrt{#it{s}} = 2.76 TeV";
const string erg5TeV = "#sqrt{#it{s}} = 5.02 TeV";
const string erg7TeV = "#sqrt{#it{s}} = 7 TeV";
const string erg8TeV_NN = "#sqrt{#it{s}_{NN}} = 8 TeV";
const string erg13TeV = "#sqrt{#it{s}} = 13 TeV";
const string eta03 = "|#it{#eta}| < 0.3";
const string eta08 = "|#it{#eta}| < 0.8";
const string pp = "pp";
const string pPb = "p-Pb";
const string PbPb = "Pb-Pb";
const string XeXe = "Xe-Xe";
const string ptRange = "0.15 GeV/#it{c} < #it{p}_{T} < 50 GeV/#it{c}";
const string ptRange10GeV = "0.15 < #it{p}_{T} < 10 GeV/#it{c}";


//---- Colors and Markers ---------------------------------------------------
// Dataset dependent styles definitions
struct pp_5TeV{
  struct input{
    static const string analysis;
    static const string pythia;
    static const string epos_lhc;
    static const string epos;
  };
  struct marker{
    enum : int {
      data = kFullDiamond,
      pythia = kFullDiamond,
      epos_lhc = kFullCross
    };
  };
  struct color{
    enum : int {
      data = kBlue + 1,
      pythia = kBlue + 1,
      epos_lhc = kBlue + 1
      };
  };
  struct range{
    enum : int {
      mult = 60,
      mult_view = 60
    };
  };
};

// default markers
struct marker{
  enum : int {
    circle = kFullCircle,
    square = kFullSquare,
    star = kFullStar,
    circle_open = kOpenCircle,
    square_open = kOpenSquare,
    star_open = kOpenStar
  };
};
// default colors
struct color{
  enum : int {
    red = kRed+1,
    blue = kBlue+1,
    green = kOpenSquare,
    yellow = kOpenSquare,
    magenta = kMagenta+1
  };
};
// default ranges
struct range{
  static const double minPt;
  static const double maxPt;
  static const int minMult;
  static const int maxMult;
};

} // end namespace MultDepSpec
#endif /* MultDepSpec_h */

