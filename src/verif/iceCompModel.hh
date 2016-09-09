// Copyright (C) 2004-2016 Jed Brown, Ed Bueler and Constantine Khroulev
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

#ifndef __iceCompModel_hh
#define __iceCompModel_hh

#include "base/iceModel.hh"
#include "base/energy/BedThermalUnit.hh"

namespace pism {

class IceCompModel : public IceModel {

public:
  IceCompModel(IceGrid::Ptr g, Context::Ptr ctx, int mytest);
  virtual ~IceCompModel() {}
  
  // re-defined steps of init() sequence:
  virtual void setFromOptions();
  virtual void createVecs();
  virtual void allocate_stressbalance();
  virtual void allocate_bedrock_thermal_unit();
  virtual void allocate_bed_deformation();
  virtual void allocate_couplers();

  virtual void bootstrap_2d(const PIO &input_file);
  virtual void bootstrap_3d();

  virtual void initialize_2d();
  virtual void initialize_3d();

  void reportErrors();

protected:
  // related to all (or most) tests
  bool exactOnly;
  int testname;
  virtual void additionalAtStartTimestep();
  virtual void additionalAtEndTimestep();
  // all tests except K
  void computeGeometryErrors(double &gvolexact, double &gareaexact, double &gdomeHexact,
                                       double &volerr, double &areaerr,
                                       double &gmaxHerr, double &gavHerr, double &gmaxetaerr,
                                       double &centerHerr);
  virtual void summary(bool tempAndAge);

  // related to tests A B C D H
  void initTestABCDH();
  void fillSolnTestABCDH();  // only used with exactOnly == true
  
  // related to test E
  void fillSolnTestE();  // only used with exactOnly == true

  // test E only
  void computeBasalVelocityErrors(double &exactmaxspeed,
                                            double &gmaxvecerr, double &gavvecerr,
                                            double &gmaxuberr, double &gmaxvberr);

  void reset_thickness_test_A();

  // related to test L
  IceModelVec2S   vHexactL;
  void initTestL();
  void fillSolnTestL();  // only used with exactOnly == true

  // related to tests F G; see iCMthermo.cc
  virtual void temperatureStep(unsigned int* vertSacrCount, unsigned int* bulgeCount);
  void initTestFG();
  void getCompSourcesTestFG();
  void fillSolnTestFG();  // only used with exactOnly == true
  // tests F and G
  void computeTemperatureErrors(double &gmaxTerr, double &gavTerr);
  // tests F and G
  void computeBasalTemperatureErrors(double &gmaxTerr, double &gavTerr, double &centerTerr);
  // tests F and G
  void compute_strain_heating_errors(double &gmax_strain_heating_err, double &gav_strain_heating_err);

  // tests F and G
  void computeSurfaceVelocityErrors(double &gmaxUerr, double &gavUerr,  // 2D vector errors
                                              double &gmaxWerr, double &gavWerr); // scalar errors
  
  IceModelVec3   strain_heating3_comp;

  // related to tests K and O; see iCMthermo.cc
  void initTestsKO();
  void fillTemperatureSolnTestsKO();  // used in initialzation
  //   and with exactOnly == true
  void fillBasalMeltRateSolnTestO();  // used only with exactOnly == true
 // tests K and O only
  void computeIceBedrockTemperatureErrors(double &gmaxTerr, double &gavTerr,
                                                    double &gmaxTberr, double &gavTberr);
  // test O only
  void computeBasalMeltRateErrors(double &gmaxbmelterr, double &gminbmelterr);

  // using Van der Veen's exact solution to test CFBC and the part-grid code
  void test_V_init();

  static const double secpera;

private:
  double f;       // ratio of ice density to bedrock density
  bool bedrock_is_ice_forK;

  // see iCMthermo.cc
  static const double ST;      // K m^-1;  surface temperature gradient: T_s = ST * r + Tmin
  static const double Tmin;    // K;       minimum temperature (at center)
  static const double LforFG;  // m;  exact radius of tests F&G ice sheet
  static const double ApforG;  // m;  magnitude A_p of annular perturbation for test G;
  // period t_p is set internally to 2000 years
};

} // end of namespace pism

#endif /* __iceCompModel_hh */
