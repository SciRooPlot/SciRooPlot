
#ifndef MultDepSpec_h
#define MultDepSpec_h

#include "PlottingFramework.h"
#include "PlotManager.h"
#include "Plot.h"

namespace MultDepSpec{

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

// EPOS, EPOS-LHC, PYTHIA, PYTHIA Angantyr, HERWIG, HIJING++
// data vs mc?
struct marker{
  enum : int {
    // dataset specifics
    pp_2 = kFullDiamond,
    pp_5 = kFullCross,
    pp_7 = kFullCircle,
    pp_13 = kFullSquare,
    pPb_5 = kOpenSquare,
    pPb_8 = kOpenSquare,
    PbPb_2 = kOpenSquare,
    PbPb_5 = kOpenSquare,
    XeXe_5 = kOpenSquare,
    // generic markers
    circle = kFullCircle,
    square = kFullSquare,
    star = kFullStar,
    circle_open = kOpenCircle,
    square_open = kOpenSquare,
    star_open = kOpenStar
  };
};

struct color{
  enum : int {
    // dataset specifics
    pp_2 = kGreen+3,
    pp_5 = kBlue+1,
    pp_7 = kMagenta+1,
    pp_13 = kRed+1,
    pPb_5 = kRed,
    pPb_8 = kRed,
    PbPb_2 = kRed,
    PbPb_5 = kRed,
    XeXe_5 = kRed,
    // generic colors
    red = kRed+1,
    blue = kBlue+1,
    green = kOpenSquare,
    yellow = kOpenSquare,
    magenta = kMagenta+1
  };
};

struct range{
  enum : int {
    pp_2 = 40,
    pp_5 = 60,
    pp_7 = 60,
    pp_13 = 60,
    pPb_5 = 120,
    pPb_8 = 120,
    PbPb_2 = 3700,
    PbPb_5 = 3700,
    XeXe_5 = 2300
   };
};


} // end namespace MultDepSpecPlots


#endif

