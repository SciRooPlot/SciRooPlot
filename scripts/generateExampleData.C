// Generates the pseudo-data used by the project template (srp init-cpp / srp init-py).
// It runs at build time in <build>/share/scirooplot and is installed to ${SCIROOPLOT_DATA_DIR}/ExampleData.
// The random seed is fixed, so every build produces identical files.
//
// All numbers are made up, but shaped like what one meets in a real analysis:
//
//   higgs.root                        a Higgs "analysis": one directory per channel
//     diphoton/    mgg_data, mgg_signalMC, mgg_bkgFit (TF1), mgg_sigBkgFit (TF1)
//     fourLepton/  m4l_data, m4l_background, m4l_sigPlusBkg
//     limits/      expected, observed (TGraph), band1sigma, band2sigma (TGraphAsymmErrors)
//   spectra.root                      charged-particle spectra, data and simulation
//     data/        ptSpec, multDist, ptVsMult (TH2)
//     mc/          ptSpec, multDist
//   AnalysisResults_data.root         typical grid-analysis output: a directory per task,
//   AnalysisResults_mc.root           each holding one TList called "output"
//     EventQA/output     hVtxZ, hEventSelection (labelled bins), hNPV (pile-up vertices)
//     TriggerQA/output   hLeadingPtAll, hLeadingPtTriggered
//     TrackQA/output     hEtaPhi, hPt, [mc only: hPtGen, hPtRecVsGen]
//     PIDQA/output       hTPCdEdxVsP, hTOFBetaVsP
//     CaloQA/output      hInvMassGG, hInvMassGGVsPt (TH2: mass vs. pair pT in six pT bins), hInvMassGGVsPtMixed (mixed events)
//   dimuon.root                       mMuMu: dimuon invariant mass from J/psi to the Z (raw counts, log bins)
//   jets/jets_R0{2,4,6}.root           jet spectra for three radii (a data source given as a directory)
//     hJetPt_R02 ... hJetPt_R06, [R04 only: fNLO_R04 (TF1), hJESResponse (TH2)]
//   heavyion.root                     Pb-Pb collisions
//     hV0M (centrality estimator), hPtEtaCent (THnSparse: pT, eta, centrality)
//     cent_0-10/, cent_10-30/, cent_30-50/   ptSpec (per event), v2 (TGraphErrors)
//   tracks.root                       TTree "tracks": pt, eta, phi, charge, nClustersTPC, dcaXY, isPrimary
//   decays.root                       TTree "v0s": alpha, qT, pt (V0 candidates for an Armenteros-Podolanski plot)
//                                     TTree "d0ToK0sPiPi": m2KsPip, m2KsPim, m2PiPi (D0 -> K0S pi+ pi- for a Dalitz plot)
//   ml.root                           a classifier: score distributions hBDT_{sig,bkg}_{train,test} (same for DNN),
//                                     ROC curves roc_BDT, roc_DNN (TGraph, the AUC in the title), hFeatureImportance
//   medical.root                      ion beams in water and a treatment plan
//     depthDose/   proton_100MeV, proton_150MeV, proton_200MeV, carbon_280MeVu (dose vs. depth, % of the maximum)
//     plan/        doseMap (TH2: dose of a two-field proton plan in a transverse slice, % of the prescribed dose)
//   atomic.root                       collinear laser spectroscopy: hScan (counts vs. laser frequency offset over
//                                     the hyperfine structure of an isotope), fitScan (TF1, four Voigt lines)
//   accelerator.root                  a beam line: hBeamSpot (TH2: transverse beam profile at a screen),
//                                     TTree "beam": x, xp (horizontal phase space), rmsEllipse (TGraph, the emittance in the title)
//   co2.csv                           monthly atmospheric CO2: year (decimal), co2, deseasonalized
//   stars.csv                         a star catalogue for a Hertzsprung-Russell diagram: bv (colour index), logL
//   spikes.csv                        spike times of a neuron in 40 trials around a stimulus: trial, time (ms)
//   ptSpec_published.csv              "HEPData" table: pt, pt_err, value, stat, syst
//   testbeam.csv                      calorimeter test beam: energy, resolution, resolution_err
//   luminosity.csv                    integrated luminosity per week: time (unix), lumi
//   raa_published.csv                 nuclear modification factor 0-10%: pt, pt_err, raa, stat, syst_low, syst_high
//   runQA.csv                         run-by-run QA: run, meanNch, meanNch_err
//   systematics.csv                   relative systematic uncertainties vs. pT per source

#include <TF1.h>
#include <TFile.h>
#include <TGenPhaseSpace.h>
#include <TGraph.h>
#include <TGraphErrors.h>
#include <TGraphAsymmErrors.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TLorentzVector.h>
#include <THnSparse.h>
#include <TList.h>
#include <TMath.h>
#include <TRandom3.h>
#include <TSystem.h>
#include <TTree.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace
{
const std::string kFolder = "ExampleData/";

// --------------------------------------------------------------------------------------------------
// shared physics models
// --------------------------------------------------------------------------------------------------

// charged-particle pT spectrum: two-component model (exponential + power law), times pT
std::unique_ptr<TF1> PtSpectrumFunc(const char* name, double powerN)
{
  auto tcm = [](double* x, double* par) {
    const double pT = x[0];
    const double m = 0.13957039;  // pion mass
    const double y = par[0] * std::exp(-(std::sqrt(pT * pT + m * m) - m) / par[1]) + par[2] / std::pow(1 + (pT * pT) / (par[3] * par[3] * par[4]), par[4]);
    return pT * y;
  };
  auto func = std::make_unique<TF1>(name, tcm, 0.1, 20., 5);
  func->SetParameters(95.31849945, 0.16398725, 6.60023282, 0.74124785, powerN);
  func->SetNpx(2000);
  return func;
}

// charged-particle multiplicity: sum of two negative binomial distributions
std::unique_ptr<TF1> MultiplicityFunc(const char* name, double meanSemihard)
{
  auto doubleNBD = [](double* x, double* par) {
    auto NBD = [](double n, double mean, double k) {
      return TMath::Gamma(n + k) / (TMath::Gamma(n + 1) * TMath::Gamma(k)) * std::pow(mean / (k + mean), n) / std::pow(1.0 + mean / k, k);
    };
    return par[0] * NBD(x[0], par[1], par[2]) + (1 - par[0]) * NBD(x[0], par[3], par[4]);
  };
  auto func = std::make_unique<TF1>(name, doubleNBD, 0., 100., 5);
  func->SetParameters(0.59482043, 4.51789303, 1.62701245, meanSemihard, 3.20841373);
  func->SetNpx(1000);
  return func;
}

// mean energy loss in the TPC (ALEPH parameterisation, in units of the minimum-ionising signal ~ 50)
double TPCdEdx(double p, double mass)
{
  const double bg = p / mass;
  const double beta = bg / std::sqrt(1. + bg * bg);
  const double kp1 = 0.76176e-1, kp2 = 10.632, kp3 = 0.13279e-4, kp4 = 1.8631, kp5 = 1.9479;
  const double aa = std::pow(beta, kp4);
  const double bb = std::log(kp3 + std::pow(1. / bg, kp5));
  return 50. * kp1 * (kp2 - aa - bb) / aa;
}

struct Species {
  const char* name;
  double mass;
  double abundance;
};
const std::vector<Species> kSpecies = {{"e", 0.000511, 0.02}, {"pi", 0.13957, 0.80}, {"K", 0.49368, 0.12}, {"p", 0.93827, 0.06}};

const Species& RandomSpecies()
{
  double r = gRandom->Rndm();
  for (const auto& species : kSpecies) {
    if (r < species.abundance) return species;
    r -= species.abundance;
  }
  return kSpecies[1];
}

std::vector<double> LogBins(int nBins, double min, double max)
{
  std::vector<double> edges;
  for (int i = 0; i <= nBins; ++i) edges.push_back(min * std::pow(max / min, static_cast<double>(i) / nBins));
  return edges;
}

const std::vector<double> kPtBins = {0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5, 0.55, 0.6, 0.65, 0.7, 0.75,
                                     0.8, 0.85, 0.9, 0.95, 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9,
                                     2.0, 2.2, 2.4, 2.6, 2.8, 3.0, 3.2, 3.4, 3.6, 3.8, 4.0, 4.5, 5.0, 5.5,
                                     6.0, 6.5, 7.0, 8.0, 9.0, 10.0, 11., 12., 13., 14., 15., 16., 18., 20.};

// --------------------------------------------------------------------------------------------------
// higgs.root
// --------------------------------------------------------------------------------------------------
void WriteHiggs()
{
  TFile file((kFolder + "higgs.root").data(), "RECREATE");

  {  // H -> gamma gamma: smoothly falling background with a small peak at 125 GeV
    file.mkdir("diphoton")->cd();
    const double mH = 125.1, width = 1.7, nSignal = 700.;
    TF1 truth("truth", "[0]*exp(-(x-100.)/[1]) + [2]*TMath::Gaus(x,[3],[4],true)", 100., 160.);
    truth.SetParameters(4200., 32., nSignal, mH, width);

    auto* data = new TH1D("mgg_data", "", 60, 100., 160.);
    data->GetXaxis()->SetTitle("#it{m}_{#gamma#gamma} (GeV)");
    data->GetYaxis()->SetTitle("events / GeV");
    for (int i = 1; i <= data->GetNbinsX(); ++i) {
      const double expected = truth.Integral(data->GetBinLowEdge(i), data->GetBinLowEdge(i + 1));
      const double observed = gRandom->Poisson(expected);
      data->SetBinContent(i, observed);
      data->SetBinError(i, std::sqrt(observed));
    }
    data->Write();

    auto* signalMC = new TH1D("mgg_signalMC", "", 120, 115., 135.);
    signalMC->GetXaxis()->SetTitle("#it{m}_{#gamma#gamma} (GeV)");
    signalMC->GetYaxis()->SetTitle("events / 0.17 GeV");
    signalMC->Sumw2();
    for (int i = 0; i < 50000; ++i) {
      // core resolution plus a low-mass tail from photon conversions
      signalMC->Fill((gRandom->Rndm() < 0.85) ? gRandom->Gaus(mH, width) : mH - std::abs(gRandom->Gaus(0., 3. * width)));
    }
    signalMC->Write();

    TF1 bkgFit("mgg_bkgFit", "[0]*exp(-(x-100.)/[1])", 100., 160.);
    bkgFit.SetParameters(4000., 30.);
    data->Fit(&bkgFit, "Q0N");
    bkgFit.Write();
    TF1 sigBkgFit("mgg_sigBkgFit", "[0]*exp(-(x-100.)/[1]) + [2]*TMath::Gaus(x,[3],[4],true)", 100., 160.);
    sigBkgFit.SetParameters(bkgFit.GetParameter(0), bkgFit.GetParameter(1), 500., mH, width);
    sigBkgFit.FixParameter(4, width);
    data->Fit(&sigBkgFit, "Q0N");
    sigBkgFit.SetNpx(1000);
    sigBkgFit.Write();
  }
  {  // H -> ZZ* -> 4 leptons: Z -> 4l peak, ZZ continuum, reducible background, Higgs peak
    file.mkdir("fourLepton")->cd();
    TF1 background("background", "[0]*TMath::Gaus(x,91.2,2.2,true) + [1]*(1+TMath::Erf((x-150.)/25.)) + [2]*exp(-(x-80.)/40.)", 70., 170.);
    background.SetParameters(95., 2.2, 1.6);
    TF1 signal("signal", "[0]*TMath::Gaus(x,125.1,1.8,true)", 70., 170.);
    signal.SetParameter(0, 38.);

    auto* data = new TH1D("m4l_data", "", 40, 70., 170.);
    auto* bkg = new TH1D("m4l_background", "", 40, 70., 170.);
    auto* sigPlusBkg = new TH1D("m4l_sigPlusBkg", "", 40, 70., 170.);
    for (auto* hist : {data, bkg, sigPlusBkg}) {
      hist->GetXaxis()->SetTitle("#it{m}_{4l} (GeV)");
      hist->GetYaxis()->SetTitle("events / 2.5 GeV");
    }
    for (int i = 1; i <= data->GetNbinsX(); ++i) {
      const double lo = data->GetBinLowEdge(i), hi = data->GetBinLowEdge(i + 1);
      const double b = background.Integral(lo, hi) / 2.5;
      const double s = signal.Integral(lo, hi) / 2.5;
      bkg->SetBinContent(i, b);
      sigPlusBkg->SetBinContent(i, b + s);
      const double observed = gRandom->Poisson(b + s);
      data->SetBinContent(i, observed);
      data->SetBinError(i, std::sqrt(observed));
    }
    data->Write();
    bkg->Write();
    sigPlusBkg->Write();
  }
  {  // 95% CL upper limits on the signal strength vs. Higgs-boson mass, with an excess near 125 GeV
    file.mkdir("limits")->cd();
    const int n = 41;
    TGraph expected(n), observed(n);
    TGraphAsymmErrors band1(n), band2(n);
    for (int i = 0; i < n; ++i) {
      const double m = 110. + i;
      const double median = 0.25 + 0.3 * std::pow((m - 110.) / 40. - 0.45, 2) + 0.12 * std::exp(-std::pow((m - 145.) / 6., 2));
      const double excess = 1.6 * std::exp(-0.5 * std::pow((m - 125.1) / 2.5, 2));
      expected.SetPoint(i, m, median);
      observed.SetPoint(i, m, median * (1. + 0.15 * gRandom->Gaus()) + excess);
      band1.SetPoint(i, m, median);
      band1.SetPointError(i, 0., 0., 0.28 * median, 0.39 * median);
      band2.SetPoint(i, m, median);
      band2.SetPointError(i, 0., 0., 0.47 * median, 0.86 * median);
    }
    for (auto* graph : std::vector<TGraph*>{&expected, &observed, &band1, &band2}) {
      graph->GetXaxis()->SetTitle("#it{m}_{H} (GeV)");
      graph->GetYaxis()->SetTitle("95% CL limit on #sigma/#sigma_{SM}");
    }
    expected.Write("expected");
    observed.Write("observed");
    band1.Write("band1sigma");
    band2.Write("band2sigma");
  }
}

// --------------------------------------------------------------------------------------------------
// spectra.root
// --------------------------------------------------------------------------------------------------
TH1D* PtSpectrum(const char* name, TF1& func, int nTracks, double nEvents)
{
  auto* hist = new TH1D(name, "", kPtBins.size() - 1, kPtBins.data());
  hist->Sumw2();
  hist->FillRandom(func.GetName(), nTracks);
  hist->Scale(1. / nEvents, "width");
  hist->GetXaxis()->SetTitle("#it{p}_{T} (GeV/#it{c})");
  hist->GetYaxis()->SetTitle("1/#it{N}_{ev} d#it{N}/d#it{p}_{T} (GeV/#it{c})^{-1}");
  return hist;
}

TH1D* MultDist(const char* name, TF1& func, int nEvents)
{
  auto* hist = new TH1D(name, "", 71, -0.5, 70.5);
  hist->Sumw2();
  hist->FillRandom(func.GetName(), nEvents);
  hist->Scale(1. / nEvents);
  hist->GetXaxis()->SetTitle("#it{N}_{ch}");
  hist->GetYaxis()->SetTitle("#it{P}(#it{N}_{ch})");
  return hist;
}

void WriteSpectra(TF1& ptData, TF1& ptMC, TF1& multData, TF1& multMC)
{
  const int nEvents = 100000;
  const int nTracks = 300000;
  TFile file((kFolder + "spectra.root").data(), "RECREATE");

  file.mkdir("data")->cd();
  PtSpectrum("ptSpec", ptData, nTracks, nEvents)->Write();
  MultDist("multDist", multData, nEvents)->Write();
  // mean pT rises with multiplicity
  auto* ptVsMult = new TH2D("ptVsMult", "", 60, 0.5, 60.5, 50, 0., 5.);
  ptVsMult->GetXaxis()->SetTitle("#it{N}_{ch}");
  ptVsMult->GetYaxis()->SetTitle("#it{p}_{T} (GeV/#it{c})");
  ptVsMult->GetZaxis()->SetTitle("tracks");
  for (int i = 0; i < nTracks; ++i) {
    const double nch = multData.GetRandom(1., 60.);
    const double meanPt = 0.4 + 0.006 * nch;
    // Gamma(k = 2) distributed pT with mean meanPt
    ptVsMult->Fill(nch, gRandom->Exp(meanPt / 2.) + gRandom->Exp(meanPt / 2.));
  }
  ptVsMult->Write();

  // the simulation has ten times more events, stored per event like the data
  file.mkdir("mc")->cd();
  PtSpectrum("ptSpec", ptMC, 10 * nTracks, 10 * nEvents)->Write();
  MultDist("multDist", multMC, 10 * nEvents)->Write();
}

// --------------------------------------------------------------------------------------------------
// AnalysisResults_{data,mc}.root
// --------------------------------------------------------------------------------------------------
void WriteAnalysisResults(bool isMC, TF1& ptFunc)
{
  const int nEvents = isMC ? 400000 : 1000000;
  const int nTracks = 200000;
  TFile file((kFolder + (isMC ? "AnalysisResults_mc.root" : "AnalysisResults_data.root")).data(), "RECREATE");
  auto writeTask = [&](const char* task, TList& output) {
    file.mkdir(task)->cd();
    output.Write("output", TObject::kSingleKey);
  };

  {  // event QA: vertex position and the event selection
    TList output;
    output.SetOwner();
    auto* vtxZ = new TH1D("hVtxZ", "", 120, -30., 30.);
    vtxZ->GetXaxis()->SetTitle("#it{z}_{vtx} (cm)");
    vtxZ->GetYaxis()->SetTitle("events");
    const std::vector<const char*> steps = {"all", "triggered", "vertex found", "|#it{z}_{vtx}| < 10 cm", "no pile-up"};
    auto* selection = new TH1D("hEventSelection", "", steps.size(), 0., steps.size());
    for (size_t i = 0; i < steps.size(); ++i) selection->GetXaxis()->SetBinLabel(i + 1, steps[i]);
    selection->GetYaxis()->SetTitle("events");
    const double vtxMean = isMC ? 0. : 0.4, vtxSigma = isMC ? 6.0 : 6.6;
    for (int i = 0; i < nEvents; ++i) {
      selection->Fill(0.5);
      if (gRandom->Rndm() > 0.86) continue;
      selection->Fill(1.5);
      if (gRandom->Rndm() > 0.96) continue;
      selection->Fill(2.5);
      const double z = gRandom->Gaus(vtxMean, vtxSigma);
      vtxZ->Fill(z);
      if (std::abs(z) > 10.) continue;
      selection->Fill(3.5);
      if (gRandom->Rndm() < (isMC ? 0.005 : 0.02)) continue;
      selection->Fill(4.5);
    }
    // number of reconstructed pile-up vertices; the tail reaches beyond the histogram range
    auto* nPV = new TH1D("hNPV", "", 60, -0.5, 59.5);
    nPV->GetXaxis()->SetTitle("#it{N}_{PV}");
    nPV->GetYaxis()->SetTitle("events");
    for (int i = 0; i < nEvents / 10; ++i) nPV->Fill(gRandom->Poisson(isMC ? 30. : 34.) + ((gRandom->Rndm() < 0.03) ? gRandom->Exp(25.) : 0.));
    output.Add(vtxZ);
    output.Add(selection);
    output.Add(nPV);
    writeTask("EventQA", output);
  }
  {  // trigger: leading-track pT of all events and of events accepted by a high-pT trigger (threshold ~8 GeV)
    TList output;
    output.SetOwner();
    auto* all = new TH1D("hLeadingPtAll", "", 40, 0., 20.);
    all->GetXaxis()->SetTitle("leading track #it{p}_{T} (GeV/#it{c})");
    all->GetYaxis()->SetTitle("events");
    auto* triggered = static_cast<TH1D*>(all->Clone("hLeadingPtTriggered"));
    for (int i = 0; i < nEvents / 5; ++i) {
      const double pt = gRandom->Uniform(0., 20.);
      all->Fill(pt);
      if (gRandom->Rndm() < 0.98 * 0.5 * (1. + TMath::Erf((pt - 8.) / (std::sqrt(2.) * 1.5)))) triggered->Fill(pt);
    }
    output.Add(all);
    output.Add(triggered);
    writeTask("TriggerQA", output);
  }
  {  // track QA: acceptance and pT, for simulation also the generated tracks and the pT response
    TList output;
    output.SetOwner();
    auto* etaPhi = new TH2D("hEtaPhi", "", 36, -0.9, 0.9, 72, 0., 2. * TMath::Pi());
    etaPhi->GetXaxis()->SetTitle("#eta");
    etaPhi->GetYaxis()->SetTitle("#varphi (rad)");
    etaPhi->GetZaxis()->SetTitle("tracks");
    auto* pt = new TH1D("hPt", "", kPtBins.size() - 1, kPtBins.data());
    pt->GetXaxis()->SetTitle("#it{p}_{T} (GeV/#it{c})");
    pt->GetYaxis()->SetTitle("reconstructed tracks");
    auto* ptGen = static_cast<TH1D*>(pt->Clone("hPtGen"));
    ptGen->GetYaxis()->SetTitle("generated particles");
    auto* response = new TH2D("hPtRecVsGen", "", 50, 0., 10., 50, 0., 10.);
    response->GetXaxis()->SetTitle("#it{p}_{T}^{gen} (GeV/#it{c})");
    response->GetYaxis()->SetTitle("#it{p}_{T}^{rec} (GeV/#it{c})");
    for (int i = 0; i < nTracks; ++i) {
      const double ptTrue = ptFunc.GetRandom();
      const double eta = gRandom->Uniform(-0.9, 0.9);
      const double phi = gRandom->Uniform(0., 2. * TMath::Pi());
      ptGen->Fill(ptTrue);
      // tracking efficiency rises steeply at low pT; one TPC sector (of 18) is switched off in data
      const double sector = std::floor(phi / (2. * TMath::Pi() / 18.));
      const double efficiency = 0.82 * (1. - std::exp(-(ptTrue - 0.1) / 0.18)) * ((!isMC && sector == 5.) ? 0. : 1.);
      if (gRandom->Rndm() > efficiency) continue;
      // relative pT resolution grows linearly with pT
      const double ptRec = ptTrue * (1. + gRandom->Gaus(0., 0.01 + 0.004 * ptTrue));
      etaPhi->Fill(eta, phi);
      pt->Fill(ptRec);
      response->Fill(ptTrue, ptRec);
    }
    output.Add(etaPhi);
    output.Add(pt);
    if (isMC) {
      output.Add(ptGen);
      output.Add(response);
    } else {
      delete ptGen;
      delete response;
    }
    writeTask("TrackQA", output);
  }
  {  // particle identification: TPC energy loss and TOF velocity vs. momentum
    TList output;
    output.SetOwner();
    const auto pBins = LogBins(150, 0.15, 10.);
    auto* dEdx = new TH2D("hTPCdEdxVsP", "", pBins.size() - 1, pBins.data(), 200, 20., 220.);
    dEdx->GetXaxis()->SetTitle("#it{p} (GeV/#it{c})");
    dEdx->GetYaxis()->SetTitle("TPC d#it{E}/d#it{x} (arb. units)");
    dEdx->GetZaxis()->SetTitle("tracks");
    auto* beta = new TH2D("hTOFBetaVsP", "", pBins.size() - 1, pBins.data(), 200, 0.3, 1.1);
    beta->GetXaxis()->SetTitle("#it{p} (GeV/#it{c})");
    beta->GetYaxis()->SetTitle("TOF #beta");
    beta->GetZaxis()->SetTitle("tracks");
    for (int i = 0; i < 2 * nTracks; ++i) {
      const auto& species = RandomSpecies();
      const double p = ptFunc.GetRandom(0.15, 10.) * 1.1;
      dEdx->Fill(p, TPCdEdx(p, species.mass) * (1. + gRandom->Gaus(0., 0.06)));
      // 3.7 m flight path, 80 ps time resolution
      const double b = p / std::sqrt(p * p + species.mass * species.mass);
      const double tof = 3.7 / (b * 0.299792458) + gRandom->Gaus(0., 0.08);
      if (p > 0.3) beta->Fill(p, 3.7 / (tof * 0.299792458));
    }
    output.Add(dEdx);
    output.Add(beta);
    writeTask("PIDQA", output);
  }
  {  // calorimeter: two-photon invariant mass with pi0 and eta peaks on a combinatorial background
    TList output;
    output.SetOwner();
    auto* invMass = new TH1D("hInvMassGG", "", 160, 0., 0.8);
    invMass->GetXaxis()->SetTitle("#it{m}_{#gamma#gamma} (GeV/#it{c}^{2})");
    invMass->GetYaxis()->SetTitle("pairs");
    TF1 background("combinatorial", "x*exp(-x/0.12)", 0., 0.8);
    for (int i = 0; i < 400000; ++i) invMass->Fill(background.GetRandom());
    const double resolution = isMC ? 0.010 : 0.012;
    for (int i = 0; i < 40000; ++i) invMass->Fill(gRandom->Gaus(0.135, resolution));
    for (int i = 0; i < 10000; ++i) invMass->Fill(gRandom->Gaus(0.548, 2.5 * resolution));
    output.Add(invMass);
    // the same pairs in bins of their pT: at high pT there are fewer pi0, but the peak gets narrower and stands out a bit more; it moves up with the
    // calorimeter non-linearity (stronger in data); an own random generator keeps all other example data unchanged
    TRandom3 random(isMC ? 7 : 8);
    const std::vector<double> pairPtBins = {1., 1.5, 2., 3., 4., 6., 10.};
    auto* invMassVsPt = new TH2D("hInvMassGGVsPt", "", 120, 0., 0.3, pairPtBins.size() - 1, pairPtBins.data());
    invMassVsPt->GetXaxis()->SetTitle("#it{m}_{#gamma#gamma} (GeV/#it{c}^{2})");
    invMassVsPt->GetYaxis()->SetTitle("pair #it{p}_{T} (GeV/#it{c})");
    invMassVsPt->GetZaxis()->SetTitle("pairs");
    // combinatorial background m exp(-m / T) (a sum of two exponentials): falls steeply with pT, its maximum moves up with pT
    auto combinatorial = [&random](double& mass, double& pairPt) {
      pairPt = 1. + random.Exp(0.9);
      mass = (0.035 + 0.02 * pairPt) * (random.Exp(1.) + random.Exp(1.));
    };
    double mass{}, pairPt{};
    for (int i = 0; i < 1500000; ++i) {
      combinatorial(mass, pairPt);
      invMassVsPt->Fill(mass, pairPt);
    }
    for (int i = 0; i < 300000; ++i) {  // pi0: falls with pT a little slower than the background, resolution improving with pT
      pairPt = 1. + random.Exp(1.05);
      const double width = (isMC ? 0.0085 : 0.010) * (0.6 + 0.5 / std::sqrt(pairPt));
      const double peak = 0.135 * (1. + (isMC ? 0.002 : 0.006) * std::log(pairPt));
      invMassVsPt->Fill(random.Gaus(peak, width), pairPt);
    }
    output.Add(invMassVsPt);
    // the background estimate from event mixing (photons of different events): the combinatorial shape only, with more pairs
    auto* invMassVsPtMixed = static_cast<TH2D*>(invMassVsPt->Clone("hInvMassGGVsPtMixed"));
    invMassVsPtMixed->Reset();
    for (int i = 0; i < 6000000; ++i) {
      combinatorial(mass, pairPt);
      invMassVsPtMixed->Fill(mass, pairPt);
    }
    output.Add(invMassVsPtMixed);
    writeTask("CaloQA", output);
  }
}

// --------------------------------------------------------------------------------------------------
// tracks.root
// --------------------------------------------------------------------------------------------------
void WriteTracks(TF1& ptFunc)
{
  TFile file((kFolder + "tracks.root").data(), "RECREATE");
  TTree tree("tracks", "reconstructed tracks");
  float pt{}, eta{}, phi{}, dcaXY{};
  int charge{}, nClustersTPC{};
  bool isPrimary{};
  tree.Branch("pt", &pt);
  tree.Branch("eta", &eta);
  tree.Branch("phi", &phi);
  tree.Branch("charge", &charge);
  tree.Branch("nClustersTPC", &nClustersTPC);
  tree.Branch("dcaXY", &dcaXY);
  tree.Branch("isPrimary", &isPrimary);
  for (int i = 0; i < 40000; ++i) {
    pt = ptFunc.GetRandom();
    eta = gRandom->Uniform(-0.9, 0.9);
    phi = gRandom->Uniform(0., 2. * TMath::Pi());
    charge = (gRandom->Rndm() < 0.5) ? -1 : 1;
    // tracks at large |eta| leave the TPC early and have fewer clusters
    nClustersTPC = std::min(159, static_cast<int>(gRandom->Gaus(150. - 40. * std::pow(std::abs(eta), 3), 8.)));
    // secondaries (weak decays, material) have a wide distance-of-closest-approach distribution
    isPrimary = gRandom->Rndm() < 0.9;
    const double dcaResolution = 0.002 + 0.003 / pt;  // cm
    dcaXY = isPrimary ? gRandom->Gaus(0., dcaResolution) : gRandom->Gaus(0., 0.3) * gRandom->Exp(1.);
    tree.Fill();
  }
  tree.Write();
}

// --------------------------------------------------------------------------------------------------
// csv tables
// --------------------------------------------------------------------------------------------------
void WriteTables(TF1& ptData)
{
  {  // a published measurement in HEPData style, with statistical and systematic uncertainties
    const double norm = 300000. / (ptData.Integral(0.1, 20.) * 100000.);
    std::ofstream table(kFolder + "ptSpec_published.csv");
    table << "pt,pt_err,value,stat,syst\n";
    for (double pt : {0.15, 0.25, 0.35, 0.5, 0.7, 0.9, 1.25, 1.75, 2.5, 3.5, 5.0, 7.0, 10.0, 14.0}) {
      const double value = norm * ptData.Eval(pt) * (1. + gRandom->Gaus(0., 0.02));
      table << pt << ",0," << value << "," << 0.01 * value * (1. + pt / 4.) << "," << 0.06 * value << "\n";
    }
  }
  {  // calorimeter test beam: sigma_E / E = 10%/sqrt(E) (+) 1% (+) 5%/E
    std::ofstream table(kFolder + "testbeam.csv");
    table << "energy,resolution,resolution_err\n";
    for (double energy : {0.5, 1., 2., 3., 5., 8., 10., 15., 20., 30., 50., 80., 100.}) {
      const double resolution = std::sqrt(std::pow(0.10 / std::sqrt(energy), 2) + std::pow(0.01, 2) + std::pow(0.05 / energy, 2));
      table << energy << "," << resolution * (1. + gRandom->Gaus(0., 0.02)) << "," << 0.02 * resolution << "\n";
    }
  }
  {  // integrated luminosity delivered per week of a data-taking year (unix time)
    std::ofstream table(kFolder + "luminosity.csv");
    table << "time,lumi\n";
    const double start = 1680307200.;  // 2023-04-01
    double lumi = 0.;
    for (int week = 0; week < 30; ++week) {
      const double ramp = std::min(1., week / 8.);           // intensity ramp-up
      const bool technicalStop = (week == 12 || week == 13);  // no beam
      if (!technicalStop) lumi += ramp * gRandom->Uniform(1.2, 1.8);
      table << static_cast<long>(start + week * 7 * 86400) << "," << lumi << "\n";
    }
  }
  {  // a published nuclear modification factor, 0-10%, with asymmetric systematic uncertainties
    std::ofstream table(kFolder + "raa_published.csv");
    table << "pt,pt_err,raa,stat,syst_low,syst_high\n";
    for (double pt : {0.6, 1.0, 1.5, 2.5, 3.5, 5.0, 7.0, 9.0, 12.0, 16.0}) {
      const double supp = 0.85 * std::exp(-std::pow(std::log(pt / 7.) / 1.6, 2));
      const double raa = ((1. - supp) * (1. - std::exp(-pt / 1.2)) + 0.25 * std::exp(-pt / 1.2)) * (1. + gRandom->Gaus(0., 0.03));
      table << pt << "," << 0.1 * pt << "," << raa << "," << 0.02 * raa << "," << 0.08 * raa << "," << 0.12 * raa << "\n";
    }
  }
  {  // run-by-run QA: mean multiplicity per run, two runs with a detector problem
    std::ofstream table(kFolder + "runQA.csv");
    table << "run,meanNch,meanNch_err\n";
    int run = 544013;
    for (int i = 0; i < 60; ++i) {
      run += 1 + static_cast<int>(gRandom->Exp(6.));
      double mean = 9.6 * (1. + gRandom->Gaus(0., 0.004));
      if (i == 17 || i == 42) mean *= 0.93;  // TPC sector off
      table << run << "," << mean << "," << 0.02 << "\n";
    }
  }
  {  // relative systematic uncertainties of the pT spectrum (in %) per source
    std::ofstream table(kFolder + "systematics.csv");
    table << "pt,tracking,pid,material,normalisation,total\n";
    for (double pt = 0.15; pt < 20.; pt *= 1.35) {
      const double tracking = 2.0 + 1.5 * std::exp(-pt / 0.3) + 0.15 * pt;
      const double pid = 1.0 + 2.0 * std::exp(-std::pow(std::log(pt / 2.) / 0.6, 2));
      const double material = 3.0 * std::exp(-pt / 0.5) + 0.3;
      const double normalisation = 2.5;
      const double total = std::sqrt(tracking * tracking + pid * pid + material * material + normalisation * normalisation);
      table << pt << "," << tracking << "," << pid << "," << material << "," << normalisation << "," << total << "\n";
    }
  }
}

// --------------------------------------------------------------------------------------------------
// dimuon.root
// --------------------------------------------------------------------------------------------------
void WriteDimuon()
{
  TFile file((kFolder + "dimuon.root").data(), "RECREATE");
  // continuum plus resonances (mass, relative resolution, yield), shaped like the classic dimuon spectrum
  struct Resonance {
    double mass, width, yield;
  };
  const std::vector<Resonance> resonances = {{0.782, 0.012, 2.0e5}, {1.019, 0.012, 2.5e5}, {3.097, 0.010, 1.2e6}, {3.686, 0.010, 4.0e4}, {9.460, 0.010, 1.2e5}, {10.023, 0.010, 3.5e4}, {10.355, 0.010, 1.8e4}, {91.19, 0.027, 4.0e4}};
  auto density = [&](double m) {
    double value = 1.5e6 * std::pow(m, -3.2) * (1. - std::exp(-std::pow(m / 0.5, 3)));
    for (const auto& r : resonances) value += r.yield * TMath::Gaus(m, r.mass, r.width * r.mass, true);
    return value;
  };
  const auto edges = LogBins(400, 0.3, 150.);
  auto* hist = new TH1D("mMuMu", "", edges.size() - 1, edges.data());
  hist->GetXaxis()->SetTitle("#it{m}_{#mu#mu} (GeV)");
  hist->GetYaxis()->SetTitle("events / bin");
  for (int i = 1; i <= hist->GetNbinsX(); ++i) {
    double expected = 0.;
    const int nSteps = 20;
    for (int k = 0; k < nSteps; ++k) {
      const double m = hist->GetBinLowEdge(i) + (k + 0.5) * hist->GetBinWidth(i) / nSteps;
      expected += density(m) * hist->GetBinWidth(i) / nSteps;
    }
    const double observed = gRandom->Poisson(expected);
    hist->SetBinContent(i, observed);
    hist->SetBinError(i, std::sqrt(observed));
  }
  hist->Write();
}

// --------------------------------------------------------------------------------------------------
// jets/jets_R0*.root
// --------------------------------------------------------------------------------------------------
void WriteJets()
{
  gSystem->mkdir((kFolder + "jets").data(), true);
  const auto edges = LogBins(30, 20., 500.);
  for (const auto& [tag, radius] : std::vector<std::pair<std::string, double>>{{"R02", 0.2}, {"R04", 0.4}, {"R06", 0.6}}) {
    TFile file((kFolder + "jets/jets_" + tag + ".root").data(), "RECREATE");
    // larger radii collect more of the jet energy: the spectrum shifts to higher pT
    TF1 spectrum("jetSpectrum", "[0]*pow(x, -[1])*exp(-x/[2])", 20., 500.);
    spectrum.SetParameters(4.e13 * std::pow(radius / 0.4, 0.6), 5.0, 450.);
    auto* hist = new TH1D(("hJetPt_" + tag).data(), "", edges.size() - 1, edges.data());
    hist->GetXaxis()->SetTitle("#it{p}_{T}^{jet} (GeV/#it{c})");
    hist->GetYaxis()->SetTitle("d#it{N}/d#it{p}_{T} (GeV/#it{c})^{-1}");
    for (int i = 1; i <= hist->GetNbinsX(); ++i) {
      const double expected = spectrum.Integral(hist->GetBinLowEdge(i), hist->GetBinLowEdge(i + 1));
      const double observed = gRandom->Poisson(expected);
      hist->SetBinContent(i, observed / hist->GetBinWidth(i));
      hist->SetBinError(i, std::sqrt(observed) / hist->GetBinWidth(i));
    }
    hist->Write();
    if (radius == 0.4) {
      // a theory curve over a wider range than the data
      TF1 nlo("fNLO_R04", "[0]*pow(x, -[1])*exp(-x/[2])", 10., 800.);
      nlo.SetParameters(4.3e13, 5.0, 450.);
      nlo.SetNpx(1000);
      nlo.Write();
      // jet energy response: rec/gen, slightly below 1 and sharper at high pT
      auto* response = new TH2D("hJESResponse", "", 48, 20., 500., 60, 0.4, 1.6);
      response->GetXaxis()->SetTitle("#it{p}_{T}^{gen} (GeV/#it{c})");
      response->GetYaxis()->SetTitle("#it{p}_{T}^{rec} / #it{p}_{T}^{gen}");
      response->GetZaxis()->SetTitle("jets");
      for (int i = 0; i < 300000; ++i) {
        const double ptGen = 20. * std::pow(25., gRandom->Rndm());
        const double scale = 0.93 + 0.05 * (1. - std::exp(-ptGen / 150.));
        const double resolution = std::sqrt(std::pow(0.9 / std::sqrt(ptGen), 2) + std::pow(0.05, 2));
        response->Fill(ptGen, gRandom->Gaus(scale, resolution));
      }
      response->Write();
    }
  }
}

// --------------------------------------------------------------------------------------------------
// heavyion.root
// --------------------------------------------------------------------------------------------------
struct CentralityClass {
  std::string name;
  double ncoll;      // mean number of binary collisions (illustrative)
  double depth;      // strength of the suppression
  double v2max;      // maximum elliptic flow
};
const std::vector<CentralityClass> kCentralities = {{"0-10", 1500., 0.85, 0.08}, {"10-30", 750., 0.70, 0.15}, {"30-50", 260., 0.45, 0.20}};

void WriteHeavyIon(TF1& ptPP)
{
  TFile file((kFolder + "heavyion.root").data(), "RECREATE");
  // per-event pp yield density, normalised like spectra.root:data/ptSpec
  const double ppNorm = 3. / ptPP.Integral(0.1, 20.);
  auto raaModel = [](double pt, double depth) {
    // suppression strongest around 7 GeV, weaker at low and high pT
    const double supp = depth * std::exp(-std::pow(std::log(pt / 7.) / 1.6, 2));
    return (1. - supp) * (1. - std::exp(-pt / 1.2)) + 0.25 * std::exp(-pt / 1.2);
  };
  for (const auto& cent : kCentralities) {
    file.mkdir(("cent_" + cent.name).data())->cd();
    const double nEvents = 200000.;
    auto* spec = new TH1D("ptSpec", "", kPtBins.size() - 1, kPtBins.data());
    spec->GetXaxis()->SetTitle("#it{p}_{T} (GeV/#it{c})");
    spec->GetYaxis()->SetTitle("1/#it{N}_{ev} d#it{N}/d#it{p}_{T} (GeV/#it{c})^{-1}");
    for (int i = 1; i <= spec->GetNbinsX(); ++i) {
      const double pt = spec->GetBinCenter(i), width = spec->GetBinWidth(i);
      const double perEvent = cent.ncoll * ppNorm * ptPP.Eval(pt) * raaModel(pt, cent.depth);
      const double counts = gRandom->Poisson(perEvent * width * nEvents);
      spec->SetBinContent(i, counts / (width * nEvents));
      spec->SetBinError(i, std::sqrt(counts) / (width * nEvents));
    }
    spec->Write();

    const int nPoints = 14;
    TGraphErrors v2(nPoints);
    for (int i = 0; i < nPoints; ++i) {
      const double pt = 0.3 + 0.5 * i;
      const double value = cent.v2max * (pt / 2.5) * std::exp(1. - pt / 2.5) + gRandom->Gaus(0., 0.003);
      v2.SetPoint(i, pt, value);
      v2.SetPointError(i, 0., 0.004 + 0.002 * pt);
    }
    v2.GetXaxis()->SetTitle("#it{p}_{T} (GeV/#it{c})");
    v2.GetYaxis()->SetTitle("#it{v}_{2}");
    v2.Write("v2");
  }
  file.cd();
  // centrality estimator: forward-detector amplitude, falling with a knee for the most central events
  auto* v0m = new TH1D("hV0M", "", 250, 0., 25000.);
  v0m->GetXaxis()->SetTitle("V0M amplitude (a.u.)");
  v0m->GetYaxis()->SetTitle("events");
  TF1 amplitude("amplitude", "1./(x+300.)^0.8 * 0.5*(1.-TMath::Erf((x-20000.)/1500.))", 0., 25000.);
  amplitude.SetNpx(2000);
  for (int i = 0; i < 500000; ++i) v0m->Fill(amplitude.GetRandom());
  v0m->Write();
  // multi-dimensional histogram: pT, eta and centrality
  const int nBins[3] = {40, 16, 10};
  const double xMin[3] = {0., -0.8, 0.};
  const double xMax[3] = {10., 0.8, 100.};
  auto* sparse = new THnSparseD("hPtEtaCent", "", 3, nBins, xMin, xMax);
  sparse->GetAxis(0)->SetTitle("#it{p}_{T} (GeV/#it{c})");
  sparse->GetAxis(1)->SetTitle("#eta");
  sparse->GetAxis(2)->SetTitle("centrality (%)");
  for (int i = 0; i < 400000; ++i) {
    const double centrality = gRandom->Uniform(0., 100.);
    const double eta = gRandom->Uniform(-0.8, 0.8);
    // spectra harden towards central collisions and are slightly softer at larger |eta|
    const double meanPt = (0.75 - 0.002 * centrality) * (1. - 0.1 * std::abs(eta));
    const double x[3] = {gRandom->Exp(meanPt / 2.) + gRandom->Exp(meanPt / 2.), eta, centrality};
    sparse->Fill(x);
  }
  sparse->Write();
}
// --------------------------------------------------------------------------------------------------
// decays.root
// --------------------------------------------------------------------------------------------------
void WriteDecays()
{
  TFile file((kFolder + "decays.root").data(), "RECREATE");
  TRandom3 random(11);  // an own generator keeps all other example data unchanged
  const double mPi = 0.13957, mProton = 0.938272, mElectron = 0.000511;

  // V0 candidates (two-body decays of neutral particles) for the Armenteros-Podolanski plot: K0S -> pi+ pi-,
  // Lambda -> p pi-, anti-Lambda -> anti-p pi+, photon conversions gamma -> e+ e-, and random pairs
  TTree v0s("v0s", "V0 candidates");
  float alpha{}, qT{}, pt{};
  v0s.Branch("alpha", &alpha);
  v0s.Branch("qT", &qT);
  v0s.Branch("pt", &pt);
  struct V0Type {
    double fraction, mass, mPos, mNeg;
  };
  const std::vector<V0Type> types = {{0.45, 0.497611, mPi, mPi}, {0.25, 1.115683, mProton, mPi}, {0.18, 1.115683, mPi, mProton}, {0.12, 0., mElectron, mElectron}};
  for (int i = 0; i < 50000; ++i) {
    pt = 0.3 + random.Exp(1.2);
    if (random.Rndm() < 0.12) {  // random pairs
      alpha = random.Uniform(-1., 1.);
      qT = random.Exp(0.06);
      if (qT < 0.25) v0s.Fill();
      continue;
    }
    double r = random.Rndm();
    size_t type = 0;
    while (type + 1 < types.size() && r > types[type].fraction) r -= types[type++].fraction;
    const auto& v0 = types[type];
    // decay momentum in the rest frame; a conversion has (almost) none
    const double pStar = (v0.mass > 0.) ? std::sqrt((v0.mass * v0.mass - std::pow(v0.mPos + v0.mNeg, 2)) * (v0.mass * v0.mass - std::pow(v0.mPos - v0.mNeg, 2))) / (2. * v0.mass) : 0.;
    const double cosTheta = random.Uniform(-1., 1.);
    if (v0.mass > 0.) {
      const double energy = std::sqrt(pt * pt + v0.mass * v0.mass), beta = pt / energy, gamma = energy / v0.mass;
      const double ePos = std::sqrt(pStar * pStar + v0.mPos * v0.mPos), eNeg = std::sqrt(pStar * pStar + v0.mNeg * v0.mNeg);
      const double pLPos = gamma * (pStar * cosTheta + beta * ePos), pLNeg = gamma * (-pStar * cosTheta + beta * eNeg);
      alpha = (pLPos - pLNeg) / (pLPos + pLNeg) + random.Gaus(0., 0.01);
      qT = pStar * std::sqrt(1. - cosTheta * cosTheta) + random.Gaus(0., 0.003);
    } else {
      alpha = cosTheta * 0.95 + random.Gaus(0., 0.01);
      qT = std::abs(random.Gaus(0., 0.004));
    }
    v0s.Fill();
  }
  v0s.Write();

  // D0 -> K0S pi+ pi- for a Dalitz plot: phase space with the resonances K*(892)- -> K0S pi-, rho(770) -> pi+ pi-
  // and a little K*(892)+ (doubly Cabibbo suppressed), and a non-resonant part
  TTree d0("d0ToK0sPiPi", "D0 -> K0S pi+ pi- candidates");
  float m2KsPip{}, m2KsPim{}, m2PiPi{};
  d0.Branch("m2KsPip", &m2KsPip);
  d0.Branch("m2KsPim", &m2KsPim);
  d0.Branch("m2PiPi", &m2PiPi);
  const double mD0 = 1.86484, mK0s = 0.497611;
  TLorentzVector mother(0., 0., 0., mD0);
  const double masses[3] = {mK0s, mPi, mPi};
  TGenPhaseSpace phaseSpace;
  phaseSpace.SetDecay(mother, 3, masses);
  using cplx = std::complex<double>;
  auto breitWigner = [](double s, double mass, double width) { return 1. / cplx(mass * mass - s, -mass * width); };
  auto weight = [&](double sKsPip, double sKsPim, double sPiPi) {
    const cplx amplitude = cplx(8., 0.) + 1.0 * breitWigner(sKsPim, 0.89166, 0.0508) + std::polar(2.5, 1.2) * breitWigner(sPiPi, 0.77526, 0.1491) +
                           std::polar(0.12, -0.8) * breitWigner(sKsPip, 0.89166, 0.0508);
    return std::norm(amplitude);
  };
  double maxWeight = 0.;
  for (int pass = 0; pass < 2; ++pass) {  // the first pass finds the largest weight for the accept-reject of the second
    int accepted = 0;
    while (pass == 0 ? accepted++ < 200000 : accepted < 30000) {
      const double w = phaseSpace.Generate() * 1.;
      const TLorentzVector kS = *phaseSpace.GetDecay(0), piPlus = *phaseSpace.GetDecay(1), piMinus = *phaseSpace.GetDecay(2);
      const double sKsPip = (kS + piPlus).M2(), sKsPim = (kS + piMinus).M2(), sPiPi = (piPlus + piMinus).M2();
      const double total = w * weight(sKsPip, sKsPim, sPiPi);
      if (pass == 0) {
        maxWeight = std::max(maxWeight, total);
      } else if (random.Rndm() * 1.1 * maxWeight < total) {
        m2KsPip = sKsPip;
        m2KsPim = sKsPim;
        m2PiPi = sPiPi;
        d0.Fill();
        ++accepted;
      }
    }
  }
  d0.Write();
}
// --------------------------------------------------------------------------------------------------
// ml.root
// --------------------------------------------------------------------------------------------------
void WriteML()
{
  TFile file((kFolder + "ml.root").data(), "RECREATE");
  TRandom3 random(12);  // an own generator keeps all other example data unchanged
  // classifier scores in [0, 1] of signal and background candidates, for the training and the test sample;
  // the training sample is separated a little better than the test sample (slight overtraining)
  auto score = [&random](double mean, double width) { return 1. / (1. + std::exp(-random.Gaus(mean, width))); };
  struct Classifier {
    const char* name;
    double separation;
  };
  for (const Classifier& c : {Classifier{"BDT", 1.5}, Classifier{"DNN", 1.8}}) {
    std::map<std::string, TH1D*> hists;
    for (const char* cls : {"sig", "bkg"}) {
      for (const char* sample : {"train", "test"}) {
        auto* h = new TH1D(Form("h%s_%s_%s", c.name, cls, sample), "", 50, 0., 1.);
        h->GetXaxis()->SetTitle(Form("%s score", c.name));
        h->GetYaxis()->SetTitle("candidates");
        const bool isSignal = std::string(cls) == "sig", isTrain = std::string(sample) == "train";
        const double mean = (isSignal ? 1. : -1.) * (c.separation + (isTrain ? 0.15 : 0.));
        for (int i = 0; i < (isSignal ? 20000 : 60000); ++i) h->Fill(score(mean, 1.3));
        hists[Form("%s_%s", cls, sample)] = h;
      }
    }
    // ROC curve from the test samples: background rejection vs. signal efficiency for every cut on the score;
    // the area under the curve goes into the title, where a label can show it with <title>
    const TH1D* sig = hists["sig_test"];
    const TH1D* bkg = hists["bkg_test"];
    TGraph roc;
    double auc = 0., lastEff = 0., lastRej = 1.;
    for (int bin = sig->GetNbinsX() + 1; bin >= 0; --bin) {
      const double effSig = sig->Integral(bin, sig->GetNbinsX() + 1) / sig->Integral(0, sig->GetNbinsX() + 1);
      const double rejBkg = 1. - bkg->Integral(bin, bkg->GetNbinsX() + 1) / bkg->Integral(0, bkg->GetNbinsX() + 1);
      roc.AddPoint(effSig, rejBkg);
      auc += (effSig - lastEff) * 0.5 * (rejBkg + lastRej);
      lastEff = effSig;
      lastRej = rejBkg;
    }
    roc.SetTitle(Form("%s (AUC = %.3f)", c.name, auc));
    roc.GetXaxis()->SetTitle("signal efficiency");
    roc.GetYaxis()->SetTitle("background rejection");
    roc.Write(Form("roc_%s", c.name));
    for (auto& [key, h] : hists) {
      h->Write();
      delete h;
    }
  }
  // importance of the input features of the BDT (labelled bins)
  const std::vector<std::pair<const char*, double>> features = {
    {"#it{p}_{T}", 0.24}, {"DCA_{#it{xy}}", 0.19}, {"cos(#theta_{p})", 0.16}, {"#it{L}_{xy}", 0.13}, {"#chi^{2}_{vtx}", 0.1}, {"TPC n#sigma_{#pi}", 0.08}, {"TOF n#sigma_{K}", 0.06}, {"#eta", 0.04}};
  auto* importance = new TH1D("hFeatureImportance", "", features.size(), 0., features.size());
  for (size_t i = 0; i < features.size(); ++i) {  // most important on top in a horizontal bar chart
    importance->GetXaxis()->SetBinLabel(features.size() - i, features[i].first);
    importance->SetBinContent(features.size() - i, features[i].second);
  }
  importance->GetYaxis()->SetTitle("relative importance");
  importance->Write();
  delete importance;
}

void WriteMedical()
{
  TFile file((kFolder + "medical.root").data(), "RECREATE");
  TRandom3 random(13);  // an own generator keeps all other example data unchanged
  file.mkdir("depthDose")->cd();
  // depth-dose curve of an ion beam in water: the stopping power at residual range r grows as r^(1/p - 1) (Bragg-Kleeman rule,
  // p = 1.77, range = 0.022 mm x (E/MeV per nucleon)^1.77, a third of it for carbon ions), averaged over the straggling of
  // the range; nuclear reactions remove about 1% of the ions per cm, those of carbon leave lighter fragments beyond the peak
  auto depthDose = [&random](const char* name, double range, double straggling, double fragments) {
    TH1D h(name, "", 300, 0., 300.);
    h.GetXaxis()->SetTitle("depth in water (mm)");
    h.GetYaxis()->SetTitle("relative dose (%)");
    constexpr double p = 1.77;
    constexpr int kSteps = 400;
    for (int bin = 1; bin <= h.GetNbinsX(); ++bin) {
      const double z = h.GetBinCenter(bin);
      double dose = 0.;
      for (int k = 0; k < kSteps; ++k) {  // Gaussian range straggling, in equal steps of its cumulative distribution
        const double residual = range + straggling * TMath::NormQuantile((k + 0.5) / kSteps) - z;
        if (residual > 0.5) dose += std::pow(residual, 1. / p - 1.) * (1. - 0.001 * z);  // half a bin keeps the peak finite
      }
      h.SetBinContent(bin, dose / kSteps);
    }
    h.Scale(100. / h.GetMaximum());
    for (int bin = 1; bin <= h.GetNbinsX(); ++bin) {
      const double z = h.GetBinCenter(bin);
      double dose = h.GetBinContent(bin) + fragments * std::exp(-(z - range) / 80.) * 0.5 * (1. + TMath::Erf((z - range) / straggling));
      dose *= random.Gaus(1., 0.004);  // statistical fluctuations of the simulation
      h.SetBinContent(bin, dose);
      h.SetBinError(bin, 0.004 * dose);
    }
    h.Write();
  };
  auto range = [](double energyPerNucleon) { return 0.022 * std::pow(energyPerNucleon, 1.77); };              // protons, mm
  auto straggling = [](double rangeMM) { return std::hypot(0.12 * std::pow(rangeMM / 10., 0.935), 1.); };  // and 1 mm energy spread
  for (int energy : {100, 150, 200}) {
    depthDose(Form("proton_%dMeV", energy), range(energy), straggling(range(energy)), 0.);
  }
  const double carbonRange = range(280.) / 3.;
  depthDose("carbon_280MeVu", carbonRange, std::hypot(0.12 * std::pow(carbonRange / 10., 0.935) / std::sqrt(12.), 0.5), 9.);

  // dose of a two-field proton plan in a transverse slice: each field covers the target (an ellipse) with half the prescribed
  // dose, enters with half of that in front of the target and stops behind it, without exit dose
  file.mkdir("plan")->cd();
  TH2D doseMap("doseMap", "", 100, -100., 100., 100, -100., 100.);
  doseMap.GetXaxis()->SetTitle("#it{x} (mm)");
  doseMap.GetYaxis()->SetTitle("#it{y} (mm)");
  doseMap.GetZaxis()->SetTitle("dose (% of prescribed)");
  constexpr double x0 = 10., y0 = -5., a = 32., b = 24.;  // target centre and half axes, mm
  constexpr double edge = 3.5;                           // width of the dose fall-off, mm
  auto step = [](double u) { return 0.5 * (1. + TMath::Erf(u)); };
  // one field along the depth d (lateral position l) through an ellipse of half axes along (ad) and across (al) the beam
  auto field = [&step](double d, double l, double d0, double l0, double ad, double al) {
    const double lateral = step((al - std::abs(l - l0)) / edge);
    const double half = ad * std::sqrt(std::max(0., 1. - std::pow((l - l0) / al, 2.)));
    const double entrance = 0.5 + 0.5 * step((d - (d0 - half)) / edge);  // plateau in front of the target, full dose inside it
    return 50. * lateral * entrance * step(((d0 + half) - d) / edge);   // no dose behind the distal edge
  };
  for (int ix = 1; ix <= doseMap.GetNbinsX(); ++ix) {
    for (int iy = 1; iy <= doseMap.GetNbinsY(); ++iy) {
      const double x = doseMap.GetXaxis()->GetBinCenter(ix), y = doseMap.GetYaxis()->GetBinCenter(iy);
      const double dose = field(x, y, x0, y0, a, b)      // field 1 from the left, along x
                          + field(-y, x, -y0, x0, b, a);  // field 2 from the top, along -y
      if (dose > 1.) doseMap.SetBinContent(ix, iy, dose * random.Gaus(1., 0.005));  // below 1% nothing is scored
    }
  }
  doseMap.Write();
}

void WriteAtomic()
{
  TFile file((kFolder + "atomic.root").data(), "RECREATE");
  TRandom3 random(14);  // an own generator keeps all other example data unchanged
  // collinear laser spectroscopy: fluorescence counts while the laser frequency is scanned over four hyperfine components
  // (Voigt profiles: Doppler and natural width), on a flat background of scattered laser light
  const char* lines = "[0] + [1]*(0.55*TMath::Voigt(x+1820.-[2],[3],[4]) + 0.35*TMath::Voigt(x+1240.-[2],[3],[4])"
                      " + TMath::Voigt(x-610.-[2],[3],[4]) + 0.6*TMath::Voigt(x-1390.-[2],[3],[4]))";
  TF1 truth("truth", lines, -3000., 3000.);
  truth.SetParameters(40., 60000., 12., 38., 22.);  // background, line strength, centroid shift (MHz), Doppler sigma, natural FWHM
  TH1D scan("hScan", "", 300, -3000., 3000.);
  scan.GetXaxis()->SetTitle("laser frequency offset (MHz)");
  scan.GetYaxis()->SetTitle("counts per 20 MHz");
  for (int bin = 1; bin <= scan.GetNbinsX(); ++bin) {
    const double counts = random.Poisson(truth.Eval(scan.GetBinCenter(bin)));
    scan.SetBinContent(bin, counts);
    scan.SetBinError(bin, std::sqrt(std::max(counts, 1.)));
  }
  scan.Write();
  TF1 fit("fitScan", lines, -3000., 3000.);
  fit.SetParameters(35., 50000., 0., 30., 15.);
  scan.Fit(&fit, "Q0N");
  fit.SetNpx(2000);
  fit.Write();
}

void WriteAccelerator()
{
  TFile file((kFolder + "accelerator.root").data(), "RECREATE");
  TRandom3 random(15);  // an own generator keeps all other example data unchanged
  // transverse beam profile on a screen: a flat Gaussian core with a faint halo around it
  TH2D spot("hBeamSpot", "", 120, -6., 6., 120, -6., 6.);
  spot.GetXaxis()->SetTitle("#it{x} (mm)");
  spot.GetYaxis()->SetTitle("#it{y} (mm)");
  spot.GetZaxis()->SetTitle("intensity (a.u.)");
  for (int i = 0; i < 400000; ++i) {
    const bool halo = random.Uniform() < 0.05;
    spot.Fill(random.Gaus(0.3, halo ? 3.0 : 1.4), random.Gaus(-0.2, halo ? 1.6 : 0.7));
  }
  spot.Write();
  // horizontal phase space of the particles of a bunch, from the Twiss parameters alpha, beta and the emittance:
  // x = sqrt(eps beta) u, x' = sqrt(eps / beta) (v - alpha u) with u, v standard normal
  constexpr double alpha = -1.2, beta = 8., emittance = 0.5;  // beta in mm/mrad (= m), emittance in mm mrad
  TTree tree("beam", "particles of a bunch");
  double x{}, xp{};
  tree.Branch("x", &x);
  tree.Branch("xp", &xp);
  double sxx = 0., spp = 0., sxp = 0.;
  constexpr int kParticles = 30000;
  for (int i = 0; i < kParticles; ++i) {
    const double u = random.Gaus(), v = random.Gaus();
    x = std::sqrt(emittance * beta) * u;
    xp = std::sqrt(emittance / beta) * (v - alpha * u);
    sxx += x * x;
    spp += xp * xp;
    sxp += x * xp;
    tree.Fill();
  }
  tree.Write();
  // the rms ellipse gamma x^2 + 2 alpha x x' + beta x'^2 = eps, with the emittance measured from the particles in its title
  const double measured = std::sqrt(sxx * spp - sxp * sxp) / kParticles;
  TGraph ellipse;
  for (int i = 0; i <= 200; ++i) {
    const double t = 2. * TMath::Pi() * i / 200.;
    ellipse.AddPoint(std::sqrt(emittance * beta) * std::cos(t), -std::sqrt(emittance / beta) * (alpha * std::cos(t) + std::sin(t)));
  }
  ellipse.SetTitle(Form("%.3f", measured));
  ellipse.Write("rmsEllipse");
}

void WriteOtherFields()
{
  TRandom3 random(16);  // an own generator keeps all other example data unchanged
  {  // monthly atmospheric CO2 since 1960: a growing trend with a seasonal cycle (maximum in May)
    std::ofstream table(kFolder + "co2.csv");
    table << "year,co2,deseasonalized\n";
    for (int year = 1960; year <= 2025; ++year) {
      for (int month = 1; month <= 12; ++month) {
        const double t = year - 1960 + (month - 0.5) / 12.;
        const double trend = 316. + 0.8 * t + 0.0125 * t * t + random.Gaus(0., 0.15);
        const double co2 = trend + 3. * std::sin(2. * TMath::Pi() * (month - 2.) / 12.) + random.Gaus(0., 0.3);
        table << year + (month - 0.5) / 12. << "," << co2 << "," << trend << "\n";
      }
    }
  }
  {  // a star catalogue: main sequence, red giants with their clump and white dwarfs, as colour index B-V and luminosity
    std::ofstream table(kFolder + "stars.csv");
    table << "bv,logL\n";
    for (int i = 0; i < 15000; ++i) {
      const double kind = random.Uniform();
      double bv{}, logL{};
      if (kind < 0.80) {  // main sequence: more cool than hot stars
        bv = -0.25 + 2.05 * std::pow(random.Uniform(), 0.6);
        logL = -3.1 * (bv - 0.65) + random.Gaus(0., 0.25);
      } else if (kind < 0.97) {  // red giant branch, and the red clump at B-V ~ 1, log L ~ 1.7
        const bool clump = random.Uniform() < 0.4;
        bv = clump ? random.Gaus(1.0, 0.05) : random.Uniform(0.85, 1.7);
        logL = clump ? random.Gaus(1.7, 0.08) : 0.6 + 2.2 * (bv - 0.85) + random.Gaus(0., 0.15);
      } else {  // white dwarfs: hot but faint
        bv = random.Uniform(-0.25, 0.6);
        logL = -2.6 - 1.5 * bv + random.Gaus(0., 0.2);
      }
      table << bv << "," << logL << "\n";
    }
  }
  {  // spike times of a neuron in 40 trials: 8 Hz spontaneous rate, a burst of 60 Hz after the stimulus at 0 ms that decays
    std::ofstream table(kFolder + "spikes.csv");
    table << "trial,time\n";
    auto rate = [](double t) { return 8. + ((t > 30.) ? 70. * std::exp(-(t - 30.) / 120.) : 0.); };  // Hz, t in ms
    for (int trial = 1; trial <= 40; ++trial) {
      for (double t = -500.; t < 1000.;) {  // Poisson process by thinning: candidates at the maximum rate
        t += random.Exp(1000. / 78.);
        if (t < 1000. && random.Uniform() < rate(t) / 78.) table << trial << "," << t << "\n";
      }
    }
  }
}
}  // namespace

void generateExampleData()
{
  gRandom = new TRandom3(42);
  gSystem->mkdir(kFolder.data(), true);

  auto ptData = PtSpectrumFunc("ptData", 3.06268642);
  auto ptMC = PtSpectrumFunc("ptMC", 3.25);
  auto multData = MultiplicityFunc("multData", 16.94642265);
  auto multMC = MultiplicityFunc("multMC", 15.5);

  WriteHiggs();
  WriteSpectra(*ptData, *ptMC, *multData, *multMC);
  WriteAnalysisResults(false, *ptData);
  WriteAnalysisResults(true, *ptMC);
  WriteTracks(*ptData);
  WriteTables(*ptData);
  WriteDimuon();
  WriteJets();
  WriteHeavyIon(*ptData);
  WriteDecays();
  WriteML();
  WriteMedical();
  WriteAtomic();
  WriteAccelerator();
  WriteOtherFields();
}
