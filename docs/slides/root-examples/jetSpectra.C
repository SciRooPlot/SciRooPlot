// The plot "spectra" of the SciRooPlot example (collisions/jets), written as a plain ROOT macro
// (no SciRooPlot needed): one input per file of a directory, and a theory curve that sets the axis range.
// Run it with:
//   root -l -b -q 'jetSpectra.C("<path to ExampleData>")'
void jetSpectra(const char* dataDir = "ExampleData", const char* output = "jetSpectra.pdf")
{
  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);

  // read the inputs: one file per jet radius in the directory "jets", the NLO curve in the file for R = 0.4
  TFile nloFile(Form("%s/jets/jets_R04.root", dataDir));
  auto nlo = nloFile.Get<TF1>("fNLO_R04");
  if (!nlo) {
    Error("jetSpectra", "fNLO_R04 not found in %s/jets/jets_R04.root", dataDir);
    return;
  }
  const std::vector<std::pair<std::string, std::string>> radii = {{"R02", "#it{R} = 0.2"}, {"R04", "#it{R} = 0.4"}, {"R06", "#it{R} = 0.6"}};
  const Color_t colors[3] = {kBlack, kBlue + 1, kRed + 1};
  std::vector<TH1*> spectra;
  for (const auto& [tag, label] : radii) {
    TFile file(Form("%s/jets/jets_%s.root", dataDir, tag.data()));
    auto hist = file.Get<TH1>(("hJetPt_" + tag).data());
    if (!hist) {
      Error("jetSpectra", "hJetPt_%s not found in %s/jets/jets_%s.root", tag.data(), dataDir, tag.data());
      return;
    }
    hist->SetDirectory(nullptr);
    hist->Rebin(2);
    spectra.push_back(hist);
  }

  TCanvas canvas("canvas", "jetSpectra");
  canvas.SetCanvasSize(788, 788);
  canvas.SetMargin(0.15, 0.04, 0.15, 0.04);
  canvas.SetTicks(1, 1);
  canvas.SetLogx();
  canvas.SetLogy();

  // the curve spans a wider range than the data, so it is drawn first and makes the axes
  nlo->SetLineColor(kGray + 2);
  nlo->SetLineStyle(kDashed);
  nlo->SetLineWidth(2);
  nlo->SetNpx(500);
  nlo->Draw();
  auto frame = nlo->GetHistogram();
  for (auto axis : {frame->GetXaxis(), frame->GetYaxis()}) {
    axis->SetLabelFont(43);
    axis->SetLabelSize(31);
  }
  frame->GetXaxis()->SetNoExponent();
  TLegend legend(0.212, 0.2, 0.6, 0.454);
  legend.SetHeader("anti-#it{k}_{T} jets, |#eta| < 0.5");
  for (size_t i = 0; i < spectra.size(); ++i) {
    spectra[i]->SetMarkerStyle(kFullSquare);
    spectra[i]->SetMarkerSize(1.2);
    spectra[i]->SetMarkerColor(colors[i]);
    spectra[i]->SetLineColor(colors[i]);
    spectra[i]->SetLineWidth(4);
    spectra[i]->Draw("E X0 SAME");
    legend.AddEntry(spectra[i], radii[i].second.data(), "pe");
  }
  nlo->Draw("SAME");
  legend.AddEntry(nlo, "NLO, #it{R} = 0.4", "l");
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  legend.SetTextFont(43);
  legend.SetTextSize(31);
  if (auto header = static_cast<TLegendEntry*>(legend.GetListOfPrimitives()->First())) { header->SetTextFont(43); header->SetTextSize(31); }
  legend.Draw();
  canvas.SaveAs(output);
}
