/* MGD-77 Data Record Field Defaults:
 *    Copyright (c) 2005-2012 by P. Wessel and M. T. Chandler *
 *      See LICENSE.TXT file for copying and redistribution conditions.
		   Name,                          Abbrev,     Start,  Length,  FortranCode,   Factor,   readMGD77,   order,   printMGD77,   printVALS,   Not_given    */
		{ "Data Record Type",                      "drt",         1,	   1,        "int",        1,       "%1d",       1,        "%1d",        NULL,           "9" },
		{ "Time Zone Correction",                   "tz",        10,	   3,        "int",        1,      "%03d",       3,      "%+03d",        NULL,         "+99" },
		{ "Year",                                 "year",        13,	   4,        "int",        1,      "%04d",       4,       "%04d",        NULL,        "9999" },
		{ "Month",                               "month",        17,	   2,        "int",        1,      "%02d",       5,       "%02d",        NULL,          "99" },
		{ "Day of Month",                          "day",        19,	   2,        "int",        1,      "%02d",       6,       "%02d",        NULL,          "99" },
		{ "Hour",                                 "hour",        21,	   2,        "int",        1,      "%02d",       7,       "%02d",        NULL,          "99" },
		{ "Minutes",                               "min",        23,	   5,       "real",     1000,      "%05d",       8,       "%05d",        NULL,       "99999" },
		{ "Latitude",                              "lat",        28,	   8,       "real",   100000,      "%08d",       9,      "%+08d",     "%9.5f",    "+9999999" },
		{ "Longitude",                             "lon",        36,	   9,       "real",   100000,      "%09d",      10,      "%+09d",    "%10.5f",   "+99999999" },
		{ "Position Type Code",                    "ptc",        45,	   1,        "int",        1,       "%1d",      11,        "%1d",        NULL,           "9" },
		{ "Bathymetry Two-Way Travel-Time",        "twt",        46,	   6,       "real",    10000,      "%06d",      12,       "%06d",     "%7.4f",      "999999" },
		{ "Bathymetry Corrected Depth",          "depth",        52,	   6,       "real",       10,      "%06d",      13,       "%06d",     "%7.1f",      "999999" },
		{ "Bathymetry Correction Code",            "bcc",        58,	   2,        "int",        1,      "%02d",      14,       "%02d",        NULL,          "99" },
		{ "Bathymetry Type Code",                  "btc",        60,	   1,        "int",        1,       "%1d",      15,        "%1d",        NULL,           "9" },
		{ "Magnetics First Sensor Total Field",   "mtf1",        61,	   6,       "real",       10,      "%06d",      16,       "%06d",     "%7.1f",      "999999" },
		{ "Magnetics Second Sensor Total Field",  "mtf2",        67,	   6,       "real",       10,      "%06d",      17,       "%06d",     "%7.1f",      "999999" },
		{ "Magnetics Residual Field", 	           "mag",        73,	   6,       "real",       10,      "%06d",      18,      "%+06d",     "%7.1f",      "+99999" },
		{ "Magnetics Sensor For Residual Field", "msens",        79,	   1,        "int",        1,       "%1d",      19,        "%1d",        NULL,           "9" },
		{ "Magnetics Diurnal Correction",         "diur",        80,	   5,       "real",       10,      "%05d",      20,      "%+05d",     "%6.1f",       "+9999" },
		{ "Magnetics Sensor Depth or Altitude",    "msd",        85,	   6,        "int",        1,      "%06d",      21,      "%+06d",       "%7d",      "+99999" },
		{ "Gravity Observed",                     "gobs",        91,	   7,       "real",       10,      "%07d",      22,       "%07d",     "%8.1f",     "9999999" },
		{ "Gravity Eotvos Correction",             "eot",        98,	   6,       "real",       10,      "%06d",      23,      "%+06d",     "%7.1f",      "+99999" },
		{ "Gravity Free-Air Anomaly",              "faa",       104,	   5,       "real",       10,      "%05d",      24,      "%+05d",     "%6.1f",       "+9999" },
		{ "Navigation Quality Code",               "nqc",       120,	   1,        "int",        1,       "%1d",      27,        "%1d",        NULL,           "9" },
		{ "Survey ID",                              "id",         2,	   8,       "char",    FALSE,       "%8s",       2,        "%-8s",       NULL,    "99999999" },
		{ "Seismic Line Number",                   "sln",       109,	   5,       "char",    FALSE,       "%5s",      25,        "%-5s",       NULL,       "99999" },
		{ "Seismic Shot-Point Number",            "sspn",       114,	   6,       "char",    FALSE,       "%6s",      26,        "%-6s",       NULL,      "999999" }
