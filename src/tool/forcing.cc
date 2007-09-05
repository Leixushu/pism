// Copyright (C) 2007 Nathan Shemonski and Ed Bueler
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

#include <petscda.h>
#include <cstring>
#include <netcdf.h>
// next is only needed for verbPrintF()
#include "../base/iceModel.hh"
#include "forcing.hh"

PetscErrorCode nc_check(int stat) {
  if (stat)
    SETERRQ1(1, "NC_ERR: %s\n", nc_strerror(stat));
  return 0;
}


Data1D::Data1D() {
  vecsAllocated = PETSC_FALSE;
  interpCode = DATA1D_LINEAR_INTERP;
}


Data1D::~Data1D() {
  if (vecsAllocated == PETSC_TRUE) {
    VecDestroy(vindep);
    VecDestroy(vdata);
    vecsAllocated = PETSC_FALSE;
  }
}


// Read data from NetCDF file (specified by a file name) into a Data1D.
/* Takes NetCDF file name and names of variables for independent variable
    (e.g. time variable) and dependent variable (e.g. temperature anomaly in
    case of ice core data).  Creates instance of Data1D class.  Reads data on
    processor zero.  Allocates sequential Vec on each processor.  Broadcasts
    processor zero data to all processors.
 */
PetscErrorCode Data1D::readData(MPI_Comm mycom, PetscMPIInt myrank,
                                    const char *myncfilename,
                                    const char *myindepvarname, const char *mydatavarname) {
  PetscErrorCode ierr;
  int stat, ncid = 0;
  if (myrank == 0) {
      stat = nc_open(myncfilename, 0, &ncid); CHKERRQ(nc_check(stat));
  }
  MPI_Bcast(&ncid, 1, MPI_INT, 0, mycom);
  ierr = readData(mycom, myrank, ncid, myindepvarname, mydatavarname); CHKERRQ(ierr);
  if (myrank == 0) {
    stat = nc_close(ncid); CHKERRQ(nc_check(stat));
  }
  return 0;
}


PetscErrorCode Data1D::readData(MPI_Comm mycom, PetscMPIInt myrank, int myncid, 
                                const char *myindepvarname, const char *mydatavarname) {
  PetscErrorCode ierr;
  int indepid, dataid;

  com = mycom;
  rank = myrank;
  strcpy(indepvarname, myindepvarname);
  strcpy(datavarname, mydatavarname);
  if (rank == 0) {
    int stat;
    stat = nc_inq_varid(myncid, indepvarname, &indepid); CHKERRQ(nc_check(stat));
    stat = nc_inq_varid(myncid, datavarname, &dataid); CHKERRQ(nc_check(stat));
  }
  ierr = getInterpolationCode(myncid, dataid, &interpCode); CHKERRQ(ierr);
  MPI_Bcast(&interpCode, 1, MPI_INT, 0, com);
  ierr = ncVarBcastVec(myncid, indepid, &vindep); CHKERRQ(ierr);  // creates this Vec
  ierr = ncVarBcastVec(myncid, dataid, &vdata); CHKERRQ(ierr);  // creates this Vec
  vecsAllocated = PETSC_TRUE;
  return 0;
}


PetscErrorCode Data1D::getInterpolationCode(int ncid, int vid, int *code) {
  PetscErrorCode ierr;

  if (rank == 0) {
    int stat;
    char attr[NC_MAX_NAME+1];
    size_t len;

    stat = nc_get_att_text(ncid, vid, "interpolation", attr);
    if (stat == NC_NOERR) {
      stat = nc_inq_attlen(ncid, vid, "interpolation", &len); CHKERRQ(nc_check(stat));
      attr[len] = '\0';
      if (strcmp(attr, "constant_piecewise_forward") == 0) {
        *code = DATA1D_CONST_PIECE_FWD_INTERP;
      } else if (strcmp(attr, "constant_piecewise_backward") == 0) {
        *code = DATA1D_CONST_PIECE_BCK_INTERP;
      } else if (strcmp(attr, "linear") == 0) {
        *code = DATA1D_LINEAR_INTERP;
      } else {
        ierr = verbPrintf(5, com, 
            "ATTENTION: interpolation '%s' for 1D data %s is unknown; defaulting to linear\n",
            attr,datavarname); CHKERRQ(ierr);
        *code = DATA1D_LINEAR_INTERP;
      }
    } else {
      ierr = verbPrintf(5, com, 
          "ATTENTION: interpolation attribute for 1D data %s is not found; defaulting to linear\n",
          datavarname); CHKERRQ(ierr);
      *code = DATA1D_LINEAR_INTERP;
    }
  } else {
    *code = -1;
  }
  return 0;
}


PetscErrorCode Data1D::ncVarBcastVec(int ncid, int vid, Vec *vecg) {
  // spread (broadcast) sequential Vecs containing ice or sea bed core-derived 
  // climate data to each processor
  
  PetscErrorCode ierr;
  int stat;
  size_t M_s;
  int M;
  float *f = NULL;

  if (rank == 0) {
    int dimids[NC_MAX_VAR_DIMS];
    int ndims, natts;
    nc_type xtype;
    char name[NC_MAX_NAME+1];
    stat = nc_inq_var(ncid, vid, name, &xtype, &ndims, dimids, &natts); CHKERRQ(nc_check(stat));
    if (ndims != 1) {
      SETERRQ2(1, "number of dimensions = %d for %s; should have ndims=1\n",
               ndims, name);
    }
    stat = nc_inq_dimlen(ncid, dimids[0], &M_s); CHKERRQ(nc_check(stat));
    M = (int)M_s;
    f = new float[M];
    stat = nc_get_var_float(ncid, vid, f); CHKERRQ(nc_check(stat));
  }
  ierr = MPI_Bcast(&M, 1, MPI_INT, 0, com); CHKERRQ(ierr); // broadcast the length
 
  // if you're not rank 0, you still need to create the array
  if (rank != 0){
    f = new float[M];
  }
  ierr = MPI_Bcast(f, M, MPI_FLOAT, 0, com); CHKERRQ(ierr);
  ierr = VecCreateSeq(PETSC_COMM_SELF, M, vecg); CHKERRQ(ierr);
  for (int x=0; x<(int)M; x++) {
    ierr = VecSetValue(*vecg, x, f[x], INSERT_VALUES); CHKERRQ(ierr);
  }
  ierr = VecAssemblyBegin(*vecg); CHKERRQ(ierr);
  ierr = VecAssemblyEnd(*vecg); CHKERRQ(ierr);
  delete [] f;
  return 0;
}


PetscErrorCode Data1D::getIndexMax(PetscInt *len) {
  PetscErrorCode  ierr = VecGetLocalSize(vdata, len); CHKERRQ(ierr);
  return 0;
}


PetscErrorCode Data1D::getIndexedDataValue(PetscInt index, PetscScalar *value) {
  PetscErrorCode ierr;
  PetscScalar *data;
  PetscInt    len;

  if (index < 0) {
    SETERRQ(1,"index negative");
  }
  ierr = VecGetLocalSize(vdata, &len); CHKERRQ(ierr);
  if (index >= len) {
    SETERRQ(1,"index out of bounds: too large");
  }
  ierr = VecGetArray(vdata, &data); CHKERRQ(ierr);
  *value = data[index];
  ierr = VecRestoreArray(vdata, &data); CHKERRQ(ierr);
  return 0;
}


PetscErrorCode Data1D::getInterpolatedDataValue(PetscScalar myindep, PetscScalar *value) {
  PetscErrorCode ierr;
  PetscScalar *indep, *data;
  PetscInt    index, len;

  // determine index into data
  ierr = VecGetLocalSize(vindep, &len); CHKERRQ(ierr);
  ierr = VecGetArray(vindep, &indep); CHKERRQ(ierr);
  PetscInt r, l=0;
  r = len;
  // do a binary search to find where our value of the independent variable fits in.
  while (r > l + 1) {
    PetscInt j = (r + l)/2;
    if (myindep < indep[j]) {
      r = j;
    } else {
      l = j;
    }
  }    
  index = l;
  if (index < 0) {
    SETERRQ(1,"computed index is negative");
  }
  if (index >= len) {
    SETERRQ(2,"computed index exceeds length");
  }
  ierr = VecRestoreArray(vindep, &indep); CHKERRQ(ierr);

  ierr = VecGetArray(vdata, &data); CHKERRQ(ierr);
  if (myindep == indep[index]) {
    *value = data[index]; // if we have exact data, use it
  } else { // otherwise we need to interpolate
    switch (interpCode) {
      case DATA1D_CONST_PIECE_BCK_INTERP:
        // use the data point we are in front of
        if (index == 0) {
          *value = data[index];
        } else {
          *value = data[index-1];
        }
        break;
      case DATA1D_CONST_PIECE_FWD_INTERP:
        *value = data[index];
        break;
      case DATA1D_LINEAR_INTERP:
        if (index == 0) {
          *value = data[index];
        } else {
          const PetscScalar slope = (myindep - data[index-1]) / (data[index] - data[index-1]);
          *value = data[index-1] + slope * (data[index]- data[index-1]);
        }
        break;
      default:
        SETERRQ1(1, "unknown interpolation method for %s\n", datavarname);
    } // end switch
  }
  ierr = VecRestoreArray(vdata, &data); CHKERRQ(ierr);

  return 0;
}



IceSheetForcing::IceSheetForcing() : Data1D() {
  forcingActive = PETSC_FALSE;
  index = 0;
}


IceSheetForcing::~IceSheetForcing() {
  forcingActive = PETSC_FALSE;
}


PetscErrorCode IceSheetForcing::readStandardIceCoreClimateData(MPI_Comm mycom, PetscMPIInt myrank,
        int ncid, PetscScalar curr_year, PetscInt datatype) {
  PetscErrorCode ierr;
  
  switch (datatype) {
    case ISF_DELTA_T:
      ierr = readData(mycom,myrank,ncid,"t","delta_T"); CHKERRQ(ierr);
      break;
    case ISF_DELTA_SEA_LEVEL:
      ierr = readData(mycom,myrank,ncid,"t","delta_sea_level"); CHKERRQ(ierr);
      break;
    default:
      SETERRQ(1,"invalid datatype\n");
  } 
  // times are positive (years b.p.) in data; change to negative (years *after* present)
  vtimeinyears = vindep;
  ierr = VecScale(vtimeinyears,-1.0); CHKERRQ(ierr);
  ierr = initStandardIceCoreIndex(curr_year); CHKERRQ(ierr);
  return 0;
}


PetscErrorCode IceSheetForcing::initStandardIceCoreIndex(PetscScalar curr_year) {
  PetscErrorCode ierr;
  PetscInt       len;
  PetscScalar    *timeinyears;

  ierr = VecGetArray(vtimeinyears, &timeinyears); CHKERRQ(ierr);
  ierr = VecGetLocalSize(vtimeinyears, &len); CHKERRQ(ierr);

  int r, l=0;
  r = len;
  // do a binary search to find where our year fits in.
  while (r > l + 1) {
    PetscInt j = (r + l)/2;
    if(curr_year < timeinyears[j]) {
      r = j;
    } else {
      l = j;
    }
  }    
  index = l;
  printf("index found: %d\n", index);
  // maybe we are already past our place.
  if (l >= len) {
    forcingActive = PETSC_FALSE;
    ierr = verbPrintf(1, com, 
             "ATTENTION: past end of climate forcing data %s.  Using last value.\n",datavarname);
             CHKERRQ(ierr);
  } else {
    forcingActive = PETSC_TRUE;
  }

  ierr = VecRestoreArray(vtimeinyears, &timeinyears); CHKERRQ(ierr);
  return 0;
}


PetscErrorCode IceSheetForcing::updateFromStandardIceCoreData(PetscScalar curr_year, PetscScalar *change) {
  PetscErrorCode ierr;
  PetscScalar *timeinyears, *data;
  PetscInt    len;

  ierr = VecGetArray(vtimeinyears, &timeinyears); CHKERRQ(ierr);
  ierr = VecGetLocalSize(vtimeinyears, &len); CHKERRQ(ierr);
  ierr = VecGetArray(vdata, &data); CHKERRQ(ierr);

  // if there was a large time step, it is possible
  // that we skip over multiple entries
  while (index < len && curr_year > data[index]) {
    index++;
  }
  if (index >= len) {
    ierr = verbPrintf(1, com, 
             "ATTENTION: no more data for climate forcing %s.\n", datavarname); CHKERRQ(ierr);
    forcingActive = PETSC_FALSE;
  } else {
    if (curr_year == timeinyears[index]) {
      *change = data[index]; // if we have exact data, use it
    } else { // otherwise we need to interpolate
      PetscScalar y0, y1;
      switch (interpCode) {
        case DATA1D_CONST_PIECE_BCK_INTERP:
          // use the data point we are infront of
          if (index == 0) {
            *change = 0.0;
            ierr = verbPrintf(1, com, 
                     "ATTENTION: model year precedes beginning of data for climate forcing %s;"
                     " setting change=0\n", datavarname); CHKERRQ(ierr);
          } else {
            *change = data[index-1];
          }
          break;
        case DATA1D_CONST_PIECE_FWD_INTERP:
          *change = data[index];
          break;
        case DATA1D_LINEAR_INTERP:
          if (index == 0) {
            *change = 0.0;
            ierr = verbPrintf(1, com, 
                     "ATTENTION: model year precedes beginning of data for climate forcing %s;"
                     " setting change=0\n", datavarname); CHKERRQ(ierr);
          } else {
            y0 = data[index-1];
            y1 = data[index];
            *change = y0 + ((y1-y0) / (timeinyears[index] - timeinyears[index-1]))
                                  * (curr_year - timeinyears[index-1]);
          }
          break;
        default:
          SETERRQ1(1, "Unknown interpolation method for climate forcing %s\n", datavarname);
      } // end switch
    }
  }
  
  ierr = VecRestoreArray(vtimeinyears, &timeinyears); CHKERRQ(ierr);
  ierr = VecRestoreArray(vdata, &data); CHKERRQ(ierr);
  return 0;
}

