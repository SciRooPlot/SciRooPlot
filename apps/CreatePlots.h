
#ifndef CreatePlots_h
#define CreatePlots_h


// helper functions
string GetPtString(int pTbin)
{
  double binsPtDefault[53] = {0.1,0.15,0.2,0.25,0.3,0.35,0.4,0.45,0.5,0.55,0.6,0.65,0.7,0.75,0.8,0.85,0.9,0.95,1.0,1.1,1.2,1.3,1.4,1.5,1.6,1.7,1.8,1.9,2.0,2.2,2.4,2.6,2.8,3.0,3.2,3.4,3.6,3.8,4.0,4.5,5.0,5.5,6.0,6.5,7.0,8.0,9.0,10.0,20.0,30.0,40.0,50.0,60.0};

  double pTlow = binsPtDefault[pTbin-1];
  double pThigh = binsPtDefault[pTbin];

  std::ostringstream out;
  out.precision(2);
  out << std::fixed << pTlow << " GeV/c - " << pThigh << " GeV/c";


  return out.str();
}


//---- Lable definitions -----------------------------------------------------
string newLine = " // ";
string alice = "";
string aliceWIP = "#bf{ALICE work in progress}";
string alicePrel = "#bf{ALICE Preliminary}";
string chargedParticles = "charged particles";
string erg5TeV_NN = "#sqrt{#it{s}_{NN}} = 5.02 TeV";
string erg544TeV_NN = "#sqrt{#it{s}_{NN}} = 5.44 TeV";
string erg2TeV = "#sqrt{#it{s}} = 2.76 TeV";
string erg5TeV = "#sqrt{#it{s}} = 5.02 TeV";
string erg7TeV = "#sqrt{#it{s}} = 7 TeV";
string erg8TeV_NN = "#sqrt{#it{s}_{NN}} = 8 TeV";
string erg13TeV = "#sqrt{#it{s}} = 13 TeV";
string eta03 = "|#it{#eta}| < 0.3";
string eta08 = "|#it{#eta}| < 0.8";
string pp = "pp";
string pPb = "p-Pb";
string PbPb = "Pb-Pb";
string XeXe = "Xe-Xe";
string ptRange = "0.15 GeV/#it{c} < #it{p}_{T} < 50 GeV/#it{c}";
string ptRange10GeV = "0.15 < #it{p}_{T} < 10 GeV/#it{c}";
//---------------------------------------------------------------------------


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


//---------------------------------------------------------------------------



#endif

