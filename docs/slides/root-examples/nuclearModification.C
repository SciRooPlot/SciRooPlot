// The plot "nuclearModification" of the SciRooPlot example (heavyion), written as a plain ROOT macro
// (no SciRooPlot needed): R_AA in three touching panels, with published values from a CSV table in the first.
// Run it with:
//   root -l -b -q 'nuclearModification.C("<path to ExampleData>")'
void nuclearModification(const char* dataDir = "ExampleData", const char* output = "nuclearModification.pdf")
{
  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);

  // read the inputs: the pp spectrum, the Pb-Pb spectra per centrality class, the published table
  TFile ppFile(Form("%s/spectra.root", dataDir));
  TFile pbpbFile(Form("%s/heavyion.root", dataDir));
  auto pp = ppFile.Get<TH1>("data/ptSpec");
  if (!pp) {
    Error("nuclearModification", "data/ptSpec not found in %s/spectra.root", dataDir);
    return;
  }
  const std::vector<std::pair<std::string, double>> centralities = {{"0-10", 1500.}, {"10-30", 750.}, {"30-50", 260.}};  // <N_coll>
  TTree table;
  if (table.ReadFile(Form("%s/raa_published.csv", dataDir), "", ',') <= 0) {
    Error("nuclearModification", "cannot read %s/raa_published.csv", dataDir);
    return;
  }
  float pt, ptErr, raa, stat, systLow, systHigh;  // ReadFile makes float columns
  table.SetBranchAddress("pt", &pt);
  table.SetBranchAddress("pt_err", &ptErr);
  table.SetBranchAddress("raa", &raa);
  table.SetBranchAddress("stat", &stat);
  table.SetBranchAddress("syst_low", &systLow);
  table.SetBranchAddress("syst_high", &systHigh);
  TGraphAsymmErrors syst;
  TGraphErrors statistical;
  for (Long64_t i = 0; i < table.GetEntries(); ++i) {
    table.GetEntry(i);
    syst.AddPointError(pt, raa, ptErr, ptErr, systLow, systHigh);
    statistical.AddPointError(pt, raa, 0., stat);
  }

  // three pads of equal frame width that share the y axis: only the first one has room for its labels
  TCanvas canvas("canvas", "nuclearModification");
  canvas.SetCanvasSize(2064, 788);
  const double edges[4] = {0., 756., 1394., 2064.};
  std::vector<TObject*> keep;  // pads, histograms, lines, texts and legends must live until the canvas is saved
  for (size_t i = 0; i < centralities.size(); ++i) {
    canvas.cd();
    auto pad = new TPad(Form("pad%zu", i), "", edges[i] / 2064., 0., edges[i + 1] / 2064., 1.);
    keep.push_back(pad);
    pad->SetMargin(i == 0 ? 118. / 756. : 0., i == 2 ? 32. / 670. : 0., 0.15, 0.04);
    pad->SetTicks(1, 1);
    pad->SetLogx();
    pad->Draw();
    pad->cd();

    const auto& [cent, ncoll] = centralities[i];
    auto pbpb = pbpbFile.Get<TH1>(("cent_" + cent + "/ptSpec").data());
    if (!pbpb) {
      Error("nuclearModification", "cent_%s/ptSpec not found", cent.data());
      return;
    }
    auto ratio = static_cast<TH1*>(pbpb->Clone(Form("raa%zu", i)));
    ratio->SetDirectory(nullptr);
    keep.push_back(ratio);
    ratio->Scale(1. / ncoll);
    ratio->Divide(pp);
    const double width = (edges[i + 1] - edges[i]) / 2064.;  // pixel sizes per pad: same look in all three
    for (auto axis : {ratio->GetXaxis(), ratio->GetYaxis()}) {
      axis->SetLabelFont(43);
      axis->SetLabelSize(i == 0 || axis == ratio->GetXaxis() ? 31 : 0);
      axis->SetTitleFont(43);
      axis->SetTitleSize(39);
    }
    ratio->GetXaxis()->SetRangeUser(0.15, 10.);
    ratio->GetXaxis()->SetTitle("#it{p}_{T} (GeV/#it{c})");
    ratio->GetXaxis()->SetTitleOffset(1.2);
    ratio->GetXaxis()->SetNoExponent();
    ratio->GetXaxis()->SetTickLength(0.03 / width * 0.38);
    ratio->GetYaxis()->SetTitle(i == 0 ? "#it{R}_{AA}" : "");
    ratio->GetYaxis()->CenterTitle();
    ratio->GetYaxis()->SetTitleOffset(1.45);
    ratio->GetYaxis()->SetTickLength(0.03 * 0.38 / width);
    ratio->SetMinimum(0.);
    ratio->SetMaximum(1.4);
    ratio->SetMarkerStyle(kFullCircle);
    ratio->SetMarkerColor(kBlack);
    ratio->SetLineColor(kBlack);
    ratio->SetLineWidth(4);
    ratio->Draw("E X0");

    auto one = new TLine(0.15, 1., 10., 1.);
    keep.push_back(one);
    one->SetLineColor(kGray + 1);
    one->SetLineStyle(kDashed);
    one->SetLineWidth(2);
    one->Draw();
    auto text = new TLatex(0.5, 0.88, (cent + "%").data());
    keep.push_back(text);
    text->SetNDC();
    text->SetTextAlign(21);
    text->SetTextFont(43);
    text->SetTextSize(31);
    text->Draw();

    if (i == 0) {
      syst.SetFillColor(kOrange - 9);
      syst.SetLineColor(kOrange + 1);
      syst.Draw("2");
      statistical.SetMarkerStyle(kOpenCircle);
      statistical.SetMarkerSize(1.2);
      statistical.SetMarkerColor(kOrange + 1);
      statistical.SetLineColor(kOrange + 1);
      statistical.Draw("P Z");
      ratio->Draw("E X0 SAME");
      auto published = new TLegend(0.2, 0.62, 0.7, 0.78);
      keep.push_back(published);
      published->AddEntry(&syst, "published, syst.", "f");
      published->AddEntry(&statistical, "published, stat.", "pe");
      auto measured = new TLegend(0.2, 0.22, 0.6, 0.3);
      keep.push_back(measured);
      measured->AddEntry(ratio, "Pb-Pb / pp", "pe");
      for (auto legend : {published, measured}) {
        legend->SetBorderSize(0);
        legend->SetFillStyle(0);
        legend->SetTextFont(43);
        legend->SetTextSize(31);
        legend->Draw();
      }
    }
    if (i == 2) {
      auto box = new TH1D("normBox", "", 1, 0., 1.);  // a legend row needs an object with the fill
      keep.push_back(box);
      box->SetFillColor(kGray);
      auto note = new TLegend(0.1, 0.22, 0.6, 0.3);
      keep.push_back(note);
      note->AddEntry(box, "norm. unc. 5%", "f");
      note->SetBorderSize(0);
      note->SetFillStyle(0);
      note->SetTextFont(43);
      note->SetTextSize(31);
      note->Draw();
    }
  }
  canvas.SaveAs(output);
}
