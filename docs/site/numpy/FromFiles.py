import os
import runpy

import pandas as pd
from SciRooPlot import *  # pyright: ignore[reportWildcardImportFromLibrary]

# Part 2 of the web page "Coming from numpy and pandas": the results of DefinePlots.py written to files, and plots that read them.
# build-site.sh runs it as a project and draws its plots, so the blocks the page shows ("# ---- store", "# ---- files") are tested.
# The folder for the files comes from NUMPY_RESULTS (build-site.sh sets it); on the page it is just `results`.
results = os.environ["NUMPY_RESULTS"] + "/"
data = runpy.run_path(os.path.join(os.path.dirname(os.path.abspath(__file__)), "DefinePlots.py"), run_name="data")
df, fpr, tpr, epochs, trainLoss, validationLoss = (data[k] for k in ("df", "fpr", "tpr", "epochs", "trainLoss", "validationLoss"))

# ---- store
df.to_csv(results + "events.csv", index=False)
pd.DataFrame({"fpr": fpr, "tpr": tpr}).to_csv(results + "roc.csv", index=False)
pd.DataFrame({"epoch": epochs, "train": trainLoss, "validation": validationLoss}).to_csv(results + "history.csv", index=False)
# ----


def main():
    pm = PlotManager("numpyfiles")
    pm.AddBasePlot(PlotManager.MakeBasePlot("1d"))
    # ---- files
    pm.AddDataSource("ml", results)  # a folder: events.csv, roc.csv and history.csv are the tables events, roc and history

    plot = Plot("overtraining", "files", "1d")
    for label, name, color in [(1, "signal", kRed + 1), (0, "background", kAzure + 2)]:
        for split in ["train", "test"]:
            data = plot[1].AddData("events", "ml", f"{name}, {split}").Project1D(("score", 25, [0.0, 1.0]))
            data.Filter(f'label == {label} && split == "{split}"').Density().SetColor(color)  # unchanged
            if split == "train":
                data.SetDrawOption(hist).SetFill(1001, color).SetFillAlpha(0.25)
    plot[1].AddLegend()
    plot[1]['Y'].SetRoomAboveMax(0.4)
    pm.AddPlot(plot)

    plot = Plot("roc", "files", "1d")
    plot[1].AddData("roc", "ml", "classifier").Scatter("fpr", "tpr").SetLine(kSolid, 3.0, kRed + 1)  # a curve from two columns
    pm.AddPlot(plot)

    plot = Plot("learning", "files", "1d")
    plot[1].AddData("history", "ml", "training").Scatter("epoch", "train").SetLine(kSolid, 3.0, kAzure + 2)
    plot[1].AddData("history", "ml", "validation").Scatter("epoch", "validation").SetLine(kSolid, 3.0, kOrange + 7)
    plot[1].AddLegend()
    pm.AddPlot(plot)
    # ----
    pm.SaveProject()


if __name__ == "__main__":
    main()
