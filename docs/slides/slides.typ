#import "@preview/polylux:0.4.0": *

#set page(
  width: 16in,
  height: 9in,
  margin: 0.5in,
)
#set text(
  font: ("Arial", "Liberation Sans", "DejaVu Sans"),
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
#let prompt = text(fill: main-color, weight: "bold")[\$]
#let terminal(body, size: 16pt, inset: 15pt) = block(
  fill: rgb("fafafa"),
  stroke: 1pt + rgb("d0d7de"),
  radius: 10pt,
  inset: inset,
)[
  #set text(font: "DejaVu Sans Mono", size: size)
  #body
]

#let card(title, color, body, width: auto, inset: 15pt, title-size: 24pt) = rect(
  fill: rgb("fafafa"),
  stroke: 1pt + rgb("d0d7de"),
  radius: 10pt,
  inset: inset,
  width: width,
)[
  #text(
    size: title-size,
    weight: "bold",
    fill: color,
  )[#title]
  #linebreak()
  #body
]

#let names(..items) = items.pos().map(i => raw(i)).join(linebreak())

#let step(n, title, desc) = rect(
  fill: rgb(28, 148, 165, 14),
  stroke: (left: 5pt + accent-color, rest: 1pt + rgb("d0d7de")),
  radius: 10pt,
  inset: 20pt,
  width: 100%,
)[
  #align(center)[
    #box(
      fill: accent-color,
      radius: 100%,
      width: 2.6em,
      height: 2.6em,
    )[
      #align(center + horizon)[
        #text(size: 26pt, weight: "bold", fill: white)[#n]
      ]
    ]
    #v(0.7em)
    #text(size: 20pt, weight: "bold", fill: main-color)[#title]
    #v(0.4em)
    #text(size: 14.5pt)[#desc]
  ]
]

#let flow-arrow = align(center + horizon)[
  #text(size: 40pt, fill: accent-color, weight: "bold")[→]
]

#let api-table(rows, col-widths: (44%, 56%), text-size: 13pt) = block(
  fill: rgb("fafafa"),
  stroke: 1pt + rgb("d0d7de"),
  radius: 10pt,
  inset: 0pt,
  width: 100%,
  clip: true,
)[
  #set text(size: text-size)
  #table(
    columns: col-widths,
    stroke: none,
    inset: (x: 0.6em, y: 0.5em),
    align: (left + top, left + top),
    fill: (x, y) => if calc.rem(y, 2) == 0 { rgb("eef2f5") } else { white },
    ..rows
  )
]

#let api-section(title, rows, col-widths: (44%, 56%), text-size: 13pt) = [
  #text(size: 18pt, weight: "bold", fill: main-color)[#title]
  #v(0.3em)
  #api-table(rows, col-widths: col-widths, text-size: text-size)
]


#slide[
  #grid(
    columns: (1fr, auto),
    align: (left + horizon, right + horizon),
    text(fill: main-color, size: 28pt, weight: "bold")[= SciRooPlot User Manual],
    text(fill: accent-color, size: 20pt, weight: "bold")[
      #if lang == "py" { "Python Version" } else { "C++ Version" }
    ],
  )
  #line(length: 100%, stroke: 3pt + accent-color)
  #v(0.5em)
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
          main-color,
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
  #v(1fr)
  #grid(
    columns: (1fr, 3.2em, 1fr, 3.2em, 1fr, 3.2em, 1fr),
    align: (top, horizon, top, horizon, top, horizon, top),
    step("1", "Store", "Keep your analysis results in ROOT or CSV files."),
    flow-arrow,
    step("2", "Define", [Describe the plots you want, in C++ or Python.]),
    flow-arrow,
    step("3", "Generate", [Browse and export plots with the `plot` command.]),
    flow-arrow,
    step("4", "Organize", [Manage projects and settings with `srp`.]),
  )
  #v(1fr)
]


#slide[
  #slide-title("Installation")
  - Works on Linux and macOS -- pick a package manager, or use the installer if you'd rather build from source.

  #v(0.06em)
  #card(
    "Homebrew",
    main-color,
    [
      #terminal(size: 11pt, inset: 5pt)[
        #prompt brew tap SciRooPlot/scirooplot

        #prompt brew install SciRooPlot

        #prompt source "\$(brew -\-prefix scirooplot)/share/scirooplot/env.sh"
      ]
    ],
    width: 100%,
    inset: 7pt,
    title-size: 15pt,
  )

  #v(0.06em)
  #card(
    "Conda",
    main-color,
    [
      #terminal(size: 11pt, inset: 5pt)[
        #prompt conda install conda-forge::scirooplot

        #prompt source "\$\{CONDA_PREFIX\}/share/scirooplot/env.sh"
      ]
    ],
    width: 100%,
    inset: 7pt,
    title-size: 15pt,
  )

  #v(0.06em)
  #card(
    "Installer",
    main-color,
    [
      #text(size: 10pt)[Builds SciRooPlot from source -- requires #link("https://github.com/root-project/root")[#text(fill: main-color)[ROOT]] and #link("https://www.boost.org/")[#text(fill: main-color)[Boost]] to already be installed.]
      #v(0.06em)
      #terminal(size: 11pt, inset: 5pt)[
        #prompt git clone https://github.com/SciRooPlot/SciRooPlot.git

        #prompt cd SciRooPlot && ./scripts/install.sh

        #prompt source ./install/share/scirooplot/env.sh
      ]
    ],
    width: 100%,
    inset: 7pt,
    title-size: 15pt,
  )

  #text(size: 14pt)[
    - Whichever method you pick, sourcing that `env.sh` script is what puts the `srp` and `plot` commands in your terminal -- add the same `source` line to your shell startup file (e.g. `~/.bashrc` or `~/.zshrc`; with the installer, use the absolute path it prints at the end) so it happens automatically in every new shell.

    - Later on, `$ srp update` upgrades SciRooPlot in place, however you installed it.
  ]
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

      - Includes working examples using dummy data.

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
            #prompt srp set \<project\> outdir \<path\>
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

          *Match one of a few names*
          #v(-0.5em)
          #terminal[
            #prompt plot paperPlots moneyPlot[1,2]
          ]

          *Match everything in a group*
          #v(-0.5em)
          #terminal[
            #prompt plot paperPlots .+
          ]

          *Match groups starting with a prefix*
          #v(-0.5em)
          #terminal[
            #prompt plot pp\_.+ myPlot
          ]

          *Combine names with OR*
          #v(-0.5em)
          #terminal[
            #prompt plot paperPlots \'pt(Spec|Mean)\'
          ]
          NB.: in `bash`, `()` and `|` are shell syntax, so they still need quotes -- unlike `[]`, which `plot` always handles safely unquoted.
        ],
      )
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
            PlotManager pm("myProject");
            pm.AddDataSource("dataSource", "/path/to/file.root");
            { // ---------------------------------------------------------
              Plot plot("plotName", "groupName");
              plot[1].AddData("dataObjectName", "dataSource", "my label");
              plot[1].AddLegend();
              plot[1].AddText("some text");
              plot[1]['X'].SetTitle("x axis title");
              pm.AddPlot(plot);
            } // ---------------------------------------------------------
            pm.SaveProject();
            return 0;
          }
          ```
        ],
        [
          ```python
          def main():
            pm = PlotManager("myProject")
            pm.AddDataSource("dataSource", "/path/to/file.root")
            # -----------------------------------------------------------
            plot = Plot("plotName", "groupName")
            plot[1].AddData("dataObjectName", "dataSource", "my label")
            plot[1].AddLegend()
            plot[1].AddText("some text")
            plot[1]['X'].SetTitle("x axis title")
            pm.AddPlot(plot)
            # -----------------------------------------------------------
            pm.SaveProject()

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
  #slide-title("Adding Data Sources")
  #grid(
    columns: (42%, 55%),
    gutter: 3%,
    [
      - A data source is a collection of input files with a unique identifier that is later used for the plot definitions.

      - Entire ROOT files or individual subdirectories/lists therein can be registered as input sources.

      - Multiple inputs can be added either via successive `AddDataSource()` calls or by passing them as a list.

      - Adding a directory registers every ROOT file inside it, including subdirectories.

      - File paths are always absolute, but the `SRC_DIR` helper enables paths relative to #plot-def-file.

      - Shell environment variables (including user-defined ones) are supported and expanded automatically.

      - Registered files are searched in alphabetical order. Within each file, the directory hierarchy is traversed until the first matching data object is found.

      - Local ROOT objects can also be directly added.

    ],
    [
      #code-block(
        [
          ```cpp
          pm.AddDataSource("sourceA", "/path/to/file1.root");

          pm.AddDataSource("sourceF", "/path/to/file6.root:dir/or/list");

          pm.AddDataSource("sourceD", {"/path/to/file4.root",
                                      "/path/to/file5.root"});

          pm.AddDataSource("sourceE", "/path/to/directory/");

          pm.AddDataSource("sourceB", SRC_DIR + "../path/file2.root");

          pm.AddDataSource("sourceC", "${HOME}/path/to/file/file3.root");

          ```
        ],
        [
          ```python
          pm.AddDataSource("sourceA", "/path/to/file1.root")

          pm.AddDataSource("sourceB", SRC_DIR + "../path/file2.root")

          pm.AddDataSource("sourceC", "${HOME}/path/to/file/file3.root")

          pm.AddDataSource("sourceD", ["/path/to/file4.root",
                                      "/path/to/file5.root"])

          pm.AddDataSource("sourceE", "/path/to/directory/")

          pm.AddDataSource("sourceF", "/path/to/file6.root:dir/or/list")
          ```
        ],
      )
      #code-block(
        [
          ```cpp
          auto myHist = new TH1D("myHist", "", 100, -5, 5);
          pm.AddDataSource("sourceG", myHist);

          auto myGraph = new TGraph();
          myGraph->SetName("myGraph"); // graph needs a name!
          auto myFunc = new TF1("myFunc", "gaus", -5, 5);
          pm.AddDataSource("sourceG", {myGraph, myFunc});
          ```
        ],
        [
          ```python
          myHist = ROOT.TH1D("myHist", "", 100, -5, 5)
          pm.AddDataSource("sourceG", myHist)

          myGraph = ROOT.TGraph()
          myGraph.SetName("myGraph")  # graph needs a name!
          myFunc = ROOT.TF1("myFunc", "gaus", -5, 5)
          pm.AddDataSource("sourceG", [myGraph, myFunc])
          ```
        ],
      )

    ],
  )
]

#if lang == "py" [
  #slide[
    #slide-title("Python Convenience Helpers")
    #grid(
      columns: (42%, 55%),
      gutter: 3%,
      [
        - `histo()`, `histo2d()` and `graph()` build ROOT objects straight from plain Python data, without going through PyROOT's own constructors.

        - `histo()` takes `bins` as a plain number and `range` as a `(min, max)` pair; if omitted, `bins` defaults to 100 and `range` is inferred from the data.

        - `histo2d()` takes `bins` and `range` as `(x, y)` pairs; if omitted, `bins` defaults to a 50 x 50 grid and `range` is inferred from the data, just like `histo()`.

        - `graph()` builds a `TGraphErrors`, with optional per-point `xerr`/`yerr`.

        - Inputs can be plain Python lists/tuples or numpy arrays -- anything iterable of numbers works.

        - All three return an object usable directly as `AddDataSource()` input.

        - Mismatched array lengths raise a clear error rather than silently producing bad data.
      ],
      [
        #code-block(
          [],
          [
            ```python
            # a plain list, a tuple, or a numpy array all work
            values = np.random.normal(0, 1, 1000)

            myHist = histo("myHist", values, bins=200, range=(0.1, 20.))

            xValues = np.linspace(0., 10., 10)
            yValues = np.random.rand(10)

            my2DHist = histo2d("my2DHist", xValues, yValues,
                               bins=(60, 40),
                               range=((0., 10.), (-5., 5.)))

            xErrors = yErrors = np.full(10, 0.1)

            myGraph = graph("myGraph", xValues, yValues,
                            xerr=xErrors, yerr=yErrors)

            pm.AddDataSource("sourceH", [myGraph, myHist, my2DHist])
            ```
          ],
        )
      ],
    )
  ]
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

      - Several ready-to-use base plots (e.g. `1d`, `2d`, `1d_ratio`, `1d_3panels`) are provided with SciRooPlot.

      - Existing base plots can be used directly or modified to create custom layouts.

      - Axis ranges, titles, scales, and other axis properties are configured intuitively through plot[pad]['X'], ['Y'], and ['Z'].

      - See the #link(<appx-plot>)[#text(fill: main-color)[appendix]] for the full list of `Plot`, `Pad` and `Axis` accessors.
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

      - Data is identified by name and automatically searched for in the corresponding data source across all associated files.

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
          plot[1].AddData("myGraph", "sourceA");
          plot[1].AddData("myFunc", "sourceA");
          plot[1].AddData("path/to/myHist", "sourceB");

          plot[1].AddData("largerHisto", "sourceB")
                          .SetDefinesFrame();

          plot[1].AddRatio("numDataName", "sourceA",
                           "denomDataName", "sourceB");
          ```
        ],
        [
          ```python
          plot[1].AddData("myGraph", "sourceA")
          plot[1].AddData("myFunc", "sourceA")
          plot[1].AddData("path/to/myHist", "sourceB")

          plot[1].AddData("largerHisto", "sourceB") \
                 .SetDefinesFrame()

          plot[1].AddRatio("numDataName", "sourceA",
                            "denomDataName", "sourceB")
          ```
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
      - Chain setters directly onto `AddData()`, or adjust the appearance later via `plot[1](n)` -- short for `plot[1].GetData(n)`.

      - Drawing styles are configured via `SetOptions()`, accepting both native ROOT drawing option strings and predefined aliases (e.g. `points`, `line`, `curve`, `band`, `boxes`, `hist`, `colz`).

      - Complete `Data` layouts can be defined once and reused across many plots, ensuring a consistent appearance.

      - See the #link(<appx-data>)[#text(fill: main-color)[appendix]] for the complete list of available `Data` and `Ratio` setters.
    ],
    [
      #code-block(
        [
          ```cpp
          plot[1].AddData("h", "sourceA").SetOptions("HIST");
          plot[1].AddData("h", "sourceA").SetOptions(points);

          // modify data appearance later
          plot[1](1).SetColor(kRed);
          plot[1](2).SetMarkerStyle(kOpenCircle);

          Data layout = Data()
            .SetDataSource("sourceA")
            .SetOptions(curve)
            .SetLineStyle(kDashDotted)
            .SetLineWidth(4.)
            .SetLineColor(kOrange);

          // reuse appearance (requires data source to be defined)
          plot[1].AddData("graph1", layout);
          plot[1].AddData("graph2", layout).SetColor(kBlue);

          plot[1].AddRatio("h1", layout, "h2", "sourceB");
          ```
        ],
        [
          ```python
          plot[1].AddData("h", "sourceA").SetOptions("HIST")
          plot[1].AddData("h", "sourceA").SetOptions(points)

          # modify data appearance later
          plot[1](1).SetColor(kRed)
          plot[1](2).SetMarkerStyle(kOpenCircle)

          layout = (Data()
              .SetDataSource("sourceA")
              .SetOptions(curve)
              .SetLineStyle(kDashDotted)
              .SetLineWidth(4.)
              .SetLineColor(kOrange))

          # reuse appearance (requires data source to be defined)
          plot[1].AddData("graph1", layout)
          plot[1].AddData("graph2", layout).SetColor(kBlue)

          plot[1].AddRatio("h1", layout, "h2", "sourceB")
          ```
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
          }; // (r,g,b,pos)
          plot[1].SetDefaultColors(rainbowColors);

          // same gradient as 2D palette
          plot[1].SetPalette(rainbowColors);

          // as alternative to standard root palettes
          plot[1].SetPalette(kBird);
          ```
        ],
        [
          ```python
          # cyclic default styling
          colors = [kBlue, kRed, kGreen+2]
          styles = [kOpenCircle, kOpenCross]
          plot[1].SetDefaultMarkerColors(colors)
          plot[1].SetDefaultMarkerStyles(styles)

          # continuous color gradient
          rainbowColors = [
              (0., 0., 1., 0.),    # blue
              (0., 1., 1., 0.25),  # cyan
              (0., 1., 0., 0.50),  # green
              (1., 1., 0., 0.75),  # yellow
              (1., 0., 0., 1.),    # red
          ]  # (r, g, b, pos)
          plot[1].SetDefaultColors(rainbowColors)

          # same gradient as 2D palette
          plot[1].SetPalette(rainbowColors)

          # as alternative to standard root palettes
          plot[1].SetPalette(kBird)
          ```
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

      - Labels support placeholders that are expanded automatically, e.g. `<name>`, `<title>`, `<entries>`, `<integral>`, `<maximum>`, `<minimum>`, `<mean>`.

      - Add optional printf-style formatting to a placeholder, e.g. `<mean[.2f]>` for two decimal places.

      - Multiple legends can coexist and data can be assigned to specific legends via `SetLegend(n)`.

      - Individual entries are looked up via `GetEntry(n)` and customized independently, e.g. to override a label.

      - For multi-line text, join the lines into one string with `" // "` in between; `SetLineSpacing()` sets the gap between them.
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

          plot[1].AddText("first line // second line");
          ```
        ],
        [
          ```python
          plot[1].AddData("h1", "input", "Data")
          plot[1].AddData("f1", "input", "Fit")
          plot[1].AddData("g1", "input", "Graph").SetLegend(2)

          # generates a legend with entries "Data", "Fit"
          plot[1].AddLegend(0.7, 0.8)
          # generates a second legend with entry "Graph"
          plot[1].AddLegend(0.2, 0.2)

          plot[1].GetLegend(2).GetEntry(1).SetLabel("Best fit")

          plot[1].AddLegend(0.9, 0.1)  # rel. (x, y) pos. in pad

          plot[1].AddText(0.18, 0.88, "Work in progress")

          plot[1].AddText("first line // second line")
          ```
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

      - The generic `Project(dims, ranges)` function supports arbitrary-dimensional histograms: `dims` lists the axes to keep (in order), `ranges` restricts any other axis via `(axis, low, high)` tuples.

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

          // Restrict Y to bins/coords [20, 80] before projecting
          plot[1].AddData("my2DHist", "input")
                 .ProjectX(20, 80);

          // Generic N-dim projection: keep axes 2 and 0
          // (in that order); restrict axis 1 to [20, 80]
          // and axis 3 to [10, 40]
          plot[1].AddData("myNDHist", "input")
                 .Project({2, 0}, {{1, 20, 80}, {3, 10, 40}});

          // Profile of a 2D histogram: <Y> as a function of X
          plot[1].AddData("my2DHist", "input")
                 .ProfileX();

          // Profile keeping axis 2; restrict axis 0 to [10, 50]
          plot[1].AddData("myNDHist", "input")
                 .Profile({2}, {{0, 10, 50}});
          ```
        ],
        [
          ```python
          # Standard projection of a 2D histogram
          plot[1].AddData("my2DHist", "input").ProjectX()

          # Restrict Y to bins/coords [20, 80] before projecting
          plot[1].AddData("my2DHist", "input").ProjectX(20, 80)

          # Generic N-dim projection: keep axes 2 and 0
          # (in that order); restrict axis 1 to [20, 80]
          # and axis 3 to [10, 40]
          plot[1].AddData("myNDHist", "input") \
                 .Project([2, 0], [[1, 20, 80], [3, 10, 40]])

          # Profile of a 2D histogram: <Y> as a function of X
          plot[1].AddData("my2DHist", "input").ProfileX()

          # Profile keeping axis 2; restrict axis 0 to [10, 50]
          plot[1].AddData("myNDHist", "input") \
                 .Profile([2], [[0, 10, 50]])
          ```
        ],
      )
    ],
  )
]


#slide[
  #slide-title("Processing Tree and Table Data")
  #grid(
    columns: (50%, 45%),
    gutter: 5%,
    [
      - Tree leaves and table columns of csv files are accessed through the same interface.

      - Create histogram projections, profiles and scatter plots directly from tabular data.

      - Binning can be inferred automatically or specified explicitly with uniform or custom bin edges.

      - Filter rows, define derived quantities, and use arbitrary ROOT expressions for projections and selections.

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
          ```python
          # Histogram projection
          plot[1].AddData("myTree", "input") \
                 .Project1D(("pt", 100)) \
                 .Filter("eta > 0")

          # Profile
          plot[1].AddData("myTree", "input") \
                 .Profile1D("eta", "pt")  # <pt> vs eta

          # Scatter plot using arbitrary expressions
          plot[1].AddData("myTree", "input") \
                 .Define("r", "sqrt(x*x+y*y)") \
                 .Scatter("r", "z") \
                 .Filter("abs(z) < 5")

          # CSV tables are handled identically
          plot[1].AddData("myData", "input") \
                 .Scatter("a", "b")
          ```
        ],
      )
    ],
  )
]


#slide[
  #slide-title("Ratios in Detail")
  #grid(
    columns: (45%, 50%),
    gutter: 5%,
    [
      - Ratios combine a numerator and a denominator into a single `Ratio` object via `AddRatio()`.

      - Numerator and denominator can be looked up by name and source, or supplied as complete `Data` layouts.

      - Use `SetIsCorrelated()` when the numerator is a sub-sample of the denominator (e.g. a selection vs. its parent sample) -- this applies Bayesian error propagation instead of treating the two as independent.

      - `SetDivideNormalized()` normalizes both terms to unity before dividing -- useful for comparing shapes rather than absolute yields.

      - The individual numerator and denominator remain accessible via `Numer()` and `Denom()`, which switch which side subsequent modifiers apply to (numerator by default).
    ],
    [
      #code-block(
        [
          ```cpp
          plot[1].AddRatio("h1", "sourceA", "h2", "sourceB")
                 .SetIsCorrelated();

          plot[1].AddRatio("h1", "sourceA", "h2", "sourceB")
                 .SetDivideNormalized();

          auto& ratio = plot[1].AddRatio("h1", "sourceA",
                                          "h2", "sourceB");
          ratio.Numer().SetColor(kBlue);
          ratio.Denom().SetColor(kRed);
          ```
        ],
        [
          ```python
          plot[1].AddRatio("h1", "sourceA", "h2", "sourceB") \
                 .SetIsCorrelated()

          plot[1].AddRatio("h1", "sourceA", "h2", "sourceB") \
                 .SetDivideNormalized()

          ratio = plot[1].AddRatio("h1", "sourceA", "h2", "sourceB")
          ratio.Numer().SetColor(kBlue)
          ratio.Denom().SetColor(kRed)
          ```
        ],
      )
    ],
  )
]

#slide[
  #slide-title("Pro Tips")
  #v(1fr)
  #grid(
    columns: (32%, 32%, 32%),
    gutter: 2%,
    [
      #card(
        "Interactive navigation",
        main-color,
        [
          - In `show` mode, browse through matching plots with `s` (next) and `a` (previous).

          - Press `q` to quit, or double-click the right/left side of the plot window.
        ],
      )
    ],
    [
      #card(
        "Positioning text & legends",
        main-color,
        [
          - Double-clicking on a text box or legend prints its current relative position to the terminal.

          - Move the box to where you want it, double-click, then copy the printed coordinates into your #plot-def-file.
        ],
      )
    ],
    [
      #card(
        "Comparing plots side by side",
        main-color,
        [
          - `plot` opens its picker window and blocks until you quit it -- append `&` to background it, then launch a second `plot` right away.

          - E.g. `plot paperPlots moneyPlot & plot paperPlots ptSpec` opens both windows at once, so you can compare them directly.
        ],
      )
    ],
  )
  #v(1fr)
]

// ============================================================================
// Appendix: complete accessor reference, generated from PlotManager.h / Plot.h
// Every signature is written once, in C++ form; when compiled for lang=py
// the show rule below rewrites `true`/`false`/`{}` to `True`/`False`/`None`
// wherever they appear in inline code spans, so both manuals stay correct
// without maintaining two copies of every row.
// ============================================================================

#show raw.where(block: false): it => {
  if lang == "py" {
    let t = it.text
    t = t.replace("{}", "None")
    t = t.replace(regex("\btrue\b"), "True")
    t = t.replace(regex("\bfalse\b"), "False")
    // Re-implement raw's own default look (DejaVu Sans Mono at 0.8em) rather
    // than calling raw() again, which would re-trigger this same show rule.
    text(font: "DejaVu Sans Mono", size: 0.8em, t)
  } else {
    it
  }
}

#slide[
  #[]<appx-plot>
  #slide-title("Appendix: PlotManager & Plot")
  #v(1fr)
  #grid(
    columns: (49%, 49%),
    gutter: 2%,
    [
      #api-section("PlotManager", (
        [`PlotManager(projectName = "")`], [Construct a manager for a named project.],
        [`MakeBasePlot(name = "1d", screenResolution = 100)`], [Static: retrieve a predefined base plot (`1d`, `2d`, `1d_ratio`, `1d_3panels`).],
        [`AddDataSource(id, files, replace = false)`], [Register file(s), a directory, or in-memory ROOT objects under an ID (`replace` first clears existing entries for that ID).],
        [#names("SaveDataSources(file = {})", "LoadDataSources(file = {}, replace = false)")], [Persist or reload registered data sources to/from a config file (defaults to the project's data-sources file).],
        [`AddPlot(plot)`], [Register a plot with the manager.],
        [`AddBasePlot(basePlot)`], [Register a reusable base plot layout.],
        [`SavePlots(name, group, file = {})`], [Write matching plots to a config file.],
        [`LoadPlots(name, group, file = {})`], [Load plots from a config file.],
        [`GeneratePlots(mode, name, group)`], [Generate plots matching the request in the given mode.],
        [`ListPlots()`], [Print all registered plots.],
        [#names("ClearDataBuffer()", "ClearCanvasRegistry()")], [Free the buffered input data, or clear the cache of already-generated canvases.],
        [`SetOutputDirectory(path)`], [Set the output directory for generated files.],
        [`SaveProject()`], [Persist the current project to disk.],
      ))
    ],
    [
      #api-section("Plot", (
        [`Plot(name, group = "", basePlot = {})`], [Create a plot in a figure group, optionally from a base plot.],
        [`Plot(other, name, group = {})`], [Copy `other` under a new name (and optionally a new group).],
        [`plot[padID]` / `GetPad(padID)`], [Access a pad by index.],
        [`plot += other`], [Append the pads of another plot into this one.],
        [#names("SetName(name)", "SetGroup(group)", "AppendGroup(subgroup)")], [Rename the plot, reassign its figure group, or append a subgroup path.],
        [`SetBasePlot(name)`], [Apply a base plot layout by name.],
        [`SetDimensions(width, height, fixAspectRatio = false)`], [Set canvas dimensions.],
        [#names("SetFill(color, style = {}, alpha = {})", "SetFillColor(color)", "SetFillStyle(style)", "SetFillAlpha(alpha)")], [Configure plot background fill, all at once or individually.],
        [`SetTransparent()`], [Make the plot background transparent.],
      ))
    ],
  )
  #v(1fr)
]

#slide[
  #[]<appx-pad>
  #slide-title("Appendix: Pad (1/2) – Adding Content")
  #v(1fr)
  #grid(
    columns: (49%, 49%),
    gutter: 2%,
    [
      #api-section("Accessing pad content", (
        [`pad['X']` / `GetAxis(axis)`], [Access an axis object ('X', 'Y', 'Z').],
        [`pad(dataID)` / `GetData(dataID)`], [Access a `Data` object by index.],
        [`GetLegend(legendID)` / `GetText(textID)`], [Retrieve a previously added legend or text box.],
      ))
    ],
    [
      #api-section("Adding content", (
        [`AddData(name, dataset, label = {})`], [Add data looked up by name from a registered source.],
        [`AddData(name, layout, label = {})`], [Add data using a predefined `Data` layout.],
        [`AddFunction(function, label = {})`], [Add a `TF1`-style function expression.],
        [`AddPoints(x, y, label = {})`], [Add points from separate X/Y arrays, or (x, y) pairs.],
        [`AddLine(pos1, pos2, label = {})`], [Add a line between two points.],
        [`AddRatio(...)`], [Add a ratio; overloads accept names, sources, or `Data` layouts.],
        [`AddText(xPos, yPos, text)` / `AddText(text)`], [Add a text box, fixed or auto-placed.],
        [`AddLegend(xPos, yPos, title = {})` / `AddLegend(title = {})`], [Add a legend, fixed or auto-placed.],
      ))
    ],
  )
  #v(1fr)
]

#slide[
  #slide-title("Appendix: Pad (2/2) – Layout & Palette")
  #v(1fr)
  #grid(
    columns: (49%, 49%),
    gutter: 2%,
    [
      #api-section("Position & view", (
        [`SetPosition(xlow, ylow, xup, yup)`], [Set the pad's position within the canvas.],
        [`SetMargins(top, bottom, left, right)`], [Set all four margins at once.],
        [#names("SetTopMargin(top)", "SetBottomMargin(bottom)", "SetLeftMargin(left)", "SetRightMargin(right)")], [Set an individual margin.],
        [`SetView(theta, phi)`], [Set the 3D viewing angles.],
        [`SetRedrawAxes(redraw = true)`], [Redraw axes on top of the drawn data.],
      ))
    ],
    [
      #api-section("Palette & background", (
        [`SetPalette(palette)`], [Use a predefined ROOT palette (e.g. `kBird`).],
        [`SetPalette(rgbEndpoints, alpha = {}, nColors = {})`], [Define a custom colour-gradient palette.],
        [#names("SetFill(color, style = {}, alpha = {})", "SetFillColor(color)", "SetFillStyle(style)", "SetFillAlpha(alpha)")], [Configure pad background fill.],
        [`SetTransparent()`], [Make the pad background transparent.],
        [#names("SetFrameFill(color, style = {}, alpha = {})", "SetFrameFillColor(color)", "SetFrameFillStyle(style)", "SetFrameFillAlpha(alpha)")], [Configure the frame's fill.],
        [#names("SetFrameBorder(color, style = {}, width = {}, alpha = {})", "SetFrameBorderColor(color)", "SetFrameBorderStyle(style)", "SetFrameBorderWidth(width)", "SetFrameBorderAlpha(alpha)")], [Configure the frame's border.],
      ))
    ],
  )
  #v(1fr)
]

#slide[
  #slide-title("Appendix: Pad Default Styling")
  - These set the fallback style applied to newly added data unless overridden per `Data` object -- see the "Default Styles and Color Palettes" slide for usage.
  #v(1fr)
  #grid(
    columns: (49%, 49%),
    gutter: 2%,
    [
      #api-section("Cyclic defaults (lists)", (
        [`SetDefaultColors(colors)`], [Cyclic colour list applied to fill/text where relevant.],
        [`SetDefaultMarkerColors(colors)`], [Cyclic marker colour list.],
        [`SetDefaultMarkerStyles(styles)`], [Cyclic marker style list.],
        [`SetDefaultLineColors(colors)`], [Cyclic line colour list.],
        [`SetDefaultLineStyles(styles)`], [Cyclic line style list.],
        [`SetDefaultFillColors(colors)`], [Cyclic fill colour list.],
        [`SetDefaultFillStyles(styles)`], [Cyclic fill style list.],
        [Any `SetDefault*Colors()` above], [Also accepts `(rgbEndpoints, alpha = {}, nColors = {})` for a continuous gradient instead of a list.],
      ))
    ],
    [
      #api-section("Scalar defaults & draw options", (
        [#names("SetDefaultTextSize(size)", "SetDefaultTextColor(color)", "SetDefaultTextAlpha(alpha)", "SetDefaultTextFont(font)")], [Default text styling applied pad-wide.],
        [#names("SetDefaultMarkerAlpha(alpha)", "SetDefaultMarkerSize(size)")], [Default marker transparency/size.],
        [#names("SetDefaultLineAlpha(alpha)", "SetDefaultLineWidth(width)")], [Default line transparency/width.],
        [`SetDefaultAlpha(alpha)`], [Default transparency for marker, line, *and* fill together.],
        [`SetDefaultFillAlpha(alpha)`], [Default fill transparency only.],
        [#names("SetDefaultDrawingOptionGraph(option)", "SetDefaultDrawingOptionHist(option)", "SetDefaultDrawingOptionHist2d(option)")], [Default draw-option alias per data kind.],
      ))
    ],
  )
  #v(1fr)
]

#slide[
  #[]<appx-axis>
  #slide-title("Appendix: Axis")
  #v(1fr)
  #grid(
    columns: (49%, 49%),
    gutter: 2%,
    [
      #api-section("Range, Ticks & Color", (
        [`SetTitle(title)`], [Axis title.],
        [#names("SetRange(min, max)", "SetMinRange(min)", "SetMaxRange(max)")], [Set the axis range, fully or partially.],
        [`SetColor(color)`], [Set both axis-line and tick colour.],
        [#names("SetAxisColor(color)", "SetAxisAlpha(alpha)")], [Axis line colour or transparency.],
        [#names("SetNumDivisions(n)", "SetMaxDigits(n)", "SetTickLength(len)")], [Tick divisions, max digits before scientific notation, and tick length.],
        [#names("SetLog(isLog = true)", "SetGrid(isGrid = true)")], [Logarithmic axis, or grid lines.],
      ))
    ],
    [
      #api-section("Title, Label & Display", (
        [#names("SetTitleFont(font)", "SetLabelFont(font)", "SetTitleSize(size)", "SetLabelSize(size)")], [Title/label font and size.],
        [#names("SetTitleColor(color)", "SetLabelColor(color)", "SetTitleAlpha(alpha)", "SetLabelAlpha(alpha)")], [Title/label colour and transparency.],
        [#names("SetTitleOffset(offset)", "SetLabelOffset(offset)")], [Title/label offset from the axis.],
        [#names("SetTitleCenter(center = true)", "SetLabelCenter(center = true)")], [Center the title or labels.],
        [#names("SetOppositeTicks(isOppositeTicks = true)", "SetNoExponent(isNoExponent = true)", "SetTickOrientation(mode)")], [Ticks on both sides, disable exponent notation, or set tick orientation (e.g. `"+-"`).],
        [`SetTimeFormat(fmt)`], [ROOT time-axis format string.],
      ))
    ],
  )
  #v(1fr)
]

#slide[
  #slide-title("Appendix: Ratio")
  - Every `Data` accessor from the next two slides is also available on `Ratio`, forwarded to whichever side (`Numer()`/`Denom()`) is currently active.
  #v(1fr)
  #api-section("Ratio-specific", (
    [`SetIsCorrelated(isCorrelated = true)`], [Treat numerator/denominator as correlated (e.g. one is a sub-sample of the other) -- applies Bayesian error propagation.],
    [`SetDivideNormalized(scaleBinWidth = false)`], [Normalize both sides to unity before dividing, to compare shapes rather than absolute yields.],
    [`Numer()` / `Denom()`], [Switch which side subsequent modifiers apply to (numerator by default).],
  ))
  #v(1fr)
]

#slide[
  #[]<appx-data>
  #slide-title("Appendix: Data (1/2) – Source, Range & Appearance")
  #v(1fr)
  #grid(
    columns: (49%, 49%),
    gutter: 2%,
    [
      #api-section("Source, range & legend", (
        [`SetDataSource(dataset)`], [Set/override the data-source ID.],
        [`SetLayout(layout)` / `ApplyLayout(layout)`], [Assign or merge in a predefined `Data` layout.],
        [#names("SetRangeX(min, max)", "SetRangeY(min, max)")], [Set the X or Y display range.],
        [`UnsetRangeX()` / `UnsetRangeY()`], [Remove a previously set range restriction.],
        [#names("SetScaleMinimum(f)", "SetScaleMaximum(f)")], [Scale the displayed minimum/maximum.],
        [`SetShowOverflowBins(showOverflowBins = true)`], [Include overflow bins in the display.],
        [`SetLegendLabel(label)` / `SetLegend(legendID)`], [Set legend text, or assign to a specific legend.],
        [`SetOptions(options)` / `SetOptions(alias)`], [Set a ROOT draw-option string, or one of the #link(<appx-drawing-options>)[#text(fill: main-color)[predefined aliases]].],
        [`SetTextFormat(fmt)`], [Printf-style format for label placeholders, e.g. `<mean[.2f]>`.],
      ))
    ],
    [
      #api-section("Appearance", (
        [`SetColor(color)` / `SetAlpha(alpha)`], [Set (or fade) marker, line and fill colour together.],
        [#names("SetMarker(color, style, size, alpha = {})", "SetMarkerColor(color)", "SetMarkerStyle(style)", "SetMarkerSize(size)", "SetMarkerAlpha(alpha)")], [Marker appearance, all at once or individually.],
        [#names("SetLine(color, style, width, alpha = {})", "SetLineColor(color)", "SetLineStyle(style)", "SetLineWidth(width)", "SetLineAlpha(alpha)")], [Line appearance, all at once or individually.],
        [#names("SetFill(color, style, alpha = {})", "SetFillColor(color)", "SetFillStyle(style)", "SetFillAlpha(alpha)")], [Fill appearance, all at once or individually.],
        [`SetDefinesFrame(dontDraw = false)`], [Use this object to define the plot frame.],
        [`SetContours(contours)` / `SetContours(nContours)`], [Set explicit contour levels or their count.],
      ))
    ],
  )
  #v(1fr)
]

#slide[
  #slide-title("Appendix: Data (2/2) – Modifiers & Processing")
  #v(1fr)
  #grid(
    columns: (49%, 49%),
    gutter: 2%,
    [
      #api-section("Transform existing histograms", (
        [`Normalize(scaleBinWidth = false)` / `NormalizeToMaximum(normMaximum = true)`], [Normalize the integral or the maximum to one.],
        [`Scale(scaleFactor)`], [Scale the object's contents.],
        [`DivideBinWidth(divideBinWidth = true)`], [Divide contents by bin width.],
        [`RebinX(n)` / `RebinY(n)` / `RebinXY(nx, ny)`], [Rebin one or both axes.],
        [`Smooth(nIterSmooth = 1)`], [Apply smoothing.],
        [`Project(dims, ranges = {}, isUserCoord = {})`], [Generic N-dimensional projection.],
        [`ProjectX(...)` / `ProjectY(...)`], [Standard projection of a 2D histogram.],
        [`Profile(dims, ranges = {}, isUserCoord = {})`], [Generic multi-dimensional profile.],
        [`ProfileX(...)` / `ProfileY(...)`], [Standard profile of a 2D histogram.],
      ))
    ],
    [
      #api-section("Tree & table data", (
        [`Project1D(x, weight = {})` / `Project2D(x, y, weight = {})`], [1D/2D histogram from a column; `x` accepts a name, `(name, nBins)`, `(name, nBins, (min, max))`, or `(name, edges)`.],
        [`Profile1D(x, profile, weight = {})` / `Profile2D(x, y, profile, weight = {})`], [1D/2D profile of one column over one or two others.],
        [`Scatter(x, y)`], [Scatter plot from two columns; expressions like `"sqrt(x*x+y*y)"` are valid.],
        [`Scatter(x, y, xErr, yErr)`], [Scatter plot with symmetric errors.],
        [`Scatter(x, y, xLo, xHi, yLo, yHi)`], [Scatter plot with asymmetric errors.],
        [`Define(key, value)`], [Define a derived variable for use in expressions.],
        [`Filter(filter)`], [Apply a row-selection expression, e.g. `"eta > 0"`. Chainable.],
        [`Entries(n)` / `Entries(min, max)`], [Limit processing to the first N entries or a range.],
      ))
    ],
  )
  #v(1fr)
]

#slide[
  #slide-title("Appendix: TextBox, LegendBox & LegendEntry")
  #v(1fr)
  #grid(
    columns: (33%, 33%, 32%),
    gutter: 1%,
    [
      #api-section("Box (shared)", (
        [`SetPosition(x, y)` / `SetSize(w, h)`], [Box position or dimensions.],
        [`SetAutoPlacement()`], [Let SciRooPlot place the box automatically.],
        [`SetUserCoordinates(isUserCoord = true)`], [Interpret position/size as user (data) coordinates instead of relative pad coordinates.],
        [#names("SetBorder(color, style, width, alpha = {})", "SetBorderColor(color)", "SetBorderStyle(style)", "SetBorderWidth(width)")], [Border, all at once or individually.],
        [#names("SetText(color, font, size, alpha = {})", "SetTextColor(color)", "SetTextFont(font)", "SetTextSize(size)", "SetTextAlpha(alpha)")], [Text styling, all at once or individually.],
        [#names("SetFill(color, style, alpha = {})", "SetFillColor(color)", "SetFillStyle(style)")], [Background fill.],
        [`SetTransparent()` / `SetNoBox()`], [Transparent background, or remove border and fill.],
        [`SetMargin(m)` / `SetLineSpacing(s)`], [Internal margin, or spacing between lines/entries.],
      ), col-widths: (56%, 44%), text-size: 12pt)
    ],
    [
      #api-section("TextBox & LegendBox", (
        ..(if lang == "cpp" {
          ([`TextBox(text)` / `TextBox(xPos, yPos, text)`], [Construct directly, auto-placed or fixed (in Python, create via `pad.AddText()` instead).])
        } else { () }),
        [`SetText(text)`], [Set the displayed text content.],
        ..(if lang == "cpp" {
          ([`LegendBox(title = {})` / `LegendBox(xPos, yPos, title = {})`], [Construct directly, auto-placed or fixed (in Python, create via `pad.AddLegend()` instead).])
        } else { () }),
        [`GetEntry(entryID)`], [Access a legend entry by index.],
        [`SetTitle(title)` / `SetNumColumns(n)`], [Legend title, or number of columns.],
        [#names("SetDefaultLineColor(color)", "SetDefaultMarkerColor(color)", "SetDefaultFillColor(color)")], [Default per-kind colour for entries without one.],
        [`SetDefaultDrawStyle(style)`], [Default draw style for entries without one.],
        [`SetSymbolColScale(scale)`], [Scale the width of the symbol column.],
      ), col-widths: (56%, 44%), text-size: 12pt)
    ],
    [
      #api-section("LegendEntry", (
        [`SetLabel(label)`], [Set the entry's label text.],
        [`SetRefData(refDataID)`], [Reference a `Data` object to copy its styling.],
        [`SetDrawStyle(style)`], [Override the legend draw style, e.g. `"L"`, `"EP"`.],
        [`SetColor(color)` / `SetAlpha(alpha)`], [Set overall colour/transparency.],
        [#names("SetMarkerColor(color)", "SetLineColor(color)", "SetFillColor(color)", "SetTextColor(color)")], [Per-kind colour override for this entry.],
      ), col-widths: (56%, 44%), text-size: 12pt)
    ],
  )
  #v(1fr)
]

#slide[
  #[]<appx-drawing-options>
  #slide-title("Appendix: Drawing Options – Histograms & Graphs")
  - Pass any alias below to `SetOptions()` (or `SetDefaultDrawingOption*()`) instead of a raw ROOT draw-option string -- the table shows the actual ROOT string it expands to for each data kind.
  #v(1fr)
  #block(
    fill: rgb("fafafa"),
    stroke: 1pt + rgb("d0d7de"),
    radius: 10pt,
    inset: 12pt,
    width: 100%,
  )[
    #set text(size: 11.5pt)
    #table(
      columns: (13%, 18.5%, 18.5%, 13%, 18.5%, 18.5%),
      stroke: none,
      inset: (x: 0.5em, y: 0.28em),
      align: left + horizon,
      fill: (x, y) => if calc.rem(y, 2) == 0 { rgb("eef2f5") } else { white },
      [*Alias*], [*Histograms*], [*Graphs*], [*Alias*], [*Histograms*], [*Graphs*],
      [`points`], [`X0 EP`], [`P Z`], [`boxes`], [`E2`], [`P2`],
      [`points_xerr`], [`EP`], [--], [`boxes_only`], [--], [`2`],
      [`points_endcaps`], [`E1`], [`P`], [`hist`], [`HIST`], [--],
      [`points_line`], [--], [`P Z L`], [`hist_no_borders`], [`HIST ][`], [--],
      [`line`], [`HIST L`], [`X L`], [`stars`], [`*H`], [--],
      [`curve`], [`HIST C`], [`X C`], [`text`], [`TEXT`], [--],
      [`band`], [`E5`], [`3`], [`brackets`], [--], [`[]`],
      [`band_smooth`], [`E6`], [`4`], [`hbar`], [`HIST HBAR`], [--],
      [`bar`], [`HIST B`], [`X B`], [`hbar_no_borders`], [`HBAR ][`], [--],
      [`area`], [`HIST F`], [`X CF`], [`hbar1`–`hbar4`], [`HBAR1`–`HBAR4`], [--],
      [`area_curve`], [`HIST CF`], [--], [`fit`], [#text(size: 9.5pt, fill: gray)[no mapping]], [#text(size: 9.5pt, fill: gray)[currently a no-op]],
      [`area_line`], [`HIST LF`], [`X LC`], [], [], [],
    )
  ]
  #v(1fr)
]

#slide[
  #slide-title("Appendix: Drawing Options – 2D Histograms")
  - These aliases apply to 2D data (`TH2`-like histograms): colour maps, surfaces, contours, legos, and candle plots.
  #v(1fr)
  #grid(
    columns: (49%, 49%),
    gutter: 2%,
    [
      #api-table((
        [`box`], [`BOX`],
        [`box1`], [`BOX1`],
        [`colz`], [`COLZ`],
        [`lego`], [`LEGO1 0`],
        [`lego_no_borders`], [`LEGO3 0`],
        [`legoz`], [`LEGO2Z 0`],
        [`surf`], [`SURF`],
        [`surf1`], [`SURF1`],
        [`surf1z`], [`SURF1Z`],
        [`surf2`], [`SURF2`],
        [`surf2z`], [`SURF2Z`],
        [`surf3`], [`SURF3`],
        [`surf3z`], [`SURF3Z`],
        [`surf4`], [`SURF4`],
      ), col-widths: (38%, 62%), text-size: 13pt)
    ],
    [
      #api-table((
        [`surf7`], [`SURF7`],
        [`surf7z`], [`SURF7Z`],
        [`cont`], [`CONT3`],
        [`contz`], [`CONTZ`],
        [`cont1z`], [`CONT1Z`],
        [`cont4z`], [`CONT4Z`],
        [`text`], [`TEXT`],
        [`candle1`–`candle6`], [`CANDLEX1`–`CANDLEX6`],
        [`candle7`], [`CANDLEX(111101)` #text(size: 10pt, fill: gray)[(mean instead of median)]],
      ), col-widths: (38%, 62%), text-size: 13pt)
    ],
  )
  #v(1fr)
]
