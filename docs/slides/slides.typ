#import "@preview/polylux:0.4.0": *

#set page(
  width: 16in,
  height: 9in,
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
            #prompt srp init-py \<project\> [\<dir\>]
          ]
        } else {
          terminal[
            #prompt srp init-cpp \<project\> [\<dir\>]
          ]
        }

      - Creates a project directory (`./<project>` or `./<dir>`).

      - Contains #plot-def-file, where plot specifications are defined.

        #folder-struct

      - Project automatically registered with SciRooPlot.

      - Includes working examples based on the bundled dataSources.

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
            #prompt srp select \<project\>
          ]

          *List all projects*
          #v(-0.5em)
          #terminal[
            #prompt srp projects
          ]

          *Change output directory* (default is `./output`)
          #v(-0.5em)
          #terminal[
            #prompt srp set \<project\> OUT \<path\>
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
      - Plots specified in #plot-def-file have a unique *name* within a *group*.

      - These identifiers are used to select plots from the command line.
        #terminal[
          #prompt plot \<group\> \<name\> [\<mode\>]
        ]

      - Tab completion simplifies browsing through available names.
      - Both `<group>` and `<name>` support regular expressions.
      - Multiple plots can be generated with a single command.

      #block(
        fill: rgb("fafafa"),
        stroke: 1pt + rgb("d0d7de"),
        radius: 10pt,
        inset: 10pt,
      )[
        #table(
          columns: (20%, 80%),
          stroke: none,
          align: (left + horizon, left + horizon),
          table.header([*Mode*], [*Description*]),
          table.hline(stroke: 1pt + rgb("d0d7de")),
          [`show`], [Open plots interactively (default mode).],
          table.hline(stroke: 1pt + rgb("d0d7de")),
          [`list, print`],
          [List plots matching the request or print their settings.],
          table.hline(stroke: 1pt + rgb("d0d7de")),
          [`pdf`, `eps`,#linebreak()`svg`, `png`],
          [Export plots as graphics files.],
          table.hline(stroke: 1pt + rgb("d0d7de")),
          [`macro`], [Generate ROOT `.C` macros that reproduce the plots.],
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
            pm.AddDataSource("dataSource", "/path/to/file.root");
            { // ---------------------------------------------------------
              Plot plot("plotName", "groupName");
              plot[1].AddData("dataObjectName", "dataSource", "my label");
              plot[1].AddLegend();
              plot[1].AddText("some text");
              plot[1]['X'].SetTitle("x axis title");
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
            pm.AddDataSource("dataSource", "/path/to/file.root")
            # -----------------------------------------------------------
            plot = Plot("plotName", "groupName")
            plot[1].AddData("dataObjectName", "dataSource", "my label")
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
  #slide-title("Adding DataSources")
  #grid(
    columns: (42%, 55%),
    gutter: 3%,
    [
      - A dataSource is a collection of input sources with a unique identifier that is later used for the plot definitions.

      - Entire ROOT files or individual subdirectories/lists therein can be registered as input sources.

      - Multiple inputs can be added either via successive `AddDataSource()` calls or by passing them as a list.

      - Adding a directory to a dataSource registers all ROOT files it contains, including the ones in subdirectories.

      - File paths are always absolute, but the `SRC_DIR` helper enables paths relative to #plot-def-file.

      - Shell environment variables (including user-defined ones) are supported and expanded automatically.

      - Registered files are searched in alphabetical order. Within each file, the directory hierarchy is traversed until the first matching data object is found.

      - Local ROOT objects can also be directly added.

    ],
    [
      #code-block(
        [
          ```cpp
          pm.AddDataSource("dataSourceA", "/path/to/file1.root");

          pm.AddDataSource("dataSourceF", "/path/to/file6.root:dir/or/list");

          pm.AddDataSource("dataSourceD", {"/path/to/file4.root",
                                      "/path/to/file5.root"});

          pm.AddDataSource("dataSourceE", "/path/to/directory/");

          pm.AddDataSource("dataSourceB", SRC_DIR + "../rel/path/file2.root");

          pm.AddDataSource("dataSourceC", "${HOME}/path/to/file/file3.root");

          ```
        ],
        [
          ```python
          pm.AddDataSource("dataSourceA", "/path/to/file1.root")

          pm.AddDataSource("dataSourceB", SRC_DIR + "../rel/path/file2.root")

          pm.AddDataSource("dataSourceC", "${HOME}/path/to/file/file3.root")

          pm.AddDataSource("dataSourceD", ["/path/to/file4.root",
                                      "/path/to/file5.root"])

          pm.AddDataSource("dataSourceE", "/path/to/directory/")

          pm.AddDataSource("dataSourceF", "/path/to/file6.root:dir/or/list")
          ```
        ],
      )
      #code-block(
        [
          ```cpp
          auto myHist = new TH1D("myHist", "", 100, -5, 5);
          pm.AddDataSource("dataSourceG", myHist);

          auto myGraph = new TGraph();
          myGraph->SetName("myGraph"); // graph needs a name!
          auto myFunc = new TF1("myFunc", "gaus", -5, 5);
          pm.AddDataSource("dataSourceG", {myGraph, myFunc});
          ```
        ],
        [
          ```python
          myHist = ROOT.TH1D("myHist", "", 100, -5, 5)
          pm.AddDataSource("dataSourceG", myHist)

          myGraph = ROOT.TGraph()
          myGraph.SetName("myGraph")  # graph needs a name!
          myFunc = ROOT.TF1("myFunc", "gaus", -5, 5)
          pm.AddDataSource("dataSourceG", [myGraph, myFunc])
          ```
        ],
      )

    ],
  )
]

#slide[
  #slide-title("Plot Appearance")
  #grid(
    columns: (45%, 50%),
    gutter: 5%,
    [
      - Defining base plots avoids repeating common layout settings across many plots.

      - Base plots are ordinary `Plot` objects registered with the `PlotManager` via `AddBasePlot()`.

      - Default settings applied to `plot[0]` automatically affect all pads unless overridden.

      - Several ready-to-use base plots (e.g. `1d`, `2d`, `1d_ratio`) are provided with SciRooPlot.

      - Existing base plots can be used directly or modified to create custom layouts.

      - Axis ranges, titles, scales, and other axis properties are configured intuitively through plot[pad]['X'], ['Y'], and ['Z'].

      - List of accessors of Plot, Pad and Axis in appendix.
    ],
    [
      #code-block(
        [
          ```cpp
          Plot plot("myBasePlot");
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

          pm.AddBasePlot(plot);
          ```
        ],
        [
          ```python
          plot = Plot("myBasePlot")
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

          pm.AddBasePlot(plot)
          ```
        ],
      )
      #code-block(
        [
          ```cpp
          pm.AddBasePlot(PlotManager::MakeBasePlot("1d"));
          // create new plot on basis of "1d" base plot:
          Plot plot("myPlot", "myGroup", "1d");
          plot[1]['X'].SetRange(0., 10.).SetTitle("x title");
          ```
        ],
        [
          ```python
          pm.AddBasePlot(PlotManager.MakeBasePlot("1d"))
          # create a new plot based on the "1d" base plot
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
      - The data-type agnostic `AddData()` function accepts virtually all ROOT histograms, graphs, profiles, functions and trees.

      - Data is identified by name and automatically searched for in the corresponding dataSource across all associated files.

      - Directory or list paths within ROOT files can be prepended to object names to disambiguate identical names.

      - Data from multiple input aliases can be combined seamlessly within the same plot.

      - By default, the first added object defines the plot frame. This can be overridden via `SetDefinesFrame()`.

      - Ratios are added analogously via `AddRatio()` and support divisions between different data types. If the abscissae or binning differ, the denominator is interpolated.
    ],
    [
      #card(
        "Supported types",
        main-color,
        [
          - Histograms (`TH1`, `TH2`, `TH3`, `THn`, `THnSparse`)
          - Profiles (`TProfile`, `TProfile2D`)
          - Graphs (`TGraph`, `TGraph2D`)
          - Functions (`TF1`, `TF2`, `TF3`)
          - Efficiencies (`TEfficiency`)
          - Trees (`TTree`)
          - Tables (`.csv`, `.dat`, `.txt`, `.tsv`, `.tab`)
        ],
      )
      #code-block(
        [
          ```cpp
          plot[1].AddData("myGraph", "dataSourceA");
          plot[1].AddData("myFunc", "dataSourceA");
          plot[1].AddData("path/to/myHist", "dataSourceB");

          plot[1].AddData("largerHisto", "dataSourceB")
                          .SetDefinesFrame();

          plot[1].AddRatio("numDataName", "dataSourceA",
                           "denomDataName", "dataSourceB");
          ```
        ],
        [

        ],
      )
    ],
  )
]

#slide[
  #slide-title("Data Appearance")
  #grid(
    columns: (45%, 50%),
    gutter: 5%,
    [
      - Data appearance is configured by chaining setters directly to `AddData()` or afterwards by accessing previously added data via `plot[1](n)` (equivalent to `plot[1].GetData(n)`).

      - Drawing styles are configured via `SetOptions()`, accepting both native ROOT drawing option strings and predefined aliases (e.g. `points`, `line`, `curve`, `band`, `boxes`, `hist`, `colz`).

      - Complete `Data` layouts can be defined once and reused across many plots, ensuring a consistent appearance.

      - See the appendix for the complete list of available `Data` and `Ratio` setters.
    ],
    [
      #code-block(
        [
          ```cpp
          plot[1].AddData("h", "dataSourceA").SetOptions("HIST");
          plot[1].AddData("h", "dataSourceA").SetOptions(points);

          // modify data appearance later
          plot[1](1).SetColor(kRed);
          plot[1](2).SetMarkerStyle(kOpenCirlcle);

          Data layout = Data()
            .SetDataSource("dataSourceA")
            .SetOptions(curve)
            .SetLineStyle(kDashDotted)
            .SetLineWidth(4.)
            .SetLineColor(kOrange);

          // reuse appearance (requires dataSource to be defined)
          plot[1].AddData("graph1", layout);
          plot[1].AddData("graph2", layout).SetColor(kBlue);

          plot[1].AddRatio("h1", layout, "h2", "dataSourceB");
          ```
        ],
        [

        ],
      )
    ],
  )
]

#slide[
  #slide-title("Default Styles and Color Palettes")
  #grid(
    columns: (45%, 50%),
    gutter: 5%,
    [
      - Default colors, marker styles, line styles and other properties can be defined once per pad.

      - Style lists are applied cyclically to newly added data.

      - Continuous color gradients can be generated from arbitrary color endpoints.

      - The same gradients can be used both for automatic styling of many data objects and as palettes for 2D histograms.
    ],
    [
      #code-block(
        [
          ```cpp
          // cyclic default styling
          vector<int> colors = {kBlue, kRed, kGreen+2};
          vector<int> styles = {kOpenCircle, kOpenCross};
          plot[1].SetDefaultMarkerColors(colors);
          plot[1].SetDefaultMarkerStyles(styles);

          // continuous color gradient
          vector<tuple<float, float, float, float>> rainbowColors =
          { {0., 0., 1., 0.},   // blue
            {0., 1., 1., 0.25}, // cyan
            {0., 1., 0., 0.50}, // green
            {1., 1., 0., 0.75}, // yellow
            {1., 0., 0., 1.}    // red
          };
          plot[1].SetDefaultColors(rainbowColors);

          // same gradient as 2D palette
          plot[1].SetPalette(rainbowColors);

          // as alternative to standard root palettes
          plot[1].SetPalette(kBird);
          ```
        ],
        [

        ],
      )
    ],
  )
]

#slide[
  #slide-title("Legends and Text")
  #grid(
    columns: (45%, 50%),
    gutter: 5%,
    [
      - Legend entries are generated automatically and inherit the appearance of the corresponding data by default.

      - Labels support placeholders such as `<mean>`, `<integral>`, `<entries>`, `<maximum>`, which are expanded automatically.

      - Multiple legends can coexist and data can be assigned to specific legends via `SetLegend(n)`.

      - Legend entries can be customized individually.

      - custom legends and legend entries can be made

      - Text can be added also with multiple lines
    ],
    [
      #code-block(
        [
          ```cpp
          plot[1].AddData("h1", "input", "Data");
          plot[1].AddData("f1", "input", "Fit");
          plot[1].AddData("g1", "input", "Graph").SetLegend(2);

          // generates a legend with entries "Data", "Fit"
          plot[1].AddLegend(0.7, 0.8);
          // generates a second legend with entry "Graph"
          plot[1].AddLegend(0.2, 0.2);

          plot[1].GetLegend(2)
                 .GetEntry(1)
                 .SetLabel("Best fit");

          plot[1].AddLegend(0.9, 0.1); // rel. (x, y) pos. in pad

          plot[1].AddText(0.18, 0.88, "Work in progress");
          ```
        ],
        [

        ],
      )
    ],
  )
]

#slide[
  #slide-title("Projecting and Profiling Multidimensional Data")
  #grid(
    columns: (50%, 50%),
    gutter: 0%,
    [
      - Existing multidimensional histograms can be projected directly when adding them to a plot.

      - Standard projections (`ProjectX()`, `ProjectY()`, ...) are available for 2D and 3D histograms.

      - Projections can be restricted to selected ranges of the remaining axes using either bin numbers or user coordinates.

      - The generic Project() function supports arbitrary-dimensional histograms and arbitrary projection axes.

      - One- and two-dimensional profiles can be created analogously from multidimensional histograms.

      - The same functionality is available for numerator and denominator of ratios.
    ],
    [
      #code-block(
        [
          ```cpp
          // Standard projection of a 2D histogram
          plot[1].AddData("my2DHist", "input")
                 .ProjectX();

          // Restrict Y before projecting
          plot[1].AddData("my2DHist", "input")
                 .ProjectX(20, 80);

          // Generic projection of an N-dimensional histogram
          plot[1].AddData("myNDHist", "input")
                 .Project({2, 0}, {{1, 20, 80}, {3, 10, 40}});

          // Profile of a 2D histogram
          plot[1].AddData("my2DHist", "input")
                 .ProfileX();

          plot[1].AddData("myNDHist", "input")
                 .Profile({2}, {{0, 10, 50}});
          ```
        ],
        [

        ],
      )
    ],
  )
]


#slide[
  #slide-title("Processing Tree and CSV Data")
  #grid(
    columns: (50%, 45%),
    gutter: 5%,
    [
      - Tree leaves and CSV columns are accessed through the same interface.

      - Create histogram projections, profiles and scatter plots directly from tabular data.

      - Binning can be inferred automatically or specified explicitly with uniform or custom bin edges.

      - Data can be filtered, derived quantities defined and arbitrary C++ expressions used for projections and selections.

      - Entry ranges can be selected to process only subsets of the input.

      - The same functionality is available for both ROOT trees and CSV tables.
    ],
    [
      #code-block(
        [
          ```cpp
          // Histogram projection
          plot[1].AddData("myTree", "input")
                 .Project1D({"pt", 100})
                 .Filter("eta > 0");

          // Profile
          plot[1].AddData("myTree", "input")
                 .Profile1D("eta", "pt");     // <pt> vs eta

          // Scatter plot using arbitrary expressions
          plot[1].AddData("myTree", "input")
                 .Define("r", "sqrt(x*x+y*y)")
                 .Scatter("r", "z")
                 .Filter("abs(z) < 5");

          // CSV tables are handled identically
          plot[1].AddData("myData", "input")
                 .Scatter("a", "b");
          ```
        ],
        [

        ],
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
