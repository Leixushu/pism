/* Copyright (C) 2015 PISM Authors
 *
 * This file is part of PISM.
 *
 * PISM is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * PISM is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PISM; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _REGIONALDEFAULTYIELDSTRESS_H_
#define _REGIONALDEFAULTYIELDSTRESS_H_

#include "base/basalstrength/PISMMohrCoulombYieldStress.hh"

namespace pism {

class RegionalDefaultYieldStress : public MohrCoulombYieldStress {
public:
  RegionalDefaultYieldStress(IceGrid::ConstPtr g, hydrology::Hydrology *hydro)
    : MohrCoulombYieldStress(g, hydro) {}
  virtual ~RegionalDefaultYieldStress() {}
  virtual void init();
  virtual const IceModelVec2S& basal_material_yield_stress();
};

} // end of namespace pism

#endif /* _REGIONALDEFAULTYIELDSTRESS_H_ */