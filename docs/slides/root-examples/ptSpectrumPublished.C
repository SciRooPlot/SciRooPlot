// The plot "ptSpectrumPublished" of the SciRooPlot example (spectra), written as a plain ROOT macro
// (no SciRooPlot needed). The measurement comes from a ROOT file, the published values from a CSV table.
// Run it with:
//   root -l -b -q 'ptSpectrumPublished.C("<path to ExampleData>")'
void ptSpectrumPublished(const char* dataDir = "ExampleData", const char* output = "ptSpectrumPublished.pdf")
{
  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);
  gStyle->SetEndErrorSize(0);

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

  // the frame takes every published point; the measurement is drawn where it has the statistics
  TH1* frame = canvas.DrawFrame(0.1, 1e-5, 20., 15.);
  frame->GetXaxis()->SetTitle(data->GetXaxis()->GetTitle());
  frame->GetYaxis()->SetTitle(data->GetYaxis()->GetTitle());
  for (auto axis : {frame->GetXaxis(), frame->GetYaxis()}) {
    axis->SetLabelFont(43);
    axis->SetLabelSize(31);
    axis->SetTitleFont(43);
    axis->SetTitleSize(39);
    axis->SetTickLength(0.03);
  }
  frame->GetXaxis()->SetNoExponent();
  frame->GetXaxis()->SetTitleOffset(1.0);
  frame->GetYaxis()->SetTitleOffset(1.45);
  data->GetXaxis()->SetRangeUser(0.15, 10.);
  data->SetMarkerStyle(kFullCircle);
  data->SetMarkerColor(kBlack);
  data->SetLineColor(kBlack);
  data->SetLineWidth(5);
  published.SetMarkerStyle(kOpenSquare);
  published.SetMarkerSize(1.6);
  const int blue = TColor::GetColor("#3a78c2");
  published.SetMarkerColor(blue);
  published.SetLineColor(blue);
  published.SetLineWidth(5);
  data->Draw("E X0 SAME");
  published.Draw("P Z SAME");

  TLegend legend(0.184, 0.184, 0.630, 0.288);
  legend.SetMargin(0.12);
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  legend.SetTextFont(43);
  legend.SetTextSize(31);
  legend.AddEntry(data, "this analysis", "pe");
  legend.AddEntry(&published, "published (stat. #oplus syst.)", "pe");
  legend.Draw();

  canvas.SaveAs(output);
}
