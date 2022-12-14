// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT

#include "hit.h"

#include <cassert>
#include <ostream>

#include "cluster.h"

namespace proteus {

Hit::Hit(int col, int row, int timestamp, int value)
    : m_digitalCol(col)
    , m_digitalRow(row)
    , m_col(col)
    , m_row(row)
    , m_timestamp(timestamp)
    , m_value(value)
    , m_region(kInvalidIndex)
    , m_cluster(kInvalidIndex)
{
}

void Hit::setPhysicalAddress(int col, int row)
{
  m_col = col;
  m_row = row;
}

void Hit::setCluster(Index cluster)
{
  assert((m_cluster == kInvalidIndex) && "hit can only be in one cluster");
  m_cluster = cluster;
}

std::ostream& operator<<(std::ostream& os, const Hit& hit)
{
  if ((hit.digitalCol() != hit.col()) || (hit.digitalRow() != hit.row())) {
    os << "addr0=" << hit.digitalCol();
    os << " addr1=" << hit.digitalRow();
    os << " ";
  }
  os << "col=" << hit.col();
  os << " row=" << hit.row();
  os << " ts=" << hit.timestamp();
  os << " value=" << hit.value();
  if (hit.hasRegion()) {
    os << " region=" << hit.region();
  }
  if (hit.isInCluster()) {
    os << " cluster=" << hit.cluster();
  }
  return os;
}

} // namespace proteus
