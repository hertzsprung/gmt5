/* $Id$
 *
 * Code automatically generated by mgd77netcdfhelper.sh
 * To be included by mgd77_functions.c
 *
 * Copyright (c) 2005-2011 by P. Wessel
 * See README file for copying and redistribution conditions.
 */

#ifndef _MGD77_FUNCTIONS_H
#define _MGD77_FUNCTIONS_H

#include "mgd77.h"

struct MGD77_HEADER_LOOKUP {    /* Book-keeping for one header parameter  */
        char name[64];          /* Name of this parameter (e.g., "Gravity_Sampling_Rate") */
        GMT_LONG length;        /* Number of bytes to use */
        int record;             /* Header record number where it occurs (1-24) */
        int item;               /* Sequential item order in this record (1->) */
        GMT_BOOLEAN check;         /* TRUE if we actually do a test on this item */
        GMT_BOOLEAN revised;       /* TRUE if read in via the _REVISED attribute */
        char *ptr[2];           /* Pointers to the corresponding named variable in struct MGD77_HEADER_PARAMS (orig and revised) */
};

struct MGD77_HEADER_PARAMS {            /* See MGD-77 Documentation from NGDC for details */
  /* Sequence No 01: */
  char Record_Type;
	char Survey_Identifier[9];
	char Format_Acronym[6];
	char Data_Center_File_Number[9];
	char Parameters_Surveyed_Code[6];
	char File_Creation_Year[5];
	char File_Creation_Month[3];
	char File_Creation_Day[3];
	char Source_Institution[40];
	/* Sequence No 02: */
	char Country[19];
	char Platform_Name[22];
	char Platform_Type_Code;
	char Platform_Type[7];
	char Chief_Scientist[33];
	/* Sequence No 03: */
	char Project_Cruise_Leg[59];
	char Funding[21];
	/* Sequence No 04: */
	char Survey_Departure_Year[5];
	char Survey_Departure_Month[3];
	char Survey_Departure_Day[3];
	char Port_of_Departure[33];
	char Survey_Arrival_Year[5];
	char Survey_Arrival_Month[3];
	char Survey_Arrival_Day[3];
	char Port_of_Arrival[31];
	/* Sequence No 05: */
	char Navigation_Instrumentation[41];
	char Geodetic_Datum_Position_Determination_Method[39];
	/* Sequence No 06: */
	char Bathymetry_Instrumentation[41];
	char Bathymetry_Add_Forms_of_Data[39];
	/* Sequence No 07: */
	char Magnetics_Instrumentation[41];
	char Magnetics_Add_Forms_of_Data[39];
	/* Sequence No 08: */
	char Gravity_Instrumentation[41];
	char Gravity_Add_Forms_of_Data[39];
	/* Sequence No 09: */
	char Seismic_Instrumentation[41];
	char Seismic_Data_Formats[39];
	/* Sequence No 10: */
	char Format_Type;
	char Format_Description[95];
	/* Sequence No 11: */
	char Topmost_Latitude[4];
	char Bottommost_Latitude[4];
	char Leftmost_Longitude[5];
	char Rightmost_Longitude[5];
	/* Sequence No 12: */
	char Bathymetry_Digitizing_Rate[4];
	char Bathymetry_Sampling_Rate[13];
	char Bathymetry_Assumed_Sound_Velocity[6];
	char Bathymetry_Datum_Code[3];
	char Bathymetry_Interpolation_Scheme[57];
	/* Sequence No 13: */
	char Magnetics_Digitizing_Rate[4];
	char Magnetics_Sampling_Rate[3];
	char Magnetics_Sensor_Tow_Distance[5];
	char Magnetics_Sensor_Depth[6];
	char Magnetics_Sensor_Separation[4];
	char Magnetics_Ref_Field_Code[3];
	char Magnetics_Ref_Field[13];
	char Magnetics_Method_Applying_Res_Field[48];
	/* Sequence No 14: */
	char Gravity_Digitizing_Rate[4];
	char Gravity_Sampling_Rate[3];
	char Gravity_Theoretical_Formula_Code;
	char Gravity_Theoretical_Formula[18];
	char Gravity_Reference_System_Code;
	char Gravity_Reference_System[17];
	char Gravity_Corrections_Applied[39];
	/* Sequence No 15: */
	char Gravity_Departure_Base_Station[8];
	char Gravity_Departure_Base_Station_Name[34];
	char Gravity_Arrival_Base_Station[8];
	char Gravity_Arrival_Base_Station_Name[32];
	/* Sequence No 16: */
	char Number_of_Ten_Degree_Identifiers[3];
	char Ten_Degree_Identifier[151];
	/* Sequence No 18: */
	char Additional_Documentation_1[79];
	/* Sequence No 19: */
	char Additional_Documentation_2[79];
	/* Sequence No 20: */
	char Additional_Documentation_3[79];
	/* Sequence No 21: */
	char Additional_Documentation_4[79];
	/* Sequence No 22: */
	char Additional_Documentation_5[79];
	/* Sequence No 23: */
	char Additional_Documentation_6[79];
	/* Sequence No 24: */
	char Additional_Documentation_7[79];
};

void MGD77_Write_Header_Params (struct GMT_CTRL *C, struct MGD77_CONTROL *F, struct MGD77_HEADER_PARAMS **P);

GMT_LONG MGD77_Get_Param (struct GMT_CTRL *C, struct MGD77_CONTROL *F, char *name, char *value_orig, char *value_revised);
void MGD77_Put_Param (struct GMT_CTRL *C, struct MGD77_CONTROL *F, char *name, size_t length_orig, char *value_orig, size_t length_rev, char *value_revised, GMT_LONG revised);
void MGD77_Read_Header_Params (struct GMT_CTRL *C, struct MGD77_CONTROL *F, struct MGD77_HEADER_PARAMS **P);
void MGD77_Dump_Header_Params (struct GMT_CTRL *C, struct MGD77_CONTROL *F, struct MGD77_HEADER_PARAMS *P);
void MGD77_Reset_Header_Params (struct GMT_CTRL *C, struct MGD77_CONTROL *F);
void MGD77_Init_Ptr (struct GMT_CTRL *C, struct MGD77_HEADER_LOOKUP *H, struct MGD77_HEADER_PARAMS **P);
int MGD77_Param_Key (struct GMT_CTRL *C, GMT_LONG record, int item);

#define MGD77_N_HEADER_PARAMS 72

extern struct MGD77_HEADER_LOOKUP MGD77_Header_Lookup[];

#endif /* _MGD77_FUNCTIONS_H */
