from SciRooPlot import *
from pathlib import Path
SRC_DIR = str(Path(__file__).resolve().parent) + "/"

def DefineInputIdentifiers(pm: PlotManager):
  inputFolder = "${SCIROOPLOT_DATA_DIR}/TestFiles"
  pm.AddInput("measurements", [inputFolder + "/file1.root", inputFolder + "/file2.root"])
  pm.AddInput("theory", inputFolder + "/subfolder/file3.root")
  pm.AddInput("pseudodata", inputFolder + "/subfolder/file3.root:pseudodata")
  # pm.AddInput("test", SRC_DIR + "../rel/path/to/file.root")

def DefinePlotTemplates(pm: PlotManager):
  # -----------------------------------------------------------------------
  plot = PlotManager.GetPlotTemplate("1d")
  pm.AddPlotTemplate(plot)
  # -----------------------------------------------------------------------
  plot = PlotManager.GetPlotTemplate("1d_ratio")
  pm.AddPlotTemplate(plot)
  # -----------------------------------------------------------------------
  plot = PlotManager.GetPlotTemplate("2d")
  pm.AddPlotTemplate(plot)
  # -----------------------------------------------------------------------

def DefinePlots(pm: PlotManager):
  # -----------------------------------------------------------------------
  plot = Plot("plot1", "dummyFigures", "1d")
  plot[1].AddData("hist3", "measurements", "gaus")
  plot[1].AddData("hist1", "measurements", "anothergaus")
  plot[1].AddData("hist7", "theory", "prediction")
  plot[1].AddLegend()
  pm.AddPlot(plot)
  # -----------------------------------------------------------------------
  plot = Plot("plot2", "dummyFigures", "1d")
  plot[1].AddData("dir2/subdir2/hist3", "measurements")
  plot[1].AddData("list1/hist5", "measurements")
  pm.AddPlot(plot)
  # -----------------------------------------------------------------------
  plot = Plot("plot3", "dummyFigures", "1d_ratio")
  plot[1].AddData("dir2/subdir1/mygausrand", "measurements", "gaus random")
  plot[1].AddData("dir2/subdir1/mygaus", "measurements", "gaus function")
  plot[2].AddRatio("dir2/subdir1/mygausrand", "measurements", "dir2/subdir1/mygaus", "measurements")
  plot[2]['Y'].SetTitle("rand / func")
  plot[2]['X'].SetTitle("my X title")
  plot[1]['Y'].SetRange(0.04, 0.45)
  plot[0]['X'].SetRange(-2, 2)
  pm.AddPlot(plot)
  # -----------------------------------------------------------------------
  plot = Plot("invMassDist", "examples", "1d")
  plot[1].AddData("invMassDist", "pseudodata").Scale(0.00001).SetColor(kBlue + 2)
  pm.AddPlot(plot)
  # -----------------------------------------------------------------------
  plot = Plot("multDist", "examples", "1d")
  plot[1].AddData("multDist", "pseudodata")
  plot[1].AddText("My Experiment // pp collisions, X TeV")
  plot[1]['Y'].SetLog()
  pm.AddPlot(plot)
  # -----------------------------------------------------------------------
  plot = Plot("ptSpec", "examples", "1d")
  plot[1].AddData("ptSpec", "pseudodata").SetColor(kRed + 2)
  plot[1]['X'].SetLog()
  plot[1]['Y'].SetLog()
  pm.AddPlot(plot)
  # -----------------------------------------------------------------------
  
def main():
  pm = PlotManager()
  DefineInputIdentifiers(pm)
  DefinePlotTemplates(pm)
  DefinePlots(pm)
  pm.SaveProject("PROJECT_NAME")

if __name__ == "__main__":
  main()
