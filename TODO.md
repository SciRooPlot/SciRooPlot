Autocomplete:
- category search?
- how to handle multiple figureGroups?
- how to handle multiple xml config files?

Bugs:
- histograms without error can be drawn only as hist??
- if same histogram is added to plot twice, the name is no longer unique and this results in unwanted behaviour for ref data specified in legend entries

Missing Box Features
- make no box default setting
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

Planned Features:
- add setting for nContours in 2d plots
- add option in main program to list all loaded plots
- possibility to do fast loading of all data from input files (skip chek if data was requested)
- give warning if selected drawing alias not available for this type of data
- decide where data manipulation part should be put (before or after setting ranges?)

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
- for 2d plots SAME0 can be used to avoid axis range change when drawing on top
- is it possible to set the order in multi column tlegends? left-right vs top-bottom
- maybe at some point it could become useful to also copy data and boxes from template
- maybe generalized version of white patch hiding the truncated zero in ratio pads would be nice
- should smoothing be done before or after division for ratios (or both)??

Structural:
- mark all getters as const
- use constexpr everywhere!!
- use str_contians() everywhere
- put data pointers in vector instead of map and use index for all comparisons
- think about how things can be implemented in a more general way as templates
- change arguments referring to internal variables to const ref if possible to be more memory efficient, use lambdas
- fully grasp the difference between emplace_back and push_back in terms of copy/move: why does push_back not work for variants?
- remove puts empty items to end and gives new end of vector! -> avoid useless erase calls per item
- can multi-threading be used somewhere?
- capture via const ref: [&best_string = std::as_const(best_string)]
- use for_each() instead of range based loops
- use size_t in the loops
- use brace-or-equal initializers in class definitions
- what about std::format?? is it the same as fmt? can dependency be dropped?
