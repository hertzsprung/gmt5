/*	$Id: meca.h,v 1.7 2009-01-09 04:02:35 guru Exp $
 *    Copyright (c) 1996-2009 by G. Patau
 *    Distributed under the GNU Public Licence
 *    See README file for copying and redistribution conditions.
 */

#include <stdio.h>
#include <math.h>

#define EPSIL 0.0001

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef M_PI_4
#define M_PI_4          0.78539816339744830962
#endif
#ifndef M_PI_2
#define M_PI_2          1.57079632679489661923
#endif

#define squared(x) ((x) * (x))

struct AXIS {
    double str;
    double dip;
    double val;
    int e;
};
/* val in 10**e dynes-cm */

struct MOMENT {
    double mant;
    int exponent;
};

struct nodal_plane {
    double str;
    double dip;
    double rake;
}; 

struct MECHANISM {
    struct nodal_plane NP1;
    struct nodal_plane NP2;
    struct MOMENT moment;
    double magms;
};

struct M_TENSOR {
    int expo;
    double f[6];
};
/* mrr mtt mff mrt mrf mtf in 10**expo dynes-cm */

typedef struct MOMENT st_mo;
typedef struct MECHANISM st_me;

double datan2(double y,double x);
double zero_360(double str);
void dc_to_axe(st_me meca,struct AXIS *T,struct AXIS *N,struct AXIS *P);
