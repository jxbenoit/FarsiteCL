/*newfms.cpp
  Fuel Moisture Stick (FMS) C source code.
  version 0.7.0

  The FMS function library models fuel stick temperature and moisture content
  given an input weather stream of air temperature, air humidity, solar
  radiation, and cumulative rainfall.

  Copyright (C) 1997-2004 by Collin D. Bevins.

  Permission to use, copy, modify, and distribute this software and its
  documentation under the terms of the GNU General Public License is hereby
  granted. No representations are made about the suitability of this software
  for any purpose. It is provided "as is" without express or implied warranty.
  See the GNU General Public License for more details.

  Introduction

  The FMS function library models fuel stick temperature and moisture content
  given an input weather stream of air temperature, air humidity, solar 
  radiation, and cumulative rainfall.

  The fms.c and fms.h files contain functions to
  (1) create fuel moisture sticks of various sizes,
  (2) initialize the stick's internal and external environment and clock,
  (3) update the stick's external environment at arbitrary time intervals,
      thereby forcing its internal environment to be updated according to
      Ralph Nelson's stick moisture model,
  (4) determine the volume-weighted mean moisture content of the stick, and
  (5) destroy the stick and its memory resources.

  In addition to the above "public" functions, there are a number of
  "private" functions to configure and reinitialize the stick and to
  calculate and convert time and date information.

  Usage
  The demo.c program shows how to use the public FMS functions.
  Five or six functions are normally used:

  1 Call one or more of the stick creation functions (Fms_Create1Hour(),
    Fms_Create10Hour(), Fms_Create100Hour(), or Fms_Create1000Hour())
    to create a fuel moisture stick object.

  2 Call Fms_InitializeAt() to initialize the stick's environment and
    internal state at a given date and time.

  3 Call Fms_UpdateAt() whenever a new weather observation is available,
    passing it the new date, time, air temperature, air humidity, solar
    radiation, and cumulative rainfall parameters. The stick's moisture
    profile is automatically updated for the current conditions.

  4 Call Fms_MeanWtdMoisture() whenever you need the stick's volume-
    weighted mean moisture content.

  5 Call Fms_Destroy() once for each stick you created.

  6 See Fms_WriteToAsciiStream() and Fms_ReadFromAsciiStream()
    for examples of how to store and load sticks to/from files.

  File List
  The pre-release software package consists of the following files:
  CHANGES      Change log
  MAKEFILE     Type "make" to create "demo"
  demo.c       Shows use of all FMS public functions
  fms.c        FMS ANSI C code and support functions
  fms.h        FMS ANSI C header file
  testburn.dat Burnsville, NC 10-h moisture input file for "demo"
  testmio.dat  Mio, MI 10-h moisture input file for "demo"

  Function List

  Public Functions
  The following public functions are available to the user:

  Fms_Create1Hour()     -- creates a 1-h fuel moisture stick.
  Fms_Create10Hour()    -- creates a standard 10-h fuel moisture pine stick.
  Fms_Create100Hour()   -- creates a 100-h fuel moisture stick.
  Fms_Create1000Hour()  -- creates a 1000-h fuel moisture stick.
  Fms_CreateStick()     -- creates a custom fuel moisture stick.
  Fms_Destroy()         -- destroys a fuel moisture stick.
  Fms_InitializeAt()    -- initialize a stick's interior and external
                           environment at a specific date and time.
  Fms_MeanWtdMoisture() -- determine the stick's volume-weighted
                           mean moisture content.
  Fms_UpdateAt()        -- update the stick's internal environment given the
                           current date, time, air temperature, air humidity,
                           solar radiation, and cumulative rainfall values.

  Example Functions
  The following functions are working examples of how to store the necessary
  stick information to a file, and how to read it back again.

  Fms_ReadFromAsciiStream() Reads all the necessary Fms stick properties from
                            a file.
  Fms_WriteToAsciiStream()  Stores all the necessary Fms stick properties to
                            a file.

  Deprecated Functions
  Use of the following functions is discouraged; they are included only for
  completeness, internal usage, and backwards compatibility.

  Fms_Initialize()   Initializes a stick's interior and external environment
                     (without date-time tracking). Replaced by
                     Fms_InitializeAt().
  Fms_MeanMoisture() Determines the stick's radial profile mean moisture
                     content. Replaced by Fms_MeanWtdMoisture().
  Fms_Update()       Updates the stick's internal environment given the
                     elapsed time since the previous update and the current
                     air temperature, air humidity, solar radiation, and
                     cumulative rainfall values.  Replaced by Fms_UpdateAt().

  Private Functions
  CDT_CalendarDate()
  CDT_JulianDate()
  CDT_DecimalDay()
  CDT_MillisecondOfDay()
  Fms_CreateParameters()
  Fms_Diffusivity()
*/
#include<limits.h>
#include<math.h>
#include<stdlib.h>
#include<string.h>
#include"newfms.h"

//----------------------------------------------------------------------------
//Stick-independent "manifest" constants
//----------------------------------------------------------------------------

//FMS_MAX_NODES
//Maximum number of stick nodes (64); used only to dimension local variables.
#define FMS_MAX_NODES (64)

//const double Aks
//Permeability of a water saturated stick (2.0e-13 cm2).
const double Aks = 2.0e-13;

//const double Alb
//Shortwave albido (0.6 dl).
const double Alb = 0.6;

//const double Alpha
//Fraction of cell length that overlaps adjacent cells (0.25 cm/cm).
const double Alpha = 0.25;

//const double Ap
//Psychrometric constant (0.000772 / oC).
const double Ap = 0.000772;

//const double Aw
//Ratio of cell cavity to total cell width (0.8 cm/cm).
const double Aw = 0.8;

//const double Eps
//Longwave emissivity of stick surface (0.85 dl).
const double Eps = 0.85;

//const double Hfs
//Saturation value of the stick surface humidity (0.99 g/g).
const double Hfs = 0.99;

//const double Kelvin
//Celsius-to-Kelvin offset (273.2 oC).
const double Kelvin = 273.2;

//const double Pi
//A well-rounded number (dl).
const double Pi = 3.14159;

//const double Pr
//Prandtl number (0.7 dl).
const double Pr = 0.7;

//const double Sbc
//Stefan-Boltzmann constant (1.37e-12 cal/cm2-s-K4).
const double Sbc = 1.37e-12;

//const double Sc
//Schmidt number (0.58 dl).
const double Sc = 0.58;

//const double Smv
//Factor to convert solar radiation from Watts/m2 to milliVolts 
//milliVolts = solarRadiation / 94.743;
const double Smv = 94.743;

//const double St
//Surface tension (72.8).
const double St = 72.8;

//const double Tcd
//Day time clear sky temperature (6 oC).
const double Tcd = 6.;

//const double Tcn
//Night time clear sky temperature (3 oC).
const double Tcn = 3.;

//const double Thdiff
//Thermal diffusivity ( 8.0 cms/h).
const double Thdiff = 8.0;

//const double Wl
//Diameter of interior cell cavity ( 0.0023 cm).
const double Wl = 0.0023;

//----------------------------------------------------------------------------
//Stick-independent intermediates
//----------------------------------------------------------------------------

//const double Srf
//Factor to derive "sr", solar radiation received (14.82052 cal/cm2-h).
const double Srf = 14.82052;

//const double Wsf
//Manifest constant equal to -log(1.0 - 0.99)
const double Wsf = 4.60517;

//const double Hrd
//Factor to derive daytime long wave radiative surface heat transfer.
//Let Sb = 4. * 3600. * Sbc * Eps = 1.67688e-08;
//Let tsk = Tcd + Kelvin;
//Then Hrd = Sb * tsk * tsk * tsk / Pi;
//And Hrd = 0.116171;
const double Hrd = 0.116171;

//const double Hrn
//Factor to derive nighttime long wave radiative surface heat transfer.
//Let Sb = 4. * 3600. * Sbc * Eps = 1.67688e-08;
//Let tsk = Tcn + Kelvin;
//Then Hrn = Sb * tsk * tsk * tsk / Pi;
//And Hrn = 0.112467;
const double Hrn = 0.112467;

//const double Sir
//Saturation value below which liquid water columns no longer exist.
//Sir = Aw * Alpha / (4. * (1.-(2.-Aw) * Alpha));
const double Sir = 0.0714285;

//const double Scr
//Saturation value at which liquid miniscus first enters the tapered
//portion of wood cells.
//Scr = 4. * Sir = 0.285714;
const double Scr = 0.285714;

//----------------------------------------------------------------------------
//Static function prototypes
//----------------------------------------------------------------------------

static int Fms_CreateParameters( Fms* fms );
static void Fms_Diffusivity( Fms* fms );

/*============================================================================
  Fms_Create1Hour
  Creates a new standard 1-h time-lag fuel moisture stick.

  The stick has the following properties:
   Radius = 0.2 cm (0.0787")
   Length = 25.0 cm (9.8")
   Density = 0.40 g/cm3 (25.0 lb/ft3)
   dT = 0.004 h

  Call Fms_Initialize() to set the stick's initial internal and external
  environment. Call Fms_Destroy() to free the stick memory.
  name - Stick name or other documentary text of arbitrary length.
  A copy of the passed string is allocated and stored with the stick's
  internal data.
  Returns Pointer to the newly allocated stick on success, NULL on failure.
  See also Fms_Create10Hour(), Fms_Create100Hour(), Fms_Create1000Hour().
*/
Fms* Fms_Create1Hour( char* name )
{ //Fms_Create1Hour
  return ( Fms_CreateStick(
    name,   /* Name for the new stick instance.                             */
    11,     /* Number of calculation nodes.                                 */
    0.20,   /* Stick radius                                            (cm) */
    25.0,   /* Stick length                                            (cm) */
    0.40,   /* Stick density                                       (gm/cm3) */
    0.004,  /* Moisture computation time step                           (h) */
    0.05,   /* Diffusivity computation time step                        (h) */
    0.0218, /* Barometric pressure                                (cal/cm3) */
    0.85,   /* Max local moisture due to rain                         (g/g) */
    2.5,    /* Planar heat transfer                           (cal/cm2-h-C) */
    0.065,  /* Surface mass transfer - adsorption             ((cm3/cm2)/h) */
    0.08,   /* Surface mass transfer - desorption             ((cm3/cm2)/h) */
    5.0,    /* Runoff factor during initial rainfall observation            */
    10.0,   /* Runoff factor after initial rainfall observation             */
    0.006,  /* Storm transition value                                (cm/h) */
    0.10    /* Water film contribution to moisture content          (gm/gm) */
         ) );
} //Fms_Create1Hour

/*============================================================================
  Fms_Create10Hour
  Creates a new standard 10-h time-lag fuel moisture stick.

  The stick has the following properties:
   Radius = 0.64 cm (0.0787")
   Length = 50.0 cm (19.6")
   Density = 0.40 g/cm3 (25.0 lb/ft3)
   dT = 0.02 h

  Call Fms_Initialize() to set the stick's initial internal and external
  environment. Call Fms_Destroy() to free the stick memory.
  name - Stick name or other documentary text of arbitrary length.
  A copy of the passed string is allocated and stored with the stick's
  internal data.
  Returns Pointer to the newly allocated stick on success, NULL on failure.
  See also Fms_Create1Hour(), Fms_Create100Hour(), Fms_Create1000Hour().
*/
Fms* Fms_Create10Hour( char* name )
{ //Fms_Create10Hour
  return ( Fms_CreateStick(
    name,   /* Name for the new stick instance.                             */
    11,     /* Number of calculation nodes.                                 */
    0.64,   /* Stick radius                                            (cm) */
    50.0,   /* Stick length                                            (cm) */
    0.40,   /* Stick density                                       (gm/cm3) */
    0.10,   /* 0.02, Moisture computation time step                     (h) */
    0.25,   /* Diffusivity computation time step                        (h) */
    0.0218, /* Barometric pressure                                (cal/cm3) */
    0.60,   /* Max local moisture due to rain                         (g/g) */
    0.38,   /* Planar heat transfer                           (cal/cm2-h-C) */
    0.02,   /* Surface mass transfer - adsorption             ((cm3/cm2)/h) */
    0.06,   /* Surface mass transfer - desorption             ((cm3/cm2)/h) */
    0.55,   /* Runoff factor during initial rainfall observation            */
    0.15,   /* Runoff factor after initial rainfall observation             */
    0.05,   /* Storm transition value                                (cm/h) */
    0.05    /* Water film contribution to moisture content          (gm/gm) */
         ) );
} //Fms_Create10Hour

/*============================================================================
  Fms_Create100Hour
  Creates a new standard 100-h time-lag fuel moisture stick.

  The stick has the following properties:
   Radius = 2.0 cm (0.787")
   Length = 105.0 cm (39.37")
   Density = 0.40 g/cm3 (25.0 lb/ft3)
   dT = 0.05 h

  Call Fms_Initialize() to set the stick's initial internal and external
  environment. Call Fms_Destroy() to free the stick memory.
  name - Stick name or other documentary text of arbitrary length.
  A copy of the passed string is allocated and stored with the stick's
  internal data.
  Returns Pointer to the newly allocated stick on success, NULL on failure.
  See also Fms_Create1Hour(), Fms_Create10Hour(), Fms_Create1000Hour().
*/
Fms* Fms_Create100Hour( char* name )
{ //Fms_Create100Hour
  return ( Fms_CreateStick(
    name,   /* Name for the new stick instance.                             */
    11,     /* Number of calculation nodes.                                 */
    2.0,    /* Stick radius                                            (cm) */
    100.,   /* Stick length                                            (cm) */
    0.40,   /* Stick density                                       (gm/cm3) */
    0.20,   /* 0.05, Moisture computation time step                     (h) */
    0.25,   /* Diffusivity computation time step                        (h) */
    0.0218, /* Barometric pressure                                (cal/cm3) */
    0.40,   /* Max local moisture due to rain                         (g/g) */
    0.30,   /* Planar heat transfer                           (cal/cm2-h-C) */
    0.012,  /* Surface mass transfer - adsorption             ((cm3/cm2)/h) */
    0.06,   /* Surface mass transfer - desorption             ((cm3/cm2)/h) */
    0.05,   /* Runoff factor during initial rainfall observation            */
    0.12,   /* Runoff factor after initial rainfall observation             */
    5.0,    /* Storm transition value                                (cm/h) */
    0.005   /* Water film contribution to moisture content          (gm/gm) */
         ) );
} //Fms_Create100Hour

/*============================================================================
  Fms_Create1000Hour
  Creates a new standard 1000-h time-lag fuel moisture stick.

  The stick has the following properties:
   Radius = 6.40 cm (2.52")
   Length = 200.0 cm (78.74")
   Density = 0.40 g/cm3 (25.0 lb/ft3)
   dT = 0.20 h

  Call Fms_Initialize() to set the stick's initial internal and external
  environment. Call Fms_Destroy() to free the stick memory.
  name - Stick name or other documentary text of arbitrary length.
  A copy of the passed string is allocated and stored with the stick's
  internal data.
  Returns Pointer to the newly allocated stick on success, NULL on failure.
  See also Fms_Create1Hour(), Fms_Create100Hour(), Fms_Create1000Hour().
*/
Fms* Fms_Create1000Hour( char* name )
{ //Fms_Create1000Hour
  return ( Fms_CreateStick(
    name,   /* Name for the new stick instance.                             */
    11,     /* Number of calculation nodes.                                 */
    6.4,    /* Stick radius                                            (cm) */
    200.,   /* Stick length                                            (cm) */
    0.40,   /* Stick density                                       (gm/cm3) */
    0.25,   /* 0.20, Moisture computation time step                     (h) */
    0.25,   /* Diffusivity computation time step                        (h) */
    0.0218, /* Barometric pressure                                (cal/cm3) */
    0.32,   /* Max local moisture due to rain                         (g/g) */
    0.12,   /* Planar heat transfer                           (cal/cm2-h-C) */
    0.00001, /* Surface mass transfer - adsorption            ((cm3/cm2)/h) */
    0.06,   /* Surface mass transfer - desorption             ((cm3/cm2)/h) */
    0.07,   /* Runoff factor during initial rainfall observation            */
    0.12,   /* Runoff factor after initial rainfall observation             */
    7.5,    /* Storm transition value                                (cm/h) */
    0.003   /* Water film contribution to moisture content          (gm/gm) */
         ) );
} //Fms_Create1000Hour

/*============================================================================
  Fms_CreateStick
  Creates a custom fuel moisture stick.

  Creates a new Fms fuel moisture stick object with the passed properties.
  A user normally calls one of the convenience routines Fms_Create1Hour(),
  Fms_Create10Hour(), Fms_Create100Hour(), or Fms_Create1000Hour() unless you
  know what you're doing!

  Call Fms_Initialize() to set the stick's initial internal and external
  environment. Call Fms_Destroy() to free the stick memory.

  Notes
  First the passed parameters are stored for the stick.
  Then Fms_CreateParameters() is called to allocate and initialize
  parameters for the requested number of stick nodes and to derive
  intermediates that are dependent upon the stick properties.

   name      Stick name or other documentary text of arbitrary length.
             A copy of the passed string is allocated and stored with the
             stick's internal data.
   nodes     Number of calculation nodes.
   radius    Stick radius (cm).
   length    Stick length (cm).
   density   Stick density (gm/cm3).
   mdt       Moisture computation time step (h).
   ddt       Diffusivity computation time step (h).
   pressure  Barometric pressure (cal/cm3).
   wmx       Maximum local moisture content due to rain (g/g).
   hc        Planar heat transfer (cal/cm2-h-C).
   stca      Adsorption surface mass transfer rate (cm2/h).
   stcd      Desportion surface mass transfer rate (cm2/h).
   rai0      Runoff factor during initial rainfall observation.
   rai1      Runoff factor after initial rainfall observation.
   stv       Storm transition value (cm/h).
   wfilmk    Water film contribution to stick moisture content (gm/gm).

  Returns pointer to the newly allocated stick on success, NULL on failure.
  See also Fms_Create1Hour(), Fms_Create10Hour(), Fms_Create100Hour(),
  Fms_Create1000Hour().
*/
Fms* Fms_CreateStick(
  char*  name,     /* Name for the new stick instance.                      */
  int    nodes,    /* Number of calculation nodes.                          */
  double radius,   /* Stick radius                                     (cm) */
  double length,   /* Stick length                                     (cm) */
  double density,  /* Stick density                                (gm/cm3) */
  double mdt,      /* Moisture computation time step                    (h) */
  double ddt,      /* Diffusivity computation time step                 (h) */
  double pressure, /* Barometric pressure                         (cal/cm3) */
  double wmx,      /* Max local moisture due to rain                  (g/g) */
  double hc,       /* Planar heat transfer                    (cal/cm2-h-C) */
  double stca,     /* Surface mass transfer - adsorption      ((cm3/cm2)/h) */
  double stcd,     /* Surface mass transfer - desorption      ((cm3/cm2)/h) */
  double rai0,     /* Runoff factor during initial rainfall observation     */
  double rai1,     /* Runoff factor after initial rainfall observation      */
  double stv,      /* Storm transition value                         (cm/h) */
  double wfilmk    /* Water film contribution to moisture content   (gm/gm) */
 )
{ //Fms_CreateStick
  Fms* fms;

  //Allocate the basic Fms object.
  if( (fms = new Fms) == NULL ) return( NULL );

  //Store the stick name.
  fms->name = NULL;
  if( (fms->name = new char[strlen(name) + 1]) == NULL ) {
    delete fms;
    return( NULL );
  }
  strcpy( fms->name, name );

  //Store the passed parameters.
  fms->n = nodes;
  fms->a = radius;
  fms->al = length;
  fms->dp = density;
  fms->bp = pressure;
  fms->mdt = mdt;
  fms->ddt = ddt;
  fms->wmx = wmx;
  fms->hc = hc;
  fms->stca = stca;
  fms->stcd = stcd;
  fms->rai0 = rai0;
  fms->rai1 = rai1;
  fms->stv = stv;
  fms->wfilmk = wfilmk;

  //Initialize some other parameters and nodal array pointers.
  fms->x = NULL;
  fms->d = NULL;
  fms->t = NULL;
  fms->s = NULL;
  fms->w = NULL;
  fms->v = NULL;

  //Initialize nodes and stick-dependent intermediates.
  if( Fms_CreateParameters(fms) != FMS_ERROR ) return( fms );

  return( NULL );
} //Fms_CreateStick

/*============================================================================
  Fms_CreateParameters
  Initializes a newly allocated stick's internal parameters.

  Allocates the stick nodal parameters and initializes all intermediate
  variables that depend solely upon stick properties. Called only by
  Fms_CreateStick().

  fms - Pointer to the Fms fuel moisture stick object to be initialized.
  Returns 0 on success, -1 on failure.
*/
static int Fms_CreateParameters( Fms* fms )
{ //Fms_CreateParameters
  int    i;
  double a2, rcav, ro, ri, vwt;

  //Create new nodal parameters.
  fms->x = new double[fms->n];
  fms->d = new double[fms->n];
  fms->t = new double[fms->n];
  fms->s = new double[fms->n];
  fms->w = new double[fms->n];
  fms->v = new double[fms->n];

  if( fms->x == NULL || fms->d == NULL || fms->t == NULL || fms->s == NULL ||
      fms->w == NULL || fms->v == NULL ) {
    Fms_Destroy( fms );
    return( -1 );
  }

  //dx = internodal distance (cm).
  fms->dx = fms->a / (fms->n - 1);

  //wmax = maximum possible stick moisture content (g/g).
  fms->wmax = (1. / fms->dp) - (1. / 1.53);

  //Stick nodal values.
  for( i = 0; i < fms->n; i++ ) {
    //x = radial distance from center of stick (cm).
    fms->x[i] = fms->a - (fms->dx * i);
    //d = bound water diffusivity (cm2/h).
    fms->d[i] = 0.;
    //t = temperature (oC).
    fms->t[i] = 20.;
    //s = saturation (g/g).
    fms->s[i] = 0.5 * fms->wmx;
    //w = water content (g/g).
    fms->w[i] = 0.5 * fms->wmx;
    //v = fraction of stick volume at this node (g3/g3).
    fms->v[i] = 0.0;
  }

  //Volume weighting factors.
  ro = fms->a;
  ri = ro - 0.5 * fms->dx;
  a2 = fms->a * fms->a;
  fms->v[0] = (ro * ro - ri * ri) / a2;
  vwt = fms->v[0];
  for( i = 1; i < fms->n - 1; i++ ) {
    ro = ri;
    ri = ro - fms->dx;
    fms->v[i] = (ro * ro - ri * ri) / a2;
    vwt += fms->v[i];
  }
  fms->v[fms->n - 1] = ri * ri / a2;
  vwt += fms->v[fms->n - 1];

  //Mark as uninitialized (in terms of weather).
  fms->init = 0;
  fms->updates = 0;
  fms->state = FMS_STATE_None;

  //-------------------------------------------------------------------------
  //Stick property-dependent optimization factors.
  //-------------------------------------------------------------------------

  //hwf = hw and aml computation factor used in Fms_Update().
  fms->hwf = 0.622 * fms->hc * pow((Pr / Sc), 0.667);

  //amlf = aml optimization factor
  fms->amlf = fms->hwf / (0.24 * fms->dp * fms->a);

  //capf = cap optimization factor.
  rcav = 0.5 * Aw * Wl;
  fms->capf = 3600. * Pi * St * rcav * rcav /
              (16. * fms->a * fms->a * fms->al * fms->dp);

  //sf = s factor used in Fms_Update() loop.
  fms->sf = 3600. * fms->mdt / (2. * fms->dx * fms->dp);

  //Vf = optimization factor used in Fms_Update().
  //WAS: fms->Vf = St / (Aw * Wl * Scr);
  //WAS: fms->Vf = St / (Wl * Scr);
  fms->vf = St / (fms->dp * Wl * Scr);

  //Some silly little time shavers.
  fms->dx_2 = fms->dx * 2.;
  fms->mdt_2 = fms->mdt * 2.;

  return( 0 );
} //Fms_CreateParameters

/*============================================================================
  Fms_Destroy
  Destroys a fuel moisture stick object and frees its resources.
  fms - Pointer to the Fms object to be destroyed.
*/
void Fms_Destroy( Fms* fms )
{ //Fms_Destroy
  if( fms == NULL ) return;
  if( fms->name ) delete[] fms->name;
  if( fms->x ) delete[] fms->x;
  if( fms->d ) delete[] fms->d;
  if( fms->t ) delete[] fms->t;
  if( fms->s ) delete[] fms->s;
  if( fms->w ) delete[] fms->w;
  if( fms->v ) delete[] fms->v;
  fms->name = 0;
  fms->x = 0;
  fms->d = 0;
  fms->t = 0;
  fms->s = 0;
  fms->w = 0;
  fms->v = 0;
  delete fms;

  return;
} //Fms_Destroy

/*============================================================================
  Fms_Diffusivity
  Determines bound water diffusivity at the stock's radial nodes.

  fms - Pointer to fuel moisture stick object.
  The stick's nodal bound water diffusivity values fms->d[i] (cm2/h) are
  recalculated.
*/
static void Fms_Diffusivity( Fms* fms )
{ //Fms_Diffusivity
  int    i;
  double c1;    /* Emc sorption isotherm parameter                    (g/g) */
  double c2;    /* Emc sorption isotherm parameter                    (g/g) */
  double con;   /* Correction for tortuous paths in cell wall               */
  double cpv;   /* Specific heat of water vapor               (cal/(mol*K)) */
  double daw;   /* Density of adsorbed water                        (g/cm3) */
  double dhdm;  /* Reciprocal slope of the sorption isotherm                */
  double dv;
  double dvpr;
  double e;     /* Activation energy for bound water diffusion    (cal/mol) */
  double fac;   /* Converts D from wood substance to whole wood basis       */
  double ps1;   /* Water satur vapor press at surface temp        (cal/cm3) */
  double qv;    /* Latent heat of vaporization of water           (cal/mol) */
  double qw;    /* Differential heat of sorption ofwater          (cal/mol) */
  double rfcw;  /* Converts D from wood substance to whole wood basis       */
  double svaw;  /* Specific volume of adsorbed water                (cm3/g) */
  double tk;    /* Stick temperature                                   (oK) */
  double vfaw;  /* Volume fraction of adborbed water                   (dl) */
  double vfcw;  /* Volume fraction of moist cell wall                  (dl) */
  double wc;    /* Lesser of nodal or fiber saturation moisture       (g/g) */

  for( i = 0; i < fms->n; i++ ) {
    tk = fms->t[i] + 273.2;
    qv = 13550. - 10.22 * tk;
    cpv = 7.22 + .002374 * tk + 2.67e-07 * tk * tk;
    //Sea level atmospheric pressure = 0.0242 cal/cm3
    dv = 0.22 * 3600. * (0.0242 / fms->bp) * pow((tk / 273.2), 1.75);
    ps1 = 0.0000239 * exp(20.58 - (5205. / tk));
    c1 = 0.1617 - 0.001419 * fms->t[i];
    c2 = 0.4657 + 0.003578 * fms->t[i];
    if( fms->w[i] < fms->wsa ) {
      wc = fms->w[i];
      dhdm = (1.0 - fms->hf) * pow(-log(1.0 - fms->hf), (1.0 - c2)) /
             (c1 * c2);
    }
    else {
      wc = fms->wsa;
      dhdm = (1.0 - Hfs) * pow(Wsf, (1.0 - c2)) / (c1 * c2);
    }
    daw = 1.3 - 0.64 * wc;
    svaw = 1. / daw;
    vfaw = svaw * wc / (0.685 + svaw * wc);
    vfcw = (0.685 + svaw * wc) / ((1.0 / fms->dp) + svaw * wc);
    rfcw = 1.0 - sqrt(1.0 - vfcw);
    fac = 1.0 / (rfcw * vfcw);
    con = 1.0 / (2.0 - vfaw);
    qw = 5040. * exp(-14.0 * wc);
    e = (qv + qw - cpv * tk) / 1.2;

    //-----------------------------------------------------------------------
    //The factor 0.016 below is a correction for hindered water vapor
    //diffusion (this is 62.5 times smaller than the bulk vapor diffusion);
    //0.0242 cal/cm3 = sea level atm pressure.
    //  -- Ralph Nelson
    //-----------------------------------------------------------------------
    dvpr = 18.0 * 0.016 * (1.0 - vfcw) * dv * ps1 * dhdm /
           (fms->dp * 1.987 * tk);
    fms->d[i] = dvpr + 3600. * 0.0985 * con * fac * exp(-e / (1.987 * tk));
  }
} //Fms_Diffusivity

/*============================================================================
  Fms_Initialize
  Initializes a fuel moisture stick's internal and external environment.

  Initializes the stick's internal and external environmental variables. 
  The stick's internal temperature and water content are presumed to be 
  uniformly distributed.

  Deprecated. Use Fms_InitializeAt() to initialize the stick's internal 
  clock as well as its environment. This frees the user from having to 
  keep track of elapsed time between stick updates.

   fms - Pointer to the fuel moisture stick object.
   ta  - Initial air temperature (oC).
   ha  - Initial air humidity (g/g).
   sr  - Initial solar voltage (W/m2).
   ti  - Initial stick temperature (oC).
   hi  - Initial stick surface humidty (g/g).
   wi  - Initial stick water fraction (g/g).
*/
void Fms_Initialize(
  Fms*   fms, /* Pointer to fuel moisture stick object.        */
  double ta,  /* Initial air temperature                  (oC) */
  double ha,  /* Initial air humidity                    (g/g) */
  double sr,  /* Initial solar radiation                (W/m2) */
  double rc,  /* Initial cumulative rainfall              (cm) */
  double ti,  /* Initial stick temperature                (oC) */
  double hi,  /* Initial stick surface humidity          (g/g) */
  double wi   /* Initial stick moisture content          (g/g) */
 )
{ //Fms_Initialize
  int i;

  //Environment initialization.
  fms->ta0 = fms->ta1 = ta;        //Air temperature (oC)
  fms->ha0 = fms->ha1 = ha;        //Air relative humidity (g/g)
  fms->sv0 = fms->sv1 = sr / Smv;  //Solar voltage (millivolts)
  fms->rc0 = fms->rc1 = rc;        //Cumulative rainfall amount (cm)
  fms->ra0 = fms->ra1 = 0.;        //Observation rainfall amount (cm)

  //Stick initialization.
  fms->hf = hi;                    //Relative humidity at fuel surface (g/g)
  fms->wfilm = 0.0;                //Water film moisture contribution (g/g)
  for( i = 0; i < fms->n; i++ ) {
    fms->t[i] = ti;          //Fuel nodal temperature (oC)
    fms->w[i] = wi;          //Fuel nodal moisture content (g/g)
    fms->s[i] = 0.;          //Fuel nodal saturation
  }
  fms->wsa = wi + .1;
  Fms_Diffusivity( fms );
  fms->init = 1;                   //Set initialization flag.
} //Fms_Initialize

/*============================================================================
  Fms_InitializeAt
  Initializes a fuel moisture stick's internal clock and its internal and
  external environment.

  Initializes the stick's internal clock and its internal and external
  environmental variables.  The stick's internal temperature and water
  content are presumed to be uniformly distributed.

   fms         - Pointer to the fuel moisture stick object.
   year        - Year of the Western (Julian-Gregorian) calendar.
   month       - Month of the year (1=Jan, 12=Dec)
   day         - Day of the month (1-31).
   hour        - Hour of the day (0-23).
   minute      - Minute of the hour (0-59).
   second      - Second of the minute (0-59).
   millisecond - Millisecond of the second (0-999).
   ta          - Initial air temperature (oC).
   ha          - Initial air humidity (g/g).
   sr          - Initial solar voltage (W/m2).
   ti          - Initial stick temperature (oC).
   hi          - Initial stick surface humidty (g/g).
   wi          - Initial stick water fraction (g/g).
*/
void Fms_InitializeAt(
  Fms*   fms,         /* Pointer to fuel moisture stick object.             */
  int    year,        /* Year of the update                      (4 digits) */
  int    month,       /* Month of the update                (1=Jan, 12=Dec) */
  int    day,         /* Day of the update                           (1-31) */
  int    hour,        /* Hour of the update                          (0-23) */
  int    minute,      /* Minute of the update                        (0-59) */
  int    second,      /* Second of the update                        (0-59) */
  int    millisecond, /* Millisecond of the update                  (0-999) */
  double ta,          /* Initial air temperature                       (oC) */
  double  ha,         /* Initial air humidity                         (g/g) */
  double  sr,         /* Initial solar radiation                     (W/m2) */
  double  rc,         /* Initial cumulative rainfall                   (cm) */
  double  ti,         /* Initial stick temperature                     (oC) */
  double  hi,         /* Initial stick surface humidity               (g/g) */
  double  wi          /* Initial stick moisture content               (g/g) */
 )
{ //Fms_InitializeAt
  //Set the initial modified Julian date and milliseconds of the day.
  fms->jdate = CDT_JulianDate( year, month, day, hour, minute, second,
                               millisecond );
  //Now initialize the stick's environmental variables.
  Fms_Initialize( fms, ta, ha, sr, rc, ti, hi, wi );
} //Fms_InitializeAt

/*============================================================================
  Fms_MeanMoisture
  Determines the mean moisture content of the stick's radial profile.

  Note:
  The integral average of the stick's nodal moisture contents is calculated
  without consideration to the nodes' volumetric representation.
  Deprecated. Use Fms_MeanWtdMoisture() for a volume-weighted mean moisture
  content.
   fms - Pointer to the fuel moisture stick object.
  Returns the mean moisture content of the stick's radial profile in grams of
  water per gram of wood.
*/
double Fms_MeanMoisture( Fms* fms )
{ //Fms_MeanMoisture
  int    i;
  double wea, web, wec, wei, wbr;

  wec = fms->w[0];
  wei = fms->dx / (3. * fms->a);

  for( i = 1; i < fms->n - 1; i += 2 ) {
    wea = 4. * fms->w[i];
    web = 2. * fms->w[i + 1];
    if( (i + 1) == (fms->n - 1) ) web = fms->w[fms->n - 1];
    wec += web + wea;
  }
  wbr = wei * wec;

  //Add water film.
  wbr += fms->wfilm;
  if( wbr > fms->wmx ) wbr = fms->wmx;

  return( wbr );
} //Fms_MeanMoisture

/*============================================================================
  Fms_MeanWtdMoisture
  Determines the volume-weighted mean moisture content of the stick's radial
  profile.
   fms - Pointer to the fuel moisture stick object.
  Returns the volume-weighted mean moisture content of the stick's radial
  profile in grams of water per gram of wood.
*/
double Fms_MeanWtdMoisture( Fms* fms )
{ //Fms_MeanWtdMoisture
  int    i;
  double wbr;
  wbr = 0.0;
  for( i = 0; i < fms->n; i++ ) wbr += fms->w[i] * fms->v[i];

  //Add water film.
  wbr += fms->wfilm;
  if( wbr > fms->wmx ) wbr = fms->wmx;

  return( wbr );
} //Fms_MeanWtdMoisture

/*============================================================================
  Fms_MeanWtdTemperature
  Determines the volume-weighted mean temperature of the stick's radial
  profile.
   fms - Pointer to the fuel moisture stick object.
  Returns the volume-weighted mean temperature of the stick's radial 
  profile in oC.
*/
double Fms_MeanWtdTemperature( Fms* fms )
{ //Fms_MeanWtdTemperature
  int    i;
  double wbr;
  wbr = 0.0;
  for( i = 0; i < fms->n; i++) wbr += fms->t[i] * fms->v[i];

  return( wbr );
} //Fms_MeanWtdTemperature

/*============================================================================
  Fms_UpdateAt
  Updates the stick's internal environment (temperature, diffusivity,
  saturation, and moisture content) from the new weather parameters.

  Determines the elapsed time since the previous update, then calls
  Fms_Update() to update the stick's fuel moisture.

   fms         - Pointer to the fuel moisture stick object.
   year        - Year of the Western (Julian-Gregorian) calendar.
   month       - Month of the year (1=Jan, 12=Dec)
   day         - Day of the month (1-31).
   hour        - Hour of the day (0-23).
   minute      - Minute of the hour (0-59).
   second      - Second of the minute (0-59).
   millisecond - Millisecond of the second (0-999).
   aC          - New air temperature (oC).
   hFtn        - New air humidity (g/g).
   sW          - New solar radiation (W/m2).
   rcumCm      - New cumulative rainfall (cm).
*/
void Fms_UpdateAt(
  Fms*   fms,         /* Pointer to the fuel moisture stick object.         */
  int    year,        /* Year of the update                      (4 digits) */
  int    month,       /* Month of the update                (1=Jan, 12=Dec) */
  int    day,         /* Day of the update                           (1-31) */
  int    hour,        /* Hour of the update                          (0-23) */
  int    minute,      /* Minute of the update                        (0-59) */
  int    second,      /* Second of the update                        (0-59) */
  int    millisecond, /* Millisecond of the update                  (0-999) */
  double aC,          /* New air temperature                           (oC) */
  double hFtn,        /* New air humidity                             (g/g) */
  double sW,          /* New solar radiation                         (W/m2) */
  double rcumCm       /* New cumulative rainfall                       (cm) */
 )
{ //Fms_UpdateAt
  double last;    //Previous observation Julian date    (days)
  double eHr;     //Elapsed time since last observation    (h)
  int sec;

  //If stick is not initialized, do so using environmental variables.
  if( ! fms->init )
    Fms_InitializeAt( fms, year, month, day, hour, minute, second,
                      millisecond, aC, hFtn, sW, rcumCm, aC, hFtn,
                      0.5 * fms->wmx );

  //Get the current modified Julian date and milliseconds of the day.
  last = fms->jdate;
  fms->jdate = CDT_JulianDate( year, month, day, hour, minute, second,
                               millisecond );

  //Determine elapsed hours to nearest second and call Fms_Update().
  //This is needed because of lack of precision in double Julian date:
  // Some 1 hr differences are 1.000000 and others are 0.99722,
  // which changes the number of loops run by Fms_Update().
  sec = (int) (86400 * (fms->jdate - last));
  eHr = (double) (sec / 3600);
  Fms_Update( fms, eHr, aC, hFtn, sW, rcumCm );
} //Fms_UpdateAt

/*============================================================================
  Fms_Update
  Updates the stick's internal environment (temperature, diffusivity,
  saturation, and moisture content) from the new weather parameters.

  The air temperature, air humidity, solar radiation, and cumulative rainfall
  between the previous and current update is interpolated (straight line) over
  a series of calculation time steps. The time step is a property of the stick
  (and usually depends upon the stick radius).

   fms         - Pointer to the fuel moisture stick object.
   eHr         - Elapsed time since last observation (h).
   month       - Month of the year (1=Jan, 12=Dec).
   day         - Day of the month (1-31).
   hour        - Hour of the day (0-23).
   minute      - Minute of the hour (0-59).
   second      - Second of the minute (0-59).
   millisecond - Millisecond of the second (0-999).
   aC          - New air temperature (oC).
   hFtn        - New air humidity (g/g).
   sW          - New solar radiation (W/m2).
   rcumCm      - New cumulative rainfall (cm).
*/
void Fms_Update(
  Fms*   fms,   /* Pointer to the fuel moisture stick object.        */
  double eHr,   /* Elapsed time since last observation           (h) */
  double aC,    /* New air temperature                          (oC) */
  double hFtn,  /* New air humidity                            (g/g) */
  double sW,    /* New solar radiation                        (W/m2) */
  double rcumCm /* New cumulative rainfall                      (cm) */
 )
{ //Fms_Update
  double ak;      /* permiability of stick when nonsaturated          (cm2) */
  double aml;     /* factor related to rate of condensation       ((g/g)/h) */
  double bi;      /* mass transfer biot number                         (dl) */
  double c1;      /* Emc sorption isotherm parameter                  (g/g) */
  double c2;      /* Emc sorption isotherm parameter                  (g/g) */
  double ddtNext; /* next time (tt) to run diffusivity calculations     (h) */
  double fsc;     /* solar constant interpolated between obs           (mv) */
  double gnu;     /* kinematic viscosity of liquid water            (cm2/s) */
  double ha;      /* air humidity interpolated between obs             (dl) */
  double hf_log;  /* -log(1. - fms->hf)                               (g/g) */
  double hr;      /* longwave radiative surface heat transfer (cal/cm2-h-C) */
  double hw;      /* stick heat transfer coeff for vapor diffusion above FSP*/
  double rai;     /* rainfall runoff factor                       ((g/g)/h) */
  double p1;      /* water vapor press at the stick surface       (cal/cm3) */
  double pa;      /* water vapor pressure in air                  (cal/cm3) */
  double par;     /* dummy variable for s                              (dl) */
  double ps1;     /* water satur vapor press at surface temp      (cal/cm3) */
  double psa;     /* water satur vapor press in ambient air       (cal/cm3) */
  double psd;     /* water satur vapor press at dewpoint          (cal/cm3) */
  double qv;      /* latent heat of vaporization of water         (cal/mol) */
  double qw;      /* differential heat of sorption of water       (cal/mol) */
  double sr;      /* solar rad received by half the stick      (cal/(cm2*h) */
  double sv;      /* current solar radiation                   (millivolts) */
  double tfd;     /* intermediate stick surface temperature            (oC) */
  double tfract;  /* fraction of time elapsed between updates          (dl) */
  double ta;      /* air temperature interpolated between obs          (oC) */
  double tdp;     /* dewpoint temperature                              (oC) */
  double tdw;     /* dewpoint temperature                              (oK) */
  double tka;     /* air temperature                                   (oK) */
  double tkf;     /* stick temperature                                 (oK) */
  double tsk;     /* sky temperature                                   (oK) */
  double tt;      /* elapsed moisture computation time                  (h) */
  double oldw;    /* previous value of w[0]                           (g/g) */
  double wdiff;   /* maximum minus current fiber saturation           (g/g) */
  double wold[FMS_MAX_NODES]; /* moisture content at previous time step     */
  double sold[FMS_MAX_NODES]; /* saturation at previous time step           */
  double told[FMS_MAX_NODES]; /* temperature at previous time step     (oC) */
  double g[FMS_MAX_NODES]; /* free water transport coefficient      (cm2/h) */
  double o[FMS_MAX_NODES]; /* used to redistribute moisture content         */
  double v[FMS_MAX_NODES]; /* used to redistribute fuel temperature         */
  double ae, ap, ar, aw;   /* interpolation parameters                      */
  int    i, less, do_gnu;

  //Increment update counter.
  fms->updates++;

  //Catch bad data here.
  if( rcumCm < fms->ra1 ) {
    fprintf( stderr, "****** Update %ld has bad rcumCom = %f\n",
             fms->updates, rcumCm );
    //Assume a RAWS station reset.
    fms->rc1 = rcumCm;
    fms->ra0 = 0.;
    return;
  }
  if( hFtn <0.001 || hFtn> 1.0 ) {
    fprintf( stderr, "****** Update %ld has bad hFtn = %f\n",
             fms->updates, hFtn );
    return;
  }
  if( aC <-40. || aC> 50. ) {
    fprintf( stderr, "****** Update %ld has bad aC = %f\n",
             fms->updates, aC );
    return;
  }
  if( sW <0.0 || sW> 2000. ) {
    fprintf( stderr, "****** Update %ld has bad sW = %f\n",
             fms->updates, sW );
    return;
  }

  //First save the previous weather observation values.
  fms->ta0 = fms->ta1;  //Previous air temperature (oC).
  fms->ha0 = fms->ha1;  //Previous air relative humidity (g/g).
  fms->sv0 = fms->sv1;  //Previous pyranometer voltage (millivolts).
  fms->rc0 = fms->rc1;  //Previous cumulative rainfall (cm).
  fms->ra0 = fms->ra1;  //Previous period's rainfall amount (cm).

  //Then save the current weather observation values.
  fms->ta1 = aC;        //Current air temperature (oC).
  fms->ha1 = hFtn;      //Current air relative humidity (g/g).
  fms->sv1 = sW / Smv;  //Current pyranometer voltage (millivolts).
  fms->rc1 = rcumCm;    //Current cumulative rainfall (cm).

  //Precipitation amount since last observation adjusted by Pi (cm).
  fms->ra1 = (fms->rc1 - fms->rc0) / Pi;

  //If the elapsed time < 0.00027 (<0.01 sec), then treat this as a
  //duplicate or corrected observation and return.
  if( eHr < 0.0000027 ) {
    fprintf( stderr, "****** Update %ld has bad eHr = %f\n",
             fms->updates, eHr );
    return;
  }

  //rai = factor related to runoff during rain (g/(g-h)).
  //Its value depends on whether this is the first rainfall observation,
  //but does not vary within an observation, so its calculated here.
  if( fms->ra1 > 0.00001 && fms->ra0 < 0.00001 ) {
    rai = fms->rai0 * (1.0 - exp(-100. * fms->ra1));
    if( fms->ha1 < fms->ha0 ) rai = 0.15 * rai;
  }
  else rai = fms->rai1 * fms->ra1 / eHr;
  rai *= fms->mdt;

  //ddtNext = next time (tt) to run diffusivity computations.
  ddtNext = fms->ddt;

  //Loop for each moisture time step between environmental inputs.
  for( tt = fms->mdt; tt <= eHr + fms->mdt; tt += fms->mdt ) {
    //Interpolate environmental values between old and current obs.
    tfract = tt / eHr;
    ta = fms->ta0 + (fms->ta1 - fms->ta0) * tfract;
    ha = fms->ha0 + (fms->ha1 - fms->ha0) * tfract;
    sv = fms->sv0 + (fms->sv1 - fms->sv0) * tfract;

    //fsc = fraction of the solar constant.
    fsc = 0.07 * sv;

    //tka = ambient air temperature (oK).
    tka = ta + Kelvin;

    //tdw = dew point temperature (oK).
    tdw = 5205. / ((5205. / tka) - log(ha));

    //tdp = dew point temperature (oC).
    tdp = tdw - Kelvin;

    //Day- and night-time parameters.
    // tsk = sky temperature (oK).
    //  hr = longwave radiative surface heat transfer coeff (cal/cm2-h-C).
    //  sr = solar radiation received by half the stick (cal/cm2-h).
    if( fsc < 0.000001 ) {
      tsk = Tcn + Kelvin;
      hr = Hrn;
      sr = 0;
    }
    else {
      tsk = Tcd + Kelvin;
      hr = Hrd;
      sr = Srf * fsc;
    }

    //psa = water saturation vapor pressure in ambient air (cal/cm3).
    psa = 0.0000239 * exp(20.58 - (5205. / tka));

    //pa = water vapor pressure in air (cal/cm3).
    pa = ha * psa;

    //-----------------------------------------------------------------------
    //Compute stick temperature and surface humidity.
    //-----------------------------------------------------------------------

    //tfd = intermediate stick surface temperature (oC).
    tfd = ta + (sr - hr * (ta - tsk + Kelvin)) / (hr + fms->hc);

    //qv = latent heat of vaporization of water (cal/mole).
    qv = 13550. - 10.22 * (tfd + Kelvin);

    //qw = differential heat of sorption of water (cal/mole).
    qw = 5040. * exp(-14. * fms->w[0]);

    //hw = stick heat transfer coeff for vapor diffusion above FSP.
    hw = (fms->hwf * Ap / 0.24) * qv / 18.;

    //fms->t[0] = stick surface temperature (oC).
    fms->t[0] = tfd - (hw * (tfd - ta) / (hr + fms->hc + hw));
    tkf = fms->t[0] + Kelvin;

    //c1 = EMC sorption isotherm parameter (g/g).
    c1 = 0.1617 - 0.001419 * fms->t[0];

    //c2 = EMC sorption isotherm parameter (g/g).
    c2 = 0.4657 + 0.003578 * fms->t[0];

    //wsa = stick fiber saturation point (g/g).
    fms->wsa = c1 * pow(Wsf, c2);
    wdiff = fms->wmax - fms->wsa;

    //ps1 = water saturation vapor pressure at surface temp (cal/cm3).
    ps1 = 0.0000239 * exp(20.58 - (5205. / tkf));

    //p1 = water vapor pressure at stick surface (cal/cm3).
    p1 = pa + Ap * fms->bp * (qv / (qv + qw)) * (tka - tkf);
    if( p1 < 0.000001) p1 = 0.000001;

    //fms->hf = stick surface humidity (g/g).
    if( (fms->hf = p1 / ps1) > Hfs ) fms->hf = Hfs;

    //fms->sem = stick equilibrium moisture content (g/g).
    hf_log = -log(1. - fms->hf);
    fms->sem = c1 * pow(hf_log, c2);

    //-----------------------------------------------------------------------
    //Compute surface moisture content.
    //-----------------------------------------------------------------------

    fms->state = FMS_STATE_None;

    if( fms->ra1 > 0.0 ) {    //1: if it is raining:
      fms->t[0] = tfd;
      fms->hf = Hfs;

      if( fms->ra1 >= fms->stv ) {     //1a: If this is a rainstorm:
        fms->state = FMS_STATE_Rainstorm;
        fms->wfilm = fms->wfilmk;
        fms->w[0] = fms->wmx;
      }
      else {                           //1b: Else this is NOT a rainstorm:
        fms->state = FMS_STATE_Rainfall;
        fms->wfilm = 0.0;
        fms->w[0] += rai;
        if( fms->w[0] > fms->wmx ) fms->w[0] = fms->wmx;
        fms->s[0] = (fms->w[0] - fms->wsa) / wdiff;
        if( fms->s[0] < 0.0 ) fms->s[0] = 0.0;
      }
    }
    else {                    //2: else it is not raining:
      fms->wfilm = 0.;
      //2a: If moisture content exceeds the fiber saturation point:
      if( fms->w[0] > fms->wsa ) {
        p1 = ps1;
        fms->hf = Hfs;

        //psd = water saturation vapor pressure at dewpt (cal/cm3).
        psd = 0.0000239 * exp(20.58 - (5205. / tdw));

        //aml = factor related to rate of evap or conden ((g/g)/h).
        if( fms->t[0] <= tdp && p1 > psd ) aml = 0.;
        else aml = fms->amlf * (ps1 - psd) / fms->bp;

        oldw = fms->w[0];
        fms->w[0] = fms->w[0] - aml * fms->mdt_2;
        if( aml > 0.) {
          gnu = 0.00439 + 0.00000177 * pow((338.76 - tkf), 2.1237);
          fms->w[0] = fms->w[0] - fms->mdt * fms->capf / gnu;
        }

        if( fms->w[0] > fms->wmx ) fms->w[0] = fms->wmx;

        //2a1: if moisture content is rising: condensation
        if( fms->w[0] > oldw ) {
          fms->state = FMS_STATE_Condensation;
          fms->s[0] = (fms->w[0] - fms->wsa) / wdiff;
        }
        //2a2: else if moisture content is steady: stagnation
        else if( fms->w[0] == oldw ) fms->state = FMS_STATE_Stagnation;
        //2a3: else if moisture content is falling: evaporation
        else if( fms->w[0] < oldw ) {
          fms->state = FMS_STATE_Evaporation;
          if( (fms->s[0] = (fms->w[0] - fms->wsa) / wdiff) < 0.)
            fms->s[0] = 0.;
        }
      }
      //2b: else if fuel temperature is less than dewpoint: condensation
      else if( fms->t[0] <= tdp ) {
        fms->state = FMS_STATE_Condensation;

        //psd = water saturation vapor pressure at dewpt (cal/cm3).
        psd = 0.0000239 * exp(20.58 - (5205. / tdw));
        if( p1 > psd ) aml = 0.0;
        else aml = fms->amlf * (p1 - psd) / fms->bp;
        fms->w[0] = fms->w[0] - aml * fms->mdt_2;
        if( (fms->s[0] = (fms->w[0] - fms->wsa) / wdiff) < 0.) fms->s[0] = 0.;
      }
      //2c: else surface moisture content less than fiber saturation pt
      //         and stick temperature greater than dewpoint ...
      else {
        // fms->sem computations were moved out of here back to the
        // top of the function so its available regardless of state.

        if( fms->w[0] != fms->sem ) {
          //2c1: if surface moisture greater than equilibrium: desorption
          if( fms->w[0] > fms->sem ) {
            fms->state = FMS_STATE_Desorption;
            bi = fms->stcd * fms->dx / fms->d[0];
          }
          //2c2: else surface moisture less than equilibrium: adsorption
          else {
            fms->state = FMS_STATE_Adsorption;
            bi = fms->stca * fms->dx / fms->d[0];
          }
          fms->w[0] = (fms->w[1] + bi * fms->sem) / (1. + bi);
          fms->s[0] = 0.;
        }
      }
    }  //end of not raining

    //-----------------------------------------------------------------------
    //Compute interior nodal moisture content values.
    //-----------------------------------------------------------------------

    for( i = 0; i < fms->n; i++ ) {
      wold[i] = fms->w[i];
      sold[i] = fms->s[i];
      told[i] = fms->t[i];
      v[i] = Thdiff * fms->x[i];
      o[i] = 0.0;
      g[i] = 0.0;
    }

    //Skip the rest if in stagnation state.
    if( fms->state != FMS_STATE_Stagnation ) {
      do_gnu = 1;  //do gnu computation once per mdt iteration

      for( i = 0; i < fms->n; i++ ) {
        // par = dummy parameter for fms->s[i]
        par = (fms->w[i] - fms->wsa) / wdiff;
        fms->s[i] = 0.0;
        if( par > 0. ) {
          fms->s[i] = par;
          if( par > Sir && par < Scr ) {
            // ak = permeability of stick under nonsaturated conditions (cm2).
            ak = Aks * (2. * sqrt(par / Scr) - 1.);

            // gnu = kinematic viscosity of liquid water (cm2/s).
            if( do_gnu == 1 ) {  // do this just one time
              gnu = 0.00439 + 0.00000177 * pow( (338.76 - tkf), 2.1237 );
              do_gnu = 0;   // don't do this again
            }

            // g = free water transport coefficient (cm2/h) for
            //calculating new s[].
            g[i] = (ak / (gnu * wdiff)) * fms->x[i] * fms->vf *
                   pow( (Scr / fms->s[i]), 1.5 );
          }
        }   //end if par > 0.
        o[i] = fms->x[i] * fms->d[i];
      }

      // Propagate saturation changes
      for( i = 1; i < fms->n - 1; i++ ) {
        ae = g[i + 1] / fms->dx;
        aw = g[i - 1] / fms->dx;
        ar = fms->x[i] * fms->dx / fms->mdt;
        ap = ae + aw + ar;
        fms->s[i] = (ae * sold[i + 1] + aw * sold[i - 1] + ar * sold[i]) / ap;
        if( fms->s[i] > 1. ) fms->s[i] = 1.;
      }
      fms->s[fms->n - 1] = fms->s[fms->n - 2];

      // Check if fms->s[] is less than Sir (limit of continuous liquid
      // columns) at ANY stick node.
      for( less = 0, i = 1; i < fms->n - 1; i++ ) {
        if( fms->s[i] < Sir ) {
          less = 1;
          break;
        }
      }

      // If all nodes have continuous liquid columns (s >= Sir):
      if( less == 0 ) {
        for( i = 1; i < fms->n - 1; i++ ) {
          fms->w[i] = fms->wsa + fms->s[i] * wdiff;
          if( fms->w[i] > fms->wmx ) fms->w[i] = fms->wmx;
        }
      }
      // Else at least one node has s < Sir:
      else {
        // Propagate moisture changes
        for( i = 1; i < fms->n - 1; i++ ) {
          ae = o[i + 1] / fms->dx;
          aw = o[i - 1] / fms->dx;
          ar = fms->x[i] * fms->dx / fms->mdt;
          ap = ae + aw + ar;
          fms->w[i] = (ae * wold[i + 1] + aw * wold[i - 1] + ar * wold[i]) /
                      ap;
          if( fms->w[i] > fms->wmx ) fms->w[i] = fms->wmx;
        }
      }
      fms->w[fms->n - 1] = fms->w[fms->n - 2];
    }

    // Propagate temperature changes
    for( i = 1; i < fms->n - 1; i++ ) {
      ae = v[i + 1] / fms->dx;
      aw = v[i - 1] / fms->dx;
      ar = fms->x[i] * fms->dx / fms->mdt;
      ap = ae + aw + ar;
      fms->t[i] = (ae * told[i + 1] + aw * told[i - 1] + ar * told[i]) / ap;
      if( fms->t[i] > 71. ) fms->t[i] = 71.;
    }
    fms->t[fms->n - 1] = fms->t[fms->n - 2];

    // Update moisture diffusivity if within less than half a time step.
    if( (ddtNext - tt) < (0.5 * fms->mdt) ) {
      Fms_Diffusivity( fms );
      ddtNext += fms->ddt;
    }
  }  // Next observation
} //Fms_Update

/*============================================================================
  Fms_ReadFromAsciiStream
  Example function of reading all the required stick properties  from a file
  into a fuel moisture stick object.

  Some applications may find it necessary to store the state of fuel moisture
  sticks to a file between weather observation. This function is an example of
  what properties need to be read from the file into the stick's internal
  structure for an ensuing update. Most applications that need to perform
  permanent storage will have a custom routine that reads/writes these same
  properties, but to a binary or data base file.

   fms  - Pointer to the fuel moisture stick object
   fptr - Pointer to a file stream opened for input

  Note:
  The fms argument must point to an existing Fms object that has the
  appropriate number of nodes allocated. The existing fms->name will be free'd
  and then reallocated with the new name string read from the file.
  The existing fms->s[], fms->w[], etc will be reused without any 
  deallocation/allocation.

  See also Fms_WriteToAsciiStream().
*/
void Fms_ReadFromAsciiStream( Fms* fms, FILE* fptr )
{ //Fms_ReadFromAsciiStream
  int  i, j;
  char name[64], fname[256];

  //Stick attributes passed to Fms_CreateStick().
  fscanf( fptr, "%s %[^\n]\n", name, fname );
  if( fms->name ) {
    delete[] fms->name;
    fms->name = 0;
    fms->name = new char[strlen(fname + 1)];
    strcpy( fms->name, fname );
  }
  fscanf( fptr, "%s %d\n", name, &(fms->n) );
  fscanf( fptr, "%s %lf\n", name, &(fms->a) );
  fscanf( fptr, "%s %lf\n", name, &(fms->al) );
  fscanf( fptr, "%s %lf\n", name, &(fms->dp) );
  fscanf( fptr, "%s %lf\n", name, &(fms->mdt) );
  fscanf( fptr, "%s %lf\n", name, &(fms->ddt) );
  fscanf( fptr, "%s %lf\n", name, &(fms->bp) );
  fscanf( fptr, "%s %lf\n", name, &(fms->wmx) );
  fscanf( fptr, "%s %lf\n", name, &(fms->hc) );
  fscanf( fptr, "%s %lf\n", name, &(fms->stca) );
  fscanf( fptr, "%s %lf\n", name, &(fms->stcd) );
  fscanf( fptr, "%s %lf\n", name, &(fms->rai0) );
  fscanf( fptr, "%s %lf\n", name, &(fms->rai1) );
  fscanf( fptr, "%s %lf\n", name, &(fms->stv) );
  fscanf( fptr, "%s %lf\n", name, &(fms->wfilmk) );
  // Stick environmental variables set in Fms_Update().
  fscanf( fptr, "%s %d\n", name, &(fms->init) );
  fscanf( fptr, "%s %lf\n", name, &(fms->jdate) );
  fscanf( fptr, "%s %lf\n", name, &(fms->ta0) );
  fscanf( fptr, "%s %lf\n", name, &(fms->ta1) );
  fscanf( fptr, "%s %lf\n", name, &(fms->ha0) );
  fscanf( fptr, "%s %lf\n", name, &(fms->ha1) );
  fscanf( fptr, "%s %lf\n", name, &(fms->sv0) );
  fscanf( fptr, "%s %lf\n", name, &(fms->sv1) );
  fscanf( fptr, "%s %lf\n", name, &(fms->rc0) );
  fscanf( fptr, "%s %lf\n", name, &(fms->rc1) );
  fscanf( fptr, "%s %lf\n", name, &(fms->ra0) );
  fscanf( fptr, "%s %lf\n", name, &(fms->ra1) );
  // Stick-dependent intermediate constants set in Fms_CreateParameters().
  fscanf( fptr, "%s %lf\n", name, &(fms->dx) );
  fscanf( fptr, "%s %lf\n", name, &(fms->wmax) );
  // Stick-dependent optimization factors set in Fms_CreateParameters().
  fscanf( fptr, "%s %lf\n", name, &(fms->hwf) );
  fscanf( fptr, "%s %lf\n", name, &(fms->amlf) );
  fscanf( fptr, "%s %lf\n", name, &(fms->capf) );
  fscanf( fptr, "%s %lf\n", name, &(fms->mdt_2) );
  fscanf( fptr, "%s %lf\n", name, &(fms->dx_2) );
  fscanf( fptr, "%s %lf\n", name, &(fms->sf) );
  fscanf( fptr, "%s %lf\n", name, &(fms->vf) );
  // Stick- & weather-dependent output parameters derived in Fms_Update().
  fscanf( fptr, "%s %lf\n", name, &(fms->hf) );
  fscanf( fptr, "%s %lf\n", name, &(fms->wsa) );
  fscanf( fptr, "%s %lf\n", name, &(fms->sem) );
  fscanf( fptr, "%s %lf\n", name, &(fms->wfilm) );
  fscanf( fptr, "%s %ld\n", name, &(fms->updates) );
  fscanf( fptr, "%s %d\n", name, &(fms->state) );
  for( i = 0; i < fms->n; i++ ) {
    fscanf( fptr, "    %d %lf %lf %lf %lf %lf %lf\n", &j, &(fms->x[i]),
            &(fms->t[i]), &(fms->d[i]), &(fms->s[i]), &(fms->w[i]),
            &(fms->v[i]) );
  }
} //Fms_ReadFromAsciiStream

/*============================================================================
  Fms_WriteToAsciiStream
  Example function of writing all the required stick properties from a fuel
  moisture stick object into permanent (disk) storage.

  Some applications may find it necessary to store the state of fuel moisture
  sticks to a file between weather observation. This function is an example of
  what properties need to be written to the file from the stick's internal
  structure to accommodate subsequent updates. Most applications that need to
  perform permanent storage will have a custom routine that reads/writes these
  same properties, but to a binary or data base file.

   fms  - Pointer to the fuel moisture stick object.
   fptr - Pointer to a file stream opened for output.

  Note: The "Fms *fms" argument must point to an existing Fms object that has
  the appropriate number of nodes allocated. The existing fms->name will be
  free'd and then reallocated with the new name string read from the file.
  The existing fms->s[], fms->w[], etc will be reused without any 
  deallocation/allocation.

  See also Fms_ReadFromAsciiStream().
*/
void Fms_WriteToAsciiStream( Fms* fms, FILE* fptr )
{ //Fms_WriteToAsciiStream
  int i;

  //Stick attributes passed to Fms_CreateStick().
  fprintf( fptr, "name    %s\n", fms->name );
  fprintf( fptr, "n       %d\n", fms->n );
  fprintf( fptr, "a       %f\n", fms->a );
  fprintf( fptr, "al      %f\n", fms->al );
  fprintf( fptr, "dp      %f\n", fms->dp );
  fprintf( fptr, "mdt     %f\n", fms->mdt );
  fprintf( fptr, "ddt     %f\n", fms->ddt );
  fprintf( fptr, "bp      %f\n", fms->bp );
  fprintf( fptr, "wmx     %f\n", fms->wmx );
  fprintf( fptr, "hc      %f\n", fms->hc );
  fprintf( fptr, "stca    %f\n", fms->stca );
  fprintf( fptr, "stcd    %f\n", fms->stcd );
  fprintf( fptr, "rai0    %f\n", fms->rai0 );
  fprintf( fptr, "rai1    %f\n", fms->rai1 );
  fprintf( fptr, "stv     %f\n", fms->stv );
  fprintf( fptr, "wfilmk  %f\n", fms->wfilmk );
  // Stick environmental variables set in Fms_Update().
  fprintf( fptr, "init    %d\n", fms->init );
  fprintf( fptr, "jdate   %f\n", fms->jdate );
  fprintf( fptr, "ta0     %f\n", fms->ta0 );
  fprintf( fptr, "ta1     %f\n", fms->ta1 );
  fprintf( fptr, "ha0     %f\n", fms->ha0 );
  fprintf( fptr, "ha1     %f\n", fms->ha1 );
  fprintf( fptr, "sv0     %f\n", fms->sv0 );
  fprintf( fptr, "sv1     %f\n", fms->sv1 );
  fprintf( fptr, "rc0     %f\n", fms->rc0 );
  fprintf( fptr, "rc1     %f\n", fms->rc1 );
  fprintf( fptr, "ra0     %f\n", fms->ra0 );
  fprintf( fptr, "ra1     %f\n", fms->ra1 );
  // Stick-dependent intermediates derived in Fms_CreateParameters().
  fprintf( fptr, "dx      %f\n", fms->dx );
  fprintf( fptr, "wmax    %f\n", fms->wmax );
  // Stick-dependent optimization factors set in Fms_CreateParameters().
  fprintf( fptr, "hwf     %f\n", fms->hwf );
  fprintf( fptr, "amlf    %f\n", fms->amlf );
  fprintf( fptr, "capf    %f\n", fms->capf );
  fprintf( fptr, "mdt_2   %f\n", fms->mdt_2 );
  fprintf( fptr, "dx_2    %f\n", fms->dx_2 );
  fprintf( fptr, "sf      %f\n", fms->sf );
  fprintf( fptr, "vf      %f\n", fms->vf );
  // Stick- & weather-dependent outputs derived in Fms_Update().
  fprintf( fptr, "hf      %f\n", fms->hf );
  fprintf( fptr, "wsa     %f\n", fms->wsa );
  fprintf( fptr, "sem     %f\n", fms->sem );
  fprintf( fptr, "wfilm   %f\n", fms->wfilm );
  fprintf( fptr, "updates %ld\n", fms->updates );
  fprintf( fptr, "state   %d\n", fms->state );
  for( i = 0; i < fms->n; i++ ) {
    fprintf( fptr, "%5d %f %f %f %f %f %f\n", i, fms->x[i], fms->t[i],
             fms->d[i], fms->s[i], fms->w[i], fms->v[i] );
  }
} //Fms_WriteToAsciiStream

/*============================================================================
  CDT_CalendarDate
  Determines the Western calendar date from the Julian date.

  Calculates the Western (Julian-Gregorian) calendar date from the Julian date
  jdate using the method described by Duffett-Smith (and similarly by Meeus).

  I used Montenbruck & Pfleger (p 13) because it gave the correct calendar 
  date for Julian date 1.0, whereas the Meeus (p 26) and Duffett-Smith (p 11) 
  algorithms said Julian date 1 is -4712 Jan 02.

  Warning: No date or time validation is performed.
   jdate        - Julian date as returned by CDT_JulianDate().
   year         - Returned year (-4712 (4713 B.C.) or greater).
   month        - Returned month of the year (1-12).
   day          - Returned day of the month (1-31).
   hour         - Returned hours past midnight (0-23).
   minute       - Returned minutes past the hour (0-59).
   second       - Returned seconds past the minute (0-59).
   milliseconds - Returned milliseconds past the second (0-999).
  All calculated values are returned in the function parameters.
*/
void CDT_CalendarDate( double jd, int* year, int* month, int* day, int* hour,
                       int* minute, int* second, int* millisecond )
{ //CDT_CalendarDate
  int    b, d, f;
  double jd0, c, e, dh, dm, ds, ms;

  jd0 = (double) ((long) (jd + 0.5));

  if( jd0 < 2299161.0) c = jd0 + 1524.0;
  else {
    b = (int) ((jd0 - 1867216.25) / 36524.25);
    c = jd0 + (b - (int) (b / 4)) + 1525.0;
  }
  d = (int) ((c - 122.1) / 365.25);
  e = 365.25 * d; //CORRECTED
  f = (int) ((c - e) / 30.6001);

  *day = (int) (c - e + 0.5) - (int) (30.6001 * f);
  *month = f - 1 - 12 * (int) (f / 14);
  *year = d - 4715 - (int) ((7 + *month) / 10);
  dh = 24.0 * (jd + 0.5 - jd0);
  *hour = (int) dh;
  dm = 60. * (dh - (double) * hour);
  *minute = (int) dm;
  ds = 60. * (dm - (double) * minute);
  *second = (int) ds;
  ms = 1000. * (ds - (double) * second);
  *millisecond = (int) ms;
} //CDT_CalendarDate

/*============================================================================
  CDT_DayOfYear
  Determines the day-of-the-year number.

  Day 1 is always January 1 of the year. The day number is adjusted for the
  occurrence of leap years in the Julian and Gregorian calendars.
  An adjustment is also made for the Gregorian calendar reform of 1582 when
  Pope Gregory dropped Oct 5-14 from the Julian calendar to begin the 
  Gregorian calendar (thus, 1582 had only 355 days).

   year  - Year (-4712 or later)
   month - Month of the year (1-12)
   day   - Day of the month (1-31)
   Returns day of the year (1-366).
 */
int CDT_DayOfYear( int year, int month, int day )
{ //CDT_DayOfYear
  //Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
  static int DaysToMonth[] =
                    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

  int doy = day + DaysToMonth[month - 1];

  // 1582 AD is missing the ten days of Oct 5-14
  if( year == 1582 && doy > 277 ) doy -= 10;
  else if( doy > 59 && CDT_LeapYear(year) ) doy++;

  return( doy );
} //CDT_DayOfYear

/*============================================================================
  CDT_DecimalDay
  Determines the elapsed portion of the day since midnight.

   hour         - Hours past midnight (0-23)
   minute       - Minutes past the hour (0-59)
   second       - Seconds past the minute (0-59)
   milliseconds - Milliseconds past the second (0-999)

   Returns the elapsed portion of the day since midnight in days.
*/
double CDT_DecimalDay( int hour, int minute, int second, int millisecond )
{ //CDT_DecimalDay
  return( (double) CDT_MillisecondOfDay(hour, minute, second, millisecond) /
          86400000.);
} //CDT_DecimalDay

/*============================================================================
  CDT_JulianDate
  Calculates the Julian date from the passed date and time.

  The Julian Date is the number of days that have elapsed since noon,
  12h Universal Time on January 1, 4713 B.C.

  The Julian Date was developed in 1583 by French scholar Joseph Justus
  Scaliger. The beginning day of January 1, 4713 B.C. is the previous
  coincidence of the 28-year solar cycle, the 19-year Golden Number (Metonic)
  lunar cycle, and the 15-year indiction cycle used for Roman taxation.

  The Gregorian calendar reform is taken into account when calculating the
  Julian Date. Thus the day following 1582 October 4 is 1582 October 15.
  The Julian calendar is used through 1582 Oct 4 (leap years every 4th year),
  and the Gregorian calendar is used for dates on and after 1582 Oct 15
  (leap year exceptions). Together, the Julian-Gregorian calendar system may
  be referred to simply as the Western calendar.

  The "B.C." years are counted astronomically; the year before A.D. 1 
  is year 0, and the year preceeding this is -1.

  References:

  Duffett-Smith, Peter. 1981.  Practical astronomy with your calculator.
  Second Edition. Cambridge University Press. 188 pp. (see page 9).

  Latham, Lance. 1998. Standard C date/time library. Miller-Freeman. 560 pp.
  (see page 41).

  Meeus, Jean. 1988. Astronomical formulae for calculators. Fourth 
  Edition. Willman-Bell, Inc. 218 pp. (see page 24).

  Montenbruck, Oliver; Pfleger, Thomas.  Astronomy on the personal computer.
  Third Edition. Springer.  312 pp. (see page 13).
  Warning: No date or time validation is performed.

  Bug
  While all authors agree that the Julian Date starts at zero at 12:00 noon of
  4713 B.C. January  1, the Duffett-Smith and Meeus algorithms actually yields
  a JD of 1.0 for this date and time!  The function here correctly reproduces
  all the examples in their texts, but yield a JD of 1.0 for -4712
  Jan 1 12:00.

    Calendar Date          Julian Date   Source
     1985 Feb 17 06:00:00  2,446,113.75  (Duffett-Smith p9)
     1980 Jan 40 00:00:00  2,444,238.50  (Duffett-Smith, p10)
     1957 Oct 04 19:26:24  2,436,116.31  (Meeus, p23)
     0333 Jan 27 12:00:00  1,842,713.00  (Meeus, p24)
    -4712 Jan 01 12:00:00  1.00          Should be 0.0!

  Note the last example.

  While this certainly deviates from the formal definition of JD, I will
  continue to use this algorithm since I also use their other algorithms
  for Julian-to-calendar conversions and astronomical date derivations.

   year         - -4712 (4713 B.C.) or greater
   month        - Month of the year (1-12)
   day          - Day of the month (1-31)
   hour         - Hours past midnight (0-23)
   minute       - Minutes past the hour (0-59)
   second       - Seconds past the minute (0-59)
   milliseconds - Milliseconds past the second (0-999)
  Returns the Julian date in decimal days since 1 Jan -4712.
*/
double CDT_JulianDate( int year, int month, int day, int hour, int minute,
                       int second, int millisecond )
{ //CDT_JulianDate
  double jdate;
  int    a, b, c, d;

  jdate = 10000 * year + 100 * month + day;
  if( month <= 2 ) {
    year--;
    month += 12;
  }

  b = 0;
  if( jdate >= 15821015.0 ) {
    a = (int) (year / 100);
    b = 2 - a + (int) (a / 4);
  }
  c = (int) (365.25 * year);
  d = (int) (30.6001 * (month + 1));
  jdate = b + c + d + day +
          CDT_DecimalDay(hour, minute, second, millisecond) + 1720994.5;

  return( jdate );
} //CDT_JulianDate

/*============================================================================
  CDT_LeapYear
  Determines if the specified year is a Julian-Gregorian leap year.
   year - Year to be assessed (-4712 or later).
  Returns number of leap days in the year (0 or 1).
*/
int CDT_LeapYear( int year )
{ //CDT_LeapYear
  //If its not divisible by 4, its not a leap year.
  if( year % 4 != 0 ) return( 0 );
  //All years divisible by 4 prior to 1582 were leap years.
  else if( year < 1582 ) return( 1 );
  //If divisible by 4, but not by 100, its a leap year.
  else if( year % 100 != 0 ) return( 1 );
  //If divisible by 100, but not by 400, its not a leap year.
  else if( year % 400 != 0 ) return( 0 );
  //If divisible by 400, its a leap year.
  else return( 1 );
} //CDT_LeapYear

/*============================================================================
  CDT_MillisecondOfDay
  Determines the milliseconds elapsed since midnight.

   hour         - Hours past midnight (0-23)
   minute       - Minutes past the hour (0-59)
   second       - Seconds past the minute (0-59)
   milliseconds - Milliseconds past the second (0-999)

  Returns the elapsed time since midnight in milliseconds.
  See also CDT_DecimalDay(), CDT_DecimalHour().
*/
int CDT_MillisecondOfDay( int hour, int minute, int second, int millisecond )
{ //CDT_MillisecondOfDay
  return( millisecond + 1000 * second + 60000 * minute + 3600000 * hour );
} //CDT_MillisecondOfDay