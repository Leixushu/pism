// Copyright (C) 2004-2011, 2013, 2014, 2015, 2016 PISM Authors
//
// This file is part of Pism.
//
// Pism is free software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation; either version 3 of the License, or (at your option) any later
// version.
//
// Pism is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License
// along with Pism; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#include <petsc.h>
#include "base/util/pism_const.hh"
#include "base/rheology/FlowLawFactory.hh"
#include "base/enthalpyConverter.hh"
#include "base/util/pism_options.hh"

#include "base/util/Context.hh"

#include "base/util/petscwrappers/PetscInitializer.hh"
#include "base/util/error_handling.hh"

#include "base/util/PISMConfigInterface.hh"

using namespace pism;

static char help[] =
  "Calls FlowLaw with various values of arguments and prints results.\n"
  "Used for software tests.  Tests the flow() method but prints\n"
  "temperature and liquid fraction as inputs and flow coefficient as output.\n"
  "Thus also tests methods pressure(), melting_temperature(), and\n"
  "enthalpy() methods of EnthalpyConverter.  Nonetheless a change to the\n"
  "enthalpy normalization only should not affect the outcome.  Only physically-\n"
  "meaningful inputs and output appear at stdout.\n";

int main(int argc, char *argv[]) {

  MPI_Comm com = MPI_COMM_WORLD;

  petsc::Initializer petsc(argc, argv, help);

  com = PETSC_COMM_WORLD;

  /* This explicit scoping forces destructors to be called before PetscFinalize() */
  try {

    Context::Ptr ctx = context_from_options(com, "flowlaw_test");

    EnthalpyConverter::Ptr EC(new EnthalpyConverter(*ctx->config()));

    rheology::FlowLaw *flow_law = NULL;
    rheology::FlowLawFactory ice_factory("stress_balance.sia.", ctx->config(), EC);
    flow_law = ice_factory.create();

    double     TpaC[]  = {-30.0, -5.0, 0.0, 0.0},  // pressure-adjusted, deg C
               depth   = 2000.0,
               gs      = 1.0e-3, // some laws use grain size; fixed
               omega0  = 0.005,  // some laws use liquid fraction; used w TpaC[3]
               sigma[] = {1e4, 5e4, 1e5, 1.5e5};

    double     p       = EC->pressure(depth),
               Tm      = EC->melting_temperature(p);

    printf("flow law:   \"%s\"\n", flow_law->name().c_str());
    printf("pressure = %9.3e Pa = (hydrostatic at depth %7.2f m)\n",
           p,depth);
    printf("flowtable:\n");
    printf("  (dev stress)   (abs temp) (liq frac) =   (flow)\n");

    for (int i=0; i<4; ++i) {
      for (int j=0; j<4; ++j) {

        double T     = Tm + TpaC[j],
               omega = (j == 3) ? omega0 : 0.0;

        double E = EC->enthalpy(T, omega, p);
        double flowcoeff = flow_law->flow(sigma[i], E, p, gs);

        printf("    %10.2e   %10.3f  %9.3f = %10.6e\n",
               sigma[i], T, omega, flowcoeff);

      }
    }

    delete flow_law;
  }
  catch (...) {
    handle_fatal_errors(com);
    return 1;
  }

  return 0;
}
