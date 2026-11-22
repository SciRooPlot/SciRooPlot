/*
 ******************************************************************************************
 * --------------------------------------- SciRooPlot -------------------------------------
 * Copyright (c) 2019-2026 Mario Krüger
 * Contact: mario.kruger@cern.ch
 * For a full list of contributors please see doc/CONTRIBUTORS.md.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation in version 3 (or later) of the License.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * The GNU General Public License can be found here: <https://www.gnu.org/licenses/>.
 ******************************************************************************************
 */

#include "util/HepData.h"

#include "SciRooPlot/Logger.h"

#include <TArrayD.h>
#include <TAxis.h>
#include <TDirectory.h>
#include <TFile.h>
#include <TGraph2DErrors.h>
#include <TGraphAsymmErrors.h>
#include <TH1.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TKey.h>
#include <TSystem.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <regex>
#include <set>
#include <string>
#include <sys/wait.h>
#include <utility>
#include <vector>

#include "util/Paths.h"
#include "util/RootContainers.h"
#include "util/ScopeGuard.h"
#include "util/Strings.h"

using std::string;
using std::vector;

namespace SciRooPlot::util
{
namespace
{
constexpr std::string_view kHepDataScheme = "hepdata:";

// the objects HEPData writes for dependent variable y<i> of a table (format=root): a graph with the total errors, the bins of a binned
// table (with the values as contents and their own value as error, so not to be drawn) and one histogram per error source, holding the
// error as contents (an asymmetric one in two: up and down, as given, down mostly negative)
struct hepdata_variable_t {
  const TGraphAsymmErrors* graph{};
  const TGraph2DErrors* graph2d{};  // only when all errors are symmetric
  const TH1* bins{};
  struct error_source_t {
    string label;
    const TH1* up{};
    const TH1* down{};  // the same histogram as up for a symmetric error
  };
  std::map<int, error_source_t> sources;  // by their number in the table
};

// for each dependent variable the total and every error source: name, errors below and above each point (positive)
using variants_t = vector<std::pair<string, vector<std::pair<double, double>>>>;

// what the ROOT file of a record does not hold, by table, for one message per record
struct record_notes_t {
  vector<string> withoutBins;  // tables of points: HEPData writes their total errors only
  vector<string> overlapping;  // "<table> (y<i>)": bins that overlap, the error of a point per source is lost
  vector<string> threeD;       // tables with three independent variables: 3d histograms only
};

// adds to the copy of a table an object per dependent variable y<i> and error source: y<i> with the total errors, y<i>_<label> with one
// error source; a histogram for a binned table with one point per bin (text bins: per bin with a value) and symmetric errors, a graph
// otherwise
void add_variables(TDirectory* table, TDirectory* out, const string& tableName, record_notes_t& notes)
{
  static const std::regex objectName(R"((Graph|Hist)([12])D_y(\d+)(_e(\d+)(plus|minus)?)?)");
  std::map<int, hepdata_variable_t> variables;
  vector<std::unique_ptr<TObject>> owned;
  bool has3d = false;
  for (auto* key : TRangeDynCast<TKey>(table->GetListOfKeys())) {
    if (!key || key_inherits(key, TDirectory::Class())) continue;
    std::unique_ptr<TObject> obj(key->ReadObj());
    if (!obj) continue;
    if (auto* hist = dynamic_cast<TH1*>(obj.get())) hist->SetDirectory(nullptr);
    out->WriteTObject(obj.get(), key->GetName());  // everything as downloaded
    std::smatch match;
    const string name = key->GetName();
    if (str_starts_with(name, "Hist3D_y")) has3d = true;
    if (!std::regex_match(name, match, objectName)) continue;
    auto& variable = variables[std::stoi(match[3])];
    auto* hist = dynamic_cast<TH1*>(obj.get());
    if (match[1] == "Graph") {
      if (match[2] == "1") variable.graph = dynamic_cast<TGraphAsymmErrors*>(obj.get());
      if (match[2] == "2") variable.graph2d = dynamic_cast<TGraph2DErrors*>(obj.get());
    } else if (!hist) {
      continue;
    } else if (!match[4].matched) {
      variable.bins = hist;
    } else {
      // the label of an error source is the title of the value axis (asymmetric ones with _plus and _minus appended)
      string label = ((hist->GetDimension() == 1) ? hist->GetYaxis() : hist->GetZaxis())->GetTitle();
      for (const string suffix : {"_plus", "_minus"}) {
        if (match[6].matched && str_ends_with(label, suffix)) label.erase(label.size() - suffix.size());
      }
      auto& source = variable.sources[std::stoi(match[5])];
      source.label = label;
      if (match[6] == "minus") {
        source.down = hist;
      } else {
        source.up = hist;
        if (!match[6].matched) source.down = hist;
      }
    }
    owned.push_back(std::move(obj));
  }
  if (has3d && variables.empty()) notes.threeD.push_back(tableName);

  bool withoutBins = false;
  for (const auto& [index, variable] : variables) {
    const string name = "y" + std::to_string(index);
    const bool is2d = (variable.graph2d != nullptr);
    if (!variable.graph && !is2d) continue;
    const TGraph* graph = (is2d) ? static_cast<const TGraph*>(nullptr) : variable.graph;
    const TGraph2DErrors* graph2d = variable.graph2d;
    const int n = (is2d) ? variable.graph2d->GetN() : graph->GetN();
    const double* x = (is2d) ? variable.graph2d->GetX() : graph->GetX();
    const double* y = (is2d) ? variable.graph2d->GetY() : graph->GetY();
    const TH1* bins = variable.bins;
    if (!bins) withoutBins = true;
    auto binOf = [&](const TH1* hist, int i) { return (is2d) ? hist->FindFixBin(x[i], y[i]) : hist->FindFixBin(x[i]); };

    variants_t variants{{name, {}}};
    for (int i = 0; i < n; ++i) {
      if (is2d) {
        variants[0].second.emplace_back(variable.graph2d->GetErrorZ(i), variable.graph2d->GetErrorZ(i));
      } else {
        variants[0].second.emplace_back(variable.graph->GetErrorYlow(i), variable.graph->GetErrorYhigh(i));
      }
    }
    // the error of a point from a source is the content of its bin in the histogram of the source: lost where points share a bin (bins
    // that overlap), as HEPData fills them all into it
    bool sourcesLost = false;
    if (!variable.sources.empty()) {
      const auto& first = variable.sources.begin()->second;
      const TH1* sourceBins = (first.up) ? first.up : first.down;
      std::set<int> used;
      for (int i = 0; i < n && sourceBins && !sourcesLost; ++i) {
        const int bin = binOf(sourceBins, i);
        sourcesLost = sourceBins->IsBinUnderflow(bin) || sourceBins->IsBinOverflow(bin) || !used.insert(bin).second;
      }
    }
    if (sourcesLost) notes.overlapping.push_back(tableName + " (" + name + ")");
    std::set<string> names{name};
    for (const auto& [number, source] : variable.sources) {
      if (sourcesLost || !source.up || !source.down) continue;
      // in characters allowed in data names; a label that becomes the same as another one gets the number of its source
      string label = source.label;
      std::replace_if(label.begin(), label.end(), [](char c) { return !std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-'; }, '_');
      string variantName = name + "_" + label;
      if (!names.insert(variantName).second) names.insert(variantName += "_" + std::to_string(number));
      auto& errors = variants.emplace_back(variantName, vector<std::pair<double, double>>{}).second;
      for (int i = 0; i < n; ++i) {
        const double up = source.up->GetBinContent(binOf(source.up, i));
        const double down = (source.down == source.up) ? -up : source.down->GetBinContent(binOf(source.down, i));
        errors.emplace_back(-std::min({up, down, 0.}), std::max({up, down, 0.}));
      }
    }

    // values of a 1d table are the y of its graph, of a 2d table the z
    const double* values = (is2d) ? variable.graph2d->GetZ() : y;
    const bool symmetric = std::all_of(variants.begin(), variants.end(), [](const auto& variant) {
      return std::all_of(variant.second.begin(), variant.second.end(), [](const auto& e) { return std::abs(e.first - e.second) <= 1e-9 * std::max(e.first, e.second); });
    });
    // a binned table: every bin of the HEPData histogram holds exactly one point; text bins: every bin with a value (HEPData keeps the
    // bins of the values it leaves out)
    std::set<int> filledBins;
    vector<int> pointBins;
    if (bins && bins->GetDimension() == ((is2d) ? 2 : 1)) {
      for (int i = 0; i < n; ++i) {
        const int bin = binOf(bins, i);
        if (bins->IsBinUnderflow(bin) || bins->IsBinOverflow(bin) || !filledBins.insert(bin).second) break;
        pointBins.push_back(bin);
      }
    }
    const bool onePerBin = bins && static_cast<int>(pointBins.size()) == n;
    const bool labelledSubset = onePerBin && !is2d && bins->GetXaxis()->GetLabels() && n > 0 && n < bins->GetNbinsX();
    const bool binned = (onePerBin && n == bins->GetNbinsX() * ((is2d) ? bins->GetNbinsY() : 1)) || labelledSubset;

    // the axis titles of the graph, else of HEPData's histogram (the graph of a 2d table has none)
    auto axisTitle = [&](int axis) -> string {
      const TAxis* fromGraph{};
      if (is2d) fromGraph = (axis == 0) ? graph2d->GetXaxis() : (axis == 1) ? graph2d->GetYaxis()
                                                                            : graph2d->GetZaxis();
      if (!is2d) fromGraph = (axis == 0) ? graph->GetXaxis() : graph->GetYaxis();
      if (fromGraph && !is_blank(fromGraph->GetTitle())) return fromGraph->GetTitle();
      if (!bins) return "";
      return ((axis == 0) ? bins->GetXaxis() : (axis == 1) ? bins->GetYaxis()
                                                           : bins->GetZaxis())
        ->GetTitle();
    };
    const string xTitle = axisTitle(0);
    const string yTitle = axisTitle(1);
    const string zTitle = (is2d) ? axisTitle(2) : "";

    const string title = (is2d) ? variable.graph2d->GetTitle() : graph->GetTitle();
    auto copyLabels = [](const TAxis* from, TAxis* to) {
      if (from->GetLabels()) {
        for (int bin = 1; bin <= from->GetNbins(); ++bin) {
          to->SetBinLabel(bin, from->GetBinLabel(bin));
        }
      }
    };
    auto edges = [](const TAxis* axis) {
      const TArrayD* binEdges = axis->GetXbins();
      if (binEdges->GetSize()) return vector<double>(binEdges->GetArray(), binEdges->GetArray() + binEdges->GetSize());
      vector<double> fixed;
      for (int bin = 1; bin <= axis->GetNbins() + 1; ++bin) {
        fixed.push_back(axis->GetBinLowEdge(bin));
      }
      return fixed;
    };
    for (const auto& [variantName, errors] : variants) {
      std::unique_ptr<TObject> made;
      if (binned && symmetric) {
        std::unique_ptr<TH1> hist;
        if (labelledSubset) {
          // the bins with a value, in their order, with their labels
          hist = std::make_unique<TH1D>(variantName.data(), title.data(), n, 0., static_cast<double>(n));
          int newBin = 0;
          for (const int bin : filledBins) {
            hist->GetXaxis()->SetBinLabel(++newBin, bins->GetXaxis()->GetBinLabel(bin));
          }
        } else if (is2d) {
          const auto xEdges = edges(bins->GetXaxis());
          const auto yEdges = edges(bins->GetYaxis());
          hist = std::make_unique<TH2D>(variantName.data(), title.data(), xEdges.size() - 1, xEdges.data(), yEdges.size() - 1, yEdges.data());
          copyLabels(bins->GetXaxis(), hist->GetXaxis());
          copyLabels(bins->GetYaxis(), hist->GetYaxis());
        } else {
          const auto xEdges = edges(bins->GetXaxis());
          hist = std::make_unique<TH1D>(variantName.data(), title.data(), xEdges.size() - 1, xEdges.data());
          copyLabels(bins->GetXaxis(), hist->GetXaxis());
        }
        hist->SetDirectory(nullptr);
        hist->GetXaxis()->SetTitle(xTitle.data());
        hist->GetYaxis()->SetTitle(yTitle.data());
        hist->GetZaxis()->SetTitle(zTitle.data());
        for (int i = 0; i < n; ++i) {
          const int bin = (labelledSubset) ? static_cast<int>(std::distance(filledBins.begin(), filledBins.find(pointBins[i]))) + 1 : binOf(hist.get(), i);
          hist->SetBinContent(bin, values[i]);
          hist->SetBinError(bin, errors[i].second);
        }
        hist->SetEntries(n);
        made = std::move(hist);
      } else if (is2d) {
        if (!symmetric) continue;  // a graph in 2d has symmetric errors only (HEPData writes none then)
        vector<double> ex(variable.graph2d->GetEX(), variable.graph2d->GetEX() + n);
        vector<double> ey(variable.graph2d->GetEY(), variable.graph2d->GetEY() + n);
        vector<double> ez;
        for (const auto& error : errors) {
          ez.push_back(error.second);
        }
        auto graph2d = std::make_unique<TGraph2DErrors>(n, const_cast<double*>(x), const_cast<double*>(y), const_cast<double*>(values), ex.data(), ey.data(), ez.data());
        graph2d->SetDirectory(nullptr);
        graph2d->SetName(variantName.data());
        graph2d->SetTitle(title.data());
        graph2d->GetXaxis()->SetTitle(xTitle.data());
        graph2d->GetYaxis()->SetTitle(yTitle.data());
        graph2d->GetZaxis()->SetTitle(zTitle.data());
        made = std::move(graph2d);
      } else {
        auto made1d = std::make_unique<TGraphAsymmErrors>(n, x, values, variable.graph->GetEXlow(), variable.graph->GetEXhigh());
        for (int i = 0; i < n; ++i) {
          made1d->SetPointEYlow(i, errors[i].first);
          made1d->SetPointEYhigh(i, errors[i].second);
        }
        made1d->SetName(variantName.data());
        made1d->SetTitle(title.data());
        made1d->GetXaxis()->SetTitle(xTitle.data());
        made1d->GetYaxis()->SetTitle(yTitle.data());
        made = std::move(made1d);
      }
      out->WriteTObject(made.get(), variantName.data());
    }
  }
  if (withoutBins) notes.withoutBins.push_back(tableName);
}

// whether a file starts as gzip does
bool is_gzip_file(const string& file)
{
  std::ifstream in(file, std::ios::binary);
  unsigned char magic[2]{};
  return in.read(reinterpret_cast<char*>(magic), 2) && magic[0] == 0x1f && magic[1] == 0x8b;
}

// a record or file whose copy cannot be made is tried and reported once per run (several data sources may name it)
std::mutex failedMutex;
std::set<string> failedCopies;
bool failed_before(const string& key)
{
  std::lock_guard lock(failedMutex);
  return failedCopies.count(key) > 0;
}
void remember_failure(const string& key)
{
  std::lock_guard lock(failedMutex);
  failedCopies.insert(key);
}

// a string as one word of a shell command
string shell_quoted(const string& str)
{
  string result = "'";
  for (char c : str) {
    result += (c == '\'') ? string("'\\''") : string(1, c);
  }
  return result + "'";
}

// makes the copy <file> of a HEPData ROOT file, or of a .tar.gz holding one, with an object per dependent variable and error source next to
// HEPData's own; label names it in the messages (false with a warning if it cannot be made)
bool make_hepdata_copy(const string& source, const string& file, const string& label)
{
  std::error_code ec;
  Int_t status{};
  TString output;
  // hepdata.net serves the ROOT file packed in a .tar.gz (HEPData-<record>-v<version>-root.root); a bare ROOT file is taken as it is
  string rootFile = source;
  const string unpacked = temporary_file(file) + ".unpacked";
  auto removeUnpacked = make_scope_guard([&unpacked]() {
    std::error_code removeError;
    std::filesystem::remove_all(unpacked, removeError);
  });
  if (is_gzip_file(source)) {
    std::filesystem::create_directories(unpacked, ec);
    {
      auto quiet = ignore_root_messages_below(kFatal);
      output = gSystem->GetFromPipe(("tar -xzf " + shell_quoted(source) + " -C " + shell_quoted(unpacked)).data(), &status, true);
    }
    if (ec || status != 0) {
      logger::warning("Cannot unpack {}: {}.", label, (ec) ? ec.message() : string(output.Data()));
      return false;
    }
    rootFile.clear();
    for (const auto& entry : std::filesystem::recursive_directory_iterator(unpacked, ec)) {
      if (entry.is_regular_file() && entry.path().extension() == ".root" && (rootFile.empty() || entry.path().string() < rootFile)) rootFile = entry.path().string();
    }
    if (rootFile.empty()) {
      logger::warning("Cannot use {}: it holds no ROOT file.", label);
      return false;
    }
  }

  std::unique_ptr<TFile> in;
  {
    auto quiet = ignore_root_messages_below(kFatal);  // reported below
    in.reset(TFile::Open(rootFile.data(), "READ"));
  }
  if (!in || in->IsZombie()) {
    logger::warning("Cannot read {}: it is no readable ROOT file.", label);
    return false;
  }
  const string tmpFile = temporary_file(file);
  size_t nTables{};
  string version;
  record_notes_t notes;
  vector<std::pair<string, string>> renamed;  // tables with ':' in their name, which cannot be part of a data name
  {
    TFile out(tmpFile.data(), "RECREATE");
    if (out.IsZombie()) {
      logger::warning("Cannot write the copy of {} to {}.", label, file);
      return false;
    }
    static const std::regex doi(R"(hepdata\.\d+\.v(\d+))");
    for (auto* key : TRangeDynCast<TKey>(in->GetListOfKeys())) {
      // read by its key: GetDirectory would take a ':' in the name of a table for the end of a file name
      auto* table = (key && key_inherits(key, TDirectory::Class())) ? dynamic_cast<TDirectory*>(key->ReadObj()) : nullptr;
      if (!table) continue;
      ++nTables;
      const string tableName = hepdata_table_name(key->GetName());
      if (tableName != key->GetName()) renamed.emplace_back(key->GetName(), tableName);
      add_variables(table, out.mkdir(tableName.data()), tableName, notes);
      // the titles of the objects are the DOI of their table, which ends with the version of the record
      std::smatch match;
      for (auto* objKey : TRangeDynCast<TKey>(table->GetListOfKeys())) {
        const string title = (objKey) ? objKey->GetTitle() : "";
        if (version.empty() && std::regex_search(title, match, doi)) version = match[1];
      }
    }
    out.Write();
  }
  if (!nTables) {
    std::filesystem::remove(tmpFile, ec);
    logger::warning("Cannot use {}: it contains no tables.", label);
    return false;
  }
  if (!replace_file(tmpFile, file, ec)) {
    logger::warning("Cannot write the copy of {} to {} ({}).", label, file, ec.message());
    return false;
  }
  logger::info("Saved {}{} to {}.", label, (version.empty()) ? "" : " (version " + version + ")", file);
  auto listed = [](const vector<string>& tables) {
    const size_t shown = std::min<size_t>(tables.size(), 5);
    const string more = (tables.size() > shown) ? ", and " + std::to_string(tables.size() - shown) + " more" : "";
    return join_strings(vector<string>(tables.begin(), tables.begin() + shown), ", ") + more;
  };
  if (!notes.threeD.empty()) {
    logger::warning("{}: {} not converted: a table with three independent variables has HEPData's own 3d histograms only (Hist3D_y1, ...).", label, listed(notes.threeD));
  }
  if (!notes.overlapping.empty()) {
    logger::warning("{}: no objects per error source for {}: the bins overlap, so HEPData's ROOT file does not hold the error of each point (y<i> has the total errors).",
                    label, listed(notes.overlapping));
  }
  if (!notes.withoutBins.empty()) {
    const bool one = (notes.withoutBins.size() == 1);
    logger::info("{}: {} {} no bins: HEPData's ROOT file holds {} total errors only (y<i>, no y<i>_<label>).", label, listed(notes.withoutBins),
                 (one) ? "has" : "have", (one) ? "its" : "their");
  }
  for (const auto& [original, tableName] : renamed) {
    logger::info("{}: table '{}' is '{}' in the copy, as a data name cannot contain ':'.", label, original, tableName);
  }
  return true;
}
}  // namespace

bool is_hepdata_input(const string& input)
{
  return str_starts_with(input, kHepDataScheme);
}

bool is_hepdata_record_id(const string& recordOrFile)
{
  return !recordOrFile.empty() && std::all_of(recordOrFile.begin(), recordOrFile.end(), [](char c) { return std::isalnum(static_cast<unsigned char>(c)); });
}

std::pair<string, string> split_hepdata_input(const string& input)
{
  const string rest = input.substr(std::min(input.size(), kHepDataScheme.size()));
  const size_t colon = rest.find(':');
  if (is_hepdata_record_id(rest.substr(0, colon))) return {rest.substr(0, colon), (colon == string::npos) ? "" : rest.substr(colon + 1)};
  // a file: the folder follows the ending of its name
  for (const string ending : {".root:", ".tar.gz:", ".tgz:"}) {
    const size_t pos = rest.find(ending);
    if (pos != string::npos) return {rest.substr(0, pos + ending.size() - 1), rest.substr(pos + ending.size())};
  }
  return {rest, ""};
}

string hepdata_table_name(string table)
{
  std::replace(table.begin(), table.end(), ':', '_');
  return table;
}

std::optional<string> hepdata_file(const string& record, const std::filesystem::path& folder)
{
  const string file = (folder / "hepdata" / (record + ".root")).string();
  std::error_code ec;
  if (std::filesystem::exists(file, ec)) return file;
  if (failed_before(file)) return std::nullopt;
  bool made = false;
  auto rememberFailure = make_scope_guard([&]() {
    if (!made) remember_failure(file);
  });
  std::filesystem::create_directories(std::filesystem::path(file).parent_path(), ec);
  if (ec) {
    logger::warning("Cannot download HEPData record {}: cannot create folder {} ({}).", record, std::filesystem::path(file).parent_path().string(), ec.message());
    return std::nullopt;
  }

  // downloaded with curl (ROOT opens https only when built with Davix); the server can be replaced (tests, mirrors)
  const char* server = std::getenv("SCIROOPLOT_HEPDATA_SERVER");
  const string url = string((server) ? server : "https://www.hepdata.net") + "/record/" + record + "?format=root";
  const string download = temporary_file(file) + ".download";
  logger::info("Downloading HEPData record {} from {}.", record, url);
  Int_t status{};
  TString output;
  {
    auto quiet = ignore_root_messages_below(kFatal);  // ROOT reports a failed command itself
    output = gSystem->GetFromPipe(("curl -sSfL --connect-timeout 30 --max-time 600 -o " + shell_quoted(download) + " " + shell_quoted(url)).data(), &status, true);
  }
  auto removeDownload = make_scope_guard([&download]() {
    std::error_code removeError;
    std::filesystem::remove(download, removeError);
  });
  if (status != 0) {
    const bool noCurl = WIFEXITED(status) && WEXITSTATUS(status) == 127;
    logger::warning("Cannot download HEPData record {}: {}.", record, (noCurl) ? "the program curl was not found" : string(output.Data()));
    return std::nullopt;
  }
  made = make_hepdata_copy(download, file, "HEPData record " + record);
  if (!made) return std::nullopt;
  return file;
}

std::optional<string> hepdata_local_file(const string& given, const std::filesystem::path& folder)
{
  const auto expanded = expand_path(given);
  if (!expanded) return std::nullopt;  // reported
  std::error_code ec;
  const std::filesystem::path source = std::filesystem::absolute(*expanded, ec).lexically_normal();
  if (given.empty() || ec || !std::filesystem::is_regular_file(source, ec)) {
    if (!failed_before(string(kHepDataScheme) + given)) {
      logger::warning("Skipping input {}{}: neither a HEPData record (its INSPIRE id, ins1419244, or its record number) nor an existing file.", kHepDataScheme, given);
      remember_failure(string(kHepDataScheme) + given);
    }
    return std::nullopt;
  }
  // the copy is named after the file and its path, and made again when the file is newer than it
  const string file = (folder / "hepdata" / "files" / (source.stem().string() + "-" + std::to_string(std::hash<string>{}(source.string())) + ".root")).string();
  std::error_code timeError;
  if (std::filesystem::exists(file, ec) && std::filesystem::last_write_time(file, timeError) >= std::filesystem::last_write_time(source, timeError) && !timeError) return file;
  if (failed_before(file)) return std::nullopt;
  std::filesystem::create_directories(std::filesystem::path(file).parent_path(), ec);
  if (ec) {
    logger::warning("Cannot convert HEPData file {}: cannot create folder {} ({}).", source.string(), std::filesystem::path(file).parent_path().string(), ec.message());
    remember_failure(file);
    return std::nullopt;
  }
  if (!make_hepdata_copy(source.string(), file, "HEPData file " + source.string())) {
    remember_failure(file);
    return std::nullopt;
  }
  return file;
}
}  // namespace SciRooPlot::util
