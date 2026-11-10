"""The rows of a legend are in the order of the data, unless a row is placed with its legend entry: at the top (AtTop), before or after
the place of a data item (BeforeData, AfterData; whether or not that data has a row in the legend); rows added with AddEntry go to the
end unless placed the same way. Rows at the same place: the rows of the data in the order of the data, then the added rows in the order
they were added. A place relative to data the pad does not have is warned about once per row (the row goes before or after where that
data would be).
Cases: four data items (the third without a row) with places for the second and the fourth, and an added row with a place of its own.
Reference: the documented rule, written out here.
Covers: a model drawn first listed after the data it describes.
"""

from srptest import Project, S, add_data, cases, check, describe, finish

PLACES = ("none", "top", "before_1", "before_3", "after_1", "after_3", "after_4", "before_9")
ROWS = {1: "d1", 2: "d2", 4: "d4"}  # the data with a row, by number (data 3 has none)


def place(entry, where):
    """places a legend entry"""
    if where == "top":
        entry.AtTop()
    elif where.startswith("before_"):
        entry.BeforeData(int(where[7:]))
    elif where.startswith("after_"):
        entry.AfterData(int(where[6:]))
    return entry


def key(where, unplaced):
    """the place of a row as MergeLegendEntries orders them: (data, 0 before / 1 at / 2 after it)"""
    if where == "none":
        return unplaced
    if where == "top":
        return (0, 0)
    side, data = where.split("_")
    return (int(data), 0 if side == "before" else 2)


todo = cases(
    {"second": list(PLACES), "fourth": list(PLACES), "added": list(PLACES) + ["absent"]},
    named=[
        {"second": "after_4", "fourth": "none", "added": "absent", "why": "a model drawn first listed after the data"},
        {"second": "before_3", "fourth": "after_3", "added": "after_3", "why": "rows around data without a row of its own"},
        {"second": "after_1", "fourth": "after_1", "added": "after_1", "why": "rows at the same place: data in order, then added rows"},
    ],
)

p = Project("legend_order")
p.inputs()
for i, case in enumerate(todo):
    plot = S.Plot(f"o{i}", "order", "1d")
    for number in range(1, 5):
        data = add_data(plot[1], "hist")
        if number in ROWS:
            data.SetLegendLabel(ROWS[number])
        if number == 2:
            place(data.GetLegendEntry(), case["second"])
        elif number == 4:
            place(data.GetLegendEntry(), case["fourth"])
    legend = plot[1].AddLegend()
    if case["added"] != "absent":
        place(legend.AddEntry("added"), case["added"])
    p.pm.AddPlot(plot)
run = p.plot()

for i, case in enumerate(todo):
    what = describe(case)
    pad = run.pad(f"o{i}", "order")
    legend = next((o for o in pad.GetListOfPrimitives() if o.InheritsFrom("TLegend")), None) if pad else None
    got = [entry.GetLabel() for entry in legend.GetListOfPrimitives()] if legend else None
    rows = [(key("none", (1, 1)), "d1"), (key(case["second"], (2, 1)), "d2"), (key(case["fourth"], (4, 1)), "d4")]
    if case["added"] != "absent":
        rows.append((key(case["added"], (float("inf"), 0)), "added"))
    want = [label for _, label in sorted(rows, key=lambda row: row[0])]  # stable: data rows first, then the added row
    check(f"rows in the documented order: {what}", got == want, f"rows {got}, expected {want}")
    outside = sum(case[k] == "before_9" for k in ("second", "fourth", "added"))
    warned = [line for line in run.messages(f"o{i}", "order") if "is placed relative to data" in line]
    check(f"{outside} warning(s) for places beside data the pad does not have: {what}", len(warned) == outside, " | ".join(warned))
check("no other warnings", all("is placed relative to data" in w for w in run.warnings()), " | ".join(run.warnings()[:5]))
check("no errors", not run.errors(), " | ".join(run.errors()[:5]))
finish()
