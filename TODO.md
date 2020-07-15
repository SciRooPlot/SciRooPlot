ANALYSIS:
---------
- straighten up Fit, Publication input names
- categorize plots
- add data specific colors and markers
- clean up file names and make useful sub-folders
- add more qa plots for unfolding
- finally fix the naming of the output -> unfoldedMeanPt, etc or use subfolders: generated/meanPt, unfolded/meanPt, generatedMC/meanPt?
- mc lines instead of markers

FRAMEWORK:
----------

WIP:
- option to set predefined data layout via struct or such
- allow for multi-column text boxes and make tab width between them a text box property
- add setters and getters to boxes + fix constructor
- gStyle->SetPaintTextFormat("4.1f m");
- for 2d -> use SAME0 to avoid axis range change when drawing on top?
- xErrors!! toggle on and off?

Minor Features:
- mabye it could even be nice for the user to specify which default color / marker ID to use?? -> feels like a hack...
- add nContours as option to pad
- normalize and scale should use same function
- use same as previous color/marker/style feature (color = -1)?
- option in main program to list loaded plots
- possibility to load all data from input files
- is it possible to set the order in multi column tlegends? left-right vs top-bottom
- possibility to split the legend?
- think about more placeholders for legends and texts and how to format them
- fix overlap between axis title and tick marks if they have too many digits
- generalized version of white patch hiding the truncated zero
- should smoothing be before or after doing ratio (or both)??

Feature ideas:
- find a general way to automatically determine optimal axis offsets (in particular for 2d and 3d views) (what unit is title offset?)
- AddFunction() and AddLine() accessor so one can directly plot simple functions
- revisit idea of axis linking
- can exponents on x axis be moved down, so they do not interfere with col z axis? (property of GAxis?)
- maybe at some point it could become useful to also copy data and boxes from template?
- add root style, color, marker, linestyle pictures to documentation
- support csv format (flexible string and delimiter)
- functions and fitting
- shape objects (arrows)
- support for input types T{H,F}3, THn{Sparse} (->Projection functionality), THStack, TMultiGraph
- support for setting up TView to have a 3d representations of 2d data
- add angle to textboxes
- maybe define 'top left' 'bottom right' etc default positons for boxes in general manner? (flexible minimal distance to ticks)


Structural changes:
- fix header inclusion and be sure to not pollute global namespace
- use constexpr everywhere!!
- understand why move constructors implicitly delete assignment operators
- move analysis stuff out of the repository and make external repository (link cmakes?)
- use str_contians() everywhere
- always use smallest possible datatype whenever possible
- put data pointers in vector instead of map and use index for all comparisons
- think about how things can be implemented in a more general way as templates
- change arguments referring to internal variables to const ref if possible to be more memory efficient, use lambdas
- add (copy-, move-) constructors and destructors; make sure all variables are initialized properly
