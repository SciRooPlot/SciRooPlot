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
    [`DefinePlots.cxx`]
  }
}

#let create-cmd = {
  if lang == "py" {
    [create-python]
  } else {
    [create]
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
  #v(5% * slide-height)
  #line(
    length: 100%,
    stroke: 3pt + accent-color,
  )
  #place(
    center + horizon,
    image("logo.png", width: 22cm),
  )
  #v(76% * slide-height)
  #line(
    length: 100%,
    stroke: 3pt + accent-color,
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

      *The SciRooPlot approach*

      - Introduces an abstraction layer on top of ROOT.
      - Provides a common plotting infrastructure and organization.
      - Lets users describe the desired figure rather than its implementation.
      - Preserves the familiar ROOT ecosystem and styling conventions.
      - Enables interactive, project-based plotting workflows.

      *The result*

      - Less code.
      - Faster iteration.
      - Better organization.
      - Familiar ROOT backend.
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
  - Future updates can be installed with:
    #terminal[#prompt srp update]
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
        ├── DefinePlots.cxx
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
        #terminal[
          #prompt srp #create-cmd myProject [someDir]
        ]

      - Creates a project directory (`./someDir` or `./myProject`).
      - Contains #plot-def-file, where plot specifications are defined.

        #folder-struct

      - Project automatically registerd with SciRooPlot.
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
          [`inputs`], [Save input data for requested plots into a ROOT file.],
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
            pm.AddInput("inputAlias", "/path/to/file.root");
            { // ---------------------------------------------------------
              Plot plot("plotName", "groupName");
              plot[1].AddData("dataObjectName", "inputAlias", "my label");
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
            pm.AddInput("inputAlias", "/path/to/file.root")
            # -----------------------------------------------------------
            plot = Plot("plotName", "groupName")
            plot[1].AddData("dataObjectName", "inputAlias", "my label")
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
      #v(2em)
      #let link-to-other-doc = {
        [
          This documentation uses the
          #if lang == "py" {
            [Python interface. #linebreak() Find the #link("https://scirooplot.github.io/SciRooPlot/slides/SciRooPlot_UserManual_cpp.pdf")[#text(fill: main-color)[C++ version] here].]
          } else {
            [C++ interface. #linebreak() Find the #link("https://scirooplot.github.io/SciRooPlot/slides/SciRooPlot_UserManual_py.pdf")[#text(fill: main-color)[Python version] here].]
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
  #slide-title("Adding Input Data")
  #grid(
    columns: (42%, 55%),
    gutter: 3%,
    [
      - Input files are registered with the `PlotManager` under a user-defined alias that is later used to access the data.

      - File paths are always absolute, but the `SRC_DIR` helper enables paths relative to #plot-def-file.

      - Shell environment variables (including user-defined ones) are supported and expanded automatically.

      - Multiple files may be registered under the same alias using lists and/or successive `AddInput()` calls.

      - Adding a directory automatically registers all ROOT files it contains, including those in subdirectories.

      - Instead of an entire ROOT file, also individual subdirectories or object lists can be registered.

      - Registered files are searched in alphabetical order. Within each file, the directory hierarchy is traversed until the first matching data object is found.

      - In addition to files, transient in-memory ROOT objects can also be registered directly as inputs.
    ],
    [
      #code-block(
        [
          ```cpp
          pm.AddInput("inputAliasA", "/path/to/file1.root");

          pm.AddInput("inputAliasB", SRC_DIR + "../rel/path/file2.root");

          pm.AddInput("inputAliasC", "${HOME}/path/to/file/file3.root");

          pm.AddInput("inputAliasD", {"/path/to/file4.root",
                                      "/path/to/file5.root"});

          pm.AddInput("inputAliasE", "/path/to/directory/");

          pm.AddInput("inputAliasF", "/path/to/file6.root:dir/or/list");
          ```
        ],
        [
          ```python
          pm.AddInput("inputAliasA", "/path/to/file1.root")

          pm.AddInput("inputAliasB", SRC_DIR + "../rel/path/file2.root")

          pm.AddInput("inputAliasC", "${HOME}/path/to/file/file3.root")

          pm.AddInput("inputAliasD", ["/path/to/file4.root",
                                      "/path/to/file5.root"])

          pm.AddInput("inputAliasE", "/path/to/directory/")

          pm.AddInput("inputAliasF", "/path/to/file6.root:dir/or/list")
          ```
        ],
      )
      #code-block(
        [
          ```cpp
          auto myHist = new TH1D("myHist", "", 100, -5, 5);
          pm.AddInput("inputAliasG", myHist);

          auto myGraph = new TGraph();
          myGraph->SetName("myGraph"); // graph needs a name!
          auto myFunc = new TF1("myFunc", "gaus", -5, 5);
          pm.AddInput("inputAliasG", {myGraph, myFunc});
          ```
        ],
        [
          ```python
          myHist = ROOT.TH1D("myHist", "", 100, -5, 5)
          pm.AddInput("inputAliasG", myHist)

          myGraph = ROOT.TGraph()
          myGraph.SetName("myGraph")  # graph needs a name!
          myFunc = ROOT.TF1("myFunc", "gaus", -5, 5)
          pm.AddInput("inputAliasG", [myGraph, myFunc])
          ```
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
