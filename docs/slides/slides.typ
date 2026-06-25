#import "@preview/polylux:0.4.0": *

#set document(
  title: "SciRooPlot Intro",
)
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
  if lang == "python" {
    [`DefinePlots.py`]
  } else {
    [`DefinePlots.cxx`]
  }
}

#let create-cmd = {
  if lang == "python" {
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
    #slide-title(title)
    #columns(
      desc,
      code-block(
        cpp,
        py,
      ),
    )
  ]
}

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

#let folder-struct = {
  if lang == "python" {
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

  #grid(
    columns: (55%, 45%),
    gutter: 1cm,
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
          #terminal[
            #prompt srp select myOtherProject
          ]
          *List all projects*
          #terminal[
            #prompt srp projects
          ]
          *Show help*
          #terminal[
            #prompt srp help
          ]

          *Change output directory* (default is `./output`)
          #terminal[
            #prompt srp set myProject OUT /path/to/output
          ]
        ],
      )
    ],
  )
]

#slide[
  #slide-title("Using the App")
  #grid(
    columns: (55%, 45%),
    gutter: 1cm,
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
          inset: (x: 0.3em, y: 0.3em),
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
          [`gif`], [Combine matching plots into an animated GIF #linebreak() (`gif+N` sets the frame delay in units of 10 ms).],
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
          #terminal[
            #prompt plot paperPlots \'moneyPlot[1,2]\'
          ]
          NB.: Quotes are only required if regex contains #linebreak() special shell characters like `[]`, `()` and `|`.

          *Select all plots in a figure group*
          #terminal[
            #prompt plot paperPlots .+
          ]

          *Select plots in groups starting same pattern*
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
        width: 84%
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

#slide[
  = Work in progress...
]
