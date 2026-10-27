// The plot "trackingEfficiency" of the SciRooPlot example (detector/tracking), written as a plain ROOT macro
// (no SciRooPlot needed): an efficiency from two histograms, with binomial errors.
// Run it with:
//   root -l -b -q 'trackingEfficiency.C("<path to ExampleData>")'
void trackingEfficiency(const char* dataDir = "ExampleData", const char* output = "trackingEfficiency.pdf")
{
  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);

  // read the inputs: reconstructed and generated tracks, in the TList "output" of the directory "TrackQA"
  TFile file(Form("%s/AnalysisResults_mc.root", dataDir));
  auto list = file.Get<TList>("TrackQA/output");
  auto rec = list ? static_cast<TH1*>(list->FindObject("hPt")) : nullptr;
  auto gen = list ? static_cast<TH1*>(list->FindObject("hPtGen")) : nullptr;
  if (!rec || !gen) {
    Error("trackingEfficiency", "TrackQA/output/hPt(Gen) not found in %s/AnalysisResults_mc.root", dataDir);
    return;
  }

  // binomial errors: the reconstructed tracks are a subset of the generated ones
  auto efficiency = static_cast<TH1*>(rec->Clone("efficiency"));
  efficiency->SetDirectory(nullptr);
  efficiency->Divide(rec, gen, 1., 1., "B");

  TCanvas canvas("canvas", "trackingEfficiency");
  canvas.SetCanvasSize(788, 788);
  canvas.SetMargin(0.15, 0.04, 0.15, 0.04);
  canvas.SetTicks(1, 1);
  canvas.SetLogx();
  canvas.SetGridy();
  for (auto axis : {efficiency->GetXaxis(), efficiency->GetYaxis()}) {
    axis->SetLabelFont(43);
    axis->SetLabelSize(31);
    axis->SetTitleFont(43);
    axis->SetTitleSize(39);
  }
  efficiency->GetXaxis()->SetRangeUser(0.1, 10.);
  efficiency->GetXaxis()->SetNoExponent();
  efficiency->GetXaxis()->SetTitleOffset(1.2);
  efficiency->GetYaxis()->SetTitle("tracking efficiency");
  efficiency->GetYaxis()->SetTitleOffset(1.45);
  efficiency->SetMinimum(0.);
  efficiency->SetMaximum(1.);
  efficiency->SetMarkerStyle(kFullCircle);
  efficiency->SetMarkerColor(kBlack);
  efficiency->SetLineColor(kBlack);
  efficiency->SetLineWidth(4);
  efficiency->Draw("E X0");
  canvas.SaveAs(output);
}
