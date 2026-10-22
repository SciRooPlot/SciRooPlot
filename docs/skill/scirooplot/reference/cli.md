# SciRooPlot command line

Both commands are shell functions defined by `share/scirooplot/env.sh` (source it in `.zshrc`/`.bashrc`); they act on the **current project**.

## `srp` — projects and settings

```
Project management:        srp projects | srp select <project> | srp show [<project>|@current] | srp stats (<project>|@current)
                           srp rename (<project>|@current) <name> | srp remove (<project>|@current)
Project initialization:    srp init-cpp <project> [<dir>] | srp init-py <project> [<dir>]            (minimal skeleton, becomes the current project)
                           srp example-cpp <project> [<dir>] | srp example-py <project> [<dir>]      (commented examples with example data)
                           srp add <project> <program> [<outdir>]                                    (register an existing program)
Project configuration:     srp get (<project>|@current) <property> | srp set ... <property> <value> | srp unset ... <property>
                           property = program | outdir | <user variable>   (user variables are read in code via pm.GetProjectProperty("<name>"))
Project access:            srp confdir [<project>] (config folder) | srp cd [<project>] (program folder) | srp edit [<project>]
Settings:                  srp settings | srp color (bright|dark|off) | srp verbosity (debug|log|info|warning|error) | srp plotmode (show|pdf|eps|svg|png|gif|jpg)
                           srp matchmode (exact|contains) | srp matchcase (sensitive|insensitive) | srp screenscale <factor> | srp bitmapscale <factor>
Maintenance:               srp info | srp update | srp clean | srp reset | srp help
```
The verbosity applies to log messages only: output a command is asked for (`srp projects`, `srp settings`, `srp print`, `plot … list`, `plot … print`, `Print()`) is always shown.

Files: `~/.SciRooPlot/` (or `$SCIROOPLOT_CONFIG_PATH/`) holds `settings.info`, `projects.info` (programs, output folders, user variables, current project) and one folder per project with `plots.info` and `dataSources.info` written by `pm.SaveProject()`.

## `plot` — generate plots

```
plot <group> <name> [<mode>]
```
- `group` and `name` are regular expressions matched against the whole group / plot name (`srp matchmode contains` matches substrings). All plots of a group: `plot myGroup .+`.
- A group includes its subgroups (`myGroup` also matches `myGroup/QA`); `myGroup$` excludes them; `myGroup/QA` selects a subgroup.
- Quote patterns containing `(`, `)` or `|` in bash: `plot thesis 'fig_(a|b)' pdf`.
- Modes: `show` (default; interactive: `s`/`a` next/previous plot (circular), `d` moves the current plot into a window of its own, the browsing window shows the plot seen before it (no effect on the last plot left); `q` closes a window, `x` all; the keys also work in the terminal, where `q` closes all; double-click a legend or text to print its coordinates), `spread` (all matching plots at once, one window each, as a cascade; `q` closes a window, `x` all); both end when the last window is closed, `list` (names only), `print` (definition), `pdf`, `eps`, `ps`, `svg`, `png`, `jpg`, `gif` and `gif+<centiseconds>` (all matching plots as one animation), `html`, `json`, `xml`, `root`, `macro` (ROOT macro reproducing the plot), `file` (canvases into one ROOT file), `data` (the drawn objects into a ROOT file).
- Files go to the project's `outdir`, organized in subfolders per group; bitmaps are scaled by `srp bitmapscale`.
- Before plotting, the program of the current project is rebuilt (CMake projects) and re-run when its sources are newer than `plots.info`.
- Tab completion covers commands, groups and plot names.
