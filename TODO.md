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
- fix title stuff in Pad!
    gStyle->SetTitleW((pad->GetX2() - pad->GetRightMargin()) - (pad->GetX1() + pad->GetLeftMargin()));
    gStyle->SetTitleH(pad->GetTopMargin()*0.8);
    gStyle->SetTitleAlign(kHAlignCenter + kVAlignTop);
- add nContours as option to pad
- add axis linking

- better to use Data::SetFrame() to indicate which data should define frame! (automatically add copy to front of pad data vector)
- add setters and getters to boxes + fix constructor
- fix visibility of constructors + check if everything is initialized correctly
- gStyle->SetPaintTextFormat("4.1f m");
- what should be default drawing style for graphs?
- for 2d -> use SAME0 to avoid axis range change when drawing on top?
- xErrors!! toggle on and off?
- predefined styles for the different datatypes (graph,hist,2d)
- add angle to textboxes
- put in some feasible defaults for colors, markers, line styles


Major Features:
- Plot needs AddFunction() and AddLine() accessor so one can directly plot simple functions. This must then also be of type data??
- add some more formatting features for legend (floating point precision, exponent..)
- add constructor to plotStyle that already gives useful default markers, colors, 2d style, etc
- be clear about coordinate systems (fix relative positioning, maybe)
- find a general way to automatically determine optimal axis offsets (in particular for 2d and 3d views) (what unit is title offset?)
- generalize legend function to handle text boxes in a similar manner

Minor Features:
- remove name argument from axis?
- make ref-line configurable
- normalize and scale should use same function
- consider all possibilities where displayed symbols in legend have to be adjusted (in particular filled histos are missing)
- can there be a 'use same as previous' color/marker/style feature (color = -1)
- add graph division feature with tspline3
- add another stand-alone app to quickly create and extend input file file
- option in main program to list loaded plots
- possibility to load all data from input files
- maybe define 'top left' 'bottom right' etc default positons for boxes in general manner? (flexible minimal distance to ticks)
- is it possible to set the order in multi column tlegends? left-right vs top-bottom
- possibility to split the legend?
- think about more placeholders for legends and texts and how to format them
- fix overlap between axis title and tick marks if they have too many digits
- add possibility for grey tilted overlayed text like "draft"
- generalized version of white patch hiding the truncated zero
- axis->SetTicks("+-")


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
- understand why move constructors implicitly delete assignment operators
- move analysis stuff out of the repository and make external repository (link cmakes?)
- store strings in vector and then only work with indices everywhere
- use str_contians() everywhere
- always use smallest possible datatype whenever possible
- put data pointers in vector instead of map and use index for all comparisons
- use for (auto& [key, value]: myMap) {} loops everywhere
- think about how things can be implemented in a more general way as templates
- change arguments referring to internal variables to const ref if possible to be more memory efficient, use lambdas
- add (copy-, move-) constructors and destructors; make sure all variables are initialized properly

Feature ideas:
- maybe at some point it could become useful to also copy data and boxes from template?
- add alpha (transparency level) to plot,pad,frame
- add root style, color, marker, linestyle pictures to documentation
- support csv format (flexible string and delimiter)
- add reverse axis and time axis and axis on opposite side (X+, Y+)
- functions and fitting
- shape objects (arrows)
- support for input types T{H,F}3, THn{Sparse} (->Projection functionality), THStack, TMultiGraph
- support for setting up TView to have a 3d representations of 2d data
