// The plot "nuclearModification" of the SciRooPlot example (heavyion), written as a plain ROOT macro
// (no SciRooPlot needed): R_AA in three touching panels, with published values from a CSV table in the first, and one x title for the row.
// Run it with:
//   root -l -b -q 'nuclearModification.C("<path to ExampleData>")'
void nuclearModification(const char* dataDir = "ExampleData", const char* output = "nuclearModification.pdf")
{
  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);
  gStyle->SetEndErrorSize(0);

  // read the inputs: the pp spectrum, the Pb-Pb spectra per centrality class, the published table
  TFile ppFile(Form("%s/spectra.root", dataDir));
  TFile pbpbFile(Form("%s/heavyion.root", dataDir));
  auto pp = ppFile.Get<TH1>("data/ptSpec");
  if (!pp) {
    Error("nuclearModification", "data/ptSpec not found in %s/spectra.root", dataDir);
    return;
  }
  const std::vector<std::pair<std::string, double>> centralities = {{"0-10", 1500.0}, {"10-30", 750.0}, {"30-50", 260.0}};  // <N_coll>
  const char* colors[3] = {"#e0453a", "#f28e2b", "#3a78c2"};
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
    statistical.AddPointError(pt, raa, 0.0, stat);
  }

  // three pads of equal frame width that share the y axis: only the first one has room for its labels
  TCanvas canvas("canvas", "nuclearModification");
  canvas.SetCanvasSize(2064, 788);
  const double edges[4] = {0.0, 756.0, 1394.0, 2064.0};
  std::vector<TObject*> keep;  // pads, histograms, lines, texts and legends must live until the canvas is saved
  for (size_t i = 0; i < centralities.size(); ++i) {
    canvas.cd();
    auto pad = new TPad(Form("pad%zu", i), "", edges[i] / 2064.0, 0.0, edges[i + 1] / 2064.0, 1.0);
    keep.push_back(pad);
    pad->SetMargin(i == 0 ? 118.0 / 756.0 : 0.0, i == 2 ? 32.0 / 670.0 : 0.0, 0.15, 0.04);
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
    ratio->Scale(1.0 / ncoll);
    ratio->Divide(pp);
    const double width = (edges[i + 1] - edges[i]) / 2064.0;  // pixel sizes per pad: same look in all three
    for (auto axis : {ratio->GetXaxis(), ratio->GetYaxis()}) {
      axis->SetLabelFont(43);
      axis->SetLabelSize(i == 0 || axis == ratio->GetXaxis() ? 31 : 0);
      axis->SetTitleFont(43);
      axis->SetTitleSize(39);
    }
    ratio->GetXaxis()->SetRangeUser(0.15, 10.0);
    ratio->GetXaxis()->SetTitle(i == 2 ? "#it{p}_{T} (GeV/#it{c})" : "");  // one x title, at the end of the row
    ratio->GetXaxis()->SetTitleOffset(1.0);
    ratio->GetXaxis()->SetNoExponent();
    if (i < 2) ratio->GetXaxis()->ChangeLabel(-1, -1, 0);  // the label at the edge, cut by the next pad
    ratio->GetXaxis()->SetTickLength(0.03 / width * 0.38);
    ratio->GetYaxis()->SetTitle(i == 0 ? "#it{R}_{AA}" : "");
    ratio->GetYaxis()->CenterTitle();
    ratio->GetYaxis()->SetTitleOffset(1.29);
    ratio->GetYaxis()->SetTickLength(0.03 * 0.38 / width);
    ratio->SetMinimum(0.0);
    ratio->SetMaximum(1.4);
    ratio->SetMarkerStyle(kFullCircle);
    ratio->SetMarkerSize(1.4);
    ratio->SetMarkerColor(TColor::GetColor(colors[i]));
    ratio->SetLineColor(TColor::GetColor(colors[i]));
    ratio->SetLineWidth(5);
    ratio->Draw("AXIS");

    auto one = new TF1(Form("one%zu", i), "1", 0.15, 10.0);
    keep.push_back(one);
    one->SetLineColor(kGray + 1);
    one->SetLineStyle(kDashed);
    one->SetLineWidth(3);
    one->Draw("SAME");
    ratio->DrawCopy("E X0 SAME")->GetXaxis()->SetRangeUser(0.15, 6.0);  // where the spectra have the statistics
    const double textX[3] = {0.568, 0.489, 0.466};
    auto text = new TLatex(textX[i], 0.877, (cent + "%").data());
    keep.push_back(text);
    text->SetNDC();
    text->SetTextAlign(12);
    text->SetTextFont(43);
    text->SetTextSize(31);
    text->Draw();

    if (i == 0) {
      const int orange = TColor::GetColor("#f28e2b");
      syst.SetFillColor(TColor::GetColor("#fbd5a5"));
      syst.SetLineColor(orange);
      syst.Draw("2");
      statistical.SetMarkerStyle(kOpenCircle);
      statistical.SetMarkerSize(1.2);
      statistical.SetMarkerColor(orange);
      statistical.SetLineColor(orange);
      statistical.SetLineWidth(5);
      statistical.Draw("P Z");
      auto published = new TLegend(0.198, 0.677, 0.527, 0.781);
      published->SetMargin(0.17);
      keep.push_back(published);
      published->AddEntry(&syst, "published, syst.", "f");
      published->AddEntry(&statistical, "published, stat.", "pe");
      auto measured = new TLegend(0.198, 0.252, 0.446, 0.304);
      measured->SetMargin(0.23);
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
      auto box = new TH1D("normBox", "", 1, 0.0, 1.0);  // a legend row needs an object with the fill
      keep.push_back(box);
      box->SetFillColor(kGray);
      auto note = new TLegend(0.105, 0.252, 0.456, 0.304);
      note->SetMargin(0.18);
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
