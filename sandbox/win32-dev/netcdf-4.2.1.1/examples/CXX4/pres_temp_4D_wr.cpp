/* This is part of the netCDF package.
   Copyright 2006 University Corporation for Atmospheric Research/Unidata.
   See COPYRIGHT file for conditions of use.

   This is an example program which writes some 4D pressure and
   temperatures. This example demonstrates the netCDF C++ API. 

   This is part of the netCDF tutorial:
   http://www.unidata.ucar.edu/software/netcdf/docs/netcdf-tutorial

   Full documentation of the netCDF C++ API can be found at:
   http://www.unidata.ucar.edu/software/netcdf/docs/netcdf-cxx

   $Id$
*/

#include <netcdf>
#include <iostream>
#include <string>
using namespace std;
using namespace netCDF;
using namespace netCDF::exceptions;

// This is the name of the data file we will create. 
#define FILE_NAME "pres_temp_4D.nc"

// We are writing 4D data, a 2 x 6 x 12 lvl-lat-lon grid, with 2
// timesteps of data.
#define NDIMS    4
#define NLVL     2
#define NLAT     6
#define NLON     12
#define NREC     2

// Names of things. 
#define LVL_NAME "level"
#define LAT_NAME "latitude"
#define LON_NAME "longitude"
#define REC_NAME "time"
#define PRES_NAME     "pressure"
#define TEMP_NAME     "temperature"
#define MAX_ATT_LEN  80
// These are used to construct some example data. 
#define SAMPLE_PRESSURE 900
#define SAMPLE_TEMP     9.0
#define START_LAT       25.0
#define START_LON       -125.0


string  UNITS = "units";
string  DEGREES_EAST =  "degrees_east";
string  DEGREES_NORTH = "degrees_north";


// For the units attributes. 
string PRES_UNITS = "hPa";
string TEMP_UNITS = "celsius";
string LAT_UNITS = "degrees_north";
string LON_UNITS = "degrees_east";

// Return this code to the OS in case of failure.
#define NC_ERR 2

int main()
{
   // We will write latitude and longitude fields. 
   float lats[NLAT],lons[NLON];

   // Program variables to hold the data we will write out. We will
   // only need enough space to hold one timestep of data; one record.
   float pres_out[NLVL][NLAT][NLON];
   float temp_out[NLVL][NLAT][NLON];

   int i=0;  //used in the data generation loop
  
   // create some pretend data. If this wasn't an example program, we
   // would have some real data to write for example, model output.
   for (int lat = 0; lat < NLAT; lat++)
      lats[lat] = START_LAT + 5. * lat;
   for (int lon = 0; lon < NLON; lon++)
      lons[lon] = START_LON + 5. * lon;

   for (int lvl = 0; lvl < NLVL; lvl++)
     for (int lat = 0; lat < NLAT; lat++)
       for (int lon = 0; lon < NLON; lon++)
	 {
	   pres_out[lvl][lat][lon] =(float) (SAMPLE_PRESSURE + i);
	   temp_out[lvl][lat][lon]  = (float)(SAMPLE_TEMP + i++);
	 }
   
   try
   {
    
   
      // Create the file.
      NcFile test(FILE_NAME, NcFile::replace);

      // Define the dimensions. NetCDF will hand back an ncDim object for
      // each.
      NcDim lvlDim = test.addDim(LVL_NAME, NLVL);
      NcDim latDim = test.addDim(LAT_NAME, NLAT);
      NcDim lonDim = test.addDim(LON_NAME, NLON);
      NcDim recDim = test.addDim(REC_NAME);  //adds an unlimited dimension
       
      // Define the coordinate variables.
      NcVar latVar = test.addVar(LAT_NAME, ncFloat, latDim);
      NcVar lonVar = test.addVar(LON_NAME, ncFloat, lonDim);
       
      // Define units attributes for coordinate vars. This attaches a
      // text attribute to each of the coordinate variables, containing
      // the units.
      latVar.putAtt(UNITS, DEGREES_NORTH);
      lonVar.putAtt(UNITS, DEGREES_EAST);
       
      // Define the netCDF variables for the pressure and temperature
      // data.
      vector<NcDim> dimVector;
      dimVector.push_back(recDim);
      dimVector.push_back(lvlDim);
      dimVector.push_back(latDim);
      dimVector.push_back(lonDim);
      NcVar pressVar = test.addVar(PRES_NAME, ncFloat, dimVector);
      NcVar tempVar = test.addVar(TEMP_NAME, ncFloat, dimVector);
       
      // Define units attributes for coordinate vars. This attaches a
      // text attribute to each of the coordinate variables, containing
      // the units.
      pressVar.putAtt(UNITS, PRES_UNITS);
      tempVar.putAtt(UNITS, TEMP_UNITS);

      // Write the coordinate variable data to the file.
      latVar.putVar(lats);
      lonVar.putVar(lons);
            
      // Write the pretend data. This will write our surface pressure and
      // surface temperature data. The arrays only hold one timestep
      // worth of data. We will just rewrite the same data for each
      // timestep. In a real application, the data would change between
      // timesteps.
      vector<size_t> startp,countp;
      startp.push_back(0);
      startp.push_back(0);
      startp.push_back(0);
      startp.push_back(0);
      countp.push_back(1);
      countp.push_back(NLVL);
      countp.push_back(NLAT);
      countp.push_back(NLON);
      for (size_t rec = 0; rec < NREC; rec++) 
      {
	startp[0]=rec;
	pressVar.putVar(startp,countp,pres_out);
	tempVar.putVar(startp,countp,temp_out);
      }

      // The file is automatically closed by the destructor. This frees
      // up any internal netCDF resources associated with the file, and
      // flushes any buffers.
   
      //cout << "*** SUCCESS writing example file " << FILE_NAME << "!" << endl;
      return 0;
   }
   catch(NcException& e)
   {
      e.what(); 
      return NC_ERR;
   }
}
