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
//     CaloQA/output      hInvMassGG
//   dimuon.root                       mMuMu: dimuon invariant mass from J/psi to the Z (raw counts, log bins)
//   jets/jets_R0{2,4,6}.root           jet spectra for three radii (a data source given as a directory)
//     hJetPt_R02 ... hJetPt_R06, [R04 only: fNLO_R04 (TF1), hJESResponse (TH2)]
//   heavyion.root                     Pb-Pb collisions
//     hV0M (centrality estimator), hPtEtaCent (THnSparse: pT, eta, centrality)
//     cent_0-10/, cent_10-30/, cent_30-50/   ptSpec (per event), v2 (TGraphErrors)
//   tracks.root                       TTree "tracks": pt, eta, phi, charge, nClustersTPC, dcaXY, isPrimary
//   ptSpec_published.csv              "HEPData" table: pt, pt_err, value, stat, syst
//   testbeam.csv                      calorimeter test beam: energy, resolution, resolution_err
//   luminosity.csv                    integrated luminosity per week: time (unix), lumi
//   raa_published.csv                 nuclear modification factor 0-10%: pt, pt_err, raa, stat, syst_low, syst_high
//   runQA.csv                         run-by-run QA: run, meanNch, meanNch_err
//   systematics.csv                   relative systematic uncertainties vs. pT per source

#include <TF1.h>
#include <TFile.h>
#include <TGraph.h>
#include <TGraphErrors.h>
#include <TGraphAsymmErrors.h>
#include <TH1D.h>
#include <TH2D.h>
#include <THnSparse.h>
#include <TList.h>
#include <TMath.h>
#include <TRandom3.h>
#include <TSystem.h>
#include <TTree.h>

#include <cmath>
#include <fstream>
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
}
