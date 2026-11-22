# SciRooPlot command line

Both commands are shell functions defined by `share/scirooplot/env.sh` (source it in `.zshrc`/`.bashrc`); they act on the **current project**.

## `srp` — projects and settings

```
Project management:        srp projects [<project>|@current] (program, outdir, settings, plots, inputs; without one: every project) | srp select <project>
                           srp rename (<project>|@current) <name> | srp remove (<project>|@current)
Project initialization:    srp init-cpp <project> [<dir>] | srp init-py <project> [<dir>]            (minimal skeleton, becomes the current project)
                           srp example-cpp <project> [<dir>] | srp example-py <project> [<dir>]      (commented examples with example data)
                           srp add <project> <program> [<outdir>]                                    (register an existing program)
Project configuration:     srp get (<project>|@current) <property> | srp set ... <property> <value> | srp unset ... <property>
                           property = program | outdir | <setting>   (settings are declared and read by the program, pm.Setting("<name>", <default>, ...))
Project access:            srp confdir [<project>] (config folder) | srp cd [<project>] (program folder) | srp edit [<project>] (source files) | srp out [<project>] (output folder in the file manager)
Settings:                  srp settings | srp <setting> without a value prints it | srp color (bright|dark|off) | srp verbosity (debug|hint|log|info|warning|error|silent) | srp plotmode <mode> (any mode of plot, e.g. show|spread|pdf|png|gif+<centiseconds>; the default of plot, initially show)
                           srp matchmode (exact|contains) | srp matchcase (sensitive|insensitive) | srp screenscale <factor> | srp bitmapscale <factor> | srp cache (on [<folder>]|off) (local copies of remote inputs: in the config folder, or in <folder>/SciRooPlot)
Maintenance:               srp info | srp update | srp reinstall | srp clean | srp reset | srp help
Utilities:                 srp print <file>[:<path>] (contents of a ROOT file, or below a path in it; a path to one object gives its axes, binning, entries and contents) | srp print hepdata:<record>[:<table>] (the data names of a HEPData record or downloaded file, per table) | srp open <file> (in a TBrowser)
                           srp colorwheel [<file>]   (ROOT's colour wheel, names under the mouse in the status bar; with a file: saved there)
```
Project settings: the program declares them with `pm.Setting(name, default, description, choices)` (a bool default makes a switch set `on`/`off`, an int or float default a number, a text default a text, restricted to the choices if given) and gets the value given with `srp set`, otherwise the default. `srp set` checks the value and refuses undeclared names, Tab completes the names (in zsh with their descriptions) and the values of switches and choices, `srp unset` goes back to the default, `srp projects` lists them, and `plot` runs the program again after a value changed. The names `program`, `outdir` and `plots` are reserved.

The verbosity applies to log messages only (`hint` adds what a plot shows at a glance, such as a box covering data or labels running into each other, for programs reading the output; `SCIROOPLOT_VERBOSITY=<level>` sets it for one command without changing the stored setting): output a command is asked for (`srp projects`, `srp settings`, `srp print`, `plot … list`, `plot … print`, `Print()`) is always shown.

Files: `~/.SciRooPlot/` (or `$SCIROOPLOT_CONFIG_PATH/`) holds `settings.info`, `projects.info` (programs, output folders, setting values, current project) and one folder per project with `plots.info`, `dataSources.info` and `settings.info` (the settings declared) written by `pm.SaveProject()`.

## `plot` — generate plots

```
plot <group> <name> [<mode>]
```
- `group` and `name` are regular expressions matched against the whole group / plot name (`srp matchmode contains` matches substrings). All plots of a group: `plot myGroup .+`.
- A group includes its subgroups (`myGroup` also matches `myGroup/QA`); `myGroup$` excludes them; `myGroup/QA` selects a subgroup.
- Quote patterns containing `(`, `)` or `|` in bash: `plot thesis 'fig_(a|b)' pdf`.
- In bash, `plot` is an alias that turns off filename expansion for its arguments (`.*` would otherwise become the dotfiles of the current directory); it works on its own, after `;` and after `&&`. After `||`, `!`, `time` or `|`, put it in braces: `make || { plot g .*; }`.
- Modes: `show` (the default unless `srp plotmode` sets another; interactive: `s`/`a` next/previous plot (circular), `d` moves the current plot into a window of its own, the browsing window shows the plot seen before it (no effect on the last plot left); `q` closes a window, `x` all; the keys also work in the terminal, where `q` closes all; double-click a legend, text, image or shape to print its name and coordinates), `spread` (all matching plots at once, one window each, as a cascade; `q` closes a window, `x` all); both end when the last window is closed, `list` (names only), `print` (definition), `pdf`, `eps`, `ps`, `svg`, `png`, `jpg`, `gif` and `gif+<centiseconds>` (all matching plots as one animation, 50 centiseconds per frame unless given, played once; `gif+<centiseconds>++` loops endlessly, `gif+<centiseconds>++<repeats>` repeats it that many times; the file is `<group>.gif` next to the folder of the deepest group all frames are in, `animation.gif` without a common group; the gif has the size of its first frame, so give its plots one canvas size: a frame of another size is cut by viewers, with a warning), `html` (needs ROOT 6.42 or newer), `json`, `xml`, `root`, `macro` (ROOT macro reproducing the plot), `file` (canvases into one ROOT file), `data` (the input objects of the plots, as read, into a ROOT file).
- Files go to the project's `outdir`, organized in subfolders per group; bitmaps are scaled by `srp bitmapscale`.
- Before plotting, the program of the current project is rebuilt (CMake projects) and re-run when its sources are newer than `plots.info`.
- Tab completion covers commands, groups and plot names.
