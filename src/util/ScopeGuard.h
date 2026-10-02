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

#ifndef SRC_UTIL_SCOPEGUARD_H_
#define SRC_UTIL_SCOPEGUARD_H_

#include <type_traits>
#include <utility>

namespace SciRooPlot::util
{
// calls the given function when it goes out of scope (unless dismissed)
template <typename F>
class ScopeGuard
{
 public:
  explicit ScopeGuard(F onExit) : mOnExit(std::move(onExit)) {}
  ~ScopeGuard()
  {
    if (mActive) mOnExit();
  }

  ScopeGuard(const ScopeGuard&) = delete;
  ScopeGuard& operator=(const ScopeGuard&) = delete;
  ScopeGuard& operator=(ScopeGuard&&) = delete;

  ScopeGuard(ScopeGuard&& other) noexcept : mOnExit(std::move(other.mOnExit)), mActive(other.mActive)
  {
    other.mActive = false;
  }

  void Dismiss() { mActive = false; }

 private:
  F mOnExit;
  bool mActive{true};
};

template <typename F>
ScopeGuard<std::decay_t<F>> make_scope_guard(F&& onExit)
{
  return ScopeGuard<std::decay_t<F>>(std::forward<F>(onExit));
}
}  // end namespace SciRooPlot::util
#endif  // SRC_UTIL_SCOPEGUARD_H_
