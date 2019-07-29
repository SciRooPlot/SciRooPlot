#include "PlottingFramework.h"
#include "Plot.h"
#include <sstream>

using PlottingProject::PlottingFramework;
using PlottingProject::Plot;
using std::cout;
using std::endl;
using std::flush;
using std::string;
using std::vector;

string GetPtString(int pTbin);
// TODO: add feature to also add external histogram in plots
// TODO: extend so multipad plots are possible
// TODO: add scaling feature!!
// TODO: setndivisions from outside
// TODO: auto-change Ranges are not cut

int main(int argc, char *argv[]) {

  cout << "Hello World" << endl;
 // return;
  string outputPath = "../../../../../TempPlots";
  string outputFileName = "myPlots.root";
  bool createBinWiseClosureTests = true;
  bool skipDatasetPlots = false;


  vector<string> dataSets;
  if(argc > 1) {dataSets.push_back(argv[1]); skipDatasetPlots = false;}
  else
  { // define datasets to include in analysis
    dataSets.push_back("pp_5TeV");

    /* --- Unused Datasets ---

    dataSets.push_back("Publications");
    dataSets.push_back("pPb_5TeV");
    dataSets.push_back("PbPb_5TeV");
    dataSets.push_back("pp_5TeV");
    dataSets.push_back("pp_7TeV");
    dataSets.push_back("Simulations");
    dataSets.push_back("Energyscan");
    dataSets.push_back("pp_2TeV");
    dataSets.push_back("pp_13TeV");

    dataSets.push_back("pp_13TeV_old");

    dataSets.push_back("pp_13TeV_withDDC");
    dataSets.push_back("PbPb_5TeV_noZDC");


    dataSets.push_back("pp_5TeV_10GeV_CENT");
    dataSets.push_back("pp_5TeV_10GeV_CENTwoSDD");
    dataSets.push_back("pp_5TeV_10GeV_FAST");
  */
  }

  // create plotting environment
  PlottingFramework plotEnv;
  plotEnv.SetOutputDirectory(outputPath);
  //plotEnv.SetDrawTimestamps(true);
  //plotEnv.SetPalette(kRainBow);//kBlueGreenYellow, kDarkRainBow

  for(string dataSet : dataSets){
    string folder = "../../../../Datasets/" + dataSet + "/";
    string inputFile = folder + dataSet + "_Results.root";
    string inputFileSyst = folder + dataSet + "_Syst.root";

    plotEnv.AddHistosFromInputFile(dataSet, inputFile);
    plotEnv.AddHistosFromInputFile(dataSet, inputFileSyst);
  }


//  plotEnv.ListHistos();
//  plotEnv.LoadPlotsFromConfigFile(plotConfigFile); // loads all available plots for the loaded datasets
//  plotEnv.LoadPlot("myImportantPlot", plotConfigFile);
//  plotEnv.WritePlotsToFile(outputFileName);
//  plotEnv.ListAvailableCanvasStyles();

  //---- Lable definitions -----------------------------------------------------
  string newLine = " // ";
  string alice = "#bf{ALICE work in progress}";
  string alicePrel = "#bf{ALICE Preliminary}";
  string chargedParticles = "charged particles";
  string erg5TeV_NN = "#sqrt{#it{s}_{NN}} = 5.02 TeV";
  string erg2TeV = "#sqrt{#it{s}} = 2.76 TeV";
  string erg5TeV = "#sqrt{#it{s}} = 5.02 TeV";
  string erg7TeV = "#sqrt{#it{s}} = 7 TeV";
  string erg13TeV = "#sqrt{#it{s}} = 13 TeV";
  string eta03 = "|#it{#eta}| < 0.3";
  string eta08 = "|#it{#eta}| < 0.8";
  string pp = "pp";
  string pPb = "p-Pb";
  string PbPb = "Pb-Pb";
  string ptRange = "0.15 GeV/#it{c} < #it{p}_{T} < 50 GeV/#it{c}";
  string ptRange10GeV = "0.15 < #it{p}_{T} < 10 GeV/#it{c}";
  //---------------------------------------------------------------------------


  for(string dataSet : dataSets)
  { //==========================================================================
    if(skipDatasetPlots) break;
    string erg = "0 TeV";
    string colSys = "x-x";
    double multRange = 60;

    if(dataSet.find("pp") != string::npos) colSys = pp;
    if(dataSet.find("pPb") != string::npos) {colSys = pPb; multRange = 150;}
    if(dataSet.find("PbPb") != string::npos) {colSys = PbPb; multRange = 3000;}

    if(dataSet.find("pp_2TeV") != string::npos) {erg = erg2TeV; multRange = 40;}
    if(dataSet.find("pp_7TeV") != string::npos) erg = erg7TeV;
    if(dataSet.find("pp_13TeV") != string::npos) erg = erg13TeV;
    if(dataSet.find("pp_5TeV") != string::npos) erg = erg5TeV;
    if(dataSet.find("pPb_5TeV") != string::npos) erg = erg5TeV_NN;
    if(dataSet.find("PbPb_5TeV") != string::npos) erg = erg5TeV_NN;
    if(dataSet == "Publications" || dataSet == "Simulations"|| dataSet == "Energyscan") continue;

    string datasetLable = alice + newLine + chargedParticles + ", " + colSys + ", " + erg + newLine + eta08 + ", " + ptRange;
    string datasetLablePrel = alicePrel + newLine + chargedParticles + ", " + colSys + ", " + erg + newLine + eta08 + ", " + ptRange;
    string qaFolder = "ConsistencyChecks";
    string systematicsFolder = "SystematicUncertainties";

// Start layground

{ // -----------------------------------------------------------------------
  string plotName = "covMat_multDistUnfolded";
  Plot myPlot(plotName);
  myPlot.SetFigureGroup(dataSet);
//  myPlot.SetDrawingProperties("logZ");
  myPlot.AddHisto(plotName.c_str(), "", "");
  myPlot.AddLegendBox(0.6, 0.7);
  myPlot.AddTextBox(0.4, 0.91, datasetLable);
//  myPlot.SetAxisRange("Z", -1e6, 1e6);
//  myPlot.SetAxisRange("Y", 0.1, 1e6);
  plotEnv.CreateNewPlot(myPlot, "default", false);
  plotEnv.SavePlot(plotName, dataSet, qaFolder);
} // -----------------------------------------------------------------------

{ // -----------------------------------------------------------------------
  string plotName = "sign_covMat_multDistUnfolded";
  Plot myPlot(plotName);
  myPlot.SetFigureGroup(dataSet);
//  myPlot.SetDrawingProperties("logZ");
  myPlot.AddHisto(plotName.c_str(), "", "");
  myPlot.AddLegendBox(0.6, 0.7);
//  myPlot.AddTextBox(0.4, 0.91, datasetLable);
//  myPlot.SetAxisRange("Z", -1e6, 1e6);
//  myPlot.SetAxisRange("Y", 0.1, 1e6);
  plotEnv.CreateNewPlot(myPlot, "default", false);
  plotEnv.SavePlot(plotName, dataSet, qaFolder);
} // -----------------------------------------------------------------------

{ // -----------------------------------------------------------------------
  string plotName = "abs_covMat_multDistUnfolded";
  Plot myPlot(plotName);
  myPlot.SetFigureGroup(dataSet);
  myPlot.SetDrawingProperties("logZ");
  myPlot.AddHisto(plotName.c_str(), "", "");
  myPlot.AddLegendBox(0.6, 0.7);
//  myPlot.AddTextBox(0.4, 0.91, datasetLable);
//  myPlot.SetAxisRange("Z", -1e6, 1e6);
//  myPlot.SetAxisRange("Y", 0.1, 1e6);
  plotEnv.CreateNewPlot(myPlot, "default", false);
  plotEnv.SavePlot(plotName, dataSet, qaFolder);
} // -----------------------------------------------------------------------


{ // -----------------------------------------------------------------------
  string plotName = "edgeContamPt";
  Plot myPlot(plotName);
  myPlot.SetFigureGroup(dataSet);
  myPlot.SetDrawingProperties("logX");
  myPlot.AddHisto(plotName.c_str(), "", "");
  //myPlot.SetAxisRange("Y", 0.4, 1.01);
  myPlot.SetAxisRange("X", 0, 20);
  myPlot.SetAxisTitle("Y", "contamination");
  myPlot.AddLegendBox(0.3, 0.5);
  myPlot.AddTextBox(0.41, 0.3, datasetLable);
  plotEnv.CreateNewPlot(myPlot, "default", false);
  plotEnv.SavePlot(plotName, dataSet, qaFolder);
} // -----------------------------------------------------------------------

{ // -----------------------------------------------------------------------
  string plotName = "edgeContamMult";
  Plot myPlot(plotName);
  myPlot.SetFigureGroup(dataSet);
  myPlot.AddHisto(plotName.c_str(), "", "");
  //myPlot.SetAxisRange("Y", 0.4, 1.01);
  myPlot.SetAxisRange("X", 0, 20);
  myPlot.SetAxisTitle("Y", "contamination");
  myPlot.AddLegendBox(0.3, 0.5);
  myPlot.AddTextBox(0.41, 0.3, datasetLable);
  plotEnv.CreateNewPlot(myPlot, "default", false);
  plotEnv.SavePlot(plotName, dataSet, qaFolder);
} // -----------------------------------------------------------------------


{ // -----------------------------------------------------------------------
  string plotName = "triggerEff";
  Plot myPlot(plotName);
  myPlot.SetFigureGroup(dataSet);
  myPlot.AddHisto("eventEfficiency", "", "efficiency of selected event to be measured");
  myPlot.AddHisto(plotName.c_str(), "", "efficiency including event selection");
  myPlot.AddHisto("eventLossEff", "", "remaining");
  myPlot.SetAxisRange("Y", 0.4, 1.01);
  myPlot.SetAxisRange("X", 0, 10);
  myPlot.SetAxisTitle("Y", "efficiency");
  myPlot.AddLegendBox(0.3, 0.5);
  myPlot.AddTextBox(0.41, 0.3, datasetLable);
  plotEnv.CreateNewPlot(myPlot, "default", false);
  plotEnv.SavePlot(plotName, dataSet, qaFolder);
} // -----------------------------------------------------------------------

{ // -----------------------------------------------------------------------
  string plotName = "chi2_multDistUnfolded";
  Plot myPlot(plotName);
  myPlot.SetFigureGroup(dataSet);
  myPlot.SetDrawingProperties("logY");
  myPlot.AddHisto("chi2_multDistUnfolded", dataSet, "data", kFullCross, kRed+3);
  myPlot.AddHisto("chi2_multDistUnfoldedMC", dataSet, "mc", kFullCross, kBlue+3);
  myPlot.AddHisto("chi2_multDistUnfoldedClosureTestFlat", dataSet, "mc flat prior", kFullCross, kGreen+3);
  myPlot.AddLegendBox(0.6, 0.7);
  myPlot.AddTextBox(0.4, 0.91, datasetLable);
//  myPlot.SetAxisRange("X", 0, 20);
  myPlot.SetAxisRange("Y", 0.1, 1e6);
  plotEnv.CreateNewPlot(myPlot, "default", false);
  plotEnv.SavePlot(plotName, dataSet, qaFolder);
} // -----------------------------------------------------------------------

{ // -----------------------------------------------------------------------
  string plotName = "chi2_multPtUnfoldedRaw_PtBin";
  Plot myPlot(plotName);
  myPlot.SetFigureGroup(dataSet);
  myPlot.SetDrawingProperties("logY");
  vector<int> ptBins = {2, 3, 5, 7, 11, 13, 15, 17, 20, 30, 40, 50, 51};
  int i = 0;
  for(int ptBin : ptBins)
  {
    myPlot.AddHisto(plotName + "_" + std::to_string(ptBin), dataSet, GetPtString(ptBin), 0, 0, "hist");
    i++;
  }
  myPlot.AddLegendBox(0.5, 0.78);
  myPlot.AddTextBox(0.3, 0.92, datasetLable);
//  myPlot.SetAxisRange("X", 0, 20);
  plotEnv.CreateNewPlot(myPlot, "default", false);
  plotEnv.SavePlot(plotName, dataSet, qaFolder);
} // -----------------------------------------------------------------------

{ // -----------------------------------------------------------------------
  string plotName = "chi2_multPtUnfoldedRaw_MultBin";
  Plot myPlot(plotName);
  myPlot.SetFigureGroup(dataSet);
  myPlot.SetDrawingProperties("logY");
  vector<int> multBins = {2, 6, 9, 11, 16, 21, 31, 41, 51, 61};
  int i = 0;
  for(int multBin : multBins)
  {
    myPlot.AddHisto(plotName + "_" + std::to_string(multBin), dataSet, "Nch = " + std::to_string(multBin-1), 0, 0, "hist");
    i++;
  }
  myPlot.AddLegendBox(0.6, 0.7);
  myPlot.AddTextBox(0.4, 0.91, datasetLable);
//  myPlot.SetAxisRange("X", 0, 20);
  plotEnv.CreateNewPlot(myPlot, "default", false);
  plotEnv.SavePlot(plotName, dataSet, qaFolder);
} // -----------------------------------------------------------------------

{ // -----------------------------------------------------------------------
  string plotName = "particleClosure";
  Plot myPlot(plotName);
  myPlot.SetFigureGroup(dataSet);
//  myPlot.AddHisto(plotName + "MCmeasVsNch", "", "sanityCheck MC meas vs Nch", kOpenCross, kBlue+1);
//  myPlot.AddHisto(plotName + "MCprimaries", "", "sanityCheck MC meas prim vs Nch", kOpenCross, kBlack+1);
//  myPlot.AddHisto(plotName + "MCmeas", "", "sanityCheck MC meas", kOpenCross, kMagenta+1);
  myPlot.AddHisto(plotName + "MCtruth", "", "MC truth", kOpenSquare, kGreen+2);
  myPlot.AddHisto(plotName + "MC", "", "MC unfolded", kOpenCircle, kRed+1);
  myPlot.AddHisto(plotName, "", "data unfolded", kFullCross, kBlue+3);
//  myPlot.AddHisto();
  myPlot.AddLegendBox(0.5, 0.7);
  myPlot.SetAxisRange("Y", 0.9, 1.6);
  myPlot.SetAxisRange("X", 0, multRange);
  myPlot.SetAxisTitle("Y", "integrated / true #it{N}_{ch}");
  myPlot.AddTextBox(0.6, 0.5, "particles per event in // measured spectra // over respective #it{N}_{ch}");
  myPlot.AddTextBox(0.15, 0.91, datasetLable);
  plotEnv.CreateNewPlot(myPlot, "default", false);
  plotEnv.SavePlot(plotName, dataSet, qaFolder);
} // -----------------------------------------------------------------------

{ // -----------------------------------------------------------------------
  string plotName = "particeContaminationVsNch";
  Plot myPlot(plotName);
  myPlot.SetFigureGroup(dataSet);
  myPlot.AddHisto("particleClosureMCmeasVsNch", "", "prim and sec", kOpenCross, kBlue+1);
  myPlot.AddHisto("particleClosureMCprimaries", "", "only prim", kFullCross, kBlack+1);
  myPlot.AddRatio("particleClosureMCmeasVsNch", "", "particleClosureMCprimaries", "", "sec contam");
  myPlot.SetAxisTitle("ratio", "all / prim");
  myPlot.SetAxisTitle("Y", "measured over true particles");
  myPlot.SetAxisRange("X", 0, 60);
  myPlot.SetAxisRange("Y", 0.6, 1.2);
  myPlot.SetAxisRange("ratio", 0.95, 1.1);
  myPlot.AddLegendBox(0.6, 0.6);
  myPlot.AddTextBox(0.15, 0.91, datasetLable);
  myPlot.AddTextBox(0.5, 0.4, "particles per event in // measured spectra // over respective #it{N}_{ch}");
  plotEnv.CreateNewPlot(myPlot, "default ratio", false);
  plotEnv.SavePlot(plotName, dataSet, qaFolder);
} // -----------------------------------------------------------------------

{

  int ptBin = 28;

{ // -----------------------------------------------------------------------
  string plotName = "presentation_PtBin_measuredRawMC";
  Plot myPlot(plotName);
  myPlot.SetFigureGroup(dataSet);
  myPlot.SetDrawingProperties("logY");
  myPlot.AddHisto(string("multPtMeasuredMC_PtBin_") + std::to_string(ptBin), "", "measured", kFullCross, kGreen+3);
  myPlot.AddLegendBox(0.2, 0.4);
  myPlot.SetAxisRange("Y", 1e-8, 1e-3);
  myPlot.AddTextBox(0.4, 0.91, datasetLable);
  plotEnv.CreateNewPlot(myPlot, "default", false);
  plotEnv.SavePlot(plotName, dataSet);
} // -----------------------------------------------------------------------

{ // -----------------------------------------------------------------------
  string plotName = "presentation_PtBin_unfoldedRawMC";
  Plot myPlot(plotName);
  myPlot.SetFigureGroup(dataSet);
  myPlot.SetDrawingProperties("logY");
  myPlot.AddHisto(string("multPtUnfoldedMC_PtBin_") + std::to_string(ptBin), "", "un-smeared", kFullCircle, kBlue+1);
  myPlot.AddLegendBox(0.2, 0.4);
  myPlot.SetAxisRange("Y", 1e-8, 1e-3);
  myPlot.AddTextBox(0.4, 0.91, datasetLable);
  plotEnv.CreateNewPlot(myPlot, "default", false);
  plotEnv.SavePlot(plotName, dataSet);
} // -----------------------------------------------------------------------

int multBin = 18;

{ // -----------------------------------------------------------------------
  string plotName = "presentation_MultBin_unfoldedRawMC";
  Plot myPlot(plotName);
  myPlot.SetFigureGroup(dataSet);
  myPlot.SetDrawingProperties("logY logX");
  //myPlot.AddHisto(string("multPtUnfoldedMC_MultBin_") + std::to_string(multBin), "", "unfolded", kFullCircle, kBlue+1);
  myPlot.AddHisto(string("multPtUnfoldedMC_MultBin_") + std::to_string(multBin), "", "un-smeared", kFullCircle, kBlue+1);
  myPlot.AddLegendBox(0.6, 0.8);
  myPlot.SetAxisRange("Y", 1e-12, 2);
  myPlot.SetAxisTitle("X", "#it{p}^{ meas}_{T}");
  myPlot.AddTextBox(0.16, 0.3, datasetLable);
  plotEnv.CreateNewPlot(myPlot, "default", false);
  plotEnv.SavePlot(plotName, dataSet);
} // -----------------------------------------------------------------------


}

// End playground

    //--------------------------------------------------------------------------
    //---------------------- result plots --------------------------------------
    //--------------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "multDensity";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logY");
      myPlot.AddHisto("multDensityUnfolded", dataSet, "", kFullCircle, kBlue+1);
      myPlot.AddHisto("multDensityUnfolded_Syst", dataSet, "data", kFullCircle, kBlue+1, "boxes");
      myPlot.AddHisto("multDensityGeneratedMC", dataSet, "mc", kOpenCircle, kBlue+1);
      myPlot.AddRatio("multDensityUnfolded_Syst", "", "multDensityGeneratedMC", "", "");
      myPlot.AddLegendBox(0.2, 0.4);
      myPlot.SetAxisRange("X", 0, 130);
      myPlot.AddTextBox(0.4, 0.91, datasetLable);
      plotEnv.CreateNewPlot(myPlot, "default ratio", false);
      plotEnv.SavePlot(plotName, dataSet);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "multDists";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logY");
      myPlot.AddHisto("multDistMeasured", dataSet, "measured");
      myPlot.AddHisto("multDistUnfolded", dataSet, "", kFullCircle, kBlue+1);
      myPlot.AddHisto("multDistUnfolded_Syst", dataSet, "unfolded", kFullCircle, kBlue+1, "boxes");
      //myPlot.SetAxisRange("X", 0, multRange);
      myPlot.AddLegendBox(0.2, 0.4);
      myPlot.AddTextBox(0.4, 0.91, datasetLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "multDistsMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logY");
      myPlot.AddHisto("multDistMeasuredMC", dataSet, "measured");
      myPlot.AddHisto("multDistUnfoldedMC", dataSet, "unfolded");
      myPlot.AddHisto("multDistGeneratedMC", dataSet, "generated");
      //myPlot.SetAxisRange("X", 0, multRange);
      myPlot.AddLegendBox(0.2, 0.4);
      myPlot.AddTextBox(0.4, 0.91, datasetLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "multPtMeasured";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logZ logY");
      myPlot.AddHisto(plotName);
      //myPlot.SetAxisRange("X", 0, multRange);
      myPlot.SetAxisRange("Y", 0.15, 50.0);
      myPlot.SetAxisRange("Z", 1e-15, 5);
      //myPlot.SetAxisTitle("X", "#it{N}^{ meas}_{ch}");
      myPlot.SetAxisTitle("Y", "#it{p}^{ meas}_{T}");
      myPlot.SetAxisTitle("Z", "norm. yield");
      //myPlot.SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{acc}) (GeV/#it{c})^{-2}");
      myPlot.AddTextBox(0.15, 0.8, datasetLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "multPtMeasuredRaw";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logZ logY");
      myPlot.AddHisto(plotName);
      //myPlot.SetAxisRange("X", 0, multRange);
      myPlot.SetAxisRange("Y", 0.15, 50.0);
      //myPlot.SetAxisRange("Z", 1e-15, 5);
      //myPlot.SetAxisTitle("X", "#it{N}^{ meas}_{ch}");
      myPlot.SetAxisTitle("Y", "#it{p}^{ meas}_{T} (GeV/#it{c})");
      //myPlot.SetAxisTitle("Z", "norm. yield");
      myPlot.SetAxisTitle("Z", "raw yield");
      //myPlot.SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{acc}) (GeV/#it{c})^{-2}");
      myPlot.AddTextBox(0.15, 0.8, datasetLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "multPtUnfolded";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logZ logY");
      myPlot.AddHisto(plotName);
      myPlot.SetAxisRange("X", 0, multRange);
      myPlot.SetAxisRange("Y", 0.15, 50.0);
      myPlot.SetAxisRange("Z", 1e-15, 0.5);
      //myPlot.SetAxisTitle("Z", "raw yield");
      //myPlot.SetAxisTitle("Z", "raw yield");abcde
      myPlot.SetAxisTitle("Y", "#it{p}_{T} (GeV/#it{c})");
      myPlot.SetAxisTitle("X", "#it{N}_{ch}");

      //myPlot.SetAxisTitle("Y", "#it{p}^{ true}_{T} (GeV/#it{c})");
      //myPlot.SetAxisTitle("Z", "norm. yield");
      myPlot.SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{ch}) [(GeV/#it{c})^{-2}]");
      myPlot.AddTextBox(0.15, 0.8, datasetLablePrel);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "multPtMeasuredMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logZ logY");
      myPlot.AddHisto(plotName);
      //myPlot.SetAxisRange("X", 0, multRange);
      myPlot.SetAxisRange("Y", 0.15, 50.0);
      myPlot.SetAxisRange("Z", 1e-15, 5);
      //myPlot.SetAxisTitle("X", "#it{N}^{ meas}_{ch}");
      myPlot.SetAxisTitle("Y", "#it{p}^{ meas}_{T}");
      myPlot.SetAxisTitle("Z", "norm. yield");
      //myPlot.SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{acc}) (GeV/#it{c})^{-2}");
      myPlot.AddTextBox(0.15, 0.8, datasetLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "multPtMeasuredRawMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logZ logY");
      myPlot.AddHisto(plotName);
      //myPlot.SetAxisRange("X", 0, multRange);
      myPlot.SetAxisRange("Y", 0.15, 50.0);
      //myPlot.SetAxisRange("Z", 1e-15, 5);
      //myPlot.SetAxisTitle("X", "#it{N}^{ meas}_{ch}");
      myPlot.SetAxisTitle("Y", "#it{p}^{ meas}_{T}");
      //myPlot.SetAxisTitle("Z", "norm. yield");
      myPlot.SetAxisTitle("Z", "raw yield");
      //myPlot.SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{acc}) (GeV/#it{c})^{-2}");
      myPlot.AddTextBox(0.15, 0.8, datasetLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "multPtUnfoldedMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logZ logY");
      myPlot.AddHisto(plotName);
      //myPlot.SetAxisRange("X", 0, multRange);
      myPlot.SetAxisRange("Y", 0.15, 50.0);
      myPlot.SetAxisRange("Z", 1e-15, 5);
      //myPlot.SetAxisTitle("X", "#it{N}^{ true}_{ch}");
      myPlot.SetAxisTitle("Y", "#it{p}^{ true}_{T}");
      myPlot.SetAxisTitle("Z", "norm. yield");
      //myPlot.SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{ch}) (GeV/#it{c})^{-2}");
      myPlot.AddTextBox(0.15, 0.8, datasetLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "multPtGeneratedMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logZ logY");
      myPlot.AddHisto(plotName);
      //myPlot.SetAxisRange("X", 0, multRange);
      //myPlot.SetAxisRange("Y", 0.15, 50.0);
      //myPlot.SetAxisRange("Z", 1e-15, 5);
      //myPlot.SetAxisTitle("X", "#it{N}^{ true}_{ch}");
      myPlot.SetAxisTitle("Y", "#it{p}^{ true}_{T}");
      myPlot.SetAxisTitle("Z", "norm. yield");
      //myPlot.SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{ch}) (GeV/#it{c})^{-2}");
      myPlot.AddTextBox(0.15, 0.8, datasetLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "meanPt";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto("momentUnfolded1", "", "", kFullSquare, kBlue+1, "", multRange);
      myPlot.AddHisto("momentUnfolded1_Syst", "", "data", kFullSquare, kBlue+1, "boxes", multRange);
      myPlot.AddHisto("momentGeneratedMC1", "", "MC", kOpenCircle, kBlack, "", multRange);
      myPlot.SetAxisRange("X", 0, multRange);
      myPlot.SetAxisRange("Y", 0.45, 0.85);
      myPlot.AddLegendBox(0.5, 0.3);
      myPlot.AddTextBox(0.16, 0.92, datasetLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "variance";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto("varianceUnfolded", "", "", kFullSquare, kBlue+1);
      myPlot.AddHisto("varianceUnfolded_Syst", "", "unfolded data", kFullSquare, kBlue+1, "boxes");
      myPlot.AddHisto("varianceGeneratedMC", "", "MC", kOpenCircle, kBlack, "");
      myPlot.SetAxisRange("X", 0, multRange);
      myPlot.SetAxisRange("Y", 0., 0.9);
      myPlot.AddLegendBox(0.2, 0.6);
      myPlot.AddTextBox(0.16, 0.92, datasetLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet);
    } // -----------------------------------------------------------------------

    //--------------------------------------------------------------------------
    //---------------------- technical plots -----------------------------------
    //--------------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "multCorrelationMatrix";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logZ");
      myPlot.AddHisto(plotName);
      myPlot.SetAxisRange("X", 0, 10);
      myPlot.SetAxisRange("Y", 0, 10);
      myPlot.AddTextBox(0.3, 0.3, datasetLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "responseMatrixOrig";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logZ");
      myPlot.AddHisto(plotName);
      //myPlot.SetAxisRange("X", 0, multRange);
      //myPlot.SetAxisRange("Y", 0, multRange);
      myPlot.AddTextBox(0.3, 0.3, datasetLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "unfoldingMatrix";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logZ");
      myPlot.AddHisto(plotName);
      //myPlot.SetAxisRange("X", 0, multRange);
      //myPlot.SetAxisRange("Y", 0, multRange);
      myPlot.AddTextBox(0.3, 0.3, datasetLable);
      //myPlot.SetAxisTitle("X", "#it{N}^{ meas}_{ch}");
      //myPlot.SetAxisTitle("Y", "#it{N}^{ true}_{ch}");
      //myPlot.SetAxisTitle("Z", "#it{P}(#it{N}_{acc} | #it{N}_{ch})");
      //myPlot.SetAxisTitle("Z", "#it{P}(#it{N}^{ meas}_{ch} | #it{N}^{ true}_{ch})");
      myPlot.SetAxisTitle("Z", "#it{S}(#it{N}_{acc} | #it{N}_{ch})");
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "responseMatrix";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logZ");
      myPlot.AddHisto(plotName);
      //myPlot.SetAxisRange("X", 0, multRange);
      //myPlot.SetAxisRange("Y", 0, multRange);
      myPlot.AddTextBox(0.3, 0.3, datasetLable);
      //myPlot.SetAxisTitle("X", "#it{N}^{ meas}_{ch}");
      //myPlot.SetAxisTitle("Y", "#it{N}^{ true}_{ch}");
      //myPlot.SetAxisTitle("Z", "#it{P}(#it{N}_{acc} | #it{N}_{ch})");
      //myPlot.SetAxisTitle("Z", "#it{P}(#it{N}^{ meas}_{ch} | #it{N}^{ true}_{ch})");
      myPlot.SetAxisTitle("Z", "#it{S}(#it{N}_{acc} | #it{N}_{ch})");
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "ptResolutionMatrix";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logX logY logZ");
      //myPlot.SetAxisRange("X", 0.15, 50);
      //myPlot.SetAxisRange("Y", 0.15, 50);
      myPlot.SetAxisTitle("X", "#it{p}^{ meas}_{T}");
      myPlot.SetAxisTitle("Y", "#it{p}^{ true}_{T}");
      myPlot.AddTextBox(0.3, 0.3, datasetLable);
      myPlot.AddHisto(plotName);
      myPlot.SetAxisTitle("Z", "#it{S}(#it{p}^{ meas}_{T} | #it{p}^{ true}_{T})");
      //myPlot.SetAxisTitle("Z", "#it{P}(#it{p}^{ meas}_{T} | #it{p}^{ true}_{T})");
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "relPtResoFromCovData";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logZ logX");
      myPlot.SetAxisRange("X", 0.15, 50);
      myPlot.SetAxisRange("Z", 2, 5e7);
      myPlot.SetAxisTitle("Z", "# Tracks");
      myPlot.AddTextBox(0.3, 0.9, datasetLable);
      myPlot.AddHisto(plotName);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "relPtResoFromCovMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logZ logX");
      myPlot.SetAxisRange("X", 0.15, 50);
      myPlot.SetAxisTitle("Z", "# Tracks");
      myPlot.AddTextBox(0.3, 0.9, datasetLable);
      myPlot.AddHisto(plotName);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet);
    } // -----------------------------------------------------------------------

    //--------------------------------------------------------------------------
    //--------------------- QA histograms --------------------------------------
    //--------------------------------------------------------------------------

    // ---------------------- Efficiencies------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "eventEfficiency";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto(plotName.c_str());
      myPlot.SetAxisRange("Y", 0.5, 1.01);
      myPlot.SetAxisRange("X", 0, 10);
      myPlot.SetAxisTitle("Y", "efficiency");
      myPlot.AddTextBox(0.41, 0.3, datasetLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet, qaFolder);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "fakeEventContam";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto(plotName.c_str());
      myPlot.SetAxisRange("X", 0, 10);
      myPlot.AddTextBox(0.41, 0.92, datasetLable);
      myPlot.SetAxisTitle("Y", "contamination");
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet, qaFolder);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "efficiencyMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto("primTrackEff", dataSet, "", 0, kBlue);
      myPlot.SetDrawingProperties("logX");
      myPlot.SetAxisRange("X", 0.15, 50);
      myPlot.SetAxisRange("Y", 0.3, 0.9);
      myPlot.AddLegendBox(0.6, 0.4);
      myPlot.AddTextBox(0.15, 0.9, datasetLable);
      myPlot.SetAxisTitle("X", "#it{p}^{ true}_{T}");
      myPlot.SetAxisTitle("Y", "efficiency x acceptance");
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "secContamMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto(plotName, dataSet, "", 0, kBlue);
      myPlot.SetDrawingProperties("logX");
      myPlot.SetAxisRange("X", 0.15, 50);
      myPlot.SetAxisTitle("X", "#it{p}^{ true}_{T}");
      myPlot.AddTextBox(0.4, 0.9, datasetLable);
      myPlot.SetAxisTitle("Y", "secondary contamination");
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "secContamMC_mult";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto(plotName, dataSet, "", 0, kBlue);
      //myPlot.SetDrawingProperties("logX");
      myPlot.SetAxisRange("Y", 0.0, 0.08);
      //myPlot.SetAxisTitle("X", "#it{p}^{ true}_{T}");
      myPlot.AddTextBox(0.4, 0.9, datasetLable);
      myPlot.SetAxisTitle("Y", "secondary contamination");
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "efficiencyMultPtRaw_MultDep";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      vector<int> ptBins = {3, 5, 7, 11, 13, 15, 17};
      int i = 0;
      for(int ptBin : ptBins)
      {
        string name = "efficiencyMultPtRaw_PtBin_" + std::to_string(ptBin);
        myPlot.AddHisto(name, dataSet, GetPtString(ptBin));
        i++;
      }
      myPlot.SetAxisRange("X", 0., multRange);
      myPlot.SetAxisRange("Y", 0.25, 0.8);
      myPlot.AddLegendBox(0.16, 0.52);
      //myPlot.AddTextBox(0.41, 0.3, datasetLable);
      myPlot.SetAxisTitle("Y", "acceptance x efficiency");
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet, qaFolder);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "efficiencyMultPtRaw_PtDep";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto("primTrackEff", dataSet, "integrated");
      vector<int> multBins = {2, 3, 5, 7, 9, 11};
      for(int multBin : multBins)
      {
        string multLable = "#it{N}_{ch} = " + std::to_string(multBin-1);
        string name = "efficiencyMultPtRaw_MultBin_" + std::to_string(multBin);
        myPlot.AddHisto(name, dataSet, multLable);
      }
      myPlot.SetDrawingProperties("logX");
      myPlot.AddLegendBox(0.22, 0.52);
      myPlot.AddTextBox(0.41, 0.3, datasetLable);
      myPlot.SetAxisRange("Y", 0.2, 0.9);
      myPlot.SetAxisTitle("Y", "acceptance x efficiency");
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet, qaFolder);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "secContamMultPtRaw_MultDep";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      vector<int> ptBins = {3, 5, 7, 11, 13, 15, 17};
      int i = 0;
      for(int ptBin : ptBins)
      {
        string name = "secContamMultPtRaw_PtBin_" + std::to_string(ptBin);
        myPlot.AddHisto(name, dataSet, GetPtString(ptBin));
        i++;
      }
      myPlot.AddLegendBox(0.16, 0.92, "");
      myPlot.SetAxisRange("X", 0., multRange);
      myPlot.SetAxisRange("Y", 0, 0.12);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet, qaFolder);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "secondariesMultPtRaw_PtDep";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto("secContamMC", dataSet, "MC truth");
      myPlot.AddHisto("secContamMultPtRaw_full", dataSet, "inclusive");
      vector<int> multBins = {2, 3, 5, 7, 9, 11};
      for(int multBin : multBins)
      {
        string name = "secContamMultPtRaw_MultBin_" + std::to_string(multBin);
        myPlot.AddHisto(name, dataSet, std::to_string(multBin));
      }
      myPlot.SetDrawingProperties("logX");
      myPlot.AddLegendBox(0.7, 0.3);
      myPlot.SetAxisRange("Y", 0, 0.15);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, dataSet, qaFolder);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "meanRelPtReso_DataVsMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logX");
      myPlot.SetAxisTitle("Y", "< #sigma(#it{p}_{T}) / #it{p}_{T} >");
      myPlot.SetAxisRange("X", 0.15, 50);
      myPlot.SetAxisRange("ratio", 0.95, 1.05);
      myPlot.AddHisto("meanRelPtResoFromCovMC", "", "MC");
      myPlot.AddHisto("meanRelPtResoFromCovData", "", "data");
      myPlot.AddRatio("meanRelPtResoFromCovData", "", "meanRelPtResoFromCovMC");
      myPlot.SetAxisTitle("ratio", "data / MC");
      myPlot.AddLegendBox(0.4, 0.5, "");
      myPlot.AddTextBox(0.3, 0.9, datasetLable);
      plotEnv.CreateNewPlot(myPlot, "default ratio", false);
      plotEnv.SavePlot(plotName, dataSet, qaFolder);
    } // -----------------------------------------------------------------------
    // ---------------------- Closure tests ------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "closureTestMultDist";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto("multDistGeneratedClosureTest", "", "generated (prior)", kFullSquare, kGray+1);
      myPlot.AddHisto("multDistUnfoldedClosureTest", "", "unfolded", kOpenCircle);
      myPlot.AddHisto("multDistMeasuredClosureTest", "", "measured");
      myPlot.AddRatio("multDistUnfoldedClosureTest", "", "multDistGeneratedClosureTest", "", "");
      myPlot.SetAxisTitle("ratio", "ratio");
      myPlot.SetDrawingProperties("logY");
      myPlot.SetAxisRange("X", 0, multRange);
      //myPlot.SetAxisRange("Y", 0.9, 3e5);
      myPlot.AddLegendBox(0.6, 0.9);
      myPlot.AddTextBox(0.16, 0.2, datasetLable);
      myPlot.SetAxisRange("ratio", 0.95, 1.05);
      plotEnv.CreateNewPlot(myPlot, "default ratio", false);
      plotEnv.SavePlot(plotName, dataSet, qaFolder);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "closureTestMultDistFlat";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto("multDistGeneratedClosureTest", "", "generated", kFullSquare, kGray+1);
      myPlot.AddHisto("multDistUnfoldedClosureTestFlat", "", "unfolded", kOpenCircle);
      myPlot.AddHisto("multDistMeasuredClosureTest", "", "measured");
      myPlot.AddHisto("multDistInitialClosureTestFlat", "", "prior", 0, 0, "hist");
      myPlot.AddRatio("multDistUnfoldedClosureTestFlat", "", "multDistGeneratedClosureTest", "", "");
      myPlot.SetAxisTitle("ratio", "ratio");
      myPlot.SetDrawingProperties("logY");
      myPlot.SetAxisRange("X", 0, multRange);
      //myPlot.SetAxisRange("Y", 0.9, 3e5);
      myPlot.SetAxisRange("ratio", 0.95, 1.05);
      myPlot.AddLegendBox(0.7, 0.8);
      myPlot.AddTextBox(0.16, 0.2, datasetLable);
      plotEnv.CreateNewPlot(myPlot, "default ratio", false);
      plotEnv.SavePlot(plotName, dataSet, qaFolder);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "closureTestProjectionPt";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logX logY");
      myPlot.SetAxisRange("X", 0.15, 50);
      myPlot.AddHisto("trueParticlesPt", "", "true");
      myPlot.AddHisto("unfoParticlesPt", "", "unfolded");
      myPlot.AddHisto("TEST", "", "TEST");
      myPlot.AddHisto("TESTDATA", "", "TESTDATA");
      myPlot.AddHisto("TESTDATAMEAS", "", "TESTDATAMEAS");
      myPlot.AddHisto("TESTMCMEAS", "", "TESTMCMEAS");
      myPlot.AddRatio("unfoParticlesPt", "", "trueParticlesPt");
//      myPlot.SetAxisRange("ratio", 0.9, 1.1);
      myPlot.SetAxisTitle("X", "#it{p}_{T} (GeV/#it{c})");
      myPlot.SetAxisTitle("ratio", "ratio");
      myPlot.AddLegendBox(0.7, 0.85);
      myPlot.AddTextBox(0.16, 0.2, datasetLable);
      plotEnv.CreateNewPlot(myPlot, "default ratio", false);
      plotEnv.SavePlot(plotName, dataSet, qaFolder);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "closureTestProjectionMult";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto("trueParticlesMult", "", "true");
      myPlot.AddHisto("unfoParticlesMult", "", "unfolded");
      myPlot.AddRatio("unfoParticlesMult", "", "trueParticlesMult");
      myPlot.SetAxisTitle("ratio", "ratio");
      myPlot.SetDrawingProperties("logY");
      myPlot.SetAxisRange("ratio", 0.95, 1.05);
      myPlot.AddLegendBox(0.7, 0.7);
      myPlot.AddTextBox(0.16, 0.2, datasetLable);
      plotEnv.CreateNewPlot(myPlot, "default ratio", false);
      plotEnv.SavePlot(plotName, dataSet, qaFolder);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "closureTestMoment1";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto("momentGeneratedMC1", "", "generated");
      myPlot.AddHisto("momentUnfoldedMC1", "", "unfolded");
      myPlot.AddHisto("momentReweightedMC1", "", "re-weighted");
      myPlot.AddRatio("momentUnfoldedMC1", "", "momentGeneratedMC1");
      myPlot.AddRatio("momentReweightedMC1", "", "momentGeneratedMC1");
      myPlot.SetAxisRange("X", 0, multRange);
      myPlot.SetAxisRange("Y", 0.45, 0.85);
      myPlot.SetAxisRange("ratio", 0.95, 1.05);
      myPlot.SetAxisTitle("ratio", "ratio");
      myPlot.AddLegendBox(0.6, 0.4);
      myPlot.AddTextBox(0.16, 0.92, datasetLable);
      plotEnv.CreateNewPlot(myPlot, "default ratio", false);
      plotEnv.SavePlot(plotName, dataSet, qaFolder);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "closureTestMoment2";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto("momentGeneratedMC2", "", "generated");
      myPlot.AddHisto("momentUnfoldedMC2", "", "unfolded");
      //myPlot.AddHisto("momentReweightedMC2", "", "re-weighted");
      myPlot.AddRatio("momentUnfoldedMC2", "", "momentGeneratedMC2");
      //myPlot.AddRatio("momentReweightedMC2", "", "momentGeneratedMC2");
      myPlot.SetAxisRange("X", 0, multRange);
      myPlot.SetAxisRange("Y", 0.2, 1.6);
      myPlot.SetAxisRange("ratio", 0.95, 1.05);
      myPlot.SetAxisTitle("ratio", "ratio");
      myPlot.AddLegendBox(0.6, 0.4);
      myPlot.AddTextBox(0.16, 0.92, datasetLable);
      plotEnv.CreateNewPlot(myPlot, "default ratio", false);
      plotEnv.SavePlot(plotName, dataSet, qaFolder);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "closureTestMoment3";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto("momentGeneratedMC3", "", "generated");
      myPlot.AddHisto("momentUnfoldedMC3", "", "unfolded");
      //myPlot.AddHisto("momentReweightedMC3", "", "re-weighted");
      myPlot.AddRatio("momentUnfoldedMC3", "", "momentGeneratedMC3");
      //myPlot.AddRatio("momentReweightedMC3", "", "momentGeneratedMC3");
      myPlot.SetAxisRange("X", 0, multRange);
      myPlot.SetAxisRange("Y", 0.1, 7.0);
      myPlot.SetAxisRange("ratio", 0.95, 1.05);
      myPlot.SetAxisTitle("ratio", "ratio");
      myPlot.AddLegendBox(0.7, 0.3);
      myPlot.AddTextBox(0.16, 0.92, datasetLable);
      plotEnv.CreateNewPlot(myPlot, "default ratio", false);
      plotEnv.SavePlot(plotName, dataSet, qaFolder);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      for(int ptBin = 2; ptBin < 50; ptBin++){
        if(!createBinWiseClosureTests) break;
        string plotName = "closureTestPtBin_" + std::to_string(ptBin);
        Plot myPlot(plotName);
        myPlot.SetFigureGroup(dataSet);
        myPlot.AddHisto(string("multPtGeneratedMC_PtBin_") + std::to_string(ptBin), "", "", kFullSquare, kBlack);
        myPlot.AddHisto(string("multPtMeasuredMC_PtBin_") + std::to_string(ptBin), "", "measured", kFullCross, kGreen+3);
        myPlot.AddHisto(string("multPtGeneratedMC_PtBin_") + std::to_string(ptBin), "", "generated", kFullSquare, kBlack);
        myPlot.AddHisto(string("multPtUnfoldedMC_PtBin_") + std::to_string(ptBin), "", "unfolded", kFullCircle, kBlue+1);
        myPlot.AddRatio(string("multPtUnfoldedMC_PtBin_") + std::to_string(ptBin), "", string("multPtGeneratedMC_PtBin_") + std::to_string(ptBin));
        myPlot.SetAxisRange("X", 0, multRange);
        myPlot.SetAxisRange("ratio", 0.95, 1.05);
        myPlot.SetAxisTitle("ratio", "ratio");
        myPlot.AddTextBox(0.6, 0.65, GetPtString(ptBin));
        myPlot.AddLegendBox(0.7, 0.5);
        myPlot.AddTextBox(0.42, 0.92, datasetLable);
        plotEnv.CreateNewPlot(myPlot, "default ratio", false);
        plotEnv.SavePlot(plotName, dataSet, qaFolder + "/closureTestPtBins");
      }
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      for(int multBin = 2; multBin < 101; multBin++){
        if(!createBinWiseClosureTests) break;
        string plotName = "closureTestMultBin_" + std::to_string(multBin);
        Plot myPlot(plotName);
        myPlot.SetDrawingProperties("logX logY");
        myPlot.SetFigureGroup(dataSet);
        myPlot.AddHisto(string("multPtGeneratedMC_MultBin_") + std::to_string(multBin), "", "", kFullSquare, kBlack);
        myPlot.AddHisto(string("multPtMeasuredMC_MultBin_") + std::to_string(multBin), "", "measured", kFullCross, kGreen+3);
        myPlot.AddHisto(string("multPtGeneratedMC_MultBin_") + std::to_string(multBin), "", "generated", kFullSquare, kBlack);
        myPlot.AddHisto(string("multPtUnfoldedMC_MultBin_") + std::to_string(multBin), "", "unfolded", kFullCircle, kBlue+1);
        myPlot.AddRatio(string("multPtUnfoldedMC_MultBin_") + std::to_string(multBin), "", string("multPtGeneratedMC_MultBin_") + std::to_string(multBin));
        myPlot.SetAxisRange("ratio", 0.9, 1.1);
        myPlot.SetAxisTitle("ratio", "ratio");
        myPlot.AddTextBox(0.72, 0.9, "#it{N}_{ch} = " + std::to_string(multBin-1));
        myPlot.AddLegendBox(0.7, 0.7);
        myPlot.AddTextBox(0.16, 0.2, datasetLable);
        plotEnv.CreateNewPlot(myPlot, "default ratio", false);
        plotEnv.SavePlot(plotName, dataSet, qaFolder + "/closureTestMultBins");
      }
    } // -----------------------------------------------------------------------

    //--------------------------------------------------------------------------
    //----------------------- Systematics --------------------------------------
    //--------------------------------------------------------------------------


    { // -----------------------------------------------------------------------
      vector<string> histNames = {"multDistUnfolded", "momentUnfolded1", "varianceUnfolded"};//, "momentUnfolded2", "momentUnfolded3", "momentReweighted1", "momentReweighted2", "momentReweighted3"};
      Short_t colorsSyst[14]={kPink+8, kGreen+2, 28, kOrange+2 ,8 , kCyan-6, kMagenta+3, kRed, kBlue ,15, kGreen+4, kMagenta, 17, 12};
      for(string& histName : histNames)
      {
        string plotName = histName + "_SystContrib";
        Plot myPlot(plotName);
        myPlot.SetFigureGroup(dataSet);
        myPlot.SetDrawingProperties("thick");
        myPlot.AddHisto(histName + "_RelSyst_total", "", "", 0, kWhite, "hist");
        myPlot.AddHisto(histName + "_RelSyst_DCAtoVertexZ", "", " 1", 0, colorsSyst[0], "hist");
        myPlot.AddHisto(histName + "_RelSyst_DCAtoVertexXYPtDep", "", " 2", 0, colorsSyst[1], "hist");
        myPlot.AddHisto(histName + "_RelSyst_RatioCrossedRowsOverFindableClustersTPC", "", " 3", 0, colorsSyst[2], "hist");
        myPlot.AddHisto(histName + "_RelSyst_FractionSharedClustersTPC", "", " 4", 0, colorsSyst[3], "hist");
        myPlot.AddHisto(histName + "_RelSyst_Maxchi2perTPCclu", "", " 5", 0, colorsSyst[4], "hist");
        myPlot.AddHisto(histName + "_RelSyst_Maxchi2perITSclu", "", " 6", 0, colorsSyst[5], "hist");
        myPlot.AddHisto(histName + "_RelSyst_ClusterReqITS", "", " 7", 0, colorsSyst[6], "hist");
        myPlot.AddHisto(histName + "_RelSyst_DeadzoneWidth", "", " 8", 0, colorsSyst[7], "hist");
        myPlot.AddHisto(histName + "_RelSyst_Ncrnclgeomlength", "", " 9", 0, colorsSyst[8], "hist");
        myPlot.AddHisto(histName + "_RelSyst_MaxChi2TPCConstrained", "", "10", 0, colorsSyst[9], "hist");
        myPlot.AddHisto(histName + "_RelSyst_nonClosure", "", "11", 0, colorsSyst[11], "hist");
        myPlot.AddHisto(histName + "_RelSyst_total", "", "total", 0, kBlack, "hist");
        //myPlot.AddHisto(histName + "_RelSyst_ZVertexSelection", "", "11", 0, colorsSyst[10], "hist");
        myPlot.AddTextBox(0.39, 0.91, datasetLable);
        myPlot.AddLegendBox(0.15, 0.8, "", 2);
        myPlot.SetAxisRange("X", 0, multRange);
        //myPlot.SetAxisRange("Y", 0, 0.06);
        plotEnv.CreateNewPlot(myPlot, "default", false);
        plotEnv.SavePlot(plotName, dataSet, systematicsFolder);
      }
    } // -----------------------------------------------------------------------
  } //==========================================================================

  if(plotEnv.ContainsDatasets({"pp_5TeV", "pPb_5TeV", "PbPb_5TeV"}))
  { //==========================================================================
    string plotGroup = "system_comparison";
    string systemSizeLable = alice + newLine + chargedParticles + ", " + erg5TeV_NN + newLine + eta08 + ", " + ptRange;
    string systemSizeLablePrel = alicePrel + newLine + chargedParticles + ", " + erg5TeV_NN + newLine + eta08 + ", " + ptRange;
    vector<string> centrality = {"0-5%","5-10%","10-20%","20-30%","30-40%","40-50%","50-60%","60-70%","70-80%"};
    vector<double> nchCent = {2869.49, 2342.32, 1740.05, 1156.23, 750.512, 463.796, 265.249, 138.504, 64.0346};
    vector<double> nchCentErrors = {82.7027, 70.8452, 45.7133, 29.4205, 21.9877, 17.5017, 11.5956, 8.3419, 4.84894};
    //bool drawcent = false;
    bool drawcent = false;

    { // -----------------------------------------------------------------------
      string plotName = "meanPtFullRange";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.SetDrawingProperties("logX");
      myPlot.AddHisto("momentUnfolded1", "PbPb_5TeV", "", kFullCross, kRed+1, "", 3000);
      myPlot.AddHisto("momentUnfolded1", "pp_5TeV", "", kFullSquare, kBlue+1, "", 60);
      myPlot.AddHisto("momentUnfolded1_Syst", "pp_5TeV", "pp", kFullSquare, kBlue+1, "boxes", 60);
      myPlot.AddHisto("momentUnfolded1", "pPb_5TeV", "", kFullCircle, kMagenta+1, "", 110);
      myPlot.AddHisto("momentUnfolded1_Syst", "pPb_5TeV", "p-Pb", kFullCircle, kMagenta+1, "boxes", 110);
      myPlot.AddHisto("momentUnfolded1", "PbPb_5TeV", "", kFullCross, kRed+1, "", 3000);
      myPlot.AddHisto("momentUnfolded1_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes", 3000);
      //myPlot.AddGraph("meanPtPbPb_5TeV", "Publications", "Pb-Pb published", kFullCross, kGreen+2);
      myPlot.SetAxisRange("X", 0.1, 4000);
      myPlot.SetAxisRange("Y", 0.4, 0.85);
      myPlot.SetAxisTitle("X", "#it{N}_{ch}");

//      myPlot.AddLegendBox(0.2, 0.8);
      myPlot.AddLegendBox(0.2, 0.8);
      myPlot.AddTextBox(0.4, 0.3, systemSizeLablePrel);

      if(drawcent){
        for(int i = 0; i < 9; i++)
        {
          for(int j = 0; j < 6; j++)
          {
            //myPlot.AddTextBox(nchCent[i], 0.705 + 0.01*j, "#bf{|}", true);
          }
          myPlot.AddTextBox(nchCent[i], 0.69 - 0.02*i, (string("#bf{") + centrality[i] + string("}")).c_str(), true);
        }
      }
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "varianceFullRange";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.SetDrawingProperties("logX");
      myPlot.AddHisto("varianceUnfolded", "PbPb_5TeV", "", kFullCross, kRed+1, "", 3000);
      myPlot.AddHisto("varianceUnfolded", "pp_5TeV", "", kFullSquare, kBlue+1, "", 60);
      myPlot.AddHisto("varianceUnfolded_Syst", "pp_5TeV", "pp", kFullSquare, kBlue+1, "boxes", 60);
      myPlot.AddHisto("varianceUnfolded", "pPb_5TeV", "", kFullCircle, kMagenta+1, "", 100);
      myPlot.AddHisto("varianceUnfolded_Syst", "pPb_5TeV", "p-Pb", kFullCircle, kMagenta+1, "boxes", 100);
      myPlot.AddHisto("varianceUnfolded", "PbPb_5TeV", "", kFullCross, kRed+1, "", 3000);
      myPlot.AddHisto("varianceUnfolded_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes", 3000);
      myPlot.SetAxisRange("X", 0.1, 4000);
      myPlot.SetAxisRange("Y", 0.001, 0.75);
      myPlot.SetAxisTitle("X", "#it{N}_{ch}");
      myPlot.AddLegendBox(0.2, 0.8);
      myPlot.AddTextBox(0.4, 0.3, systemSizeLablePrel);

//      myPlot.AddTextBox(0.14, 0.92, systemSizeLablePrel);

      if(drawcent){
        for(int i = 0; i < 9; i++)
        {
          for(int j = 0; j < 6; j++)
          {
            myPlot.AddTextBox(nchCent[i], 0.36 + 0.01*j, "#bf{|}", true);
          }
          myPlot.AddTextBox(nchCent[i], 0.46 + 0.025*i, (string("#bf{") + centrality[i] + string("}")).c_str(), true);
        }
      }
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------


    { // -----------------------------------------------------------------------
      string plotName = "moment1FullRangeMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.SetDrawingProperties("logX");
      myPlot.AddHisto("momentGeneratedMC1", "PbPb_5TeV", "", kFullCross, kRed+1, "", 3000);
      myPlot.AddHisto("momentGeneratedMC1", "pp_5TeV", "pp (Pythia8)", kFullSquare, kBlue+1, "", 60);
      myPlot.AddHisto("momentGeneratedMC1", "pPb_5TeV", "p-Pb (DPMJET)", kFullCircle, kMagenta+1, "", 110);
      myPlot.AddHisto("momentGeneratedMC1", "PbPb_5TeV", "Pb-Pb (HIJING)", kFullCross, kRed+1, "", 3000);
      myPlot.SetAxisRange("X", 0.1, 4000);
      myPlot.SetAxisRange("Y", 0.4, 0.85);
      myPlot.AddLegendBox(0.6, 0.8);
      myPlot.AddTextBox(0.4, 0.29, systemSizeLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "multDensityFullRange";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.SetDrawingProperties("logY logX");
      myPlot.AddHisto("multDensityUnfolded", "PbPb_5TeV", "", kFullCross, kRed+1, "", 3000);
      myPlot.AddHisto("multDensityUnfolded", "pp_5TeV", "", kFullSquare, kBlue+1, "", 60);
      myPlot.AddHisto("multDensityUnfolded_Syst", "pp_5TeV", "pp", kFullSquare, kBlue+1, "boxes", 60);
      myPlot.AddHisto("multDensityUnfolded", "pPb_5TeV", "", kFullCircle, kMagenta+1, "", 110);
      myPlot.AddHisto("multDensityUnfolded_Syst", "pPb_5TeV", "p-Pb", kFullCircle, kMagenta+1, "boxes", 110);
      myPlot.AddHisto("multDensityGeneratedMC", "pPb_5TeV", "", kFullCircle, kMagenta+1, "", 110);
      myPlot.AddHisto("multDensityUnfolded", "PbPb_5TeV", "", kFullCross, kRed+1, "", 3000);
      myPlot.AddHisto("multDensityUnfolded_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes", 3000);

      for(int i = 0; i < 9; i++)
      {
        for(int j = 0; j < 6; j++)
        {
          myPlot.AddTextBox(nchCent[i], 1e-4 - 0.00001*j, "#bf{|}", true);
        }
//        myPlot.AddTextBox(nchCent[i], 1e-3 + 0.001*i, (string("#bf{") + centrality[i] + string("}")).c_str(), true);
      }
      myPlot.SetAxisRange("X", 0.1, 3000);
      myPlot.SetAxisRange("Y", 0.00001, 0.15);
      myPlot.AddLegendBox(0.2, 0.5);
      myPlot.AddTextBox(0.42, 0.92, systemSizeLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "moment1dNchdeta";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("momentUnfolded1dNchDeta", "pp_5TeV", "pp", 0, kBlue+1);
      myPlot.SetAxisRange("Y", 0.45, 0.85);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "varianceFullRangeMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.SetDrawingProperties("logX");
      myPlot.AddHisto("varianceGeneratedMC", "PbPb_5TeV", "", kFullCross, kRed+1, "", 3000);
      myPlot.AddHisto("varianceGeneratedMC", "pp_5TeV", "pp (Pythia8)", kFullSquare, kBlue+1, "", 50);
      myPlot.AddHisto("varianceGeneratedMC", "pPb_5TeV", "p-Pb (DPMJET)", kFullCircle, kMagenta+1, "", 85);
      myPlot.AddHisto("varianceGeneratedMC", "PbPb_5TeV", "Pb-Pb (HIJING)", kFullCross, kRed+1, "", 3000);
      myPlot.SetAxisRange("X", 0.1, 4000);
      myPlot.SetAxisRange("Y", 0.05, 0.8);
      myPlot.AddLegendBox(0.15, 0.6);
      myPlot.AddTextBox(0.14, 0.92, systemSizeLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------


    { // -----------------------------------------------------------------------
      string plotName = "meanPt";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("momentUnfolded1", "pp_5TeV", "", kFullSquare, kBlue+1, "", 60);
      myPlot.AddHisto("momentUnfolded1_Syst", "pp_5TeV", "pp", kFullSquare, kBlue+1, "boxes", 60);
      myPlot.AddHisto("meanPt_5.02TeV", "Simulations", "", kOpenSquare, kBlue+1, "band", 60);
      myPlot.AddHisto("momentUnfolded1", "pPb_5TeV", "", kFullCircle, kMagenta+1, "", 100);
      myPlot.AddHisto("momentUnfolded1_Syst", "pPb_5TeV", "p-Pb", kFullCircle, kMagenta+1, "boxes", 100);
      myPlot.AddHisto("meanPt_pPb_5.02TeV", "Simulations", "", kOpenCircle, kMagenta+1, "band", 100);
      myPlot.AddHisto("momentUnfolded1", "PbPb_5TeV", "", kFullCross, kRed+1, "", 100);
      myPlot.AddHisto("momentUnfolded1_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes", 100);
      myPlot.AddHisto("meanPt_PbPb_5.02TeV", "Simulations", "", kOpenCross, kRed+1, "band", 100);
      myPlot.SetAxisRange("Y", 0.45, 0.85);
      myPlot.AddLegendBox(0.2, 0.9, "");
      myPlot.SetAxisTitle("X", "#it{N}_{ch}");
      myPlot.AddTextBox(0.4, 0.3, systemSizeLablePrel);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "moment1WithMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("momentUnfolded1", "pp_5TeV", "", kFullSquare, kBlue+1, "", 60);
      myPlot.AddHisto("momentUnfolded1_Syst", "pp_5TeV", "pp", kFullSquare, kBlue+1, "boxes", 60);
      myPlot.AddHisto("momentGeneratedMC1", "pp_5TeV", "", 0, kBlue+1, "band", 60);
      myPlot.AddHisto("momentUnfolded1", "pPb_5TeV", "", kFullCircle, kMagenta+1, "", 100);
      myPlot.AddHisto("momentUnfolded1_Syst", "pPb_5TeV", "p-Pb", kFullCircle, kMagenta+1, "boxes", 100);
      myPlot.AddHisto("momentGeneratedMC1", "pPb_5TeV", "", 0, kMagenta+1, "band");
      myPlot.AddHisto("momentUnfolded1", "PbPb_5TeV", "", kFullCross, kRed+1, "", 100);
      myPlot.AddHisto("momentUnfolded1_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes", 100);
      myPlot.AddHisto("momentGeneratedMC1", "PbPb_5TeV", "", 0, kRed+1, "band");
      myPlot.SetAxisRange("Y", 0.45, 0.85);
      myPlot.AddLegendBox(0.2, 0.9, "");
      myPlot.SetAxisTitle("X", "#it{N}_{ch}");
      myPlot.AddTextBox(0.4, 0.3, systemSizeLablePrel);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "variance";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("varianceUnfolded", "pp_5TeV", "", kFullSquare, kBlue+1, "", 60);
      myPlot.AddHisto("varianceUnfolded_Syst", "pp_5TeV", "pp", kFullSquare, kBlue+1, "boxes", 60);
      myPlot.AddHisto("varianceUnfolded", "pPb_5TeV", "", kFullCircle, kMagenta+1, "", 100);
      myPlot.AddHisto("varianceUnfolded_Syst", "pPb_5TeV", "p-Pb", kFullCircle, kMagenta+1, "boxes", 100);
      myPlot.AddHisto("varianceUnfolded", "PbPb_5TeV", "", kFullCross, kRed+1, "", 100);
      myPlot.AddHisto("varianceUnfolded_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes", 100);
      myPlot.SetAxisRange("Y", 0.05, 0.8);
      myPlot.AddLegendBox(0.2, 0.9, "");
      myPlot.SetAxisTitle("X", "#it{N}_{ch}");
      myPlot.AddTextBox(0.4, 0.3, systemSizeLablePrel);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "varianceMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("varianceGeneratedMC", "pp_5TeV", "pp (Pythia8)", kFullSquare, kBlue+1, "", 60);
      myPlot.AddHisto("varianceGeneratedMC", "pPb_5TeV", "p-Pb (DPMJET)", kFullCircle, kMagenta+1, "", 100);
      myPlot.AddHisto("varianceGeneratedMC", "PbPb_5TeV", "Pb-Pb (HIJING)", kFullCross, kRed+1, "", 100);
      myPlot.SetAxisRange("Y", 0.05, 0.8);
      myPlot.AddLegendBox(0.2, 0.9, "");
      myPlot.AddTextBox(0.4, 0.3, systemSizeLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------


    { // -----------------------------------------------------------------------
      string plotName = "moment1UnfoldedVsReWeighted";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("momentReweighted1", "pp_5TeV", "", kFullSquare, kBlue+1, "", 60);
      myPlot.AddHisto("momentReweighted1_Syst", "pp_5TeV", "re-weighted", kFullSquare, kBlue+1, "boxes", 60);
      myPlot.AddHisto("momentUnfolded1", "pp_5TeV", "", kFullCross, kRed+1, "", 60);
      myPlot.AddHisto("momentUnfolded1_Syst", "pp_5TeV", "unfolded", kFullCross, kRed+1, "boxes", 60);
      myPlot.AddRatio("momentUnfolded1_Syst", "pp_5TeV", "momentReweighted1_Syst", "pp_5TeV", "", kFullCross, kRed+1, "boxes", 60);
      myPlot.SetAxisRange("X", 0., 60);
      myPlot.SetAxisRange("Y", 0.45, 0.85);
      myPlot.SetAxisRange("ratio", 0.9, 1.1);
      myPlot.AddLegendBox(0.2, 0.9, "");
      myPlot.AddTextBox(0.4, 0.3, systemSizeLable);
      plotEnv.CreateNewPlot(myPlot, "default ratio", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------


    { // -----------------------------------------------------------------------
      string plotName = "moment2";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("momentUnfolded2", "pp_5TeV", "", kFullSquare, kBlue+1, "", 60);
      myPlot.AddHisto("momentUnfolded2_Syst", "pp_5TeV", "pp", kFullSquare, kBlue+1, "boxes", 60);
      myPlot.AddHisto("momentUnfolded2", "pPb_5TeV", "", kFullCircle, kMagenta+1);
      myPlot.AddHisto("momentUnfolded2_Syst", "pPb_5TeV", "p-Pb", kFullCircle, kMagenta+1, "boxes");
      myPlot.AddHisto("momentUnfolded2", "PbPb_5TeV", "", kFullCross, kRed+1);
      myPlot.AddHisto("momentUnfolded2_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes");
      myPlot.AddHisto("momentUnfolded2_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes");
      myPlot.SetAxisRange("Y", 0.2, 1.5);
      myPlot.AddLegendBox(0.2, 0.9, "");
      myPlot.AddTextBox(0.4, 0.3, systemSizeLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "moment3";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("momentUnfolded3", "pp_5TeV", "", kFullSquare, kBlue+1, "", 60);
      myPlot.AddHisto("momentUnfolded3_Syst", "pp_5TeV", "pp", kFullSquare, kBlue+1, "boxes", 60);
      myPlot.AddHisto("momentUnfolded3", "pPb_5TeV", "", kFullCircle, kMagenta+1);
      myPlot.AddHisto("momentUnfolded3_Syst", "pPb_5TeV", "p-Pb", kFullCircle, kMagenta+1, "boxes");
      myPlot.AddHisto("momentUnfolded3", "PbPb_5TeV", "", kFullCross, kRed+1);
      myPlot.AddHisto("momentUnfolded3_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes");
      myPlot.SetAxisRange("Y", 0.2, 5);
      myPlot.AddLegendBox(0.6, 0.4, "");
      myPlot.AddTextBox(0.15, 0.9, systemSizeLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "moment1MC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("momentUnfoldedMC1", "pp_5TeV", "pp", 0, kBlue+1);
      myPlot.AddHisto("momentUnfoldedMC1", "pPb_5TeV", "p-Pb", 0, kMagenta+1);
      myPlot.AddHisto("momentUnfoldedMC1", "PbPb_5TeV", "Pb-Pb", 0, kRed+1);
      myPlot.SetAxisRange("Y", 0.45, 0.85);
      myPlot.AddLegendBox(0.6, 0.4, "");
      myPlot.AddTextBox(0.15, 0.9, systemSizeLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "moment1MCtruth";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("momentGeneratedMC1", "pp_5TeV", "pp (Pythia8)", 0, kBlue+1, "", 60);
      myPlot.AddHisto("momentGeneratedMC1", "pPb_5TeV", "p-Pb (DPMJET)", 0, kMagenta+1, "", 100);
      myPlot.AddHisto("momentGeneratedMC1", "PbPb_5TeV", "Pb-Pb (HIJING)", 0, kRed+1, "", 100);
      myPlot.AddLegendBox(0.15, 0.9, "");
      myPlot.AddTextBox(0.4, 0.3, systemSizeLable);
      myPlot.SetAxisRange("Y", 0.45, 0.85);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------
  } //==========================================================================



  if(plotEnv.ContainsDatasets({"pp_5TeV", "pp_7TeV", "pp_13TeV"}))
  { //==========================================================================
    string plotGroup = "energy_comparison";
    string energyLable = alice + newLine + chargedParticles + ", " + "pp collisions" + newLine + eta08 + ", " + ptRange;
    string energyLablePrel = alicePrel + newLine + chargedParticles + ", " + "pp collisions" + newLine + eta08 + ", " + ptRange;
    string energyLableMC = "#bf{Pythia8 Monash13}" + newLine + chargedParticles + ", " + "pp collisions" + newLine + eta08 + ", " + ptRange;


    { // -----------------------------------------------------------------------
      string plotName = "energyScanMean";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      vector<int> multBins = {2, 6, 9, 11, 16, 21, 26, 31, 36};
      for(auto& multBin : multBins)
      {
        myPlot.AddHisto((string("meanPt_") + std::to_string(multBin)).c_str(), "Energyscan", (string("Nch = ") + std::to_string(multBin-1)).c_str(), 0, 0, "boxes");
      }
      myPlot.SetAxisRange("Y", 0.3, 1.0);
      myPlot.AddLegendBox(0.24, 0.91, "", 3);
      myPlot.AddTextBox(0.4, 0.3, energyLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "energyScanMeanMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
//      myPlot.SetDrawingProperties("graphs");
      vector<int> multBins = {2, 6, 9, 11, 16, 21, 26, 31, 36};
      for(auto& multBin : multBins)
      {
        myPlot.AddHisto((string("meanPtMC_") + std::to_string(multBin)).c_str(), "Energyscan", "", 0, 0, "band");
//        myPlot.AddHisto((string("meanPtMC_") + std::to_string(multBin)).c_str(), "Energyscan", (string("Nch = ") + std::to_string(multBin-1)).c_str(), 0, 0, "boxes");
        myPlot.AddHisto((string("meanPt_") + std::to_string(multBin)).c_str(), "Energyscan", (string("Nch = ") + std::to_string(multBin-1)).c_str(), 0, -1, "boxes");
      }
      myPlot.SetAxisRange("Y", 0.3, 1.0);
      myPlot.AddLegendBox(0.24, 0.91, "", 3);
      //myPlot.AddTextBox(0.4, 0.3, energyLable);
      myPlot.AddTextBox(0.4, 0.3, energyLableMC);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "energyScanMeanMCLog";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
//      myPlot.SetDrawingProperties("graphs");
      vector<int> multBins = {2, 6, 9, 11, 16, 21, 26, 31, 36};
      for(auto& multBin : multBins)
      {
        myPlot.AddHisto((string("meanPtMC_Log_") + std::to_string(multBin)).c_str(), "Energyscan", "", 0, 0, "band");
//        myPlot.AddHisto((string("meanPtMC_") + std::to_string(multBin)).c_str(), "Energyscan", (string("Nch = ") + std::to_string(multBin-1)).c_str(), 0, 0, "boxes");
        myPlot.AddHisto((string("meanPt_Log_") + std::to_string(multBin)).c_str(), "Energyscan", (string("Nch = ") + std::to_string(multBin-1)).c_str(), 0, -1, "boxes");
      }
      myPlot.SetAxisRange("Y", 0.3, 1.0);
      myPlot.AddLegendBox(0.24, 0.91, "", 3);
      //myPlot.AddTextBox(0.4, 0.3, energyLable);
      myPlot.AddTextBox(0.4, 0.3, energyLableMC);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "energyScanVariance";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      vector<int> multBins = {2, 6, 9, 11, 16, 21, 26, 31, 36};
      for(auto& multBin : multBins)
      {
        myPlot.AddHisto((string("variance_") + std::to_string(multBin)).c_str(), "Energyscan", (string("Nch = ") + std::to_string(multBin-1)).c_str(), 0, 0, "boxes");
      }
      myPlot.SetAxisRange("Y", -0.4, 0.8);
      myPlot.AddLegendBox(0.24, 0.91, "", 3);
      myPlot.AddTextBox(0.4, 0.3, energyLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "energyScanVarianceMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      vector<int> multBins = {2, 6, 9, 11, 16, 21, 26, 31, 36};
      for(auto& multBin : multBins)
      {
        myPlot.AddHisto((string("varianceMC_") + std::to_string(multBin)).c_str(), "Energyscan", "", 0, 0, "band");
        myPlot.AddHisto((string("variance_") + std::to_string(multBin)).c_str(), "Energyscan", (string("Nch = ") + std::to_string(multBin-1)).c_str(), 0, -1, "boxes");
      }
      myPlot.SetAxisRange("Y", -0.4, 0.8);
      myPlot.AddLegendBox(0.24, 0.91, "", 3);
      //myPlot.AddTextBox(0.4, 0.3, energyLable);
      myPlot.AddTextBox(0.4, 0.3, energyLableMC);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "energyScanVarianceMCLog";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      vector<int> multBins = {2, 6, 9, 11, 16, 21, 26, 31, 36};
      for(auto& multBin : multBins)
      {
        myPlot.AddHisto((string("varianceMC_Log_") + std::to_string(multBin)).c_str(), "Energyscan", "", 0, 0, "band");
        myPlot.AddHisto((string("variance_Log_") + std::to_string(multBin)).c_str(), "Energyscan", (string("Nch = ") + std::to_string(multBin-1)).c_str(), 0, -1, "boxes");
      }
      myPlot.SetAxisRange("Y", -0.4, 1.4);
      myPlot.AddLegendBox(0.24, 0.91, "", 3);
      //myPlot.AddTextBox(0.4, 0.3, energyLable);
      myPlot.AddTextBox(0.4, 0.3, energyLableMC);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------



    { // -----------------------------------------------------------------------
      string plotName = "meanPt";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("momentUnfolded1", "pp_13TeV", "", kFullSquare, kRed+1, "", 60);
      myPlot.AddHisto("momentUnfolded1_Syst", "pp_13TeV", "13 TeV", kFullSquare, kRed+1, "boxes", 60);
      //myPlot.AddHisto("mbMeanPtMeanNchUnfolded", "pp_13TeV", "", kStar, kRed+1, "");
      myPlot.AddHisto("momentUnfolded1", "pp_7TeV", "", kFullCircle, kMagenta+1, "", 60);
      myPlot.AddHisto("momentUnfolded1_Syst", "pp_7TeV", "7 TeV", kFullCircle, kMagenta+1, "boxes", 60);
      //myPlot.AddHisto("mbMeanPtMeanNchUnfolded", "pp_7TeV", "", kStar, kMagenta+1, "");
      myPlot.AddHisto("momentUnfolded1", "pp_5TeV", "", kFullCross, kBlue+1, "", 60);
      myPlot.AddHisto("momentUnfolded1_Syst", "pp_5TeV", "5.02 TeV", kFullCross, kBlue+1, "boxes", 60);
      //myPlot.AddHisto("mbMeanPtMeanNchUnfolded", "pp_5TeV", "", kStar, kBlue+1, "");
      myPlot.AddHisto("momentUnfolded1", "pp_2TeV", "", kFullDiamond, kGreen+3, "", 40);
      myPlot.AddHisto("momentUnfolded1_Syst", "pp_2TeV", "2.76 TeV", kFullDiamond, kGreen+3, "boxes", 40);
      //myPlot.AddHisto("mbMeanPtMeanNchUnfolded", "pp_2TeV", "", kStar, kGreen+3, "");

      myPlot.SetAxisTitle("X", "#it{N}_{ch}");
      myPlot.SetAxisRange("X", 0, 60);
      myPlot.SetAxisRange("Y", 0.45, 0.85);
      myPlot.AddLegendBox(0.2, 0.9, "");
      myPlot.AddTextBox(0.4, 0.3, energyLablePrel);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "variance";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("varianceUnfolded", "pp_13TeV", "", kFullSquare, kRed+1, "", 60);
      myPlot.AddHisto("varianceUnfolded_Syst", "pp_13TeV", "13 TeV", kFullSquare, kRed+1, "boxes", 60);
      //myPlot.AddHisto("mbVarianceMeanNchUnfolded", "pp_13TeV_old", "", kStar, kRed+1, "");
      myPlot.AddHisto("varianceUnfolded", "pp_7TeV", "", kFullCircle, kMagenta+1, "", 60);
      myPlot.AddHisto("varianceUnfolded_Syst", "pp_7TeV", "7 TeV", kFullCircle, kMagenta+1, "boxes", 60);
      //myPlot.AddHisto("mbVarianceMeanNchUnfolded", "pp_7TeV", "", kStar, kMagenta+1, "");
      myPlot.AddHisto("varianceUnfolded", "pp_5TeV", "", kFullCross, kBlue+1, "", 60);
      myPlot.AddHisto("varianceUnfolded_Syst", "pp_5TeV", "5.02 TeV", kFullCross, kBlue+1, "boxes", 60);
      //myPlot.AddHisto("mbVarianceMeanNchUnfolded", "pp_5TeV", "", kStar, kBlue+1, "");
      myPlot.AddHisto("varianceUnfolded", "pp_2TeV", "", kFullDiamond, kGreen+3, "", 40);
      myPlot.AddHisto("varianceUnfolded_Syst", "pp_2TeV", "2.76 TeV", kFullDiamond, kGreen+3, "boxes", 40);
      //myPlot.AddHisto("mbVarianceMeanNchUnfolded", "pp_2TeV", "", kStar, kGreen+3, "");

      myPlot.SetAxisTitle("X", "#it{N}_{ch}");
      myPlot.SetAxisRange("X", 0, 60);
      myPlot.SetAxisRange("Y", 0.05, 0.75);
      myPlot.AddLegendBox(0.2, 0.9, "");
      myPlot.AddTextBox(0.4, 0.3, energyLablePrel);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------



    { // -----------------------------------------------------------------------
      string plotName = "moment1WithMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("meanPt_2.76TeV", "Simulations", "", kFullSquare, kGreen+3, "band", 40);
      myPlot.AddHisto("momentGeneratedMC1", "pp_5TeV", "", kFullSquare, kBlue+1, "band");
      //myPlot.AddHisto("meanPt_5.02TeV", "Simulations", "", kFullCircle, kBlue+1, "");
      myPlot.AddHisto("meanPt_7TeV", "Simulations", "", kFullCircle, kMagenta+1, "band");
      //myPlot.AddHisto("momentGeneratedMC1", "pp_7TeV", "", kFullCircle, kMagenta+1, "band");
      myPlot.AddHisto("momentGeneratedMC1", "pp_13TeV", "", kFullCross, kRed+1, "band");
      myPlot.AddHisto("meanPt_13TeV", "Simulations", "", kFullCircle, kRed+1, "band");

      myPlot.AddHisto("momentUnfolded1", "pp_2TeV", "", kFullSquare, kGreen+3, "", 40);
      myPlot.AddHisto("momentUnfolded1_Syst", "pp_2TeV", "2.76 TeV", kFullSquare, kGreen+3, "boxes", 40);
      //myPlot.AddHisto("mbMeanPtMeanNchUnfolded", "pp_2TeV", "", kStar, kGreen+3, "");
      myPlot.AddHisto("momentUnfolded1", "pp_5TeV", "", kFullSquare, kBlue+1);
      myPlot.AddHisto("momentUnfolded1_Syst", "pp_5TeV", "5.02 TeV", kFullSquare, kBlue+1, "boxes");
      //myPlot.AddHisto("mbMeanPtMeanNchUnfolded", "pp_5TeV", "", kStar, kBlue+1, "");
      myPlot.AddHisto("momentUnfolded1", "pp_7TeV", "", kFullCircle, kMagenta+1);
      myPlot.AddHisto("momentUnfolded1_Syst", "pp_7TeV", "7 TeV", kFullCircle, kMagenta+1, "boxes");
      //myPlot.AddHisto("mbMeanPtMeanNchUnfolded", "pp_7TeV", "", kStar, kMagenta+1, "");
      myPlot.AddHisto("momentUnfolded1", "pp_13TeV", "", kFullCross, kRed+1);
      myPlot.AddHisto("momentUnfolded1_Syst", "pp_13TeV", "13 TeV", kFullCross, kRed+1, "boxes");
      //myPlot.AddHisto("mbMeanPtMeanNchUnfolded", "pp_13TeV", "", kStar, kRed+1, "");
      myPlot.SetAxisTitle("X", "#it{N}_{ch}");

      myPlot.SetAxisRange("X", 0, 60);
      myPlot.SetAxisRange("Y", 0.45, 0.85);
      myPlot.AddLegendBox(0.2, 0.9, "");
      myPlot.AddTextBox(0.4, 0.3, energyLablePrel);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------


    { // -----------------------------------------------------------------------
      string plotName = "varianceMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("variance_2TeV", "Simulations", "2 TeV", 0, 0, "band");
      myPlot.AddHisto("variance_2.5TeV", "Simulations", "2.5 TeV", 0, 0, "band");
      myPlot.AddHisto("variance_3TeV", "Simulations", "3 TeV", 0, 0, "band");
      myPlot.AddHisto("variance_3.5TeV", "Simulations", "3.5 TeV", 0, 0, "band");
      myPlot.AddHisto("variance_4TeV", "Simulations", "4 TeV", 0, 0, "band");
      myPlot.AddHisto("variance_4.5TeV", "Simulations", "4.5 TeV", 0, 0, "band");
      myPlot.AddHisto("variance_5TeV", "Simulations", "5 TeV", 0, 0, "band");
      myPlot.AddHisto("variance_5.5TeV", "Simulations", "5.5 TeV", 0, 0, "band");
      myPlot.AddHisto("variance_6TeV", "Simulations", "6 TeV", 0, 0, "band");
      myPlot.AddHisto("variance_6.5TeV", "Simulations", "6.5 TeV", 0, 0, "band");
      myPlot.AddHisto("variance_7TeV", "Simulations", "7 TeV", 0, 0, "band");
      myPlot.AddHisto("variance_7.5TeV", "Simulations", "7.5 TeV", 0, 0, "band");
      myPlot.AddHisto("variance_8TeV", "Simulations", "8 TeV", 0, 0, "band");
      myPlot.AddHisto("variance_8.5TeV", "Simulations", "8.5 TeV", 0, 0, "band");
      myPlot.AddHisto("variance_9TeV", "Simulations", "9 TeV", 0, 0, "band");
      myPlot.AddHisto("variance_9.5TeV", "Simulations", "9.5 TeV", 0, 0, "band");
      myPlot.AddHisto("variance_10TeV", "Simulations", "10 TeV", 0, 0, "band");
      myPlot.AddHisto("variance_10.5TeV", "Simulations", "10.5 TeV", 0, 0, "band");
      myPlot.AddHisto("variance_11TeV", "Simulations", "11 TeV", 0, 0, "band");
      myPlot.AddHisto("variance_11.5TeV", "Simulations", "11.5 TeV", 0, 0, "band");
      myPlot.AddHisto("variance_12TeV", "Simulations", "12 TeV", 0, 0, "band");
      myPlot.AddHisto("variance_12.5TeV", "Simulations", "12.5 TeV", 0, 0, "band");
      myPlot.AddHisto("variance_13TeV", "Simulations", "13 TeV", 0, 0, "band");
      myPlot.AddHisto("variance_13.5TeV", "Simulations", "13.5 TeV", 0, 0, "band");
      myPlot.AddHisto("variance_14TeV", "Simulations", "14 TeV", 0, 0, "band");


      //myPlot.AddHisto("variance_2.76TeV", "Simulations", "2.76 TeV", kFullSquare, kGreen+3, "", 40);
      //myPlot.AddHisto("varianceGeneratedMC", "pp_5TeV", "5.02 TeV", kFullSquare, kBlue+1);
      //myPlot.AddHisto("variance_7TeV", "Simulations", "7 TeV", kFullCircle, kMagenta+1);
      //myPlot.AddHisto("varianceGeneratedMC", "pp_13TeV", "13 TeV", kFullCross, kRed+1);
      myPlot.SetAxisRange("X", 0, 40);
      myPlot.SetAxisRange("Y", 0.05, 0.9);
      myPlot.AddLegendBox(0.13, 0.9, "",4);
      myPlot.AddTextBox(0.4, 0.3, energyLableMC);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "moment1MC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("meanPt_2TeV", "Simulations", "2 TeV", 0, 0, "band");
      myPlot.AddHisto("meanPt_2.5TeV", "Simulations", "", 0, 0, "band");
      myPlot.AddHisto("meanPt_3TeV", "Simulations", "", 0, 0, "band");
      myPlot.AddHisto("meanPt_3.5TeV", "Simulations", "", 0, 0, "band");
      myPlot.AddHisto("meanPt_4TeV", "Simulations", "", 0, 0, "band");
      myPlot.AddHisto("meanPt_4.5TeV", "Simulations", "", 0, 0, "band");
      myPlot.AddHisto("meanPt_5TeV", "Simulations", "", 0, 0, "band");
      myPlot.AddHisto("meanPt_5.5TeV", "Simulations", "", 0, 0, "band");
      myPlot.AddHisto("meanPt_6TeV", "Simulations", "", 0, 0, "band");
      myPlot.AddHisto("meanPt_6.5TeV", "Simulations", "", 0, 0, "band");
      myPlot.AddHisto("meanPt_7TeV", "Simulations", "7 TeV", 0, 0, "band");
      myPlot.AddHisto("meanPt_7.5TeV", "Simulations", "", 0, 0, "band");
      myPlot.AddHisto("meanPt_8TeV", "Simulations", "", 0, 0, "band");
      myPlot.AddHisto("meanPt_8.5TeV", "Simulations", "", 0, 0, "band");
      myPlot.AddHisto("meanPt_9TeV", "Simulations", "", 0, 0, "band");
      myPlot.AddHisto("meanPt_9.5TeV", "Simulations", "", 0, 0, "band");
      myPlot.AddHisto("meanPt_10TeV", "Simulations", "", 0, 0, "band");
      myPlot.AddHisto("meanPt_10.5TeV", "Simulations", "", 0, 0, "band");
      myPlot.AddHisto("meanPt_11TeV", "Simulations", "", 0, 0, "band");
      myPlot.AddHisto("meanPt_11.5TeV", "Simulations", "", 0, 0, "band");
      myPlot.AddHisto("meanPt_12TeV", "Simulations", "", 0, 0, "band");
      myPlot.AddHisto("meanPt_12.5TeV", "Simulations", "", 0, 0, "band");
      myPlot.AddHisto("meanPt_13TeV", "Simulations", "", 0, 0, "band");
      myPlot.AddHisto("meanPt_13.5TeV", "Simulations", "", 0, 0, "band");
      myPlot.AddHisto("meanPt_14TeV", "Simulations", "14 TeV", 0, 0, "band");


      //myPlot.AddHisto("meanPt_2.76TeV", "Simulations", "2.76 TeV", kFullSquare, kGreen+3, "", 40);
      //myPlot.AddHisto("momentGeneratedMC1", "pp_5TeV", "5.02 TeV", kFullSquare, kBlue+1);
      //myPlot.AddHisto("meanPt_7TeV", "Simulations", "7 TeV", kFullCircle, kMagenta+1);
      //myPlot.AddHisto("momentGeneratedMC1", "pp_13TeV", "13 TeV", kFullCross, kRed+1);
      myPlot.SetAxisRange("X", 0, 60);
      myPlot.SetAxisRange("Y", 0.45, 0.85);
      myPlot.AddLegendBox(0.35, 0.3, "",4);
      myPlot.AddTextBox(0.4, 0.5, energyLableMC);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "mbMult";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("minBiasNchVsNch_7TeV", "Simulations", "7 TeV", kFullCircle, kMagenta+1);
      myPlot.AddHisto("minBiasNchVsNch_5TeV", "Simulations", "5.02 TeV", kFullSquare, kBlue+1);
      myPlot.AddHisto("minBiasNchVsNch_2TeV", "Simulations", "2.76 TeV", kFullSquare, kGreen+3);
      myPlot.SetAxisRange("X", 0, 60);
//      myPlot.SetAxisRange("Y", 0., 0.9);
      myPlot.AddLegendBox(0.2, 0.9, "");
      myPlot.AddTextBox(0.4, 0.3, energyLableMC);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "moment1MCGenerated";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("momentGeneratedMC1", "pp_2TeV", "2.76 TeV", kFullSquare, kGreen+3, "", 40);
      myPlot.AddHisto("momentGeneratedMC1", "pp_5TeV", "5.02 TeV", kFullSquare, kBlue+1, "");
      myPlot.AddHisto("momentGeneratedMC1", "pp_7TeV", "7 TeV", kFullCircle, kMagenta+1, "");
      myPlot.AddHisto("momentGeneratedMC1", "pp_13TeV", "13 TeV", kFullCross, kRed+1, "");
      myPlot.SetAxisRange("X", 0, 60);
      myPlot.SetAxisRange("Y", 0.45, 0.85);
      myPlot.AddLegendBox(0.6, 0.5, "");
      myPlot.AddTextBox(0.15, 0.92, energyLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "moment2";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("momentUnfolded2", "pp_2TeV", "", kFullSquare, kGreen+3, "", 40);
      myPlot.AddHisto("momentUnfolded2_Syst", "pp_2TeV", "2.76 TeV", kFullSquare, kGreen+3, "boxes", 40);
      myPlot.AddHisto("momentUnfolded2", "pp_5TeV", "", kFullSquare, kBlue+1);
      myPlot.AddHisto("momentUnfolded2_Syst", "pp_5TeV", "5.02 TeV", kFullSquare, kBlue+1, "boxes");
      myPlot.AddHisto("momentUnfolded2", "pp_7TeV", "", kFullCircle, kMagenta+1);
      myPlot.AddHisto("momentUnfolded2_Syst", "pp_7TeV", "7 TeV", kFullCircle, kMagenta+1, "boxes");
      myPlot.AddHisto("momentUnfolded2", "pp_13TeV", "", kFullCross, kRed+1);
      myPlot.AddHisto("momentUnfolded2_Syst", "pp_13TeV", "13 TeV", kFullCross, kRed+1, "boxes");
      myPlot.SetAxisRange("X", 0, 60);
      myPlot.SetAxisRange("Y", 0.2, 1.5);
      myPlot.AddLegendBox(0.2, 0.9, "");
      myPlot.AddTextBox(0.4, 0.3, energyLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "moment3";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("momentUnfolded3", "pp_2TeV", "", kFullSquare, kGreen+3, "", 40);
      myPlot.AddHisto("momentUnfolded3_Syst", "pp_2TeV", "2.76 TeV", kFullSquare, kGreen+3, "boxes", 40);
      myPlot.AddHisto("momentUnfolded3", "pp_5TeV", "", kFullSquare, kBlue+1);
      myPlot.AddHisto("momentUnfolded3_Syst", "pp_5TeV", "5.02 TeV", kFullSquare, kBlue+1, "boxes");
      myPlot.AddHisto("momentUnfolded3", "pp_7TeV", "", kFullCircle, kMagenta+1);
      myPlot.AddHisto("momentUnfolded3_Syst", "pp_7TeV", "7 TeV", kFullCircle, kMagenta+1, "boxes");
      myPlot.AddHisto("momentUnfolded3", "pp_13TeV", "", kFullCross, kRed+1);
      myPlot.AddHisto("momentUnfolded3_Syst", "pp_13TeV", "13 TeV", kFullCross, kRed+1, "boxes");
      myPlot.SetAxisRange("X", 0, 60);
      myPlot.SetAxisRange("Y", 0.1, 7.0);
      myPlot.AddLegendBox(0.6, 0.4, "");
      myPlot.AddTextBox(0.15, 0.92, energyLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "multDensity";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.SetDrawingProperties("logY");
      myPlot.AddHisto("multDensityUnfolded", "pp_2TeV", "", kFullDiamond, kGreen+3, "", 42);
      myPlot.AddHisto("multDensityUnfolded", "pp_13TeV", "", kFullSquare, kRed+1);
      myPlot.AddHisto("multDensityUnfolded_Syst", "pp_13TeV", "13 TeV", kFullSquare, kRed+1, "boxes");
      //myPlot.AddHisto("multDensityUnfolded", "pp_7TeV", "", kFullCircle, kMagenta+1);
      //myPlot.AddHisto("multDensityUnfolded_Syst", "pp_7TeV", "7 TeV", kFullCircle, kMagenta+1, "boxes");
      myPlot.AddHisto("multDist_pp_7TeV_Stat", "Publications", "", kFullCircle, kMagenta+1, "", 52);
      myPlot.AddHisto("multDist_pp_7TeV_Syst", "Publications", "7 TeV", kFullCircle, kMagenta+1, "boxes", 52);
      myPlot.AddHisto("multDensityUnfolded", "pp_5TeV", "", kFullCross, kBlue+1, "", 49);
      myPlot.AddHisto("multDensityUnfolded_Syst", "pp_5TeV", "5.02 TeV", kFullCross, kBlue+1, "boxes", 49);
      myPlot.AddHisto("multDensityUnfolded", "pp_2TeV", "", kFullDiamond, kGreen+3, "", 42);
      myPlot.AddHisto("multDensityUnfolded_Syst", "pp_2TeV", "2.76 TeV", kFullDiamond, kGreen+3, "boxes", 42);
      myPlot.AddHisto("multDist_pp_0.9TeV_Stat", "Publications", "", kFullStar, kBlack, "", 33);
      myPlot.AddHisto("multDist_pp_0.9TeV_Syst", "Publications", "0.9 TeV", kFullStar, kBlack, "boxes", 33);

      myPlot.SetAxisTitle("X", "#it{N}_{ch}");


      myPlot.SetAxisTitle("Y", "#it{P}(#it{N}_{ch})");
      myPlot.SetAxisRange("X", 0, 60);
      myPlot.SetAxisRange("Y", 1e-4, 3e-1);
      myPlot.AddLegendBox(0.18, 0.45);
      myPlot.AddTextBox(0.4, 0.9, energyLablePrel);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------
    { // -----------------------------------------------------------------------
      string plotName = "multDistKNO";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.SetDrawingProperties("logY");
      myPlot.AddHisto("multDistUnfoldedKNO", "pp_5TeV", "", kOpenCross, kBlue+1);
      myPlot.AddHisto("multDistUnfoldedKNO", "pp_13TeV", "", kOpenSquare, kRed+1);
      myPlot.AddHisto("multDistUnfoldedKNO_Syst", "pp_13TeV", "13 TeV", kOpenSquare, kRed+1, "boxes");
      myPlot.AddHisto("multDistUnfoldedKNO", "pp_7TeV", "", kOpenCircle, kMagenta+1);
      myPlot.AddHisto("multDistUnfoldedKNO_Syst", "pp_7TeV", "7 TeV", kOpenCircle, kMagenta+1, "boxes");
      //myPlot.AddHisto("multDist_pp_7TeVKNO_Stat", "Publications", "", kOpenCircle, kMagenta+1, "");
      //myPlot.AddHisto("multDist_pp_7TeVKNO_Syst", "Publications", "7 TeV", kOpenCircle, kMagenta+1, "boxes");
      myPlot.AddHisto("multDistUnfoldedKNO", "pp_5TeV", "", kOpenCross, kBlue+1);
      myPlot.AddHisto("multDistUnfoldedKNO_Syst", "pp_5TeV", "5.02 TeV", kOpenCross, kBlue+1, "boxes");
      myPlot.AddHisto("multDistUnfoldedKNO", "pp_2TeV", "", kOpenDiamond, kGreen+3);
      myPlot.AddHisto("multDistUnfoldedKNO_Syst", "pp_2TeV", "2.76 TeV", kOpenDiamond, kGreen+3, "boxes");
      myPlot.AddHisto("multDist_pp_0.9TeVKNO_Stat", "Publications", "", kOpenStar, kBlack, "");
      myPlot.AddHisto("multDist_pp_0.9TeVKNO_Syst", "Publications", "0.9 TeV", kOpenStar, kBlack, "boxes");
      myPlot.SetAxisRange("X", 0, 5);
      //myPlot.SetAxisTitle("X", "#it{N}_{ch}");

      myPlot.AddLegendBox(0.7, 0.9);
      myPlot.AddTextBox(0.15, 0.3, energyLablePrel);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "multDistKNORatio";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.SetDrawingProperties("logY");
      myPlot.AddHisto("multDistUnfoldedKNO", "pp_5TeV", "5.02 TeV", 0, kBlue+1);
      myPlot.AddHisto("multDistUnfoldedKNO", "pp_2TeV", "2.76 TeV", 0, kGreen+3);
      myPlot.AddHisto("multDistUnfoldedKNO", "pp_7TeV", "7 TeV", 0, kMagenta+1);
      myPlot.AddHisto("multDistUnfoldedKNO", "pp_13TeV", "13 TeV", 0, kRed+1);
      myPlot.AddRatio("multDistUnfoldedKNO", "pp_2TeV", "multDistUnfoldedKNO", "pp_5TeV", "", 0, kGreen+3);
      myPlot.AddRatio("multDistUnfoldedKNO", "pp_7TeV", "multDistUnfoldedKNO", "pp_5TeV", "", 0, kMagenta+1);
      myPlot.AddRatio("multDistUnfoldedKNO", "pp_13TeV", "multDistUnfoldedKNO", "pp_5TeV", "", 0, kRed+1);
      myPlot.SetAxisRange("ratio", 0.45, 1.55);
      myPlot.SetAxisTitle("ratio", "ratio to 5");
      myPlot.SetAxisRange("X", 0, 7);
      myPlot.AddLegendBox(0.7, 0.8);
      myPlot.AddTextBox(0.15, 0.3, energyLable);
      plotEnv.CreateNewPlot(myPlot, "default ratio", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------

  } //==========================================================================


  if(plotEnv.ContainsDatasets({"Publications"}))
  { //==========================================================================
    string publicationLable = alice + ", " + chargedParticles +  newLine + eta03 + ", " + ptRange10GeV + newLine + "#it{N}_{ch} cut: #it{p}_{T} > 0 GeV/#it{c}";
    string plotGroup = "system_comparison";
    { // -----------------------------------------------------------------------
      string plotName = "publication";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.SetDrawingProperties("logX");
      myPlot.AddHisto("meanPt_PbPb_2.76TeV_Stat", "Publications", "", kFullCross, kWhite);
      myPlot.AddHisto("meanPt_pp_7TeV_Stat", "Publications", "", kFullSquare, kBlue+1);
      myPlot.AddHisto("meanPt_pp_7TeV_Syst", "Publications", "pp #sqrt{s} = 7 TeV", kFullSquare, kBlue+1, "boxes");
      myPlot.AddHisto("meanPt_pPb_5.02TeV_Stat", "Publications", "", kFullCircle, kMagenta+1);
      myPlot.AddHisto("meanPt_pPb_5.02TeV_Syst", "Publications", "p-Pb #sqrt{#it{s}_{NN}} = 5.02 TeV", kFullCircle, kMagenta+1, "boxes");
      myPlot.AddHisto("meanPt_PbPb_2.76TeV_Stat", "Publications", "", kFullCross, kRed+1);
      myPlot.AddHisto("meanPt_PbPb_2.76TeV_Syst", "Publications", "Pb-Pb  #sqrt{#it{s}_{NN}} = 2.76 TeV", kFullCross, kRed+1, "boxes");
      myPlot.SetAxisRange("X", 0, 100);
      myPlot.SetAxisRange("Y", 0.45, 0.92);
//      myPlot.AddLegendBox(0.5, 0.92, "");
//      myPlot.AddTextBox(0.3, 0.3, publicationLable);
      myPlot.AddLegendBox(0.15, 0.7, "");
      myPlot.AddTextBox(0.15, 0.92, publicationLable);
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "publicationdNchdEta";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("meanPt_PbPb_2.76TeV_dNchdEta_Stat", "Publications", "", kFullCross, kWhite);
      myPlot.AddHisto("meanPt_pp_7TeV_dNchdEta_Stat", "Publications", "", kFullSquare, kBlue+1);
      myPlot.AddHisto("meanPt_pp_7TeV_dNchdEta_Syst", "Publications", "pp #sqrt{s} = 7 TeV", kFullSquare, kBlue+1, "boxes");
      myPlot.AddHisto("meanPt_pPb_5.02TeV_dNchdEta_Stat", "Publications", "", kFullCircle, kMagenta+1);
      myPlot.AddHisto("meanPt_pPb_5.02TeV_dNchdEta_Syst", "Publications", "p-Pb #sqrt{#it{s}_{NN}} = 5.02 TeV", kFullCircle, kMagenta+1, "boxes");
      myPlot.AddHisto("meanPt_PbPb_2.76TeV_dNchdEta_Stat", "Publications", "", kFullCross, kRed+1);
      myPlot.AddHisto("meanPt_PbPb_2.76TeV_dNchdEta_Syst", "Publications", "Pb-Pb #sqrt{#it{s}_{NN}} = 2.76 TeV", kFullCross, kRed+1, "boxes");

      myPlot.AddHisto("momentUnfolded1dNchDeta", "pp_5TeV", "pp", kOpenSquare, kBlue+1);
      myPlot.AddHisto("momentUnfolded1dNchDeta", "pPb_5TeV", "pPb", kOpenCircle, kMagenta+1);
      myPlot.AddHisto("momentUnfolded1dNchDeta", "PbPb_5TeV", "PbPb", kOpenCross, kRed+1);

//      myPlot.CropHisto("momentUnfolded1", "pp_5TeV", "X", 0, 40)
      myPlot.SetAxisRange("Y", 0.45, 0.92);
      myPlot.AddLegendBox(0.55, 0.3, "");
      myPlot.AddTextBox(0.5, 0.92, publicationLable);
    //  myPlot3.SetDrawingProperties("thick");
      plotEnv.CreateNewPlot(myPlot, "default", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------

    { // -----------------------------------------------------------------------
      string plotName = "multDistsComparisonPub";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.SetDrawingProperties("logY");
      myPlot.AddHisto("multDist_pp_7TeV_Stat", "Publications", "", kFullSquare, kMagenta+1);
      myPlot.AddHisto("multDist_pp_7TeV_Syst", "Publications", "preliminary (ALICE-PUBLIC-2013-001)", kFullSquare, kMagenta+1, "boxes");
      myPlot.AddHisto("multDensityUnfolded", "pp_7TeV", "", kOpenCircle, kBlue+1);
      myPlot.AddHisto("multDensityUnfolded_Syst", "pp_7TeV", "this work", kOpenCircle, kBlue+1, "boxes");
      myPlot.AddHisto("multDensityGeneratedMC", "pp_7TeV", "Pythia6", kOpenCross, kGray+2);
//      myPlot.AddHisto("multDensityGeneratedMC", "Simulations", "Pythia8", kOpenCross, kGray+2);
      myPlot.AddRatio("multDensityUnfolded_Syst", "pp_7TeV", "multDist_pp_7TeV_Syst", "Publications", "", kOpenCircle, kBlue+1, "boxes");
//      myPlot.AddRatio("multDensityUnfolded", "pp_7TeV", "multDist_pp_7TeV_Stat", "Publications");
      myPlot.SetAxisRange("X", 0, 60);
      myPlot.SetAxisRange("ratio", 0.75, 1.25);
      myPlot.AddLegendBox(0.2, 0.25);
      string energyLable = alice + newLine + chargedParticles + ", " + "pp collisions" + newLine + eta08 + ", " + ptRange;
      myPlot.AddTextBox(0.42, 0.92, energyLable);
      plotEnv.CreateNewPlot(myPlot, "default ratio", false);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -----------------------------------------------------------------------


  } //==========================================================================

  { // -----------------------------------------------------------------------
    string plotName = "multDistMeasured13TeV";
    Plot myPlot(plotName);
    myPlot.SetFigureGroup("QA");
    myPlot.SetDrawingProperties("logY");
  //  myPlot.SetDrawingProperties("logZ");
  myPlot.AddHisto("multDistMeasured", "pp_13TeV_old", "event selection before", kFullSquare, kRed+1);
  myPlot.AddHisto("multDistMeasuredMC", "pp_13TeV_old", "", kOpenSquare, kRed+1);
  myPlot.AddHisto("multDistMeasured", "pp_13TeV", "AilEventCuts and PhysSel plieup rejection", kFullCircle, kBlue+1);
  myPlot.AddHisto("multDistMeasuredMC", "pp_13TeV", "", kOpenCircle, kBlue+1);
    myPlot.AddLegendBox(0.3, 0.9);
    myPlot.SetAxisRange("X", 0, 60);
  //  myPlot.SetAxisRange("Y", 0.1, 1e6);
    plotEnv.CreateNewPlot(myPlot, "default", false);
    plotEnv.SavePlot(plotName, "QA");
  } // -----------------------------------------------------------------------

  { // -----------------------------------------------------------------------
    string plotName = "scaleEffectDDCMeas";
    Plot myPlot(plotName);
    myPlot.SetFigureGroup("QA");
    myPlot.SetDrawingProperties("logZ");
    myPlot.AddHisto("scaleEffectMultMeas", "pp_13TeV_withDDC", "");
    myPlot.SetAxisTitle("Y", "#it{N}^{MC}_{acc}");
    myPlot.SetAxisTitle("X", "#it{N}^{scaled}_{acc}");
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0, 60);
    myPlot.AddLegendBox(0.2, 0.9, "");
    plotEnv.CreateNewPlot(myPlot, "default", false);
    plotEnv.SavePlot(plotName, "QA");
  } // -----------------------------------------------------------------------

  { // -----------------------------------------------------------------------
    string plotName = "scaleEffectDDCTrue";
    Plot myPlot(plotName);
    myPlot.SetFigureGroup("QA");
    myPlot.SetDrawingProperties("logZ");
    myPlot.AddHisto("scaleEffectMultTrue", "pp_13TeV_withDDC", "");
    myPlot.SetAxisTitle("Y", "#it{N}^{MC}_{ch}");
    myPlot.SetAxisTitle("X", "#it{N}^{scaled}_{ch}");

    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0, 60);
    myPlot.AddLegendBox(0.2, 0.9, "");
    plotEnv.CreateNewPlot(myPlot, "default", false);
    plotEnv.SavePlot(plotName, "QA");
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multDist_withDDC";
    Plot myPlot(plotName);
    myPlot.SetFigureGroup("QA");
    myPlot.SetDrawingProperties("logY");
    myPlot.AddHisto("multDistUnfolded_Syst", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.AddHisto("multDistUnfolded", "pp_13TeV", "", kFullSquare, kBlue+1, "", 60);
    myPlot.AddHisto("multDistUnfolded", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    myPlot.AddRatio("multDistUnfolded_Syst", "pp_13TeV", "multDistUnfolded", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.SetAxisRange("X", 0, 60);
//    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.AddTextBox(0.2, 0.25, "data unfolded");
    myPlot.SetAxisRange("ratio", 0.8, 1.2);
    myPlot.AddLegendBox(0.4, 0.9, "");
    plotEnv.CreateNewPlot(myPlot, "default ratio", false);
    plotEnv.SavePlot(plotName, "QA");
  } // -----------------------------------------------------------------------

  { // -----------------------------------------------------------------------
    string plotName = "multDistKNO_withDDC";
    Plot myPlot(plotName);
    myPlot.SetFigureGroup("QA");
    myPlot.SetDrawingProperties("logY");
    myPlot.AddHisto("multDistUnfoldedKNO_Syst", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.AddHisto("multDistUnfoldedKNO", "pp_13TeV", "", kFullSquare, kBlue+1, "", 60);
    myPlot.AddHisto("multDistUnfoldedKNO", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    myPlot.AddRatio("multDistUnfoldedKNO_Syst", "pp_13TeV", "multDistUnfoldedKNO", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.SetAxisRange("X", 0, 60);
//    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.AddTextBox(0.2, 0.25, "data unfolded");
    myPlot.SetAxisRange("ratio", 0.8, 1.2);
    myPlot.AddLegendBox(0.4, 0.9, "");
    plotEnv.CreateNewPlot(myPlot, "default ratio", false);
    plotEnv.SavePlot(plotName, "QA");
  } // -----------------------------------------------------------------------

  { // -----------------------------------------------------------------------
    string plotName = "multDensity_withDDC";
    Plot myPlot(plotName);
    myPlot.SetFigureGroup("QA");
    myPlot.SetDrawingProperties("logY");
    myPlot.AddHisto("multDensityUnfolded_Syst", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.AddHisto("multDensityUnfolded", "pp_13TeV", "", kFullSquare, kBlue+1, "", 60);
    myPlot.AddHisto("multDensityUnfolded", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    myPlot.AddRatio("multDensityUnfolded_Syst", "pp_13TeV", "multDensityUnfolded", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.SetAxisRange("X", 0, 60);
//    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.AddTextBox(0.2, 0.25, "data unfolded");
    myPlot.SetAxisRange("ratio", 0.8, 1.2);
    myPlot.AddLegendBox(0.4, 0.9, "");
    plotEnv.CreateNewPlot(myPlot, "default ratio", false);
    plotEnv.SavePlot(plotName, "QA");
  } // -----------------------------------------------------------------------

  { // -----------------------------------------------------------------------
    string plotName = "multDistMeasuredMC_withDDC";
    Plot myPlot(plotName);
    myPlot.SetFigureGroup("QA");
    myPlot.SetDrawingProperties("logY");
    myPlot.AddHisto("multDistMeasuredMC", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "", 60);
    myPlot.AddHisto("multDistMeasuredMC", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    myPlot.AddRatio("multDistMeasuredMC", "pp_13TeV", "multDistMeasuredMC", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "", 60);
    myPlot.SetAxisRange("X", 0, 60);
//    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.AddTextBox(0.2, 0.25, "MC measured");
    myPlot.SetAxisRange("ratio", 0.8, 1.2);
    myPlot.AddLegendBox(0.4, 0.9, "");
    plotEnv.CreateNewPlot(myPlot, "default ratio", false);
    plotEnv.SavePlot(plotName, "QA");
  } // -----------------------------------------------------------------------

  { // -----------------------------------------------------------------------
    string plotName = "multDistGeneratedMC_withDDC";
    Plot myPlot(plotName);
    myPlot.SetFigureGroup("QA");
    myPlot.SetDrawingProperties("logY");
    myPlot.AddHisto("multDistGeneratedMC", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "", 60);
    myPlot.AddHisto("multDistGeneratedMC", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    myPlot.AddRatio("multDistGeneratedMC", "pp_13TeV", "multDistMeasuredMC", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "", 60);
    myPlot.SetAxisRange("X", 0, 60);
//    myPlot.SetAxisRange("Y", 0.45, 0.85);
//    myPlot.SetAxisRange("ratio", 0.8, 1.2);
    myPlot.AddTextBox(0.2, 0.25, "MC truth");
    myPlot.AddLegendBox(0.4, 0.9, "");
    plotEnv.CreateNewPlot(myPlot, "default ratio", false);
    plotEnv.SavePlot(plotName, "QA");
  } // -----------------------------------------------------------------------

  { // -----------------------------------------------------------------------
    string plotName = "multCorrelationMatrix_noDDC";
    Plot myPlot(plotName);
    myPlot.SetFigureGroup("QA");
    myPlot.SetDrawingProperties("logZ");
    myPlot.AddHisto("multCorrelationMatrix", "pp_13TeV");
    myPlot.AddTextBox(0.4, 0.25, "no data-driven corrections");
    plotEnv.CreateNewPlot(myPlot, "default", false);
    plotEnv.SavePlot(plotName, "QA");
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "multCorrelationMatrix_withDDC";
    Plot myPlot(plotName);
    myPlot.SetFigureGroup("QA");
    myPlot.SetDrawingProperties("logZ");
    myPlot.AddHisto("multCorrelationMatrix", "pp_13TeV_withDDC");
    myPlot.AddTextBox(0.4, 0.25, "with data-driven corrections");
    plotEnv.CreateNewPlot(myPlot, "default", false);
    plotEnv.SavePlot(plotName, "QA");
  } // -----------------------------------------------------------------------

  { // -----------------------------------------------------------------------
    string plotName = "responseMatrix_Mean";
    Plot myPlot(plotName);
    myPlot.SetFigureGroup("QA");
    myPlot.AddHisto("mean", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1);
    myPlot.AddHisto("mean", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1);
    myPlot.SetAxisTitle("Y", "<#it{N}_{acc}>");
    myPlot.SetAxisRange("Y", 0, 45);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("ratio", 0.95, 1.05);
    myPlot.AddRatio("mean", "pp_13TeV", "mean", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1);
    myPlot.AddLegendBox(0.2, 0.9);
    plotEnv.CreateNewPlot(myPlot, "default ratio", false);
    plotEnv.SavePlot(plotName, "QA");
  } // -----------------------------------------------------------------------

  { // -----------------------------------------------------------------------
    string plotName = "responseMatrix_Sigma";
    Plot myPlot(plotName);
    myPlot.SetFigureGroup("QA");
    myPlot.AddHisto("sigma", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1);
    myPlot.AddHisto("sigma", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1);
    myPlot.SetAxisTitle("Y", "#sigma(#it{N}_{acc})");
    myPlot.SetAxisRange("Y", 0, 5);
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("ratio", 0.5, 1.5);
    myPlot.AddRatio("sigma", "pp_13TeV", "sigma", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1);
    myPlot.AddLegendBox(0.2, 0.9);
    plotEnv.CreateNewPlot(myPlot, "default ratio", false);
    plotEnv.SavePlot(plotName, "QA");
  } // -----------------------------------------------------------------------

  { // -----------------------------------------------------------------------
    string plotName = "responseMatrix_noDDC";
    Plot myPlot(plotName);
    myPlot.SetFigureGroup("QA");
    myPlot.SetDrawingProperties("logZ");
    myPlot.AddHisto("responseMatrix", "pp_13TeV");
    myPlot.AddTextBox(0.4, 0.25, "no data-driven corrections");
    plotEnv.CreateNewPlot(myPlot, "default", false);
    plotEnv.SavePlot(plotName, "QA");
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "responseMatrix_withDDC";
    Plot myPlot(plotName);
    myPlot.SetFigureGroup("QA");
    myPlot.SetDrawingProperties("logZ");
    myPlot.AddHisto("responseMatrix", "pp_13TeV_withDDC");
    myPlot.AddTextBox(0.4, 0.25, "with data-driven corrections");
    plotEnv.CreateNewPlot(myPlot, "default", false);
    plotEnv.SavePlot(plotName, "QA");
  } // -----------------------------------------------------------------------

  { // -----------------------------------------------------------------------
    string plotName = "moment1_withDDC";
    Plot myPlot(plotName);
    myPlot.SetFigureGroup("QA");
    myPlot.AddHisto("momentUnfolded1", "pp_13TeV", "", kFullSquare, kBlue+1, "", 60);
    myPlot.AddHisto("momentUnfolded1_Syst", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.AddHisto("momentUnfolded1", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    myPlot.AddRatio("momentUnfolded1_Syst", "pp_13TeV", "momentUnfolded1", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "boxes", 60);

    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.SetAxisRange("ratio", 0.99, 1.01);
    myPlot.AddLegendBox(0.2, 0.9, "");
    plotEnv.CreateNewPlot(myPlot, "default ratio", false);
    plotEnv.SavePlot(plotName, "QA");
  } // -----------------------------------------------------------------------
  { // -----------------------------------------------------------------------
    string plotName = "variance_withDDC";
    Plot myPlot(plotName);
    myPlot.SetFigureGroup("QA");
    myPlot.AddHisto("varianceUnfolded", "pp_13TeV", "", kFullSquare, kBlue+1, "", 60);
    myPlot.AddHisto("varianceUnfolded_Syst", "pp_13TeV", "without PCC and SecScaling", kFullSquare, kBlue+1, "boxes", 60);
    myPlot.AddHisto("varianceUnfolded", "pp_13TeV_withDDC", "with PCC and SecScaling", kFullSquare, kRed+1, "", 60);
    myPlot.AddRatio("varianceUnfolded_Syst", "pp_13TeV", "varianceUnfolded", "pp_13TeV_withDDC", "", kFullSquare, kBlue+1, "boxes", 60);

    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.05, 0.9);
    myPlot.SetAxisRange("ratio", 0.95, 1.05);
    myPlot.AddLegendBox(0.2, 0.9, "");
    plotEnv.CreateNewPlot(myPlot, "default ratio", false);
    plotEnv.SavePlot(plotName, "QA");
  } // -----------------------------------------------------------------------


  
  plotEnv.PrintErrors();
}


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
