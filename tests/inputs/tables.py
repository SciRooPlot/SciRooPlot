"""Table files (.csv, .tsv, .dat, .txt, .tab; the data name is the file name without its ending) give the values written in them: the delimiter
is the one of , ; tab | space that splits every line into as many fields as the header (in quotes it does not count); quoted column names,
a byte order mark (also on a comment line), comment lines starting with # anywhere, blank lines, Windows line ends, spaces around the
line and a missing last line end are read as intended. A column is numeric when all its values are numbers: integers stay integers
(exact also beyond 32 bits, bins of whole-number width), a decimal point anywhere (also only in a late row), exponents, inf and nan make
it floating point, an empty cell of a numeric column is nan; a column with text is text (usable in filters). Column names that are no C++
identifiers can be requested as they are. A table that cannot be read is reported with the reason, and only its plots are lost: lines
with another number of fields (with a hint for columns aligned by several spaces, also in the header), no data, a value not matching its column (for each
request); spaces around the delimiters and decimal commas are warned about.
Covers: the CSV parts of the 2026-10-15 items (decimals after integer rows, inf, BOM), the CSV type inference of open items 4 and 10,
b334843 (a value not matching its column reported for each request), the bad CSV value of the old command-line suite (only its plot lost), the ROOT error for a graph of points with x values that
are not finite, the hint for aligned headers.
"""

import math

import numpy as np
from srptest import ROOT, Project, S, check, drawn_diff, finish

p = Project("tables")
NAN = float("nan")


def table(file_name, text, raw=b""):
    with open(p.path(file_name), "wb") as f:
        f.write(raw + text.encode())
    name = file_name.rsplit(".", 1)[0]
    p.pm.AddDataSource(name, p.path(file_name))
    return name


late = "".join(f"{i},{i}\n" for i in range(200))
manyComments = "a,b\n" + "".join(f"{i},{2 * i}\n" for i in range(150)) + "# a comment after the first lines\n150,300\n"
TABLES = {  # file: (content, raw prefix, {column: values}, columns of a scatter)
    "comma.csv": ("a,b\n1,2.5\n2,3.5\n-3,+4.5\n", b"", {"a": [1, 2, -3], "b": [2.5, 3.5, 4.5]}),
    "semicolon.csv": ("a;b\n1;2.5\n2;3.5\n", b"", {"a": [1, 2], "b": [2.5, 3.5]}),
    "tabs.tsv": ("a\tb\n1\t2.5\n2\t3.5\n", b"", {"a": [1, 2], "b": [2.5, 3.5]}),
    "pipes.txt": ("a|b\n1|2.5\n2|3.5\n", b"", {"a": [1, 2], "b": [2.5, 3.5]}),
    "spaces.dat": ("a b\n1 2.5\n2 3.5\n", b"", {"a": [1, 2], "b": [2.5, 3.5]}),
    "tabbed.tab": ("a\tb\n1\t2.5\n2\t3.5\n", b"", {"a": [1, 2], "b": [2.5, 3.5]}),
    "aligned_equally.txt": ("a  b\n1  2.5\n2  3.5\n", b"", {"a": [1, 2], "b": [2.5, 3.5]}),
    "quoted.csv": ('"a","b"\n1,2.5\n2,3.5\n', b"", {"a": [1, 2], "b": [2.5, 3.5]}),
    "quoted_delimiter.csv": ('a,b,c\n1,2.5,"x, y"\n2,3.5,z\n', b"", {"a": [1, 2], "b": [2.5, 3.5]}),
    "bom.csv": ("a,b\n1,2\n2,3.5\n", b"\xef\xbb\xbf", {"a": [1, 2], "b": [2.0, 3.5]}),
    "bom_comment.csv": ("# a comment\na,b\n1,2\n2,3.5\n", b"\xef\xbb\xbf", {"a": [1, 2], "b": [2.0, 3.5]}),
    "bom_quoted.csv": ('"a","b"\n1,2\n2,3\n', b"\xef\xbb\xbf", {"a": [1, 2], "b": [2, 3]}),
    "bom_semicolon.csv": ("a;b\n1;2\n2;3.5\n", b"\xef\xbb\xbf", {"a": [1, 2], "b": [2.0, 3.5]}),
    "comments.csv": ("# first\na,b\n# between\n1,2\n\n2,3\n# last\n", b"", {"a": [1, 2], "b": [2, 3]}),
    "late_comment.csv": (manyComments, b"", {"a": list(range(151)), "b": [2 * i for i in range(151)]}),
    "crlf.csv": ("a,b\r\n1,2.5\r\n2,3.5\r\n", b"", {"a": [1, 2], "b": [2.5, 3.5]}),
    "trimmed.csv": ("  a,b  \n  1,2.5\n2,3.5  \n", b"", {"a": [1, 2], "b": [2.5, 3.5]}),
    "no_last_newline.csv": ("a,b\n1,2.5\n2,3.5", b"", {"a": [1, 2], "b": [2.5, 3.5]}),
    "late_decimal.csv": ("a,b\n" + late + "200.5,7\n", b"", {"a": [*range(200), 200.5], "b": [*range(200), 7]}),
    "exponent.csv": ("a,b\n1e-3,1\n2E-3,2\n5e+2,3\n", b"", {"a": [1e-3, 2e-3, 500.0], "b": [1, 2, 3]}),
    "not_finite.csv": ("a,b\ninf,1\n0.5,2\nnan,3\n-inf,4\n", b"", {"a": [math.inf, 0.5, NAN, -math.inf], "b": [1, 2, 3, 4]}),
    "empty_cell.csv": ("a,b\n1,2\n,3\n3,4\n", b"", {"a": [1.0, NAN, 3.0], "b": [2, 3, 4]}),
    "empty_first.csv": ("a,b\n,1\n0.5,2\n1.5,3\n", b"", {"a": [NAN, 0.5, 1.5], "b": [1, 2, 3]}),
    "graph_not_finite.csv": ("a,b\n1,2\ninf,3\n3,nan\n4,5\n", b"", {"a": [1, 4], "b": [2, 5]}),
    "large.csv": ("a,b\n4294967297,1\n-9007199254740991,2\n", b"", {"a": [4294967297, -9007199254740991], "b": [1, 2]}),
    "names.csv": ("pT [GeV],y-value\n1.5,2\n2.5,3\n", b"", {"pT [GeV]": [1.5, 2.5], "y-value": [2, 3]}),
}
for file_name, (text, raw, _) in TABLES.items():
    table(file_name, text, raw)
TEXT = {  # file: (content, filter on its text column, the values of column a it keeps)
    "text.csv": ("a,c\n1,ab\n2,cd\n3,ab\n", 'c == "ab"', [1, 3]),
    "text_exponent.csv": ("a,c\n1,1e-3\n2,abc\n3,1e-3\n", 'c == "1e-3"', [1, 3]),
    "quoted_text.csv": ('a,c\n1,"x, y"\n2,z\n', 'c == "x, y"', [1]),
}
for file_name, (text, _, _) in TEXT.items():
    table(file_name, text)
ints = "n\n" + "".join(f"{i % 7}\n" for i in range(50))
table("ints.csv", ints)
FAILING = {  # file: (content, the error, it is a warning)
    "ragged.csv": ("a,b\n1,2\n3,4,5\n", "the header has 2 fields separated by ',', but line 3 has 3", False),
    "ragged_late.csv": ("a,b\n" + "".join(f"{i},{i}\n" for i in range(150)) + "1,2,3\n", "the header has 2 fields separated by ',', but line 152 has 3", False),
    "aligned_header.txt": ("a b\n10  2\n3 4\n", "columns seem to be aligned with several spaces or tabs; use exactly one delimiter between the values", False),
    "aligned.txt": ("a    b\n10   2\n3    4\n", "columns seem to be aligned with several spaces or tabs; use exactly one delimiter between the values", False),
    "comments_only.csv": ("# nothing\n\n", "does not contain any data", False),
    "bad_value.csv": ("a,b\n1,2\n2,3\n3,x\n", "does not match the type of its column", False),
    "spaces_around.csv": ("a ,b\n1 ,2.5\n", "has spaces around its delimiters (line 1)", True),
    "decimal_comma.csv": ("a;b\n1,5;2,5\n", "seems to use decimal commas (e.g. '1,5')", True),  # the values are text, the plot of them lost
}
for file_name, (text, _, _) in FAILING.items():
    table(file_name, text)

expected = {}  # plot: (what, points)
# values that are not finite are coded as finite ones (a graph does not draw points that are not finite)
CODE = "std::isnan({0}) ? -100. : std::isinf({0}) ? ({0} > 0 ? 100. : -200.) : {0}"


def coded(v):
    return -100.0 if math.isnan(v) else (100.0 if v > 0 else -200.0) if math.isinf(v) else v


for file_name, (_, _, columns) in TABLES.items():
    name = file_name.rsplit(".", 1)[0]
    a, b = list(columns)
    plot = S.Plot(name, "tables", "1d")
    data = plot[1].AddData(name, name)
    if not all(math.isfinite(v) for v in columns[a] + columns[b]) and name != "graph_not_finite":
        data.Define("coded", CODE.format(a)).Scatter("coded", b)
        columns = {a: [coded(v) for v in columns[a]], b: columns[b]}
    else:
        data.Scatter(a, b)
    p.pm.AddPlot(plot)
    expected[name] = (file_name, {"x": columns[a], "y": columns[b]})
for file_name, (_, flt, kept) in TEXT.items():
    name = file_name.rsplit(".", 1)[0]
    plot = S.Plot(name, "tables", "1d")
    plot[1].AddData(name, name).Filter(flt).Scatter("a", "a")
    p.pm.AddPlot(plot)
    expected[name] = (f"{file_name}: a text column in a filter", {"x": kept, "y": kept})
plot = S.Plot("ints", "tables", "1d")
plot[1].AddData("ints", "ints").Project1D("n").SetShowOverflowBins()
p.pm.AddPlot(plot)
plot = S.Plot("names", "names", "1d")
plot[1].AddData("names", "names").Project1D(("pT [GeV]", 2, [1.0, 3.0]), "y-value")
p.pm.AddPlot(plot)
for file_name in FAILING:
    name = file_name.rsplit(".", 1)[0]
    plot = S.Plot(name, "failing", "1d")
    plot[1].AddData(name, name).Project1D(("a", 10, [0.0, 10.0]))
    if name == "bad_value":
        plot[1].AddData(name, name).Project1D(("b", 10, [0.0, 10.0]))
    p.pm.AddPlot(plot)
plot = S.Plot("beside", "failing", "1d")
plot[1].AddData("comma", "comma").Scatter("a", "b")
p.pm.AddPlot(plot)

run = p.plot("tables|names")
for plotName, (what, points) in expected.items():
    obs = run.objects(plotName, "tables")
    got = {"x": [obs[0].GetPointX(i) for i in range(obs[0].GetN())], "y": [obs[0].GetPointY(i) for i in range(obs[0].GetN())]} if obs else None
    ok = got is not None and all(len(got[k]) == len(points[k]) and all(a == b or (math.isnan(a) and math.isnan(b)) for a, b in zip(got[k], points[k])) for k in "xy")
    check(f"{what}: the values written", ok, f"{got}, expected {points}" if got else "not drawn")
ref = ROOT.TH1D("ref", "", 7, -0.5, 6.5)
ref.Sumw2()
for i in range(50):
    ref.Fill(i % 7)
diff = drawn_diff([ref], run.objects("ints", "tables"))
check("an integer column: automatic bins of whole-number width centred on the integers", not diff, diff)
ref = ROOT.TH1D("ref", "", 2, 1.0, 3.0)
ref.Sumw2()
ref.Fill(1.5, 2.0)
ref.Fill(2.5, 3.0)
obs = run.objects("names", "names")
check("column names that are no C++ identifiers can be requested and weighted by", obs and all(obs[0].GetBinContent(i) == ref.GetBinContent(i) for i in (1, 2)), [obs[0].GetBinContent(i) for i in (1, 2)] if obs else "not drawn")
notDrawn = "graph_not_finite has 2 points with NaN or infinite values, which are not drawn."
check("points that are not finite: not drawn, with a warning", [w for w in run.warnings() if notDrawn in w], " | ".join(run.warnings()))
check("the tables give no other warnings, no errors", not [w for w in run.warnings() if notDrawn not in w] and not run.errors(), " | ".join(run.warnings() + run.errors()))
nSources = len(expected) + 1  # and ints
reading = f"Reading {nSources} data items from {nSources} inputs in {nSources} data sources."  # a table is one data item, whatever is requested from it
check("the reading summary counts the table files read", reading in run.log, f"{[line for line in run.lines('INFO') if 'Reading' in line]}, expected '{reading}'")

failed = p.plot("failing")
for file_name, (_, text, isWarning) in FAILING.items():
    name = file_name.rsplit(".", 1)[0]
    lines = [line for line in (failed.warnings() if isWarning else failed.errors()) if text in line and p.path(file_name) in line or (name == "bad_value" and text in line)]
    made = bool(failed.canvas(name, "failing"))
    if isWarning:
        # the values are read as text, the column names with their spaces: the plot of a lost as by the warning
        check(f"{file_name}: warned about", not made and len(lines) == 1, f"plot {'made' if made else 'lost'}, {len(lines)} warnings: {failed.warnings()[:3]}")
    else:
        count = 2 if name == "bad_value" else 1
        check(f"{file_name}: reported{' for each request' if count > 1 else ''}, the plot is lost", not made and len(lines) == count, f"plot {'made' if made else 'lost'}, {len(lines)} errors: {failed.errors()[:3]}")
diff = drawn_diff([ROOT.TGraph(3, np.array([1.0, 2.0, -3.0]), np.array([2.5, 3.5, 4.5]))], failed.objects("beside", "failing"))
check("a table beside one that cannot be read is read as usual", not diff, diff)
finish()
