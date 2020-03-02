/*fsairatk.h
  Aerial Suppression Capabilities.
  Copyright 1997  Mark A. Finney, Systems for Environmental Management
  See LICENSE.TXT file for license information.
*/
#ifndef FSAIRATK_H
#define FSAIRATK_H

#include<stdlib.h>
#include<math.h>
#include<string.h>
#include<time.h>
#include<sys/timeb.h>
#include"fsxwbar.h"
#include"fsxlandt.h"

//----------------------------------------------------------------------------
//Bomber Class and Functions.
//----------------------------------------------------------------------------

//============================================================================
struct AirCraft
{ //AirCraft
  char   AirCraftName[256];
  long   Units;    //Meters by default 0, 1 for feet
  double PatternLength[6];
  double ReturnTime;
  double Cost;
  long   NumDrops;

  AirCraft();
};//AirCraft

//----------------------------------------------------------------------------
//Air Attack Class and Functions.
//----------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  long   AirAttackNumber;  //Unique index to attack sequence
  long   AirCraftNumber;  //Index to AirCraft type
  long   CoverageLevel;  //Retardant coveragelevel
  long   PatternNumber;  //Number of retardant pattern in fire perimeter array
  double ElapsedTime;  //Time elapsed since attack started
  double EffectiveDuration;  //Duration (minutes) of retardant
  void* next;
} AirAttackData;

//============================================================================
class AirAttack
{ //AirAttack
  AirAttackData* attack;

public:
  long AirAttackNumber;

  AirAttack();
  ~AirAttack();

  bool CheckEffectiveness( AirAttackData* atk, double TimeStep );
};//AirAttack

#define GROUPSIZE 20

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  long    Suspended;    //Flag for suspension of group air attack
  char    GroupName[256];    //Name of group air attack
  long    GroupAttackNumber;    //Unique ID for group attack
  long    Direction;    //Direction around fire front, original or reversed
  double* IndirectLine;    //Array of vertices to follow for indirectattack
  long    NumPoints;    //Number of points in indirect line
  long    FireNumber;    //Fire number
  long*   CoverageLevel;    //Array of coverage levels for indiv aircraft
  long*   EffectiveDuration;    //Array of retardant duraitons for aircraft
  long*   AircraftNumber;    //Array of aircraft types used here
  double* WaitTime;    //Indiviual wait times until drop for each aircraft
  long    NumCurrentAircraft;    //Num aircraft in group
  long    NumTotalAircraft;    //Num currently allocated for group
  void*   next;    //Pointer to next group
} GroupAttackData;

//============================================================================
class GroupAirAttack
{ //GroupAirAttack
  GroupAttackData* attack;

public:
  long GroupAttackNumber;

  GroupAirAttack();
  GroupAirAttack( GroupAttackData* groupattack );
  ~GroupAirAttack();
  void GetCurrentGroup();
  void SetGroup( GroupAttackData* gatk );
  bool AllocGroup( long NewNumber );
  bool AddGroupMember( long AircraftNumber, long CoverageLevel,
                       long EffectiveDuration );
  bool SetGroupAssignment( double* line, long FireNum, bool Reset );
  bool RemoveGroupMember( long membernumber );
  void ReorderGroupList( long Start );
  bool ExecuteAttacks( double TimeIncrement );
  void ExecuteAllIndirectAttacks( double TimeIncrement );
  double GetNextAttackTime( double TimeIncrement );
  void IncrementWaitTimes( double TimeIncrement );
  long CheckSuspendState( long OnOff );
};//GroupAirAttack

//----------------------------------------------------------------------------
//Global Air Attack Support Functions.
//----------------------------------------------------------------------------

long GetNumAirCraft();
void SetNumAirCraft( long NumCraft );
bool LoadAirCraft( char* FileName, bool AppendList );
long SetNewAirCraft();
AirCraft* GetAirCraft( long AirCraftNumber );
long SetAirCraft( long AirCraftNumber );
void FreeAirCraft( long AirCraftNumber );

long SetupAirAttack( long AirCraftNumber, long CoverageLevel, long Duration,
                     double* startpoint );
void CancelAirAttack( long AirAttackCounter );
void LoadAirAttack( AirAttackData airattackdata );
void FreeAllAirAttacks();
AirAttackData* GetAirAttack( long AirAttackCounter );
AirAttackData* GetAirAttackByOrder( long OrdinalAttackNumber );
void SetNewFireNumberForAirAttack( long OldFireNum, long NewFireNum );
long GetNumAirAttacks();
void SetNumAirAttacks( long NumAtk );

long SetupGroupAirAttack( long AirCraftNumber, long CoverageLevel,
                          long Duration, double* line, long FireNum,
                          char* GroupName );
GroupAttackData* ReassignGroupAirAttack( GroupAttackData* atk );
void CancelGroupAirAttack( long GroupCounter );
void LoadGroupAttack( GroupAttackData groupattackdata );
void FreeAllGroupAirAttacks();
GroupAttackData* GetGroupAirAttack( long GroupAttackCounter );
GroupAttackData* GetGroupAttackByOrder( long OrdinalAttackNumber );
GroupAttackData* GetGroupAttackForFireNumber( long CurrentFire, long StartNum,
                                              long* LastNum );
void SetNewFireNumberForGroupAttack( long OldFireNum, long NewFireNum );
long GetNumGroupAttacks();
void SetNumGroupAttacks( long NumGroups );

void WriteAirAttackLog( AirAttackData* atk, char* AirAttackLog );

//Externs
extern long NumAircraft;
extern AirCraft* aircraft[500];

//Constants
const int    METERS    =   0;  //For Aircraft Units
const int    FEET      =   1;  //For Aircraft Units

#endif
