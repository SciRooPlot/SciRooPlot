#import "@preview/polylux:0.4.0": *

// ---- palette -----------------------------------------------------------------------------------------------
#let ink = rgb("16233A")         // titles and dark surfaces
#let main-color = rgb("1C94A5")  // teal of the logo: links, prompts, highlights
#let accent-color = rgb("E0A526") // amber: one accent per slide (title bar, step numbers)
#let paper = rgb("FBFAF7")       // page background
#let muted = rgb("6B7686")       // secondary text
#let hairline = rgb("DCE1E7")
#let tint = rgb("EEF6F7")        // light teal surface

// ---- navigation: the part of the manual each slide belongs to ----------------------------------------------
#let parts = ("Setting Up", "Defining Plots", "Building Plots", "Processing Data", "Reference")
// a divider slide places a part marker; the header shows the last marker on or before its page
#let part-marker(title) = [#metadata(title)<part>]

#set page(
  width: 16in,
  height: 9in,
  margin: 0.5in,
  fill: paper,
  header-ascent: 34%,
  header: context {
    let n = counter(page).get().first()
    let total = counter(page).final().first()
    if n > 1 {
      set text(size: 11pt, fill: muted, top-edge: "cap-height")
      let part = query(<part>).filter(m => m.location().page() <= n)
      grid(
        columns: (1fr, 1.6in, auto),
        column-gutter: 12pt,
        align: horizon,
        [#text(weight: "bold", fill: main-color)[SciRooPlot] #h(0.4em) · #h(0.4em) #(if part.len() > 0 { part.last().value } else { "Introduction" })],
        box(width: 100%, height: 3pt, radius: 1.5pt, fill: hairline, place(box(width: 100% * n / total, height: 3pt, radius: 1.5pt, fill: main-color))),
        [#n / #total],
      )
    }
  },
)
#set text(
  font: ("Arial", "Liberation Sans", "DejaVu Sans"),
  size: 18pt,
  fill: rgb("1F2933"),
  // fixed top edge of the lines (the cap height of Arial): Arial 2.82 (Microsoft core fonts, used to build the
  // slides in CI) has no cap height in its OS/2 table, so Typst would use its ascender and every line would get taller
  top-edge: 0.688em,
)
#set list(marker: text(fill: main-color)[•])
#show strong: set text(fill: ink)
// code keeps the line height of its own font
#show raw: set text(top-edge: "cap-height")

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
  fill: white,
  stroke: 1pt + hairline,
  inset: 10pt,
)[
  *#title*

  #v(0.3em)

  #desc
]

#let slide-title(title) = {
  text(
    fill: ink,
    size: 28pt,
    weight: "bold",
  )[ = #title ]
  // zero height like the stroke of a line, so the content area of the slides is unchanged
  box(width: 100%, height: 0pt, {
    place(horizon, line(length: 100%, stroke: 1pt + hairline))
    place(horizon, line(length: 2.2in, stroke: (paint: accent-color, thickness: 4pt, cap: "round")))
  })
  v(0.5em)
}

// one-sentence take-home message of a slide
#let key-idea(body, label: "Key idea") = block(
  fill: tint,
  stroke: (left: 4pt + main-color),
  radius: (right: 8pt),
  inset: (x: 14pt, y: 10pt),
  width: 100%,
)[#text(weight: "bold", fill: main-color)[#label] #h(0.6em) #body]

// the parts of the manual as a row of chips, the current one highlighted
#let roadmap(current) = grid(
  columns: parts.len(),
  column-gutter: 10pt,
  ..parts.enumerate().map(((i, p)) => {
    let on = i + 1 == current
    box(
      width: 100%,
      fill: if on { ink } else { white },
      stroke: 1pt + if on { ink } else { hairline },
      radius: 8pt,
      inset: (x: 12pt, y: 10pt),
    )[
      #text(size: 15pt, weight: "bold", fill: if on { accent-color } else { muted })[#(i + 1)]
      #h(0.4em)
      #text(size: 15pt, weight: "bold", fill: if on { white } else { muted })[#p]
    ]
  })
)

// divider slide opening part n of the manual: what it is about and what the reader will be able to do afterwards
#let section-slide(title, n, blurb, goals) = slide[
  #part-marker(title)
  #v(1fr)
  #grid(
    columns: (3.4in, 1fr),
    align: (left + horizon, left + horizon),
    text(size: 150pt, weight: "bold", fill: accent-color)[#n],
    [
      #text(size: 44pt, weight: "bold", fill: ink)[#title]
      #v(0.2em)
      #text(size: 22pt, fill: muted)[#blurb]
    ],
  )
  #v(0.8em)
  #block(inset: (left: 3.4in))[
    #text(size: 16pt, weight: "bold", fill: main-color)[AFTER THIS PART YOU CAN]
    #v(0.2em)
    #set text(size: 19pt)
    #for g in goals [ #text(fill: accent-color)[✓] #h(0.3em) #g \ ]
  ]
  #v(1fr)
  #roadmap(n)
]

#let code-block(cpp, py) = {
  block(
    fill: white,
    stroke: 1pt + hairline,
    radius: 10pt,
    inset: 14pt,
    width: 98%,
  )[
    #place(top + right, dx: 6pt, dy: -6pt, box(fill: tint, radius: 4pt, inset: (x: 6pt, y: 3pt),
      text(size: 10pt, weight: "bold", fill: main-color, top-edge: "cap-height")[#(if lang == "py" { "Python" } else { "C++" })]))
    #(if lang == "py" { py } else { cpp })
  ]
}
#let prompt = text(fill: main-color, weight: "bold")[\$]
#let terminal(body, size: 16pt, inset: 15pt, width: auto) = block(
  width: width,
  fill: rgb("F3F5F8"),
  stroke: 1pt + hairline,
  radius: 10pt,
  inset: inset,
)[
  #set text(font: "DejaVu Sans Mono", size: size)
  // straight quotes, so that commands can be copied from the slides
  #set smartquote(enabled: false)
  #body
]

#let card(title, color, body, width: auto, inset: 15pt, title-size: 24pt) = rect(
  fill: white,
  stroke: 1pt + hairline,
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
  fill: white,
  stroke: (top: 5pt + accent-color, rest: 1pt + hairline),
  radius: 10pt,
  inset: 20pt,
  width: 100%,
)[
  // no step number: the arrows give the order, and numbers are reserved for the parts of the manual
  #align(center)[
    #text(size: 24pt, weight: "bold", fill: ink)[#title]
    #v(0.4em)
    #text(size: 14.5pt)[#desc]
  ]
]

#let flow-arrow = align(center + horizon)[
  #text(size: 40pt, fill: accent-color, weight: "bold")[→]
]

#let api-table(rows, col-widths: (44%, 56%), text-size: 13pt) = block(
  fill: white,
  stroke: 1pt + hairline,
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
    fill: (x, y) => if calc.rem(y, 2) == 0 { tint } else { white },
    ..rows
  )
]

#let api-section(title, rows, col-widths: (44%, 56%), text-size: 13pt) = [
  #box(width: 9pt, height: 9pt, fill: accent-color, radius: 2pt) #h(0.3em) #text(size: 18pt, weight: "bold", fill: ink)[#title]
  #v(0.3em)
  #api-table(rows, col-widths: col-widths, text-size: text-size)
]

#show raw.where(block: false): it => {
  if lang == "py" {
    let t = it.text
    t = t.replace("{}", "None")
    t = t.replace("::", ".")
    t = t.replace(regex("\btrue\b"), "True")
    t = t.replace(regex("\bfalse\b"), "False")
    // raw's own 0.8em size is already in effect inside this show rule, so only
    // the font is set here (an extra size: 0.8em would shrink the code to 0.64em).
    text(font: "DejaVu Sans Mono", t)
  } else {
    it
  }
}


// background = the dark of the logo, so the logo sits seamlessly on the page
#page(fill: rgb("141A24"), margin: 0.8in)[
  #grid(
    columns: (1fr, 6.2in),
    column-gutter: 0.4in,
    align: (left + horizon, center + horizon),
    [
      #stack(spacing: 24pt, ..("SciRooPlot", "User Manual").map(t => text(size: 66pt, weight: "bold", fill: white, t)))
      #v(0.4em)
      #box(width: 2.2in, height: 5pt, fill: accent-color, radius: 2.5pt)
      #v(0.6em)
      #text(size: 26pt, fill: rgb("B8C2D0"))[Scientific plotting made simple: \ describe your figures, let ROOT draw them.]
      #v(1.6em)
      #box(fill: main-color, radius: 6pt, inset: (x: 14pt, y: 9pt),
        text(size: 18pt, weight: "bold", fill: white)[#if lang == "py" { "Python Version" } else { "C++ Version" }])
    ],
    image("logo.png", width: 6in),
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


// the same plot written as a plain ROOT macro (root-examples/), shown next to its SciRooPlot definition
#let root-macro-lines = {
  let lines = read("root-examples/ptSpectrumRatio.C").trim().split("\n")
  // skip the comment block at the top of the file
  while lines.len() > 0 and lines.first().starts-with("//") { lines = lines.slice(1) }
  lines
}
#let counted(lines) = lines.filter(l => l.trim() != "" and not l.trim().starts-with("//")).len()

#slide[
  #slide-title("An Everyday Example: Spectra and their Ratio")
  #let srp-cpp = ```cpp
  PlotManager pm("myProject");
  pm.AddDataSource("data", "~/data/spectra.root:data");
  pm.AddDataSource("mc", "~/data/spectra.root:mc");
  pm.AddBasePlot(PlotManager::MakeBasePlot("1d_ratio"));

  Plot plot("ptSpectrumRatio", "spectra", "1d_ratio");
  plot[1].AddData("ptSpec", "data", "data")
         .SetMarker(kBlack, kFullCircle, 1.);
  plot[1].AddData("ptSpec", "mc", "simulation")
         .SetDrawOption(hist).SetLine(kRed + 1, kSolid, 3.);
  plot[1].AddLegend();
  plot[1].AddText({"Example Experiment",
                   "pp, #sqrt{#it{s}} = 13.6 TeV", "|#eta| < 0.8"});
  plot[1]['Y'].SetLog();
  plot[2].AddRatio({"ptSpec", "data"}, {"ptSpec", "mc"});
  plot[2]['Y'].SetTitle("data / sim.").SetRange(0.7, 1.3);
  plot[0]['X'].SetLog().SetRange(0.15, 15.);
  pm.AddPlot(plot);
  pm.SaveProject();
  ```
  #let srp-py = ```python
  pm = PlotManager("myProject")
  pm.AddDataSource("data", "~/data/spectra.root:data")
  pm.AddDataSource("mc", "~/data/spectra.root:mc")
  pm.AddBasePlot(PlotManager.MakeBasePlot("1d_ratio"))

  plot = Plot("ptSpectrumRatio", "spectra", "1d_ratio")
  plot[1].AddData("ptSpec", "data", "data") \
         .SetMarker(kBlack, kFullCircle, 1.0)
  plot[1].AddData("ptSpec", "mc", "simulation") \
         .SetDrawOption(hist).SetLine(kRed + 1, kSolid, 3.0)
  plot[1].AddLegend()
  plot[1].AddText(["Example Experiment",
                   "pp, #sqrt{#it{s}} = 13.6 TeV", "|#eta| < 0.8"])
  plot[1]['Y'].SetLog()
  plot[2].AddRatio(["ptSpec", "data"], ["ptSpec", "mc"])
  plot[2]['Y'].SetTitle("data / sim.").SetRange(0.7, 1.3)
  plot[0]['X'].SetLog().SetRange(0.15, 15.0)
  pm.AddPlot(plot)
  pm.SaveProject()
  ```
  #let srp-code = if lang == "py" { srp-py } else { srp-cpp }
  #let lines = root-macro-lines
  // split the macro into two columns at the empty line closest to its middle
  #let cut = {
    let empty = range(lines.len()).filter(i => lines.at(i).trim() == "")
    empty.sorted(key: i => calc.abs(i - lines.len() / 2)).first()
  }
  #let macro-col(part) = raw(part.join("\n"), lang: "cpp", block: true)
  #grid(
    columns: (8.7in, 1fr),
    column-gutter: 0.3in,
    [
      #card(
        [Plain ROOT #h(1fr) #text(size: 14pt, weight: "regular", fill: muted)[#counted(lines) lines]],
        ink,
        width: 100%,
        title-size: 20pt,
      )[
        #set text(size: 7pt)
        #grid(columns: (1fr, 1fr), column-gutter: 14pt, macro-col(lines.slice(0, cut)), macro-col(lines.slice(cut + 1)))
      ]
    ],
    [
      #card(
        [SciRooPlot #h(1fr) #text(size: 14pt, weight: "regular", fill: muted)[#counted(srp-code.text.split("\n")) lines]],
        main-color,
        width: 100%,
        title-size: 20pt,
      )[
        #set text(size: 9.5pt)
        #srp-code
        #v(-0.3em)
        #text(size: 13pt)[#prompt `plot spectra ptSpectrumRatio pdf`]
      ]
      #v(-0.2em)
      #grid(
        columns: (1fr, 1fr),
        column-gutter: 10pt,
        align: center,
        image("root-examples/ptSpectrumRatio_root.png", height: 1.5in),
        image("root-examples/ptSpectrumRatio_scirooplot.png", height: 1.5in),
        text(size: 12pt, fill: muted)[plain ROOT], text(size: 12pt, fill: muted)[SciRooPlot],
      )
      #v(-0.2em)
      #key-idea[#set text(size: 13pt); Same plot, without file handling, pads, margins, text sizes and box placement.]
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
  #key-idea[You describe _what_ a figure shows, once. SciRooPlot finds the data, draws it with ROOT, and keeps every plot of your project in one place.]
]


#section-slide("Setting Up", 1, [Install SciRooPlot and get a first plot on screen.], (
  [install it with conda, homebrew or from source,],
  [create a project and run your first plot,],
  [switch between projects and use the `plot` app.],
))

#slide[
  #slide-title("Installation")
  // one way of installing per row: name and when to pick it on the left, the commands on the right
  #let route(n, name, desc, commands) = rect(width: 100%, fill: white, stroke: 1pt + hairline, radius: 10pt, inset: 16pt)[
    #grid(
      columns: (3.9in, 1fr),
      column-gutter: 0.3in,
      align: (left + horizon, left + horizon),
      [
        #box(fill: accent-color, radius: 100%, width: 1.6em, height: 1.6em, align(center + horizon, text(size: 15pt, weight: "bold", fill: white)[#n]))
        #h(0.3em)
        #text(size: 22pt, weight: "bold", fill: ink)[#name]
        #v(-0.4em)
        #text(size: 15pt, fill: muted)[#desc]
      ],
      terminal(size: 13pt, inset: 12pt, width: 100%, commands),
    )
  ]
  #text(size: 20pt)[Pick *one* of three ways -- each gives you the `srp` and `plot` commands. Linux and macOS.]
  #v(1fr)
  #stack(spacing: 14pt,
  route("A", "Homebrew", [The prebuilt package \ from the SciRooPlot tap.])[
    #prompt brew tap SciRooPlot/scirooplot \
    #prompt brew install SciRooPlot \
    #prompt source "\$(brew -\-prefix scirooplot)/share/scirooplot/env.sh"
  ],
  route("B", "Conda", [The conda-forge package, \ into the active environment.])[
    #prompt conda install conda-forge::scirooplot \
    #prompt source "\$\{CONDA_PREFIX\}/share/scirooplot/env.sh"
  ],
  route("C", "From source", [Needs #link("https://github.com/root-project/root")[#text(fill: main-color)[ROOT]] (6.36.04 or newer) \ and #link("https://www.boost.org/")[#text(fill: main-color)[Boost]] installed already.])[
    #prompt git clone https://github.com/SciRooPlot/SciRooPlot.git \
    #prompt cd SciRooPlot && ./scripts/install.sh \
    #prompt source ./install/share/scirooplot/env.sh
  ],
  )
  #v(1fr)
  #grid(
    columns: (1fr, 1fr),
    column-gutter: 0.3in,
    key-idea(label: "In every shell")[#set text(size: 15pt); The last line of each way, `source …/env.sh`, adds `srp` and `plot` to the shell. Put it into `~/.bashrc` or `~/.zshrc` (from source: the absolute path the installer prints) to have them in every new shell.],
    key-idea(label: "Updates")[#set text(size: 15pt); Later on, `srp update` upgrades SciRooPlot in place, whichever way you installed it.],
  )
]


#slide[
  #slide-title("Your First Project")
  #let folder-struct = {
    if lang == "py" {
      terminal(size: 16pt, inset: 10pt)[
        ```text
        myProject/
        ├── DefinePlots.py    your plot definitions
        └── output/           exported plots
        ```
      ]
    } else {
      terminal(size: 16pt, inset: 10pt)[
        ```text
        myProject/
        ├── CMakeLists.txt
        ├── DefinePlots.cpp   your plot definitions
        ├── build/            compiled program
        └── output/           exported plots
        ```
      ]
    }
  }
  #grid(
    columns: (52%, 45%),
    gutter: 3%,
    [
      - Create a project with a minimal working setup:
        #if lang == "py" {
          terminal[
            #prompt srp init-py \<project\> [\<dir\>]
          ]
        } else {
          terminal[
            #prompt srp init-cpp \<project\> [\<dir\>]
          ]
        }

      - Creates `./<project>` (or `./<dir>`), #if lang == "py" [runs the script] else [builds and runs the program], and registers and selects the project:

        #folder-struct

      - It already contains one plot, so your first plot is just:
        #terminal[
          #prompt plot myGroup myFirstPlot
        ]

      - Define your own plots in #plot-def-file -- `plot` #if lang == "py" [re-runs it] else [rebuilds and re-runs it] automatically whenever it changed.
    ],
    [
      #card(
        "Learn from the examples",
        main-color,
        width: 100%,
        title-size: 20pt,
        [
          #set text(size: 16pt)
          A project with many commented example plots and the example data they use -- Higgs searches, spectra, detector performance, jets and heavy ions:
          #v(-0.3em)
          #terminal(size: 14pt, inset: 10pt)[
            #if lang == "py" [#prompt srp example-py \<project\> [\<dir\>]] else [#prompt srp example-cpp \<project\> [\<dir\>]]
          ]
          #v(-0.3em)
          Browse its Higgs plots, or export them all as pdf (`.+` stands for any name):
          #v(-0.3em)
          #terminal(size: 14pt, inset: 10pt)[
            #prompt plot higgs .+

            #prompt plot higgs .+ pdf
          ]
          #v(-0.3em)
          To see how something is done, look it up in its #plot-def-file and copy what you need.
        ],
      )
    ],
  )
]

#slide[
  #slide-title("Managing Projects")
  #let cmd-table(..rows) = {
    set text(size: 14pt)
    table(
      columns: (auto, 1fr),
      stroke: none,
      inset: (x: 0pt, y: 0.3em),
      column-gutter: 1em,
      align: left + top,
      ..rows
    )
  }
  #grid(
    columns: (49%, 49%),
    gutter: 2%,
    [
      #card(
        "Projects",
        main-color,
        width: 100%,
        title-size: 20pt,
        cmd-table(
          [#prompt `srp projects`], [List all projects (`*` marks the selected one).],
          [#prompt `srp select <project>`], [Select the project that `plot` works on.],
          [#prompt `srp show [<project>]`], [Show the properties of one or all projects.],
          [#prompt `srp add <project> <program> [<outdir>]`], [Register an existing program as project (rebuilt automatically if it lives in a CMake build directory).],
          [#prompt `srp rename <project> <name>`], [Rename a project (then also adjust the name given to `PlotManager`).],
          [#prompt `srp remove <project>`], [Unregister the project and delete its stored plot definitions (user code and output are kept).],
          [#prompt `srp clean`], [Remove all projects whose program folder no longer exists.],
          [#prompt `srp cd [<project>]`], [Go to the folder of the project's program.],
          [#prompt `srp edit [<project>]`], [Open the project's source files.],
          [#prompt `srp stats <project>`], [Number of plots and inputs, and when the definitions were last saved.],
        ),
      )
      #v(-0.4em)
      #text(size: 14pt)[Instead of a project name, `@current` refers to the selected project.]
    ],
    [
      #card(
        "Project properties",
        main-color,
        width: 100%,
        title-size: 20pt,
        cmd-table(
          [#prompt `srp set <project> outdir <path>`], [Where exported plots are stored (one subfolder per group).],
          [#prompt `srp set <project> program <path>`], [The program `plot` runs to update the plot definitions.],
          [#prompt `srp set <project> <var> <value>`], [Your own variables, e.g. the input path on this machine -- read in code via `pm.GetProjectProperty("<var>")`.],
          [#prompt `srp get|unset <project> <var>`], [Read or remove a property.],
        ),
      )
      #v(-0.4em)
      #card(
        "Settings & tools",
        main-color,
        width: 100%,
        title-size: 20pt,
        cmd-table(
          [#prompt `srp settings`], [Show all settings.],
          [#prompt `srp plotmode <mode>`], [Default mode of `plot` (initially `show`).],
          [#prompt `srp matchmode exact|contains`], [Patterns must match the whole name, or any part of it.],
          [#prompt `srp matchcase sensitive|insensitive`], [Case sensitivity of the patterns.],
          [#prompt `srp screenscale|bitmapscale <factor>`], [Size of plots on screen, or resolution of png/jpg/gif files (initially `1`).],
          [#prompt `srp print|open <file>`], [List the contents of a ROOT file, or open it in a `TBrowser`.],
          [#prompt `srp info` / `srp help`], [Show installation details, or all commands.],
        ),
      )
    ],
  )
]

#slide[
  #slide-title("Using the App")
  #grid(
    columns: (55%, 40%),
    gutter: 5%,
    [
      - Plots specified in #plot-def-file have a unique *name* within a *group*, which can have subgroups (e.g. `higgs/diphoton`).

      - These identifiers select plots on the command line:
        #terminal[
          #prompt plot \<group\> \<name\> [\<mode\>]
        ]

      - Tab completion browses the available names.
      - `<group>` and `<name>` are regular expressions matching the whole name; without `<mode>`, the default mode (`srp plotmode`) is used.

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
          [`show`, `spread`], [Show plots one by one (default) or all at once.],
          table.hline(stroke: 1pt + rgb("d0d7de")),
          [`list, print`],
          [List plots matching the request or print their settings.],
          table.hline(stroke: 1pt + rgb("d0d7de")),
          [`pdf`, `eps`, `ps`,#linebreak()`svg`, `png`, `jpg`],
          [Export each plot into `<outdir>/<group>/` (also `root`, `json`, `xml`, `html`).],
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
        width: 100%,
        title-size: 20pt,
        [
          #set text(size: 16pt)

          *One of a few names, or a plot and its variants*
          #v(-0.5em)
          #terminal(size: 14pt, inset: 10pt)[
            #prompt plot paperPlots moneyPlot[12]

            #prompt plot paperPlots moneyPlot.\*
          ]

          *Match everything in a group and its subgroups*
          #v(-0.5em)
          #terminal(size: 14pt, inset: 10pt)[
            #prompt plot paperPlots .+
          ]

          *Only a subgroup, or no subgroups*
          #v(-0.5em)
          #terminal(size: 14pt, inset: 10pt)[
            #prompt plot paperPlots/QA .+

            #prompt plot paperPlots\$ .+
          ]

          *Combine names with OR*
          #v(-0.5em)
          #terminal(size: 14pt, inset: 10pt)[
            #prompt plot paperPlots \'pt(Spec|Mean)\'
          ]
          Quote a pattern containing `()` or `|`, which are shell syntax; `[]`, `*` and a final `$` need no quotes.
        ],
      )
    ],
  )
]

#section-slide("Defining Plots", 2, [Tell SciRooPlot where your data lives and which plots to make.], (
  [read the structure of a plot definition file,],
  [split many plots across several files,],
  [point plots to data in ROOT and CSV files.],
))

#slide[
  #slide-title("General Code Structure")
  #grid(
    columns: (40%, 57%),
    gutter: 3%,
    [
      - Plots are handled and exported by `PlotManager`.

      - Input files are registered under user-defined aliases.

      - Data objects within input files are identified by name.

      - Each Plot has a unique name within a figure group and can build on a base plot layout.

      - It consists of one or more Pads (`plot[1]`, `plot[2]`, ..)

      - Pads contain settings of Axis, Data, Text and Legend.

        #terminal[
          ```
          PlotManager
          ├── Plot
          │     ├── Pad
          │     │     ├── Axis
          │     │     ├── Data
          │     │     ├── Legend
          │     │     └── Text
          │     ├── Pad
          │     └── ...
          ├── Plot
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
            pm.AddBasePlot(PlotManager::MakeBasePlot("1d"));
            { // ---------------------------------------------------------
              Plot plot("plotName", "groupName", "1d");
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
            pm.AddBasePlot(PlotManager.MakeBasePlot("1d"))
            # -----------------------------------------------------------
            plot = Plot("plotName", "groupName", "1d")
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
  #slide-title("Splitting Plots Across Multiple Files")
  #grid(
    columns: (42%, 55%),
    gutter: 3%,
    [
      - As #plot-def-file grows, split it into several files -- e.g. one per figure group -- each holding a plain function that takes the `PlotManager`.

      #if lang == "cpp" [
        - Each file includes `SciRooPlot/PlotManager.h`; declare its function in #plot-def-file (or a shared header) and call it from `main()`.

        - Every file is a separate translation unit -- add it to `CMakeLists.txt`'s `SOURCES` list, or the build won't see it.
      ] else [
        - Each module starts with `from SciRooPlot import *`; `import` its function in #plot-def-file and call it from `main()`.

        - Nothing to register -- the script's own folder is already on the import path, so the new module is found automatically.

        - `plot` re-runs #plot-def-file whenever any `.py` file of the project folder (or its direct subfolders) changed.
      ]
    ],
    [
      #if lang == "cpp" [
        #grid(
          columns: (auto, 1fr),
          gutter: 3%,
          terminal[
            ```text
            myProject/
            ├── CMakeLists.txt
            ├── DefinePlots.cpp
            ├── HiggsPlots.cpp
            └── DetectorPlots.cpp
            ```
          ],
          terminal[
            ```text
            # CMakeLists.txt
            add_plotting_executable(definePlots
              SOURCES
                DefinePlots.cpp
                HiggsPlots.cpp
                DetectorPlots.cpp
            )
            ```
          ],
        )
        #v(0.35em)
        #[
          #code-block(
            [
              ```cpp
              // DefinePlots.cpp
              void DefineHiggsPlots(PlotManager& pm);
              void DefineDetectorPlots(PlotManager& pm);

              int main() {
                PlotManager pm("myProject");
                DefineDataSources(pm);
                DefineBasePlots(pm);
                DefineHiggsPlots(pm);
                DefineDetectorPlots(pm);
                pm.SaveProject();
              }
              ```
            ],
            [],
          )
        ]
      ] else [
        #terminal[
          ```text
          myProject/
          ├── DefinePlots.py     main(), imports the below
          ├── higgs_plots.py     DefineHiggsPlots()
          └── detector_plots.py  DefineDetectorPlots()
          ```
        ]
        #v(0.5em)
        #code-block(
          [],
          [
            ```python
            # DefinePlots.py
            from higgs_plots import DefineHiggsPlots
            from detector_plots import DefineDetectorPlots

            def main():
                pm = PlotManager("myProject")
                DefineDataSources(pm)
                DefineBasePlots(pm)
                DefineHiggsPlots(pm)
                DefineDetectorPlots(pm)
                pm.SaveProject()
            ```
          ],
        )
      ]
    ],
  )
]


#slide[
  #slide-title("Adding Data Sources")
  #grid(
    columns: (42%, 55%),
    gutter: 3%,
    [
      - A data source is a collection of inputs -- ROOT files (or folders/lists within them) and tables (`.csv`, ...) -- registered under a unique identifier that the plot definitions refer to.

      - Inputs are added via successive `AddDataSource()` calls or as a list; a directory adds every ROOT and table file inside it, also in subdirectories.

      - Wildcards select all matching files and/or folders within the files (e.g. `AO2D_*.root:DF_*`).

      - Paths are absolute, but `SRC_DIR` enables paths relative to #plot-def-file; environment variables are expanded automatically.

      - Inputs are searched in the order they were added (files from directories or wildcards alphabetically) and the first match counts -- but trees found in several inputs are chained.

      - Local ROOT objects can be added directly; lists and TDirectories become folders.

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

          pm.AddDataSource("sourceG", "/path/to/AO2D_*.root:DF_*");
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

          pm.AddDataSource("sourceG", "/path/to/AO2D_*.root:DF_*")
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
          import ROOT
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

#slide[
  #slide-title("Finding Data in an Input")
  #let found(body) = text(fill: accent-color, weight: "bold", body)
  #let order(n) = text(fill: main-color, weight: "bold")[#n]
  #grid(
    columns: (47%, 50%),
    gutter: 3%,
    [
      #terminal(size: 15pt)[
        #show raw: set text(size: 1.25em)
        #grid(
          columns: (auto, 1fr),
          row-gutter: 0.55em,
          align: (left, right),
          raw("file.root"), found(raw("AddData(\"h\", \"src\")")),
          raw("├─ h"), [#order[1] taken if it exists here],
          raw("├─ dirA/"), [#order[2] else dirA, completely:],
          raw("│  ├─ x"), [],
          raw("│  └─ sub/"), [#order[3] its sub-folder],
          raw("│     └─ h"), found[← found: dirA/sub/h],
          raw("└─ dirB/"), [not reached],
          raw("   └─ h"), [],
        )
      ]
      #v(0.6em)
      - The current folder is searched first, then its sub-folders and lists one after the other in stored order -- each completely, including its own sub-folders (depth-first). The first match counts, always the same one.

      - A name that exists more than once: give the path, `AddData("dirB/h", "src")`, or restrict the source, `AddData("h", "src:dirB")`.

      - A path always goes through folders and lists: a histogram of the same name does not block it.
    ],
    [
      #card("Names", main-color, title-size: 20pt, width: 100%)[
        #set text(size: 15pt)
        Directories: the key name (newest cycle; older cycles of a folder or list are not searched) -- lists and folders: the object name -- maps: the key.
      ]
      #v(0.3em)
      #card("Containers", main-color, title-size: 20pt, width: 100%)[
        #set text(size: 15pt)
        Directories, `TList`, `THashList`, `TObjArray`, `TOrdCollection`, `TSortedList`, `THashTable`, `TMap`, `TClonesArray`, `TFolder` and classes derived from them -- nested in any combination.
      ]
      #v(0.3em)
      #card("Data", main-color, title-size: 20pt, width: 100%)[
        #set text(size: 15pt)
        Histograms and profiles (1D--3D, `THnSparse` via projections), graphs (incl. `TGraph2D`), `TF1`/`TF2`/`TF3`, `TEfficiency`, trees (chained over all inputs if stored in folders, processed in memory if stored in a list).
      ]
      #v(0.3em)
      #card("Not readable", accent-color, title-size: 20pt, width: 100%)[
        #set text(size: 15pt)
        Classes without a loaded dictionary (a warning tells you), objects without a name (e.g. `TObjString`), `THStack` and `TMultiGraph` as input.
      ]
    ],
  )
]

#slide[
  #slide-title("Data Sources with Several Inputs")
  #let found(body) = text(fill: accent-color, weight: "bold", body)
  #let order(n) = text(fill: main-color, weight: "bold")[#n]
  #grid(
    columns: (52%, 45%),
    gutter: 3%,
    [
      #terminal(size: 15pt)[
        #show raw: set text(size: 1.25em)
        #raw(if lang == "py" { "AddDataSource(\"src\", [\"a.root\", \"runs/*.root\"])" } else { "AddDataSource(\"src\", {\"a.root\", \"runs/*.root\"})" })
        #grid(
          columns: (auto, 1fr),
          row-gutter: 0.55em,
          align: (left, right),
          [#order[1] #raw("a.root")], raw("h, t"),
          [#order[2] #raw("runs/run1.root")], raw("t"),
          [#order[3] #raw("runs/run2.root")], raw("h, t"),
        )
        #v(0.4em)
        #grid(
          columns: (auto, 1fr),
          row-gutter: 0.55em,
          align: (left, right),
          raw("AddData(\"h\", \"src\")"), found[h of 1 (first input)],
          raw("AddData(\"t\", \"src\")"), found[t of 1+2+3 chained],
        )
      ]
      #v(0.4em)
      #terminal(size: 15pt)[
        #show raw: set text(size: 1.25em)
        #raw("AddDataSource(\"ao2d\", \"AO2D_*.root:DF_*\")")
        #grid(
          columns: (auto, 1fr),
          row-gutter: 0.55em,
          align: (left, right),
          [#order[1] #raw("AO2D_1.root:DF_11")], raw("O2track"),
          [#order[2] #raw("AO2D_1.root:DF_12")], raw("O2track"),
          [#order[3] #raw("AO2D_2.root:DF_31")], raw("O2track"),
        )
        #v(0.4em)
        #grid(
          columns: (auto, 1fr),
          align: (left, right),
          raw("AddData(\"O2track\", \"ao2d\")"), found[chained over 1+2+3],
        )
      ]
    ],
    [
      - The inputs of a data source are searched in the order they were added; files from a directory or a wildcard come alphabetically, folders matching a wildcard (`file.root:DF_*`) in the order they appear in the file -- each folder is an input of its own.

      - Within each input the search works as on the previous slide.

      - Histograms, graphs, functions: the first input containing the name counts.

      - Trees stored in folders: all inputs containing the tree are chained and processed together (e.g. one file per run, one folder per time frame). A tree stored in a list cannot be chained: the first one found is used.

      - `"src:folder"` in `AddData` restricts the search to that folder in each input (wildcards allowed, e.g. `"ao2d:DF_1*"`); inputs without it are skipped.

      - Data that must not be merged (e.g. train and test samples) belongs into separate data sources.
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
            import numpy as np

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

#section-slide("Building Plots", 3, [Put data on a plot and make it look the way you want.], (
  [choose a base plot and fill its pads,],
  [style data with colours, markers and palettes,],
  [add legends and text boxes.],
))

#slide[
  #slide-title("Plot Appearance")
  #grid(
    columns: (45%, 50%),
    gutter: 5%,
    [
      - Base plots are ordinary `Plot` objects registered via `AddBasePlot()`, so common layout settings are not repeated in every plot.

      - Settings of `plot[0]` apply to all pads unless overridden.

      - `PlotManager::MakeBasePlot(name)` provides base plots with consistent layout among them: text, ticks, distances, lines and markers are the same in every panel, so plots of different layouts look alike. The name describes the layout: `<type>[_wide|_tall][_<columns>x<rows>][_ratio][_gap]`, e.g. `1d`, `1d_ratio`, `2d`, `1d_wide`, `1d_2x1`, `1d_3x1`, `1d_2x1_ratio`, `1d_2x2`, `2d_2x1`. Other sizes: `MakeBasePlot(name, PanelLayout().SetTextSize(24))`.

      - Other layouts are made with `PanelLayout`#if lang == "cpp" [ (`#include "SciRooPlot/PanelLayout.h"`)]: panels in rows (`Panel`, `Gap()`, `Empty()`), lengths in pixels; panels that touch share their axis, pads are numbered row by row. The code of `MakeBasePlot` is the start for your own.

      - Axes are configured via `plot[pad]['X']`, `['Y']` and `['Z']`; see the #link(<appx-plot>)[#text(fill: main-color)[appendix]] for all `Plot`, `Pad` and `Axis` accessors.
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
          pm.AddBasePlot(PlotManager::MakeBasePlot("1d_ratio"));
          // create new plot on basis of this base plot:
          Plot plot("myPlot", "myGroup", "1d_ratio");
          // an own layout in the same style (see MakeBasePlot):
          Plot two("2x2_3", PanelLayout({{Panel(), Panel()},
                                         {Panel(), Empty()}}));
          two[0].SetDefaultTextFont(42);
          pm.AddBasePlot(two);
          ```
        ],
        [
          ```python
          pm.AddBasePlot(PlotManager.MakeBasePlot("1d_ratio"))
          # create a new plot based on this base plot
          plot = Plot("myPlot", "myGroup", "1d_ratio")
          # an own layout in the same style (see MakeBasePlot)
          two = Plot("2x2_3", PanelLayout([[Panel(), Panel()],
                                           [Panel(), Empty()]]))
          two[0].SetDefaultTextFont(42)
          pm.AddBasePlot(two)
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

          plot[1].AddRatio({"numDataName", "sourceA"},
                           {"denomDataName", "sourceB"});
          ```
        ],
        [
          ```python
          plot[1].AddData("myGraph", "sourceA")
          plot[1].AddData("myFunc", "sourceA")
          plot[1].AddData("path/to/myHist", "sourceB")

          plot[1].AddData("largerHisto", "sourceB") \
                 .SetDefinesFrame()

          plot[1].AddRatio(["numDataName", "sourceA"],
                           ["denomDataName", "sourceB"])
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

      - Drawing styles are configured via `SetDrawOption()`, accepting both native ROOT drawing option strings and predefined aliases (e.g. `points`, `line`, `curve`, `band`, `boxes`, `hist`, `colz`).

      - A `Data` object can be defined once and reused as template: `layout("name")` gives the data with all its properties (appearance, projections, modifiers), ensuring a consistent appearance.

      - See the #link(<appx-data>)[#text(fill: main-color)[appendix]] for the complete list of available `Data` and `Ratio` setters.
    ],
    [
      #code-block(
        [
          ```cpp
          plot[1].AddData("h", "sourceA").SetDrawOption("HIST");
          plot[1].AddData("h", "sourceA").SetDrawOption(points);

          // modify data appearance later
          plot[1](1).SetColor(kRed);
          plot[1](2).SetMarkerStyle(kOpenCircle);

          Data layout = Data()
            .SetDataSource("sourceA")
            .SetDrawOption(curve)
            .SetLineStyle(kDashDotted)
            .SetLineWidth(4.)
            .SetLineColor(kOrange);

          // reuse as template: layout("name") gives the data
          plot[1].AddData(layout("graph1"));
          plot[1].AddData(layout("graph2"), "label").SetColor(kBlue);

          plot[1].AddRatio(layout("h1"), Data("h2", "sourceB"));
          ```
        ],
        [
          ```python
          plot[1].AddData("h", "sourceA").SetDrawOption("HIST")
          plot[1].AddData("h", "sourceA").SetDrawOption(points)

          # modify data appearance later
          plot[1](1).SetColor(kRed)
          plot[1](2).SetMarkerStyle(kOpenCircle)

          layout = (Data()
              .SetDataSource("sourceA")
              .SetDrawOption(curve)
              .SetLineStyle(kDashDotted)
              .SetLineWidth(4.)
              .SetLineColor(kOrange))

          # reuse as template: layout("name") gives the data
          plot[1].AddData(layout("graph1"))
          plot[1].AddData(layout("graph2"), "label").SetColor(kBlue)

          plot[1].AddRatio(layout("h1"), Data("h2", "sourceB"))
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
          vector<int16_t> colors = {kBlue, kRed, kGreen+2};
          vector<int16_t> styles = {kOpenCircle, kOpenCross};
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
      - Legend entries are generated automatically and look like their data by default.

      - Placeholders in labels: `<name>`, `<title>`, `<entries>`, `<integral>`, `<maximum>`, `<minimum>`, `<mean>` (printf formats: `<mean[.2f]>`), `<source>`, `<projection>`, `<selection>`.

      - Several legends can coexist, numbered from 1 in the order they're added; `SetLegend(n)` assigns data to one, e.g. `SetLegend(2)`.

      - Legends and text boxes go to a fixed position, or are auto-placed if it is left out: as close as possible to the best-fitting frame corner (`best_corner`) or a requested one (`top_left`, `top_right`, `bottom_right`, `bottom_left`).

      - `SetLegendEntry(LegendEntry()...)` on the data styles its row; `AddEntry(label)` adds a row of its own (no label: an empty line), at the end or placed with `AtTop()`, `BeforeData(n)`, `AfterData(n)`.

      - Multi-line text: a list of lines (#if lang == "py" [`["a", "b"]`] else [`{"a", "b"}`]) or one string with newlines; `SetLineSpacing()` sets the gap.
    ],
    [
      #code-block(
        [
          ```cpp
          plot[1].AddData("h1", "input", "Data");
          plot[1].AddData("f1", "input", "Fit");
          plot[1].AddData("g1", "input", "Graph").SetLegend(2);

          // legend 1 ("Data", "Fit") at rel. (x, y) pos. in pad
          plot[1].AddLegend(0.7, 0.8);
          // legend 2 ("Graph")
          plot[1].AddLegend(0.2, 0.2);

          plot[1].GetLegend(2)
                 .AddEntry("Best fit")
                 .SetSymbolElements("L").SetLine(kRed, kSolid, 2.);

          // auto-placed: best-fitting or given corner
          plot[1].AddLegend();
          plot[1].AddLegend(top_right);

          plot[1].AddText(0.18, 0.88, "Work in progress");
          plot[1].AddText({"first line", "second line"});
          ```
        ],
        [
          ```python
          plot[1].AddData("h1", "input", "Data")
          plot[1].AddData("f1", "input", "Fit")
          plot[1].AddData("g1", "input", "Graph").SetLegend(2)

          # legend 1 ("Data", "Fit") at rel. (x, y) pos. in pad
          plot[1].AddLegend(0.7, 0.8)
          # legend 2 ("Graph")
          plot[1].AddLegend(0.2, 0.2)

          plot[1].GetLegend(2).AddEntry("Best fit").SetSymbolElements("L").SetLine(kRed, kSolid, 2.)

          # auto-placed: best-fitting or given corner
          plot[1].AddLegend()
          plot[1].AddLegend(top_right)

          plot[1].AddText(0.18, 0.88, "Work in progress")
          plot[1].AddText(["first line", "second line"])
          ```
        ],
      )
    ],
  )
]

#section-slide("Processing Data", 4, [Transform data on the fly instead of in your analysis code.], (
  [normalize, rebin and scale histograms,],
  [project multidimensional data and read trees and tables,],
  [build ratios and use the interactive tricks.],
))

#slide[
  #slide-title("Modifying Data")
  #grid(
    columns: (47%, 50%),
    gutter: 3%,
    [
      - `Scale(factor)` multiplies the object's values (`y` for 1D data, `z` for 2D data); `Normalize()` and `NormalizeToMaximum()` pick that factor automatically, from the integral or the peak.

      - `DivideBinWidth()` divides the contents by the bin widths; `Density()` does that and normalizes the integral to one (a probability density from counts). For data that already is a density (e.g. dN/dpT) use `Normalize(true)`, which normalizes with the bin-width-weighted integral.

      - `RebinX()` / `RebinY()` / `RebinZ()` -- and the `RebinXY()` / `RebinXYZ()` shortcuts -- merge neighboring bins along an axis. Histograms only.

      - `Cumulative()` replaces the histogram with its running sum, low to high; pass `false` for a high-to-low sum. 1D histograms only.

      - `Smooth()` applies ROOT's smoothing algorithm, repeated `nIterSmooth` times. For profiles it smooths the mean values of the filled bins (empty bins have no mean) and keeps their errors.

      - `ScaleX()` / `ScaleY()` / `ScaleZ()` rescale the axis itself (e.g. a unit conversion).
    ],
    [
      #code-block(
        [
          ```cpp
          // Normalize the integral to 1, merge bin pairs,
          // and add 30% headroom to the y-axis for a legend
          plot[1].AddData("h", "sourceA")
                 .Normalize()
                 .RebinX(2)
                 .SetScaleMaximum(1.3);

          // Running sum (low -> high), then convert axis units
          plot[1].AddData("h2", "sourceA")
                 .Cumulative()
                 .ScaleX(1000.);  // e.g. GeV -> MeV

          // dN/dpT-style density: content divided by bin width
          plot[1].AddData("h3", "sourceA")
                 .DivideBinWidth();
          ```
        ],
        [
          ```python
          # Normalize the integral to 1, merge bin pairs,
          # and add 30% headroom to the y-axis for a legend
          plot[1].AddData("h", "sourceA") \
                 .Normalize() \
                 .RebinX(2) \
                 .SetScaleMaximum(1.3)

          # Running sum (low -> high), then convert axis units
          plot[1].AddData("h2", "sourceA") \
                 .Cumulative() \
                 .ScaleX(1000.)  # e.g. GeV -> MeV

          # dN/dpT-style density: content divided by bin width
          plot[1].AddData("h3", "sourceA") \
                 .DivideBinWidth()
          ```
        ],
      )
    ],
  )
]


#slide[
  #slide-title("Projecting and Profiling Multidimensional Data")
  #grid(
    columns: (47%, 50%),
    gutter: 3%,
    [
      - Existing multidimensional histograms can be projected directly when adding them to a plot.

      - Standard projections (`ProjectX()`, `ProjectY()`, ...) are available for 2D and 3D histograms.

      - Projections can be restricted to selected ranges of the remaining axes, given as bin numbers or -- with `true` as last argument -- as axis values.

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

          // Restrict Y to bins 20 to 80 before projecting
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

          // Profile keeping axis 2; axis 0 to bins [10, 50]
          // (false: bins; one range alone needs it in C++)
          plot[1].AddData("myNDHist", "input")
                 .Profile({2}, {{0, 10, 50}}, false);
          ```
        ],
        [
          ```python
          # Standard projection of a 2D histogram
          plot[1].AddData("my2DHist", "input").ProjectX()

          # Restrict Y to bins 20 to 80 before projecting
          plot[1].AddData("my2DHist", "input").ProjectX(20, 80)

          # Generic N-dim projection: keep axes 2 and 0
          # (in that order); restrict axis 1 to [20, 80]
          # and axis 3 to [10, 40]
          plot[1].AddData("myNDHist", "input") \
                 .Project([2, 0], [[1, 20, 80], [3, 10, 40]])

          # Profile of a 2D histogram: <Y> as a function of X
          plot[1].AddData("my2DHist", "input").ProfileX()

          # Profile keeping axis 2; axis 0 to bins [10, 50]
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
      - Leaves of ROOT trees and columns of tables (`.csv`, `.txt`, `.dat`, `.tsv`, `.tab`) are accessed through the same interface.

      - Create histogram projections, profiles and scatter plots directly from tabular data.

      - Binning can be inferred automatically or specified explicitly with uniform or custom bin edges.

      - Filter rows, define derived quantities, and use arbitrary C++ expressions of the columns for projections and selections.

      - `Join()` adds the columns of another tree, row by row or matched by up to two integer key columns (rows without a match are skipped).

      - Entry ranges can be selected to process only subsets of the input.
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

          // Add the event columns to each track (by key)
          plot[1].AddData("tracks", "input")
                 .Join("events", {"run", "event"})
                 .Profile1D("centrality", "pt");

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

          # Add the event columns to each track (by key)
          plot[1].AddData("tracks", "input") \
                 .Join("events", ["run", "event"]) \
                 .Profile1D("centrality", "pt")

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

      - Numerator and denominator are given as `{name, source}` (python: `[name, source]`) or as complete `Data` objects, e.g. from a template (`style("name")`); their projections and modifiers act on the inputs, the ratio takes the appearance of the numerator. A predefined `Ratio(numerator, denominator)` can be added as well.

      - Use `SetIsCorrelated()` when the numerator is a sub-sample of the denominator (e.g. a selection vs. its parent sample) -- this applies binomial error propagation instead of treating the two as independent.

      - Modifiers (`RebinX()`, `Scale()`, `Normalize()`, `Smooth()`, `Cumulative()`, ...) act on the ratio itself by default. `Numer()`, `Denom()` and `Both()` switch them to the inputs, applied before dividing; `Result()` switches back.

      - `Both().Normalize()` normalizes both terms to unity before dividing -- useful for comparing shapes rather than absolute yields.
    ],
    [
      #code-block(
        [
          ```cpp
          plot[1].AddRatio({"h1", "sourceA"}, {"h2", "sourceB"})
                 .SetIsCorrelated();

          plot[1].AddRatio({"h1", "sourceA"}, {"h2", "sourceB"})
                 .Both().Normalize();

          plot[1].AddRatio({"h1", "sourceA"}, {"h2", "sourceB"})
                 .Both().RebinX(2)
                 .Denom().Smooth()
                 .Result().SetColor(kRed);
          ```
        ],
        [
          ```python
          plot[1].AddRatio(["h1", "sourceA"], ["h2", "sourceB"]) \
                 .SetIsCorrelated()

          plot[1].AddRatio(["h1", "sourceA"], ["h2", "sourceB"]) \
                 .Both().Normalize()

          plot[1].AddRatio(["h1", "sourceA"], ["h2", "sourceB"]) \
                 .Both().RebinX(2) \
                 .Denom().Smooth() \
                 .Result().SetColor(kRed)
          ```
        ],
      )
    ],
  )
]

// tips as cards of equal size, three per row
#let tip-grid(..tips) = layout(size => {
  let gap = 0.3in
  let width = (size.width - 2 * gap) / 3
  let bodies = tips.pos().map(((title, body)) => [
    #text(size: 20pt, weight: "bold", fill: main-color)[#title]
    #v(0.2em)
    #body
  ])
  let inset = 18pt
  let height = calc.max(..bodies.map(b => measure(block(width: width - 2 * inset, b)).height)) + 2 * inset
  grid(
    columns: (width,) * 3,
    gutter: gap,
    ..bodies.map(b => rect(width: width, height: height, inset: inset, radius: 10pt, fill: white, stroke: 1pt + hairline, b)),
  )
})

#slide[
  #slide-title("Pro Tips: In the Plot Window")
  #v(1fr)
  #tip-grid(
    ([Browse through plots], [
      - In `show` mode, browse through the matching plots with `s` (next) and `a` (previous), in the window or in the terminal.
      - Double-clicking the right or left half of the window works too; `q` closes a window, `x` all.
    ]),
    ([Place text and legends by hand], [
      - Double-click a text box or legend: the position of its upper left corner is printed to the terminal.
      - Drag the box to where you want it, double-click, and copy the printed coordinates into your #plot-def-file.
    ]),
    ([Compare plots side by side], [
      - Press `d` while browsing in `show` mode to move the current plot into a window of its own next to the browsing window.
      - `plot paperPlots .+ spread` opens all matching plots at once, each in its own window.
    ]),
  )
  #v(1fr)
]

#slide[
  #slide-title("Pro Tips: Workflow")
  #v(1fr)
  #tip-grid(
    ([Edit, save, plot], [
      - No build step of your own: before plotting, `plot` rebuilds and re-runs your program when its sources changed.
      - So the loop is: change #plot-def-file, run the same `plot` command again.
    ]),
    ([Variants of a plot], [
      - Copy a finished plot and change only what differs: #if lang == "py" [`ratioPlot = Plot(plot, "ptSpectrumRatio")`] else [`Plot ratioPlot(plot, "ptSpectrumRatio")`], then e.g. `ratioPlot.SetBasePlot("1d_ratio")`.
      - In a loop this gives a whole family, e.g. one plot per multiplicity class.
    ]),
    ([Animate a series], [
      - Mode `gif` turns all matching plots into one animation, e.g. a spectrum per multiplicity class.
      - `plot spectra/classes .+ gif+80` shows each frame for 0.8 s.
    ]),
    ([Same code on every machine], [
      - Keep machine-specific paths out of the code: `srp set @current dataDir /data/run3` once on each machine.
      - The code reads it with `pm.GetProjectProperty("dataDir")` and builds the input paths from it.
    ]),
    ([Share with ROOT-only colleagues], [
      - `macro` writes a ROOT macro (`.C`) per plot that redraws it without SciRooPlot.
      - `file` collects the canvases in one ROOT file, `data` the input objects of the plots.
    ]),
    ([When a plot looks wrong], [
      - `plot <group> <name> print` prints the full definition of a plot, merged with its base plot.
      - `srp verbosity debug` shows in detail what SciRooPlot reads and does.
    ]),
  )
  #v(1fr)
]

// ============================================================================
// Appendix: complete accessor reference, generated from PlotManager.h / Plot.h / PanelLayout.h
// (C++/Python parity for these signatures is handled by the show rule near
// the top of this file.)
// ============================================================================

#section-slide("Reference", 5, [Every setting on one page per class: look things up while you work.], (
  [find any setter of a class quickly,],
  [see which drawing option alias gives which ROOT option.],
))

#slide[
  #[]<appx-plot>
  #slide-title("Appendix: PlotManager & Plot")
  #grid(
    columns: (49%, 49%),
    gutter: 2%,
    [
      #api-section("PlotManager", (
        [`PlotManager(projectName = "")`], [Construct a manager for a named project.],
        [`AddDataSource(id, files, replace = false)`], [Register files, directories or ROOT objects (also lists, open files) under an ID (`replace`: clear the ID first).],
        [#names("SaveDataSources(file = {})", "LoadDataSources(file = {}, replace = false)")], [Save or reload the registered data sources (default: the project's file).],
        [`AddPlot(plot)` / `AddBasePlot(basePlot)`], [Register a plot, or a reusable base plot layout.],
        [`AddColorOverview(name, group, colors = {})`], [Add a plot showing the ROOT colour wheel, or the given colours with their indices.],
        [#names("SavePlots(name = \".+\", group = \".+\", file = {})", "LoadPlots(name = \".+\", group = \".+\", file = {})")], [Write matching plots to, or load them from, a config file.],
        [`GeneratePlots(mode = "show", name = ".+", group = ".+")`], [Generate plots matching the request in the given mode (groups include their subgroups, `group$` excludes them).],
        [`ListPlots()`], [Print all registered plots.],
        [#names("ClearDataBuffer()", "ClearCanvasRegistry()")], [Free the buffered input data, or clear the cache of already-generated canvases.],
        [`SetOutputDirectory(path)`], [Set the output directory for generated files.],
        [#names("SetPlotsRootFile(fileName)", "SetDataRootFile(fileName)")], [Names of the ROOT files written in the modes `file` and `data` (default: `Plots.root`, `Data.root`).],
        [`GetProjectProperty(property)`], [Read a project variable set via `srp set <project> <property> <value>`.],
        [`SaveProject()`], [Persist the current project to disk.],
      ), text-size: 12pt)
    ],
    [
      #api-section("Plot", (
        [`Plot(name, group = "", basePlot = {})`], [Create a plot in a figure group, optionally from a base plot.],
        [`Plot(other, name, group = {})`], [Copy `other` under a new name (and optionally a new group).],
        [`plot[padID]` / `GetPad(padID)`], [Access a pad by index.],
        [#names("base + plot", "base += plot")], [Put `plot` on top of `base`: its name, group, base plot and every setting it defines win (how base plots are applied).],
        [#names("SetName(name)", "SetGroup(group)", "AppendGroup(subgroup)")], [Rename the plot, reassign its figure group, or append a subgroup path.],
        [`SetBasePlot(name)`], [Apply a base plot layout by name.],
        [#names("SetDimensions(width, height, fixAspectRatio = false)", "SetWidth(width)", "SetHeight(height)", "SetFixAspectRatio(fixAspectRatio = true)")], [Set canvas dimensions (in pixels), all at once or individually.],
        [#names("SetFill(color, style = {}, alpha = {})", "SetFillColor(color)", "SetFillStyle(style)", "SetFillAlpha(alpha)")], [Configure plot background fill, all at once or individually.],
        [`SetTransparent()`], [Make the plot background transparent.],
      ), text-size: 12pt)
    ],
  )
]

#slide[
  #slide-title("Appendix: Base Plots & PanelLayout")
  #grid(
    columns: (49%, 49%),
    gutter: 2%,
    [
      #api-section("Base plots", (
        [`PlotManager::MakeBasePlot(name = "1d")`], [Base plots with consistent layout, named `<type>[_wide|_tall][_<columns>x<rows>][_ratio][_gap]` (type `1d` or `2d`), e.g. `1d_ratio`, `2d`, `1d_3x1`, `1d_2x1_ratio`.],
        [`PlotManager::MakeBasePlot(name, sizes)`], [The same with the lengths of the `PanelLayout` `sizes` (text sizes, line width, margins, ...), equal in all panels.],
        [`Plot(name, layout)`], [Base plot from a `PanelLayout`, its lengths kept equal in every panel.],
        [#names(if lang == "py" { "PanelLayout(rows = [[Panel()]])" } else { "PanelLayout(rows = {{Panel()}})" }, "PanelLayout(sizes, rows)")], [Panels in rows (top to bottom), or the same lengths with other panels.],
        [#names("Panel(width = 1, height = 1)", "Panel().ZAxis()", "Gap()", "Empty()")], [A panel (in units of the standard panel), with room for a colour scale; a gap between independent panels; a cell without axes (a pad for texts).],
      ), text-size: 12pt)
    ],
    [
      #api-section("PanelLayout lengths (pixels of the plot at scale 1)", (
        [#names("SetPanelSize(px)", "SetMargins(top, bottom, left, right)", "SetZAxisMargin(px)", "SetPanelSpacing(px)")], [Standard panel (638), space around the panels (32, 118, 118, 32), for the colour scale (268), between touching panels (0).],
        [#names("SetTextSize(px)", "SetTitleSize(px)", "SetTickLength(px)", "SetLabelOffset(px)", "SetXTitleOffset(px)", "SetYTitleOffset(px)", "SetZTitleOffset(px)", "SetLineWidth(w)", "SetMarkerSize(s)")], [Text (31.52), axis titles (39.4), ticks (19.15), labels from the axis (3.94), titles from the axis (x 63.04, y automatic, z 122.9), ROOT line width (5) and marker size (1.4).],
      ), text-size: 12pt)
    ],
  )
]

#slide[
  #[]<appx-pad>
  #slide-title("Appendix: Pad (1/2) – Adding Content")
  #grid(
    columns: (49%, 49%),
    gutter: 2%,
    [
      #api-section("Accessing pad content", (
        [`pad['X']` / `GetAxis(axis)`], [Access an axis object ('X', 'Y', 'Z').],
        [`pad(dataID)` / `GetData(dataID)`], [Access a `Data` object by index.],
        [`GetLegend(legendID)` / `GetText(textID)`], [Retrieve a previously added legend or text box.],
      ))
      #v(0.6em)
      #api-section("Text, legends & reference lines", (
        [#names("AddText(xPos, yPos, text)", "AddText(placement, text)", "AddText(text)")], [Add a text box at a fixed position (upper left corner, pad coordinates), close to a given corner, or auto-placed.],
        [#names("AddLegend(xPos, yPos, title = {})", "AddLegend(placement, title = {})", "AddLegend(title = {})")], [Add a legend, the same ways.],
        [`SetRefFunc(function)`], [Draw a reference function (e.g. `"1"`) behind the data; returns it for styling.],
      ))
    ],
    [
      #api-section("Adding content", (
        [`AddData(name, dataset, label = {})`], [Add data looked up by name from a registered source.],
        [`AddData(data, label = {})`], [Add a complete `Data` object, e.g. `Data(name, dataSource)` or `layout(name)`.],
        [`AddFunction(function, label = {})`], [Add a `TF1`-style function expression.],
        [`AddPoints(x, y, label = {})`], [Add points from separate X/Y arrays, or (x, y) pairs.],
        [`AddLine(pos1, pos2, label = {})`], [Add a line between two points.],
        [`AddRatio(numerator, denominator, label = {})`], [Add a ratio of two `Data` objects (or `{name, source}`); it takes the appearance of the numerator.],
        [`AddRatio(ratio, label = {})`], [Add a predefined `Ratio(numerator, denominator)`.],
        [`AddRatio(numName, numSource, denomName, denomSource, label = {})`], [Shorthand for a ratio of two named data.],
      ))
    ],
  )
]

#slide[
  #slide-title("Appendix: Pad (2/2) – Position & Palette")
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
        [`SetTransparent()` / `SetFrameTransparent()`], [Make the pad, or its frame, transparent.],
        [#names("SetFrameFill(color, style = {}, alpha = {})", "SetFrameFillColor(color)", "SetFrameFillStyle(style)", "SetFrameFillAlpha(alpha)")], [Configure the frame's fill.],
        [#names("SetFrameBorder(color, style = {}, width = {}, alpha = {})", "SetFrameBorderColor(color)", "SetFrameBorderStyle(style)", "SetFrameBorderWidth(width)", "SetFrameBorderAlpha(alpha)")], [Configure the frame's border.],
      ))
    ],
  )
]

#slide[
  #slide-title("Appendix: Pad Default Styling")
  - These set the fallback style applied to newly added data unless overridden per `Data` object -- see the "Default Styles and Color Palettes" slide for usage.
  #v(0.6em)
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
        [#names("SetDefaultCandleBoxRange(range)", "SetDefaultCandleWhiskerRange(range)")], [Box and whisker range of candle plots.],
      ))
    ],
  )
]

#slide[
  #[]<appx-axis>
  #slide-title("Appendix: Axis")
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
]

#slide[
  #slide-title("Appendix: Ratio")
  - Every `Data` accessor from the next two slides is also available on `Ratio`. Styling and ranges always act on the ratio; data modifiers and data selection follow the active mode.
  #v(0.6em)
  #api-section("Ratio-specific", (
    [`SetIsCorrelated(isCorrelated = true)`], [Treat numerator/denominator as correlated (e.g. one is a sub-sample of the other) -- applies binomial error propagation.],
    [`Numer()` / `Denom()` / `Both()`], [Following modifiers act on the numerator, the denominator, or both, before dividing. Data selection (`Project*`, `Define`, `Filter`, ...) follows the same mode.],
    [`Result()`], [Following modifiers act on the ratio itself (default). Data selection cannot act on the ratio: it is interpreted as `Numer()` with a warning.],
    [`AsRatio()`], [Called on data obtained via `pad(n)` / `GetData(n)`: access it as `Ratio`, e.g. `pad(2).AsRatio().SetIsCorrelated()`.],
  ))
]

#slide[
  #[]<appx-data>
  #slide-title("Appendix: Data (1/2) – Source, Range & Appearance")
  #grid(
    columns: (49%, 49%),
    gutter: 2%,
    [
      #api-section("Source, range & legend", (
        [`template(name, dataSource = {})`], [Use a `Data` object as template: a copy with the given name (and data source).],
        [`SetDataSource(dataset)`], [Set/override the data-source ID.],
        [`SetLayout(layout)` / `ApplyLayout(layout)`], [Take over the appearance of a `Data` layout (options, ranges, marker, line, fill, contours), or only the parts set in it.],
        [#names("SetRangeX(min, max) / SetRangeY(min, max)", "SetMinRangeX(min) / SetMaxRangeX(max)", "SetMinRangeY(min) / SetMaxRangeY(max)")], [Restrict the range in which the data is drawn, fully or partially.],
        [`SetRangeX()` / `SetRangeY()`], [Remove a previously set range restriction.],
        [`SetScaleMinimum(factor)` / `SetScaleMaximum(factor)`], [Stretch the auto-computed draw range by a factor, without touching the data.],
        [`SetShowOverflowBins(showOverflowBins = true)`], [Include overflow bins in the display.],
        [`SetLegendLabel(label)` / `SetLegend(legendID)`], [Set legend text, or assign to a specific legend.],
        [`SetLegendEntry(entry)`], [Appearance of the data's legend row, e.g. `LegendEntry().SetSymbolElements("F")`.],
        [#names("SetDrawOption(option) / SetDrawOption(alias)", "SetDrawOption()")], [Set a ROOT draw-option string or one of the #link(<appx-drawing-options>)[#text(fill: main-color)[predefined aliases]], or remove it.],
        [`SetTextFormat(fmt)`], [Number format of bin contents drawn with the `TEXT` option, e.g. `".0f"`.],
      ), text-size: 12pt)
    ],
    [
      #api-section("Appearance & statistics box", (
        [`SetColor(color)` / `SetAlpha(alpha)`], [Set (or fade) marker, line and fill colour together.],
        [#names("SetMarker(color, style, size, alpha = {})", "SetMarkerColor(color)", "SetMarkerStyle(style)", "SetMarkerSize(size)", "SetMarkerAlpha(alpha)")], [Marker appearance, all at once or individually.],
        [#names("SetLine(color, style, width, alpha = {})", "SetLineColor(color)", "SetLineStyle(style)", "SetLineWidth(width)", "SetLineAlpha(alpha)")], [Line appearance, all at once or individually.],
        [#names("SetFill(color, style, alpha = {})", "SetFillColor(color)", "SetFillStyle(style)", "SetFillAlpha(alpha)")], [Fill appearance, all at once or individually.],
        [`SetKeepInputStyle()`], [Keep the input's line, marker and fill.],
        [`SetDefinesFrame(dontDraw = false)`], [Use this object to define the plot frame (`dontDraw`: without drawing it).],
        [`SetContours(contours)` / `SetContours(nContours)`], [Set explicit contour levels or their count.],
        [`SetShowStats(format = "emr", title = {})`], [Statistics box of a histogram: ROOT's letters (`TStyle::SetOptStat`, e.g. `"eMR"`), the title as its first line.],
        [`SetStatsBox(statsBox)`], [Placement and appearance of the statistics box: a `StatsBox()` with the setters of `Box`.],
      ), text-size: 12pt)
    ],
  )
]

#slide[
  #slide-title("Appendix: Data (2/2) – Modifiers & Processing")
  #grid(
    columns: (49%, 49%),
    gutter: 2%,
    [
      #api-section("Transform existing data", (
        [`Normalize(isDensity = false, min, max)` / `NormalizeToMaximum(min, max)`], [Normalize the integral or the maximum to one, optionally within an x range (`isDensity`: the data already is a density).],
        [`Scale(scaleFactor)`], [Scale the object's contents.],
        [`DivideBinWidth(divideBinWidth = true)`], [Divide contents by bin width.],
        [`Density()`], [Divide by bin width and normalize the integral to one: a probability density from counts.],
        [`Cumulative(forward = true)`], [Replace with the running sum. 1D histograms only.],
        [`RebinX(n)` / `RebinY(n)` / `RebinZ(n)`], [Rebin one axis. Histograms only.],
        [`RebinXY(nx, ny)` / `RebinXYZ(nx, ny, nz)`], [Rebin two or three axes at once.],
        [`Smooth(nIterSmooth = 1)`], [Apply smoothing.],
        [`ScaleX(factor)` / `ScaleY(factor)` / `ScaleZ(factor)`], [Rescale an axis itself (e.g. a unit conversion) -- unlike `Scale()`, which rescales the values.],
      ), text-size: 12pt)
      #v(0.6em)
      #api-section("Projections & profiles", (
        [`Project(dims, ranges = {}, isUserCoord = {})`], [Generic N-dimensional projection.],
        [`ProjectX(...)` / `ProjectY(...)`], [Standard projection of a 2D histogram.],
        [`Profile(dims, ranges = {}, isUserCoord = {})`], [Generic multi-dimensional profile.],
        [`ProfileX(...)` / `ProfileY(...)`], [Standard profile of a 2D histogram.],
      ), text-size: 12pt)
    ],
    [
      #api-section("Tree & table data", (
        [`Project1D(x, weight = {})` / `Project2D(x, y, weight = {})`], [1D/2D histogram from a column; `x` accepts a name, #if lang == "py" [`(name, nBins)`, `(name, nBins, (min, max))`, or `(name, edges)`] else [`{name, nBins}`, `{name, nBins, {min, max}}`, or `{name, edges}`].],
        [`Profile1D(x, profile, weight = {})` / `Profile2D(x, y, profile, weight = {})`], [1D/2D profile of one column over one or two others.],
        [`Scatter(x, y)`], [Scatter plot from two columns; expressions like `"sqrt(x*x+y*y)"` are valid.],
        [`Scatter(x, y, xErr, yErr)`], [Scatter plot with symmetric errors.],
        [`Scatter(x, y, xLo, xHi, yLo, yHi)`], [Scatter plot with asymmetric errors.],
        [`Define(key, value)`], [Define a derived variable for use in expressions.],
        [`Filter(filter)`], [Apply a row-selection expression, e.g. `"eta > 0"`. Chainable.],
        [`Join(tree, keys, alias)`], [Add the columns of another tree, e.g. `Join("scores")` row by row or #if lang == "py" [`Join(["events", "src"], ["run", "event"])`] else [`Join({"events", "src"}, {"run", "event"})`] by key (rows without a match are skipped); columns as `"scores.col"`. Chainable.],
        [`Entries(n)` / `Entries(min, max)`], [Limit processing to the first N entries or a range.],
      ), text-size: 12pt)
    ],
  )
]

#slide[
  #slide-title("Appendix: Text Boxes & Legends")
  #grid(
    columns: (49%, 49%),
    gutter: 2%,
    [
      #api-section("Box (shared by TextBox, LegendBox and StatsBox)", (
        [`SetPosition(x, y)` / `SetSize(w, h)`], [Position of the upper left corner, or dimensions of the box.],
        [`SetAutoPlacement(placement = best_corner)`], [Let SciRooPlot place the box: as close as possible to the corner of the frame where it fits best, or to a given one (`top_left`, `top_right`, `bottom_right`, `bottom_left`).],
        [`SetUserCoordinates(isUserCoord = true)`], [Interpret position/size as user (data) coordinates instead of relative pad coordinates.],
        [#names("SetBorder(color, style, width, alpha = {})", "SetBorderColor(color)", "SetBorderStyle(style)", "SetBorderWidth(width)", "SetBorderAlpha(alpha)")], [Border, all at once or individually.],
        [#names("SetText(color, font, size, alpha = {})", "SetTextColor(color)", "SetTextFont(font)", "SetTextSize(size)", "SetTextAlpha(alpha)")], [Text styling, all at once or individually.],
        [#names("SetFill(color, style, alpha = {})", "SetFillColor(color)", "SetFillStyle(style)", "SetFillAlpha(alpha)")], [Background fill, all at once or individually.],
        [`SetTransparent()` / `SetNoBox()`], [Transparent background, or remove border and fill.],
        [`SetMargin(m)` / `SetLineSpacing(s)`], [Internal margin, or spacing between lines/entries.],
      ), col-widths: (52%, 48%), text-size: 12pt)
    ],
    [
      #api-section("TextBox", (
        [`SetText(text)`], [Set the displayed text (a list of lines, or one string with newlines).],
        [`SetTextAlign(align)`], [ROOT alignment code, e.g. `kHAlignCenter + kVAlignTop` (default `kHAlignLeft + kVAlignCenter`).],
      ), col-widths: (52%, 48%), text-size: 12pt)
      #v(0.6em)
      #api-section("LegendBox", (
        [`SetTitle(title)` / `SetNumColumns(n)`], [Legend title, or number of columns.],
        [`AddEntry(label = "")`], [Add a row of its own (no label: an empty line), see the next slide.],
        [`SetSymbolColScale(scale)`], [Scale the width of the symbol column.],
      ), col-widths: (52%, 48%), text-size: 12pt)
      #v(0.6em)
      #api-section("LegendEntry: content and place", (
        [`SetLabel(label)` / `SetSymbolElements(elements)`], [Label text, or symbol elements of this entry (e.g. `"L"`, `"EP"`, `"F"`).],
        [`SetRefData(refDataID)`], [Take the style from data number `refDataID` of the pad.],
        [`AtTop()` / `BeforeData(n)` / `AfterData(n)`], [Place a row added with `AddEntry`.],
      ), col-widths: (52%, 48%), text-size: 12pt)
      #if lang == "py" [
        #v(0.4em)
        #text(size: 12pt)[In Python, boxes are created via `pad.AddText()` and `pad.AddLegend()`.]
      ]
    ],
  )
]

#slide[
  #slide-title("Appendix: Legend Entries")
  - Legend entries take their marker, line and fill style from the data they belong to. Settings for all entries of a `LegendBox` override this, and settings of a single `LegendEntry` override both.
  - Rows added with `AddEntry` come after the rows of the data, or at the place given relative to the data of the pad (numbered as in `plot[1](n)`), whether or not that data has a row in this legend.
  #v(0.6em)
  #grid(
    columns: (49%, 49%),
    gutter: 2%,
    [
      #api-section("LegendBox: all entries", (
        [`SetDefaultSymbolElements(elements)`], [Symbol elements of entries without their own, e.g. `"L"`, `"EP"`, `"F"` (default: chosen from the data).],
        [`SetDefaultColor(color)` / `SetDefaultAlpha(alpha)`], [Marker, line and fill colour (or transparency) together.],
        [#names("SetDefaultMarkerColor(color)", "SetDefaultMarkerAlpha(alpha)", "SetDefaultMarkerStyle(style)", "SetDefaultMarkerSize(size)")], [Marker appearance.],
        [#names("SetDefaultLineColor(color)", "SetDefaultLineAlpha(alpha)", "SetDefaultLineStyle(style)", "SetDefaultLineWidth(width)")], [Line appearance.],
        [#names("SetDefaultFillColor(color)", "SetDefaultFillAlpha(alpha)", "SetDefaultFillStyle(style)")], [Fill appearance.],
      ), col-widths: (52%, 48%), text-size: 12pt)
    ],
    [
      #api-section("LegendEntry: appearance", (
        [`SetColor(color)` / `SetAlpha(alpha)`], [Marker, line and fill colour (or transparency) together.],
        [#names("SetMarker(color, style, size)", "SetLine(color, style, width)", "SetFill(color, style)")], [Marker, line or fill at once (optional last argument: alpha).],
        [#names("SetMarkerColor(color)", "SetMarkerAlpha(alpha)", "SetMarkerStyle(style)", "SetMarkerSize(size)")], [Marker appearance.],
        [#names("SetLineColor(color)", "SetLineAlpha(alpha)", "SetLineStyle(style)", "SetLineWidth(width)")], [Line appearance.],
        [#names("SetFillColor(color)", "SetFillAlpha(alpha)", "SetFillStyle(style)")], [Fill appearance.],
        [#names("SetTextColor(color)", "SetTextAlpha(alpha)", "SetTextFont(font)", "SetTextSize(size)")], [Label text appearance.],
      ), col-widths: (52%, 48%), text-size: 12pt)
    ],
  )
]

#slide[
  #[]<appx-drawing-options>
  #slide-title("Appendix: Drawing Options – Histograms & Graphs")
  - Pass any alias below to `SetDrawOption()` instead of a raw ROOT draw-option string -- the table shows the actual ROOT string it expands to for each data kind. Without an option: `hist` with a fill (graphs: `area`), `points` with a marker set or errors, else `hist` (graphs: `line`); 2D: `colz`.
  - The base word names the appearance, suffixes add one feature: `_xerr` also draws the x errors, `_endcaps` draws end caps, `_text` writes the values, `_arrows` ends the error bars with arrows (limits), `_smooth` smooths, `_nomarkers` omits the markers, `_clipped` cuts the line at the frame instead of drawing it along the frame edge. Every alias works for both kinds: where ROOT has no option for one of them, the data is converted and drawn as the other.
  #v(0.4em)
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
      fill: (x, y) => if calc.rem(y, 2) == 0 { tint } else { white },
      [*Alias*], [*Histograms*], [*Graphs*], [*Alias*], [*Histograms*], [*Graphs*],
      [`points`], [`X0 E P`], [`P Z` #text(size: 9.5pt, fill: gray)[(x errors removed)]], [`area`], [`HIST LF2`], [`LF` #text(size: 9.5pt, fill: gray)[(polygon to zero)]],
      [`points_xerr`], [`E P`], [`P Z`], [`area_smooth`], [`HIST CF`], [as histogram],
      [`points_endcaps`], [`X0 E1 P`], [`P` #text(size: 9.5pt, fill: gray)[(x errors removed)]], [`band`], [`E5`], [`3`],
      [`points_xerr_endcaps`], [`E1 P`], [`P`], [`band_smooth`], [`E6`], [`4`],
      [`points_line`], [as graph], [`P Z L` #text(size: 9.5pt, fill: gray)[(x errors removed)]], [`boxes`], [`E2`], [`P2`],
      [`points_text`], [`X0 E P` #text(size: 9.5pt, fill: gray)[(+ values)]], [`P Z` #text(size: 9.5pt, fill: gray)[(+ values)]], [`boxes_nomarkers`], [`E2` #text(size: 9.5pt, fill: gray)[(markers removed)]], [`2`],
      [`points_arrows`], [as graph], [`P Z |>` #text(size: 9.5pt, fill: gray)[(x errors removed)]], [`brackets`], [as graph], [`[]`],
      [`line`], [`HIST L` #text(size: 9.5pt, fill: gray)[(no fill)]], [`X L`], [`text`], [`HIST TEXT`], [as histogram],
      [`curve`], [`HIST C` #text(size: 9.5pt, fill: gray)[(no fill)]], [`X C`], [`curve_clipped`], [as graph #text(size: 9.5pt, fill: gray)[(cut at frame)]], [`X C`],
      [`hist`], [`HIST`], [as histogram], [`hist_clipped`], [as graph #text(size: 9.5pt, fill: gray)[(cut at frame)]], [as histogram],
      [`hist_open`], [`HIST ][`], [as histogram], [`hist_open_clipped`], [as graph #text(size: 9.5pt, fill: gray)[(cut at frame)]], [as histogram],
      [`bars`], [as graph], [`X B`], [], [], [],
      [`hbars`], [`HIST HBAR`], [as histogram], [], [], [],
    )
  ]
]

#slide[
  #slide-title("Appendix: Drawing Options – 2D Histograms")
  - These aliases apply to 2D data (`TH2`-like histograms): colour maps, surfaces, contours, legos, and candle plots.
  - The base word is the plain style, `_col` colours the lines by the contents, `_fill` fills by the contents, a trailing `z` also draws the colour axis.
  #v(0.4em)
  #grid(
    columns: (49%, 49%),
    gutter: 2%,
    [
      #api-table((
        [`col` / `colz`], [`COL` / `COLZ`],
        [`box`], [`BOX`],
        [`text`], [`TEXT`],
        [`lego`], [`LEGO1 0`],
        [`lego_col` / `lego_colz`], [`LEGO2 0` / `LEGO2Z 0`],
        [`lego_noborders`], [`LEGO3 0`],
        [`surf`], [`SURF`],
        [`surf_col` / `surf_colz`], [`SURF1` / `SURF1Z`],
        [`surf_fill` / `surf_fillz`], [`SURF2` / `SURF2Z`],
        [`surf_contours`], [`SURF3`],
        [`surf_shaded`], [`SURF4`],
      ), col-widths: (44%, 56%), text-size: 13pt)
    ],
    [
      #api-table((
        [`cont`], [`CONT3`],
        [`cont_col` / `cont_colz`], [`CONT1` / `CONT1Z`],
        [`cont_fill` / `cont_fillz`], [`CONT0` / `CONTZ`],
        [`cont_smooth` / `cont_smoothz`], [`CONT4` / `CONT4Z`],
        [`candle`], [`CANDLEX2` #text(size: 10pt, fill: gray)[(whiskers 1.5 IQR, outliers)]],
        [`candle_minmax`], [`CANDLEX1` #text(size: 10pt, fill: gray)[(whiskers to extremes, mean)]],
        [`candle_mean` / `candle_notched`], [`CANDLEX3` / `CANDLEX4`],
        [`candle_points` / `candle_scatter`], [`CANDLEX5` / `CANDLEX6`],
        [`candle_meanline`], [`CANDLEX(111101)` #text(size: 10pt, fill: gray)[(mean line instead of median)]],
        [`violin` / `violin_minmax`], [`VIOLINX2` / `VIOLINX1`],
      ), col-widths: (44%, 56%), text-size: 13pt)
    ],
  )
]
