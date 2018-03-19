// Copyright (C) 2012-2016, 2018 Ricarda Winkelmann, Ronja Reese, Torsten Albrecht
// and Matthias Mengel
//
// This file is part of PISM.
//
// PISM is free software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
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

#ifndef _POPICO_H_
#define _POPICO_H_

#include "CompleteOceanModel.hh"

#include "pism/coupler/util/PGivenClimate.hh"
#include "pism/util/IceModelVec2CellType.hh"

namespace pism {
namespace ocean {

struct TocBox1 {
  bool failed;
  double value;
};

class BoxModel {
public:
  BoxModel(const Config &config);

  double pressure(double ice_thickness) const;
  double T_star(double salinity, double temperature, double pressure) const;

  TocBox1 Toc_box1(double area, double T_star,
                   double Soc_box0, double Toc_box0) const;
  double Soc_box1(double Toc_box0, double Soc_box0, double Toc) const;

  double Toc(double box_area,
             double temperature, double T_star,
             double overturning, double salinity) const;

  double Soc(double salinity, double temperature, double Toc) const;

  double theta_pm(double salinity, double pressure) const;
  double T_pm(double salinity, double pressure) const;

  double melt_rate(double pm_point, double Toc) const;

  double melt_rate_beckmann_goose(double pot_pm_point, double Toc) const;

  double overturning(double Soc_box0, double Soc,
                     double Toc_box0, double Toc) const;

  double gamma_T() const;
  double overturning_coeff() const;
  double T_dummy() const;
  double S_dummy() const;
  double ice_density() const;
  double continental_shelf_depth() const;
private:
  double p_coeff(double g1, double s1) const;
  double q_coeff(double g1, double s1, double T_star) const;

  double m_gamma_T, m_overturning_coeff, m_T_dummy, m_S_dummy;
  double m_ice_density, m_continental_shelf_depth;

  double m_earth_grav, m_sea_water_density, m_rho_star, m_nu, m_latentHeat,
    m_c_p_ocean, m_alpha, m_beta;

  double m_lambda;

  // coefficients of the parameterization of the potential temperature
  double m_a_pot, m_b_pot, m_c_pot;

  // coefficients of the parameterization of the in situ temperature
  double m_a_in_situ, m_b_in_situ, m_c_in_situ;

  double m_meltFactor;
};

//! \brief Implements the PICO ocean model as submitted to The Cryosphere (March 2017).
//!
//! Generalizes the two dimensional ocean box model of [@ref OlbersHellmer2010] for
//! use in PISM, i.e. three dimensions.
//!
class Pico : public PGivenClimate<CompleteOceanModel, CompleteOceanModel> {
public:
  Pico(IceGrid::ConstPtr g);
  virtual ~Pico();

protected:
  void update_impl(double t, double dt);
  void init_impl();

  void define_model_state_impl(const PIO &output) const;
  void write_model_state_impl(const PIO &output) const;

  std::map<std::string, Diagnostic::Ptr> diagnostics_impl() const;

  bool m_exicerises_set; // FIXME shouldn't this be always used?

private:
  IceModelVec2S m_Soc, m_Soc_box0;
  IceModelVec2S m_Toc, m_Toc_box0, m_T_star;
  IceModelVec2S m_overturning;
  IceModelVec2S m_basal_melt_rate;

  // a basin defines the domain where one box model instance is solved
  IceModelVec2Int m_DistGL, m_DistIF, m_icerise_mask, m_basin_mask, m_shelf_mask, m_lake_mask,
    m_ocean_box_mask, m_ocean_mask, m_continental_shelf_mask;

  IceModelVec2T *m_theta_ocean, *m_salinity_ocean;

  void identifyMASK(IceModelVec2S &inputmask, std::string masktype);
  void identify_shelf_mask();
  void compute_ocean_input_per_basin(const BoxModel &box_model,
                                     const IceModelVec2Int &basin_mask,
                                     const IceModelVec2Int &continental_shelf_mask,
                                     const IceModelVec2S &salinity_ocean,
                                     const IceModelVec2S &theta_ocean,
                                     std::vector<double> &temperature,
                                     std::vector<double> &salinity);
  void set_ocean_input_fields(const BoxModel &box_model,
                              const IceModelVec2S &ice_thickness,
                              const IceModelVec2CellType &mask,
                              const IceModelVec2Int &basin_mask,
                              const IceModelVec2Int &shelf_mask,
                              const std::vector<double> basin_temperature,
                              const std::vector<double> basin_salinity,
                              IceModelVec2S &Toc_box0,
                              IceModelVec2S &Soc_box0);

  void compute_distances();
  void identify_ocean_box_mask(const BoxModel &constants);

  void process_box1(const IceModelVec2S &ice_thickness,
                    const IceModelVec2Int &shelf_mask,
                    const IceModelVec2Int &box_mask,
                    const IceModelVec2S &Toc_box0,
                    const IceModelVec2S &Soc_box0,
                    const BoxModel &cc,
                    IceModelVec2S &T_star,
                    IceModelVec2S &Toc,
                    IceModelVec2S &Soc,
                    IceModelVec2S &basal_melt_rate,
                    IceModelVec2S &overturning,
                    IceModelVec2S &T_pressure_melting);

  void process_other_boxes(const IceModelVec2S &ice_thickness,
                           const IceModelVec2Int &shelf_mask,
                           const BoxModel &cc,
                           IceModelVec2Int &box_mask,
                           IceModelVec2S &T_star,
                           IceModelVec2S &Toc,
                           IceModelVec2S &Soc,
                           IceModelVec2S &basal_melt_rate,
                           IceModelVec2S &T_pressure_melting);

  void process_missing_cells(const BoxModel &cc,
                             const IceModelVec2Int &shelf_mask,
                             const IceModelVec2Int &box_mask,
                             const IceModelVec2S &ice_thickness,
                             const IceModelVec2S &Toc_box0,
                             const IceModelVec2S &Soc_box0,
                             IceModelVec2S &Toc,
                             IceModelVec2S &Soc,
                             IceModelVec2S &basal_melt_rate,
                             IceModelVec2S &T_pressure_melting);

  void compute_box_average(int box_id,
                           const IceModelVec2S &field,
                           const IceModelVec2Int &shelf_mask,
                           const IceModelVec2Int &box_mask,
                           std::vector<double> &result);

  void compute_box_area(int box_id,
                        const IceModelVec2Int &shelf_mask,
                        const IceModelVec2Int &box_mask,
                        const IceModelVec2S &cell_area,
                        std::vector<double> &result);

  enum IdentifyMaskFlags {INNER = 2, OUTER = 0, EXCLUDE = 1, UNIDENTIFIED = -1};

  int m_n_basins, m_n_boxes, m_n_shelves, m_Mx, m_My;
};

void round_basins(IceModelVec2S &basin_mask);

} // end of namespace ocean
} // end of namespace pism

#endif /* _POPICO_H_ */