/* MGD-77 Data Record Field Defaults:
		   Name,                          Abbrev,     Start,  Length,  FortranCode,   Factor,   readMGD77,   order,   printMGD77,   printVALS,   Not_given,      min,          max,    maxSlope */
		{ "DataRecordType",                "drt",         1,	   1,        "int",        1,       "%1d",       1,        "%1d",        NULL,   "9",		   3,            5,       FALSE},
		{ "TimeZoneCorrection",           "time",        10,	   3,        "int",        1,      "%03d",       3,      "%+03d",        NULL,   "+99",		 -13,           12,       FALSE},
		{ "Year",                         "time",        13,	   4,        "int",        1,      "%04d",       4,       "%04d",        NULL,   "9999",	1939,         2004,       FALSE},
		{ "Month",                        "time",        17,	   2,        "int",        1,      "%02d",       5,       "%02d",        NULL,   "99",		   1,           12,       FALSE},
		{ "Day",                          "time",        19,	   2,        "int",        1,      "%02d",       6,       "%02d",        NULL,   "99",		   1,           31,       FALSE},
		{ "Hour",                         "time",        21,	   2,        "int",        1,      "%02d",       7,       "%02d",        NULL,   "99",		   0,           24,       FALSE},
		{ "Minutes",                      "time",        23,	   5,       "real",     1000,      "%05d",       8,       "%05d",        NULL,   "99999",	   0,           60,       FALSE},
		{ "Latitude",                      "lat",        28,	   8,       "real",   100000,      "%08d",       9,      "%+08d",     "%9.5f",   "+9999999",	 -90,           90,       FALSE},
		{ "Longitude",                     "lon",        36,	   9,       "real",   100000,      "%09d",      10,      "%+09d",    "%10.5f",   "+99999999",	-180,          180,       FALSE},
		{ "PositionTypeCode",              "ptc",        45,	   1,        "int",        1,       "%1d",      11,        "%1d",        NULL,   "9",		   1,            9,       FALSE},
		{ "BathyTwoWayTravelTime",         "twt",        46,	   6,       "real",    10000,      "%06d",      12,       "%06d",     "%7.4f",   "999999",         0,           20,           1},
		{ "BathyCorrectedDepth",         "depth",        52,	   6,       "real",       10,      "%06d",      13,       "%06d",     "%7.1f",   "999999",         0,        15000,         200},
		{ "BathyCorrectionCode",           "bcc",        58,	   2,        "int",        1,      "%02d",      14,       "%02d",        NULL,   "99",		   1,           99,       FALSE},
		{ "BathyTypeCode",                 "btc",        60,	   1,        "int",        1,       "%1d",      15,        "%1d",        NULL,   "9",		   1,            9,       FALSE},
		{ "MagFirstSensorTotalField",     "mtf1",        61,	   6,       "real",       10,      "%06d",      16,       "%06d",     "%7.1f",   "999999",     20000,        70000,        2000},
		{ "MagSecondSensorTotalField",    "mtf2",        67,	   6,       "real",       10,      "%06d",      17,       "%06d",     "%7.1f",   "999999",     20000,        70000,          40},
		{ "MagResidualField", 	           "mag",        73,	   6,       "real",       10,      "%06d",      18,      "%+06d",     "%7.1f",   "+99999",    -10000,        10000,         200},
		{ "MagSensorForResidualField",   "msens",        79,	   1,        "int",        1,       "%1d",      19,        "%1d",        NULL,   "9",		   1,            9,       FALSE},
		{ "MagDiurnalCorrection",         "diur",        80,	   5,       "real",       10,      "%05d",      20,      "%+05d",     "%6.1f",   "+9999",	-500,          500,         1.5},
		{ "MagSensorDepthAltitude",        "msd",        85,	   6,        "int",        1,      "%06d",      21,      "%+06d",       "%7d",   "+99999",     -1000,        10000,          10},
		{ "GravObserved",                 "gobs",        91,	   7,       "real",       10,      "%07d",      22,       "%07d",     "%8.1f",   "9999999",   970000,       990000,          40},
		{ "GravEotvosCorrection",          "eot",        98,	   6,       "real",       10,      "%06d",      23,      "%+06d",     "%7.1f",   "+99999",      -300,          300,           5},
		{ "GravFreeAirAnomaly",            "faa",       104,	   5,       "real",       10,      "%05d",      24,      "%+05d",     "%6.1f",   "+9999",      -1000,         1000,          16},
		{ "NavQualityCode",                "nqc",       120,	   1,        "int",        1,       "%1d",      27,        "%1d",        NULL,   "9",		   1,            9,       FALSE},
		{ "SurveyID",                       "id",         2,	   8,       "char",    FALSE,       "%8s",       2,        "%-8s",       NULL,   "99999999",   FALSE,        FALSE,       FALSE},
		{ "SeismicLineNumber",             "sln",       109,	   5,       "char",    FALSE,       "%5s",      25,        "%5s",        NULL,   "99999",      FALSE,        FALSE,       FALSE},
		{ "SeismicShotPointNumber",       "sspn",       114,	   6,       "char",    FALSE,       "%6s",      26,        "%6s",        NULL,   "999999",     FALSE,        FALSE,       FALSE}
