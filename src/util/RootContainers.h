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

#ifndef SRC_UTIL_ROOTCONTAINERS_H_
#define SRC_UTIL_ROOTCONTAINERS_H_

#include <TClass.h>
#include <TCollection.h>
#include <TDirectory.h>
#include <TFolder.h>
#include <THStack.h>
#include <TMap.h>
#include <TMultiGraph.h>

#include <functional>
#include <string>

namespace SciRooPlot::util
{
// directories, folders and collections (lists, arrays, maps, ...), stacks (their histograms) and multigraphs (their graphs)
// are searched for data; everything else is data
inline bool is_container(const TClass* cl)
{
  return cl && (cl->InheritsFrom(TDirectory::Class()) || cl->InheritsFrom(TFolder::Class()) || cl->InheritsFrom(TCollection::Class()) ||
                cl->InheritsFrom(THStack::Class()) || cl->InheritsFrom(TMultiGraph::Class()));
}

// calls func(name, item) for the items of an in-memory folder or collection (values of maps under the name of their key) until it returns true
inline void for_each_item(TObject* container, const std::function<bool(const std::string&, TObject*)>& func)
{
  // the histograms of a stack and the graphs of a multigraph are its items, as the contents of a list
  TCollection* items = dynamic_cast<TCollection*>(container);
  if (auto* folder = dynamic_cast<TFolder*>(container)) items = folder->GetListOfFolders();
  if (auto* stack = dynamic_cast<THStack*>(container)) items = stack->GetHists();
  if (auto* multiGraph = dynamic_cast<TMultiGraph*>(container)) items = multiGraph->GetListOfGraphs();
  if (!items) return;
  for (TObject* item : *items) {
    if (!item) continue;
    const std::string name = item->GetName();
    if (auto* pair = dynamic_cast<TPair*>(item)) item = pair->Value();  // iterating a map gives its key-value pairs (named by the key)
    if (item && func(name, item)) return;
  }
}
}  // namespace SciRooPlot::util

#endif  // SRC_UTIL_ROOTCONTAINERS_H_
