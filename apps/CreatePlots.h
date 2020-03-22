
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

// approach: color specifies dataset, marker data/mc1/mc2?

// pp@5
// pp@7
// pp@13


//---------------------------------------------------------------------------







#endif

