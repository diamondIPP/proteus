// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2018-12
 */

#pragma once

#include <algorithm>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

namespace proteus {
namespace detail {

/** Uniquely associate elements from n groups.
 *
 * Elements in each group are identified using a group-specific identifier,
 * e.g. a pointer or an index. This class stores unique association, tuples
 * of n identifiers, one from each group, where an identifier from a group
 * can appear at most once.
 *
 * You can access either the set of identifiers from each group or the combined
 * list of association tuples.
 */
template <typename... Identifiers>
class UniqueAssociation {
public:
  /** Add an association between one element from each group.
   *
   * \retval true  If the association was successfully added
   * \retval false If any of the elements is already in use
   */
  bool add(Identifiers... ids)
  {
    return add_impl(std::index_sequence_for<Identifiers...>(), ids...);
  }
  /** Associated identifiers for group I. */
  template <std::size_t I>
  auto identifiers() const
  {
    return std::get<I>(m_used);
  }
  /** All association tuples. */
  auto associations() const { return m_associations; }

private:
  template <std::size_t... Is>
  bool add_impl(std::index_sequence<Is...>, Identifiers... ids)
  {
    bool unused[] = {(std::get<Is>(m_used).count() == 0)...};
    if (!std::all_of(std::begin(unused), std::end(unused))) {
      return false;
    }
    m_associations.emplace_back(ids...);
    void{std::get<Is>(m_used).emplace(ids)...};
    return true;
  }

  std::vector<std::tuple<Identifiers...>> m_associations;
  std::tuple<std::set<Identifiers>...> m_used;
};

} // namespace detail
} // namespace proteus

#endif
