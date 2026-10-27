// The plot "ptSpectrumRatio" of the SciRooPlot example, written as a plain ROOT macro
// (no SciRooPlot needed). Run it with:
//   root -l -b -q 'ptSpectrumRatio.C("<path to ExampleData>")'
void ptSpectrumRatio(const char* dataDir = "ExampleData", const char* output = "ptSpectrumRatio.pdf")
{
  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);
  TGaxis::SetMaxDigits(3);

  // read the inputs
  TFile file(Form("%s/spectra.root", dataDir));
  auto data = file.Get<TH1>("data/ptSpec");
  auto mc = file.Get<TH1>("mc/ptSpec");
  if (!data || !mc) {
    Error("ptSpectrumRatio", "ptSpec not found in %s/spectra.root", dataDir);
    return;
  }
  data->SetDirectory(nullptr);
  mc->SetDirectory(nullptr);

  // the ratio
  auto ratio = static_cast<TH1*>(data->Clone("ratio"));
  ratio->Divide(mc);

  // canvas with two pads; font 43 (size in pixels) keeps the text equal in both
  TCanvas canvas("canvas", "ptSpectrumRatio");
  canvas.SetCanvasSize(788, 1024);
  const double split = 0.3457;
  TPad top("top", "", 0., split, 1., 1.);
  TPad bottom("bottom", "", 0., 0., 1., split);
  for (auto pad : {&top, &bottom}) {
    pad->SetLeftMargin(0.15);
    pad->SetRightMargin(0.04);
    pad->SetLogx();
    pad->SetTicks(1, 1);
    canvas.cd();
    pad->Draw();
  }
  top.SetTopMargin(0.048);
  top.SetBottomMargin(0.);
  top.SetLogy();
  bottom.SetTopMargin(0.);
  bottom.SetBottomMargin(0.333);

  auto style = [](TH1* hist, double tickX, double tickY) {
    for (auto axis : {hist->GetXaxis(), hist->GetYaxis()}) {
      axis->SetLabelFont(43);
      axis->SetLabelSize(31);
      axis->SetTitleFont(43);
      axis->SetTitleSize(39);
    }
    hist->GetXaxis()->SetRangeUser(0.15, 15.);
    hist->GetXaxis()->SetNoExponent();
    hist->GetXaxis()->SetTickLength(tickX);
    hist->GetYaxis()->SetTickLength(tickY);
  };

  // upper pad: spectra
  top.cd();
  style(data, 0.035, 0.026);
  data->GetXaxis()->SetLabelSize(0);
  data->GetYaxis()->SetTitleOffset(1.45);
  data->SetMarkerStyle(kFullCircle);
  data->SetMarkerColor(kBlack);
  data->SetLineColor(kBlack);
  data->SetLineWidth(4);
  mc->SetLineColor(kRed + 1);
  mc->SetLineWidth(3);
  data->Draw("E X0");
  mc->Draw("HIST ][ SAME");  // without the vertical lines at the outer edges

  TLegend legend(0.69, 0.81, 0.95, 0.93);
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  legend.SetTextFont(43);
  legend.SetTextSize(31);
  legend.AddEntry(data, "data", "pe");
  legend.AddEntry(mc, "simulation", "l");
  legend.Draw();

  TLatex text;
  text.SetNDC();
  text.SetTextFont(43);
  text.SetTextSize(31);
  text.DrawLatex(0.19, 0.20, "Example Experiment");
  text.DrawLatex(0.19, 0.13, "pp, #sqrt{#it{s}} = 13.6 TeV");
  text.DrawLatex(0.19, 0.06, "|#eta| < 0.8");

  // lower pad: ratio
  bottom.cd();
  style(ratio, 0.067, 0.036);
  ratio->GetYaxis()->SetTitle("data / sim.");
  ratio->GetYaxis()->CenterTitle();
  ratio->GetYaxis()->SetTitleOffset(1.45);
  ratio->GetYaxis()->SetRangeUser(0.7, 1.3);
  ratio->GetYaxis()->SetNdivisions(305);
  ratio->SetMarkerStyle(kFullCircle);
  ratio->SetMarkerSize(1.4);
  ratio->SetMarkerColor(kBlack);
  ratio->SetLineColor(kBlack);
  ratio->SetLineWidth(5);
  ratio->Draw("E X0");
  TLine one(0.15, 1., 15., 1.);
  one.SetLineWidth(4);
  one.Draw();
  ratio->Draw("E X0 SAME");

  canvas.SaveAs(output);
}
