- add xml examples to readme
- possibility to read / write multiple files
- digest file names w/o ending?
- catch cannot open file exceptions from boost ptree

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
- add support for add/subtract + pulls? / generic binary operator thing
- generic algorithm to automatically determine optimal axis offsets + avoid overlap with lables
- revisit idea of axis linking between pads
- add root style, color, marker, linestyle pictures to documentation
- fully support csv format (flexible string and delimiter)
- add user functions and fitting (AddFunction() and AddLine())
- add shape objects (arrows)
- support for input types T{H,F}3, THn{Sparse} (->Projection functionality), THStack, TMultiGraph
- support for setting up TView to have a 3d representations of 2d data
- add inclination angle setting to textboxes
- add 'same as previous' color/marker/style feature (color = -1)?
- allow for multi-column text boxes and make tab width between them a text box property
- customizable legends that can be added independent of data in plot
- option to draw help-grid on top of the plot to indicate NDC coordinates

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
- the whole Painter::GeneratePlots() function is just a big ugly mess and needs major rework!
- decide where data manipulation part should be put (before or after setting ranges?)
- put using... to avoid lengthly if constexpr
- make figureCategory optional
- fix inconsistencies in naming of variables
- how to handle boolean things? do they also belong in optionals?
- re-check which of the properties that are always stored are actually needed always
- enhance speed wherever possible!
- can callbacks / lambdas be used somewhere?
- use constexpr everywhere!!
- use str_contians() everywhere
- think about how things can be implemented in a more general way as templates
- change arguments referring to internal variables to const ref if possible to be more memory efficient, use lambdas
- fully grasp the difference between emplace_back and push_back in terms of copy/move: why does push_back not work for variants?
- remove puts empty items to end and gives new end of vector! -> avoid useless erase calls per item
- can multi-threading be used somewhere?
- capture via const ref: [&best_string = std::as_const(best_string)]
- use for_each() instead of range based loops
- use size_t in the loops
- use brace-or-equal initializers in class definitions and get rid of initializer lists

Bugs:
- can we avoid storing figureGroup and figureCategory stored twice in the xml?
- plot name must not contain / -> add savety mechanism
- automatic placing in combination with log does not seem to work simetimes (??)
- histograms without error can be drawn only as hist??
- if same histogram is added to plot twice, the name is no longer unique and this results in unwanted behaviour for ref data specified in legend entries
- csv w/o options -> invisible -> TGrapherrors has wrong defaults
- ScaleGraph does not scale errors yet
- give warning if selected drawing alias not available for this type of data
- division graph/hist, hist/graph not implemented yet -> put some printout to inform
- graphs with asym errors are not supported by any of the division functions
- with graph as numerator in ratio there is some logic problem that axis histo is not there -> seg fault


