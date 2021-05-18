Todo:
- add xml examples to readme
- illustrate projection feature in readme
- add possibility to read / write multiple files
- always call sumw2 before drawing histograms as points
- propagate color gradient settings from user interface

Box Features:
- add default-boxes (legend and text) that will be baseline for all boxes of this pad
- default positions? , topLeft, BottomRight
- box auto-size must consider that legend entries can have different text sizes
- text lines maybe also configurable separately?
- flexible width of marker part
- flexible deliminter for text lines
- add flexible margins for the boxes
- check again if auto placement works correct (also with multiple boxes)
- add some top right etc position (enum) functionality with modifyable distance to axis
- ability to set default box properties in pad
- possibility to add full layout of box
- add title feature to legends again
- AddLine function for texts?
- might be useful to specify size of boxes

Feature Ideas:
- one would probably want to be able to place legends and texts also in a different pad (or anywhere on the canvas)
- add support for add/subtract + pulls? / generic binary operator thing
- generic algorithm to automatically determine optimal axis offsets + avoid overlap with lables
- revisit idea of axis linking between pads
- add root style, color, marker, linestyle pictures to documentation
- fully support csv format (flexible string and delimiter)
- add user functions and fitting (AddFunction() and AddLine())
- add shape objects (arrows)
- support THStack, TMultiGraph
- support for setting up TView to have a 3d representations of 2d data
- add inclination angle setting to textboxes
- add 'same as previous' color/marker/style feature (color = -1)?
- allow for multi-column text boxes and make tab width between them a text box property
- customizable legends that can be added independent of data in plot
- option to draw help-grid on top of the plot to indicate NDC coordinates
- support TGraphAsymmErrors and check if TProfile{1,2,3}D would require some extra functionality
- double click on plot -> check if it touches a box and if yes print out the coordinates

Notes:
- is it possible to set the order in multi column tlegends? left-right vs top-bottom
- maybe at some point it could become useful to also copy data and boxes from template
- maybe generalized version of white patch hiding the truncated zero in ratio pads would be nice
- should smoothing be done before or after division for ratios (or both)??

Autocomplete:
- can we implement category search as well?
- how to handle multiple figureGroups and still give useful options?
- regexp matching used in group could be applied also in search! -> this would be a really big effort...

Structural:
- the whole Painter::GeneratePlots() function is just a big mess and needs major rework!
- decide where data manipulation part should be put (before or after setting ranges?)
- put using... to avoid lengthly if constexpr
- make figureCategory optional
- fix inconsistencies in naming of variables
- how to handle boolean variables? should they also be wrapped in optionals?
- re-check which of the properties that are always stored are actually required to be non-optional
- use str_contians() more
- use brace-or-equal initializers in class definitions and get rid of initializer lists

Bugs:
- catch 'cannot open file' exceptions from boost ptree
- can we avoid storing figureGroup and figureCategory stored twice in the xml?
- plot name must not contain / -> add savety mechanism
- automatic placing in combination with log does not seem to work simetimes (??)
- if same histogram is added to plot twice, the name is no longer unique and this results in unwanted behaviour for ref data specified in legend entries
- csv w/o options -> invisible -> TGraphErrors has wrong defaults
- give warning if selected drawing alias not available for this type of data
- division graph/hist, hist/graph not implemented yet -> put some printout to inform
- graphs with asym errors are not supported by any of the division functions
- with graph as numerator in ratio there is some logic problem that axis histo is not there -> seg fault
- could not find enough space to place the legend properly -> should say 'text' for text boxes...
