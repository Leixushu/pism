"""
Tests of PISM's ocean models and modifiers.
"""

import PISM
import sys, os
import numpy as np
import unittest

def create_dummy_grid():
    "Create a dummy grid"
    ctx = PISM.Context()
    params = PISM.GridParameters(ctx.config)
    params.ownership_ranges_from_options(ctx.size)
    return PISM.IceGrid(ctx.ctx, params)

config = PISM.Context().config
log = PISM.Context().log

def check(vec, value):
    "Check if values of vec are almost equal to value."
    grid = vec.grid()
    with PISM.vec.Access(nocomm=[vec]):
        for (i, j) in grid.points():
            np.testing.assert_almost_equal(vec[i, j], value)

def constant_test():
    "ocean::Constant"
    grid = create_dummy_grid()

    depth = 1000.0                  # meters

    # compute mass flux
    melt_rate   = config.get_double("ocean.constant.melt_rate", "m second-1")
    ice_density = config.get_double("constants.ice.density")
    mass_flux   = melt_rate * ice_density

    # compute pressure melting temperature
    T0          = config.get_double("constants.fresh_water.melting_point_temperature")
    beta_CC     = config.get_double("constants.ice.beta_Clausius_Clapeyron")
    g           = config.get_double("constants.standard_gravity")

    pressure  = ice_density * g * depth
    T_melting = T0 - beta_CC * pressure

    melange_back_pressure = 0.0

    sea_level = 0.0

    # create the model
    ice_thickness = PISM.model.createIceThicknessVec(grid)
    ice_thickness.set(depth)
    grid.variables().add(ice_thickness)

    model = PISM.OceanConstant(grid)

    log.message(1, "\n")
    model.init()
    model.update(0, 1)

    assert model.sea_level_elevation() == sea_level

    check(model.shelf_base_temperature(), T_melting)
    check(model.shelf_base_mass_flux(), mass_flux)
    check(model.melange_back_pressure_fraction(), melange_back_pressure)

def pik_test():
    "ocean::PIK"
    grid = create_dummy_grid()

    depth = 1000.0                  # meters

    # compute pressure melting temperature
    ice_density = config.get_double("constants.ice.density")
    T0          = config.get_double("constants.fresh_water.melting_point_temperature")
    beta_CC     = config.get_double("constants.ice.beta_Clausius_Clapeyron")
    g           = config.get_double("constants.standard_gravity")

    pressure  = ice_density * g * depth
    T_melting = T0 - beta_CC * pressure

    melange_back_pressure = 0.0

    mass_flux = 5.36591610659e-06 # stored mass flux value returned by the model

    sea_level = 0.0

    # create the model
    ice_thickness = PISM.model.createIceThicknessVec(grid)
    ice_thickness.set(depth)
    grid.variables().add(ice_thickness)

    model = PISM.OceanPIK(grid)

    log.message(1, "\n")
    model.init()
    model.update(0, 1)

    assert model.sea_level_elevation() == sea_level

    check(model.shelf_base_temperature(), T_melting)
    check(model.shelf_base_mass_flux(), mass_flux)
    check(model.melange_back_pressure_fraction(), melange_back_pressure)

def create_given_input_file(filename, grid, temperature, mass_flux):
    PISM.util.prepare_output(filename)

    T = PISM.IceModelVec2S(grid, "shelfbtemp", PISM.WITHOUT_GHOSTS)
    T.set_attrs("climate", "shelf base temperature", "Kelvin", "")
    T.set(temperature)
    T.write(filename)

    M = PISM.IceModelVec2S(grid, "shelfbmassflux", PISM.WITHOUT_GHOSTS)
    M.set_attrs("climate", "shelf base mass flux", "kg m-2 s-1", "")
    M.set(mass_flux)
    M.write(filename)

class GivenTest(unittest.TestCase):
    "Test the ocean::Given class"

    def runTest(self):
        "ocean::Given"
        log.message(1, "\n")
        model = PISM.OceanGiven(self.grid)
        model.init()
        model.update(0, 1)

        check(model.shelf_base_temperature(), self.temperature)
        check(model.shelf_base_mass_flux(), self.mass_flux)
        check(model.melange_back_pressure_fraction(), self.melange_back_pressure)

    def setUp(self):
        grid = create_dummy_grid()
        self.grid = grid
        self.filename = "given_input.nc"

        self.temperature           = 263.0
        self.mass_flux             = 3e-3
        self.melange_back_pressure = 0.0

        create_given_input_file(self.filename, self.grid, self.temperature, self.mass_flux)

        o = PISM.PETSc.Options()
        o.setValue("-ocean_given_file", self.filename)

    def tearDown(self):
        os.remove(self.filename)

class GivenTHTest(unittest.TestCase):
    def setUp(self):

        depth = 1000.0
        salinity = 35.0
        potential_temperature = 270.0
        self.melange_back_pressure = 0.0
        self.temperature = 270.17909999999995
        self.mass_flux = -6.489250000000001e-05

        grid = create_dummy_grid()
        self.grid = grid

        ice_thickness = PISM.model.createIceThicknessVec(grid)
        ice_thickness.set(depth)
        grid.variables().add(ice_thickness)

        filename = "given_th_input.nc"
        self.filename = filename

        PISM.util.prepare_output(filename)

        Th = PISM.IceModelVec2S(grid, "theta_ocean", PISM.WITHOUT_GHOSTS)
        Th.set_attrs("climate", "potential temperature", "Kelvin", "")
        Th.set(potential_temperature)
        Th.write(filename)

        S = PISM.IceModelVec2S(grid, "salinity_ocean", PISM.WITHOUT_GHOSTS)
        S.set_attrs("climate", "ocean salinity", "g/kg", "")
        S.set(salinity)
        S.write(filename)

        o = PISM.PETSc.Options()
        o.setValue("-ocean_th_file", self.filename)

    def runTest(self):
        "ocean::GivenTH"
        log.message(1, "\n")
        model = PISM.OceanGivenTH(self.grid)
        model.init()
        model.update(0, 1)

        check(model.shelf_base_temperature(), self.temperature)
        check(model.shelf_base_mass_flux(), self.mass_flux)
        check(model.melange_back_pressure_fraction(), self.melange_back_pressure)

    def tearDown(self):
        os.remove(self.filename)
