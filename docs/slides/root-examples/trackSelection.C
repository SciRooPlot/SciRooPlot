// The plot "trackSelection" of the SciRooPlot example (detector/tracking), written as a plain ROOT macro
// (no SciRooPlot needed): histograms filled from a tree, with and without cuts, in variable bins.
// Run it with:
//   root -l -b -q 'trackSelection.C("<path to ExampleData>")'
void trackSelection(const char* dataDir = "ExampleData", const char* output = "trackSelection.pdf")
{
  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);

  // read the input: the tree "tracks", one entry per track
  TFile file(Form("%s/tracks.root", dataDir));
  auto tree = file.Get<TTree>("tracks");
  if (!tree) {
    Error("trackSelection", "tracks not found in %s/tracks.root", dataDir);
    return;
  }

  // fill the histograms from the tree; the cut uses a quantity computed from the columns
  const std::vector<double> ptBins = {0.1, 0.2, 0.3, 0.5, 0.7, 1.0, 1.5, 2.0, 3.0, 5.0, 10.0};
  TH1D all("all", "", ptBins.size() - 1, ptBins.data());
  TH1D selected("selected", "", ptBins.size() - 1, ptBins.data());
  tree->Draw("pt>>all", "", "goff");
  tree->Draw("pt>>selected", "nClustersTPC > 120 && abs(dcaXY) < 3 * (0.002 + 0.003 / pt)", "goff");

  TCanvas canvas("canvas", "trackSelection");
  canvas.SetCanvasSize(788, 788);
  canvas.SetMargin(0.15, 0.04, 0.15, 0.04);
  canvas.SetTicks(1, 1);
  canvas.SetLogx();
  canvas.SetLogy();
  for (auto axis : {all.GetXaxis(), all.GetYaxis()}) {
    axis->SetLabelFont(43);
    axis->SetLabelSize(31);
    axis->SetTitleFont(43);
    axis->SetTitleSize(39);
  }
  all.GetXaxis()->SetTitle("#it{p}_{T} (GeV/#it{c})");
  all.GetXaxis()->SetTitleOffset(1.2);
  all.GetXaxis()->SetNoExponent();
  all.GetYaxis()->SetTitle("tracks");
  all.GetYaxis()->SetTitleOffset(1.45);
  all.SetMinimum(30.);
  all.SetMaximum(2.e4);
  int color = 0;
  for (auto hist : {&all, &selected}) {
    hist->SetMarkerStyle(kFullCircle);
    hist->SetMarkerColor(color ? kBlue + 1 : kBlack);
    hist->SetLineColor(color ? kBlue + 1 : kBlack);
    hist->SetLineWidth(4);
    hist->Draw(color++ ? "E X0 SAME" : "E X0");
  }

  TLegend legend(0.18, 0.18, 0.7, 0.3);
  legend.SetBorderSize(0);
  legend.SetFillStyle(0);
  legend.SetTextFont(43);
  legend.SetTextSize(31);
  legend.AddEntry(&all, "all tracks", "pe");
  legend.AddEntry(&selected, "#it{N}_{cls} > 120, |DCA_{#it{xy}}| < 3#sigma", "pe");
  legend.Draw();
  canvas.SaveAs(output);
}
