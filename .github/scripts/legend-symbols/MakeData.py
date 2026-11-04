"""Input data of the legend symbol test: 1d histograms with and without errors, graphs without, with symmetric and asymmetric errors, a 2d histogram."""

import math
import sys

import ROOT

out = ROOT.TFile(sys.argv[1], "RECREATE")
withErrors = ROOT.TH1D("h_err", "", 10, 0, 10)
withErrors.Sumw2()
noErrors = ROOT.TH1D("h_noerr", "", 10, 0, 10)
noErrors.Sumw2()  # all errors zero
graph, graphErrors, graphAsymm = ROOT.TGraph(), ROOT.TGraphErrors(), ROOT.TGraphAsymmErrors()
for i in range(10):
    y = 3 + 2 * math.sin((i + 1) / 2.0)
    withErrors.SetBinContent(i + 1, y)
    withErrors.SetBinError(i + 1, 0.4)
    noErrors.SetBinContent(i + 1, y)
    graph.SetPoint(i, i + 0.5, y)
    graphErrors.SetPoint(i, i + 0.5, y)
    graphErrors.SetPointError(i, 0.3, 0.4)
    graphAsymm.SetPoint(i, i + 0.5, y)
    graphAsymm.SetPointError(i, 0.2, 0.3, 0.3, 0.5)
hist2d = ROOT.TH2D("h2", "", 10, 0, 10, 10, 0, 10)
for i in range(1, 11):
    for j in range(1, 11):
        hist2d.SetBinContent(i, j, math.exp(-((i - 5) ** 2 + (j - 5) ** 2) / 10.0))
for obj, name in ((withErrors, "h_err"), (noErrors, "h_noerr"), (graph, "g"), (graphErrors, "ge"), (graphAsymm, "ga"), (hist2d, "h2")):
    obj.Write(name)
out.Close()
