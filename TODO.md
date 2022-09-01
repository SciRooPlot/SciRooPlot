Todo:
- example should provide / be set up to some good default settings
- installation to arbitrary location should be possible (do not assume /build in the scripts / cmake files)
- in cmake use find_package doxygen and then create documentation automatically
- package registration via cmake: export(PACKAGE PlottingFramework)
- check if box auto-size determination considers that legend entries may have different text sizes
- add again the possiblity to display a legend title
- make the LegendEntry digest a DataLayout to simplify adding legend properties in a different pad

Feature ideas:
- add default-boxes (legend and text) that will be baseline for all boxes of this pad
- add default box positions like topLeft, bottomRight (enum) with modifiable distance to axis
- add flexible margins for the boxes
- make width of marker column in legend box user definable
- maybe it could be useful to also have fixed size boxes
- generic algorithm to automatically determine optimal axis offsets + avoid overlap with labels
- fully support csv format (flexible string and delimiter)
- add user functions and maybe fitting (AddFunction() and AddLine())

Structural considerations:
- the whole Painter::GeneratePlots() function is just a big mess and needs major rework!
- decide where data manipulation part should be put (before or after setting ranges?)
- should smoothing be done before or after division for ratios (or both)??

Bugs:
- SetOption(string) does not override SetOption(defaultOption)
- if same histogram is added to plot twice, the name is no longer unique and this results in unwanted behavior for ref data specified in legend entries
- csv w/o options -> invisible -> TGraphErrors has wrong defaults

Not so urgend:
- support multiple plot definition and input files per plot-config
- allow SetRange to also specify bin numbers instead of user coordinates
- add shape objects (lines, arrows, boxes, ellipses, crowns) - below or above the data?
- support THStack, TMultiGraph
- support for setting up TView to have a 3d representation of 2d data
- add inclination angle setting to text-boxes
- support TGraphAsymmErrors
- add division hist/graph, graph/hist
