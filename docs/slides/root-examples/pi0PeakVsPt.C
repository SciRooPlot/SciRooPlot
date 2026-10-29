// The plot "pi0PeakVsPt" of the SciRooPlot example (detector/calorimeter), written as a plain ROOT macro
// (no SciRooPlot needed): six panels in two rows, one bin of pair pT each, with the background from mixed events
// scaled to the same-event pairs in a normalisation region.
// Run it with:
//   root -l -b -q 'pi0PeakVsPt.C("<path to ExampleData>")'
void pi0PeakVsPt(const char* dataDir = "ExampleData", const char* output = "pi0PeakVsPt.pdf")
{
  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);

  // read the inputs: two TH2 in the TList "output" of the directory "CaloQA"
  TFile file(Form("%s/AnalysisResults_data.root", dataDir));
  auto list = file.Get<TList>("CaloQA/output");
  auto same2d = list ? static_cast<TH2*>(list->FindObject("hInvMassGGVsPt")) : nullptr;
  auto mixed2d = list ? static_cast<TH2*>(list->FindObject("hInvMassGGVsPtMixed")) : nullptr;
  if (!same2d || !mixed2d) {
    Error("pi0PeakVsPt", "CaloQA/output/hInvMassGGVsPt(Mixed) not found in %s/AnalysisResults_data.root", dataDir);
    return;
  }

  // mass in a pT bin, in MeV, normalised to its integral in the normalisation region
  const double normLow = 190.0, normHigh = 240.0;
  auto project = [&](TH2* hist, double low, double high, const char* name) {
    const int first = hist->GetYaxis()->FindFixBin(low);
    const int last = hist->GetYaxis()->FindFixBin(high) - 1;  // high is the lower edge of the next bin
    auto proj = hist->ProjectionX(name, first, last);
    proj->SetDirectory(nullptr);
    proj->GetXaxis()->Set(proj->GetNbinsX(), 1000.0 * proj->GetXaxis()->GetXmin(), 1000.0 * proj->GetXaxis()->GetXmax());
    const int normFirst = proj->GetXaxis()->FindFixBin(normLow);
    const int normLast = proj->GetXaxis()->FindFixBin(normHigh) - 1;
    proj->Scale(1.0 / proj->Integral(normFirst, normLast));
    return proj;
  };

  // canvas with 3 x 2 pads of the same size, each with its own axes
  TCanvas canvas("canvas", "pi0PeakVsPt");
  canvas.SetCanvasSize(3 * 788, 2 * 788);
  canvas.Divide(3, 2, 0.0, 0.0);
  const std::vector<std::pair<double, double>> ptBins = {{1.0, 1.5}, {1.5, 2.0}, {2.0, 3.0}, {3.0, 4.0}, {4.0, 6.0}, {6.0, 10.0}};
  std::vector<TObject*> keep;  // histograms, lines and texts must live until the canvas is saved
  for (int i = 0; i < 6; ++i) {
    auto pad = canvas.cd(i + 1);
    pad->SetMargin(0.15, 0.04, 0.15, 0.04);
    pad->SetTicks(1, 1);

    const auto [low, high] = ptBins[i];
    auto same = project(same2d, low, high, Form("same%d", i));
    auto mixed = project(mixed2d, low, high, Form("mixed%d", i));
    keep.push_back(same);
    keep.push_back(mixed);
    for (auto axis : {same->GetXaxis(), same->GetYaxis()}) {
      axis->SetLabelFont(43);
      axis->SetLabelSize(31);
      axis->SetTitleFont(43);
      axis->SetTitleSize(39);
      axis->SetTickLength(0.03);
    }
    same->GetXaxis()->SetRangeUser(60.0, 240.0);
    same->GetXaxis()->SetNdivisions(503);
    same->GetXaxis()->SetTitle("#it{m}_{#gamma#gamma} (MeV/#it{c}^{2})");
    same->GetXaxis()->SetTitleOffset(1.2);
    same->GetYaxis()->SetTitle("pairs (norm. region = 1)");
    same->GetYaxis()->SetTitleOffset(1.45);
    same->SetMinimum(0.0);
    same->SetMarkerStyle(kFullCircle);
    same->SetMarkerColor(kBlack);
    same->SetLineColor(kBlack);
    same->SetLineWidth(4);
    mixed->SetFillColor(kAzure - 9);
    mixed->SetLineColor(kAzure - 9);
    // the same-event pairs define the frame, the background is drawn below them
    same->Draw("AXIS");
    mixed->Draw("HIST SAME");
    same->Draw("E X0 SAME");
    same->Draw("AXIS SAME");

    auto line = new TLine(normLow, 0.05, normHigh, 0.05);  // 20 bins: 1/20 each
    keep.push_back(line);
    line->SetLineColor(kRed + 1);
    line->SetLineWidth(5);
    line->Draw();

    auto text = new TLatex(0.6, 0.88, Form("%g < #it{p}_{T} < %g GeV/#it{c}", low, high));
    keep.push_back(text);
    text->SetNDC();
    text->SetTextFont(43);
    text->SetTextSize(31);
    text->Draw();
    if (i == 0) {
      auto legend = new TLegend(0.6, 0.66, 0.96, 0.81);
      keep.push_back(legend);
      legend->SetBorderSize(0);
      legend->SetFillStyle(0);
      legend->SetTextFont(43);
      legend->SetTextSize(31);
      legend->AddEntry(mixed, "mixed event", "f");
      legend->AddEntry(same, "same event", "pe");
      legend->AddEntry(line, "norm. region", "l");
      legend->Draw();
    }
  }
  canvas.SaveAs(output);
}
