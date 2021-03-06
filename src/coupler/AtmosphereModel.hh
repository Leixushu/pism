// Copyright (C) 2008-2017 Ed Bueler, Constantine Khroulev, Ricarda Winkelmann,
// Gudfinna Adalgeirsdottir and Andy Aschwanden
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

#ifndef __AtmosphereModel
#define __AtmosphereModel

#include <vector>

#include "pism/util/Component.hh"

namespace pism {
class IceModelVec2S;
//! @brief Atmosphere models and modifiers: provide precipitation and
//! temperature to a surface::SurfaceModel below
namespace atmosphere {
//! A purely virtual class defining the interface of a PISM Atmosphere Model.
class AtmosphereModel : public Component_TS {
public:
  AtmosphereModel(IceGrid::ConstPtr g);
  virtual ~AtmosphereModel();

  void init();

  //! \brief Sets result to the mean precipitation, in m/s ice equivalent.
  void mean_precipitation(IceModelVec2S &result) const;

  //! \brief Sets result to the mean annual near-surface air temperature, in degrees Kelvin.
  void mean_annual_temp(IceModelVec2S &result) const;

  void begin_pointwise_access() const;
  void end_pointwise_access() const;
  void init_timeseries(const std::vector<double> &ts) const;
  //! \brief Sets a pre-allocated N-element array "result" to the time-series of
  //! ice-equivalent precipitation (m/s) at the point i,j on the grid.
  //!
  //! See temp_time_series() for more.
  void precip_time_series(int i, int j, std::vector<double> &result) const;

  //! \brief Sets a pre-allocated N-element array "result" to the time-series
  //! of near-surface air temperature (degrees Kelvin) at the point i,j on the
  //! grid. Times (in years) are specified in ts. NB! Has to be surrounded by
  //! begin_pointwise_access() and end_pointwise_access()
  void temp_time_series(int i, int j, std::vector<double> &result) const;
protected:
  virtual void init_impl() = 0;
  virtual void mean_precipitation_impl(IceModelVec2S &result) const = 0;
  virtual void mean_annual_temp_impl(IceModelVec2S &result) const = 0;
  virtual void begin_pointwise_access_impl() const = 0;
  virtual void end_pointwise_access_impl() const = 0;
  virtual void init_timeseries_impl(const std::vector<double> &ts) const = 0;
  virtual void precip_time_series_impl(int i, int j, std::vector<double> &result) const = 0;
  virtual void temp_time_series_impl(int i, int j, std::vector<double> &result) const = 0;

  virtual std::map<std::string, Diagnostic::Ptr> diagnostics_impl() const;
protected:
  mutable std::vector<double> m_ts_times;
};

/*! @brief Instantaneous near-surface air temperature. */
class PA_air_temp_snapshot : public Diag<AtmosphereModel>
{
public:
  PA_air_temp_snapshot(const AtmosphereModel *m);
protected:
  IceModelVec::Ptr compute_impl() const;
};

/*! @brief Effective near-surface mean-annual air temperature. */
class PA_air_temp : public Diag<AtmosphereModel>
{
public:
  PA_air_temp(const AtmosphereModel *m);
protected:
  IceModelVec::Ptr compute_impl() const;
};

/*! @brief Effective precipitation rate (average over time step). */
class PA_precipitation : public Diag<AtmosphereModel>
{
public:
  PA_precipitation(const AtmosphereModel *m);
protected:
  IceModelVec::Ptr compute_impl() const;
};

} // end of namespace atmosphere
} // end of namespace pism

#endif  // __AtmosphereModel
