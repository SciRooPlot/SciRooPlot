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
  data->Scale(1.0 / data->Integral());
  mc->Scale(1.0 / mc->Integral());
  auto ratio = static_cast<TH1*>(data->Clone("ratio"));
  ratio->Divide(mc);

  // canvas with two pads; font 43 (size in pixels) keeps the text equal in both
  TCanvas canvas("canvas", "vertexZ");
  canvas.SetCanvasSize(788, 1024);
  const double split = 0.3457;
  TPad top("top", "", 0.0, split, 1.0, 1.0);
  TPad bottom("bottom", "", 0.0, 0.0, 1.0, split);
  for (auto pad : {&top, &bottom}) {
    pad->SetLeftMargin(0.15);
    pad->SetRightMargin(0.04);
    pad->SetTicks(1, 1);
    canvas.cd();
    pad->Draw();
  }
  top.SetTopMargin(0.048);
  top.SetBottomMargin(0.0);
  bottom.SetTopMargin(0.0);
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
  data->GetYaxis()->SetTitleOffset(1.12);
  data->SetMaximum(0.036);
  data->SetMinimum(0.0);
  data->GetYaxis()->ChangeLabel(1, -1, 0);  // no label at the lower edge, where the ratio pad begins
  data->SetMarkerStyle(kFullCircle);
  data->SetMarkerSize(1.4);
  data->SetMarkerColor(kBlack);
  data->SetLineColor(kBlack);
  data->SetLineWidth(5);
  mc->SetLineColor(TColor::GetColor("#d6403a"));
  mc->SetLineWidth(3);
  data->Draw("E X0");
  mc->Draw("HIST SAME");
  data->Draw("E X0 SAME");

  TLegend legend(0.190, 0.773, 0.418, 0.895);
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
  ratio->GetYaxis()->SetTitleOffset(1.27);
  ratio->GetYaxis()->SetRangeUser(0.5, 2.5);
  ratio->GetYaxis()->SetNdivisions(305);
  ratio->GetYaxis()->ChangeLabel(-1, -1, 0);  // no label at the upper edge, where the upper pad begins
  ratio->SetMarkerStyle(kFullCircle);
  ratio->SetMarkerSize(1.4);
  ratio->SetMarkerColor(kBlack);
  ratio->SetLineColor(kBlack);
  ratio->SetLineWidth(5);
  ratio->Draw("AXIS");
  TF1 one("one", "1", ratio->GetXaxis()->GetXmin(), ratio->GetXaxis()->GetXmax());
  one.SetLineStyle(kDashed);
  one.SetLineWidth(2);
  one.SetLineColor(kGray + 2);
  one.Draw("SAME");
  ratio->DrawCopy("E X0 SAME")->GetXaxis()->SetRangeUser(-20.0, 20.0);  // where both have enough events

  canvas.SaveAs(output);
}
