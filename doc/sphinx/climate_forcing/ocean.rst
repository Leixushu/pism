.. include:: shortcuts.txt

Ocean model components
----------------------

PISM Ocean model components provide sub-shelf ice temperature (:var:`shelfbtemp`) and
sub-shelf mass flux (:var:`shelfbmassflux`) to the ice dynamics core.

The sub-shelf ice temperature is used as a Dirichlet boundary condition in the energy
conservation code. The sub-shelf mass flux is used as a source in the mass-continuity
(transport) equation. Positive flux corresponds to ice loss; in other words, this
sub-shelf mass flux is a "melt rate".

.. contents::

.. _sec-ocean-constant:

Constant in time and space
++++++++++++++++++++++++++
    
:|options|: ``-ocean constant``
:|variables|: none
:|implementation|: ``pism::ocean::Constant``

.. note:: This is the default choice.

This ocean model component implements boundary conditions at the ice/ocean interface that
are constant *both* in space and time.

The sub-shelf ice temperature is set to pressure melting and the sub-shelf melt rate is
assumed to be proportional to the heat flux from the ocean into the ice (configuration
parameter :config:`ocean.sub_shelf_heat_flux_into_ice`).

Alternatively, the sub-shelf melt rate in meters per year can be set using the
:opt:`-shelf_base_melt_rate` command-line option.

.. _sec-ocean-given:

Reading forcing data from a file
++++++++++++++++++++++++++++++++
    
:|options|: ``-ocean given``
:|variables|: :var:`shelfbtemp` Kelvin,
              :var:`shelfbmassflux`  |flux|
:|implementation|: ``pism::ocean::Given``

This ocean model component reads sub-shelf ice temperature :var:`shelfbtemp` and the
sub-shelf mass flux :var:`shelfbmassflux` from a file. It takes the following command-line
options.

- :opt:`-ocean_given_file`: sets the name of the file to read forcing data from. The file
  may contain several records. If only one record is provided it is interpreted as
  time-independent.
- :opt:`-ocean_given_reference_year` specifies the reference date; see section :ref:`sec-periodic-forcing`.
- :opt:`-ocean_given_period` specifies the length of the period of the forcing data, in
  model years; see section :ref:`sec-periodic-forcing`.

Variables :var:`shelfbtemp` and :var:`shelfbmassflux` may be time-dependent. (The ``-ocean
given`` component is very similar to ``-surface given`` and ``-atmosphere given``.)

.. _sec-ocean-pik:

PIK
+++
    
:|options|: ``-ocean pik``
:|variables|: none
:|implementation|: ``pism::ocean::PIK``

This ocean model component implements the ocean forcing setup used in :cite:`Martinetal2011`.
The sub-shelf ice temperature is set to pressure-melting; the sub-shelf mass flux
computation follows :cite:`BeckmannGoosse2003`.

It takes one command-line option:

- :opt:`-meltfactor_pik`: a melt factor `F_{\mathrm{melt}}` in sub-shelf-melting
  parameterization, see equation (5) in :cite:`Martinetal2011`.

.. _sec-ocean-th:

Basal melt rate and temperature from thermodynamics in boundary layer
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

:|options|: ``-ocean th``
:|variables|: :var:`theta_ocean` (absolute potential ocean temperature), [Kelvin],
              :var:`salinity_ocean` (salinity of the adjacent ocean), [g/kg]
:|implementation|: ``pism::ocean::GivenTH``

This ocean model component derives basal melt rate and basal temperature from
thermodynamics in a boundary layer at the base of the ice shelf. It uses a set of three
equations describing

#. the energy flux balance,
#. the salt flux balance,
#. the pressure and salinity dependent freezing point in the boundary layer.

This model is described in :cite:`HollandJenkins1999` and :cite:`Hellmeretal1998`.

Inputs are potential temperature (variable :var:`theta_ocean`) and salinity (variable
:var:`salinity_ocean`) read from a file.

No ocean circulation is modeled, so melt water computed by this model is not fed back into
the surrounding ocean.

This implementation uses different approximations of the temperature gradient at the base
of an ice shelf column depending on whether there is sub-shelf melt, sub-shelf freeze-on,
or neither (see :cite:`HollandJenkins1999` for details).

It takes two command-line option:

- :opt:`-ocean_th_file`: specifies the NetCDF file providing potential temperature and
  salinity fields.
- :opt:`-clip_shelf_base_salinity`: if this is set (which is the default), the sub-shelf
  salinity is clipped so that it stays in the `[4, 40]` psu range. This is done to
  ensure that we stay in the range of applicability of the melting point temperature
  parameterization; see :cite:`HollandJenkins1999`. To disable salinity clipping, use the
  :opt:`-no_clip_shelf_base_salinity` option or set the
  :config:`ocean_three_equation_model_clip_salinity` configuration parameter to "no".

.. _sec-ocean-delta-sl:

Scalar sea level offsets
++++++++++++++++++++++++

:|options|: :opt:`-ocean ...,delta_SL`
:|variables|: :var:`delta_SL` (meters)
:|implementation|: ``pism::ocean::Delta_SL``

The ``delta_SL`` modifier implements sea level forcing using scalar offsets.

It takes the following command-line options:

- :opt:`-ocean_delta_SL_file`: specifies the name of the file containing forcing data.
  This file has to contain the :var:`delta_SL` variable using units "meters" or
  equivalent.
- :opt:`-ocean_delta_SL_period` specifies the length of the period of the forcing data, in
  model years; see section :ref:`sec-periodic-forcing`.
- :opt:`-ocean_delta_SL_reference_year` specifies the reference date; see section
  :ref:`sec-periodic-forcing`.

.. _sec-ocean-delta-t:

Scalar sub-shelf temperature offsets
++++++++++++++++++++++++++++++++++++


:|options|: :opt:`-ocean ...,delta_T`
:|variables|: :var:`delta_T` (Kelvin)
:|implementation|: ``pism::ocean::Delta_T``

This modifier implements forcing using sub-shelf ice temperature offsets.

It takes the following command-line options:

- :opt:`-ocean_delta_T_file`: specifies the name of the file containing forcing data. This
  file has to contain the :var:`delta_T` variable using units of "Kelvin" or equivalent.
- :opt:`-ocean_delta_T_period` specifies the length of the period of the forcing data, in
  model years; see section :ref:`sec-periodic-forcing`.
- :opt:`-ocean_delta_T_reference_year` specifies the reference date; see section
  :ref:`sec-periodic-forcing`.

.. _sec-ocean-delta-smb:

Scalar sub-shelf mass flux offsets
++++++++++++++++++++++++++++++++++

:|options|: ``-ocean ...,delta_SMB``
:|variables|: :var:`delta_SMB` |flux|
:|implementation|: ``pism::ocean::Delta_SMB``

This modifier implements forcing using sub-shelf mass flux (melt rate) offsets.

It takes the following command-line options:

- :opt:`-ocean_delta_SMB_file`: specifies the name of the file containing forcing data.
  This file has to contain the :var:`delta_SMB` variable using units |flux| or equivalent.
- :opt:`-ocean_delta_SMB_period` specifies the length of the period of the forcing data,
  in model years; see section :ref:`sec-periodic-forcing`.
- :opt:`-ocean_delta_SMB_reference_year` specifies the reference date; see section
  :ref:`sec-periodic-forcing`.

.. _sec-ocean-frac-smb:

Scalar sub-shelf mass flux fraction offsets
+++++++++++++++++++++++++++++++++++++++++++

:|options|: ``-ocean ...,frac_SMB``
:|variables|: :var:`frac_SMB` [1]
:|implementation|: ``pism::ocean::Frac_SMB``

This modifier implements forcing using sub-shelf mass flux (melt rate) fraction offsets.

It takes the following command-line options:

- :opt:`-ocean_frac_SMB_file`: specifies the name of the file containing forcing data.
  This file has to contain the :var:`frac_SMB` variable.
- :opt:`-ocean_frac_SMB_period` specifies the length of the period of the forcing data, in
  model years; see section :ref:`sec-periodic-forcing`.
- :opt:`-ocean_frac_SMB_reference_year` specifies the reference date; see section
  :ref:`sec-periodic-forcing`.

.. _sec-ocean-frac-mbp:

Scalar melange back pressure fraction
+++++++++++++++++++++++++++++++++++++

:|options|: :opt:`-ocean ...,frac_MBP`
:|variables|: :var:`frac_MBP`
:|implementation|: ``pism::ocean::Frac_MBP``

This modifier implements forcing using melange back pressure fraction offsets. The
variable :var:`frac_MBP` should take on values from 0 to 1; it is understood as the
fraction of the maximum melange back pressure possible at a given location. (We assume
that melange back pressure cannot exceed the pressure of the ice column at a calving
front.)

Please see :ref:`sec-model-melange-pressure` for details.

This modifier takes the following command-line options:

- :opt:`-ocean_frac_MBP_file`: specifies the name of the file containing forcing data.
  This file has to contain the :var:`frac_MBP` variable using units of "1" (a
  dimensionless parameter)
- :opt:`-ocean_frac_MBP_period` specifies the length of the period of the forcing data, in
  model years; see section :ref:`sec-periodic-forcing`.
- :opt:`-ocean_frac_MBP_reference_year` specifies the reference date; see section
  :ref:`sec-periodic-forcing`.

.. _sec-ocean-cache:

The caching modifier
++++++++++++++++++++

:|options|: :opt:`-ocean ...,cache`
:|implementation|: ``pism::ocean::Cache``
:|seealso|: :ref:`sec-surface-cache`

This modifier skips ocean model updates, so that a ocean model is called no more than
every :opt:`-ocean_cache_update_interval` years. A time-step of `1` year is used every
time a ocean model is updated.

This is useful in cases when inter-annual climate variability is important, but one year
differs little from the next. (Coarse-grid paleo-climate runs, for example.)

It takes the following options:

- :opt:`-ocean_cache_update_interval` (*years*) Specifies the minimum interval between
  updates. PISM may take longer time-steps if the adaptive scheme allows it, though.
