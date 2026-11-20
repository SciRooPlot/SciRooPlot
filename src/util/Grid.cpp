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

#include "util/Grid.h"

#include <RVersion.h>
#include <TSystem.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// TGrid is deprecated in ROOT 6.40 and removed in 6.42, together with the grid plugin (JAliEn) built on it
#define SCIROOPLOT_HAS_GRID (ROOT_VERSION_CODE < ROOT_VERSION(6, 42, 0))
#if SCIROOPLOT_HAS_GRID
#include <TGrid.h>
#include <TGridResult.h>
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

namespace SciRooPlot::util
{
namespace
{
constexpr const char* kGridScheme = "alien://";
// the result of the one connection of the process, and why it failed
struct grid_connection_t {
  bool connected{};
  std::string problem;
};
const grid_connection_t& grid_connection()
{
  static grid_connection_t connection;
  static std::once_flag once;
  std::call_once(once, []() {
#if SCIROOPLOT_HAS_GRID
    // without the certificates of the grid's certificate authorities JAliEn tries server after server for minutes: refused here
    const char* certDir = gSystem->Getenv("X509_CERT_DIR");
    std::error_code ec;
    if (!(certDir && std::filesystem::is_directory(certDir, ec)) && !std::filesystem::is_directory("/etc/grid-security/certificates", ec)) {
      connection.problem = (certDir) ? std::string("X509_CERT_DIR=") + certDir + " is no directory" : "X509_CERT_DIR is not set (the directory of the grid's CA certificates)";
      return;
    }
    TGrid::Connect(kGridScheme);
    connection.connected = gGrid && !gGrid->IsZombie();
    if (!connection.connected) connection.problem = "the connection failed: check the token (alien-token-info) and the CA certificates (X509_CERT_DIR)";
#else
    connection.problem = "this ROOT version has no grid support (TGrid was removed in ROOT 6.42)";
#endif
  });
  return connection;
}
// the path of a grid url within the grid catalogue (alien:///alice/x -> /alice/x)
std::string catalogue_path(const std::string& url)
{
  return url.substr(std::string(kGridScheme).size());
}
}  // namespace

bool is_grid_path(const std::string& path)
{
  return path.rfind("alien:", 0) == 0;
}

bool connect_grid_for(const std::string& path)
{
  return !is_grid_path(path) || grid_connection().connected;
}

std::string grid_failure_reason(const std::string& path)
{
  if (!is_grid_path(path)) return "";
  if (!grid_connection().connected) return ": no connection to the grid, " + grid_connection().problem;
  return ": it cannot be read from the grid (does it exist? is the token valid: alien-token-info?)";
}

std::optional<std::vector<std::string>> list_grid_directory(const std::string& url)
{
#if SCIROOPLOT_HAS_GRID
  if (!is_grid_path(url) || !connect_grid_for(url)) return std::nullopt;
  std::string dir = catalogue_path(url);
  while (dir.size() > 1 && dir.back() == '/')
    dir.pop_back();
  // -F marks directories with a trailing /; a file is listed as itself
  std::unique_ptr<TGridResult> result(gGrid->Ls(dir.data(), "-F"));
  if (!result) return std::nullopt;
  std::vector<std::string> names;
  for (Int_t i = 0; i < result->GetEntries(); ++i) {
    std::string name = result->GetFileName(i);
    if (result->GetEntries() == 1 && name.back() != '/' && dir.size() >= name.size() && dir.compare(dir.size() - name.size(), name.size(), name) == 0) return std::nullopt;
    if (!name.empty() && name.back() == '/') name.pop_back();
    if (!name.empty()) names.push_back(name);
  }
  std::sort(names.begin(), names.end());
  return names;
#else
  return std::nullopt;
#endif
}

bool grid_path_exists(const std::string& url)
{
#if SCIROOPLOT_HAS_GRID
  if (!is_grid_path(url) || !connect_grid_for(url)) return false;
  return std::unique_ptr<TGridResult>(gGrid->Ls(catalogue_path(url).data())) != nullptr;
#else
  return false;
#endif
}
}  // namespace SciRooPlot::util
