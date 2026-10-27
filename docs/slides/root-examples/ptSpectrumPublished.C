// The plot "ptSpectrumPublished" of the SciRooPlot example (spectra), written as a plain ROOT macro
// (no SciRooPlot needed). The measurement comes from a ROOT file, the published values from a CSV table.
// Run it with:
//   root -l -b -q 'ptSpectrumPublished.C("<path to ExampleData>")'
void ptSpectrumPublished(const char* dataDir = "ExampleData", const char* output = "ptSpectrumPublished.pdf")
{
  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);

  // the measurement from the ROOT file
  TFile file(Form("%s/spectra.root", dataDir));
  auto data = file.Get<TH1>("data/ptSpec");
  if (!data) {
    Error("ptSpectrumPublished", "data/ptSpec not found in %s/spectra.root", dataDir);
    return;
  }
  data->SetDirectory(nullptr);

  // the published values from the table: pt,pt_err,value,stat,syst
  TTree table;
  if (table.ReadFile(Form("%s/ptSpec_published.csv", dataDir), "", ',') == 0) {
    Error("ptSpectrumPublished", "could not read %s/ptSpec_published.csv", dataDir);
    return;
  }
  float pt, ptErr, value, stat, syst;  // ReadFile makes float columns
  table.SetBranchAddress("pt", &pt);
  table.SetBranchAddress("pt_err", &ptErr);
  table.SetBranchAddress("value", &value);
  table.SetBranchAddress("stat", &stat);
  table.SetBranchAddress("syst", &syst);
  TGraphErrors published;
  for (Long64_t i = 0; i < table.GetEntries(); ++i) {
    table.GetEntry(i);
    published.SetPoint(i, pt, value);
    published.SetPointError(i, ptErr, std::sqrt(stat * stat + syst * syst));
  }

  // canvas; font 43 (size in pixels) as in the other examples
  TCanvas canvas("canvas", "ptSpectrumPublished");
  canvas.SetCanvasSize(788, 788);
  canvas.SetMargin(0.15, 0.04, 0.15, 0.04);
  canvas.SetLogx();
  canvas.SetLogy();
  canvas.SetTicks(1, 1);

  for (auto axis : {data->GetXaxis(), data->GetYaxis()}) {
    axis->SetLabelFont(43);
    axis->SetLabelSize(31);
    axis->SetTitleFont(43);
    axis->SetTitleSize(39);
    axis->SetTickLength(0.03);
  }
  data->GetXaxis()->SetNoExponent();
  data->GetXaxis()->SetTitleOffset(1.2);
  data->GetYaxis()->SetTitleOffset(1.45);
  data->SetMarkerStyle(kFullCircle);
  data->SetMarkerColor(kBlack);
  data->SetLineColor(kBlack);
  data->SetLineWidth(4);
  published.SetMarkerStyle(kOpenSquare);
  published.SetMarkerSize(1.6);
  published.SetMarkerColor(kAzure + 2);
  published.SetLineColor(kAzure + 2);
  published.SetLineWidth(4);
  data->Draw("E X0");
  published.Draw("P Z SAME");

  TLegend legend(0.19, 0.19, 0.6, 0.29);
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  legend.SetTextFont(43);
  legend.SetTextSize(31);
  legend.AddEntry(data, "this analysis", "pe");
  legend.AddEntry(&published, "published (stat. #oplus syst.)", "pe");
  legend.Draw();

  canvas.SaveAs(output);
}
