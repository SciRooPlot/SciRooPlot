"""Plots of the legend symbol test: every data type with every drawing option alias (and the common ROOT options) and a set of styles,
each with a legend entry. Data: the file in $LEGEND_TEST_DATA (MakeData.py)."""

import os

from SciRooPlot import *

H1 = ["default", "points", "points_xerr", "points_endcaps", "points_xerr_endcaps", "points_line", "points_arrows", "line", "curve", "curve_clipped",
      "hist", "hist_open", "hist_clipped", "hist_open_clipped", "bars", "hbars", "area", "area_smooth", "band", "band_smooth", "boxes", "boxes_nomarkers", "brackets"]
RAW_HIST = ["E", "E0", "E1", "E3", "E4", "HIST", "L", "C", "P", "B", "BAR", "LF2", "HIST F", "HIST P", "E P", "E2 P", "*H", "HIST LP", "HIST E", "HIST E1", "HIST E2", "HIST E3"]
RAW_GRAPH = ["P", "L", "C", "B", "F", "LP", "CP", "PZ", "2", "3", "4", "5", "AP", "LF", "E", "E1", "||", "[]", "0", "X P", "*"]
FUNC = ["default", "points", "line", "curve", "area", "area_smooth"]
H2 = ["default", "col", "colz", "box", "lego", "surf", "cont", "cont_col", "cont_fill", "candle", "violin"]
# marker red, line blue and fill green where they differ, so the colour shows which attribute the legend took
STYLES = {
    "color": lambda d: d.SetColor(kRed + 1),
    "fill": lambda d: d.SetColor(kRed + 1).SetFill(1001, kRed + 1, 0.4),
    "lw0": lambda d: d.SetColor(kRed + 1).SetLineWidth(0),
    "dashed": lambda d: d.SetColor(kRed + 1).SetLine(kDashed, 3, kRed + 1),
    "marker": lambda d: d.SetColor(kRed + 1).SetMarker(25, 2, kRed + 1),
    "hatch": lambda d: d.SetColor(kRed + 1).SetFill(3354, kRed + 1),
    "split": lambda d: d.SetMarkerColor(kRed + 1).SetLineColor(kBlue + 1).SetFill(1001, kGreen + 2),
    "split_lw0": lambda d: d.SetMarkerColor(kRed + 1).SetLineColor(kBlue + 1).SetFill(1001, kGreen + 2).SetLineWidth(0),
    "hollow_split": lambda d: d.SetMarkerColor(kRed + 1).SetLineColor(kBlue + 1).SetFillColor(kGreen + 2).SetFillStyle(0),
    "split_dashed_ms0": lambda d: d.SetMarkerColor(kRed + 1).SetLine(kDashed, 3, kBlue + 1).SetFill(3354, kGreen + 2).SetMarkerSize(0),
    "solid_lw1": lambda d: d.SetColor(kRed + 1).SetFill(1001, kGreen + 2).SetLineWidth(1).SetMarker(20, 1, kRed + 1),
}


def AddPlot(pm, kind, option, style, addData):
    tag = option.replace(" ", "_").replace("*", "star").replace("|", "bar").replace("[", "lb").replace("]", "rb")
    plot = Plot(f"{kind}__{tag}__{style}", "legend", "1d")
    data = addData(plot[1])
    if option in globals():
        data.SetDrawOption(globals()[option])
    elif option != "default":
        data.SetDrawOption(option)  # a ROOT option
    STYLES[style](data)
    plot[1].AddLegend(0.6, 0.9)
    pm.AddPlot(plot)


def main():
    pm = PlotManager("legendSymbols")
    pm.AddDataSource("d", os.environ["LEGEND_TEST_DATA"])
    pm.AddBasePlot(PlotManager.MakeBasePlot("1d"))
    for style in STYLES:
        for kind in ("h_err", "h_noerr", "g", "ge", "ga"):
            for option in H1 + (RAW_HIST if kind.startswith("h") else RAW_GRAPH):
                AddPlot(pm, kind, option, style, lambda pad, k=kind: pad.AddData(k, "d", "entry"))
        for option in FUNC:
            AddPlot(pm, "f", option, style, lambda pad: pad.AddFunction("3+2*sin(x/2)", "entry"))
        for option in H2:
            AddPlot(pm, "h2", option, style, lambda pad: pad.AddData("h2", "d", "entry"))
    pm.SaveProject()


main()
