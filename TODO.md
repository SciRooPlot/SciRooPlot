- check again if auto placement works correct
- add some top right etc position (enum) functionality with modifyable distance to axis
- default box properties in pad
- separate box vector in pad
- ids for boxes
- Set
- data.SetPreferedLegend()



Planned Features:
- streamline legend and text boxes (add setters and getters + fix constructor)
- possibility to split the legend in separate boxes (add some sort of id)
- possibility to specify digit format gStyle->SetPaintTextFormat("4.1f m");
- add setting for nContours in 2d plots
- normalize and scale should use same function
- add option in main program to list all loaded plots
- possibility to do fast loading of all data from input files (skip chek if data was requested)
- add more placeholders for legends and texts and how to format them

Feature Ideas:
- generic algorithm to automatically determine optimal axis offsets + avoid overlap with lables
- revisit idea of axis linking between pads
- add root style, color, marker, linestyle pictures to documentation
- fully support csv format (flexible string and delimiter)
- add user functions and fitting (AddFunction() and AddLine())
- add shape objects (arrows)
- support for input types T{H,F}3, THn{Sparse} (->Projection functionality), THStack, TMultiGraph
- support for setting up TView to have a 3d representations of 2d data
- add inclination angle setting to textboxes
- define 'top left' 'bottom right',... positons for boxes with flexible distance to ticks
- add 'same as previous' color/marker/style feature (color = -1)?
- allow for multi-column text boxes and make tab width between them a text box property
- customizable legends that can be added independent of data in plot

Notes:
- for 2d plots SAME0 can be used to avoid axis range change when drawing on top
- is it possible to set the order in multi column tlegends? left-right vs top-bottom
- maybe at some point it could become useful to also copy data and boxes from template
- maybe generalized version of white patch hiding the truncated zero in ratio pads would be nice
- should smoothing be done before or after division for ratios (or both)??

Structural:
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

- fully grasp the difference between emplace_back and push_back in terms of copy/move: why does push_back not work for variants?

