
#ifndef MultDepSpec_h
#define MultDepSpec_h

#include "PlottingFramework.h"
#include "PlotManager.h"
#include "Plot.h"

namespace MultDepSpec{
using PlottingFramework::PlotManager;
using PlottingFramework::Plot;

// default location of config and output
const string gConfigFolder = "~/Desktop/PlottingFramework/app/MultDepSpec/config/";
const string gInputFilesConfig = gConfigFolder + "inputFiles.XML";
const string gPlotDefConfig = gConfigFolder + "plotDefinitions.XML";
const string gDatasetFolder = "~/Desktop/AliMultDepSpec/Datasets/";

// main processing functions
void DefinePlotTemplates(PlotManager& plotManager);
void DefineTestPlots(PlotManager& plotManager);
void DefineDatasetPlots(string& dataSet, PlotManager& plotManager);
void DefineSystemPlots(PlotManager& plotManager);
void DefineEnergyPlots(PlotManager& plotManager);
void DefinePublicationPlots(PlotManager& plotManager);

// helper functions
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
const string erg8TeV = "#sqrt{#it{s}} = 8 TeV";
const string erg8TeV_NN = "#sqrt{#it{s}_{NN}} = 8.16 TeV";
const string erg13TeV = "#sqrt{#it{s}} = 13 TeV";
const string eta03 = "|#it{#eta}| < 0.3";
const string eta08 = "|#it{#eta}| < 0.8";
const string pp = "pp";
const string pPb = "p-Pb";
const string PbPb = "Pb-Pb";
const string XeXe = "Xe-Xe";
const string ptRange = "0.15 GeV/#it{c} < #it{p}_{T} < 50 GeV/#it{c}";
const string ptRange10GeV = "0.15 < #it{p}_{T} < 10 GeV/#it{c}";

// usable markers
struct markers{
  enum : int {
    circle = kFullCircle,
    square = kFullSquare,
    cross = kFullCross,
    diamond = kFullDiamond,
    star = kFullStar,

    circle_open = kOpenCircle,
    square_open = kOpenSquare,
    cross_open = kOpenCross,
    diamond_open = kOpenDiamond,
    star_open = kOpenStar
  };
};
// usable colors
struct colors{
  enum : int {
    black = kBlack,

    red = kRed+1,
    green = kGreen+2,
    blue = kBlue+1,
    yellow = kYellow+1,
    magenta = kMagenta+1,

    red_dark = kRed+2,
    green_dark = kGreen+3,
    blue_dark = kBlue+2,
    yellow_dark = kYellow+2,
    magenta_dark = kMagenta+2,
    

    red_bright = kRed,
    blue_bright = kBlue,

    //kTeal-7, kCyan+2, kMagenta-4, kOrange+1, kViolet-3, kPink+3, kOrange+2, kYellow+3, kGray+2
    //kPink+8, 28, kOrange+2 ,8 , kCyan-6, kMagenta+3 ,15, kGreen+4, kMagenta, 17, 12}
  };
};
// default ranges
struct range{
  static const double minPt;
  static const double maxPt;
  static const int minMult;
  static const int maxMult;
};

// put default alice mc as option
enum inputStyleID: int {
  data,
  mc,
  pythia,
  angantyr,
  epos,
  epos_lhc,
  dpmjet,
  hijing,
  pythia6,
  geom_scaling,
};


class dataStyle{
private:
  struct style{
    string input;
    int color;
    int marker;
    int mult;
  };
  map<int, style> mStyles;
  void Add(inputStyleID idx, style myStyle)
  {
    mStyles[idx].input = myStyle.input;
    mStyles[idx].color = myStyle.color;
    mStyles[idx].marker = myStyle.marker;
    mStyles[idx].mult = myStyle.mult;
  };
public:
  dataStyle(vector< pair< inputStyleID, style> > styles)
  {
    for(auto& style : styles) Add(style.first, style.second);
  };
  const style& operator[](inputStyleID idx)
  {
    if(mStyles.find(idx) == mStyles.end()) ERROR("Style was not defined.");
    return mStyles[idx];
  };
};

// now declare styles for the different datasets
extern dataStyle pp_2TeV;
extern dataStyle pp_5TeV;
extern dataStyle pp_7TeV;
extern dataStyle pp_13TeV;
extern dataStyle pPb_5TeV;
extern dataStyle pPb_8TeV;
extern dataStyle XeXe_5TeV;
extern dataStyle PbPb_2TeV;
extern dataStyle PbPb_5TeV;


} // end namespace MultDepSpec
#endif /* MultDepSpec_h */

