import numpy as np
import pandas as pd
from SciRooPlot import *  # pyright: ignore[reportWildcardImportFromLibrary]

# The plots of the web page "Coming from numpy and pandas", rendered by build-site.sh.
# The page shows the blocks between the "# ---- <id>" markers (build.py cuts them out), so what it shows is what made the plots.

# ---- data
# Toy data standing in for yours: a DataFrame of events with two features, a class label, a train/test split,
# a classifier score (with scikit-learn: df["score"] = model.predict_proba(X)[:, 1]) and an event weight.
rng = np.random.default_rng(1)
n = 20000
df = pd.DataFrame({"label": rng.integers(0, 2, n), "split": rng.choice(["train", "test"], n)})
df["pt"] = rng.exponential(np.where(df.label == 1, 3.0, 2.0))
df["mass"] = rng.normal(np.where(df.label == 1, 125.0, 120.0), 8.0) + 0.5 * df.pt
df["score"] = np.where(df.label == 1, rng.beta(5, 2, n), rng.beta(2, 5, n))
df["weight"] = rng.uniform(0.5, 1.5, n)

# the ROC curve; with scikit-learn: fpr, tpr, _ = roc_curve(df.label, df.score)
isSig = (df.label.to_numpy() == 1)[np.argsort(-df.score.to_numpy())]
tpr = np.cumsum(isSig) / isSig.sum()
fpr = np.cumsum(~isSig) / (~isSig).sum()

# a measurement with errors and an exponential fitted to it (np.polyfit on the logarithm; scipy's curve_fit works the same way)
x = np.linspace(0.5, 9.5, 10)
y = 50.0 * np.exp(-x / 2.5) * rng.normal(1.0, 0.05, x.size)
slope, offset = np.polyfit(x, np.log(y), 1)
model = f"{np.exp(offset):.4g}*exp({slope:.4g}*x)"  # the fit result as a formula

# training history, as Keras or PyTorch Lightning log it
epochs = np.arange(1, 51)
trainLoss = 0.25 + 0.6 * np.exp(-epochs / 8.0)
validationLoss = trainLoss + 0.0015 * epochs
# ----


def DefineDataSources(pm: PlotManager):
    # ---- sources
    pm.AddDataSource("ml", [
        histo("sig", df.score[df.label == 1], bins=25, range=(0.0, 1.0)),  # like np.histogram
        histo("bkg", df.score[df.label == 0], bins=25, range=(0.0, 1.0)),
        histo2d("features", df.pt, df.mass, bins=(40, 40), range=((0.0, 15.0), (90.0, 160.0))),  # like np.histogram2d
        graph("roc", fpr[::100], tpr[::100]),  # points (x, y), like plt.plot
        graph("measurement", x, y, yerr=0.05 * y),  # with errors, like plt.errorbar
        graph("trainLoss", epochs, trainLoss),
        graph("validationLoss", epochs, validationLoss),
        table("events", df),  # the DataFrame itself, for selections when plotting
    ])
    # weighted histograms: the "data" and a simulation reweighted to it, with variable bins
    edges = [0.0, 0.5, 1.0, 1.5, 2.0, 3.0, 4.0, 6.0, 8.0, 12.0, 20.0]
    pm.AddDataSource("reweighting", [
        histo("data", df.pt[df.split == "test"], bins=edges),
        histo("simulation", df.pt[df.split == "train"], bins=edges, weights=df.weight[df.split == "train"]),
    ])
    # ----


def DefinePlots(pm: PlotManager):
    # ---- scores
    plot = Plot("scores", "ml", "1d")
    plot[1].AddData("sig", "ml", "signal").Density().SetColor(kRed + 1)
    plot[1].AddData("bkg", "ml", "background").Density().SetColor(kAzure + 2)
    plot[1].AddLegend()
    plot[1]['X'].SetTitle("classifier score")
    plot[1]['Y'].SetTitle("probability density")
    pm.AddPlot(plot)
    # ---- roc
    plot = Plot("roc", "ml", "1d")
    plot[1].AddData("roc", "ml", "classifier").SetLine(kSolid, 3.0, kRed + 1)
    plot[1].AddPoints([0.0, 1.0], [0.0, 1.0], "random guess").SetDrawOption(line).SetLine(kDashed, 2.0, kGray + 1)
    plot[1].AddLegend(bottom_right)
    plot[1]['X'].SetTitle("false positive rate").SetRange(0.0, 1.0)
    plot[1]['Y'].SetTitle("true positive rate").SetRange(0.0, 1.05)
    pm.AddPlot(plot)
    # ---- overtraining
    plot = Plot("overtraining", "ml", "1d")
    for label, name, color in [(1, "signal", kRed + 1), (0, "background", kAzure + 2)]:
        for split in ["train", "test"]:
            data = plot[1].AddData("events", "ml", f"{name}, {split}").Project1D(("score", 25, [0.0, 1.0]))
            data.Filter(f'label == {label} && split == "{split}"').Density().SetColor(color)
            if split == "train":
                data.SetDrawOption(hist).SetFill(1001, color).SetFillAlpha(0.25)
    plot[1].AddLegend()
    plot[1]['X'].SetTitle("classifier score")
    plot[1]['Y'].SetTitle("probability density").SetRoomAboveMax(0.4)  # free space for the legend
    pm.AddPlot(plot)
    # ---- features
    plot = Plot("features", "ml", "2d")
    plot[1].AddData("features", "ml")  # drawn as a colour map with a colour scale (colz)
    plot[1]['X'].SetTitle("#it{p}_{T} (GeV/#it{c})")
    plot[1]['Y'].SetTitle("mass (GeV/#it{c}^{2})")
    plot[1]['Z'].SetTitle("events").SetLog()
    pm.AddPlot(plot)
    # ---- ptBins
    plot = Plot("ptBins", "ml", "1d_2x2")
    ranges = [(0.0, 1.0), (1.0, 2.0), (2.0, 4.0), (4.0, 20.0)]
    classes = [(1, "signal", kRed + 1), (0, "background", kAzure + 2)]
    for pad, (low, high) in enumerate(ranges, start=1):
        for label, name, color in classes:
            data = plot[pad].AddData("events", "ml", name).Project1D(("score", 20, [0.0, 1.0]))
            data.Filter(f"label == {label} && pt > {low} && pt < {high}").Density().SetColor(color)
        plot[pad].AddText([f"{low:g} < #it{{p}}_{{T}} < {high:g} GeV/#it{{c}}"])
    plot[1].AddLegend()
    plot[0]['X'].SetTitle("classifier score")
    plot[0]['Y'].SetTitle("probability density").SetRoomAboveMax(0.35)
    pm.AddPlot(plot)
    # ---- reweighted
    plot = Plot("reweighted", "ml", "1d_ratio")
    plot[1].AddData("data", "reweighting", "data").Density().SetColor(kBlack)
    plot[1].AddData("simulation", "reweighting", "simulation, weighted").Density().SetDrawOption(hist).SetColor(kAzure + 2)
    plot[1].AddLegend()
    plot[1]['Y'].SetTitle("probability density").SetLog()
    plot[2].AddRatio(["data", "reweighting"], ["simulation", "reweighting"]).Both().Density().SetColor(kBlack)
    plot[2]['Y'].SetTitle("data / sim.").SetRange(0.7, 1.3)
    plot[0]['X'].SetTitle("#it{p}_{T} (GeV/#it{c})")
    pm.AddPlot(plot)
    # ---- fit
    plot = Plot("fit", "ml", "1d_ratio")
    plot[1].AddData("measurement", "ml", "measurement").SetColor(kBlack)
    plot[1].AddFunction(model, "exponential fit").SetLine(kSolid, 2.0, kRed + 1)
    plot[1].AddLegend()
    plot[1]['Y'].SetTitle("counts").SetLog().SetRoomAboveMax(0.1)
    plot[2].AddRatio(["measurement", "ml"], [model, "USER_FUNCTIONS"]).SetColor(kBlack)  # residuals: data / fit
    plot[2]['Y'].SetTitle("data / fit").SetRange(0.85, 1.15)
    plot[0]['X'].SetTitle("time (s)").SetRange(0.0, 10.0)
    pm.AddPlot(plot)
    # ---- learning
    plot = Plot("learning", "ml", "1d")
    plot[1].AddData("trainLoss", "ml", "training").SetLine(kSolid, 3.0, kAzure + 2)
    plot[1].AddData("validationLoss", "ml", "validation").SetLine(kSolid, 3.0, kOrange + 7)
    plot[1].AddLegend()
    plot[1]['X'].SetTitle("epoch")
    plot[1]['Y'].SetTitle("loss")
    pm.AddPlot(plot)
    # ----


def main():
    pm = PlotManager("numpy")
    for name in ["1d", "1d_ratio", "1d_2x2", "2d"]:
        pm.AddBasePlot(PlotManager.MakeBasePlot(name))
    DefineDataSources(pm)
    DefinePlots(pm)
    pm.SaveProject()


if __name__ == "__main__":
    main()
