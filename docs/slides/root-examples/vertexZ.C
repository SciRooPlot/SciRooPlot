// The plot "vertexZ" of the SciRooPlot example (detector/events), written as a plain ROOT macro
// (no SciRooPlot needed). The histograms live in a TList inside a directory, in two different files.
// Run it with:
//   root -l -b -q 'vertexZ.C("<path to ExampleData>")'
void vertexZ(const char* dataDir = "ExampleData", const char* output = "vertexZ.pdf")
{
  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);
  TGaxis::SetMaxDigits(3);

  // read the inputs: a TList called "output" in the directory "EventQA" of each file
  auto load = [&](const char* fileName) -> TH1* {
    TFile file(Form("%s/%s", dataDir, fileName));
    auto list = file.Get<TList>("EventQA/output");
    auto hist = list ? static_cast<TH1*>(list->FindObject("hVtxZ")) : nullptr;
    if (!hist) {
      Error("vertexZ", "EventQA/output/hVtxZ not found in %s", fileName);
      return nullptr;
    }
    hist->SetDirectory(nullptr);
    return hist;
  };
  auto data = load("AnalysisResults_data.root");
  auto mc = load("AnalysisResults_mc.root");
  if (!data || !mc) return;

  // normalise both, then divide
  data->Scale(1. / data->Integral());
  mc->Scale(1. / mc->Integral());
  auto ratio = static_cast<TH1*>(data->Clone("ratio"));
  ratio->Divide(mc);

  // canvas with two pads; font 43 (size in pixels) keeps the text equal in both
  TCanvas canvas("canvas", "vertexZ");
  canvas.SetCanvasSize(788, 1024);
  const double split = 0.3457;
  TPad top("top", "", 0., split, 1., 1.);
  TPad bottom("bottom", "", 0., 0., 1., split);
  for (auto pad : {&top, &bottom}) {
    pad->SetLeftMargin(0.15);
    pad->SetRightMargin(0.04);
    pad->SetTicks(1, 1);
    canvas.cd();
    pad->Draw();
  }
  top.SetTopMargin(0.048);
  top.SetBottomMargin(0.);
  bottom.SetTopMargin(0.);
  bottom.SetBottomMargin(0.333);

  auto style = [](TH1* hist, double tickX, double tickY) {
    for (auto axis : {hist->GetXaxis(), hist->GetYaxis()}) {
      axis->SetLabelFont(43);
      axis->SetLabelSize(31);
      axis->SetTitleFont(43);
      axis->SetTitleSize(39);
    }
    hist->GetXaxis()->SetTickLength(tickX);
    hist->GetYaxis()->SetTickLength(tickY);
  };

  // upper pad: the two distributions
  top.cd();
  style(data, 0.035, 0.026);
  data->GetXaxis()->SetLabelSize(0);
  data->GetYaxis()->SetTitle("normalised counts");
  data->GetYaxis()->SetTitleOffset(1.45);
  data->SetMaximum(0.036);
  data->SetMinimum(0.);
  data->SetMarkerStyle(kFullCircle);
  data->SetMarkerColor(kBlack);
  data->SetLineColor(kBlack);
  data->SetLineWidth(4);
  mc->SetLineColor(kRed + 1);
  mc->SetLineWidth(3);
  data->Draw("E X0");
  mc->Draw("HIST SAME");
  data->Draw("E X0 SAME");

  TLegend legend(0.189, 0.80, 0.45, 0.92);
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  legend.SetTextFont(43);
  legend.SetTextSize(31);
  legend.AddEntry(data, "data", "pe");
  legend.AddEntry(mc, "simulation", "l");
  legend.Draw();

  // lower pad: ratio
  bottom.cd();
  style(ratio, 0.067, 0.036);
  ratio->GetYaxis()->SetTitle("data / sim.");
  ratio->GetYaxis()->CenterTitle();
  ratio->GetYaxis()->SetTitleOffset(1.45);
  ratio->GetYaxis()->SetRangeUser(-1., 5.);
  ratio->GetYaxis()->SetNdivisions(305);
  ratio->SetMarkerStyle(kFullCircle);
  ratio->SetMarkerColor(kBlack);
  ratio->SetLineColor(kBlack);
  ratio->SetLineWidth(4);
  ratio->Draw("E X0");
  TLine one(ratio->GetXaxis()->GetXmin(), 1., ratio->GetXaxis()->GetXmax(), 1.);
  one.SetLineWidth(4);
  one.Draw();
  ratio->Draw("E X0 SAME");

  canvas.SaveAs(output);
}
