Analysis open questions:
- does multPtUnfoldedNormalized take into account bin width?
- put proper axis titles already when data is created to avoid having to do this in plotting
- what are multPtMeasured/raw exactly?
- what are the actual z ranges of 2d spectra? be sure not to cut into distributions via setzaxis (different per dataset!)
- naming of histograms urgendly needs re-work! (e.g. the matrices!)
- resolution histogram range is smaller than data range for sigma
- event efficiency in 2TeV never goes to 1!!
- secondary contamination in 7TeV is strange at high pt
- secondary contamination vs mult has entry at nch = 0 ->makes sense because all we measure there is not primary (?)
- strange rising trend with nch in contamination for larger systems??
- why efficiency goes down vs mult in large systems? (occupancy)
- need strategy regarding qa -> optimize CutStudiesTask to suit my needs and run it for each dataset!
- pPb 5TeV data / mc is crap! -> use 2016 Dataset!!
- what is the problem with closure_multDistFromCorrelMat???
- particleClosure counting does not take into account bin width -> shifted in larger systems
- what are these event efficiencies? why so strange, how to properly normalize?
- properly understand! efficiencies -> read again what others did!
- signal loss?, INEL0?
- why are there entries below .15 TeV??
- something is severly wrong with MC truth pt distribution in 7TeV and 13 TeV and PbPb5 and xexe!!!!
- projection on mult of closure tests does not account for bin widths in larger systems -> also for the pt,mult bins
- 2TeV has no chi2 output included!
- in PbPb systematics are zero at mult <3500
- fix multdist multdensity confusion! be sure how to normalize!
- propagate somehow meta-info about number of events contained in result
- prepare QA-Task and corresponding default plots! (data / mc) -> make this flexible enough to have more than 1 dimensions (that are then filtered in intermediate step) -> centrality, mult, pt might be needed
- info about runs used + respective number of events! (must be in standard task)
- inclusive spectrum does not look close to publication!
- exactly understand normation!! be very sure to include all necessary histograms in analysis task

- extract published spectra! -> kno + mult vs cent
- fix colors and linestyles to be unique and final -> make a table for the schema!
- add qa plots per dataset (summary qa plots containing multiple datasets?)
- find proper naming scheme for plots
- I need the new framework asap to work with inputs I have and returning output in correct format!

- make the KNO things graphs instead of histograms
--------
- store box defaults (no border, transparency, etc in pad) -> maybe add default-Box so we have all settings?
- default legend and default textbox?
- deeply look into regexp part
- add option in main program to list all loaded plots -> plot list
- fix inconsistencies in naming of variables
- final streamlining / optimizing of internal data reading / buffering
- projection feature! -> closure stuff easier 2d->1d
- possibility to read / write multiple files
- file names w/o ending!
- give warning if selected drawing alias not available for this type of data
- add xml examples to readme
- why does ratio no longer put "ratio" as y axis title? -> graphs does no division -> put some printout

- graphs with asym errors are not supported by any of the division functions
- division graph/hist not implemented
- the whole Painter::GeneratePlots is just a big ugly mess!

Autocomplete:
- category search?
- how to handle multiple figureGroups?
- how to handle multiple xml config files?
- regexp matching used in group can be applied also in search! -> then show only once still

- make example that works out of the box

Bugs:
- decide where data manipulation part should be put (before or after setting ranges?)
- put using... to avoid lengthly if constexpr
- make figureCategory optional
- it is stupid to have figureGroup and category stored twice in the xml
- plot name must not contain /
- automatic placing in combination with log does not seem to work (wrong order of things?)
- histograms without error can be drawn only as hist??
- if same histogram is added to plot twice, the name is no longer unique and this results in unwanted behaviour for ref data specified in legend entries
- csv w/o options -> invisible -> TGrapherrors has wrong defaults
- ScaleGraph does not scale errors


Missing Box Features
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

Structural:
- how to handle boolean things? do they also belong in optionals?
- re-check which of the properties that are always stored are actually needed always
- enhance speed wherever possible!
- can callbacks / lambdas be used somewhere?
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
- use brace-or-equal initializers in class definitions and get rid of initializer lists
