#import "@preview/polylux:0.4.0": *

#let slide-height = 9in
#set page(
  width: 16in,
  height: slide-height,
  margin: 0.5in,
)
#set text(
  font: "Arial",
  size: 18pt,
)
#let main-color = rgb("1C94A5")
#let accent-color = rgb("D4A63A")

#let lang = sys.inputs.lang

#let plot-def-file = {
  if lang == "py" {
    [`DefinePlots.py`]
  } else {
    [`DefinePlots.cpp`]
  }
}

#let feature(title, desc) = rect(
  radius: 8pt,
  fill: rgb("#f8fafc"),
  stroke: 1pt + rgb("#dbe2ea"),
  inset: 10pt,
)[
  *#title*

  #v(0.3em)

  #desc
]

#let slide-title(title) = {
  text(
    fill: main-color,
    size: 28pt,
    weight: "bold",
  )[ = #title ]
  line(
    length: 100%,
    stroke: 3pt + accent-color,
  )
  v(0.5em)
}

#let code-block(cpp, py) = {
  block(
    fill: rgb("#f8fafc"),
    stroke: 1pt + rgb("#d0d7de"),
    radius: 10pt,
    inset: 14pt,
    width: 98%,
  )[
    #(if lang == "py" { py } else { cpp })
  ]
}
#let columns(left, right) = {}

#let prompt = text(fill: main-color, weight: "bold")[\$]
#let terminal(body) = block(
  fill: rgb("fafafa"),
  stroke: 1pt + rgb("d0d7de"),
  radius: 10pt,
  inset: 15pt,
)[
  #set text(font: "DejaVu Sans Mono", size: 16pt)
  #body
]

#let card(title, color, body) = rect(
  fill: rgb("fafafa"),
  stroke: 1pt + rgb("d0d7de"),
  radius: 10pt,
  inset: 15pt,
)[
  #text(
    size: 24pt,
    weight: "bold",
    fill: color,
  )[#title]
  #linebreak()
  #body
]


#slide[
  #if lang == "py" {
    slide-title(
      "SciRooPlot User Manual                               Python Version",
    )
  } else {
    slide-title(
      "SciRooPlot User Manual                                    C++ Version",
    )
  }
  #place(
    dy: 1cm,
    center + horizon,
    image("logo.png", width: 22cm),
  )
  #place(
    bottom + right,
    terminal("Scientific Plotting Made Simple"),
  )
]

#slide[
  #slide-title("A ROOT-based Plot Organizer")

  #grid(
    columns: (65%, 5%, 30%),
    [
      #v(1em)
      *The ROOT plotting challenge*
      - ROOT is powerful, but everyday plotting often requires substantial boilerplate code.

      - Similar I/O and styling tasks are repeatedly implemented in different macros.
      - Often plotting logic becomes intertwined with analysis code, making iteration slow.
      - Producing figures requires handling files, data types, styling, and other ROOT details.

      #v(2em)
      *The SciRooPlot approach*
      - Introduces an abstraction layer on top of ROOT.

      - Provides a common plotting infrastructure and organization.
      - Lets users describe the desired figure rather than its implementation.
      - Preserves the familiar ROOT ecosystem and styling conventions.
      - Enables interactive, project-based plotting workflows.
    ],
    [],
    [
      #align(center)[
        #v(2em)
        #card(
          "ROOT",
          black,
          [
            ✓ Histograms
            #linebreak()
            ✓ Graphs
            #linebreak()
            ✓ Functions
            #linebreak()
            ✓ Files
          ],
        )
        #v(0.1em)
        #text(size: 28pt, fill: main-color)[+]
        #v(0.1em)
        #card(
          "SciRooPlot",
          black,
          [
            ✓ Abstract plot specifications
            #linebreak()
            ✓ Plot grouping & organization
            #linebreak()
            ✓ Interactive plotting
            #linebreak()
            ✓ Project-based workflows
          ],
        )
      ]
    ],
  )
]


#slide[
  #slide-title("SciRooPlot Workflow")
  - Store analysis results in ROOT or CSV files.

  - Define plot specifications using C++ or Python interface and save them as a SciRooPlot project.

  - Generate and explore plots interactively via the `plot` command-line tool.

  - Create and manage projects with the `srp` command-line tool.

  #slide-title("Installation")

  #terminal[
    #prompt git clone https://github.com/SciRooPlot/SciRooPlot.git

    #prompt cd SciRooPlot

    #prompt ./scripts/install.sh
  ]

  - The installer prints a command that enables the SciRooPlot environment.

  - Add it to the shell startup script (e.g. `~/.bashrc` or `~/.zshrc`) to load SciRooPlot automatically in new terminals.

  - After setup, the `srp` and `plot` command-line tools are available system-wide.

  - Future updates can be installed with: `$ srp update`
]


#slide[
  #slide-title("Your First Project")
  #let folder-struct = {
    if lang == "py" {
      terminal[
        ```text
        myProject/
        ├── DefinePlots.py
        └── output/
        ```
      ]
    } else {
      terminal[
        ```text
        myProject/
        ├── DefinePlots.cpp
        └── output/
        ```
      ]
    }
  }
  #grid(
    columns: (55%, 45%),
    gutter: 0%,
    [
      - Initialize a new SciRooPlot project:
        #if lang == "py" {
          terminal[
            #prompt srp init-py myProject [someDir]
          ]
        } else {
          terminal[
            #prompt srp init-cpp myProject [someDir]
          ]
        }

      - Creates a project directory (`./someDir` or `./myProject`).

      - Contains #plot-def-file, where plot specifications are defined.

        #folder-struct

      - Project automatically registered with SciRooPlot.

      - Includes working examples based on the bundled datasets.

      - Generate your first plot immediately:
        #terminal[
          #prompt plot examples ptSpec
        ]

      - Plots always stay in sync with #plot-def-file.
    ],
    [
      #card(
        "Project Management",
        main-color,
        [

          *Select another project*
          #v(-0.5em)
          #terminal[
            #prompt srp select myOtherProject
          ]

          *List all projects*
          #v(-0.5em)
          #terminal[
            #prompt srp projects
          ]

          *Change output directory* (default is `./output`)
          #v(-0.5em)
          #terminal[
            #prompt srp set myProject OUT /path/to/output
          ]
        ],
      )
      #v(-0.5em)
      #block(
        fill: rgb("fafafa"),
        stroke: 1pt + rgb("d0d7de"),
        radius: 10pt,
        inset: 10pt,
        width: 90%,
      )[
        *Additional commands:*
        #v(-0.5em)
        #table(
          columns: (50%, 50%),
          stroke: none,
          align: (left, horizon),
          [#prompt srp help], [List available options.],
          [#prompt srp show \<project\>], [Show project settings.],
          [#prompt srp remove \<project\>], [Unregister project.],
          [#prompt srp print \<file\>], [List root file contents.],
          [#prompt srp open \<file\>], [Open root file.],
        )
      ]
    ],
  )
]

#slide[
  #slide-title("Using the App")
  #grid(
    columns: (55%, 40%),
    gutter: 5%,
    [
      - Plots specified in #plot-def-file have a *figure group* and *plot name*.

      - These identifiers are used to select plots from the command line.
        #terminal[
          #prompt plot \<figureGroup\> \<plotName\> [\<mode\>]
        ]

      - Tab completion simplifies browsing through available names.
      - Both `<figureGroup>` and `<plotName>` support regular expressions.
      - Multiple plots can be generated with a single command.

      #block(
        fill: rgb("fafafa"),
        stroke: 1pt + rgb("d0d7de"),
        radius: 10pt,
        inset: 10pt,
      )[
        #table(
          columns: (17%, 83%),
          stroke: none,
          align: (left + horizon, left + horizon),
          table.header([*Mode*], [*Description*]),
          table.hline(stroke: 1pt + rgb("d0d7de")),
          [`show`], [Open plots interactively (default mode).],
          table.hline(stroke: 1pt + rgb("d0d7de")),
          [`find`], [List plots matching the request.],
          table.hline(stroke: 1pt + rgb("d0d7de")),
          [`pdf`, `eps`, `svg`, `png`], [Export plots as graphics files.],
          table.hline(stroke: 1pt + rgb("d0d7de")),
          [`macro`], [Generate a ROOT `.C` macros that reproduce the plots.],
          table.hline(stroke: 1pt + rgb("d0d7de")),
          [`gif`],
          [Combine matching plots into an animated GIF #linebreak() (`gif+N` sets the frame delay in units of 10 ms).],
          table.hline(stroke: 1pt + rgb("d0d7de")),
          [`file`], [Save all requested plots into a single ROOT file.],
          table.hline(stroke: 1pt + rgb("d0d7de")),
          [`data`], [Save input data for requested plots into a ROOT file.],
        )
      ]
    ],
    [
      #card(
        "Regular Expressions",
        main-color,
        [

          *Select plot names matching pattern*
          #v(-0.5em)
          #terminal[
            #prompt plot paperPlots \'moneyPlot[1,2]\'
          ]
          NB.: Quotes are only required if regex contains #linebreak() special shell characters like `[]`, `()` and `|`.


          *Select all plots in a figure group*
          #v(-0.5em)
          #terminal[
            #prompt plot paperPlots .+
          ]

          *Select plots in groups starting with pattern*
          #v(-0.5em)
          #terminal[
            #prompt plot pp\_.+ myPlot
          ]
        ],
      )
      #v(-0.5em)
      #block(
        fill: rgb("fafafa"),
        stroke: 1pt + rgb("d0d7de"),
        radius: 10pt,
        inset: 10pt,
        width: 84%,
      )[
        *Additional commands:*
        #v(-0.5em)
        #table(
          columns: (30%, 70%),
          stroke: none,
          align: (left, horizon),
          [#prompt plot cd], [Navigate to project.],
          [#prompt plot open], [Open code files.],
        )
      ]
    ],
  )
]

#slide[
  #slide-title("General Code Structure")
  #grid(
    columns: (42%, 58%),
    gutter: 0%,
    [
      - Plots are handled and exported by `PlotManager`.

      - Input files are registered under user-defined aliases.

      - Data objects within input files are identified by name.

      - Each Plot has a unique name within a figure group

      - It consists of one or more Pads (`plot[1]`, `plot[2]`, ..)

      - Pads contain settings of Axis, Data, Text and Legend.

        #terminal[
          ```
          PlotManager
          │
          ├── Plot
          │     ├── Pad
          │     │     ├── Axis
          │     │     ├── Data
          │     │     ├── Legend
          │     │     └── Text
          │     ├── Pad
          │     │
          │     └── ...
          ├── Plot
          │
          └── ...
          ```]
    ],
    [
      #text(fill: gray)[*#plot-def-file*]
      #v(-0.8em)
      #code-block(
        [
          ```cpp
          int main(int argc, char* argv[])
          {
            PlotManager pm;
            pm.AddDataset("datasetName", "/path/to/file.root");
            { // ---------------------------------------------------------
              Plot plot("plotName", "groupName");
              plot[1].AddData("dataObjectName", "datasetName", "my label");
              plot[1].AddLegend();
              plot[1].AddText("some text");
              plot[1]["X"].SetTitle("x axis title");
              pm.AddPlot(plot);
            } // ---------------------------------------------------------
            pm.SaveProject("myPlots");
            return 0;
          }
          ```
        ],
        [
          ```python
          def main():
            pm = PlotManager()
            pm.AddDataset("datasetName", "/path/to/file.root")
            # -----------------------------------------------------------
            plot = Plot("plotName", "groupName")
            plot[1].AddData("dataObjectName", "datasetName", "my label")
            plot[1].AddLegend()
            plot[1].AddText("some text")
            plot[1]['X'].SetTitle("x axis title")
            pm.AddPlot(plot)
            # -----------------------------------------------------------
            pm.SaveProject("myProject")

          if __name__ == "__main__":
            main()
          ```
        ],
      )
      #v(1em)
      #let link-to-other-doc = {
        [
          This documentation uses the
          #if lang == "py" {
            [Python interface. #linebreak() Find the #link("https://scirooplot.github.io/SciRooPlot/slides/SciRooPlot_UserManual_cpp.pdf")[#text(fill: main-color)[C++ version]] here.]
          } else {
            [C++ interface. #linebreak() Find the #link("https://scirooplot.github.io/SciRooPlot/slides/SciRooPlot_UserManual_py.pdf")[#text(fill: main-color)[Python version]] here.]
          }
        ]
      }
      #card(
        "Language Note",
        main-color,
        [
          #link-to-other-doc
        ],
      )

    ],
  )
]



#slide[
  #slide-title("Registering Datasets")
  #grid(
    columns: (42%, 55%),
    gutter: 3%,
    [
      - Input files are registered with the `PlotManager` under a user-defined alias that is later used to access the data.

      - File paths are always absolute, but the `SRC_DIR` helper enables paths relative to #plot-def-file.

      - Shell environment variables (including user-defined ones) are supported and expanded automatically.

      - Multiple files may be registered under the same alias using lists and/or successive `AddDataset()` calls.

      - Adding a directory automatically registers all ROOT files it contains, including those in subdirectories.

      - Instead of an entire ROOT file, also individual subdirectories or object lists can be registered.

      - Registered files are searched in alphabetical order. Within each file, the directory hierarchy is traversed until the first matching data object is found.

      - In addition to files, transient in-memory ROOT objects can also be registered directly as inputs.
    ],
    [
      #code-block(
        [
          ```cpp
          pm.AddDataset("datasetA", "/path/to/file1.root");

          pm.AddDataset("datasetB", SRC_DIR + "../rel/path/file2.root");

          pm.AddDataset("datasetC", "${HOME}/path/to/file/file3.root");

          pm.AddDataset("datasetD", {"/path/to/file4.root",
                                      "/path/to/file5.root"});

          pm.AddDataset("datasetE", "/path/to/directory/");

          pm.AddDataset("datasetF", "/path/to/file6.root:dir/or/list");
          ```
        ],
        [
          ```python
          pm.AddDataset("datasetA", "/path/to/file1.root")

          pm.AddDataset("datasetB", SRC_DIR + "../rel/path/file2.root")

          pm.AddDataset("datasetC", "${HOME}/path/to/file/file3.root")

          pm.AddDataset("datasetD", ["/path/to/file4.root",
                                      "/path/to/file5.root"])

          pm.AddDataset("datasetE", "/path/to/directory/")

          pm.AddDataset("datasetF", "/path/to/file6.root:dir/or/list")
          ```
        ],
      )
      #code-block(
        [
          ```cpp
          auto myHist = new TH1D("myHist", "", 100, -5, 5);
          pm.AddDataset("datasetG", myHist);

          auto myGraph = new TGraph();
          myGraph->SetName("myGraph"); // graph needs a name!
          auto myFunc = new TF1("myFunc", "gaus", -5, 5);
          pm.AddDataset("datasetG", {myGraph, myFunc});
          ```
        ],
        [
          ```python
          myHist = ROOT.TH1D("myHist", "", 100, -5, 5)
          pm.AddDataset("datasetG", myHist)

          myGraph = ROOT.TGraph()
          myGraph.SetName("myGraph")  # graph needs a name!
          myFunc = ROOT.TF1("myFunc", "gaus", -5, 5)
          pm.AddDataset("datasetG", [myGraph, myFunc])
          ```
        ],
      )

    ],
  )
]

#slide[
  #slide-title("Plot Appearence")
  #grid(
    columns: (45%, 50%),
    gutter: 5%,
    [
      - Defining plot templates avoids repeating common layout settings across many plots.

      - Templates are ordinary `Plot` objects registered with the `PlotManager` via `AddPlotTemplate()`.

      - Default settings applied to `plot[0]` automatically affect all pads unless overridden.

      - Several ready-to-use templates (e.g. `1d`, `2d`, `1d_ratio`) are provided with SciRooPlot.

      - Existing templates can be used directly or modified to create custom layouts.
      
      - Axis ranges, titles, scales, and other axis properties are configured intuitively through plot[pad]['X'], ['Y'], and ['Z'].
      
      - List of accessors of Plot, Pad and Axis in appendix.
    ],
    [
      #code-block(
        [
          ```cpp
          Plot plot("myLayout", "PLOT_TEMPLATES");
          plot.SetDimensions(710, 710);
          plot.SetTransparent();

          plot[0].SetMargins(0.07, 0.14, 0.12, 0.07);
          plot[0].SetDefaultMarkerSize(1.2).SetDefaultLineWidth(2.);
          plot[0].SetDefaultTextFont(43).SetDefaultTextSize(24);
          plot[0].SetDefaultColors({kBlack, kBlue, kRed});
          plot[0].SetDefaultMarkerStyles({kFullCircle});
          plot[0].SetDefaultLineStyles({kSolid, kDashed});

          plot[0]['X'].SetTitleSize(28).SetTitleOffset(1.1);
          plot[0]['Y'].SetTitleSize(28).SetTitleOffset(1.5);

          plot[1].SetPosition(0., 0., 1., 1.);

          pm.AddPlotTemplate(plot);
          ```
        ],
        [
          ```python
          plot = Plot("myLayout", "PLOT_TEMPLATES")
          plot.SetDimensions(710, 710)
          plot.SetTransparent()

          plot[0].SetMargins(0.07, 0.14, 0.12, 0.07)
          plot[0].SetDefaultMarkerSize(1.2).SetDefaultLineWidth(2.)
          plot[0].SetDefaultTextFont(43).SetDefaultTextSize(24)
          plot[0].SetDefaultColors([kBlack, kBlue, kRed])
          plot[0].SetDefaultMarkerStyles([kFullCircle])
          plot[0].SetDefaultLineStyles([kSolid, kDashed])

          plot[0]['X'].SetTitleSize(28).SetTitleOffset(1.1)
          plot[0]['Y'].SetTitleSize(28).SetTitleOffset(1.5)

          plot[1].SetPosition(0., 0., 1., 1.)

          pm.AddPlotTemplate(plot)
          ```
        ],
      )
      #code-block(
        [
          ```cpp
          pm.AddPlotTemplate(PlotManager::GetPlotTemplate("1d"));
          // create new plot on basis of 1d template:
          Plot plot("myPlot", "myGroup", "1d");
          plot[1]['X'].SetRange(0., 10.).SetTitle("x title");
          ```
        ],
        [
          ```python
          pm.AddPlotTemplate(PlotManager.GetPlotTemplate("1d"))
          # create a new plot based on the "1d" template
          plot = Plot("myPlot", "myGroup", "1d")
          plot[1]['X'].SetRange(0., 10.).SetTitle("x title")
          ```
        ],
      )
    ],
  )
]


#slide[
  #slide-title("Adding Data")
  #grid(
    columns: (45%, 50%),
    gutter: 5%,
    [
    - The data-type agnostic `AddData()` function accepts ROOT histograms, graphs, functions, trees, and CSV tables.
        
    - Data is identified by name and automatically searched for under the registered input alias across all associated files.
    
    - Directory or list paths within ROOT files can be prepended to object names to disambiguate identical names.

    - Data from multiple input aliases can be combined seamlessly within the same plot.

    - By default, the first added object defines the plot frame. This can be overridden via `SetDefinesFrame()`.
    
    - Ratios are added analogously via `AddRatio()` and provide additional ratio-specific options.
        
    - Supports ratios between many combinations of data types (histograms, graphs and functions), including interpolation for incompatible binning or coordinates.

    - Legend entries are generated automatically and inherit the appearance of the corresponding data by default.
    
    ],
    [
      #code-block(
        [
          ```cpp
          Plot plot("myPlot", "myGroup", "1d");
          
          plot[1].AddData("myGraph", "datasetA", "g");
          plot[1].AddData("myFunc", "datasetA", "f");
          plot[1].AddData("path/to/myHist", "datasetB", "h");
          
          plot[1].AddData("largerHisto", "datasetB")
                         .SetDefinesFrame();

          plot[1].AddRatio("numDataName", "datasetA",
                           "denomDataName", "datasetB",
                           "r");
          
          // generates a legend with entries "g", "h", "f" and "r"
          plot[1].AddLegend(0.9, 0.1); // rel. (x, y) pos. in pad
          ```
        ],
        [

        ],
      )
    ],
  )
]

#slide[
  #slide-title("Data Appearance WIP")
  #grid(
    columns: (45%, 50%),
    gutter: 5%,
    [
    - Appearance is configured by chaining setters directly to AddData() or later via plot[1](dataID) or plot[1].GetData(dataID).

    - SetOptions() accepts both predefined drawing styles (e.g. `points`, `curve`, `band`, `line`, `hist`) and native ROOT drawing option strings.
  
    - Labels support placeholders such as `<mean>`, `<integral>`, `<entries>`, `<maximum>`, etc., which are expanded automatically from the displayed data.
    
    - Reusable DataLayouts can be defined once and applied across many plots to ensure a consistent appearance.

    - Data layouts may also contain the input alias, allowing both the input source and the visual appearance to be reused together.
    
    - See the appendix for the complete list of available Data and Ratio setters.
        
    ],
    [
      #code-block(
        [
        ```cpp
        plot[1].AddData("hist", "datasetA",
                        "mean = <mean[.2f]>")
                      .SetOptions(points);

        plot[1].AddData("func", "inputLabelB")
                        .SetOptions("HIST C");

        // modify later
        plot[1](2).SetColor(kRed);

        Data dataLayout = Data()
          .SetDataset("datasetA")
          .SetOptions(points)
          .SetMarker(kBlue, kFullCircle, 1.2);

        // reuse appearance (and input alias)
        plot[1].AddData("graph1", dataLayout);
        plot[1].AddData("graph2", dataLayout).SetColor(kBlue);
        ```
        ],[
        
]
      )
    ],
  )
]


#slide[
  #slide-title("Work in Progress...")
  #grid(
    columns: (50%, 50%),
    gutter: 0%,
    [

    ],
    [
      #code-block(
        [

        ],
        [

        ],
      )
    ],
  )
]
