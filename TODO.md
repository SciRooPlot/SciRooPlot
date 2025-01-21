- add marker/line transparency
- replace AddRefFunc with user-defined functions that can be modified
- check if box auto-size determination considers that legend entries may have different text sizes
- add default-boxes (legend and text) that will be baseline for all boxes of this pad
- add flexible margins for the boxes
- make width of marker column in legend box user definable
- fully support csv format (flexible string and delimiter)
- decide where data manipulation part should be put (before or after setting ranges?)
- should smoothing be done before or after division for ratios (or both)??
- SetOption(string) does not override SetOption(defaultOption)
- if same histogram is added to plot twice, the name is no longer unique and this results in unwanted behavior for ref data specified in legend entries
- csv w/o options -> invisible -> TGraphErrors has wrong defaults
- support TGraphAsymmErrors
- add division hist/graph, graph/hist

- somehow allow for data manipulation to propagate into ratios (link with the respective data)

- add install options like in a real program...

---------------------------------

- man muss ueberlegen dass wenn active leer ist es unklar ist was passieren muss -> skripte geben path der mist baut und dann werden vlt irgendwo dinge erzeugt? (check in plotrc was passiert wenn pfad nicht definiert oder ins nichts zeigt)

- definePlots als programmnamen


program
savedir
- statt executable vlt program /app + address that it assumes cmake ..
- output dir muss manuell sein vlt einfach so -> user wird einfach aufgefordert den Befehl einzugeben

waum mach ich nicht einfach in das plotrc schon cmake prefix path? das muesste doch theoretisch richtiger sein dann... (wenn mans eh sourced) aber dann braucht man das soruce ding waehrend man installiert
man koennte beides machen





- add help in plot app
- testen ob man per export wirklich das config ding gesteuert kriegt
- produce a root file that illustrates the features (1D and ratio etc mit subfolders) multiple root files! / only macro
- maybe run root macro in cmake
- remove example for sake of skeleton
- focus inintroduction only on the intended usage


- wenn man renamed wird ggf der name im programm nicht automatisch angepasst! -> echo das rein oder adjuste es da
- define plots
