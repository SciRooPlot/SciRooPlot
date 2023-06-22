/*
********************************************************************************
* --------------------------------- SciRooPlot ---------------------------------
* Copyright (c) 2019-2023 Mario Krüger
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
* The GNU General Public License can be found here: <http://www.gnu.org/licenses/>.
*******************************************************************************
*/

#include "SciRooPlot.h"
#include "PlotManager.h"
#include "Plot.h"
#include "Logging.h"

#include <boost/program_options.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "Helpers.h"

using namespace SciRooPlot;
namespace po = boost::program_options;

int main(int argc, char* argv[])
{
  string configFileName = (gSystem->Getenv("__PLOTTING_CONFIG_FILE"))
                            ? expand_path("${__PLOTTING_CONFIG_FILE}")
                            : "~/.plotconfig.xml";
  configFileName = expand_path(configFileName);

  ptree configTree;
  string activeConfig;
  if (file_exists(configFileName)) {
    using boost::property_tree::read_xml;
    read_xml(configFileName, configTree, boost::property_tree::xml_parser::trim_whitespace);
    if (auto activated = configTree.get_child_optional("activated")) {
      activeConfig = activated.get().data();
    }
  }

  string mode;
  std::vector<std::string> args;

  try {
    po::options_description arguments("positional arguments");
    arguments.add_options()("mode", po::value<string>(), "mode")("args", po::value<std::vector<std::string>>(), "args");
    po::positional_options_description pos;
    pos.add("mode", 1);
    pos.add("args", 5);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(arguments).positional(pos).run(), vm);
    po::notify(vm);

    if (vm.count("mode")) {
      mode = vm["mode"].as<string>();
    }
    if (vm.count("args")) {
      args = vm["args"].as<std::vector<std::string>>();
    }
  } catch (std::exception& e) {
    ERROR(R"(Exception "{}"! Exiting.)", e.what());
    return 1;
  } catch (...) {
    ERROR("Exception of unknown type! Exiting.");
    return 1;
  }

  if (mode == "add") {
    if (args.size() < 3) {
      ERROR("Not enough arguments.");
      return 1;
    }
    string configName = args[0];
    string property = args[1];
    string value = args[2];

    static const vector<string> options = {
      {"plotDefinitions", "inputFiles", "outputDir", "executable"}};
    if (!std::count(options.begin(), options.end(), property)) {
      ERROR("Illegal property {}.", property);
      return 1;
    }

    if (auto curConfig = configTree.get_child_optional(configName)) {
      // TODO: we may want multiple input files and plot definitions
      curConfig.get().erase(property);
    }
    configTree.add(configName + "." + property, value);

    if (activeConfig.empty()) {
      activeConfig = configName;
    }
  } else if (mode == "remove") {
    if (args.size() < 1) {
      ERROR("Not enough arguments.");
      return 1;
    }

    string configName = args[0];
    configTree.erase(configName);
    if (configName == activeConfig) {
      activeConfig.clear();
    }
  } else if (mode == "get") {
    if (args.size() < 1) {
      ERROR("Not enough arguments.");
      return 1;
    }
    string request = args[0];
    if (activeConfig.empty()) {
      return 1;
    }
    if (request == "active") {
      std::cout << activeConfig << std::endl;
      return 0;
    }
    if (auto curConfig = configTree.get_child_optional(activeConfig)) {
      if (auto property = curConfig.get().get_child_optional(request)) {
        std::cout << property->get_value<string>() << std::endl;
        return 0;
      }
    }
  } else if (mode == "switch") {
    if (args.size() < 1) {
      ERROR("Not enough arguments.");
      return 1;
    }
    if (auto it = configTree.find(args[0]); it != configTree.not_found()) {
      activeConfig = args[0];
    } else {
      ERROR("Cannot switch to undefined configuration {}.", args[0]);
    }
  } else if (mode == "reset") {
    configTree.clear();
    activeConfig.clear();
  } else if (mode == "list") {
    for (auto& entry : configTree) {
      string configName = entry.first;
      if (configName == "activated") continue;
      if (configName == activeConfig) {
        PRINT("* {}", configName);
      } else {
        PRINT("  {}", configName);
      }
    }

  } else {
    ERROR("Invalid arguments.");
  }

  configTree.erase("activated");
  configTree.add("activated", activeConfig);

  using boost::property_tree::xml_writer_settings;
  xml_writer_settings<std::string> settings('\t', 1);
  using boost::property_tree::write_xml;
  write_xml(configFileName, configTree, std::locale(), settings);

  return 0;
}
