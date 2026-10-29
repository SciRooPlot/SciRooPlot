// The plot "etaPhiDataVsSim" of the SciRooPlot example (detector/tracking), written as a plain ROOT macro
// (no SciRooPlot needed): two colour maps side by side, sharing their axes and one colour scale.
// Run it with:
//   root -l -b -q 'etaPhiDataVsSim.C("<path to ExampleData>")'
void etaPhiDataVsSim(const char* dataDir = "ExampleData", const char* output = "etaPhiDataVsSim.pdf")
{
  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);
  gStyle->SetPalette(kViridis);

  // read the inputs: the TList "output" of the directory "TrackQA" in the data and simulation files
  auto load = [&](const char* fileName) -> TH2* {
    TFile file(Form("%s/%s", dataDir, fileName));
    auto list = file.Get<TList>("TrackQA/output");
    auto hist = list ? static_cast<TH2*>(list->FindObject("hEtaPhi")) : nullptr;
    if (!hist) {
      Error("etaPhiDataVsSim", "TrackQA/output/hEtaPhi not found in %s", fileName);
      return nullptr;
    }
    hist->SetDirectory(nullptr);
    return hist;
  };
  TH2* maps[2] = {load("AnalysisResults_data.root"), load("AnalysisResults_mc.root")};
  if (!maps[0] || !maps[1]) return;

  // two touching pads: the left one with room for the y labels, the right one with room for the common colour scale;
  // both maps get the same z range, and the labels and titles of the inner axes are left out
  const double zMin = std::min(maps[0]->GetMinimum(), maps[1]->GetMinimum());
  const double zMax = std::max(maps[0]->GetMaximum(), maps[1]->GetMaximum());
  TCanvas canvas("canvas", "etaPhiDataVsSim");
  canvas.SetCanvasSize(1662, 788);
  const char* labels[2] = {"data", "simulation"};
  const double edges[3] = {0.0, 756.0 / 1662, 1.0};  // 118 + 638 pixels, then 638 + 268
  std::vector<TObject*> keep;  // pads and texts must live until the canvas is saved
  for (int i = 0; i < 2; ++i) {
    canvas.cd();
    auto pad = new TPad(Form("pad%d", i), "", edges[i], 0.0, edges[i + 1], 1.0);
    keep.push_back(pad);
    const double width = (edges[i + 1] - edges[i]) * 1662;
    pad->SetMargin(i == 0 ? 118 / width : 0.0, i == 0 ? 0.0 : 268 / width, 0.15, 0.04);
    pad->SetTicks(1, 1);
    pad->Draw();
    pad->cd();
    auto hist = maps[i];
    for (auto axis : {hist->GetXaxis(), hist->GetYaxis(), hist->GetZaxis()}) {
      axis->SetLabelFont(43);
      axis->SetLabelSize(31);
      axis->SetTitleFont(43);
      axis->SetTitleSize(39);
    }
    hist->SetMinimum(zMin);
    hist->SetMaximum(zMax);
    hist->GetXaxis()->SetTitleOffset(1.2);
    hist->GetYaxis()->SetTitleOffset(1.1);
    hist->GetZaxis()->SetTitleOffset(2.0);
    if (i == 0) hist->GetXaxis()->SetTitle("");  // one x title, at the end of both axes
    if (i == 1) hist->GetYaxis()->SetLabelSize(0);
    if (i == 1) hist->GetYaxis()->SetTitle("");
    hist->Draw(i == 0 ? "COL" : "COLZ");
    auto text = new TLatex(0.15, 0.88, labels[i]);
    keep.push_back(text);
    text->SetNDC();
    text->SetTextFont(43);
    text->SetTextSize(31);
    text->Draw();
  }
  canvas.SaveAs(output);
}
