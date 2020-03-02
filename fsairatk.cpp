/*fsairatk.cpp
  Aerial Suppression Capabilities.
  Copyright 1997  Mark A. Finney, Systems for Environmental Management
  See LICENSE.TXT file for license information.
*/
#include<stdio.h>
#include<string.h>
#include"fsairatk.h"
#include"fsglbvar.h"
#include"portablestrings.h"
#include"globals.h"

long NumAircraft = 0;
AirCraft* aircraft[500];

//============================================================================
AirCraft::AirCraft()
{ //AirCraft::AirCraft
  memset( AirCraftName, 0x0, sizeof(AirCraftName) );
  for( long i = 0; i < 6; i++ ) PatternLength[i] = -1;
  Units = 0;  //Meters by default
  NumDrops = 0;
} //AirCraft::AirCraft

//----------------------------------------------------------------------------
//AirAttack Functions
//----------------------------------------------------------------------------

//============================================================================
AirAttack::AirAttack() {}

//============================================================================
AirAttack::~AirAttack() {}

//============================================================================
bool AirAttack::CheckEffectiveness( AirAttackData* atk, double TimeStep )
{ //AirAttack::CheckEffectiveness
  atk->ElapsedTime += TimeStep;
  if( atk->EffectiveDuration > 0.0 &&
      atk->ElapsedTime > atk->EffectiveDuration )
    return false;

  return true;
} //AirAttack::CheckEffectiveness

//----------------------------------------------------------------------------
//Global Air Attack Support Functions
//----------------------------------------------------------------------------

static long NumAirAttacks = 0;
static long AirAttackCounter = 0;
static AirAttackData* FirstAirAttack;
static AirAttackData* NextAirAttack;
static AirAttackData* CurAirAttack;
static AirAttackData* LastAirAttack;
static char AirAttackLog[MAX_CUR_DIR_STR_LEN];

//============================================================================
long GetNumAirCraft() { return NumAircraft; }

//============================================================================
void SetNumAirCraft( long NumCraft ) { NumAircraft = NumCraft; }

//============================================================================
bool LoadAirCraft( char* FileName, bool AppendList )
{ //LoadAirCraft
  FILE*  AirAttackFile;
  char   garbage[256], str[256];
  char   ch[2] = "";
  long   craftnumber, covlev;
  double patlen;

  if( (AirAttackFile = fopen(FileName, "r")) != NULL ) {
    long i, j;
    j = GetNumAirCraft();
    if( ! AppendList ) {
      for( i = 0; i < j; i++ ) FreeAirCraft( i );
    }
    ch[0] = getc( AirAttackFile );
    do {
      memset( garbage, 0x0, sizeof(garbage) );
      fgets( garbage, 255, AirAttackFile );
      if( feof(AirAttackFile) ) break;
      craftnumber = SetNewAirCraft();
      if( craftnumber == -1 ) return false;
      aircraft[craftnumber] = GetAirCraft( craftnumber );
      memset( aircraft[craftnumber]->AirCraftName, 0x0,
              sizeof(aircraft[craftnumber]->AirCraftName) );
      strncpy( aircraft[craftnumber]->AirCraftName, garbage,
               strlen(garbage) - 2 );

      fgets( garbage, 255, AirAttackFile );
      sscanf( garbage, "%s", str );
      if( ! strcmp(strupr(str), "METERS") )
        aircraft[craftnumber]->Units = 0;
      else if( ! strcmp(strupr(str), "FEET") )
        aircraft[craftnumber]->Units = 1;
      else aircraft[craftnumber]->Units = 0;    //Default to meters

      for( i = 0; i < 6; i++ ) {
        fgets( garbage, 255, AirAttackFile );
        sscanf( garbage, "%ld %lf", &covlev, &patlen );
        if( covlev < 99 ) {
          if( covlev > 0 && covlev < 5 )
            aircraft[craftnumber]->PatternLength[covlev - 1] = (long) patlen;
          else if( covlev == 6 )
            aircraft[craftnumber]->PatternLength[4] = (long) patlen;
          else if( covlev == 8 )
            aircraft[craftnumber]->PatternLength[5] = (long) patlen;
        }
        else break;
      }
      ch[0] = getc( AirAttackFile );
      if( ! strcmp(strupr(ch), "R") || !strcmp(strupr(ch), "C") ) {
        fgets( garbage, 255, AirAttackFile );
        sscanf( garbage, "%s %lf", str, &patlen );
        if( ! strcmp(strupr(str), "ETURN_TIME") )
          aircraft[craftnumber]->ReturnTime = patlen;
        else if( ! strcmp(strupr(str), "OST") )
          aircraft[craftnumber]->Cost = patlen;
        ch[0] = getc(AirAttackFile);
        if( ! strcmp(strupr(ch), "R") || !strcmp(strupr(ch), "C") ) {
          fgets( garbage, 255, AirAttackFile );
          sscanf( garbage, "%s %lf", str, &patlen );
          if( ! strcmp(strupr(str), "ETURN_TIME") )
            aircraft[craftnumber]->ReturnTime = patlen;
          else if( ! strcmp(strupr(str), "OST") )
            aircraft[craftnumber]->Cost = patlen;
          ch[0] = getc( AirAttackFile );
        }
      }
      else {    //Set return time and cost to unknown values
        aircraft[craftnumber]->ReturnTime = 60.0;
        aircraft[craftnumber]->Cost = 1000.0;
      }
    } while( ! feof(AirAttackFile) );
    fclose( AirAttackFile );

    return true;
  }

  return false;
} //LoadAirCraft

//============================================================================
long SetNewAirCraft()
{ //SetNewAirCraft
  if( (aircraft[NumAircraft] = (AirCraft *) calloc(1, sizeof(AirCraft))) !=
      NULL )
    return NumAircraft++;

  return -1;
} //SetNewAirCraft

//============================================================================
AirCraft* GetAirCraft( long AirCraftNumber )
{ //GetAirCraft
  if( aircraft[AirCraftNumber] ) return aircraft[AirCraftNumber];

  return 0;
} //GetAirCraft

//============================================================================
long SetAirCraft( long AirCraftNumber )
{ //SetAirCraft
  if( (aircraft[AirCraftNumber] = (AirCraft *) calloc(1, sizeof(AirCraft))) !=
      NULL )
    return NumAircraft++;

  return -1;
} //SetAirCraft

//============================================================================
void FreeAirCraft(long AirCraftNumber)
{ //FreeAirCraft
  if( aircraft[AirCraftNumber] ) {
    free( aircraft[AirCraftNumber] );
    NumAircraft--;
    if( NumAircraft < 0 ) NumAircraft = 0;
  }

  aircraft[AirCraftNumber] = 0;
} //FreeAirCraft

//============================================================================
long GetNumAirAttacks() { return NumAirAttacks; }

//============================================================================
void SetNumAirAttacks( long NumAtk ) { NumAirAttacks = NumAtk; }

//============================================================================
long SetupAirAttack( long AirCraftNumber, long CoverageLevel, long Duration,
                     double* startpoint )
{ //SetupAirAttack
  long i;

  for(  i = 0; i <= NumAirAttacks; i++ ) {
    if( NumAirAttacks == 0 ) {
      if( (FirstAirAttack =
          (AirAttackData *) calloc(1, sizeof(AirAttackData))) != NULL ) {
        CurAirAttack = FirstAirAttack;
        if( AirAttackCounter == 0 ) {
          GetCurDir( AirAttackLog, MAX_CUR_DIR_STR_LEN );
          //Should check for overflow here (below).
          strcat( AirAttackLog, "airattk.log" );
          remove( AirAttackLog );
        }
      }
      else return 0;
    }
    else if( i == 0 ) CurAirAttack = FirstAirAttack;
    else CurAirAttack = NextAirAttack;
    if( i < NumAirAttacks )
      NextAirAttack = (AirAttackData *) CurAirAttack->next;
  }

  if( (NextAirAttack = (AirAttackData *) calloc(1, sizeof(AirAttackData))) !=
      NULL ) {
    NumAirAttacks++;
    CurAirAttack->next = (AirAttackData *) NextAirAttack;
    CurAirAttack->AirAttackNumber = ++AirAttackCounter;
    CurAirAttack->EffectiveDuration = Duration;
    CurAirAttack->CoverageLevel = CoverageLevel;
    CurAirAttack->AirCraftNumber = AirCraftNumber;
    CurAirAttack->ElapsedTime = 0.0;

    if( startpoint ) {
      double InitDist, MinDist, Mult = 1.0, Ratio;
      double NewEndPtX, NewEndPtY, XDist, YDist;
      double xpt, ypt, xmax, ymax, xmin, ymin;
      double DistRes, PerimRes, PtX, PtY;

      DistRes = GetDistRes() / 1.4 * MetricResolutionConvert();
      PerimRes = GetDistRes() / 2.0 * MetricResolutionConvert();
      if( aircraft[AirCraftNumber]->Units == 1 )
        Mult = 0.3048;  //Feet to meters
      XDist = startpoint[0] - startpoint[2];
      YDist = startpoint[1] - startpoint[3];
      InitDist = sqrt( pow2(XDist) + pow2(YDist) );
      MinDist = aircraft[AirCraftNumber]->PatternLength[CoverageLevel] *
                Mult * MetricResolutionConvert();
      Ratio = InitDist / MinDist;
      //20101001:JWB: I think the coords are supposed to be *relative*,
      //              so subtracting edge values here.
      double x_start_rel = startpoint[0] - GetWestUtm() + GetLoEast();
      double y_start_rel = startpoint[1] - GetSouthUtm() + GetLoNorth();
      NewEndPtX = x_start_rel - (XDist) / Ratio;
      NewEndPtY = y_start_rel - (YDist) / Ratio;

      Ratio = InitDist / ( MinDist + PerimRes );
      PtX = x_start_rel - (XDist) / Ratio;
      PtY = y_start_rel - (YDist) / Ratio;

      XDist = x_start_rel - NewEndPtX;
      YDist = y_start_rel - NewEndPtY;

      CurAirAttack->PatternNumber = GetNewFires();
      AllocPerimeter1( CurAirAttack->PatternNumber, 6 );
      SetInout( CurAirAttack->PatternNumber, 3 );
      SetNumPoints( CurAirAttack->PatternNumber, 5 );

      xpt = xmin = xmax = x_start_rel + DistRes / MinDist * YDist;
      ypt = ymin = ymax = y_start_rel - DistRes / MinDist * XDist;
      SetPerimeter1( GetNewFires(), 0, xpt, ypt );
      SetFireChx( GetNewFires(), 0, 0, -1 );

      xpt = x_start_rel - DistRes / MinDist * YDist;
      ypt = y_start_rel + DistRes / MinDist * XDist;
      if( xpt < xmin ) xmin = xpt;
      if( ypt < ymin ) ymin = ypt;
      if( xpt > xmax ) xmax = xpt;
      if( ypt > ymax ) ymax = ypt;
      SetPerimeter1( GetNewFires(), 1, xpt, ypt );
      SetFireChx( GetNewFires(), 1, 0, -1 );

      xpt = NewEndPtX - DistRes / MinDist * YDist;
      ypt = NewEndPtY + DistRes / MinDist * XDist;
      if( xpt < xmin ) xmin = xpt;
      if( ypt < ymin ) ymin = ypt;
      if( xpt > xmax ) xmax = xpt;
      if( ypt > ymax ) ymax = ypt;
      SetPerimeter1( GetNewFires(), 2, xpt, ypt );
      SetFireChx( GetNewFires(), 2, 0, -1 );

      xpt = PtX;
      ypt = PtY;
      if( xpt < xmin ) xmin = xpt;
      if( ypt < ymin ) ymin = ypt;
      if( xpt > xmax ) xmax = xpt;
      if( ypt > ymax ) ymax = ypt;
      SetPerimeter1( GetNewFires(), 3, xpt, ypt );
      SetFireChx( GetNewFires(), 3, 0, -1 );

      xpt = NewEndPtX + DistRes / MinDist * YDist;
      ypt = NewEndPtY - DistRes / MinDist * XDist;
      if( xpt < xmin ) xmin = xpt;
      if( ypt < ymin ) ymin = ypt;
      if( xpt > xmax ) xmax = xpt;
      if( ypt > ymax ) ymax = ypt;
      SetPerimeter1( GetNewFires(), 4, xpt, ypt );
      SetFireChx( GetNewFires(), 4, 0, -1 );

      SetPerimeter1( GetNewFires(), 5, xmin, xmax );
      SetFireChx( GetNewFires(), 5, ymin, ymax );
      IncNewFires( 1 );
      IncNumFires( 1 );
    }
  }
  else return 0;

  return AirAttackCounter;
} //SetupAirAttack

//============================================================================
void CancelAirAttack( long AirAttackCounter )
{ //CancelAirAttack
  for( long i = 0; i < NumAirAttacks; i++ ) {
    if( i == 0 ) {
      CurAirAttack = FirstAirAttack;
      NextAirAttack = (AirAttackData *) CurAirAttack->next;
    }
    if( CurAirAttack->AirAttackNumber == AirAttackCounter ) {
      if( i == 0 ) FirstAirAttack = (AirAttackData *) CurAirAttack->next;
      else LastAirAttack->next = (AirAttackData *) NextAirAttack;
      WriteAirAttackLog( CurAirAttack, AirAttackLog );
      free( CurAirAttack );
      NumAirAttacks--;
      if( NumAirAttacks == 0 ) free( NextAirAttack );
      break;
    }
    else {
      LastAirAttack = CurAirAttack;
      CurAirAttack = NextAirAttack;
      NextAirAttack = (AirAttackData *) CurAirAttack->next;
    }
  }
} //CancelAirAttack

/*============================================================================
  LoadAirAttack
  Function only for loading air attacks from bookmark.
*/
void LoadAirAttack( AirAttackData airattackdata )
{ //LoadAirAttack
  if( NumAirAttacks == 0 ) {
    FirstAirAttack = (AirAttackData *) calloc( 1, sizeof(AirAttackData) );
    CurAirAttack = FirstAirAttack;
    memcpy( FirstAirAttack, &airattackdata, sizeof(AirAttackData) );
  }
  memcpy( CurAirAttack, &airattackdata, sizeof(AirAttackData) );
  NextAirAttack = (AirAttackData *) calloc( 1, sizeof(AirAttackData) );
  CurAirAttack->next = (AirAttackData *) NextAirAttack;
  if( NumAirAttacks == 0 )
    FirstAirAttack->next = (AirAttackData *) NextAirAttack;
  NumAirAttacks++;
  CurAirAttack = NextAirAttack;
} //LoadAirAttack

//============================================================================
void FreeAllAirAttacks()
{ //FreeAllAirAttacks
  for( long i = 0; i < NumAirAttacks; i++ ) {
    if( i == 0 ) {
      CurAirAttack = FirstAirAttack;
      NextAirAttack = (AirAttackData *) CurAirAttack->next;
    }
    WriteAirAttackLog( CurAirAttack, AirAttackLog );
    free( CurAirAttack );
    CurAirAttack = NextAirAttack;
    NextAirAttack = (AirAttackData *) CurAirAttack->next;
  }
  if( NumAirAttacks > 0 ) {
    free( CurAirAttack );
    NumAirAttacks = 0;
  }
  AirAttackCounter = 0;
} //FreeAllAirAttacks

//============================================================================
AirAttackData* GetAirAttack( long AirAttackCounter )
{ //GetAirAttack
  for( long i = 0; i < NumAirAttacks; i++ ) {
    if( i == 0 ) {
      CurAirAttack = FirstAirAttack;
      NextAirAttack = (AirAttackData *) CurAirAttack->next;
    }
    if( CurAirAttack->AirAttackNumber == AirAttackCounter )
      return CurAirAttack;
    else {
      CurAirAttack = NextAirAttack;
      NextAirAttack = (AirAttackData *) CurAirAttack->next;
    }
  }

  return NULL;
} //GetAirAttack

/*============================================================================
  GetAirAttackByOrder
  Retrieves indirect attack in order.
*/
AirAttackData* GetAirAttackByOrder( long OrdinalAttackNum )
{ //GetAirAttackByOrder
  for( long i = 0; i < NumAirAttacks; i++ ) {
    if( i == 0 ) CurAirAttack = FirstAirAttack;
    else CurAirAttack = NextAirAttack;
    NextAirAttack = (AirAttackData *) CurAirAttack->next;
    if( i == OrdinalAttackNum ) return CurAirAttack;
  }

  return 0;
} //GetAirAttackByOrder

//============================================================================
void SetNewFireNumberForAirAttack( long oldnumfire, long newnumfire )
{ //SetNewFireNumberForAirAttack
  for( long i = 0; i < NumAirAttacks; i++ ) {
    if( i == 0 ) {
      CurAirAttack = FirstAirAttack;
      NextAirAttack = (AirAttackData *) CurAirAttack->next;
    }
    if( CurAirAttack->PatternNumber == oldnumfire ) {
      CurAirAttack->PatternNumber = newnumfire;

      break;
    }
    else {
      CurAirAttack = NextAirAttack;
      NextAirAttack = (AirAttackData *) CurAirAttack->next;
    }
  }
} //SetNewFireNumberForAirAttack

//============================================================================
void WriteAirAttackLog( AirAttackData* atk, char* AirAttackLog )
{ //WriteAirAttackLog
  FILE* airatklog;
  char  units[24];
  long  covlevel;

  if( (airatklog = fopen(AirAttackLog, "a")) != NULL ) {
    if( aircraft[atk->AirCraftNumber]->Units == 0 ) sprintf(units, "meters");
    else sprintf( units, "feet" );

    if( atk->CoverageLevel == 4 ) covlevel = 6;
    else if( atk->CoverageLevel == 5 ) covlevel = 8;
    else covlevel = atk->CoverageLevel + 1;

    if( atk->EffectiveDuration > 0.0 )
      fprintf( airatklog, "%s, %s %ld, %s %lf %s, %s %ld %s %ld drops\n",
               aircraft[atk->AirCraftNumber]->AirCraftName,
               "Coverage Level:", covlevel, "Line Length:",
            aircraft[atk->AirCraftNumber]->PatternLength[atk->CoverageLevel-1],
               units, "Duration:", (long) atk->EffectiveDuration, "mins",
               aircraft[atk->AirCraftNumber]->NumDrops );
    else
      fprintf( airatklog, "%s, %s %ld, %s %lf %s, %s %ld drops\n",
               aircraft[atk->AirCraftNumber]->AirCraftName, "Coverage Level:",
               covlevel, "Line Length:",
            aircraft[atk->AirCraftNumber]->PatternLength[atk->CoverageLevel-1],
               units, "Duration: Unlimited",
               aircraft[atk->AirCraftNumber]->NumDrops );

    fclose( airatklog );
  }
} //WriteAirAttackLog

//----------------------------------------------------------------------------
//Group Air Attack Support Functions
//----------------------------------------------------------------------------

static long NumGroupAttacks = 0;
static long GroupAttackCounter = 0;
static GroupAttackData* FirstGroupAttack;
static GroupAttackData* NextGroupAttack;
static GroupAttackData* CurGroupAttack;
static GroupAttackData* LastGroupAttack;
static GroupAttackData* ReAtk;

//============================================================================
long SetupGroupAirAttack( long AircraftNumber, long CoverageLevel,
                          long Duration, double* line, long FireNum,
                          char* GroupName )
{ //SetupGroupAirAttack
  CallLevel++;
  if( Verbose > CallLevel )
    printf( "%*sfsairatk:GroupAirAttack::SetupGroupAirAttack:1\n",
            CallLevel, "" );

  long i;
  for( i = 0; i <= NumGroupAttacks; i++ ) {
    if( NumGroupAttacks == 0 ) {
      if( (FirstGroupAttack = (GroupAttackData *)
          calloc(1, sizeof(GroupAttackData))) != NULL )
        CurGroupAttack = FirstGroupAttack;
      else {
        if( Verbose > CallLevel )
          printf( "%*sfsairatk:GroupAirAttack::SetupGroupAirAttack:1a\n",
                  CallLevel, "" );
        CallLevel--;
        return 0;
      }
    }
    else if( i == 0 ) CurGroupAttack = FirstGroupAttack;
    else CurGroupAttack = NextGroupAttack;
    if( i < NumGroupAttacks )
      NextGroupAttack = (GroupAttackData *) CurGroupAttack->next;
  }

  if( (NextGroupAttack = (GroupAttackData *)
      calloc(1, sizeof(GroupAttackData))) != NULL ) {
    NumGroupAttacks++;
    CurGroupAttack->next = (GroupAttackData *) NextGroupAttack;
    CurGroupAttack->GroupAttackNumber = ++GroupAttackCounter;
    strcpy( CurGroupAttack->GroupName, GroupName );
    CurGroupAttack->Suspended = 0;
    GroupAirAttack *gp = new GroupAirAttack( CurGroupAttack );
    gp->AllocGroup( GROUPSIZE );
    gp->AddGroupMember( AircraftNumber, CoverageLevel, Duration );
    gp->SetGroupAssignment( line, FireNum, false );
    delete gp;
  }

  if( Verbose > CallLevel )
    printf( "%*sfsairatk:GroupAirAttack::SetupGroupAirAttack:2\n",
            CallLevel, "" );
  CallLevel--;

  return GroupAttackCounter;
} //SetupGroupAirAttack

//============================================================================
GroupAttackData* ReassignGroupAirAttack( GroupAttackData* atk )
{ //ReassignGroupAirAttack
  if( atk != 0 ) ReAtk = atk;

  return ReAtk;
} //ReassignGroupAirAttack

//============================================================================
void CancelGroupAirAttack( long GroupCounter )
{ //CancelGroupAirAttack
  long i;

  for( i = 0; i < NumGroupAttacks; i++ ) {
    if( i == 0 ) {
      CurGroupAttack = FirstGroupAttack;
      NextGroupAttack = (GroupAttackData *) CurGroupAttack->next;
    }
    if( CurGroupAttack->GroupAttackNumber == GroupCounter ) {
      if( i == 0 )
        FirstGroupAttack = (GroupAttackData *) CurGroupAttack->next;
      else
        LastGroupAttack->next = (GroupAttackData *) NextGroupAttack;
      if( CurGroupAttack->IndirectLine )
        free( CurGroupAttack->IndirectLine );
      free( CurGroupAttack->WaitTime );
      free( CurGroupAttack->CoverageLevel );
      free( CurGroupAttack->EffectiveDuration );
      free( CurGroupAttack->AircraftNumber );
      free( CurGroupAttack );
      NumGroupAttacks--;
      if( NumGroupAttacks == 0 ) free( NextGroupAttack );
      break;
    }
    else {
      LastGroupAttack = CurGroupAttack;
      CurGroupAttack = NextGroupAttack;
      NextGroupAttack = (GroupAttackData *) CurGroupAttack->next;
    }
  }
} //CancelGroupAirAttack

//============================================================================
void LoadGroupAttack( GroupAttackData gatk )
{ //LoadGroupAttack
  if( gatk.NumCurrentAircraft <= 0 ) return;

  long i;
  for( i = 0; i <= NumGroupAttacks; i++ ) {
    if( NumGroupAttacks == 0 ) {
      if( (FirstGroupAttack = (GroupAttackData *)
          calloc(1, sizeof(GroupAttackData))) != NULL )
        CurGroupAttack = FirstGroupAttack;
      else return;
    }
    else if( i == 0 ) CurGroupAttack = FirstGroupAttack;
    else CurGroupAttack = NextGroupAttack;
    if( i < NumGroupAttacks )
      NextGroupAttack = (GroupAttackData *) CurGroupAttack->next;
  }

  if( (NextGroupAttack = (GroupAttackData *)
      calloc(1, sizeof(GroupAttackData))) != NULL ) {
    NumGroupAttacks++;
    CurGroupAttack->next = (GroupAttackData *) NextGroupAttack;
    CurGroupAttack->GroupAttackNumber = ++GroupAttackCounter;
    strcpy( CurGroupAttack->GroupName, gatk.GroupName );
    CurGroupAttack->Suspended = 0;
    if( gatk.NumPoints < 0 ) {
      CurGroupAttack->IndirectLine = (double *) calloc( 4, sizeof(double) );
      memcpy( CurGroupAttack->IndirectLine, gatk.IndirectLine,
              4 * sizeof(double) );
    }
    else {
      CurGroupAttack->IndirectLine =
                      (double *) calloc( gatk.NumPoints * 2, sizeof(double) );
      memcpy( CurGroupAttack->IndirectLine, gatk.IndirectLine,
                                        gatk.NumPoints * 2 * sizeof(double) );
    }
    CurGroupAttack->CoverageLevel =
                        (long *) calloc(gatk.NumTotalAircraft, sizeof(long) );
    CurGroupAttack->EffectiveDuration =
                       (long *) calloc( gatk.NumTotalAircraft, sizeof(long) );
    CurGroupAttack->AircraftNumber =
                       (long *) calloc( gatk.NumTotalAircraft, sizeof(long) );
    CurGroupAttack->WaitTime =
                   (double *) calloc( gatk.NumTotalAircraft, sizeof(double) );
    memcpy( CurGroupAttack->CoverageLevel, gatk.CoverageLevel,
                                     gatk.NumCurrentAircraft * sizeof(long) );
    memcpy( CurGroupAttack->EffectiveDuration, gatk.EffectiveDuration,
                                     gatk.NumCurrentAircraft * sizeof(long) );
    memcpy( CurGroupAttack->AircraftNumber, gatk.AircraftNumber,
                                     gatk.NumCurrentAircraft * sizeof(long) );
    memcpy( CurGroupAttack->WaitTime, gatk.WaitTime,
                                   gatk.NumCurrentAircraft * sizeof(double) );
    CurGroupAttack->NumTotalAircraft = gatk.NumTotalAircraft;
    CurGroupAttack->NumCurrentAircraft = gatk.NumCurrentAircraft;
    CurGroupAttack->Suspended = gatk.Suspended;
  memcpy( CurGroupAttack->GroupName, gatk.GroupName, 256 * sizeof(char) );
  CurGroupAttack->NumPoints = gatk.NumPoints;
  CurGroupAttack->FireNumber = gatk.FireNumber;
  }
} //LoadGroupAttack

//============================================================================
void FreeAllGroupAirAttacks()
{ //FreeAllGroupAirAttacks
  long i;

  for( i = 0; i < NumGroupAttacks; i++ ) {
    if( i == 0 ) {
      CurGroupAttack = FirstGroupAttack;
      NextGroupAttack = (GroupAttackData *) CurGroupAttack->next;
    }
    if( CurGroupAttack->IndirectLine )
      free( CurGroupAttack->IndirectLine );
    free( CurGroupAttack->CoverageLevel );
    free( CurGroupAttack->EffectiveDuration );
    free( CurGroupAttack->AircraftNumber );
    free( CurGroupAttack->WaitTime );
    free( CurGroupAttack );
    CurGroupAttack = NextGroupAttack;
    NextGroupAttack = (GroupAttackData *) CurGroupAttack->next;
  }
  if( NumGroupAttacks > 0 ) free( CurGroupAttack );

  NumGroupAttacks = 0;
  GroupAttackCounter = 0;
} //FreeAllGroupAirAttacks()

//============================================================================
GroupAttackData* GetGroupAirAttack( long GroupAttackCounter )
{ //GetGroupAirAttack
  for( long i = 0; i < NumGroupAttacks; i++ ) {
    if( i == 0 ) {
      CurGroupAttack = FirstGroupAttack;
      NextGroupAttack = (GroupAttackData *) CurGroupAttack->next;
    }
    if( CurGroupAttack->GroupAttackNumber == GroupAttackCounter )
      return CurGroupAttack;
    else {
      CurGroupAttack = NextGroupAttack;
      NextGroupAttack = (GroupAttackData *) CurGroupAttack->next;
    }
  }

  return NULL;
} //GetGroupAirAttack

//============================================================================
GroupAttackData* GetGroupAttackByOrder( long OrdinalAttackNumber )
{ //GetGroupAttackByOrder
  for( long i = 0; i < NumGroupAttacks; i++ ) {
    if( i == 0 ) CurGroupAttack = FirstGroupAttack;
    else CurGroupAttack = NextGroupAttack;
    NextGroupAttack = (GroupAttackData *) CurGroupAttack->next;
    if( i == OrdinalAttackNumber ) return CurGroupAttack;
  }

  return 0;
} //GetGroupAttackByOrder

//============================================================================
GroupAttackData* GetGroupAttackForFireNumber( long NumFire,
                                              long StartAttackNum,
                                              long* LastAttackNumber )
{ //GetGroupAttackForFireNumber
  for( long i = 0; i < NumGroupAttacks; i++ ) {
    if( i == 0 ) CurGroupAttack = FirstGroupAttack;
    NextGroupAttack = (GroupAttackData *) CurGroupAttack->next;
    if( i >= StartAttackNum ) {
      if( CurGroupAttack->FireNumber == NumFire) {
        *LastAttackNumber = i;

        return CurGroupAttack;
      }
    }
    CurGroupAttack = NextGroupAttack;
  }

  return 0;
} //GetGroupAttackForFireNumber

//============================================================================
void SetNewFireNumberForGroupAttack( long OldFireNum, long NewFireNum )
{ //SetNewFireNumberForGroupAttack
  long ThisGroup, NextGroup;

  ThisGroup = 0;
  while( GetGroupAttackForFireNumber(OldFireNum, ThisGroup, &NextGroup) ) {
    ThisGroup = NextGroup + 1;
    CurGroupAttack->FireNumber = NewFireNum;
  }
} //SetNewFireNumberForGroupAttack

//============================================================================
long GetNumGroupAttacks() { return NumGroupAttacks; }

//============================================================================
void SetNumGroupAttacks( long NumGroups ) { NumGroupAttacks = NumGroups; }

//----------------------------------------------------------------------------

//============================================================================
GroupAirAttack::GroupAirAttack() {}

//============================================================================
GroupAirAttack::GroupAirAttack( GroupAttackData* atk ) { attack = atk; }

//============================================================================
GroupAirAttack::~GroupAirAttack() {}

//============================================================================
void GroupAirAttack::SetGroup( GroupAttackData* gatk ) { attack = gatk; }

//============================================================================
void GroupAirAttack::GetCurrentGroup() { attack = CurGroupAttack; }

//============================================================================
void GroupAirAttack::ExecuteAllIndirectAttacks( double TimeIncrement )
{ //GroupAirAttack::ExecuteAllIndirectAttacks
  CallLevel++; 
  if( Verbose > CallLevel )
    printf( "%*s===========================================================\n"
            "%*sfsairatk:GroupAirAttack::ExecuteAllIndirectAttacks:1 "
            "TimeIncrement=%lf\n",
            CallLevel, "", CallLevel, "", TimeIncrement );

  double OriginalTimeInc = TimeIncrement;

  long   i;
  for( i = 0; i < NumGroupAttacks; i++ ) {
    attack = GetGroupAttackByOrder( i );
    if( CheckSuspendState(GETVAL) ) continue;
    if( attack->FireNumber < 0 ) {    //If indirect attack
      if( Verbose > CallLevel )
        printf( "%*s-------------------------------------------------------\n"
                "%*sfsairatk:GroupAirAttack::ExecuteAllIndirectAttacks:1a "
                "i=%ld\n",
                CallLevel, "", CallLevel, "", i );
      do {    //Find smallest attack time
        double NextTime = GetNextAttackTime( TimeIncrement );
        if( ! ExecuteAttacks(NextTime) ) {
          if( Verbose > CallLevel )
            printf( "%*sfsairatk:GroupAirAttack::ExecuteAllIndirectAttacks:"
                    "1a1a CANCELLING Attack\n",
                    CallLevel, "" );
          CancelGroupAirAttack( attack->GroupAttackNumber );
          i--;    //Go back one because NumGroupAttacks has been decremented
          break;
        }
        if( Verbose > CallLevel )
          printf( "%*sfsairatk:GroupAirAttack::ExecuteAllIndirectAttacks:"
                  "1a2 \n",
                  CallLevel, "" );

        IncrementWaitTimes( NextTime );
        TimeIncrement -= NextTime;
      } while( TimeIncrement > 0.0 );
      TimeIncrement = OriginalTimeInc;
    }
  }

  if( Verbose > CallLevel )
    printf( "%*sfsairatk:GroupAirAttack::ExecuteAllIndirectAttacks:2\n"
            "%*s===========================================================\n",
            CallLevel, "", CallLevel, "" );
  CallLevel--; 
} //GroupAirAttack::ExecuteAllIndirectAttacks

//============================================================================
bool GroupAirAttack::AllocGroup( long NewNumber )
{ //GroupAirAttack::AllocGroup
  if( (attack->CoverageLevel =
      (long *) calloc(NewNumber, sizeof(long))) == NULL )
    return false;
  if( (attack->EffectiveDuration =
      (long *) calloc(NewNumber, sizeof(long))) == NULL )
    return false;
  if( (attack->AircraftNumber =
      (long *) calloc(NewNumber, sizeof(long))) == NULL )
    return false;
  if( (attack->WaitTime =
      (double *) calloc(NewNumber, sizeof(double))) == NULL )
    return false;
  attack->NumCurrentAircraft = 0;
  attack->NumTotalAircraft = NewNumber;

  return true;
} //GroupAirAttack::AllocGroup

//============================================================================
bool GroupAirAttack::AddGroupMember( long AircraftNumber, long CoverageLevel,
                                     long EffectiveDuration )
{ //GroupAirAttack::AddGroupMember
  long    numcur, * tempcov, * tempnum, * tempdur;
  double* tempelapsed;

  numcur = attack->NumCurrentAircraft;
  if( numcur == attack->NumTotalAircraft ) {
    if( numcur > 0 ) {
      if( (tempcov = (long *) calloc(numcur, sizeof(long))) == NULL )
        return false;
      memcpy( tempcov, attack->CoverageLevel, numcur * sizeof(long) );
      if( (tempnum = (long *) calloc(numcur, sizeof(long))) == NULL )
        return false;
      memcpy( tempnum, attack->AircraftNumber, numcur * sizeof(long) );
      if( (tempdur = (long *) calloc(numcur, sizeof(long))) == NULL )
        return false;
      memcpy( tempdur, attack->EffectiveDuration, sizeof(long) );
      if( (tempelapsed = (double *) calloc(numcur, sizeof(double))) == NULL )
        return false;
      memcpy( tempelapsed, attack->WaitTime, numcur * sizeof(double) );
      free( attack->CoverageLevel );
      free( attack->EffectiveDuration );
      free( attack->AircraftNumber );
      free( attack->WaitTime );
    }
    attack->NumTotalAircraft += GROUPSIZE;
    if( ! AllocGroup(attack->NumTotalAircraft) ) return false;
    if( numcur > 0 ) {
      memcpy( attack->EffectiveDuration, tempdur, numcur * sizeof(long) );
      memcpy( attack->AircraftNumber, tempnum, numcur * sizeof(long) );
      memcpy( attack->CoverageLevel, tempcov, numcur * sizeof(long) );
      memcpy( attack->WaitTime, tempelapsed, numcur * sizeof(double) );
      free( tempdur );
      free( tempcov );
      free( tempnum );
      free( tempelapsed );
    }
  }
  attack->EffectiveDuration[numcur] = EffectiveDuration;
  attack->AircraftNumber[numcur] = AircraftNumber;
  attack->CoverageLevel[numcur] = CoverageLevel;
  attack->WaitTime[numcur] = 0.0;
  attack->NumCurrentAircraft++;

  return true;
} //GroupAirAttack::AddGroupMember

//============================================================================
bool GroupAirAttack::SetGroupAssignment( double* line, long FireNum,
                                         bool Reset )
{ //GroupAirAttack::SetGroupAssignment
  long i;
  bool OK = true;

  if( Reset ) {
    if( attack->IndirectLine ) free(attack->IndirectLine);
    attack->IndirectLine = 0;
  }

  if( FireNum < 0 ) {    //Negative flag indicating indirect line
    FireNum *= -1;
    if( (attack->IndirectLine =
                   (double *) calloc(FireNum * 2, sizeof(double))) == NULL ) {
      OK = false;

      return OK;
    }
    attack->NumPoints = FireNum;
    for( i = 0; i < FireNum; i++ ) {
      attack->IndirectLine[i * 2] = line[i * 2];
      attack->IndirectLine[i * 2 + 1] = line[i * 2 + 1];
    }
    attack->FireNumber = -1;
  }
  else {  //Positive # indicating actual fire #, line indicates 1st pt
    long   pt;
    double mindist, dist, x, y;

    if( (attack->IndirectLine = (double *) calloc(4, sizeof(double))) == 
        NULL ) {
      OK = false;

      return OK;
    }
    attack->FireNumber = FireNum;
    //attack->IndirectLine[2]=line[0];  // put in secondary positions
    //attack->IndirectLine[3]=line[1];  // to be transfered in Execute()

    pt = 0;
    for( i = 0; i < GetNumPoints(FireNum); i++ ) {
      x = GetPerimeter1Value( FireNum, i, XCOORD );
      y = GetPerimeter1Value( FireNum, i, YCOORD );
      dist = pow2( x - line[0] ) + pow2( y - line[1] );
      if( i == 0 ) {
        mindist = dist;
        continue;
      }
      if( dist < mindist ) {
        mindist = dist;
        pt = i;
      }
    }
    x = GetPerimeter1Value( FireNum, pt, XCOORD );
    y = GetPerimeter1Value( FireNum, pt, YCOORD );
    attack->IndirectLine[2] = x;  //Put in secondary positions
    attack->IndirectLine[3] = y;  //to be transfered in Execute()
    pt -= 1;
    if( pt < 0 ) pt += GetNumPoints( FireNum );
    x = GetPerimeter1Value( FireNum, pt, XCOORD );
    y = GetPerimeter1Value( FireNum, pt, YCOORD );
    dist = pow2( x - line[2] ) + pow2( y - line[3] );
    pt += 2;
    if( pt > GetNumPoints(FireNum) - 1 ) pt -= GetNumPoints( FireNum );
    x = GetPerimeter1Value( FireNum, pt, XCOORD );
    y = GetPerimeter1Value( FireNum, pt, YCOORD );
    mindist = pow2( x - line[2] ) + pow2( y - line[3] );
    if( mindist <= dist ) attack->Direction = 0;
    else attack->Direction = 1;
    attack->NumPoints = -1;
  }

  return OK;
} //GroupAirAttack::SetGroupAssignment

//============================================================================
bool GroupAirAttack::RemoveGroupMember( long membernumber )
{ //GroupAirAttack::RemoveGroupMember
  if( membernumber < attack->NumCurrentAircraft ) {
    attack->AircraftNumber[membernumber] = -1;
    ReorderGroupList( membernumber );
  }
  else return false;

  return true;
} //GroupAirAttack::RemoveGroupMember

//============================================================================
void GroupAirAttack::ReorderGroupList( long Start )
{ //GroupAirAttack::ReorderGroupList
  long i;

  if( Start < 0 ) {
    for( i = 0; i < attack->NumCurrentAircraft; i++ ) {
      if( attack->AircraftNumber[i] < 0 ) {
        Start = i;
        break;
      }
    }
  }
  for( i = Start; i < attack->NumCurrentAircraft - 1; i++ ) {
    attack->CoverageLevel[i] = attack->CoverageLevel[i + 1];
    attack->EffectiveDuration[i] = attack->EffectiveDuration[i + 1];
    attack->AircraftNumber[i] = attack->AircraftNumber[i + 1];
    attack->WaitTime[i] = attack->WaitTime[i + 1];
  }
  attack->NumCurrentAircraft--;
} //GroupAirAttack::ReorderGroupList

//============================================================================
bool GroupAirAttack::ExecuteAttacks( double TimeIncrement )
{ //GroupAirAttack::ExecuteAttacks
  CallLevel++;
  if( Verbose > CallLevel )
    printf( "%*sfsairatk:GroupAirAttack::ExecuteAttacks:1 "
            "attack->NumCurrentAircraft=%ld TimeIncrement=%lf\n",
            CallLevel, "", attack->NumCurrentAircraft, TimeIncrement );

  long   i, j;
  double AttackLine[4];
  long   CurPt, NextPt;
  double InitDist, MinDist, TestDist, Mult = 1.0, Ratio;
  double DistRes, NewEndPtX, NewEndPtY, XDist, YDist;

  for( i = 0; i < attack->NumCurrentAircraft; i++ ) {
    if( Verbose > CallLevel )
      printf( "%*sfsairatk:GroupAirAttack::ExecuteAttacks:1a %s "
              "wait time=%lf\n",
              CallLevel, "",
              aircraft[attack->AircraftNumber[i]]->AirCraftName,
              attack->WaitTime[i] );
    if( fabs(attack->WaitTime[i]) < 1e-9 ) {  //Prosecute attack
      if( Verbose > CallLevel )
        printf( "%*sfsairatk:GroupAirAttack::ExecuteAttacks:1a1 %s\n",
                CallLevel, "",
                aircraft[attack->AircraftNumber[i]]->AirCraftName );

      if( aircraft[attack->AircraftNumber[i]]->Units == FEET )
        Mult = 0.3048;    //Feet to meters

      XDist = YDist = 0.0;
      MinDist = aircraft[attack->AircraftNumber[i]]->
                PatternLength[attack->CoverageLevel[i]] * Mult *
                MetricResolutionConvert();
      if( attack->FireNumber < 0 ) {    //Indirect attack
        if( Verbose > CallLevel )
          printf( "%*sfsairatk:GroupAirAttack::ExecuteAttacks:1a1a "
                  "MinDist=%lf\n",
                  CallLevel, "", MinDist );
        CurPt = NextPt = ( attack->FireNumber + 1 ) * -1;
        if( CurPt == attack->NumPoints ) {
          if( Verbose > CallLevel )
            printf( "%*sfsairatk:GroupAirAttack::ExecuteAttacks:1a1a1\n",
                    CallLevel, "" );
          CallLevel--;
          return false;  //Nothing to do
        }
        for( j = CurPt + 1; j < attack->NumPoints; j++ ) {
          XDist += attack->IndirectLine[CurPt * 2] -
                   attack->IndirectLine[j * 2];
          YDist += attack->IndirectLine[CurPt * 2 + 1] -
                   attack->IndirectLine[j * 2 + 1];
          InitDist = sqrt( pow2(XDist) + pow2(YDist) );
          NextPt = j - 1;    //Sub one
          if( InitDist >= MinDist ) break;
        }
        if( Verbose > CallLevel )
          printf( "%*sfsairatk:GroupAirAttack::ExecuteAttacks:1a1b "
                  "InitDist=%lf\n",
                  CallLevel, "", InitDist );

        if( InitDist < MinDist ) {
          if( Verbose > CallLevel )
            printf( "%*sfsairatk:GroupAirAttack::ExecuteAttacks:1a1b1\n",
                    CallLevel, "" );
          CallLevel--;
          return false;
        }
        AttackLine[0] = attack->IndirectLine[CurPt * 2];
        AttackLine[1] = attack->IndirectLine[CurPt * 2 + 1];

        Ratio = InitDist / MinDist;
        NewEndPtX = attack->IndirectLine[CurPt * 2] - (XDist) / Ratio;
        NewEndPtY = attack->IndirectLine[CurPt * 2 + 1] - (YDist) / Ratio;
        attack->IndirectLine[NextPt * 2] = NewEndPtX; //Set up for next attack
        attack->IndirectLine[NextPt * 2 + 1] = NewEndPtY;
        attack->FireNumber = -(NextPt + 1);

        AttackLine[2] = attack->IndirectLine[NextPt * 2];
        AttackLine[3] = attack->IndirectLine[NextPt * 2 + 1];

        if( Verbose > CallLevel )
          printf( "%*sfsairatk:GroupAirAttack::ExecuteAttacks:1a2a %s "
                  "wait time=%lf\n", CallLevel, "",
                  aircraft[attack->AircraftNumber[i]]->AirCraftName,
                  attack->WaitTime[i] );
      }
      else {    //Direct attack, get current position on fire
        DistRes = GetDistRes();

        attack->IndirectLine[0] = attack->IndirectLine[2];
        attack->IndirectLine[1] = attack->IndirectLine[3];
        CurPt = 0;
        for( j = 0; j < GetNumPoints(attack->FireNumber); j++ ) {
          InitDist = pow2( attack->IndirectLine[0] -
                         GetPerimeter1Value(attack->FireNumber, j, XCOORD) ) +
                     pow2( attack->IndirectLine[1] -
                         GetPerimeter1Value(attack->FireNumber, j, YCOORD) );
          if( j == 0 ) TestDist = InitDist;
          else if( InitDist < TestDist ) {
            TestDist = InitDist;
            CurPt = j;
            if( TestDist == 0.0 ) break;
          }
        }
        for( j = 1; j <= GetNumPoints(attack->FireNumber); j++ ) {
          if( attack->Direction == 0 ) {
            NextPt = CurPt + j;
            if( NextPt > GetNumPoints(attack->FireNumber) - 1 )
              NextPt -= GetNumPoints( attack->FireNumber );
          }
          else {
            NextPt = CurPt - j;
            if( NextPt < 0 ) NextPt += GetNumPoints( attack->FireNumber );
          }
          InitDist = sqrt( pow2(attack->IndirectLine[0] -
                                GetPerimeter1Value(attack->FireNumber,
                                                   NextPt, XCOORD)) +
                           pow2(attack->IndirectLine[1] -
                                GetPerimeter1Value(attack->FireNumber,
                                                   NextPt, YCOORD)) );
          if( InitDist > MinDist ) break;
        }
        AttackLine[2] =
                     GetPerimeter1Value( attack->FireNumber, NextPt, XCOORD );
        AttackLine[3] =
                     GetPerimeter1Value( attack->FireNumber, NextPt, YCOORD );
        XDist = attack->IndirectLine[0] - AttackLine[2];
        YDist = attack->IndirectLine[1] - AttackLine[3];
        Ratio = MinDist / InitDist;
        attack->IndirectLine[2] = attack->IndirectLine[0] - XDist * Ratio;
        attack->IndirectLine[3] = attack->IndirectLine[1] - YDist * Ratio;
        AttackLine[0] = attack->IndirectLine[0];
        AttackLine[1] = attack->IndirectLine[1];

        //Move drop line out from existing fire front by DistRes.
        Ratio = DistRes / InitDist;
        if( attack->Direction == 0 ) {  //If following original fire direction
          AttackLine[0] = AttackLine[0] - YDist * Ratio;
          AttackLine[1] = AttackLine[1] + XDist * Ratio;
          AttackLine[2] = AttackLine[2] - YDist * Ratio;
          AttackLine[3] = AttackLine[3] + XDist * Ratio;
        }
        else {
          AttackLine[0] = AttackLine[0] + YDist * Ratio;
          AttackLine[1] = AttackLine[1] - XDist * Ratio;
          AttackLine[2] = AttackLine[2] + YDist * Ratio;
          AttackLine[3] = AttackLine[3] - XDist * Ratio;
        }
      }
      SetupAirAttack( attack->AircraftNumber[i], attack->CoverageLevel[i],
                      attack->EffectiveDuration[i], AttackLine );
      attack->WaitTime[i] += aircraft[attack->AircraftNumber[i]]->ReturnTime;
      if( fabs(attack->WaitTime[i] -
                    aircraft[attack->AircraftNumber[i]]->ReturnTime) <= 1e-3 )
        attack->WaitTime[i] = aircraft[attack->AircraftNumber[i]]->ReturnTime;
      aircraft[attack->AircraftNumber[i]]->NumDrops++;
      if( Verbose > CallLevel )
        printf( "%*sfsairatk:GroupAirAttack::ExecuteAttacks:1a2 %s "
                "wait time=%lf\n", CallLevel, "",
                aircraft[attack->AircraftNumber[i]]->AirCraftName,
                attack->WaitTime[i] );
    }
  }

  if( Verbose > CallLevel )
    printf( "%*sfsairatk:GroupAirAttack::ExecuteAttacks:2\n", CallLevel, "" );
  CallLevel--;

  return true;
} //GroupAirAttack::ExecuteAttacks

//============================================================================
double GroupAirAttack::GetNextAttackTime( double TimeIncrement )
{ //GroupAirAttack::GetNextAttackTime
  long i;

  for( i = 0; i < attack->NumCurrentAircraft; i++ ) {
    if( attack->WaitTime[i] < TimeIncrement )
      TimeIncrement = attack->WaitTime[i];
  }

  return TimeIncrement;
} //GroupAirAttack::GetNextAttackTime

//============================================================================
void GroupAirAttack::IncrementWaitTimes( double TimeIncrement )
{ //GroupAirAttack::IncrementWaitTimes
  CallLevel++;
  if( Verbose > CallLevel )
    printf( "%*sfsairatk:GroupAirAttack::IncrementWaitTimes:1 "
            "attack->NumCurrentAircraft=%ld\n",
            CallLevel, "", attack->NumCurrentAircraft );

  long i;

  for( i = 0; i < attack->NumCurrentAircraft; i++ ) {
    attack->WaitTime[i] -= TimeIncrement;
    if( attack->WaitTime[i] <= 0.0 ) attack->WaitTime[i] = 0.0;

    if( Verbose > CallLevel )
      printf( "%*sfsairatk:GroupAirAttack::IncrementWaitTimes:1a %ld "
              "wait time=%lf\n",
              CallLevel, "", i, attack->WaitTime[i] );
  }

  if( Verbose > CallLevel )
    printf( "%*sfsairatk:GroupAirAttack::IncrementWaitTimes:2\n",
     CallLevel, "" );
  CallLevel--;
} //GroupAirAttack::IncrementWaitTimes

//============================================================================
long GroupAirAttack::CheckSuspendState( long OnOff )
{ //GroupAirAttack::CheckSuspendState
  if( OnOff >= 0 ) attack->Suspended = OnOff;

  return attack->Suspended;
} //GroupAirAttack::CheckSuspendState
