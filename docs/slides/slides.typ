#import "@preview/polylux:0.4.0": *

#let lang = sys.inputs.lang

#let code-block(cpp, py) = {
  block(
    fill: rgb("f5f5f5"),
    stroke: 1pt + rgb("888888"),
    radius: 8pt,
    inset: 12pt,
    width: 100%,
  )[
    #(if lang == "python" { py } else { cpp })
  ]
}
#let columns(left, right) = {
  grid(
    columns: (40%, 60%),
    [#left], [#right],
  )
}
#let code-slide(title, desc, cpp, py) = {
  slide[
    #text(fill: blue, size: 30pt)[
      == #title
    ]
    #columns(
      desc,
      code-block(
        cpp,
        py,
      ),
    )
  ]
}

#set page(
  width: 16in,
  height: 9in,
  margin: 0.5in,
)
#set text(
  font: "Arial",
  size: 18pt,
)

#slide[
  #place(
    center,
  )[
    #text(fill: blue, size: 50pt)[
      = Getting Started with
    ]
  ]
  #place(
    center + horizon,
    image("logo.png", width: 15cm),
  )
  #place(
    center + bottom,
    link("https://github.com/SciRooPlot/SciRooPlot")[*GitHub*],
  )
]

#code-slide(
  "Test Slide",
  [
    *Workflow*
    - central entity `PlotManager` stores plots and input data paths
    - save settings into a project that is accessible via plot command
  ],
  [
    ```cpp
    int main(int argc, char* argv[])
    {
      PlotManager plotManager;
      plotManager.AddInputDataFiles("input", {"/path/to/file1.root"});
      plotManager.AddPlotTemplate(PlotManager::GetPlotTemplate("1d"));
      { // --------------------------------------------------------------
        Plot plot("somePlot", "someGroup", "1d");
        plot[1].AddData("someData1", "input");
        plot[1]['X'].SetRange(10.5, 20.1);
        plot[1]['Y'].SetLog();
        plotManager.AddPlot(plot);
      } // --------------------------------------------------------------
      plotManager.SaveProject("myPlots");
      return 0;
    }
    ```
  ],
  [
    ```python
    def main():
        plotManager = PlotManager()
        plotManager.AddInputDataFiles("input", ["/path/to/file1.root"])
        plotManager.AddPlotTemplate(PlotManager.GetPlotTemplate("1d"))
        # ---------------------------------------------------------------
        plot = Plot("somePlot", "someGroup", "1d")
        plot[1].AddData("someData1", "input")
        plot[1]["X"].SetRange(10.5, 20.1)
        plot[1]["Y"].SetLog()
        plotManager.AddPlot(plot)
        # ---------------------------------------------------------------
        plotManager.SaveProject("myPlots")

    if __name__ == "__main__":
        main()
    ```
  ],
)
