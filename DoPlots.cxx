
#include "AliMultDepSpecPlottingFramework.h"
#include "Plot.h"

// TODO: add feature to also add external histogram in plots
/// This is the Plotting user anchorpoint
int main(int argc, char *argv[]) {

  string outputPath = "../../../../TempPlots";
  string outputFileName = "myPlots.root";
  Bool_t datasetStudy = kFALSE;

  vector<string> dataSets;
//  dataSets.push_back("pp_5TeV_10GeV_CENT");
  dataSets.push_back("pPb_5TeV_2013");
  dataSets.push_back("pp_5TeV_2017");
  dataSets.push_back("pp_7TeV");
//  dataSets.push_back("pp_5TeV_10GeV_CENTwoSDD");
//  dataSets.push_back("pp_5TeV_10GeV_FAST");
  dataSets.push_back("PbPb_5TeV");
  dataSets.push_back("Publications");


  // create plotting environment
  AliMultDepSpecPlottingFramework plotEnv;
  plotEnv.SetOutputDirectory(outputPath);
//  plotEnv.SetDrawTimestamps(kTRUE);
//plotEnv.SetPalette(kRainBow);//kBlueGreenYellow, kDarkRainBow

  for(string dataSet : dataSets){
    string folder = "../../../Datasets/" + dataSet + "/";
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

  //---- Lable definitions ----
  string newLine = " // ";
  string alice = "#bf{ALICE work in progress}";
  string chargedParticles = "charged particles";
  string erg5TeV = "#sqrt{#it{s}} = 5.02 TeV";
  string eta08 = "|#eta| < 0.8";
  string ptRange = "0.15 GeV/#it{c} < #it{p}_{T} < 50 GeV/#it{c}";
  string lable5TeV = alice + newLine + chargedParticles + ", " + erg5TeV + newLine + eta08 + ", " + ptRange;
  //---------------------------


  for(string dataSet : dataSets)
  {
    if(dataSet == "Publications") continue;
    string colsys = "pp";
    string lable = alice + newLine + colsys + ", " + chargedParticles + ", " + erg5TeV + newLine + eta08 + ", " + ptRange;;
    { // -------------------------------------------------------------------------
      string plotName = "efficiencyTestVsMult";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      vector<Int_t> ptBins = {3, 5, 7, 9, 11, 13, 15, 17};
      for(Int_t ptBin : ptBins)
      {
        string name = "efficiencyTest_" + std::to_string(ptBin);
        myPlot.AddHisto(name, dataSet, std::to_string(ptBin));
      }
//      myPlot.AddLegendBox(3.4, 5.6, "My funny text");
//      myPlot.SetAxisRange("X", 0., 100);
      myPlot.SetAxisRange("Y", 0.5, 0.8);
      myPlot.SetAxisTitle("ratio", "wo / with");
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName, dataSet);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "closureTestMoment1";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto("momentGeneratedMC1", "", "truth (PYTHIA8 Monash-2013)");
      myPlot.AddHisto("momentUnfoldedMC1", "", "unfolding method");
      myPlot.AddHisto("momentReweightedMC1", "", "re-weighting method");
      myPlot.AddRatio("momentUnfoldedMC1", "", "momentGeneratedMC1");
      myPlot.AddRatio("momentReweightedMC1", "", "momentGeneratedMC1");
//      myPlot.SetAxisRange("X", 0, 60);
//      myPlot.SetAxisRange("Y", 0.45, 0.85);
      myPlot.SetAxisRange("ratio", 0.95, 1.05);
//      myPlot.SetAxisRange("ratio", 0.8, 1.2);
      myPlot.AddTextBox(3.4, 5.6, "My funny text");
      myPlot.SetAxisTitle("ratio", "ratio");
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, dataSet);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "closureTestMoment2";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto("momentGeneratedMC2", "", "truth (PYTHIA8 Monash-2013)");
      myPlot.AddHisto("momentUnfoldedMC2", "", "unfolding method"); // uses figure grou name as identifier if empty
      myPlot.AddHisto("momentReweightedMC2", "", "re-weighting method");
      myPlot.AddRatio("momentUnfoldedMC2", "", "momentGeneratedMC2");
      myPlot.AddRatio("momentReweightedMC2", "", "momentGeneratedMC2");
  //    myPlot.SetAxisRange("X", 0, 60);
  //    myPlot.SetAxisRange("Y", 0.45, 0.85);
  //  myPlot.SetAxisRange("ratio", 0.95, 1.05);
      myPlot.SetAxisRange("ratio", 0.8, 1.2);
      myPlot.AddTextBox(3.4, 5.6, "My funny text");
      myPlot.SetAxisTitle("ratio", "ratio");
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, dataSet);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "closureTestMoment3";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto("momentGeneratedMC3", "", "truth (PYTHIA8 Monash-2013)");
      myPlot.AddHisto("momentUnfoldedMC3", "", "unfolding method"); // uses figure grou name as identifier if empty
      myPlot.AddHisto("momentReweightedMC3", "", "re-weighting method");
      myPlot.AddRatio("momentUnfoldedMC3", "", "momentGeneratedMC3");
      myPlot.AddRatio("momentReweightedMC3", "", "momentGeneratedMC3");
//      myPlot.SetAxisRange("X", 0, 60);
  //    myPlot.SetAxisRange("Y", 0.45, 0.85);
  //  myPlot.SetAxisRange("ratio", 0.95, 1.05);
      myPlot.SetAxisRange("ratio", 0.8, 1.2);
      myPlot.AddTextBox(3.4, 5.6, "My funny text");
      myPlot.SetAxisTitle("ratio", "ratio");
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, dataSet);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "closureTestMultDist";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto("multDistGeneratedClosureTest", "", "generated");
      myPlot.AddHisto("multDistUnfoldedClosureTest", "", "unfolded"); // uses figure grou name as identifier if empty
      myPlot.AddHisto("multDistMeasuredClosureTest", "", "mesured");
      myPlot.AddRatio("multDistUnfoldedClosureTest", "", "multDistGeneratedClosureTest", "", "");
      myPlot.SetAxisTitle("ratio", "ratio");
      myPlot.SetDrawingProperties("logY");
      myPlot.AddLegendBox(0.7, 0.9, "");
      myPlot.SetAxisRange("ratio", 0.95, 1.05);
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, dataSet);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "closureTestMultDistTracks";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto("multDistTracksGeneratedClosureTest", "", "generated");
      myPlot.AddHisto("multDistTracksUnfoldedClosureTest", "", "unfolded"); // uses figure grou name as identifier if empty
      myPlot.AddHisto("multDistTracksMeasuredClosureTest", "", "mesured");
      myPlot.AddHisto("multDistTracksInitialClosureTest", "", "initial");
      myPlot.AddRatio("multDistTracksUnfoldedClosureTest", "", "multDistTracksGeneratedClosureTest", "", "");
      myPlot.SetAxisTitle("ratio", "ratio");
      myPlot.SetDrawingProperties("logY");
      myPlot.AddLegendBox(0.7, 0.9, "#sqrt{#it{s}_{NN}} = 5.02 TeV");
      myPlot.SetAxisRange("ratio", 0.95, 1.05);
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, dataSet);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "responseMatrix";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logZ");
      myPlot.AddHisto(plotName);
//      myPlot.SetAxisRange("X", 0, 60);
//      myPlot.SetAxisRange("Y", 0, 60);
      myPlot.AddTextBox(0.3, 0.3, lable);
      myPlot.SetAxisTitle("X", "#it{N}^{ meas}_{ch}");
      myPlot.SetAxisTitle("Y", "#it{N}^{ true}_{ch}");
//      myPlot.SetAxisTitle("Z", "#it{P}(#it{N}_{acc} | #it{N}_{ch})");
      myPlot.SetAxisTitle("Z", "#it{P}(#it{N}^{ meas}_{ch} | #it{N}^{ true}_{ch})");
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName, dataSet);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "ptResolutionMatrix";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logX logY logZ");
      myPlot.SetAxisRange("X", 0.15, 50);
      myPlot.SetAxisRange("Y", 0.15, 50);
      myPlot.SetAxisTitle("X", "#it{p}^{ meas}_{T}");
      myPlot.SetAxisTitle("Y", "#it{p}^{ true}_{T}");
      myPlot.AddTextBox(0.3, 0.3, lable);

      myPlot.AddHisto(plotName);
      myPlot.SetAxisTitle("Z", "#it{P}(#it{p}^{ meas}_{T} | #it{p}^{ true}_{T})");
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName, dataSet);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "relPtResoFromCovData";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logZ logX");
      myPlot.SetAxisRange("X", 0.15, 50);
      myPlot.SetAxisRange("Z", 2, 5e7);
      myPlot.SetAxisTitle("Z", "# Tracks");
      myPlot.AddTextBox(0.3, 0.9, lable);
      myPlot.AddHisto(plotName);
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName, dataSet);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "relPtResoFromCovMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logZ logX");
      myPlot.SetAxisRange("X", 0.15, 50);
      myPlot.SetAxisTitle("Z", "# Tracks");
      myPlot.AddHisto(plotName);
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName, dataSet);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "comparisonResoDataMC";
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
      myPlot.AddTextBox(0.3, 0.9, lable);
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, dataSet);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "crosscheckPt";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logX logY");
  //    myPlot.SetAxisTitle("Y", "#it{p}_{T} (GeV/#it{c})");
      myPlot.SetAxisRange("X", 0.15, 50);
      myPlot.AddHisto("trueParticlesPt", "", "true");
      myPlot.AddHisto("unfoParticlesPt", "", "unfolded");
  //    myPlot.AddHisto("corrParticlesPt", "", "corrected");
  //    myPlot.AddHisto("recoParticlesPt", "", "reconstructed");
  //    myPlot.AddRatio("corrParticlesPt", "", "trueParticlesPt");
      myPlot.AddRatio("unfoParticlesPt", "", "trueParticlesPt");
      myPlot.SetAxisRange("ratio", 0.9, 1.1);
      myPlot.SetAxisTitle("ratio", "unf / true");
      myPlot.AddLegendBox(0.2, 0.5, "");
      myPlot.AddTextBox(0.48, 0.95, lable5TeV);
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, dataSet);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "crosscheckMult";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto("trueParticlesMult", "", "true");
      myPlot.AddHisto("unfoParticlesMult", "", "unfolded");
  //    myPlot.AddHisto("corrParticlesMult", "", "corrected");
  //    myPlot.AddHisto("recoParticlesMult", "", "reconstructed");
      myPlot.AddRatio("unfoParticlesMult", "", "trueParticlesMult");
      myPlot.SetAxisTitle("ratio", "unf / true");
      myPlot.SetDrawingProperties("logY");
      myPlot.AddLegendBox(0.7, 0.9, "#sqrt{#it{s}_{NN}} = 5.02 TeV");
      myPlot.SetAxisRange("ratio", 0.95, 1.05);
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, dataSet);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "meanTrueMult";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto("responseMatrixOrig");
      myPlot.AddHisto(plotName);
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName, dataSet);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "multPtMeasured";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logZ logY");
      myPlot.AddHisto(plotName);
      myPlot.SetAxisRange("X", 0, 60);
      myPlot.SetAxisRange("Y", 0.15, 50.0);
      myPlot.SetAxisRange("Z", 1e-15, 5);
      myPlot.SetAxisTitle("X", "#it{N}^{ meas}_{ch}");
      myPlot.SetAxisTitle("Y", "#it{p}^{ meas}_{T}");
      myPlot.SetAxisTitle("Z", "norm. yield");
//      myPlot.SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{acc}) (GeV/#it{c})^{-2}");
      myPlot.AddTextBox(0.15, 0.8, lable);
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName, dataSet);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "multPtUnfolded";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logZ logY");
      myPlot.AddHisto(plotName);
//      myPlot.SetAxisRange("X", 0, 60);
      myPlot.SetAxisRange("Y", 0.15, 50.0);
      myPlot.SetAxisRange("Z", 1e-15, 5);
      myPlot.SetAxisTitle("X", "#it{N}^{ true}_{ch}");
      myPlot.SetAxisTitle("Y", "#it{p}^{ true}_{T}");
      myPlot.SetAxisTitle("Z", "norm. yield");
//      myPlot.SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{ch}) (GeV/#it{c})^{-2}");
      myPlot.AddTextBox(0.15, 0.8, lable);
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName, dataSet);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "multPtMeasuredMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logZ logY");
      myPlot.AddHisto(plotName);
//      myPlot.SetAxisRange("X", 0, 60);
      myPlot.SetAxisRange("Y", 0.15, 50.0);
      myPlot.SetAxisRange("Z", 1e-15, 5);
      myPlot.SetAxisTitle("X", "#it{N}^{ meas}_{ch}");
      myPlot.SetAxisTitle("Y", "#it{p}^{ meas}_{T}");
      myPlot.SetAxisTitle("Z", "norm. yield");
//      myPlot.SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{acc}) (GeV/#it{c})^{-2}");
      myPlot.AddTextBox(0.15, 0.8, lable);
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName, dataSet);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "multPtUnfoldedMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.SetDrawingProperties("logZ logY");
      myPlot.AddHisto(plotName);
//      myPlot.SetAxisRange("X", 0, 60);
      myPlot.SetAxisRange("Y", 0.15, 50.0);
      myPlot.SetAxisRange("Z", 1e-15, 5);
      myPlot.SetAxisTitle("X", "#it{N}^{ true}_{ch}");
      myPlot.SetAxisTitle("Y", "#it{p}^{ true}_{T}");
      myPlot.SetAxisTitle("Z", "norm. yield");
//      myPlot.SetAxisTitle("Z", "1/#it{N}_{evt} 1/(2#pi #it{p}_{T}) (d^{3}#it{N})/(d#it{p}_{T}d#it{#eta}d#it{N}_{ch}) (GeV/#it{c})^{-2}");
      myPlot.AddTextBox(0.15, 0.8, lable);
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName, dataSet);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "meanPt_SystContrib";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto("momentUnfolded1_RelSyst_total", "", "", 0, 0, "hist");
      myPlot.AddHisto("momentUnfolded1_RelSyst_Maxchi2perITSclu", "", "", 0, 0, "hist");
      myPlot.AddHisto("momentUnfolded1_RelSyst_Maxchi2perTPCclu", "", "", 0, 0, "hist");
      myPlot.AddHisto("momentUnfolded1_RelSyst_RatioCrossedRowsOverFindableClustersTPC", "", "", 0, 0, "hist");
      myPlot.AddHisto("momentUnfolded1_RelSyst_FractionSharedClustersTPC", "", "", 0, 0, "hist");
      myPlot.AddHisto("momentUnfolded1_RelSyst_MaxChi2TPCConstrained", "", "", 0, 0, "hist");
      myPlot.AddHisto("momentUnfolded1_RelSyst_DCAtoVertexXYPtDep", "", "", 0, 0, "hist");
      myPlot.AddHisto("momentUnfolded1_RelSyst_DCAtoVertexZ", "", "", 0, 0, "hist");
      myPlot.AddHisto("momentUnfolded1_RelSyst_ClusterReqITS", "", "", 0, 0, "hist");
      myPlot.AddHisto("momentUnfolded1_RelSyst_Ncrnclgeomlength", "", "", 0, 0, "hist");
      myPlot.AddHisto("momentUnfolded1_RelSyst_DeadzoneWidth", "", "", 0, 0, "hist");
      myPlot.AddHisto("momentUnfolded1_RelSyst_ZVertexSelection", "", "", 0, 0, "hist");

      myPlot.SetAxisRange("X", 0, 60);
    //  myPlot3.SetAxisRange("Y", 0.45, 0.85);
    //  myPlot3.SetDrawingProperties("thick");
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName, dataSet);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      Plot myPlot("efficiency");
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto("primTrackEff", dataSet, "", 0, kBlue);
      myPlot.SetDrawingProperties("logX");
      myPlot.SetAxisRange("X", 0.15, 50);
      myPlot.SetAxisRange("Y", 0.5, 0.9);
      myPlot.AddLegendBox(0.6, 0.4, "#sqrt{#it{s}_{NN}} = 5.02 TeV");
      myPlot.AddTextBox(0.15, 0.9, lable5TeV);
      myPlot.SetAxisTitle("X", "#it{p}^{ true}_{T}");
      myPlot.SetAxisTitle("Y", "efficiency x acceptance");
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot("efficiency", dataSet);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      Plot myPlot("secCont");
      myPlot.SetFigureGroup(dataSet);
      myPlot.AddHisto("secContamMC", dataSet, "", 0, kBlue);
      myPlot.SetDrawingProperties("logX");
      myPlot.SetAxisRange("X", 0.15, 50);
      myPlot.AddTextBox(0.15, 0.9, lable5TeV);
      myPlot.SetAxisTitle("Y", "secondary contamination");
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot("secCont", dataSet);
    } // -------------------------------------------------------------------------
  }


  if(plotEnv.ContainsDatasets({"pp_5TeV_10GeV_CENTwoSDD", "pp_5TeV_10GeV_CENT", "pp_5TeV_10GeV_FAST"}))
  {
    {
      // -------------------------------------------------------------------------
      string plotName = "meanMeasuredMult";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup("combinedPlots");
      myPlot.SetAxisTitle("Y", "< #it{N}_{acc} >");
      myPlot.AddLegendBox(0.6, 0.4);
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENT", "CENT");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENTwoSDD", "CENT (no SDD)");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_FAST", "FAST (no SDD)");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_CENTwoSDD", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_FAST", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.SetAxisRange("ratio", 0.94, 1.06);
      myPlot.SetAxisTitle("ratio", "wo / with");
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, "combinedPlots");
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "meanMeasuredMultTracks";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup("combinedPlots");
      myPlot.SetAxisTitle("Y", "< #it{N}_{acc} >");
      myPlot.AddLegendBox(0.6, 0.4);
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENT", "CENT");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENTwoSDD", "CENT (no SDD)");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_FAST", "FAST (no SDD)");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_CENTwoSDD", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_FAST", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.SetAxisRange("ratio", 0.94, 1.06);
      myPlot.SetAxisTitle("ratio", "wo / with");
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, "combinedPlots");
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "multDistGeneratedMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup("combinedPlots");
      myPlot.SetDrawingProperties("logY");
      myPlot.AddLegendBox(0.6, 0.8);
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENT", "CENT");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENTwoSDD", "CENT (no SDD)");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_FAST", "FAST (no SDD)");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_CENTwoSDD", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_FAST", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.SetAxisRange("ratio", 0.95, 1.05);
      myPlot.SetAxisTitle("ratio", "wo / with");
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, "combinedPlots");
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "eventCountMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup("combinedPlots");
  //    myPlot.SetDrawingProperties("logY normalize");
      myPlot.AddLegendBox(0.6, 0.8);
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENT", "CENT");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENTwoSDD", "CENT (no SDD)");
  //    myPlot.AddHisto(plotName, "pp_5TeV_10GeV_FAST", "FAST (no SDD)");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_CENTwoSDD", plotName, "pp_5TeV_10GeV_CENT");
  //    myPlot.AddRatio(plotName, "pp_5TeV_10GeV_FAST", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.SetAxisRange("Y", 7.0e7, 9.0e7);
      myPlot.SetAxisRange("X", 1, 3);
      myPlot.SetAxisRange("ratio", 0.986, 1.014);
      myPlot.SetAxisTitle("ratio", "wo / with");
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, "combinedPlots");
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "eventCountData";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup("combinedPlots");
  //    myPlot.SetDrawingProperties("logY normalize");
      myPlot.AddLegendBox(0.6, 0.8);
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENT", "CENT");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENTwoSDD", "CENT (no SDD)");
  //    myPlot.AddHisto(plotName, "pp_5TeV_10GeV_FAST", "FAST (no SDD)");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_CENTwoSDD", plotName, "pp_5TeV_10GeV_CENT");
  //    myPlot.AddRatio(plotName, "pp_5TeV_10GeV_FAST", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.SetAxisRange("Y", 32.0e7, 42.0e7);
      myPlot.SetAxisRange("X", 1, 3);
      myPlot.SetAxisRange("ratio", 0.986, 1.014);
      myPlot.SetAxisTitle("ratio", "wo / with");
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, "combinedPlots");
    } // -------------------------------------------------------------------------


    { // -------------------------------------------------------------------------
      string plotName = "multDistMeasuredMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup("combinedPlots");
      myPlot.SetDrawingProperties("logY");
      myPlot.AddLegendBox(0.6, 0.6);
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENT", "CENT");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENTwoSDD", "CENT (no SDD)");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_FAST", "FAST (no SDD)");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_CENTwoSDD", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_FAST", plotName, "pp_5TeV_10GeV_CENT");
  //    myPlot.SetAxisRange("ratio", 0.94, 1.06);
      myPlot.SetAxisTitle("ratio", "wo / with");
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, "combinedPlots");
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "primTrackEff";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup("combinedPlots");
      myPlot.SetDrawingProperties("logX");
      myPlot.AddLegendBox(0.6, 0.6);
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENT", "CENT");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENTwoSDD", "CENT (no SDD)");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_FAST", "FAST (no SDD)");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_CENTwoSDD", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_FAST", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.SetAxisRange("Y", 0.25, 0.8);
      myPlot.SetAxisRange("ratio", 0.94, 1.06);
      myPlot.SetAxisTitle("ratio", "wo / with");
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, "combinedPlots");
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "secContamMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup("combinedPlots");
      myPlot.SetDrawingProperties("logX");
      myPlot.AddLegendBox(0.6, 0.6);
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENT", "CENT");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENTwoSDD", "CENT (no SDD)");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_FAST", "FAST (no SDD)");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_CENTwoSDD", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_FAST", plotName, "pp_5TeV_10GeV_CENT");
  //    myPlot.SetAxisRange("Y", 0.25, 0.8);
      myPlot.SetAxisRange("ratio", 0.94, 1.06);
      myPlot.SetAxisTitle("ratio", "wo / with");
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, "combinedPlots");
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "meanRelPtResoFromCovData";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup("combinedPlots");
      myPlot.SetDrawingProperties("logX");
      myPlot.AddLegendBox(0.6, 0.6);
      myPlot.SetAxisTitle("Y", "< #sigma(#it{p}_{T}) / #it{p}_{T} >");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENT", "CENT");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENTwoSDD", "CENT (no SDD)");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_FAST", "FAST (no SDD)");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_CENTwoSDD", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_FAST", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.SetAxisRange("ratio", 0.986, 1.014);
      myPlot.SetAxisTitle("ratio", "wo / with");
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, "combinedPlots");
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "meanRelPtResoFromCovMC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup("combinedPlots");
      myPlot.SetAxisTitle("Y", "< #sigma(#it{p}_{T}) / #it{p}_{T} >");
      myPlot.SetDrawingProperties("logX normalize");
      myPlot.AddLegendBox(0.6, 0.6);
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENT", "CENT");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENTwoSDD", "CENT (no SDD)");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_FAST", "FAST (no SDD)");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_CENTwoSDD", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_FAST", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.SetAxisRange("ratio", 0.986, 1.014);
      myPlot.SetAxisTitle("ratio", "wo / with");
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, "combinedPlots");
    } // -------------------------------------------------------------------------


    { // -------------------------------------------------------------------------
      string plotName = "momentGeneratedMC1";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup("combinedPlots");
  //    myPlot.SetDrawingProperties("logX");
      myPlot.AddLegendBox(0.6, 0.6);
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENT", "CENT");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENTwoSDD", "CENT (no SDD)");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_FAST", "FAST (no SDD)");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_CENTwoSDD", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_FAST", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.SetAxisRange("X", 0., 60);
      myPlot.SetAxisRange("Y", 0.45, 0.85);
      myPlot.SetAxisRange("ratio", 0.98, 1.02);
      myPlot.SetAxisTitle("ratio", "wo / with");
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, "combinedPlots");
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "momentMeasuredMC1";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup("combinedPlots");
  //    myPlot.SetDrawingProperties("logX");
      myPlot.AddLegendBox(0.6, 0.6);
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENT", "CENT");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENTwoSDD", "CENT (no SDD)");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_FAST", "FAST (no SDD)");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_CENTwoSDD", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_FAST", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.SetAxisRange("X", 0., 60);
      myPlot.SetAxisRange("Y", 0.45, 0.85);
      myPlot.SetAxisRange("ratio", 0.98, 1.02);
      myPlot.SetAxisTitle("ratio", "wo / with");
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, "combinedPlots");
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "trueParticlesPt";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup("combinedPlots");
      myPlot.SetDrawingProperties("logX logY");
      myPlot.AddLegendBox(0.6, 0.9);
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENT", "CENT");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENTwoSDD", "CENT (no SDD)");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_FAST", "FAST (no SDD)");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_CENTwoSDD", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_FAST", plotName, "pp_5TeV_10GeV_CENT");
  //    myPlot.SetAxisRange("X", 0., 60);
  //    myPlot.SetAxisRange("Y", 0.45, 0.85);
      myPlot.SetAxisRange("ratio", 0.98, 1.02);
      myPlot.SetAxisTitle("ratio", "wo / with");
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, "combinedPlots");
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "corrParticlesPt";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup("combinedPlots");
      myPlot.SetDrawingProperties("logX logY");
      myPlot.AddLegendBox(0.6, 0.9);
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENT", "CENT");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENTwoSDD", "CENT (no SDD)");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_FAST", "FAST (no SDD)");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_CENTwoSDD", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_FAST", plotName, "pp_5TeV_10GeV_CENT");
  //    myPlot.SetAxisRange("X", 0., 60);
  //    myPlot.SetAxisRange("Y", 0.45, 0.85);
      myPlot.SetAxisRange("ratio", 0.98, 1.02);
      myPlot.SetAxisTitle("ratio", "wo / with");
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, "combinedPlots");
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "unfoParticlesPt";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup("combinedPlots");
      myPlot.SetDrawingProperties("logX logY");
      myPlot.AddLegendBox(0.6, 0.9);
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENT", "CENT");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENTwoSDD", "CENT (no SDD)");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_FAST", "FAST (no SDD)");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_CENTwoSDD", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_FAST", plotName, "pp_5TeV_10GeV_CENT");
  //    myPlot.SetAxisRange("X", 0., 60);
  //    myPlot.SetAxisRange("Y", 0.45, 0.85);
      myPlot.SetAxisRange("ratio", 0.98, 1.02);
      myPlot.SetAxisTitle("ratio", "wo / with");
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, "combinedPlots");
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "recoParticlesPt";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup("combinedPlots");
      myPlot.SetDrawingProperties("logX logY");
      myPlot.AddLegendBox(0.6, 0.9);
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENT", "CENT");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENTwoSDD", "CENT (no SDD)");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_FAST", "FAST (no SDD)");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_CENTwoSDD", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_FAST", plotName, "pp_5TeV_10GeV_CENT");
  //    myPlot.SetAxisRange("X", 0., 60);
  //    myPlot.SetAxisRange("Y", 0.45, 0.85);
      myPlot.SetAxisRange("ratio", 0.95, 1.05);
      myPlot.SetAxisTitle("ratio", "wo / with");
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, "combinedPlots");
    } // -------------------------------------------------------------------------


    { // -------------------------------------------------------------------------
      string plotName = "efficiencyTestPt";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup("combinedPlots");
      myPlot.AddLegendBox(0.6, 0.9);


      vector<Int_t> ptBins = {2, 3, 6};
      for(Int_t ptBin : ptBins)
      {
        string name = "efficiencyTest_" + std::to_string(ptBin);
        myPlot.AddHisto(name, "pp_5TeV_10GeV_CENT", "", kFullSquare, kBlack);
        myPlot.AddHisto(name, "pp_5TeV_10GeV_CENTwoSDD", "", kFullCircle, kBlue);
        myPlot.AddHisto(name, "pp_5TeV_10GeV_FAST", "", kFullCross, kRed+2);
  //      myPlot.AddRatio(name, "pp_5TeV_10GeV_CENTwoSDD", name, "pp_5TeV_10GeV_CENT", "", kFullCircle, kBlue);
  //      myPlot.AddRatio(name, "pp_5TeV_10GeV_FAST", name, "pp_5TeV_10GeV_CENT",  "", kFullCross, kRed+2);
      }
      myPlot.AddTextBox(0.4, 0.5, "low:  0.15-0.20 GeV/c // mid:  0.20-0.25 GeV/c // high: 0.35-0.40 GeV/c");
      myPlot.SetAxisRange("X", 0., 60);
      myPlot.SetAxisRange("Y", 0.25, 0.8);
  //    myPlot.SetAxisRange("ratio", 0.95, 1.05);
      myPlot.SetAxisTitle("ratio", "wo / with");
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName, "combinedPlots");
    } // -------------------------------------------------------------------------


    { // -------------------------------------------------------------------------
      string plotName = "efficiencyTestMean";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup("combinedPlots");
      myPlot.AddLegendBox(0.6, 0.9);
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENT", "CENT");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENTwoSDD", "CENT (no SDD)");
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_FAST", "FAST (no SDD)");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_CENTwoSDD", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.AddRatio(plotName, "pp_5TeV_10GeV_FAST", plotName, "pp_5TeV_10GeV_CENT");
      myPlot.SetAxisRange("X", 0., 60);
      myPlot.SetAxisRange("Y", 2.0, 3.0);
      myPlot.SetAxisRange("ratio", 0.95, 1.05);
      myPlot.SetAxisTitle("ratio", "wo / with");
      plotEnv.CreateNewPlot(myPlot, "default ratio", kFALSE);
      plotEnv.SavePlot(plotName, "combinedPlots");
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "efficiencyTest";
      Plot myPlot(plotName + "_CENTwoSDD");
      myPlot.SetFigureGroup("combinedPlots");
      myPlot.SetDrawingProperties("logY");
      myPlot.AddLegendBox(0.6, 0.9);
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENTwoSDD", "CENT (no SDD)");
      myPlot.SetAxisRange("X", 0., 60);
      myPlot.SetAxisRange("Y", 0.15, 1.0);
      myPlot.SetAxisRange("Z", 0.3, 0.8);
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName + "_CENTwoSDD", "combinedPlots");
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "efficiencyTest";
      Plot myPlot(plotName + "_CENT");
      myPlot.SetFigureGroup("combinedPlots");
      myPlot.SetDrawingProperties("logY");
      myPlot.AddLegendBox(0.6, 0.9);
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_CENT", "CENT");
      myPlot.SetAxisRange("X", 0., 60);
      myPlot.SetAxisRange("Y", 0.15, 1.0);
      myPlot.SetAxisRange("Z", 0.3, 0.8);
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName + "_CENT", "combinedPlots");
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "efficiencyTest";
      Plot myPlot(plotName + "_FAST");
      myPlot.SetFigureGroup("combinedPlots");
      myPlot.SetDrawingProperties("logY");
      myPlot.AddLegendBox(0.6, 0.9);
      myPlot.AddHisto(plotName, "pp_5TeV_10GeV_FAST", "FAST (no SDD)");
      myPlot.SetAxisRange("X", 0., 60);
      myPlot.SetAxisRange("Y", 0.15, 1.0);
      myPlot.SetAxisRange("Z", 0.3, 0.8);
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName + "_FAST", "combinedPlots");
    } // -------------------------------------------------------------------------
  }


  if(plotEnv.ContainsDatasets({"pp_5TeV_2017", "pPb_5TeV_2013", "PbPb_5TeV"}))
  {
    string plotGroup = "system_comparison";

    { // -------------------------------------------------------------------------
      string plotName = "knoScaling";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.SetDrawingProperties("logX logY");
      myPlot.AddHisto("multDistUnfoldedKNO", "pp_5TeV_2017", "5.02 TeV", 0, kBlue+1);
      myPlot.AddHisto("multDistUnfoldedKNO", "pp_7TeV", "7 TeV", 0, kRed+1);
      myPlot.AddLegendBox(0.3, 0.5);
//      myPlot.SetAxisRange("Y", 0.45, 0.85);
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "moment1KNO";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.SetDrawingProperties("logX");
      myPlot.AddHisto("momentUnfolded1KNO", "PbPb_5TeV", "Pb-Pb", 0, kRed+1);
  //    myPlot.AddHisto("momentUnfolded1KNO", "pp_5TeV_2017", "pp", 0, kBlue+1);
  //    myPlot.AddHisto("momentUnfolded1KNO", "pPb_5TeV_2013", "p-Pb", 0, kMagenta+1);
//      myPlot.AddHisto("momentUnfolded1KNO", "PbPb_5TeV", "Pb-Pb", 0, kRed+1);
//      myPlot.SetAxisRange("Y", 0.45, 0.85);
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "moment1dNchdeta";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("momentUnfolded1dNchDeta", "pp_5TeV_2017", "pp", 0, kBlue+1);
      myPlot.SetAxisRange("Y", 0.45, 0.85);
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "moment1";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("momentUnfolded1", "pp_5TeV_2017", "", kFullSquare, kBlue+1);
      myPlot.AddHisto("momentUnfolded1_Syst", "pp_5TeV_2017", "pp", kFullSquare, kBlue+1, "boxes");
      myPlot.AddHisto("momentUnfolded1", "pPb_5TeV_2013", "", kFullCircle, kMagenta+1);
      myPlot.AddHisto("momentUnfolded1_Syst", "pPb_5TeV_2013", "p-Pb", kFullCircle, kMagenta+1, "boxes");
      myPlot.AddHisto("momentUnfolded1", "PbPb_5TeV", "", kFullCross, kRed+1);
      myPlot.AddHisto("momentUnfolded1_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes");

//      myPlot.CropHisto("momentUnfolded1", "pp_5TeV_2017", "X", 0, 40)
//    myPlot.CompressAxis();
  //    myPlot.SetAxisRange("X", 0, 60);
      myPlot.SetAxisRange("Y", 0.45, 0.85);

      myPlot.AddLegendBox(0.6, 0.4, "");

      myPlot.AddTextBox(0.15, 0.9, lable5TeV);
    //  myPlot3.SetDrawingProperties("thick");
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "moment2";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("momentUnfolded2", "pp_5TeV_2017", "", kFullSquare, kBlue+1);
      myPlot.AddHisto("momentUnfolded2_Syst", "pp_5TeV_2017", "pp", kFullSquare, kBlue+1, "boxes");
      myPlot.AddHisto("momentUnfolded2", "pPb_5TeV_2013", "", kFullCircle, kMagenta+1);
      myPlot.AddHisto("momentUnfolded2_Syst", "pPb_5TeV_2013", "p-Pb", kFullCircle, kMagenta+1, "boxes");
      myPlot.AddHisto("momentUnfolded2", "PbPb_5TeV", "", kFullCross, kRed+1);
      myPlot.AddHisto("momentUnfolded2_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes");

//      myPlot.CropHisto("momentUnfolded1", "pp_5TeV_2017", "X", 0, 40)
//    myPlot.CompressAxis();
  //    myPlot.SetAxisRange("X", 0, 60);
      myPlot.SetAxisRange("Y", 0.2, 1.5);

      myPlot.AddLegendBox(0.6, 0.4, "");

      myPlot.AddTextBox(0.15, 0.9, lable5TeV);
    //  myPlot3.SetDrawingProperties("thick");
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "moment3";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("momentUnfolded3", "pp_5TeV_2017", "", kFullSquare, kBlue+1);
      myPlot.AddHisto("momentUnfolded3_Syst", "pp_5TeV_2017", "pp", kFullSquare, kBlue+1, "boxes");
      myPlot.AddHisto("momentUnfolded3", "pPb_5TeV_2013", "", kFullCircle, kMagenta+1);
      myPlot.AddHisto("momentUnfolded3_Syst", "pPb_5TeV_2013", "p-Pb", kFullCircle, kMagenta+1, "boxes");
      myPlot.AddHisto("momentUnfolded3", "PbPb_5TeV", "", kFullCross, kRed+1);
      myPlot.AddHisto("momentUnfolded3_Syst", "PbPb_5TeV", "Pb-Pb", kFullCross, kRed+1, "boxes");

//      myPlot.CropHisto("momentUnfolded1", "pp_5TeV_2017", "X", 0, 40)
//    myPlot.CompressAxis();
  //    myPlot.SetAxisRange("X", 0, 60);
      myPlot.SetAxisRange("Y", 0.2, 5);

      myPlot.AddLegendBox(0.6, 0.4, "");

      myPlot.AddTextBox(0.15, 0.9, lable5TeV);
    //  myPlot3.SetDrawingProperties("thick");
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "moment1MC";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("momentUnfoldedMC1", "pp_5TeV_2017", "pp", 0, kBlue+1);
      myPlot.AddHisto("momentUnfoldedMC1", "pPb_5TeV_2013", "p-Pb", 0, kMagenta+1);
      myPlot.AddHisto("momentUnfoldedMC1", "PbPb_5TeV", "Pb-Pb", 0, kRed+1);

//      myPlot.CropHisto("momentUnfolded1", "pp_5TeV_2017", "X", 0, 40)
//    myPlot.CompressAxis();
  //    myPlot.SetAxisRange("X", 0, 60);
      myPlot.SetAxisRange("Y", 0.45, 0.85);

      myPlot.AddLegendBox(0.6, 0.4, "");

      myPlot.AddTextBox(0.15, 0.9, lable5TeV);
    //  myPlot3.SetDrawingProperties("thick");
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "moment1MCtruth";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("momentGeneratedMC1", "pp_5TeV_2017", "pp", 0, kBlue+1);
      myPlot.AddHisto("momentGeneratedMC1", "pPb_5TeV_2013", "p-Pb", 0, kMagenta+1);
      myPlot.AddHisto("momentGeneratedMC1", "PbPb_5TeV", "Pb-Pb", 0, kRed+1);

//      myPlot.CropHisto("momentUnfolded1", "pp_5TeV_2017", "X", 0, 40)
//    myPlot.CompressAxis();
  //    myPlot.SetAxisRange("X", 0, 60);
      myPlot.SetAxisRange("Y", 0.45, 0.85);

      myPlot.AddLegendBox(0.6, 0.4, "");

      myPlot.AddTextBox(0.15, 0.9, lable5TeV);
    //  myPlot3.SetDrawingProperties("thick");
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -------------------------------------------------------------------------

  }

  {
    //---- Lable definitions ----
    string alice = "#bf{ALICE}";
    string chargedParticles = "charged particles";
    string erg5TeV = "#sqrt{#it{s}} = 5.02 TeV";
    string eta03 = "|#eta| < 0.3";
    string ptRange = "0.15 < #it{p}_{T} < 10 GeV/#it{c}";
    string lablePublication = alice + ", " + chargedParticles +  newLine + eta03 + ", " + ptRange;
    //---------------------------

    string plotGroup = "system_comparison";
    { // -------------------------------------------------------------------------
      string plotName = "publication";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("meanPt_PbPb_2.76TeV_Stat", "Publications", "", kFullCross, kWhite);
      myPlot.AddHisto("meanPt_pp_7TeV_Stat", "Publications", "", kFullSquare, kBlue+1);
      myPlot.AddHisto("meanPt_pp_7TeV_Syst", "Publications", "pp #sqrt{s} = 7 TeV", kFullSquare, kBlue+1, "boxes");
      myPlot.AddHisto("meanPt_pPb_5.02TeV_Stat", "Publications", "", kFullCircle, kMagenta+1);
      myPlot.AddHisto("meanPt_pPb_5.02TeV_Syst", "Publications", "p-Pb #sqrt{s}_{NN} = 5.02 TeV", kFullCircle, kMagenta+1, "boxes");
      myPlot.AddHisto("meanPt_PbPb_2.76TeV_Stat", "Publications", "", kFullCross, kRed+1);
      myPlot.AddHisto("meanPt_PbPb_2.76TeV_Syst", "Publications", "Pb-Pb  #sqrt{s}_{NN} = 2.76 TeV", kFullCross, kRed+1, "boxes");

//      myPlot.CropHisto("momentUnfolded1", "pp_5TeV_2017", "X", 0, 40)
//    myPlot.CompressAxis();
      myPlot.SetAxisRange("X", 0, 100);
      myPlot.SetAxisRange("Y", 0.45, 0.92);

      myPlot.AddLegendBox(0.55, 0.3, "");

      myPlot.AddTextBox(0.5, 0.92, lablePublication);
    //  myPlot3.SetDrawingProperties("thick");
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -------------------------------------------------------------------------

    { // -------------------------------------------------------------------------
      string plotName = "publicationdNchdEta";
      Plot myPlot(plotName);
      myPlot.SetFigureGroup(plotGroup);
      myPlot.AddHisto("meanPt_PbPb_2.76TeV_dNchdEta_Stat", "Publications", "", kFullCross, kWhite);
      myPlot.AddHisto("meanPt_pp_7TeV_dNchdEta_Stat", "Publications", "", kFullSquare, kBlue+1);
      myPlot.AddHisto("meanPt_pp_7TeV_dNchdEta_Syst", "Publications", "pp #sqrt{s} = 7 TeV", kFullSquare, kBlue+1, "boxes");
      myPlot.AddHisto("meanPt_pPb_5.02TeV_dNchdEta_Stat", "Publications", "", kFullCircle, kMagenta+1);
      myPlot.AddHisto("meanPt_pPb_5.02TeV_dNchdEta_Syst", "Publications", "p-Pb #sqrt{s}_{NN} = 5.02 TeV", kFullCircle, kMagenta+1, "boxes");
      myPlot.AddHisto("meanPt_PbPb_2.76TeV_dNchdEta_Stat", "Publications", "", kFullCross, kRed+1);
      myPlot.AddHisto("meanPt_PbPb_2.76TeV_dNchdEta_Syst", "Publications", "Pb-Pb  #sqrt{s}_{NN} = 2.76 TeV", kFullCross, kRed+1, "boxes");

      myPlot.AddHisto("momentUnfolded1dNchDeta", "pp_5TeV_2017", "pp", kOpenSquare, kBlue+1);
      myPlot.AddHisto("momentUnfolded1dNchDeta", "pPb_5TeV_2013", "pPb", kOpenCircle, kMagenta+1);
      myPlot.AddHisto("momentUnfolded1dNchDeta", "PbPb_5TeV", "PbPb", kOpenCross, kRed+1);

//      myPlot.CropHisto("momentUnfolded1", "pp_5TeV_2017", "X", 0, 40)
//    myPlot.CompressAxis();
//      myPlot.SetAxisRange("X", 0, 100);
      myPlot.SetAxisRange("Y", 0.45, 0.92);

      myPlot.AddLegendBox(0.55, 0.3, "");

      myPlot.AddTextBox(0.5, 0.92, lablePublication);
    //  myPlot3.SetDrawingProperties("thick");
      plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
      plotEnv.SavePlot(plotName, plotGroup);
    } // -------------------------------------------------------------------------



  }


  /*
  { // -------------------------------------------------------------------------
    Plot myPlot("moment1");
    myPlot.SetFigureGroup("combinedPlots");
    myPlot.AddHisto("momentUnfolded1", dataSetPP, "abc", 0, kBlue);
    myPlot.AddHisto("momentUnfolded1_Syst", dataSetPP, "pp", 0, kBlue, "boxes");
    myPlot.AddHisto("momentUnfolded1", dataSetPPb, "p-Pb", 0, kRed);
    myPlot.AddHisto("momentUnfolded1_Syst", dataSetPPb, "", 0, kRed, "boxes");
    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 0.45, 0.85);

    myPlot.AddLegendBox(0.6, 0.4, "");

    myPlot.AddTextBox(0.15, 0.9, lable5TeV);
  //  myPlot3.SetDrawingProperties("thick");
    plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
    plotEnv.SavePlot("moment1", "combinedPlots");
  } // -------------------------------------------------------------------------

  { // -------------------------------------------------------------------------
    Plot myPlot("moment2");
    myPlot.SetFigureGroup("combinedPlots");
    myPlot.AddHisto("momentUnfolded2", dataSetPP, "PYTIA8.23", 0, kBlue);
    myPlot.AddHisto("momentUnfolded2_Syst", dataSetPP, "pp", 0, kBlue, "boxes");
    myPlot.AddHisto("momentUnfolded2", dataSetPPb, "p-Pb", 0, kRed);
    myPlot.AddHisto("momentUnfolded2_Syst", dataSetPPb, "ssss", 0, kRed, "boxes");
    myPlot.SetAxisRange("X", 0, 60);
//    myPlot.SetAxisRange("Y", 0.45, 0.85);

    myPlot.AddLegendBox(0.6, 0.4, "", kSolid, 5, kBlue);

    myPlot.AddTextBox(0.15, 0.9, lable5TeV);
  //  myPlot3.SetDrawingProperties("thick");
    plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
    plotEnv.SavePlot("moment2", "combinedPlots");
  } // -------------------------------------------------------------------------

  { // -------------------------------------------------------------------------
    Plot myPlot("moment3");
    myPlot.SetFigureGroup("combinedPlots");
    myPlot.AddHisto("momentUnfolded3", dataSetPP, "", 0, kBlue);
    myPlot.AddHisto("momentUnfolded3_Syst", dataSetPP, "pp", 0, kBlue, "boxes");
    myPlot.AddHisto("momentUnfolded3", dataSetPPb, "p-Pb", 0, kRed);
    myPlot.AddHisto("momentUnfolded3_Syst", dataSetPPb, "", 0, kRed, "boxes");
    myPlot.SetAxisRange("X", 0, 60);
//    myPlot.SetAxisRange("Y", 0.45, 0.85);
    myPlot.AddLegendBox(0.6, 0.4, "");
    myPlot.AddTextBox(0.15, 0.9, lable5TeV);
  //  myPlot3.SetDrawingProperties("thick");
    plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
    plotEnv.SavePlot("moment3", "combinedPlots");
  } // -------------------------------------------------------------------------

  { // -------------------------------------------------------------------------
    Plot myPlot("multDist");
    myPlot.SetFigureGroup("combinedPlots");
    myPlot.AddHisto("multDistUnfolded", dataSetPP, "pp", 0, kBlue);
    myPlot.AddHisto("multDistUnfolded", dataSetPPb, "p-Pb", 0, kRed);
    myPlot.SetDrawingProperties("logY normalize");

    myPlot.SetAxisRange("X", 0, 60);
    myPlot.SetAxisRange("Y", 1e-5, 2);
    myPlot.AddLegendBox(0.15, 0.4, "");
    myPlot.AddTextBox(0.15, 0.9, "a // ab // abcdefghasdfasAAAAAsadf", kDotted, 5, kBlue);
    plotEnv.CreateNewPlot(myPlot, "default", kFALSE);
    plotEnv.SavePlot("multDist", "combinedPlots");
  } // -------------------------------------------------------------------------

*/

}
