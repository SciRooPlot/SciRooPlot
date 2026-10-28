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
//     flow/        v2_{pi,K,p} (v2 vs. pT, 30-40%), v2ncq_{pi,K,p} (v2/n_q vs. KET/n_q), v2, v3, v4 (vs. centrality)
//     meanPt/      pp_900GeV, pp_7TeV, pp_13TeV, pPb_5TeV, PbPb_5TeV (TGraphErrors: mean pT vs. N_ch)
//     correlations/ ridge (TH2: pair yield vs. Delta phi, Delta eta), fourier_longRange (TF1, V_2Delta in the title)
//     femtoscopy/  cf_{0-5,20-30,50-60} (TGraphErrors: pion correlation functions), fit_... (TF1, the radius in the title)
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
//     strongField/ xenon_{1,2,4}e13 (photoelectron spectra with Freeman resonances), argon_1e14 (ATI plateau),
//                  TTree "vmi": px, pz (velocity-map image, atomic units), fitRing1, fitRing2 (TF1, angular distributions)
//     orbitals/    psi_2pz, psi_3pz, psi_3dz2, psi_3dxz (TH2: hydrogen orbitals in the x-z plane), radial_1s ... radial_3d (TF1)
//     photoeffect/ stoppingVoltage (TGraphErrors), fitStopping (TF1, h in the title), current_{365,436,546}nm,
//                  lead_photo, lead_compton, lead_pair, lead_total (TGraph: photon attenuation in lead)
//     laser/       probeOnly, withPump (TGraph: saturated absorption of Rb), rabi_{0,1,2}MHz (TGraphErrors), theory_{0,1,2}MHz (TF1)
//   accelerator.root                  a beam line: hBeamSpot (TH2: transverse beam profile at a screen),
//                                     TTree "beam": x, xp (horizontal phase space), rmsEllipse (TGraph, the emittance in the title)
//   nuclear.root                      betaDecay/: tritium (spectrum), kurie (TGraphErrors), kurieFit (TF1, Q in the title),
//                                     endpoint_{0,05,1}eV (TF1: shape at the endpoint), cu64_betaMinus, cu64_betaPlus (TGraph)
//   neutrino.root                     reactor antineutrinos: survival (TGraphErrors vs. L/E), expectation (TGraph),
//                                     deltaChi2 (TH2: oscillation parameters), bestFit (TGraph, the values in the title)
//   searches.root                     exclusion limits
//     darkMatter/  limit_{A,B,C} (TGraph), expected_A_{1,2}sigma (TGraphAsymmErrors), neutrinoFog (TGraphAsymmErrors)
//     stop/        upperLimit (TH2), observed, observed_theory{Up,Down}, expected, expected_{up,down} (TGraph contours)
//   neutronstars.root                 neutron stars
//     structure/   eos_{soft,medium,stiff}, massRadius_{soft,medium,stiff} (TGraph), pulsar_J0740 (TGraphAsymmErrors),
//                  nicer_J0030, nicer_J0740 (TGraph ellipses)
//     merger/      strain, template (TGraph), spectrogram (TH2, log y bins), bolometric (TGraphErrors), model_{blue,red,total} (TF1)
//   biophysics.root                   TTree "dihedrals": phi, psi (Ramachandran plot); forceExtension (TGraphErrors), wormLikeChain
//                                     (TGraph: stretching DNA); fretEfficiency (TH1), fretFit, fretUnfolded, fretFolded (TF1);
//                                     rate_{noInhibitor,inhibitor} (TGraphErrors), fit_... (TF1, K_M in the title);
//                                     current (TGraph: ion channel), openDwellTimes (TH1, log bins), fit_openDwellTimes (TF1)
//   theory.root                       alphaS (TGraphAsymmErrors band), alphaS_measurements; pdf_{uv,dv,g,sea} (bands);
//                                     lattice_{3p,energy,entropy} (bands), crossover; bifurcation (TH2F), lyapunov (TGraph);
//                                     binder_L{8,16,32,64}, magnetisation_L{8,16,32,64} (TGraphErrors), onsager (TF1)
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
#include <TFitResult.h>
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
#include <array>
#include <cmath>
#include <complex>
#include <functional>
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

// strong-field ionisation of rare-gas atoms by femtosecond pulses of a Ti:sapphire laser (800 nm, photon energy 1.55 eV)
void WriteStrongField(TFile& file)
{
  TRandom3 random(17);
  file.mkdir("strongField")->cd();
  constexpr double kPhoton = 1.55;
  auto ponderomotive = [](double intensity) { return 0.597 * intensity; };  // U_p (eV) at 800 nm, intensity in 1e13 W/cm^2
  auto order = [&random](double ratio) {                                    // number of photons above the lowest open channel
    int n = 0;
    while (random.Uniform() < ratio) ++n;
    return n;
  };
  {  // xenon (ionisation energy 12.13 eV, at least 8 photons): above-threshold ionisation with Freeman resonances. The peaks
    // of direct ionisation sit at n hbar omega - IP - U_p and move down with the intensity at which the electron is set free;
    // the AC Stark shift brings Rydberg states (nf, binding energy E_b) into resonance during the pulse, and electrons
    // ionised through them leave with k hbar omega - E_b, at energies that do not depend on the intensity
    constexpr double kIP = 12.13;
    for (const int intensity : {1, 2, 4}) {
      const double up = ponderomotive(intensity);
      const double ratio = 0.3 * std::sqrt(intensity);
      std::vector<std::pair<double, double>> resonances;  // binding energy, weight
      double total = 0.;
      for (int n = 4; n <= 8; ++n) {
        const double binding = 13.606 / std::pow(n - 0.05, 2.);
        for (int m = 8; m <= 10; ++m) {  // m photons reach the shifted state at U_p = m hbar omega - IP + E_b
          const double upResonance = m * kPhoton - kIP + binding;
          if (upResonance <= 0. || upResonance > up) continue;
          resonances.emplace_back(binding, std::pow(upResonance / up, 5.5) / std::pow(n, 3.));
          total += resonances.back().second;
        }
      }
      TH1D spectrum(Form("xenon_%de13", intensity), "", 400, 0., 8.);
      spectrum.GetXaxis()->SetTitle("electron energy (eV)");
      spectrum.GetYaxis()->SetTitle("electrons per 20 meV");
      const int nElectrons = 200000 * intensity * intensity;
      for (int i = 0; i < nElectrons; ++i) {
        double energy{};
        if (!resonances.empty() && random.Uniform() < 0.4) {
          double pick = random.Uniform(total);
          auto it = resonances.begin();
          while (pick > it->second && std::next(it) != resonances.end()) pick -= (it++)->second;
          energy = (1 + order(ratio)) * kPhoton - it->first + random.Gaus(0., 0.012);
        } else {  // the rate grows as I^8 and the focal volume at intensity I as I^-2.5: born mostly near the peak
          const double upBirth = up * std::pow(random.Uniform(), 1. / 6.5);
          const int lowest = static_cast<int>(std::ceil((kIP + upBirth) / kPhoton));  // channel closing
          energy = (lowest + order(ratio)) * kPhoton - kIP - upBirth;
        }
        energy += random.Gaus(0., 0.01 + 0.01 * energy);  // spectrometer resolution
        if (energy > 0.) spectrum.Fill(energy);
      }
      spectrum.Write();
    }
  }
  {  // argon (15.76 eV) at 1e14 W/cm^2: direct electrons up to 2 U_p, a plateau of rescattered electrons up to the cutoff
    // at 10 U_p, the ATI peaks on both
    constexpr double kIP = 15.76;
    const double up = ponderomotive(10.);
    TH1D spectrum("argon_1e14", "", 320, 0., 80.);
    spectrum.GetXaxis()->SetTitle("electron energy (eV)");
    spectrum.GetYaxis()->SetTitle("electrons per 0.25 eV");
    const double firstPeak = std::fmod(-kIP - up, kPhoton) + kPhoton;
    for (int bin = 1; bin <= spectrum.GetNbinsX(); ++bin) {
      const double e = spectrum.GetBinCenter(bin);
      const double peaks = 1. + 0.6 * std::cos(2. * TMath::Pi() * (e - firstPeak) / kPhoton);
      const double direct = std::exp(-e / (0.3 * up));
      const double plateau = 2.e-4 * (1. + 0.5 * e / (10. * up)) / (1. + std::exp((e - 10. * up) / (0.35 * up)));
      const double counts = random.Poisson(2.e6 * (direct + plateau) * peaks + 0.5);
      spectrum.SetBinContent(bin, counts);
      spectrum.SetBinError(bin, std::sqrt(std::max(counts, 1.)));
    }
    spectrum.Write();
  }
  {  // velocity-map imaging of xenon: the slice through the momentum sphere (z along the laser polarisation) shows the ATI
    // rings, each with an angular distribution 1 + beta2 P2(cos theta) + beta4 P4 + beta6 P6, and some slow isotropic electrons
    struct Ring {
      double energy, beta2, beta4, beta6, weight;
    };
    const std::vector<Ring> rings = {{1.0, 1.4, 1.1, 0.6, 0.5}, {2.55, 1.6, 0.5, 0., 0.3}, {4.1, 1.9, 0.2, 0., 0.12}};
    auto distribution = [](const Ring& ring, double c) {
      const double c2 = c * c;
      return 1. + ring.beta2 * 0.5 * (3. * c2 - 1.) + ring.beta4 * 0.125 * ((35. * c2 - 30.) * c2 + 3.) +
             ring.beta6 * 0.0625 * (((231. * c2 - 315.) * c2 + 105.) * c2 - 5.);
    };
    constexpr double kHartree = 27.211;  // momenta in atomic units
    TTree tree("vmi", "photoelectrons");
    float px{}, pz{};
    tree.Branch("px", &px);
    tree.Branch("pz", &pz);
    // the angular distributions of the two inner rings, binned as in the example plot, for the fits
    const std::vector<std::pair<double, double>> cuts = {{0.8, 1.2}, {2.3, 2.8}};
    std::vector<TH1D> angular;
    for (size_t i = 0; i < cuts.size(); ++i) angular.emplace_back(Form("pad%zu", i), "", 40, -1., 1.);
    for (auto& h : angular) h.SetDirectory(nullptr);
    for (int i = 0; i < 100000; ++i) {
      double p{}, c{};
      double pick = random.Uniform();
      if (pick < 0.08) {  // slow electrons, isotropic
        p = std::sqrt(2. * random.Exp(0.6) / kHartree);
        c = random.Uniform(-1., 1.);
      } else {
        pick = random.Uniform(0.92);
        size_t r = 0;
        while (r + 1 < rings.size() && pick > rings[r].weight) pick -= rings[r++].weight;
        const Ring& ring = rings[r];
        const double maximum = 1. + ring.beta2 + ring.beta4 + ring.beta6;  // at cos theta = +-1
        do {
          c = random.Uniform(-1., 1.);
        } while (random.Uniform(maximum) > distribution(ring, c));
        p = std::sqrt(2. * ring.energy / kHartree) * random.Gaus(1., 0.015);
      }
      px = static_cast<float>(p * std::sqrt(1. - c * c) * (random.Uniform() < 0.5 ? -1. : 1.) + random.Gaus(0., 0.004));
      pz = static_cast<float>(p * c + random.Gaus(0., 0.004));
      tree.Fill();
      const double energy = 0.5 * (px * px + pz * pz) * kHartree, cosTheta = pz / std::hypot(px, pz);  // as defined in the example plot
      for (size_t k = 0; k < cuts.size(); ++k) {
        if (energy > cuts[k].first && energy < cuts[k].second) angular[k].Fill(cosTheta);
      }
    }
    tree.Write();
    for (size_t k = 0; k < cuts.size(); ++k) {
      TF1 fit(Form("fitRing%zu", k + 1), "[0]*(1 + [1]*0.5*(3*x^2-1) + [2]*0.125*(35*x^4-30*x^2+3) + [3]*0.0625*(231*x^6-315*x^4+105*x^2-5))", -1., 1.);
      fit.SetParameters(angular[k].GetMaximum() / 3., 1., 0.5, 0.);
      angular[k].Fit(&fit, "Q0N");
      fit.SetTitle(Form("#beta_{2} = %.2f, #beta_{4} = %.2f, #beta_{6} = %.2f", fit.GetParameter(1), fit.GetParameter(2), fit.GetParameter(3)));
      fit.SetNpx(500);
      fit.Write();
    }
  }
}

// the hydrogen atom: probability densities of orbitals in the x-z plane, and radial densities r^2 |R_nl|^2 (lengths in a_0)
void WriteOrbitals(TFile& file)
{
  file.mkdir("orbitals")->cd();
  const double pi = TMath::Pi();
  auto r21 = [](double r) { return r * std::exp(-r / 2.) / (2. * std::sqrt(6.)); };
  auto r31 = [](double r) { return 8. / (27. * std::sqrt(6.)) * r * (1. - r / 6.) * std::exp(-r / 3.); };
  auto r32 = [](double r) { return 4. / (81. * std::sqrt(30.)) * r * r * std::exp(-r / 3.); };
  struct Orbital {
    const char* name;
    std::function<double(double, double, double)> psi;  // of r, cos theta and the sign of x (phi = 0 or pi)
  };
  const std::vector<Orbital> orbitals = {
    {"psi_2pz", [&](double r, double c, double) { return r21(r) * std::sqrt(3. / (4. * pi)) * c; }},
    {"psi_3pz", [&](double r, double c, double) { return r31(r) * std::sqrt(3. / (4. * pi)) * c; }},
    {"psi_3dz2", [&](double r, double c, double) { return r32(r) * std::sqrt(5. / (16. * pi)) * (3. * c * c - 1.); }},
    {"psi_3dxz", [&](double r, double c, double sign) { return r32(r) * std::sqrt(15. / (4. * pi)) * std::sqrt(1. - c * c) * c * sign; }}};
  for (const auto& orbital : orbitals) {
    TH2D map(orbital.name, "", 160, -20., 20., 160, -20., 20.);
    map.GetXaxis()->SetTitle("#it{x} (#it{a}_{0})");
    map.GetYaxis()->SetTitle("#it{z} (#it{a}_{0})");
    map.GetZaxis()->SetTitle("|#psi|^{2} (10^{-3} #it{a}_{0}^{-3})");
    for (int ix = 1; ix <= map.GetNbinsX(); ++ix) {
      for (int iz = 1; iz <= map.GetNbinsY(); ++iz) {
        const double x = map.GetXaxis()->GetBinCenter(ix), z = map.GetYaxis()->GetBinCenter(iz);
        const double r = std::hypot(x, z);
        const double psi = orbital.psi(r, z / r, x < 0. ? -1. : 1.);
        map.SetBinContent(ix, iz, 1.e3 * psi * psi);
      }
    }
    map.Write();
  }
  const std::vector<std::pair<const char*, const char*>> radial = {
    {"radial_1s", "4*x^2*exp(-2*x)"},
    {"radial_2s", "0.5*x^2*(1-x/2)^2*exp(-x)"},
    {"radial_2p", "x^4*exp(-x)/24"},
    {"radial_3s", "4./27*x^2*(1-2*x/3+2*x^2/27)^2*exp(-2*x/3)"},
    {"radial_3p", "64./4374*x^4*(1-x/6)^2*exp(-2*x/3)"},
    {"radial_3d", "16./196830*x^6*exp(-2*x/3)"}};
  for (const auto& [name, formula] : radial) {
    TF1 density(name, formula, 0., 30.);
    density.SetNpx(1000);
    density.Write();
  }
}

// the photoelectric effect: Millikan's measurement of h, photocurrents, and the attenuation of photons in lead
void WritePhotoeffect(TFile& file)
{
  TRandom3 random(18);
  file.mkdir("photoeffect")->cd();
  constexpr double kPlanck = 0.41357;  // h in eV per 1e14 Hz
  constexpr double kWork = 1.85;       // work function of the cathode (eV)
  auto frequency = [](double wavelength) { return 2997.92458 / wavelength; };  // 1e14 Hz, wavelength in nm
  {  // stopping voltage for the lines of a mercury lamp: e U = h nu - W
    TGraphErrors stopping;
    for (const double wavelength : {365.0, 404.7, 435.8, 546.1, 577.0}) {
      const double nu = frequency(wavelength);
      stopping.AddPointError(nu, kPlanck * nu - kWork + random.Gaus(0., 0.025), 0., 0.03);
    }
    stopping.Write("stoppingVoltage");
    TF1 fit("fitStopping", "pol1", 4., 9.);
    stopping.Fit(&fit, "Q0N");
    constexpr double kScale = 16.02177;  // V / 1e14 Hz -> 1e-34 J s
    fit.SetTitle(Form("#it{h} = (%.2f #pm %.2f) #times 10^{-34} J s", fit.GetParameter(1) * kScale, fit.GetParError(1) * kScale));
    fit.Write();
  }
  {  // photocurrent against the voltage between anode and cathode: it sets in at minus the stopping voltage and saturates
    for (const auto& [wavelength, saturation] : std::vector<std::pair<int, double>>{{365, 8.}, {436, 12.}, {546, 6.}}) {
      const double stop = kPlanck * frequency(wavelength) - kWork;
      TGraphErrors current;
      for (int i = 0; i <= 50; ++i) {
        const double u = -2. + 0.1 * i, x = u + stop;
        const double ideal = 0.02 + ((x > 0.) ? saturation * x * x / (x * x + 0.36) : 0.);
        const double error = 0.01 * ideal + 0.03;
        current.AddPointError(u, ideal + random.Gaus(0., error), 0., error);
      }
      current.Write(Form("current_%dnm", wavelength));
    }
  }
  {  // mass attenuation coefficient of lead (cm^2/g, without coherent scattering): the photoeffect with its K and L edges,
    // Compton scattering (Klein-Nishina, damped by the binding of the electrons below 100 keV) and pair production
    auto loglog = [](const std::vector<std::pair<double, double>>& table, double e) {  // power law between the points
      for (size_t i = 1; i < table.size(); ++i) {
        if (e <= table[i].first) {
          const auto& [e0, v0] = table[i - 1];
          const auto& [e1, v1] = table[i];
          return v0 * std::pow(v1 / v0, std::log(e / e0) / std::log(e1 / e0));
        }
      }
      return table.back().second;
    };
    // photoeffect in segments between the absorption edges (keV); each segment is a table of its own
    const std::vector<std::vector<std::pair<double, double>>> photo = {
      {{10., 125.}, {13.035, 62.}},
      {{13.035, 152.}, {15.2, 108.}},
      {{15.2, 148.}, {15.86, 135.}},
      {{15.86, 156.}, {20., 82.}, {30., 28.5}, {40., 13.4}, {50., 7.4}, {60., 4.6}, {80., 2.15}, {88.0, 1.70}},
      {{88.0, 7.4}, {100., 5.2}, {150., 1.75}, {200., 0.83}, {300., 0.29}, {500., 0.080}, {1000., 0.018}, {2000., 0.0049},
       {5000., 0.0014}, {10000., 0.00065}, {100000., 0.00006}}};
    const std::vector<std::pair<double, double>> pair = {{1250., 0.00012}, {1500., 0.0008}, {2000., 0.0031}, {3000., 0.0085},
                                                         {5000., 0.019}, {10000., 0.036}, {20000., 0.053}, {50000., 0.072}, {100000., 0.082}};
    auto compton = [](double e) {
      const double k = e / 511.;  // photon energy in units of the electron mass
      const double sigma = 2. * TMath::Pi() * 7.9407e-26 *
                           ((1. + k) / (k * k) * (2. * (1. + k) / (1. + 2. * k) - std::log(1. + 2. * k) / k) + std::log(1. + 2. * k) / (2. * k) -
                            (1. + 3. * k) / std::pow(1. + 2. * k, 2.));  // cm^2 per electron
      return sigma * 6.02214e23 * 82. / 207.2 * (1. - std::exp(-e / 45.));
    };
    TGraph gPhoto, gCompton, gPair, gTotal;
    for (size_t s = 0; s < photo.size(); ++s) {
      const double low = photo[s].front().first, high = (s + 1 < photo.size()) ? photo[s].back().first : 100000.;
      constexpr int kSteps = 60;  // points per decade
      const int n = std::max(2, static_cast<int>(kSteps * std::log10(high / low)));
      for (int i = 0; i <= n; ++i) {
        const double e = low * std::pow(high / low, static_cast<double>(i) / n) * (i == 0 ? 1.000001 : i == n ? 0.999999 : 1.);
        const double tau = loglog(photo[s], e), sigma = compton(e);
        const double kappa = (e < 1022.) ? 0. : (e < 1250.) ? 0.00012 * std::pow((e - 1022.) / 228., 2.) : loglog(pair, e);
        gPhoto.AddPoint(e / 1000., tau);
        gCompton.AddPoint(e / 1000., sigma);
        if (kappa > 1e-5) gPair.AddPoint(e / 1000., kappa);
        gTotal.AddPoint(e / 1000., tau + sigma + kappa);
      }
    }
    gPhoto.Write("lead_photo");
    gCompton.Write("lead_compton");
    gPair.Write("lead_pair");
    gTotal.Write("lead_total");
  }
}

// laser spectroscopy and coherent control: saturated absorption in rubidium vapour and Rabi oscillations of a two-level atom
void WriteLaserSpectroscopy(TFile& file)
{
  TRandom3 random(19);
  file.mkdir("laser")->cd();
  {  // probe transmission through a vapour cell over the 87Rb D2 lines F = 2 -> F' = 1, 2, 3 (relative strengths 1 : 5 : 14):
    // the Doppler profile alone, and with a counter-propagating pump that burns Lamb dips at the transitions and crossover
    // resonances halfway between them (atoms moving such that the pump drives one transition and the probe the other)
    const std::vector<std::pair<double, double>> lines = {{-72.2, 1.}, {0., 5.}, {266.7, 14.}};  // MHz from F' = 2
    constexpr double kDoppler = 220., kWidth = 9.;  // sigma of the Doppler profile and half width of the dips (MHz)
    auto gauss = [](double x) { return std::exp(-0.5 * x * x / (kDoppler * kDoppler)); };
    auto lorentz = [](double x) { return kWidth * kWidth / (x * x + kWidth * kWidth); };
    auto absorption = [&](double nu, bool pump) {
      double alpha = 0.;
      for (const auto& [nu0, strength] : lines) alpha += strength * gauss(nu - nu0) * (pump ? 1. - 0.5 * lorentz(nu - nu0) : 1.);
      if (pump) {
        for (size_t i = 0; i < lines.size(); ++i) {
          for (size_t j = i + 1; j < lines.size(); ++j) {
            const double centre = 0.5 * (lines[i].first + lines[j].first);
            alpha -= 0.5 * std::sqrt(lines[i].second * lines[j].second) * gauss(0.5 * (lines[i].first - lines[j].first)) * lorentz(nu - centre);
          }
        }
      }
      return 0.6 * alpha / 17.;  // optical depth, about 0.6 at the strongest absorption
    };
    TGraph probe, pumped;
    for (int i = 0; i <= 1400; ++i) {
      const double nu = -700. + i;
      probe.AddPoint(nu, std::exp(-absorption(nu, false)) + random.Gaus(0., 0.002));
      pumped.AddPoint(nu, std::exp(-absorption(nu, true)) + random.Gaus(0., 0.002));
    }
    probe.Write("probeOnly");
    pumped.Write("withPump");
  }
  {  // excited-state population after a resonant or detuned pulse: Rabi frequency 1 MHz, decoherence time 2.5 us, each point
    // measured with 200 atoms; the detuning lowers the amplitude and speeds up the oscillation
    for (const int detuning : {0, 1, 2}) {  // MHz
      const double omega = 2. * TMath::Pi() * std::sqrt(1. + detuning * detuning);  // generalised Rabi frequency, rad/us
      const double amplitude = 1. / (1. + detuning * detuning);
      const std::string formula = Form("%.6f*0.5*(1-exp(-x/2.5)*cos(%.6f*x))", amplitude, omega);
      TGraphErrors measured;
      for (int i = 0; i <= 60; ++i) {
        const double t = 0.05 * i;
        const double p = amplitude * 0.5 * (1. - std::exp(-t / 2.5) * std::cos(omega * t));
        const double fraction = random.Binomial(200, p) / 200.;
        measured.AddPointError(t, fraction, 0., std::sqrt(std::max(fraction * (1. - fraction), 1. / 200.) / 200.));
      }
      measured.Write(Form("rabi_%dMHz", detuning));
      TF1 theory(Form("theory_%dMHz", detuning), formula.data(), 0., 3.);
      theory.SetNpx(600);
      theory.Write();
    }
  }
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

  WriteStrongField(file);
  WriteOrbitals(file);
  WritePhotoeffect(file);
  WriteLaserSpectroscopy(file);
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

// --------------------------------------------------------------------------------------------------
// heavyion.root (flow/, meanPt/), nuclear.root, neutrino.root, searches.root, neutronstars.root
// --------------------------------------------------------------------------------------------------

// identified-particle flow, flow harmonics and mean pT, added to heavyion.root
void WriteFlowAndMeanPt()
{
  TFile file((kFolder + "heavyion.root").data(), "UPDATE");
  TRandom3 random(20);
  file.mkdir("flow")->cd();
  {  // v2(pT) of pions, kaons and protons in Pb-Pb 30-40%: the quarks of a hadron carry the flow, so v2/n_q is one curve of
    // the transverse kinetic energy per quark, KET/n_q (mass ordering at low pT, baryons above mesons at intermediate pT)
    auto perQuark = [](double x) { return 0.085 * x / std::sqrt(x * x + 0.09) * (1. - 0.1 * x); };
    const std::vector<double> pts = {0.3, 0.5, 0.7, 0.9, 1.1, 1.4, 1.75, 2.25, 2.75, 3.5, 4.5, 5.5};
    struct Hadron {
      const char* name;
      double mass;
      int quarks;
    };
    for (const Hadron& hadron : {Hadron{"pi", 0.13957, 2}, Hadron{"K", 0.49368, 2}, Hadron{"p", 0.93827, 3}}) {
      TGraphErrors v2, scaled;
      for (const double pt : pts) {
        const double ket = std::sqrt(pt * pt + hadron.mass * hadron.mass) - hadron.mass;
        const double error = 0.002 + 0.03 * hadron.quarks * perQuark(ket / hadron.quarks) * (pt > 3. ? 2. : 1.);
        const double value = hadron.quarks * perQuark(ket / hadron.quarks) + random.Gaus(0., error);
        v2.AddPointError(pt, value, 0., error);
        scaled.AddPointError(ket / hadron.quarks, value / hadron.quarks, 0., error / hadron.quarks);
      }
      v2.Write(Form("v2_%s", hadron.name));
      scaled.Write(Form("v2ncq_%s", hadron.name));
    }
  }
  {  // integrated flow harmonics against centrality: v2 follows the almond shape of the overlap, v3 and v4 the fluctuations
    const std::vector<std::pair<double, double>> classes = {{0., 5.}, {5., 10.}, {10., 20.}, {20., 30.}, {30., 40.}, {40., 50.}, {50., 60.}, {60., 70.}};
    const std::vector<std::function<double(double)>> harmonics = {[](double c) { return 0.028 + 0.0028 * c - 0.000027 * c * c; },
                                                                  [](double c) { return 0.021 + 0.00022 * c - 0.0000025 * c * c; },
                                                                  [](double c) { return 0.010 + 0.00006 * c; }};
    for (size_t n = 0; n < harmonics.size(); ++n) {
      TGraphErrors vn;
      for (const auto& [low, high] : classes) {
        const double c = 0.5 * (low + high), error = 0.0006 + 0.01 * harmonics[n](c);
        vn.AddPointError(c, harmonics[n](c) + random.Gaus(0., error), 0.5 * (high - low), error);
      }
      vn.Write(Form("v%zu", n + 2));
    }
  }
  file.mkdir("meanPt")->cd();
  {  // mean pT against the charged-particle multiplicity for pp at three energies, p-Pb and Pb-Pb: strong rise in pp, weaker
    // in p-Pb, weakest in Pb-Pb, where many independent nucleon-nucleon collisions add up
    struct System {
      const char* name;
      int maxNch;
      std::function<double(double)> meanPt;
    };
    const std::vector<System> systems = {
      {"pp_900GeV", 30, [](double n) { return 0.43 + 0.013 * std::pow(n, 0.8); }},
      {"pp_7TeV", 50, [](double n) { return 0.45 + 0.015 * std::pow(n, 0.8); }},
      {"pp_13TeV", 60, [](double n) { return 0.47 + 0.016 * std::pow(n, 0.8); }},
      {"pPb_5TeV", 100, [](double n) { return 0.45 + 0.31 * (1. - std::exp(-n / 25.)); }},
      {"PbPb_5TeV", 100, [](double n) { return 0.45 + 0.23 * (1. - std::exp(-n / 40.)); }}};
    for (const auto& system : systems) {
      TGraphErrors meanPt;
      for (int n = 1; n <= system.maxNch; n += (n < 20 ? 1 : 2)) {
        const double error = 0.002 + 0.0001 * n;
        meanPt.AddPointError(n, system.meanPt(n) + random.Gaus(0., error), 0., error);
      }
      meanPt.Write(system.name);
    }
  }
}

// beta decay: the tritium spectrum, its Kurie plot and the endpoint region for three neutrino masses, and 64Cu decaying both ways
void WriteNuclear()
{
  TFile file((kFolder + "nuclear.root").data(), "RECREATE");
  TRandom3 random(21);
  constexpr double kElectron = 510.999, kAlpha = 1. / 137.036;  // keV
  // allowed spectrum: Fermi function (Coulomb attraction of the electron, repulsion of the positron) x p E (Q - T)^2
  auto spectrum = [&](double t, double q, double zDaughter, bool positron, double neutrinoMass = 0.) {
    if (t <= 0. || t >= q - neutrinoMass) return 0.;
    const double e = t + kElectron, p = std::sqrt(e * e - kElectron * kElectron);
    const double eta = (positron ? -1. : 1.) * zDaughter * kAlpha * e / p;
    const double fermi = 2. * TMath::Pi() * eta / (1. - std::exp(-2. * TMath::Pi() * eta));
    const double nu = q - t;
    return fermi * p * e * nu * std::sqrt(nu * nu - neutrinoMass * neutrinoMass);
  };
  file.mkdir("betaDecay")->cd();
  {  // tritium: 3H -> 3He e- anti-nu, Q = 18.574 keV
    constexpr double q = 18.574;
    TH1D tritium("tritium", "", 190, 0., 19.);
    tritium.GetXaxis()->SetTitle("electron kinetic energy (keV)");
    tritium.GetYaxis()->SetTitle("counts per 100 eV");
    double norm = 0.;
    for (int bin = 1; bin <= tritium.GetNbinsX(); ++bin) norm += spectrum(tritium.GetBinCenter(bin), q, 2., false);
    TGraphErrors kurie;
    for (int bin = 1; bin <= tritium.GetNbinsX(); ++bin) {
      const double t = tritium.GetBinCenter(bin);
      const double counts = random.Poisson(2.e6 * spectrum(t, q, 2., false) / norm);
      tritium.SetBinContent(bin, counts);
      tritium.SetBinError(bin, std::sqrt(std::max(counts, 1.)));
      if (counts < 10.) continue;
      // Kurie plot: sqrt(N / (F p E)) is a straight line through the endpoint (in units of its value at T = 0)
      const double e = t + kElectron, p = std::sqrt(e * e - kElectron * kElectron), eta = 2. * kAlpha * e / p;
      const double k = std::sqrt(counts / (2. * TMath::Pi() * eta / (1. - std::exp(-2. * TMath::Pi() * eta)) * p * e) * norm / 2.e6) / q;
      kurie.AddPointError(t, k, 0., 0.5 * k / std::sqrt(counts));
    }
    tritium.Write();
    kurie.Write("kurie");
    TF1 line("kurieFit", "pol1", 1., 18.);
    const auto result = kurie.Fit(&line, "Q0NS");
    const double a = line.GetParameter(0), b = line.GetParameter(1);
    const double endpoint = -a / b;
    const double error = std::sqrt(std::pow(line.GetParError(0) / b, 2.) + std::pow(a * line.GetParError(1) / (b * b), 2.) - 2. * a / (b * b * b) * result->CovMatrix(0, 1));
    line.SetTitle(Form("#it{Q} = (%.3f #pm %.3f) keV", endpoint, error));
    line.SetRange(0., 19.);
    line.Write();
    // the last eV before the endpoint E0: the shape (E0 - E)^2 sqrt(1 - m^2 / (E0 - E)^2) for three neutrino masses
    for (const auto& [name, mass] : std::vector<std::pair<const char*, double>>{{"endpoint_0eV", 0.}, {"endpoint_05eV", 0.5}, {"endpoint_1eV", 1.}}) {
      TF1 shape(name, "(x<0)*x*x*sqrt(TMath::Max(0., 1-[0]*[0]/(x*x)))", -3., 0.5);
      shape.SetParameter(0, mass);
      shape.SetNpx(1000);
      shape.Write();
    }
  }
  {  // 64Cu decays by beta- (to 64Zn, Q = 579 keV) and by beta+ (to 64Ni, Q = 653 keV): the Coulomb field of the nucleus slows
    // the electrons and pushes the positrons out; both spectra normalised to one decay
    for (const auto& [name, q, z, positron] : std::vector<std::tuple<const char*, double, double, bool>>{{"cu64_betaMinus", 579., 30., false}, {"cu64_betaPlus", 653., 28., true}}) {
      TGraph shape;
      double norm = 0.;
      for (int i = 1; i <= 700; ++i) norm += spectrum(i, q, z, positron);
      for (int i = 0; i <= 700; ++i) shape.AddPoint(i, spectrum(i, q, z, positron) / norm * 1000.);  // per MeV
      shape.Write(name);
    }
  }
}

// reactor antineutrinos: survival probability against L/E and the allowed region of the oscillation parameters
void WriteNeutrino()
{
  TFile file((kFolder + "neutrino.root").data(), "RECREATE");
  TRandom3 random(22);
  // P(anti-nu_e survives) = 1 - sin^2(2 theta) sin^2(1.267 dm^2 L / E), L/E in km/MeV, dm^2 in eV^2, averaged over the spread
  // of the reactor distances and the energy resolution (12% in L/E)
  auto survival = [](double loe, double sin2, double dm2) {
    constexpr int kSteps = 100;
    double sum = 0.;
    for (int k = 0; k < kSteps; ++k) {
      const double x = loe * (1. + 0.12 * TMath::NormQuantile((k + 0.5) / kSteps));
      sum += 1. - sin2 * std::pow(std::sin(1267. * dm2 * x), 2.);
    }
    return sum / kSteps;
  };
  constexpr double kSin2 = 0.85, kDm2 = 7.5e-5;
  std::vector<double> centres, values, errors;
  TGraphErrors data;
  for (double loe = 20.; loe <= 100.; loe += 5.) {
    const double error = 0.035 + 0.0004 * loe;
    const double value = survival(loe, kSin2, kDm2) + random.Gaus(0., error);
    data.AddPointError(loe, value, 2.5, error);
    centres.push_back(loe);
    values.push_back(value);
    errors.push_back(error);
  }
  data.Write("survival");
  // chi2 on a grid of the parameters, as Delta chi2 to the minimum
  TH2D chi2("deltaChi2", "", 100, 0.5, 1., 100, 6., 9.);
  chi2.GetXaxis()->SetTitle("sin^{2}2#theta_{12}");
  chi2.GetYaxis()->SetTitle("#Delta#it{m}^{2}_{21} (10^{-5} eV^{2})");
  chi2.GetZaxis()->SetTitle("#Delta#chi^{2}");
  double best = 1e30, bestSin2 = 0., bestDm2 = 0.;
  for (int ix = 1; ix <= chi2.GetNbinsX(); ++ix) {
    for (int iy = 1; iy <= chi2.GetNbinsY(); ++iy) {
      const double sin2 = chi2.GetXaxis()->GetBinCenter(ix), dm2 = 1e-5 * chi2.GetYaxis()->GetBinCenter(iy);
      double sum = 0.;
      for (size_t i = 0; i < centres.size(); ++i) sum += std::pow((values[i] - survival(centres[i], sin2, dm2)) / errors[i], 2.);
      chi2.SetBinContent(ix, iy, sum);
      if (sum < best) std::tie(best, bestSin2, bestDm2) = std::make_tuple(sum, sin2, dm2);
    }
  }
  for (int ix = 1; ix <= chi2.GetNbinsX(); ++ix) {
    for (int iy = 1; iy <= chi2.GetNbinsY(); ++iy) chi2.SetBinContent(ix, iy, chi2.GetBinContent(ix, iy) - best + 1e-3);  // > 0 for log z
  }
  chi2.Write();
  TGraph bestFit;
  bestFit.AddPoint(bestSin2, 1e5 * bestDm2);
  bestFit.SetTitle(Form("best fit: sin^{2}2#theta = %.2f, #Deltam^{2} = %.2f #times 10^{-5} eV^{2}", bestSin2, 1e5 * bestDm2));
  bestFit.Write("bestFit");
  TGraph expectation;
  for (double loe = 15.; loe <= 105.; loe += 0.25) expectation.AddPoint(loe, survival(loe, bestSin2, bestDm2));
  expectation.Write("expectation");
}

// exclusion limits: dark matter scattering off nuclei, and a search for top squarks in the mass plane
void WriteSearches()
{
  TFile file((kFolder + "searches.root").data(), "RECREATE");
  TRandom3 random(23);
  file.mkdir("darkMatter")->cd();
  {  // 90% CL upper limits on the spin-independent WIMP-nucleon cross-section of three experiments: the reach falls with
    // fewer target nuclei per heavy WIMP at high mass, and with the energy threshold at low mass; the expected limit of the
    // newest one with its 1 and 2 sigma bands, and the neutrino fog where solar and atmospheric neutrinos look alike
    auto reach = [](double m, double minimum, double best, double threshold) {
      return minimum * 0.5 * (m / best + best / m) * std::exp(std::pow(threshold / m, 3.));
    };
    // slow wiggles of an observed limit around its expectation (under- and overfluctuations of the background)
    auto wiggles = [&random]() {
      std::array<double, 6> phases{};
      for (auto& phase : phases) phase = random.Uniform(2. * TMath::Pi());
      return [phases](double m) {
        const double l = std::log(m);
        return std::exp(0.12 * std::sin(1.7 * l + phases[0]) + 0.08 * std::sin(3.1 * l + phases[1]) + 0.05 * std::sin(5.3 * l + phases[2]));
      };
    };
    struct Experiment {
      const char* name;
      double low, minimum, best, threshold, high;
    };
    for (const auto& [name, low, minimum, best, threshold, high] :
         {Experiment{"limit_A", 6., 2.e-47, 36., 9., 1e4}, Experiment{"limit_B", 6., 8.e-47, 30., 8., 1e4}, Experiment{"limit_C", 1.5, 3.e-45, 6., 1.5, 30.}}) {
      const auto wiggle = wiggles();
      TGraph limit;
      for (int i = 0; i <= 300; ++i) {
        const double m = low * std::pow(high / low, i / 300.);
        limit.AddPoint(m, reach(m, minimum, best, threshold) * wiggle(m));
      }
      limit.Write(name);
    }
    TGraphAsymmErrors band1, band2, fog;
    for (int i = 0; i <= 300; ++i) {
      const double m = 6. * std::pow(1e4 / 6., i / 300.);
      const double median = reach(m, 2.e-47, 36., 9.) * 1.1;
      band1.AddPointError(m, median, 0., 0., median * (1. - 0.6), median * (1.7 - 1.));
      band2.AddPointError(m, median, 0., 0., median * (1. - 0.35), median * (2.8 - 1.));
    }
    for (int i = 0; i <= 300; ++i) {
      const double m = 1. * std::pow(1e4, i / 300.);
      const double solar = 3e-45 * std::exp(-0.5 * std::pow(std::log(m / 5.5) / 0.3, 2.));                // 8B neutrinos
      const double floor = 1.5e-49 * std::pow(m / 100., 0.85) + 2e-49 * std::pow(20. / m, 2.) + 2e-50;  // atmospheric, DSNB
      const double top = solar + floor;
      fog.AddPointError(m, top, 0., 0., top - 1e-60, 0.);  // a band from the boundary down to (almost) zero
    }
    band1.Write("expected_A_1sigma");
    band2.Write("expected_A_2sigma");
    fog.Write("neutrinoFog");
  }
  file.mkdir("stop")->cd();
  {  // pair production of top squarks, each decaying to a top quark and the lightest neutralino: observed 95% CL upper limit on
    // the cross-section in the mass plane, and the excluded region where it is below the theory cross-section (observed with
    // the +-1 sigma theory uncertainty, expected with its +-1 sigma experimental band); the sensitivity drops towards the
    // kinematic limit m(stop) = m(top) + m(neutralino), where the neutralinos take little momentum
    auto theory = [](double mStop) { return 10. * std::exp(-(mStop - 200.) / 130.); };  // pb
    auto expected = [](double mStop, double mLsp) {
      const double dm = mStop - mLsp;
      return 3.5e-3 * (1. + std::pow(400. / dm, 3.)) * (1. + 0.0004 * (mStop - 200.));
    };
    std::array<double, 3> phases{};
    for (auto& phase : phases) phase = random.Uniform(2. * TMath::Pi());
    auto observed = [&](double mStop, double mLsp) {
      return expected(mStop, mLsp) * std::exp(0.15 * std::sin(mStop / 140. + phases[0]) + 0.1 * std::sin(mLsp / 90. + phases[1]) + 0.05 * std::sin((mStop + mLsp) / 60. + phases[2]));
    };
    TH2D upperLimit("upperLimit", "", 60, 200., 1400., 40, 0., 800.);
    upperLimit.GetXaxis()->SetTitle("#it{m}_{#tilde{t}} (GeV)");
    upperLimit.GetYaxis()->SetTitle("#it{m}_{#tilde{#chi}^{0}_{1}} (GeV)");
    upperLimit.GetZaxis()->SetTitle("95% CL upper limit on #sigma (pb)");
    for (int ix = 1; ix <= upperLimit.GetNbinsX(); ++ix) {
      for (int iy = 1; iy <= upperLimit.GetNbinsY(); ++iy) {
        const double mStop = upperLimit.GetXaxis()->GetBinCenter(ix), mLsp = upperLimit.GetYaxis()->GetBinCenter(iy);
        if (mStop - mLsp > 173. + 20.) upperLimit.SetBinContent(ix, iy, observed(mStop, mLsp));
      }
    }
    upperLimit.Write();
    // exclusion contour: along rays from a point deep in the excluded region, the distance at which the limit reaches the
    // (scaled) theory cross-section; it ends where it first meets the kinematic limit
    auto contour = [&](const std::function<double(double, double)>& limit, double scale) {
      constexpr double s0 = 250., l0 = 0., kinematic = 193.;
      TGraph graph;
      for (int i = 0; i <= 120; ++i) {
        const double phi = 0.5 * TMath::Pi() * i / 120. * 0.999, c = std::cos(phi), sn = std::sin(phi);
        auto excluded = [&](double r) {
          const double mStop = s0 + r * c, mLsp = l0 + r * sn;
          return mStop - mLsp > kinematic && limit(mStop, mLsp) < scale * theory(mStop);
        };
        double low = 0., high = std::min((1400. - s0) / c, (800. - l0) / std::max(sn, 1e-9));
        if (excluded(high)) {
          low = high;
        } else {
          for (int k = 0; k < 50; ++k) (excluded(0.5 * (low + high)) ? low : high) = 0.5 * (low + high);
        }
        const double mStop = s0 + low * c, mLsp = l0 + low * sn;
        graph.AddPoint(mStop, mLsp);
        if (mStop - mLsp < kinematic + 1.) break;
      }
      return graph;
    };
    contour(observed, 1.).Write("observed");
    contour(observed, 1.15).Write("observed_theoryUp");
    contour(observed, 0.85).Write("observed_theoryDown");
    contour(expected, 1.).Write("expected");
    contour([&](double s, double l) { return 0.7 * expected(s, l); }, 1.).Write("expected_up");
    contour([&](double s, double l) { return 1.4 * expected(s, l); }, 1.).Write("expected_down");
  }
}

// neutron stars: equations of state and the mass-radius relations they give (Tolman-Oppenheimer-Volkoff equations), measured
// masses and radii, and a binary neutron-star merger: gravitational waves and the kilonova
void WriteNeutronStars()
{
  TFile file((kFolder + "neutronstars.root").data(), "RECREATE");
  TRandom3 random(24);
  file.mkdir("structure")->cd();
  {  // polytropes p = K rho^Gamma (rest-mass density rho, energy density eps = rho + p / (Gamma - 1)), in units G = c = M_sun = 1,
    // integrated outwards from the centre with fourth-order Runge-Kutta steps until the pressure vanishes
    constexpr double kLength = 1.4766;    // km per unit
    constexpr double kEnergy = 3.4664e5;  // MeV/fm^3 per unit
    struct Eos {
      const char* name;
      double gamma, k;
    };
    for (const Eos& model : {Eos{"soft", 2.75, 1.5e4}, Eos{"medium", 3., 1.14e5}, Eos{"stiff", 3., 2.2e5}}) {
      const double gamma = model.gamma, k = model.k;  // plain variables: lambdas cannot capture structured bindings in C++17
      auto energy = [&](double p) { return std::pow(p / k, 1. / gamma) + p / (gamma - 1.); };
      TGraph eos;
      for (int i = 0; i <= 100; ++i) {
        const double rho = 2e-4 * std::pow(15., i / 100.);
        eos.AddPoint(kEnergy * (rho + k * std::pow(rho, gamma) / (gamma - 1.)), kEnergy * k * std::pow(rho, gamma));
      }
      eos.Write(Form("eos_%s", model.name));
      TGraph massRadius;
      double previous = 0.;
      for (int i = 0; i <= 80; ++i) {
        const double rhoCentre = 1e-4 * std::pow(60., i / 80.);
        auto derivatives = [&](double r, double m, double p) {
          const double e = energy(std::max(p, 0.));
          return std::make_pair(4. * TMath::Pi() * r * r * e, -(e + p) * (m + 4. * TMath::Pi() * r * r * r * p) / (r * (r - 2. * m)));
        };
        double r = 1e-6, m = 0., p = k * std::pow(rhoCentre, gamma);
        constexpr double dr = 2e-3;
        while (true) {
          const auto k1 = derivatives(r, m, p);
          const auto k2 = derivatives(r + 0.5 * dr, m + 0.5 * dr * k1.first, p + 0.5 * dr * k1.second);
          const auto k3 = derivatives(r + 0.5 * dr, m + 0.5 * dr * k2.first, p + 0.5 * dr * k2.second);
          const auto k4 = derivatives(r + dr, m + dr * k3.first, p + dr * k3.second);
          const double pNext = p + dr / 6. * (k1.second + 2. * k2.second + 2. * k3.second + k4.second);
          if (pNext <= 0.) break;
          m += dr / 6. * (k1.first + 2. * k2.first + 2. * k3.first + k4.first);
          p = pNext;
          r += dr;
        }
        if (m < previous) break;  // beyond the maximum mass the stars are unstable
        previous = m;
        massRadius.AddPoint(kLength * r, m);
      }
      massRadius.Write(Form("massRadius_%s", model.name));
    }
  }
  {  // measurements: the mass of the pulsar J0740+6620 (Shapiro delay) as a band over all radii, and the 68% regions of mass and
    // radius from X-ray pulse profiles (NICER) of J0030+0451 and J0740+6620 as ellipses
    TGraphAsymmErrors pulsar;
    for (const double r : {6., 18.}) pulsar.AddPointError(r, 2.08, 0., 0., 0.07, 0.07);
    pulsar.Write("pulsar_J0740");
    for (const auto& [name, r0, m0, sr, sm] : std::vector<std::tuple<const char*, double, double, double, double>>{{"nicer_J0030", 12.7, 1.44, 1.1, 0.15}, {"nicer_J0740", 12.4, 2.07, 1.1, 0.07}}) {
      TGraph ellipse;
      constexpr double rho = 0.6;  // correlation of mass and radius
      for (int i = 0; i <= 100; ++i) {
        const double t = 2. * TMath::Pi() * i / 100., scale = std::sqrt(2.3);  // 68% for two parameters
        ellipse.AddPoint(r0 + scale * sr * std::cos(t), m0 + scale * sm * (rho * std::cos(t) + std::sqrt(1. - rho * rho) * std::sin(t)));
      }
      ellipse.Write(name);
    }
  }
  file.mkdir("merger")->cd();
  {  // the last tenth of a second of the inspiral (and half a second in the time-frequency map) of two neutron stars (chirp mass 1.19 M_sun) in a detector: whitened strain with
    // noise, the template, and the time-frequency map in which the signal sweeps up in frequency
    constexpr double kChirp = 1.188 * 4.925e-6;  // G M_c / c^3 in s
    auto frequency = [](double tau) { return std::pow(5. / 256. / tau, 0.375) * std::pow(kChirp, -0.625) / TMath::Pi(); };
    auto phase = [](double tau) { return -2. * std::pow(tau / (5. * kChirp), 0.625); };
    constexpr double kMerger = 1500.;  // Hz, the signal ends when the stars touch
    const double tauMerger = 5. / 256. * std::pow(TMath::Pi() * kMerger, -8. / 3.) * std::pow(kChirp, -5. / 3.);
    auto signal = [&](double t) {  // t = 0 at the merger
      if (t > 0.) return 0.;        // after the merger nothing (the remnant rings above 2 kHz)
      const double tau = tauMerger - t;
      return 0.5 * std::pow(frequency(tau) / 300., 2. / 3.) * std::cos(phase(tau));  // amplitude grows as f^(2/3)
    };
    TGraph strain, model;
    for (int i = 0; i <= 700; ++i) {
      const double t = -0.15 + i / 4096.;
      strain.AddPoint(t, signal(t) + random.Gaus(0., 0.35));
      model.AddPoint(t, signal(t));
    }
    strain.Write("strain");
    model.Write("template");
    const auto bins = LogBins(64, 30., 2000.);
    TH2D map("spectrogram", "", 220, -0.5, 0.05, 64, bins.data());
    map.GetXaxis()->SetTitle("time from merger (s)");
    map.GetYaxis()->SetTitle("frequency (Hz)");
    map.GetZaxis()->SetTitle("normalised energy");
    for (int ix = 1; ix <= map.GetNbinsX(); ++ix) {
      const double t = map.GetXaxis()->GetBinCenter(ix), tau = tauMerger - t;
      for (int iy = 1; iy <= map.GetNbinsY(); ++iy) {
        double energy = random.Exp(1.);
        if (t <= 0.) {
          const double f = frequency(tau), f0 = map.GetYaxis()->GetBinCenter(iy);
          energy += std::min(25., 3. * std::pow(f / 50., 0.7)) * std::exp(-0.5 * std::pow(std::log(f0 / f) / 0.12, 2.));
        }
        map.SetBinContent(ix, iy, energy);
      }
    }
    map.Write();
  }
  {  // the kilonova: bolometric luminosity of the radioactive ejecta, a fast blue (lanthanide-poor) and a slow red component
    const char* blue = "60*pow(x,-1.3)*(1-exp(-pow(x/0.4,2)))";
    const char* red = "60*pow(x,-1.1)*(1-exp(-pow(x/2.,2)))";
    TF1 fBlue("model_blue", blue, 0.2, 20.), fRed("model_red", red, 0.2, 20.), fTotal("model_total", Form("%s + %s", blue, red), 0.2, 20.);
    for (TF1* f : {&fBlue, &fRed, &fTotal}) {
      f->SetNpx(500);
      f->Write();
    }
    TGraphErrors luminosity;
    for (const double t : {0.5, 0.75, 1., 1.5, 2., 3., 4., 5., 6., 7., 8., 10., 12., 14.}) {
      const double value = fTotal.Eval(t) * random.Gaus(1., 0.08);
      luminosity.AddPointError(t, value, 0., 0.1 * value);
    }
    luminosity.Write("bolometric");
  }
}

// two-particle correlations and femtoscopy, added to heavyion.root
void WriteCorrelations()
{
  TFile file((kFolder + "heavyion.root").data(), "UPDATE");
  TRandom3 random(25);
  file.mkdir("correlations")->cd();
  {  // per-trigger pair yield against Delta phi and Delta eta in high-multiplicity collisions, divided by mixed events: the jet
    // peak at the origin, the away-side ridge at pi, and the near-side ridge, long-range in Delta eta (the flow harmonics V_nDelta)
    TH2D ridge("ridge", "", 36, -0.5 * TMath::Pi(), 1.5 * TMath::Pi(), 32, -4., 4.);
    ridge.GetXaxis()->SetTitle("#Delta#varphi (rad)");
    ridge.GetYaxis()->SetTitle("#Delta#eta");
    ridge.GetZaxis()->SetTitle("pairs per trigger (a.u.)");
    for (int ix = 1; ix <= ridge.GetNbinsX(); ++ix) {
      for (int iy = 1; iy <= ridge.GetNbinsY(); ++iy) {
        const double dphi = ridge.GetXaxis()->GetBinCenter(ix), deta = ridge.GetYaxis()->GetBinCenter(iy);
        const double flow = 1. + 2. * 0.004 * std::cos(2. * dphi) + 2. * 0.0012 * std::cos(3. * dphi);
        const double jet = 0.15 * std::exp(-0.5 * (dphi * dphi / (0.25 * 0.25) + deta * deta / (0.35 * 0.35)));
        const double awaySide = 0.012 * std::exp(-0.5 * std::pow((dphi - TMath::Pi()) / 0.7, 2.));
        const double value = flow + jet + awaySide + random.Gaus(0., 0.0008);
        ridge.SetBinContent(ix, iy, value);
        ridge.SetBinError(ix, iy, 0.0008);
      }
    }
    ridge.Write();
    // the long-range part (2 < Delta eta < 4) projected on Delta phi, as ProjectX(2, 4) does, with a Fourier fit
    std::unique_ptr<TH1D> projection(ridge.ProjectionX("longRange", ridge.GetYaxis()->FindBin(2.001), ridge.GetYaxis()->FindBin(3.999)));
    projection->SetDirectory(nullptr);
    TF1 fourier("fourier_longRange", "[0]*(1 + 2*[1]*cos(x) + 2*[2]*cos(2*x) + 2*[3]*cos(3*x))", -0.5 * TMath::Pi(), 1.5 * TMath::Pi());
    fourier.SetParameters(projection->GetMean(2), 0., 0.004, 0.001);
    projection->Fit(&fourier, "Q0N");
    fourier.SetTitle(Form("Fourier fit, #it{V}_{2#Delta} = (%.2f #pm %.2f) #times 10^{-3}", 1e3 * fourier.GetParameter(2), 1e3 * fourier.GetParError(2)));
    fourier.SetNpx(300);
    fourier.Write();
  }
  file.mkdir("femtoscopy")->cd();
  {  // correlation function of identical pions against q_inv: Bose-Einstein enhancement 1 + exp(-R^2 q^2), its width measuring
    // the size of the source, with the Coulomb repulsion (Gamow factor) at small q and the purity lambda (Bowler-Sinyukov)
    const char* formula = "[0]*((1-[1]) + [1]*0.0064013/x/(exp(0.0064013/x)-1)*(1+exp(-[2]*[2]*x*x/0.038930)))";  // q in GeV, R in fm
    for (const auto& [centrality, radius] : std::vector<std::pair<const char*, double>>{{"0-5", 8.}, {"20-30", 6.}, {"50-60", 4.}}) {
      TF1 truth("truth", formula, 0.002, 0.2);
      truth.SetParameters(1., 0.4, radius);
      TGraphErrors cf;
      for (double q = 0.005; q < 0.2; q += 0.004) {
        const double error = 0.004 + 0.02 * std::exp(-q / 0.01);
        cf.AddPointError(q, truth.Eval(q) + random.Gaus(0., error), 0.002, error);
      }
      cf.Write(Form("cf_%s", centrality));
      TF1 fit(Form("fit_%s", centrality), formula, 0.003, 0.2);
      fit.SetParameters(1., 0.5, 5.);
      cf.Fit(&fit, "Q0N");
      fit.SetTitle(Form("#it{R} = %.1f #pm %.1f fm", fit.GetParameter(2), fit.GetParError(2)));
      fit.SetNpx(400);
      fit.Write();
    }
  }
}

// biophysics: protein backbone angles, stretching DNA, single-molecule FRET, enzyme kinetics and a single ion channel
void WriteBiophysics()
{
  TFile file((kFolder + "biophysics.root").data(), "RECREATE");
  TRandom3 random(26);
  {  // backbone dihedral angles of the residues of many protein structures (Ramachandran plot): right-handed alpha helix,
    // beta sheet, polyproline II and the rare left-handed helix
    TTree tree("dihedrals", "backbone dihedral angles");
    float phi{}, psi{};
    tree.Branch("phi", &phi);
    tree.Branch("psi", &psi);
    struct Region {
      double weight, phi, psi, sigmaPhi, sigmaPsi, rho;
    };
    const std::vector<Region> regions = {{0.5, -63., -43., 10., 11., -0.4}, {0.3, -120., 130., 20., 18., -0.3}, {0.12, -70., 145., 9., 12., 0.}, {0.04, 60., 45., 9., 10., 0.}};
    auto wrap = [](double angle) { return angle - 360. * std::floor((angle + 180.) / 360.); };
    for (int i = 0; i < 40000; ++i) {
      double pick = random.Uniform();
      const Region* region = nullptr;
      for (const auto& candidate : regions) {
        if (pick < candidate.weight) {
          region = &candidate;
          break;
        }
        pick -= candidate.weight;
      }
      if (!region) {  // the rest anywhere
        phi = static_cast<float>(random.Uniform(-180., 180.));
        psi = static_cast<float>(random.Uniform(-180., 180.));
      } else {
        const double u = random.Gaus(), v = random.Gaus();
        phi = static_cast<float>(wrap(region->phi + region->sigmaPhi * u));
        psi = static_cast<float>(wrap(region->psi + region->sigmaPsi * (region->rho * u + std::sqrt(1. - region->rho * region->rho) * v)));
      }
      tree.Fill();
    }
    tree.Write();
  }
  {  // force against extension of lambda-DNA (contour length 16.4 um) in optical tweezers: entropic elasticity of a worm-like
    // chain (persistence length 50 nm, stretch modulus 1000 pN), the overstretching plateau at 65 pN, then single strands
    constexpr double kLength = 16.4, kThermal = 4.11 / 50.;  // um, kT / P in pN
    auto wlc = [](double z) { return kThermal * (0.25 / std::pow(1. - z, 2.) - 0.25 + z); };  // Marko-Siggia, z = extension / L
    auto extension = [&](double force) {  // inverts the worm-like chain and adds the elastic stretch
      double low = 0., high = 0.999999;
      for (int k = 0; k < 60; ++k) (wlc(0.5 * (low + high)) < force ? low : high) = 0.5 * (low + high);
      return kLength * (low + force / 1000.);
    };
    TGraphErrors measured;
    TGraph model;
    for (int i = 0; i <= 40; ++i) {
      const double force = 0.05 * std::pow(64.5 / 0.05, i / 40.);
      const double error = 0.02 * force + 0.05;
      measured.AddPointError(extension(force) * random.Gaus(1., 0.004), force + random.Gaus(0., error), 0., error);
    }
    for (int i = 1; i <= 25; ++i) {  // the plateau, then the stiffer single strands
      const double x = extension(65.) + i * (1.72 * kLength - extension(65.)) / 25.;
      const double force = 65. + 2. * (x / kLength - 1.05);
      measured.AddPointError(x, force + random.Gaus(0., 0.8), 0., 1.3);
    }
    for (int i = 1; i <= 8; ++i) {
      const double x = 1.72 * kLength + 0.3 * i;
      const double force = 66.5 + 3. * i * i * 0.5;
      measured.AddPointError(x, force + random.Gaus(0., 1.), 0., 1.8);
    }
    for (int i = 0; i <= 300; ++i) {
      const double force = 0.03 * std::pow(70. / 0.03, i / 300.);
      model.AddPoint(extension(force), force);
    }
    measured.Write("forceExtension");
    model.Write("wormLikeChain");
  }
  {  // single-molecule FRET efficiencies of a protein that folds: donor-only molecules near zero, unfolded and folded states
    TH1D fret("fretEfficiency", "", 50, -0.1, 1.1);
    fret.GetXaxis()->SetTitle("FRET efficiency #it{E}");
    fret.GetYaxis()->SetTitle("molecules");
    for (const auto& [n, mean, sigma] : std::vector<std::tuple<int, double, double>>{{2500, 0.02, 0.05}, {4000, 0.32, 0.09}, {6000, 0.78, 0.06}}) {
      for (int i = 0; i < n; ++i) fret.Fill(random.Gaus(mean, sigma));
    }
    fret.Write();
    TF1 fit("fretFit", "gaus(0) + gaus(3) + gaus(6)", -0.1, 1.1);
    fit.SetParameters(400., 0., 0.05, 400., 0.3, 0.1, 900., 0.8, 0.06);
    fret.Fit(&fit, "Q0N");
    fit.SetNpx(500);
    fit.Write();
    const double folded = fit.GetParameter(6) * fit.GetParameter(8), unfolded = fit.GetParameter(3) * fit.GetParameter(5);
    for (const auto& [name, first, fraction] : std::vector<std::tuple<const char*, int, double>>{{"fretUnfolded", 3, unfolded / (folded + unfolded)}, {"fretFolded", 6, folded / (folded + unfolded)}}) {
      TF1 component(name, "gaus", -0.1, 1.1);
      component.SetParameters(fit.GetParameter(first), fit.GetParameter(first + 1), fit.GetParameter(first + 2));
      component.SetTitle(Form("%.0f%%", 100. * fraction));
      component.SetNpx(500);
      component.Write();
    }
  }
  {  // Michaelis-Menten kinetics: initial rate against substrate concentration, without and with a competitive inhibitor
    // (same maximal rate, larger apparent K_M), fitted with v = V_max [S] / (K_M + [S])
    for (const auto& [name, km] : std::vector<std::pair<const char*, double>>{{"noInhibitor", 25.}, {"inhibitor", 75.}}) {
      TGraphErrors rate;
      for (const double s : {1., 2., 5., 10., 20., 50., 100., 200., 500.}) {
        const double v = 10. * s / (km + s), error = 0.05 * v + 0.05;
        rate.AddPointError(s, v + random.Gaus(0., error), 0., error);
      }
      rate.Write(Form("rate_%s", name));
      TF1 fit(Form("fit_%s", name), "[0]*x/([1]+x)", 0.5, 1000.);
      fit.SetParameters(10., 30.);
      rate.Fit(&fit, "Q0N");
      fit.SetTitle(Form("#it{K}_{M} = (%.0f #pm %.0f) #muM", fit.GetParameter(1), fit.GetParError(1)));
      fit.SetNpx(500);
      fit.Write();
    }
  }
  {  // a single ion channel in a patch-clamp recording: the current jumps between closed (0 pA) and open (-5 pA), with
    // exponentially distributed dwell times; the open times in logarithmic bins (Sigworth-Sine), with the fit of one exponential
    constexpr double kOpen = 4., kClosed = 12.;  // mean dwell times in ms
    TGraph trace;
    bool open = false;
    double next = random.Exp(kClosed);
    for (int i = 0; i <= 3000; ++i) {
      const double t = 0.1 * i;  // 10 kHz sampling
      while (t > next) {
        open = !open;
        next += random.Exp(open ? kOpen : kClosed);
      }
      trace.AddPoint(t, (open ? -5. : 0.) + random.Gaus(0., 0.35));
    }
    trace.Write("current");
    const auto bins = LogBins(40, 0.1, 100.);
    TH1D dwell("openDwellTimes", "", 40, bins.data());
    dwell.GetXaxis()->SetTitle("open time (ms)");
    dwell.GetYaxis()->SetTitle("events per bin");
    for (int i = 0; i < 4000; ++i) dwell.Fill(random.Exp(kOpen));
    dwell.Write();
    const double width = std::log(1000.) / 40.;  // of a bin in ln t
    TF1 fit("fit_openDwellTimes", Form("[0]*x/[1]*exp(-x/[1])*%.6f", width), 0.1, 100.);
    fit.SetParameters(4000., 3.);
    dwell.Fit(&fit, "Q0N");
    fit.SetTitle(Form("#tau_{open} = (%.2f #pm %.2f) ms", fit.GetParameter(1), fit.GetParError(1)));
    fit.SetNpx(500);
    fit.Write();
  }
}

// theory: the running strong coupling, parton distributions, the QCD equation of state from the lattice, chaos in the
// logistic map, and the Ising model near its critical point
void WriteTheory()
{
  TFile file((kFolder + "theory.root").data(), "RECREATE");
  TRandom3 random(27);
  {  // alpha_s(Q) at two loops from alpha_s(m_Z) = 0.1180 +- 0.0009, the number of flavours changing at the quark masses
    auto running = [](double alphaZ, double q) {
      double alpha = alphaZ, t = 2. * std::log(91.1876);
      const double target = 2. * std::log(q);
      constexpr int kSteps = 2000;
      const double dt = (target - t) / kSteps;
      for (int k = 0; k < kSteps; ++k) {
        const double scale = std::exp(0.5 * (t + 0.5 * dt));
        const int nf = scale > 172.6 ? 6 : scale > 4.18 ? 5 : scale > 1.27 ? 4 : 3;
        const double b0 = 11. - 2. / 3. * nf, b1 = 102. - 38. / 3. * nf;
        auto derivative = [&](double a) { return -b0 / (4. * TMath::Pi()) * a * a - b1 / (16. * TMath::Pi() * TMath::Pi()) * a * a * a; };
        const double k1 = derivative(alpha), k2 = derivative(alpha + 0.5 * dt * k1);
        alpha += dt * k2;
        t += dt;
      }
      return alpha;
    };
    TGraphAsymmErrors band;
    for (int i = 0; i <= 200; ++i) {
      const double q = 1.5 * std::pow(2000. / 1.5, i / 200.);
      const double central = running(0.118, q), up = running(0.1189, q), down = running(0.1171, q);
      band.AddPointError(q, central, 0., 0., central - down, up - central);
    }
    band.Write("alphaS");
    // determinations at different scales (shaped like the PDG summary): tau decays, quarkonia, DIS, e+e- event shapes, the Z,
    // top-quark pairs, jets
    TGraphErrors measured;
    for (const auto& [q, error] : std::vector<std::pair<double, double>>{{1.777, 0.015}, {4.7, 0.012}, {10., 0.010}, {22., 0.006}, {35., 0.005}, {91.19, 0.0025}, {173., 0.004}, {300., 0.004}, {600., 0.005}, {1000., 0.006}, {1500., 0.007}}) {
      measured.AddPointError(q, running(0.118, q) + random.Gaus(0., 0.8 * error), 0., error);
    }
    measured.Write("alphaS_measurements");
  }
  {  // parton distributions x f(x) of the proton at Q^2 = 10 GeV^2 with their uncertainty bands (parameterised shapes):
    // valence up and down, gluons and sea quarks (both divided by 10)
    struct Parton {
      const char* name;
      double norm, a, b, scale;
    };
    for (const auto& [name, norm, a, b, scale] : {Parton{"uv", 4.0, 0.75, 3.2, 1.}, Parton{"dv", 2.2, 0.75, 4.2, 1.}, Parton{"g", 1.6, -0.2, 5.5, 0.1}, Parton{"sea", 0.55, -0.22, 7.5, 0.1}}) {
      TGraphAsymmErrors pdf;
      for (int i = 0; i <= 200; ++i) {
        const double x = 1e-4 * std::pow(1e4, i / 200.) * (i == 200 ? 0.9999 : 1.);
        const double value = scale * norm * std::pow(x, a) * std::pow(1. - x, b);
        const double relative = 0.015 + 0.03 * std::max(0., std::log10(1e-2 / x)) + 0.25 * std::pow(x, 2.);  // wider at small and large x
        pdf.AddPointError(x, value, 0., 0., relative * value, relative * value);
      }
      pdf.Write(Form("pdf_%s", name));
    }
  }
  {  // QCD thermodynamics from the lattice (2+1 flavours, continuum extrapolated; the parameterisation of HotQCD 2014):
    // pressure, energy and entropy density over T^4 (T^3) with their bands, rising through the crossover towards the
    // Stefan-Boltzmann limit of free quarks and gluons
    constexpr double tc = 154.;
    auto pressure = [](double temperature) {
      const double t = temperature / tc;
      const double ideal = 95. * TMath::Pi() * TMath::Pi() / 180.;
      const double numerator = ideal - 8.7704 / t + 3.92 / (t * t) + 0.3419 / std::pow(t, 4.);
      const double denominator = 1. - 1.26 / t + 0.8425 / (t * t) - 0.0475 / std::pow(t, 4.);
      return 0.5 * (1. + std::tanh(3.8706 * (t - 0.9761))) * numerator / denominator;
    };
    TGraphAsymmErrors p3, energy, entropy;
    for (int i = 0; i <= 140; ++i) {
      const double temperature = 130. + 2. * i, h = 0.5;
      const double p = pressure(temperature);
      const double anomaly = temperature * (pressure(temperature + h) - pressure(temperature - h)) / (2. * h);  // (eps - 3p) / T^4
      const double e = anomaly + 3. * p, s = e + p;
      const double relative = 0.03 + 0.05 * std::exp(-(temperature - 130.) / 40.);
      p3.AddPointError(temperature, 3. * p, 0., 0., 3. * p * relative, 3. * p * relative);
      energy.AddPointError(temperature, e, 0., 0., e * relative, e * relative);
      entropy.AddPointError(temperature, 0.75 * s, 0., 0., 0.75 * s * relative, 0.75 * s * relative);
    }
    p3.Write("lattice_3p");
    energy.Write("lattice_energy");
    entropy.Write("lattice_entropy");
    TGraphAsymmErrors crossover;  // T_c = (156.5 +- 1.5) MeV as a box spanning the plot
    crossover.AddPointError(156.5, 8., 1.5, 1.5, 8., 8.);
    crossover.Write("crossover");
  }
  {  // the logistic map x -> r x (1 - x): the long-time values for each r (period doubling into chaos, windows of order),
    // and the Lyapunov exponent, positive where the motion is chaotic
    TH2F bifurcation("bifurcation", "", 600, 2.8, 4., 400, 0., 1.);
    bifurcation.GetXaxis()->SetTitle("#it{r}");
    bifurcation.GetYaxis()->SetTitle("#it{x}");
    bifurcation.GetZaxis()->SetTitle("visits");
    for (int ix = 1; ix <= bifurcation.GetNbinsX(); ++ix) {
      for (int sub = 0; sub < 3; ++sub) {
        const double r = bifurcation.GetXaxis()->GetBinLowEdge(ix) + (sub + 0.5) * bifurcation.GetXaxis()->GetBinWidth(ix) / 3.;
        double x = 0.5;
        for (int k = 0; k < 600; ++k) x = r * x * (1. - x);
        for (int k = 0; k < 300; ++k) {
          x = r * x * (1. - x);
          bifurcation.Fill(r, x);
        }
      }
    }
    bifurcation.Write();
    TGraph lyapunov;
    for (int i = 0; i <= 2400; ++i) {
      const double r = 2.8 + 1.2 * i / 2400.;
      double x = 0.5, sum = 0.;
      for (int k = 0; k < 500; ++k) x = r * x * (1. - x);
      for (int k = 0; k < 2000; ++k) {
        x = r * x * (1. - x);
        sum += std::log(std::max(std::abs(r * (1. - 2. * x)), 1e-12));
      }
      lyapunov.AddPoint(r, std::max(sum / 2000., -2.5));
    }
    lyapunov.Write("lyapunov");
  }
  {  // the 2D Ising model in Monte Carlo simulations on L x L lattices (shaped like finite-size scaling with nu = 1): the
    // Binder cumulants of all sizes cross at T_c, the magnetisation approaches Onsager's exact result as L grows
    constexpr double tc = 2.269185;
    for (const int size : {8, 16, 32, 64}) {
      TGraphErrors binder, magnetisation;
      for (double temperature = 2.0; temperature <= 2.601; temperature += 0.025) {
        const double x = (temperature - tc) / tc * size;
        const double u = (2. / 3.) / (1. + 0.0929 * std::exp(x / 0.9)) + 0.03 / size * (1. - std::tanh(x));
        binder.AddPointError(temperature, u + random.Gaus(0., 0.004), 0., 0.004);
      }
      for (double temperature = 1.5; temperature <= 3.501; temperature += 0.05) {
        const double x = (temperature - tc) / tc * size;
        const double exact = (temperature < tc) ? std::pow(1. - std::pow(std::sinh(2. / temperature), -4.), 0.125) : 0.;
        const double m = std::pow(std::pow(exact, 8.) + 0.1 / size * std::exp(-2. * std::max(x, 0.) / 0.8), 0.125);  // L^(-1/8) at T_c
        magnetisation.AddPointError(temperature, m + random.Gaus(0., 0.004), 0., 0.004);
      }
      binder.Write(Form("binder_L%d", size));
      magnetisation.Write(Form("magnetisation_L%d", size));
    }
    TF1 onsager("onsager", "pow(1-pow(sinh(2/x),-4),0.125)", 1.5, 2.2691);
    onsager.SetNpx(1000);
    onsager.Write();
  }
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
  WriteFlowAndMeanPt();
  WriteNuclear();
  WriteNeutrino();
  WriteSearches();
  WriteNeutronStars();
  WriteCorrelations();
  WriteBiophysics();
  WriteTheory();
}
