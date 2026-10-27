from SciRooPlot import *  # pyright: ignore[reportWildcardImportFromLibrary]
import random

# Plots shown on the web page: the steps of the tour and one small plot per drawing option alias.
# Rendered by build-site.sh with `plot '.+' '.+' png`.

EXAMPLE = "${SCIROOPLOT_DATA_DIR}/ExampleData"

ALIASES_1D = ["points", "points_xerr", "points_endcaps", "points_xerr_endcaps", "points_line", "points_text", "points_arrows",
              "line", "curve", "curve_clipped", "hist", "hist_open", "hist_clipped", "hist_open_clipped", "bars", "hbars",
              "area", "area_smooth", "band", "band_smooth", "boxes", "boxes_nomarkers", "brackets", "text"]
ALIASES_2D = ["col", "colz", "box", "text", "lego", "lego_col", "lego_colz", "lego_noborders", "surf", "surf_col", "surf_colz",
              "surf_fill", "surf_fillz", "surf_contours", "surf_shaded", "cont", "cont_col", "cont_colz", "cont_fill", "cont_fillz",
              "cont_smooth", "cont_smoothz", "candle", "candle_minmax", "candle_mean", "candle_notched", "candle_points",
              "candle_scatter", "candle_meanline", "violin", "violin_minmax"]


def DefineTour(pm: PlotManager):
    pm.AddDataSource("data", EXAMPLE + "/spectra.root:data")
    pm.AddDataSource("mc", EXAMPLE + "/spectra.root:mc")

    plot = Plot("function", "tour", "1d")
    plot[1].AddFunction("sin(x)/x", "sin(#it{x})/#it{x}")
    plot[1].AddLegend()
    plot[1]['X'].SetTitle("#it{x}").SetRange(0.1, 20.0)
    plot[1]['Y'].SetRange(-0.3, 1.1)
    pm.AddPlot(plot)

    plot = Plot("data", "tour", "1d")
    plot[1].AddData("ptSpec", "data", "data")
    pm.AddPlot(plot)

    plot = Plot("style", "tour", "1d")
    plot[1].AddData("ptSpec", "data", "data").SetMarker(kBlack, kFullCircle, 1.0)
    plot[1].AddData("ptSpec", "mc", "simulation").SetDrawOption(hist_open).SetLine(kMagenta + 2, kDotted, 3.0)
    pm.AddPlot(plot)

    plot = Plot("axes", "tour", "1d")
    plot[1].AddData("ptSpec", "data", "data").SetMarker(kBlack, kFullCircle, 1.0)
    plot[1].AddData("ptSpec", "mc", "simulation").SetDrawOption(hist_open).SetLine(kMagenta + 2, kDotted, 3.0)
    plot[1]['X'].SetLog().SetTitle("#it{p}_{T} (GeV/#it{c})")
    plot[1]['Y'].SetLog()
    pm.AddPlot(plot)

    plot = Plot("legend", "tour", "1d")
    plot[1].AddData("ptSpec", "data", "data").SetMarker(kBlack, kFullCircle, 1.0)
    plot[1].AddData("ptSpec", "mc", "simulation").SetDrawOption(hist_open).SetLine(kMagenta + 2, kDotted, 3.0)
    plot[1]['X'].SetLog().SetTitle("#it{p}_{T} (GeV/#it{c})")
    plot[1]['Y'].SetLog()
    plot[1].AddLegend()
    plot[1].AddText(["My Experiment", "pp, #sqrt{#it{s}} = 13.6 TeV"])
    pm.AddPlot(plot)

    plot = Plot("ratio", "tour", "1d_ratio")
    plot[1].AddData("ptSpec", "data", "data").SetMarker(kBlack, kFullCircle, 1.0)
    plot[1].AddData("ptSpec", "mc", "simulation").SetDrawOption(hist_open).SetLine(kMagenta + 2, kDotted, 3.0)
    plot[1]['Y'].SetLog()
    plot[1].AddLegend()
    plot[1].AddText(["My Experiment", "pp, #sqrt{#it{s}} = 13.6 TeV"])
    plot[2].AddRatio(["ptSpec", "data"], ["ptSpec", "mc"])
    plot[2]['Y'].SetTitle("data / sim.").SetRange(0.7, 1.3)
    plot[0]['X'].SetLog().SetRange(0.15, 15.0).SetTitle("#it{p}_{T} (GeV/#it{c})")
    pm.AddPlot(plot)


def DefineGallery(pm: PlotManager):
    rng = random.Random(4)
    pm.AddDataSource("hist", histo("h", [rng.gauss(0.0, 1.0) for _ in range(600)], 14, (-3.5, 3.5)))
    x = [0.5 + i for i in range(8)]
    pm.AddDataSource("graph", graph("g", x, [2.0 + 6.0 * (xi / 8.0) ** 1.5 + rng.uniform(-0.4, 0.4) for xi in x],
                                    [0.5] * 8, [0.3 + 0.1 * xi for xi in x]))
    for alias in ALIASES_1D:
        for kind in ["hist", "graph"]:
            plot = Plot(alias, "gallery/" + kind, "1d")
            name = "h" if kind == "hist" else "g"
            plot[1].AddData(name, kind).SetDrawOption(globals()[alias]).SetColor(kAzure + 2).SetFill(kAzure + 2, 1001).SetFillAlpha(0.35)
            plot[1]['X'].SetTitle("")
            plot[1]['Y'].SetTitle("")
            if alias.startswith(("curve", "hist")):  # bins beyond the frame show what the clipped variants change
                if kind == "hist":
                    plot[1]['Y'].SetLog().SetRange(4.0, 90.0)
                else:
                    plot[1]['Y'].SetRange(2.5, 6.0)
            pm.AddPlot(plot)
    for alias in ALIASES_2D:
        candle = alias.startswith(("candle", "violin"))
        plot = Plot(alias, "gallery/hist2d", "1d" if candle else "2d_logz")
        data = plot[1].AddData("ptVsMult", "data").SetDrawOption(globals()[alias])
        if candle:
            data.RebinX(10)
            plot[1]['Y'].SetRange(0.0, 3.0)
        else:
            data.RebinXY(10, 6) if alias in ("text", "box") else data.RebinXY(4, 3)
        if alias.startswith(("lego", "surf")):
            plot[1].SetView(35.0, 225.0)
        plot[1]['X'].SetTitle("")
        plot[1]['Y'].SetTitle("")
        pm.AddPlot(plot)


def main():
    pm = PlotManager("site")
    DefineTour(pm)
    DefineGallery(pm)
    for name in ["1d", "1d_ratio", "2d"]:
        pm.AddBasePlot(PlotManager.MakeBasePlot(name))
    logz = PlotManager.MakeBasePlot("2d")
    logz.SetName("2d_logz")
    logz[1]['Z'].SetLog()
    pm.AddBasePlot(logz)
    pm.SaveProject()


if __name__ == "__main__":
    main()
