/*fsxwattk.h
  Suppression Capabilities
  Copyright 1994, 1995, 1996
  Mark A. Finney, Systems for Environemntal Management

  See LICENSE.TXT file for license information.
*/
#ifndef FSXWATTK_H
#define FSXWATTK_H

#include<stdlib.h>
#include<math.h>
#include<string.h>
#include<time.h>
#include<sys/timeb.h>
#include "fsxwbar.h"
#include "fsxlandt.h"

//----------------------------------------------------------------------------
//Crew Class and Functions
//----------------------------------------------------------------------------

//============================================================================
struct Crew
{ //Crew
  char   CrewName[256];
  long   Compound;
  long   Units;
  double FlameLimit;
  double LineProduction[51];
  double Cost;
  double FlatFee;   //Added Sep2010. Amt charged regardless of Crew's usage.

  Crew();
};//Crew

#define COMPOUNDINC 20

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  long    NumCurrentCrews;  //Number of crews in compound
  long    NumTotalCrews;
  long    CompCrew;   //Index to crew
  long*   CrewIndex;  //Array of crew indices
  double* Multiplier;   //Multipliers on spread rates
} CompoundCrew;

//------------------------------------------------------------------------
//Attack Class and Functions
//------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  long    AttackNumber;  //Unique attack ID number
  long    FireNumber;  //Store fire number that is being attacked
  long    LineNumber;  //Only used for parallel attack
  long    CrewNum;    //Store index to crew number
  long    BurnDelay;  //Burn delay (m)
  long    FireDist;  //Distance from fire edge for parallel attack
  long    CurrentPoint;   //Current point on fire perim or indirect segment
  long    NextPoint;  //Next point on fire perim or indirect segment
  long    Burnout;    //If burnout is desired
  long    Suspended;   //If suspended 1 else 0
  long    Indirect;  //Stores tactics for this attack 0, 1, or 2
  long    Reverse;     //If reverse around fire perimeter
  long    NumPoints;  //Number of points in indirect attack
  long    BurnLine[2];  //Hold endpts of line segment for burnout
  long    BurnDirection;  //0 for left, 1 for right
  double  AttackTime;  //Total time of attack
  double  LineBuilt;  //Total length of line built
  double* IndirectLine1;   //Store indirect line route
  double* IndirectLine2;  //Alternative array for indirect line route
  void*   next;
} AttackData;

//============================================================================
class Attack
{ //Attack
  AttackData* attack;
  double xpt1, ypt1, ros1, fli1;
  double xpt1n, ypt1n, ros1n, fli1n;
  double xpt2, ypt2, ros2, fli2;
  double xpt2n, ypt2n, ros2n, fli2n;
  double rcx1, rcx1n, rcx2, rcx2n;
  double ChordArcRatio;     //Ratio of chord dist to arc dist on circle
  double LineRate;
  double LineOffset;  //Random distance from stationary indirect line
  long NumInsertPoints;
  VectorBarrier vectorbarrier;
  LandscapeData ld;

  void BurnOut();
  void BufferBurnout( double xptl, double yptl, double xpt, double ypt,
                      double xptn, double yptn, double* x1, double* y1,
                      double* x2, double* y2 );
  long ProblemQuad();
  long FindCurrentPoint();
  bool Cross( double xpt1, double ypt1, double xpt2, double ypt2,
              double xpt1n, double ypt1n, double xpt2n, double ypt2n,
              double* newx, double* newy );
  bool IterateIntersection( double LineDist, double extendx, double extendy,
                            double xstart, double ystart, double xend,
                            double yend, double* newx, double* newy );
  void InsertPerimeterPoint( double newxpt, double newypt, double newros,
                             double newfli, double newrcx, long InsertType );
  double pow2( double input );
  void ConvertLandData( short slope, short aspect );
  double CalculateSlopeDist( double x1, double y1, double x2, double y2 );
  void GetConvexHull( double* Hull1, long* NumHullPts );
  void FindHullQuadrant( double* Hull1, long* NumHullPts, long StartPt,
                         long EndPt, double RefAngle );
  void ExpandConvexHulls( double* Hull1, double* Hull2, long NumHullPts );
  long FindHullPoint( double* Hull2, long NumHullPts, long* HullPt );
  bool AllocParallelLine( double* Hull, long NumHullPts, double TimeStep );
  void HullDensityControl( double* Hull1, double* Hull2, long* NumHullPts );
  void CalcChordArcRatio( double LastX, double LastY );

public:
  Attack();
  ~Attack();
  bool DirectAttack( AttackData* attack, double TimeStep );
  bool IndirectAttack( AttackData* attack, double TimeStep );
  bool ParallelAttack( AttackData* atk, double TimeStep );
  void ConductBurnout( AttackData* atk );
  long CheckOverlapAttacks( long* attacks, double TimeStep );
  void BoundingBox();
};//Attack

//----------------------------------------------------------------------------
//Global Suppression Data and Access Functions
//----------------------------------------------------------------------------

//Indirect Attack Constructor.
long SetupIndirectAttack( long CrewNum, double* startpt, long numpts );
long ResetIndirectAttack( AttackData* atk, double* coords, long numpts );
//DirectAttack Constructor.
long SetupDirectAttack(long CrewNum, long FireNum, double* coords );
long ResetDirectAttack( AttackData* atk, long FireNum, double* coords );
long GetNumAttacks();
long GetFireNumberForAttack( long AttackCounter );
AttackData* GetAttackForFireNumber( long NumFire, long start,
                                    long* LastAttackNumber );
void SetNewFireNumberForAttack( long oldnumfire, long newnumfire );
long GetNumCrews();

AttackData* GetReassignedAttack();
void ReassignAttack( AttackData* atk );
AttackData* GetAttack( long AttackCounter );
AttackData* GetAttackByOrder( long OrdinalAttackNum, bool IndirectOnly );
void LoadAttacks( AttackData attackdata );
void CancelAttack( long AttackCounter );
void FreeAllAttacks();
void WriteAttackLog( AttackData* atk, long Type, long Var1, long Var2 );
bool LoadCrews( char* FileName, bool AppendList );
Crew* GetCrew( long CrewNumber );
long SetCrew( long CrewNumber );
long SetNewCrew();
void FreeCrew( long CrewNumber );
void FreeAllCrews();

long GetNumCompoundCrews();
long SetCompoundCrew( long GroupNumber, char* CrewName,
                      long ExistingCrewNumber );
CompoundCrew* GetCompoundCrew( long CrewNumber );
void FreeCompoundCrew( long CrewNumber );
void RemoveFromCompoundCrew( long CompNumber, long CrewNumber );
void FreeAllCompoundCrews();
bool LoadCompoundCrews( char* FileName );
bool AddToCompoundCrew( long CrewNumber, long NewCrew, double Mult );
void CalculateCompoundRates( long CrewNumber );

//Externs
extern long NumCrews;
extern Crew* crew[200];

//Constants
const int    METERS_PER_MINUTE    =   1;  //For Crew Units
const int    FEET_PER_MINUTE      =   2;  //For Crew Units
const int    CHAINS_PER_HOUR      =   3;  //For Crew Units
const int    DIRECT               =   1;  //For AttackType
const int    INDIRECT             =   2;  //For AttackType

#endif