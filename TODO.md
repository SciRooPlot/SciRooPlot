ANALYSIS:
---------
- straighten up Fit, Publication input names
- categorize plots
- add data specific colors and markers
- clean up file names and make useful sub-folders
- add more qa plots for unfolding
- finally fix the naming of the output -> unfoldedMeanPt, etc or use subfolders: generated/meanPt, unfolded/meanPt, generatedMC/meanPt?
- mc linine statt punkten

FRAMEWORK:
----------
WIP:
- possibility to set alias for axis e.g. "ratio"?
- how to define default location for AddRatio() in the style?
- or: change interface overall, such that this is obsolete
- generalize axis linking
- add support for grid styles
- flexible number of color steps in 2d plots
- add graph division feature with tspline3
- font color should also be user definable


Major Features:
- merge markerstyle and linestyle?
- operator overloading for plots (+, <<, *)?
- time axis opitons?
- possibility to dump and load plot styles (manager should only read in required plot styles)
- make sure PlotStyle names are unique
- add some more formatting features for legend (floating point precision, exponent..)
- add constructor to plotStyle that already gives useful default markers, colors, 2d style, etc
- be clear about coordinate systems (fix relative positioning, maybe)
- find a general way to automatically determine optimal axis offsets (in particular for 2d and 3d views) (what unit is title offset?)
- it should be possible to conveniently pipe all root plotting functionality (drawing properties) to the plot
- generalize legend function to handle text boxes in a similar manner
- pads must inherit plot properties (text size et al unless specified otherwise)
- text size and style settings should by default be inherited by each object with text but there should be a possiblity to override

Minor Features:
- how to handle root and custom drawing options (check for allowed options?)
- think about if order of objects is plotted correctly (why is frame on top?)
- change from int to short wherever possible (colors, markers, etc)
- option in main program to list loaded plots
- possibility to load all data from input files
- setter for csv format string and delimiter
- would it be possible to define 'top left' 'bottom right' etc default positons for boxes in general manner? maybe with flexible minimal distance to ticks
- is it possible to set the order in multi column tlegends? left-right vs top-bottom
- pipe box line fill properties to drawing
- improve use last color feature (color = -1)
- change only color not marker options?
- add line in ratio plots? in plot style? value of const line flexible?
- think about more placeholders for legends and texts and how to format them
- option to set n divisions of axes
- add option in padstyle that identifies the pad as ratio plot
- fix overlap between axis title and tick marks if they have too many digits
- add possibility for grey tilted overlayed text like "draft"
- write case-insensitive "contained in string" function for control string! as lambda function
- text align, angle features!
- put in some feasible defaults for colors, markers etc
- for loading and saving styles: possibility for default values as fallback?
- possibility to specify use of only full or open markers in AddData
- also define default line styles
- generalized version of white patch hiding the truncated zero
- possibility to split the legend?
- maybe linking axes should also be possible for different axes (x in pad1 to y in pad2)

Bugs:
- width and height calculation wrong for text boxes (ndc vs relative pad coordinates?)
- using user coordinates in legend box breaks something for text box?
- moving text boxes in log scale plots has a bug in interactive mode
- when saving to macro or pdf, the boxes are slightly misplaced (some global setting missing?)
- make sure figure groups cannot contain '.'
- exponents on x axis are not in the proper position
- boxes are not transparent by default
- sometimes text boxes are randomly not drawn?
- if lables are 1, 2, 3 height is not calculated correctly?

Structural changes:
- use short instead of int whenever possible
- internally use hashes instead of strings
- use for (auto& [key, value]: myMap) {} loops
- think about how things can be implemented in a more general way as templates
- change arguments referring to internal variables to const ref if possible to be more memory efficient, use lambdas
- add (copy-, move-) constructors and destructors; make sure all variables are initialized properly

Feature ideas:
- functions and fitting
- shape objects (arrows)
- support for input types T{H,F}3, THn{Sparse}, THStack, TMultiGraph
- option for plotting projectins? is this reasonable?
- support for setting up TView to have a 3d representations of 2d data
- investigate if using TPad::DrawFrame() makes sense
- possibility for user to grep and change specific plotstyle or plot that was already loaded in manager
