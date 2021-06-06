Todo:
- add possibility to read / write multiple plot definition and input config files in app (config of config files??)
- give warning if selected drawing alias not available for present type of data
- re-check which of the properties that are always stored are actually required to be non-optional
- check again if box auto placement works correctly (also with multiple boxes)
- check if box auto-size determination considers that legend entries may have different text sizes
- add again the possiblity to display a legend title
- propagate pad default text sizes to the boxes
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
- maybe at some point it could become useful to also copy data and boxes from template
- the whole Painter::GeneratePlots() function is just a big mess and needs major rework!
- decide where data manipulation part should be put (before or after setting ranges?)
- should smoothing be done before or after division for ratios (or both)??
- put using... to avoid lengthly if constexpr
- fix inconsistencies in naming of variables

Bugs:
- should one always call Sumw2() before drawing histograms as points?
- if same histogram is added to plot twice, the name is no longer unique and this results in unwanted behavior for ref data specified in legend entries
- csv w/o options -> invisible -> TGraphErrors has wrong defaults
- division graph/hist, hist/graph not implemented yet -> put some printout to inform
- graphs with asym errors are not supported by any of the division functions
- with graph as numerator in ratio there is some logic problem that axis histo is not there -> seg fault

Not so urgend:
- add shape objects (arrows)
- support THStack, TMultiGraph
- support for setting up TView to have a 3d representations of 2d data
- add inclination angle setting to text-boxes
- support TGraphAsymmErrors and check if TProfile{1,2,3}D would require some extra functionality
