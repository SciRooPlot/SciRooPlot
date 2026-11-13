"""A legend can hold the rows of data drawn in other pads (LegendBox::AddDataEntry(pad, data)): such a row is the row the data has in a
legend of its own pad (its label with the placeholders of that data replaced, its symbol as the data is drawn), or carries its own label;
a row of data or a pad that does not exist is warned about and left out, data of the legend's own pad that has its own row there as well
keeps both rows, with a warning. A legend in a cell without frame (an empty cell of a layout)
takes such rows as well.
Cases: every data kind in the panel of a two-panel layout, its row in a legend of the other panel and in an empty strip above both.
Reference: the legend of the data's own pad.
Covers: the legend rows for data of other pads of the multi-panel features, and the named cases.
"""

from srptest import DATA_KINDS, Project, S, add_data, cases, check, data_name, describe, finish

todo = cases({"kind": [k for k, (dim, _, props) in DATA_KINDS.items() if dim < 3 and "polar" not in props], "host": ["panel", "strip"]}, always=lambda c: True)

p = Project("legend_rows", bases=("1d_2x1_gap", "2d_2x1_gap"))
p.inputs()
LABEL = "<name> row"
for i, case in enumerate(todo):
    kind = case["kind"]
    flat = "1d" if DATA_KINDS[kind][0] == 1 else "2d"
    if case["host"] == "strip":
        plot = S.Plot(f"r{i}", S.PanelLayout([[S.Empty(1, 0.3).Span(2)], [S.Panel(), S.Panel()]]))
        plot.SetGroup("rows")
        host, own, other = 1, 3, 2
    else:
        plot = S.Plot(f"r{i}", "rows", f"{flat}_2x1_gap")
        host, own, other = 1, 2, 2
    add_data(plot[own], kind).SetLegendLabel(LABEL)
    if host != own and case["host"] == "panel":
        add_data(plot[host], kind)
    if case["host"] == "strip":
        add_data(plot[other], kind)
    plot[own].AddLegend(0.1, 0.9).SetPadCoordinates()  # the reference: the row in a legend of the data's pad
    legend = plot[host].AddLegend(0.1, 0.9).SetPadCoordinates()
    legend.AddDataEntry(own, 1)
    legend.AddDataEntry(own, 1).SetLabel("own label")
    p.pm.AddPlot(plot)

plot = S.Plot("missing", "named", "1d_2x1_gap")
plot[1].AddData("hist", "inputs")
plot[2].AddData("hist", "inputs")
legend = plot[1].AddLegend()
legend.AddDataEntry(2, 5)
legend.AddDataEntry(9, 1)
p.pm.AddPlot(plot)
plot = S.Plot("own_row_twice", "named", "1d_2x1_gap")
plot[2].AddData("hist", "inputs")
plot[1].AddData("hist", "inputs", "one")
plot[1].AddData("hist", "inputs", "two")
plot[1].AddLegend().AddDataEntry(1, 2)
p.pm.AddPlot(plot)

run = p.plot()
check("the plots are made without errors", not run.errors(), " | ".join(run.errors()[:3]))


def rows(padObj):
    legend = next((o for o in padObj.GetListOfPrimitives() if o.InheritsFrom("TLegend")), None) if padObj else None
    found = []
    for e in legend.GetListOfPrimitives() if legend else []:
        found.append((e.GetLabel(), e.GetOption(), e.GetMarkerStyle(), e.GetMarkerColor(), e.GetLineColor(), e.GetLineStyle(), e.GetLineWidth(), e.GetFillColor(), e.GetFillStyle()))
    return found


for i, case in enumerate(todo):
    what = describe(case)
    own = 3 if case["host"] == "strip" else 2
    reference = rows(run.pad(f"r{i}", "rows", own))
    found = rows(run.pad(f"r{i}", "rows", 1))
    if len(reference) != 1:
        check(f"{what}: the data has a row in its own pad", False, f"{reference}")
        continue
    want = reference[0]
    check(f"{what}: the row is the one of the data's own pad", found[:1] == [want], f"{found[:1]} instead of {want}")
    check(f"{what}: a row with its own label, the symbol of the data", found[1:] == [("own label", *want[1:])], f"{found[1:]} instead of {('own label', *want[1:])}")
    check(f"{what}: the label's placeholders are those of the data", want[0] == f"{data_name(case['kind'])} row", f"{want[0]!r}")

legendRows = rows(run.pad("missing", "named", 1))
messages = run.messages("missing", "named")
check("rows of data or pads that do not exist are left out", not legendRows, f"{legendRows}")
check("rows of data or pads that do not exist are warned about", any("data 5" in m and "pad 2" in m for m in messages) and any("pad 9" in m for m in messages), " | ".join(messages))
labels = [row[0] for row in rows(run.pad("own_row_twice", "named", 1))]
messages = run.messages("own_row_twice", "named")
check("data of the legend's own pad added with AddDataEntry keeps both rows", labels == ["one", "two", "two"], f"{labels}")
check("data of the legend's own pad added with AddDataEntry is warned about", sum("data 2 has two rows" in m for m in messages) == 1, " | ".join(messages))
finish()
