#include "TSystem.h"
#include "TFile.h"
#include "TMath.h"
#include "TH1.h"
#include "TF1.h"
#include "TGraphErrors.h"

shared_ptr<TH1> getExampleMultDist(int nSamples)
{
  auto doubleNBD = [](double* x, double* par) {
    auto NBD = [](double Nch, double A, double meanNch, double k) {
      return A * TMath::Gamma(Nch + k) / (TMath::Gamma(Nch + 1) * TMath::Gamma(k)) * std::pow(meanNch / (k + meanNch), Nch) * 1 / std::pow(1.0 + meanNch / k, k);
    };
    auto Nch = x[0];
    auto A = par[0];
    auto alphaSoft = par[1];
    auto meanNchSoft = par[2];
    auto kSoft = par[3];
    auto meanNchSemihard = par[4];
    auto kSemihard = par[5];
    return A * (alphaSoft * NBD(Nch, 1, meanNchSoft, kSoft) + (1 - alphaSoft) * NBD(Nch, 1, meanNchSemihard, kSemihard));
  };
  shared_ptr<TF1> multDistNBD{new TF1("NBD", doubleNBD, 1., 100, 6)};
  multDistNBD->SetParNames("A", "alphaSoft", "meanNchSoft", "kSoft", "meanNchSemihard", "kSemihard");
  multDistNBD->SetParameters(1.07815823,0.59482043,4.51789303,1.62701245,16.94642265,3.20841373);
  shared_ptr<TH1> multDist{new TH1D("multDist", "", 70, 0.5, 70.5)};
  multDist->Sumw2();
  multDist->FillRandom("NBD", nSamples);
  multDist->Scale(1, "width");
  multDist->GetXaxis()->SetTitle("#it{N}_{ch}");
  multDist->GetYaxis()->SetTitle("events");
  return multDist;
}
shared_ptr<TH1> getExamplePtSpec(int nSamples)
{
  auto tcm = [](double* x, double* par) {
    auto pT = x[0];
    auto Ae = par[0];
    auto Te = par[1];
    auto A = par[2];
    auto T = par[3];
    auto n = par[4];
    const double m = 0.13957039; // assume pion mass
    double y = Ae * std::exp(-(std::sqrt(pT * pT + m * m) - m) / Te) + A / (std::pow(1 + (pT * pT) / (T * T * n), n));
    return pT * y; // multiply by pT since spectra are not divided by pT
  };
  shared_ptr<TF1> ptSpecTCM{new TF1("TCM", tcm, 0.1, 20., 5)};
  ptSpecTCM->SetParNames("Ae", "Te", "A", "T", "n");
  ptSpecTCM->SetParameters(95.31849945,0.16398725,6.60023282,0.74124785,3.06268642);
  const std::vector<double> ptBins = {0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5, 0.55, 0.6, 0.65, 0.7, 0.75,
                                0.8, 0.85, 0.9, 0.95, 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9,
                                2.0, 2.2, 2.4, 2.6, 2.8, 3.0, 3.2, 3.4, 3.6, 3.8, 4.0, 4.5, 5.0, 5.5,
    6.0, 6.5, 7.0, 8.0, 9.0, 10.0, 11., 12., 13., 14., 15., 16., 18., 20.};
  shared_ptr<TH1> ptSpec{new TH1D("ptSpec", "", ptBins.size()-1, ptBins.data())};
  ptSpec->Sumw2();
  ptSpec->FillRandom("TCM", nSamples);
  ptSpec->Scale(1, "width");
  ptSpec->GetXaxis()->SetTitle("#it{p}_{T} (GeV/#it{c})");
  ptSpec->GetYaxis()->SetTitle("d#it{N}/d#it{p}_{T}");
  return ptSpec;
}
shared_ptr<TH1> getExampleInvMassDist(int nSamples)
{
  shared_ptr<TF1> invMassDistParam{new TF1("invMassDistParam", "(x<[1])*([0]*(TMath::Exp(-0.5*((x-[1])/[2])^2)+TMath::Exp((x-[1])/[3])*(1.-TMath::Exp(-0.5*((x-[1])/[2])^2))))+(x>=[1])*([0]*(TMath::Exp(-0.5*((x-[1])/[2])^2)+TMath::Exp(-(x-[1])/[4])*(1.-TMath::Exp(-0.5*((x-[1])/[2])^2)))) + (x<[6])*([5]*(TMath::Exp(-0.5*((x-[6])/[7])^2)+TMath::Exp((x-[6])/[8])*(1.-TMath::Exp(-0.5*((x-[6])/[7])^2))))+(x>=[6])*([5]*TMath::Exp(-0.5*((x-[6])/[7])^2))", 0,0.8)};
  invMassDistParam->SetParameters(33294.07,0.1387916,0.01123336,0.01292341,0.02151752,2897.386,0.5596612,0.02439279,0.03186518);
  shared_ptr<TH1> invMassDist{new TH1D("invMassDist", "", 200,0,0.8)};
  invMassDist->Sumw2();
  invMassDist->FillRandom("invMassDistParam", nSamples);
  invMassDist->Scale(1, "width");
  invMassDist->GetXaxis()->SetTitle("#it{m}_{#gamma#gamma} (GeV/#it{c}^{2})");
  invMassDist->GetYaxis()->SetTitle("d#it{N}/d#it{m}_{#gamma#gamma}");
  return invMassDist;
}

void generateTestFiles()
{
  string folder = "TestFiles/";
  gSystem->Exec((string("mkdir -p ") + folder).data());
  gSystem->Exec((string("mkdir -p ") + folder + "/subfolder").data());

  TFile file1((folder + "file1.root").data(), "RECREATE");
  TFile file2((folder + "file2.root").data(), "RECREATE");
  TFile file3((folder + "/subfolder/" + "file3.root").data(), "RECREATE");

  TF1 func1("func1", "0.7 + sin(x)/x", 0, 60);

  double x[] = {10, 15, 20, 25, 30};
  double y[] = {0.65, 0.6, 0.65, 0.7, 0.77};
  double ex[] = {0.1, 0.2, 0.3, 0.4, 0.5};
  double ey[] = {0.01, 0.008, 0.005, 0.005, 0.01};
  TGraphErrors graph1(5, x, y, ex, ey);
  graph1.SetName("graph1");
  file1.cd();
  graph1.Write();

  file1.mkdir("dir1");
  file1.cd("dir1");
  func1.Write();

  TH1F hist1("hist1", "", 100, 0, 6);
  hist1.FillRandom("gaus", 10000);
  hist1.Sumw2();
  hist1.Write();

  file1.mkdir("dir2/subdir1");
  file1.cd("dir2/subdir1");

  TF1 mygaus("mygaus", "TMath::Gaus(x,3,.5)", 0, 6);

  TH1F hist2("hist2", "", 100, 0, 6);
  hist2.FillRandom("mygaus", 10000);
  hist2.Sumw2();
  hist2.Write();

  TF1 mygaus2("mygaus2", "TMath::Gaus(x,4,.8)", 0, 6);
  TH1F hist3("hist3", "", 100, 0, 6);
  hist3.FillRandom("mygaus2", 10000);
  hist3.Sumw2();
  hist3.Write();

  file1.mkdir("dir2/subdir2");
  file1.cd("dir2/subdir2");

  TF1 mygaus3("mygaus3", "TMath::Gaus(x,2,.8)", 0, 6);
  TH1F hist3b("hist3", "", 100, 0, 6);
  hist3b.FillRandom("mygaus3", 10000);
  hist3b.Sumw2();
  hist3b.Write();

  file2.cd();

  TF1 mygaus4("mygaus4", "TMath::Gaus(x,5,.8)", 0, 6);
  TH1F hist4("hist4", "", 100, 0, 6);
  hist4.FillRandom("mygaus4", 100000);
  hist4.Sumw2();
  hist4.Write();

  TList list1;
  list1.SetOwner(false);

  TF1 mygaus5("mygaus5", "TMath::Gaus(x,3.3,.1)", 0, 6);
  TH1F hist5("hist5", "", 100, 0, 6);
  hist5.FillRandom("mygaus5", 100000);
  hist5.Sumw2();
  list1.Add(&hist5);

  TF1 mygaus6("mygaus6", "TMath::Gaus(x,0.3,.2)", 0, 6);
  TH1F hist6("hist6", "", 100, 0, 6);
  hist6.FillRandom("mygaus6", 100000);
  hist6.Sumw2();
  list1.Add(&hist6);

  list1.Write("list1", TObject::kSingleKey);

  list1.Clear();

  file3.cd();

  TF1 mysin("mysin", "x + x*sin(x)", 0, 6);
  TH1F hist7("hist7", "", 100, 0, 6);
  hist7.FillRandom("mysin", 100000);
  hist7.Sumw2();
  hist7.Write();

  TF1 mysin2("mysin2", "x + x*sin(2*x)", 0, 6);
  TH1F hist8("hist8", "", 100, 0, 6);
  hist8.FillRandom("mysin2", 100000);
  hist8.Sumw2();
  hist8.Write();
  

  
  file3.mkdir("pseudodata");
  file3.cd("pseudodata");

  int nSamples = 100000;
  auto multDist = getExampleMultDist(nSamples);
  multDist->Write();
  auto ptSpec = getExamplePtSpec(nSamples);
  ptSpec->Write();
  auto invMassDist = getExampleInvMassDist(nSamples);
  invMassDist->Write();

}
