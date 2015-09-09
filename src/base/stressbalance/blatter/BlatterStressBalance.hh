// Copyright (C) 2010-2015 Jed Brown, Ed Bueler and Constantine Khroulev
//
// This file is part of PISM.
//
// PISM is free software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation; either version 3 of the License, or (at your option) any later
// version.
//
// PISM is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License
// along with PISM; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef _BLATTERSTRESSBALANCE_H_
#define _BLATTERSTRESSBALANCE_H_

#include "base/stressbalance/ShallowStressBalance.hh"
#include "base/util/IceGrid.hh"
#include "base/util/IceModelVec3Custom.hh"

#include "base/util/petscwrappers/DM.hh"

#include "Blatter_implementation.h"

namespace pism {
namespace stressbalance {
// Tell the linker that these are called from the C code:
extern "C" {
  void viscosity(void *ctx, double hardness, double gamma,
		 double *eta, double *deta);
  void drag(void *ctx, double tauc, double u, double v,
	    double *taud, double *dtaub);
}

//! \brief Blatter-Pattyn stress balance based on Jed Brown's PETSc
//! tutorial ex48.c (Brown et al. 2011).
/*!
Toy hydrostatic ice flow with multigrid in 3D

Solves the hydrostatic (aka Blatter/Pattyn/First Order) equations for ice sheet
flow using multigrid.  The ice uses a power-law rheology with Glen exponent 3
(corresponds to p=4/3 in a p-Laplacian).

The equations for horizontal velocity @f$ (u,v) @f$ are
@f{align*}
  - [\eta (4 u_x + 2 v_y)]_x - [\eta (u_y + v_x)]_y - [\eta u_z]_z + \rho g s_x &= 0 \\
  - [\eta (4 v_y + 2 u_x)]_y - [\eta (u_y + v_x)]_x - [\eta v_z]_z + \rho g s_y &= 0
@f}
where
  @f[\eta = B/2 (\epsilon + \gamma)^{(p-2)/2}.@f]
is the nonlinear effective viscosity with regularization epsilon and hardness
parameter B, written in terms of the second invariant

@f[\gamma = u_x^2 + v_y^2 + u_x v_y + \frac{1}{4} (u_y + v_x)^2 + \frac{1}{4} u_z^2 + \frac{1}{4} v_z^2.@f]

The surface boundary conditions are the natural conditions,
corresponding to the "zero stress" condition (see [\ref RappazReist05]). The
basal boundary conditions are either no-slip, or a pseudo-plastic
sliding law (see IceBasalResistancePlasticLaw).

In the code, the equations for @f$ (u,v) @f$ are multiplied through by @f$ 1/(\rho g) @f$ 
so that residuals are @f$ O(1) @f$.

The discretization is @f$ Q_1 @f$ finite elements, managed by a DA.  The grid is never
distorted in the map @f$ (x,y) @f$ plane, but the bed and surface may be bumpy.
This is handled as usual in FEM, through the Jacobian of the coordinate
transformation from a reference element to the physical element.

Since ice-flow is tightly coupled in the z-direction (within columns), the DA is
managed specially so that columns are never distributed, and are always
contiguous in memory.  This amounts to reversing the meaning of X,Y,Z compared
to the DA's internal interpretation, and then indexing as `vec[i][j][k]`.  The
exotic coarse spaces require 2D DAs which are made to use compatible domain
decomposition relative to the 3D DAs.

Note that this implementation introduces two extra simplifications
compatible with the small bed slope assumption:

- the code evaluating the integral corresponding to the basal boundary
  condition assumes that the Jacobian of the map from the 2D reference
  element is @f$ J = \frac{1}{4} \Delta x \times \Delta y @f$, which is
  correct only if the bed is a horizontal plane.

- it assumes that the horizontal ice velocity at the base approximates
  the tangential basal ice velocity, which is also correct if the base
  of the ice is horizontal.

See the source code `$PETSC_DIR/src/snes/examples/tutorials/ex48.c` for
the original implementation.
 */
class BlatterStressBalance : public ShallowStressBalance {
  friend void viscosity(void *ctx, double hardness, double gamma,
			double *eta, double *deta);
  friend void drag(void *ctx, double tauc, double u, double v,
		   double *taud, double *dtaub);
public:
  BlatterStressBalance(IceGrid::ConstPtr g,
                       EnthalpyConverter::Ptr e);

  virtual ~BlatterStressBalance();

  virtual void init();

  virtual std::string stdout_report();

  virtual void update(bool fast,
                      const IceModelVec2S &melange_back_pressure);

  const IceModelVec3& velocity_u() const;
  const IceModelVec3& velocity_v() const;

  virtual void add_vars_to_output(const std::string &/*keyword*/, std::set<std::string> &result);

  virtual PetscErrorCode define_variables(const std::set<std::string> &/*vars*/, const PIO &/*nc*/,
                                          IO_Type /*nctype*/);

  virtual PetscErrorCode write_variables(const std::set<std::string> &/*vars*/, const PIO &/*nc*/);

protected: 
  void transfer_velocity();
  void initialize_ice_hardness();
  void setup();

  void compute_volumetric_strain_heating();
  void save_velocity();

  IceModelVec3 m_u, m_v, m_strain_heating;

  const IceModelVec2S *bed_elevation, *ice_thickness, *tauc;
  const IceModelVec3 *enthalpy;
  IceModelVec3Custom m_u_sigma, m_v_sigma; // u and v components on the "sigma" vertical grid

  BlatterQ1Ctx m_ctx;
  SNES m_snes;

  petsc::DM::Ptr m_da2;

  double m_min_thickness; 	// FIXME: this should be used to set boundary conditions at ice margins
  std::string m_stdout_blatter;
};

} // end of namespace stressbalance
} // end of namespace pism

#endif /* _BLATTERSTRESSBALANCE_H_ */
