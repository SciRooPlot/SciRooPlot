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
#include <TKey.h>
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

// the class of the object behind a key, if it can be read (objects of an emulated class without dictionary cannot be read as TObject)
inline TClass* readable_class(TKey* key)
{
  TClass* cl = TClass::GetClass(key->GetClassName());
  return (cl && cl->IsLoaded() && cl->IsTObject()) ? cl : nullptr;
}

// whether the object behind a key is of the given class (e.g. a directory or a tree), without reading it
inline bool key_inherits(TKey* key, const TClass* base)
{
  const TClass* cl = TClass::GetClass(key->GetClassName());
  return cl && cl->InheritsFrom(base);
}

// the latest key of a name in a directory that holds a directory, folder or collection: a newer key of the same name holding data
// does not hide it (open a directory found this way with open_directory); the latest key of the name if none holds one
inline TKey* find_container_key(TDirectory* directory, const std::string& name)
{
  TKey* latest = directory->FindKey(name.data());
  if (!latest || is_container(readable_class(latest))) return latest;
  for (auto* key : TRangeDynCast<TKey>(directory->GetListOfKeys())) {  // the cycles of a name are listed from the latest
    if (key && name == key->GetName() && is_container(readable_class(key))) return key;
  }
  return latest;
}

// the sub-directory behind a key of a directory: by its name (the instance ROOT keeps in memory), by name and cycle only where a newer
// key of the name hides it from a lookup by name (ROOT then reads it again)
inline TDirectory* open_directory(TDirectory* directory, TKey* key)
{
  const bool isLatest = (directory->FindKey(key->GetName()) == key);
  return directory->GetDirectory(isLatest ? key->GetName() : (std::string(key->GetName()) + ";" + std::to_string(key->GetCycle())).data());
}
}  // namespace SciRooPlot::util

#endif  // SRC_UTIL_ROOTCONTAINERS_H_
