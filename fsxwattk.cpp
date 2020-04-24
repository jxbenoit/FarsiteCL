/*fsxwattk.cpp
  Suppression Capabilities.

  Copyright 1994, 1995, 1996
  Mark A. Finney, Systems for Environmental Management
  See LICENSE.TXT file for license information.
*/
#include<malloc.h>
#include<stdio.h>
#include<string.h>
#include"fsxwattk.h"
#include"fsglbvar.h"
#include"portablestrings.h"
#include"globals.h"

//----------------------------------------------------------------------------
//Crew Stuff
//----------------------------------------------------------------------------
long NumCrews = 0;
Crew* crew[200];

//============================================================================
Crew::Crew()
{ //Crew::Crew
  long i;

  for( i = 0; i < 51; i++ )  //Initialize LineProduction Rates
    LineProduction[i] = 0.0;
  memset( CrewName, 0x0, sizeof(CrewName) );
  Units = 1;
  FlameLimit = 2.0;
  Compound = -1;
  FlatFee = 0.0;
} //Crew::Crew

//----------------------------------------------------------------------------
//Attack Class Functions
//----------------------------------------------------------------------------

//============================================================================
Attack::Attack()
{ //Attack::Attack
  NumInsertPoints = 0;
  LineOffset = -1.0;
  LineRate = 0;
} //Attack::Attack

//============================================================================
Attack::~Attack() { }

//============================================================================
long Attack::ProblemQuad()
{ //Attack::ProblemQuad
  double testx, testy;

  if( Cross(xpt1, ypt1, xpt1n, ypt1n, xpt2, ypt2, xpt2n, ypt2n, &testx,
            &testy) ) {
    //Extinguish 1n and travel directly to 2n and along next perim.

    return 1;
  }
  else if( Cross(xpt1, ypt1, xpt2, ypt2, xpt1n, ypt1n, xpt2n, ypt2n, &testx,
                 &testy) ) {
    if( sqrt(pow2(xpt1 - xpt1n) + pow2(ypt1 - ypt1n)) > 1e-9 )
      return 1; //Extinguish 1n and travel directly to 2n and along next perim
  }
  else if( ! Cross(xpt1, ypt1, xpt2n, ypt2n, xpt1n, ypt1n, xpt2, ypt2,
                   &testx, &testy) ) {
    if( sqrt(pow2(xpt1 - xpt1n) + pow2(ypt1 - ypt1n)) > 1e-9 )
      return 2;      //Go to atleast 1n and 2n and along next perim
  }

  return 0;
} //Attack::ProblemQuad

//============================================================================
long Attack::FindCurrentPoint()
{ //Attack::FindCurrentPoint
  long   i;
  double dist, mindist = -1.0;

  if( attack->IndirectLine1[2] == -1.0 ) {  //First time for attack
    attack->IndirectLine1[2] = 1;   //Change value for subsequent attacks
    for( i = 0; i < GetNumPoints(attack->FireNumber); i++ ) {
      xpt1 = GetPerimeter2Value( i, XCOORD );
      ypt1 = GetPerimeter2Value( i, YCOORD );
      //Use existing xpt1,ypt1 coords from last timestep for comparison.
      if( xpt1 == attack->IndirectLine1[0] &&
          ypt1 == attack->IndirectLine1[1] ) {
        attack->CurrentPoint = i;
        break;
      }
    }
  }
  else {
    for( i = 0; i < GetNumPoints(attack->FireNumber); i++ ) {
      if( GetPerimeter2Value(i, FLIVAL) < 0.0 ) {
        xpt1 = GetPerimeter2Value( i, XCOORD );
        ypt1 = GetPerimeter2Value( i, YCOORD );
        //Use existing xpt1,ypt1 coords from last timestep for comparison.
        dist = pow2( xpt1 - attack->IndirectLine1[0] ) +
               pow2( ypt1 - attack->IndirectLine1[1] );
        if( dist == 0.0 ) {
          attack->CurrentPoint = i;

          break;
        }
        else if( mindist < 0.0 || dist < mindist ) {
          mindist = dist;
          attack->CurrentPoint = i;  //Use closest point
        }
      }
    }
    if( i == GetNumPoints(attack->FireNumber) ) {  //Couldn't find dead point
      if( attack->CurrentPoint >= i ) attack->CurrentPoint = 0;
    }
  }

  return attack->CurrentPoint;
} //Attack::FindCurrentPoint

/*============================================================================
  Attack::CalcChordArcRatio
  Computes ratio of circular arc to chord distance for given angle
  used to correct linear approximation to line construction within
  quadrilateral.
*/
void Attack::CalcChordArcRatio( double LastX, double LastY )
{ //Attack::CalcChordArcRatio
  double PerpDist1 = 0, PerpDist2 = 0, PerpDiff, TanDist1, TanDist2;
  double RefAngle, Angle, AddDist1, AddDist2, DiagDist, TestDist;
  double SinDiff, CosDiff, SpreadDist1, SpreadDist2;

  ChordArcRatio = 1.0;  //Attack:: Data Member

  return;  //AAA This must be here to short-circuit the rest of the code.

  TanDist1 = sqrt( pow2(xpt1 - xpt2) + pow2(ypt1 - ypt2) );
  if( TanDist1 == 0.0 ) return;
  RefAngle = atan2( ypt1 - ypt2, xpt1 - xpt2 );

  SpreadDist1 = sqrt( pow2(xpt1n - xpt1) + pow2(ypt1n - ypt1) );
  if( SpreadDist1 > 0.0 ) {
    Angle = atan2( ypt1 - ypt1n, xpt1 - xpt1n );
    SinDiff = sin( RefAngle ) * cos( Angle ) - cos( RefAngle ) * sin( Angle );
    PerpDist1 = fabs( SinDiff * SpreadDist1 );
  }
  CosDiff = cos( RefAngle ) * cos( Angle ) + sin( RefAngle ) * sin( Angle );
  RefAngle += PI;

  SpreadDist2 = sqrt( pow2(LastX - xpt2) + pow2(LastY - ypt2) );
  if( SpreadDist2 > 0.0 ) {
    Angle = atan2( ypt2 - LastY, xpt2 - LastX );
    SinDiff = sin( RefAngle ) * cos( Angle ) - cos( RefAngle ) * sin( Angle );
    PerpDist2 = fabs( SinDiff * SpreadDist2 );
  }
  if( SpreadDist2 < PerpDist2 ) PerpDist2 = SpreadDist2;
  PerpDiff = PerpDist2 - PerpDist1;

  if( fabs(PerpDiff) < 0.01 ) return;

  /*
  BaseDist = TanDist1;
  //Correct for perpendicular projection 2.
  AddDist2 = sqrt( pow2(SpreadDist2) - pow2(PerpDist2) );
  if( SpreadDist2 > PerpDist2) {      //Eliminate potential precision problem
    //Diagonal distance across quadrilateral.
    DiagDist = pow2( xpt2n - xpt1 ) + pow2( ypt2n - ypt1 );
    TestDist = pow2( TanDist1 ) + pow2( PerpDist2 );   //Hypotenuse
    if( TestDist < DiagDist )  //Must decrease PerpDist1
      BaseDist += AddDist2;
    else    //Must increase BaseDist AND increase PerpDist1
      BaseDist -= AddDist2;
  }
  //Correct for perpendicular projection 1.
  AddDist1 = sqrt( pow2(SpreadDist1) - pow2(PerpDist1) );
  if( SpreadDist1 > PerpDist1 ) {  //Could go either way
    //Diagonal distance across quadrilateral.
    DiagDist = pow2( xpt1n - xpt2 ) + pow2( ypt1n - ypt2 );
    TestDist = pow2( TanDist1 ) + pow2( PerpDist1 );   //Hypotenuse
    if( TestDist < DiagDist ) {
      //BaseDist += AddDist1;
      dRdX = PerpDiff / ( BaseDist + AddDist1 );
      //Increase perp height at start point (x1, y1).
      PerpDist1 += dRdX * AddDist1;
    }
    else {
      //BaseDist -= AddDist1;
      dRdX = PerpDiff / ( BaseDist-AddDist1 );
      //Reduce perp height at start point (x1, y1).
      PerpDist1 -= dRdX * AddDist1;
    }
  }
  PerpDiff = PerpDist2 - PerpDist1;  //Corrected
  if( fabs(PerpDiff) < 0.01 ) return;
  if( BaseDist < 0.0 ) return;  //Problem quadrangle
  double PDFsq, PDSsq, Lsq, Line, LDiff, LHyp, g, inc, Drop;
  PDSsq = pow2( PerpDist1 );
  PDFsq = pow2( PerpDiff );
  Lsq = pow2( LastX - xpt1 ) + pow2( LastY - ypt1 );
  //TanDist2 = sqrt( Lsq );
  if( PerpDiff > 0.0 ) g = inc = BaseDist * 0.1;
  else g = inc = -BaseDist * 0.1;
  do {
    LHyp = pow2( BaseDist + g ) + PDSsq + PDFsq + pow2( g );
    LDiff = Lsq - LHyp;
    if( PerpDiff > 0.0 ) {
      if( LDiff < 0.0 ) {
        g -= inc;
        inc /= 2.0;
        g += inc;
      }
      else g += inc;
    }
    else {
      if( LDiff > 0.0 ) {
        g -= inc;
        inc /= 2.0;
        g += inc;
      }
      else g += inc;
    }
  } while( fabs(LDiff) > 1e-6 );
  Drop = sqrt( pow2(g) + PDFsq );
  Angle = asin( Drop / sqrt(Lsq) );
  if( Angle > 0.0 )
    ChordArcRatio = fabs( sin(Angle)/Angle );//Ratio of arc dist to chord dist
  */

  //Modify PerpDiff depending on angle of SpreadDist1.
  if( SpreadDist1 > PerpDist1 ) {
    TanDist2 = sqrt( pow2(LastX - xpt1n) + pow2(LastY - ypt1n) );
    if( TanDist2 > 0.0 ) {
      AddDist1 = fabs( CosDiff * PerpDiff /
                       sqrt(pow2(TanDist2) - pow2(PerpDiff)) );
      //Diagonal distance across quadrilateral.
      DiagDist = pow2( xpt1n - xpt2 ) + pow2( ypt1n - ypt2 );
      TestDist = pow2( TanDist1 ) + pow2( PerpDist1 );   //Hypotenuse
      if( TestDist > DiagDist ) PerpDiff += AddDist1;
      else PerpDiff -= AddDist1;
    }
  }

  if( PerpDiff > 0.0 ) {
    //Modify TanDist1 depending on angle of SpreadDist2.
    AddDist2 = sqrt( pow2(SpreadDist2) - pow2(PerpDist2) );  //Additional dist
    //Diagonal distance across quadrilateral.
    DiagDist = pow2( LastX - xpt1 ) + pow2( LastY - ypt1 );
    TestDist = pow2( TanDist1 ) + pow2( PerpDist2 );  //Hypotenuse
    if( TestDist > DiagDist ) TanDist1 -= AddDist2;
    else TanDist1 += AddDist2;
    if( TanDist1 > 0.0 ) {
      //-----------------------------------
      //Angle=fabs(atan(PerpDiff/TanDist1));     // or.....
      //-----------------------------------

      if( PerpDist1 < PerpDist2 ) {
        TestDist = sqrt( pow2(TanDist1) + DiagDist );
        Angle = fabs( acos(PerpDist1 / TestDist) -
                      acos(PerpDist2 / TestDist) );
      }
      else {
        TestDist = sqrt( pow2(TanDist1) + pow2(PerpDist1) );
        Angle = fabs( acos(PerpDist2 / TestDist) -
                      acos(PerpDist1 / TestDist) );
      }

      if( Angle > 0.0 )
        //Ratio of arc dist to chord dist.
        ChordArcRatio = fabs( sin(Angle) / Angle );
    }
  }
} //Attack::CalcChordArcRatio

//============================================================================
long Attack::CheckOverlapAttacks( long* Attacks, double TimeStep )
{ //Attack::CheckOverlapAttacks
  long i;

  for( i = 0; i < GetNumAttacks(); i++ ) {
    // 1. find out how many direct attacks are on same fire
    // 2. find nearest point on fire for each fire
    // 3. estimate end point after next time step
    //    (must move past fli=-1 points
    // 4. if line routes overlap at all, alloc Attacks array with indices to
    //    Attacks
    // 5. return number of attacks that will overlap
  }

  return 0;
} //Attack::CheckOverlapAttacks

//============================================================================
bool Attack::DirectAttack( AttackData* atk, double TimeStep )
{ //Attack::DirectAttack
  CallLevel++;
  if( Verbose > CallLevel )
    printf( "%*sfsxwattk:DirectAttack:1\n", CallLevel, "" );

  //Perform direct attack.
  double HorizPerimDist, DiagonalDist, SurfDist, LineRate, SuppressionDist;
  double FlameLength, FlankDist, LineDist = 0.0;
  double ratio, PreviousSpreadRatio, CuumSpreadDist;
  double newx, newy;
  long   i, j, k, posit, FuelType;
  bool   Replace1n = false;
  bool   Replace2n = false;
  long   PointToInsert;
  double ReplaceX1n, ReplaceY1n, ReplaceR1n, ReplaceF1n, ReplaceC1n;
  double ReplaceX2n, ReplaceY2n, ReplaceR2n, ReplaceF2n, ReplaceC2n;
  double ExtendXn, ExtendYn;
  double newdistn, totaldistn, NewPt2Dist, OriginalPt2Dist;
  double OriginalTimeStep;
  celldata tempcell;
  crowndata tempcrown;
  grounddata tempground;

  attack = atk;
  FindCurrentPoint();
  PreviousSpreadRatio = CuumSpreadDist = 0.0;
  GetPerimeter2( attack->CurrentPoint, &xpt1, &ypt1, &ros1, &fli1, &rcx1 );
  xpt1n = GetPerimeter1Value( attack->FireNumber, attack->CurrentPoint,
                              XCOORD );
  ypt1n = GetPerimeter1Value( attack->FireNumber, attack->CurrentPoint,
                              YCOORD );
  ros1n = GetPerimeter1Value( attack->FireNumber, attack->CurrentPoint,
                              ROSVAL );
  fli1n = GetPerimeter1Value( attack->FireNumber, attack->CurrentPoint,
                              FLIVAL );
  rcx1n = GetPerimeter1Value( attack->FireNumber, attack->CurrentPoint,
                              RCXVAL );

  OriginalTimeStep = TimeStep;
  while( TimeStep > 0.0 ) {
    if( attack->Reverse ) {
      attack->NextPoint = attack->CurrentPoint - 1;
      if( attack->NextPoint < 0 )
        attack->NextPoint = GetNumPoints( attack->FireNumber ) - 1;
    }
    else {
      attack->NextPoint = attack->CurrentPoint + 1;
      if( attack->NextPoint > GetNumPoints(attack->FireNumber) - 1 )
        attack->NextPoint = 0;
    }
    GetPerimeter2( attack->NextPoint, &xpt2, &ypt2, &ros2, &fli2, &rcx2 );
    fli2n = GetPerimeter1Value( attack->FireNumber, attack->NextPoint,
                                FLIVAL );
    if( fli2 < 0.0 || fli2n < 0.0 ) {  //Can't suppress extinguished fire
      if( fli1n == 0.0 ) fli1n = 0.001;
      if( fli1n > 0.0 )
        SetFireChx( attack->FireNumber, attack->CurrentPoint, ros1n, -fli1n );
      k = attack->NextPoint;
      for( i = 1; i < GetNumPoints(attack->FireNumber); i++ ) {
        if( attack->Reverse ) {
          j = attack->CurrentPoint - i;
          if( j < 0 ) j = GetNumPoints( attack->FireNumber ) - i;
        }
        else {
          j = i + attack->CurrentPoint;
          if( j > GetNumPoints(attack->FireNumber) - 1 )
            j -= GetNumPoints( attack->FireNumber );
        }
        fli1 = GetPerimeter2Value( j, FLIVAL );
        if( fli1 >= 0.0 ) {
          fli1n = GetPerimeter1Value( attack->FireNumber, j, FLIVAL );
          if( fli1n >= 0.0 ) {
            attack->CurrentPoint = k;
            GetPerimeter2( attack->CurrentPoint, &xpt1, &ypt1, &ros1, &fli1,
                           &rcx1 );
            xpt1n = GetPerimeter1Value( attack->FireNumber,
                                        attack->CurrentPoint, XCOORD );
            ypt1n = GetPerimeter1Value( attack->FireNumber,
                                        attack->CurrentPoint, YCOORD );
            ros1n = GetPerimeter1Value( attack->FireNumber,
                                        attack->CurrentPoint, ROSVAL );
            fli1n = GetPerimeter1Value( attack->FireNumber,
                                        attack->CurrentPoint, FLIVAL );
            rcx1n = GetPerimeter1Value( attack->FireNumber,
                                        attack->CurrentPoint, RCXVAL );
            if( fli1n == 0.0 ) fli1n = 0.001;
            break;
          }
        }
        k = j;
      }

      //Searched fire and no unburned points found.
      if( i == GetNumPoints(attack->FireNumber) ) {
        if( Verbose > CallLevel )
          printf( "%*sfsxwattk:DirectAttack:1a\n", CallLevel, "" );
        CallLevel--;

        return false;
      }
      else continue;
    }
    xpt2n = GetPerimeter1Value( attack->FireNumber, attack->NextPoint,
                                XCOORD );
    ypt2n = GetPerimeter1Value( attack->FireNumber, attack->NextPoint,
                                YCOORD );
    ros2n = GetPerimeter1Value( attack->FireNumber, attack->NextPoint,
                                ROSVAL );
    fli2n = GetPerimeter1Value( attack->FireNumber, attack->NextPoint,
                                FLIVAL );
    rcx2n = GetPerimeter1Value( attack->FireNumber, attack->NextPoint,
                                RCXVAL );

    OriginalPt2Dist = sqrt( pow2(xpt2n - xpt2) + pow2(ypt2n - ypt2) );
    if( PreviousSpreadRatio > 0.0 ) {  //Form new pro-rated xpt2
      CuumSpreadDist = OriginalPt2Dist * PreviousSpreadRatio;
      xpt2 = xpt2 - ( xpt2 - xpt2n ) * PreviousSpreadRatio;
      ypt2 = ypt2 - ( ypt2 - ypt2n ) * PreviousSpreadRatio;
      PreviousSpreadRatio = 0.0;
    }
    CellData( xpt1, ypt1, tempcell, tempcrown, tempground, &posit );
    FuelType = GetFuelConversion( tempcell.f );
    if( FuelType > 0 ) {
      LineRate = crew[attack->CrewNum]->LineProduction[FuelType - 1] *
                 MetricResolutionConvert();
      if( ros2 == 0.0 ) {   //Create temporary false point for calcs
        xpt2n = xpt2 - ( xpt1 - xpt1n );
        ypt2n = ypt2 - ( ypt1 - ypt1n );
      }
    }
    else {
      attack->CurrentPoint = attack->NextPoint;
      xpt1 = xpt2;
      ypt1 = ypt2;
      xpt1n = xpt2n;
      ypt1n = ypt2n;
      ros1 = ros2;
      ros1n = ros2n;
      fli1 = fli2;
      fli1n = fli2n;
      rcx1 = rcx2;
      rcx1n = rcx2n;
      continue;
    }
    if( LineDist < 1e-9 ) {
      LineDist = LineRate * TimeStep;
      //Don't prosecute attack with such small line construction.
      if( LineDist < 0.001 ) {
        if( Verbose > CallLevel )
          printf( "%*sfsxwattk:DirectAttack:1b\n", CallLevel, "" );
        CallLevel--;

        return true;
      }
      attack->LineBuilt += ( LineDist / MetricResolutionConvert() );
    }
    DiagonalDist = sqrt( pow2(xpt1 - xpt2n) + pow2(ypt1 - ypt2n) );
    HorizPerimDist = sqrt( pow2(xpt1 - xpt2) + pow2(ypt1 - ypt2) );
    ConvertLandData( tempcell.s, tempcell.a );
    SurfDist = CalculateSlopeDist( xpt1, ypt1, xpt2n, ypt2n );
    LineDist *= DiagonalDist / SurfDist;
    CalcChordArcRatio( xpt2n, ypt2n );
    LineDist *= ChordArcRatio;
    FlameLength = 0.0775 * pow( fabs(fli1), 0.46 );
    FlankDist = sqrt( pow2(xpt1n - xpt1) + pow2(ypt1n - ypt1) );

    if( (posit = ProblemQuad()) > 0 ) {
      if( posit == 1 ) {
        HorizPerimDist = sqrt( pow2(xpt1 - xpt2n) + pow2(ypt1 - ypt2n) );
        if( HorizPerimDist > 0.0 ) {
          if( LineDist >= HorizPerimDist ) {
            TimeStep = 0.0;
            ReplaceX2n = newx = xpt2n;
            ReplaceY2n = newy = ypt2n;
            ReplaceR2n = ros2n;
            ReplaceF2n = fli2n;
            ReplaceC2n = rcx2n;
            Replace2n = true;
            PointToInsert = 0;
          }
          else if( LineDist < HorizPerimDist ) {
            newx = xpt1 - (xpt1 - xpt2n) * LineDist / HorizPerimDist;
            newy = ypt1 - (ypt1 - ypt2n) * LineDist / HorizPerimDist;
            PointToInsert = 3;
            TimeStep = 0.0;
          }
        }
        else {
          newx = xpt2n;
          newy = ypt2n;
          PointToInsert = 0;
          TimeStep = 0.0;
        }
        ReplaceX1n = xpt1;
        ReplaceY1n = ypt1;
        ReplaceR1n = ros1;
        ReplaceF1n = fli1;
        ReplaceC1n = rcx1;
      }
      else {
        TimeStep = 0.0;
        ReplaceX1n = xpt1n;
        ReplaceY1n = ypt1n;
        ReplaceR1n = ros1n;
        ReplaceF1n = fli1n;
        ReplaceC1n = rcx1n;
        ReplaceX2n = newx = xpt2n;
        ReplaceY2n = newy = ypt2n;
        ReplaceR2n = ros2n;
        ReplaceF2n = fli2n;
        ReplaceC2n = rcx2n;
        Replace1n = true;      //Really means it
        Replace2n = true;
        PointToInsert = 0;
      }
    }
    else if( LineDist < DiagonalDist ) {
      if( xpt1 == xpt1n && ypt1 == ypt1n ) {
        xpt1n = xpt1 - ( xpt2 - xpt2n );  //Form new pt1n from dims of pt2n
        ypt1n = ypt1 - ( ypt2 - ypt2n );
      }
      if( FlameLength < crew[attack->CrewNum]->FlameLimit ) {
        if( IterateIntersection(LineDist, xpt2n, ypt2n, xpt1, ypt1,
                                xpt1n, ypt1n, &newx, &newy) )
          PointToInsert = 1;
        else PointToInsert = 2;
      }
      else {  //Flank only;
        ratio = LineDist / FlankDist;
        if( ratio > 1.0 ) ratio = 1.0;
        newx = xpt1 - ( xpt1 - xpt1n ) * ratio;
        newy = ypt1 - ( ypt1 - ypt1n ) * ratio;
        PointToInsert = 2;
        WriteAttackLog( attack, 3, 0, 0 );
      }
      ReplaceX1n = xpt1;
      ReplaceY1n = ypt1;
      ReplaceR1n = ros1;
      ReplaceF1n = fli1;
      ReplaceC1n = rcx1;
      TimeStep = 0.0;
    }
    else if( LineDist > DiagonalDist ) {
      if( FlameLength < crew[attack->CrewNum]->FlameLimit ) {
        if( LineDist > HorizPerimDist ) ratio = LineDist * 2.0;
        else
          //Extend linedist out from x1,y1 past x2,y2 by double LineDist.
          ratio = HorizPerimDist * 2.0;
        ExtendXn = xpt1n - ( xpt1n - xpt2n ) * ( ratio / HorizPerimDist );
        ExtendYn = ypt1n - ( ypt1n - ypt2n ) * ( ratio / HorizPerimDist );

        if( IterateIntersection(LineDist, ExtendXn, ExtendYn, xpt1,
                                ypt1, xpt2n, ypt2n, &newx, &newy) ) {
          Cross( newx, newy, xpt1, ypt1, xpt2, ypt2, xpt2n, ypt2n,
                 &ReplaceX2n, &ReplaceY2n );
          newx = ReplaceX2n;
          newy = ReplaceY2n;
        }
        else {
          ReplaceX2n = newx;
          ReplaceY2n = newy;
        }
        Replace2n = true;
        ReplaceR2n = ros2n;
        ReplaceF2n = fli2n;
        ReplaceC2n = rcx2n;
        SuppressionDist = sqrt( pow2(ReplaceX2n - xpt1 ) +
                                pow2(ReplaceY2n - ypt1) );
        if( SuppressionDist > LineDist ) SuppressionDist = LineDist;
        TimeStep *= ( 1.0 - SuppressionDist / LineDist );
        LineDist -= SuppressionDist;
        PointToInsert = 0;
      }
      else {  //Flank only;
        ratio = LineDist / FlankDist;
        if( ratio > 1.0 ) ratio = 1.0;
        newx = xpt1 - ( xpt1 - xpt1n ) * ratio;
        newy = ypt1 - ( ypt1 - ypt1n ) * ratio;
        TimeStep = 0.0;
        PointToInsert = 2;
        LineDist = 0.0;
      }
      ReplaceX1n = xpt1;
      ReplaceY1n = ypt1;
      ReplaceR1n = ros1;
      ReplaceF1n = fli1;
      ReplaceC1n = rcx1;
    }
    else {  //Equal
      ReplaceX1n = xpt1;
      ReplaceY1n = ypt1;
      ReplaceR1n = ros1;
      ReplaceF1n = fli1;
      ReplaceX2n = newx = xpt2n;
      ReplaceY2n = newy = ypt2n;
      ReplaceR2n = ros2n;
      ReplaceF2n = fli2n;
      ReplaceC2n = rcx2n;
      Replace2n = true;
      PointToInsert = 0;
      TimeStep = 0.0;
    }
    if( ReplaceF1n > 0.0 ) ReplaceF1n *= -1.0;
    if( attack->AttackTime > 0.0 ) {
      SetPerimeter1( attack->FireNumber, attack->CurrentPoint,
                     ReplaceX1n, ReplaceY1n );
    if( ReplaceF1n == 0.0 ) ReplaceF1n = -0.001;
      SetFireChx( attack->FireNumber, attack->CurrentPoint, ReplaceR1n,
                  ReplaceF1n );
      SetReact( attack->FireNumber, attack->CurrentPoint, ReplaceC1n );
    }
    else {
      if( ! Replace1n ) {
        if( fli1n < 0.0 && fli1 > 0.0 ) ReplaceF1n = fabs( ReplaceF1n );
      }
      else Replace1n = false;
      SetPerimeter2( attack->CurrentPoint, ReplaceX1n, ReplaceY1n,
                     ReplaceR1n, ReplaceF1n, rcx1 );
    }
    switch ( PointToInsert ) {
      case 0: break;
      case 1:
        newdistn = sqrt( pow2(newx - xpt1n) + pow2(newy - ypt1n) );
        totaldistn = sqrt( pow2(xpt2n - xpt1n) + pow2(ypt2n - ypt1n) );
        ros1n = ros1n * ( 1.0 - newdistn / totaldistn ) + ros2n * newdistn /
                totaldistn;
        fli1n = fabs( fli1n ) * ( 1.0 - newdistn / totaldistn ) +
                fabs( fli2n ) * newdistn / totaldistn;
        rcx1n = rcx1n * ( 1.0 - newdistn / totaldistn ) + rcx2n * newdistn /
                totaldistn;
        InsertPerimeterPoint( newx, newy, ros1n, -fli1n, rcx1n, 1 );
        break;
      case 2:
        newdistn = sqrt( pow2(newx - xpt1) + pow2(newy - ypt1) );
        totaldistn = sqrt( pow2(xpt1n - xpt1) + pow2(ypt1n - ypt1) );
        ros1n = ros1 * ( 1.0 - newdistn / totaldistn ) + ros1n * newdistn /
                totaldistn;
        fli1n = fabs( fli1 ) * ( 1.0 - newdistn / totaldistn ) +
                fabs( fli1n ) * newdistn / totaldistn;
        rcx1n = rcx1 * ( 1.0 - newdistn / totaldistn ) + rcx1n * newdistn /
                totaldistn;
        InsertPerimeterPoint( newx, newy, ros1n, -fli1n, rcx1n, 2 );
        break;
      case 3:
        newdistn = sqrt( pow2(newx - xpt1) + pow2(newy - ypt1) );
        totaldistn = sqrt( pow2(xpt2n - xpt1) + pow2(ypt2n - ypt1) );
        ros1n = ros1 * ( 1.0 - newdistn / totaldistn ) + ros2n * newdistn /
                totaldistn;
        fli1n = fabs( fli1 ) * ( 1.0 - newdistn / totaldistn ) +
                fabs(fli2n) * newdistn / totaldistn;
        rcx1n = rcx1 * ( 1.0 - newdistn / totaldistn ) + rcx2n * newdistn /
                totaldistn;
        InsertPerimeterPoint( newx, newy, ros1n, -fli1n, rcx1n, 3 );
        break;
    }
    if( Replace2n ) {
      if( ReplaceF2n == 0.0 ) ReplaceF2n = 0.001;
      if( ReplaceF2n > 0.0 ) ReplaceF2n *= -1.0;
      SetPerimeter1( attack->FireNumber, attack->NextPoint, ReplaceX2n,
                     ReplaceY2n );
      SetFireChx( attack->FireNumber, attack->NextPoint, ReplaceR2n,
                  ReplaceF2n );
      SetReact( attack->FireNumber, attack->NextPoint, ReplaceC2n );
    }
    if( TimeStep > 0.0 ) {
      PointToInsert = 0;
      Replace2n = false;
      attack->CurrentPoint = attack->NextPoint;
      NewPt2Dist = sqrt( pow2(newx - xpt2) + pow2(newy - ypt2) );
      if( OriginalPt2Dist < 1e-9 ) PreviousSpreadRatio = 1.0;
      else {
        CuumSpreadDist += NewPt2Dist;
        PreviousSpreadRatio = CuumSpreadDist / OriginalPt2Dist;
        if( PreviousSpreadRatio > 1.0 ) PreviousSpreadRatio = 1.0;
      }
      xpt1n = xpt2n;
      ypt1n = ypt2n;
      ros1 = ros2;
      ros1n = ros2n;
      fli1 = fli2;
      fli1n = fli2n;
      rcx1 = rcx2;
      rcx1n = rcx2n;
    }
    xpt1 = attack->IndirectLine1[0] = newx;
    ypt1 = attack->IndirectLine1[1] = newy;
  }
  attack->AttackTime += OriginalTimeStep;

  if( Verbose > CallLevel )
    printf( "%*sfsxwattk:DirectAttack:2\n", CallLevel, "" );
  CallLevel--;

  return true;
} //Attack::DirectAttack

/*============================================================================
  Attack::IndirectAttack
  Perform indirect attack.
*/
bool Attack::IndirectAttack( AttackData* atk, double TimeStep )
{ //Attack::IndirectAttack
  CallLevel++;
  if( Verbose > CallLevel )
    printf( "%*sfsxwattk:IndirectAttack:1\n", CallLevel, "" );
  
  long   i, posit, NewPoint, PointCount;
  double ratio, LineRate, HorizDist, SurfDist;
  double LineDist = 0.0;
  double DistRes = GetDistRes();
  celldata tempcell;
  crowndata tempcrown;
  grounddata tempground;
  bool   CrewIsActive = true;
  bool   DistanceCheck, NewEndPoint;
  long   StartPoint;
  long   FuelType;

  if( Verbose > CallLevel )
    printf( "%*sfsxwattk:IndirectAttack:2\n", CallLevel, "" );

  attack = atk;
  StartPoint = attack->CurrentPoint;
  if( StartPoint > attack->NumPoints - 1 )
    StartPoint = attack->CurrentPoint = attack->NumPoints - 1;
  NewEndPoint = false;
  DistanceCheck = false;
  xpt1 = attack->IndirectLine1[attack->CurrentPoint * 2];
  ypt1 = attack->IndirectLine1[attack->CurrentPoint * 2 + 1];
  if( attack->Burnout ) {
    if( attack->BurnLine[0] == -1 ) attack->BurnLine[0] = StartPoint;
  }
  attack->AttackTime += TimeStep;

  if( Verbose > CallLevel )
    printf( "%*sfsxwattk:IndirectAttack:3\n", CallLevel, "" );

  while( TimeStep > 0.0 ) {  //Perform distance checking in line building rate
    if( Verbose > CallLevel )
      printf( "%*sfsxwattk:IndirectAttack:3a\n", CallLevel, "" );
    if( ! DistanceCheck ) {
      attack->CurrentPoint++;
      if( attack->CurrentPoint < attack->NumPoints ) DistanceCheck = false;
      else {
        attack->CurrentPoint--;
        CrewIsActive = false;
        break;
      }
    }
    else DistanceCheck = false;

    if( Verbose > CallLevel )
      printf( "%*sfsxwattk:IndirectAttack:3b\n", CallLevel, "" );

    xpt2 = attack->IndirectLine1[attack->CurrentPoint * 2];
    ypt2 = attack->IndirectLine1[attack->CurrentPoint * 2 + 1];

    if( Verbose > CallLevel )
      printf( "%*sfsxwattk:IndirectAttack:3c xpt1=%lf ypt1=%lf "
              "WestUTM=%lf SouthUTM=%lf\n",
              CallLevel, "", xpt1, ypt1, GetWestUtm(), GetSouthUtm() );

    CellData( xpt1, ypt1, tempcell, tempcrown, tempground, &posit );

    if( Verbose > CallLevel )
      printf( "%*sfsxwattk:IndirectAttack:3d\n", CallLevel, "" );

    FuelType = GetFuelConversion( tempcell.f );

    if( Verbose > CallLevel )
      printf( "%*sfsxwattk:IndirectAttack:3e\n", CallLevel, "" );

    if( FuelType > 0 )
      LineRate = crew[attack->CrewNum]->LineProduction[FuelType - 1] *
                 MetricResolutionConvert();
    else LineRate = 0.0;

    if( Verbose > CallLevel )
      printf( "%*sfsxwattk:IndirectAttack:3f\n", CallLevel, "" );

    if( LineRate == 0.0 ) {
      LineDist = sqrt( pow2(xpt1 - xpt2) + pow2(ypt1 - ypt2) );
      if( LineDist > DistRes ) {
        xpt1 = xpt1 - ( xpt1 - xpt2 ) * DistRes / LineDist;
        ypt1 = ypt1 - ( ypt1 - ypt2 ) * DistRes / LineDist;
        LineDist = 0.0;
        DistanceCheck = true;
      }
      else {
        xpt1 = xpt2;
        ypt1 = ypt2;
      }

      continue;
    }
    HorizDist = sqrt( pow2(xpt1 - xpt2) + pow2(ypt1 - ypt2) );
    if( LineDist < 1e-9 ) {
      LineDist = LineRate * TimeStep;
      attack->LineBuilt += ( LineDist / MetricResolutionConvert() );
    }

    if( Verbose > CallLevel )
      printf( "%*sfsxwattk:IndirectAttack:3g \n", CallLevel, "" );

    ConvertLandData( tempcell.s, tempcell.a );
    SurfDist = CalculateSlopeDist( xpt1, ypt1, xpt2, ypt2 );
    if( SurfDist > 0 ) LineDist *= HorizDist / SurfDist;
    else continue;

    if( LineDist < HorizDist ) {
      if( LineDist > DistRes ) {
        ratio = DistRes / LineDist;
        TimeStep *= ( 1.0 - ratio );
        LineDist -= DistRes;
        ratio = DistRes / HorizDist;
        DistanceCheck = true;
        LineDist *= SurfDist / HorizDist;  //Reset original linedist ratio
      }
      else {
        ratio = LineDist / HorizDist;
        TimeStep = 0.0;
        NewEndPoint = true;
        NumInsertPoints++;
      }
      xpt1 = xpt1 - ( xpt1 - xpt2 ) * ratio;
      ypt1 = ypt1 - ( ypt1 - ypt2 ) * ratio;
    }
    else if( LineDist > HorizDist ) {
      xpt1 = xpt2;
      ypt1 = ypt2;
      TimeStep *= ( 1.0 - HorizDist / LineDist );
      LineDist -= HorizDist;
      LineDist *= SurfDist / HorizDist;  //Reset original linedist ratio
    }
    else TimeStep = 0.0;
  }

  if( Verbose > CallLevel )
    printf( "%*sfsxwattk:IndirectAttack:4\n", CallLevel, "" );

  posit = attack->CurrentPoint - StartPoint;
  if( NewEndPoint ) {  //Insert new mid point on original line
    posit = 1;
    if( (attack->IndirectLine2 = new double[(attack->NumPoints + 1) * 2]) !=
        NULL ) {
      NewPoint = 0;
      for( i = 0; i < attack->NumPoints; i++ ) {
        attack->IndirectLine2[(i + NewPoint) * 2] =
                                                 attack->IndirectLine1[i * 2];
        attack->IndirectLine2[(i + NewPoint) * 2 + 1] =
                                             attack->IndirectLine1[i * 2 + 1];
        if( i == attack->CurrentPoint - 1 ) {
          NewPoint = 1;
          attack->IndirectLine2[(i + NewPoint) * 2] = xpt1;
          attack->IndirectLine2[(i + NewPoint) * 2 + 1] = ypt1;
        }
      }
      attack->NumPoints++;
      delete[] attack->IndirectLine1;
      attack->IndirectLine1 = new double[(attack->NumPoints + 1) * 2];
      for( i = 0; i < attack->NumPoints; i++ ) {
        attack->IndirectLine1[i * 2] = attack->IndirectLine2[i * 2];
        attack->IndirectLine1[i * 2 + 1] = attack->IndirectLine2[i * 2 + 1];
      }
      delete[] attack->IndirectLine2;
      attack->IndirectLine2 = 0;
    }
  }

  if( Verbose > CallLevel )
    printf( "%*sfsxwattk:IndirectAttack:5\n", CallLevel, "" );

  if( posit > 0 ) {
    if( Verbose > CallLevel )
      printf( "%*sfsxwattk:IndirectAttack:5a\n", CallLevel, "" );

    vectorbarrier.AllocBarrier( attack->CurrentPoint + 1 );
    for( PointCount = 0; PointCount <= attack->CurrentPoint; PointCount++ ) {
      if( Verbose > CallLevel )
        printf( "%*sfsxwattk:IndirectAttack:5a1 (%lf, %lf)\n",
                CallLevel, "", attack->IndirectLine1[PointCount * 2],
                attack->IndirectLine1[PointCount * 2 + 1] );
      vectorbarrier.SetBarrierVertex( PointCount,
                                      attack->IndirectLine1[PointCount * 2],
                                  attack->IndirectLine1[PointCount * 2 + 1] );
    }
    vectorbarrier.BufferBarrier( 1.2 );
    if( attack->FireNumber > -1 ) {  //If not first time through
    }
    else {
      attack->FireNumber = GetNewFires();
      IncNewFires( 1 );
    }

    if( Verbose > CallLevel )
      printf( "%*sfsxwattk:IndirectAttack:5b attack->FireNumber=%ld\n",
              CallLevel, "", attack->FireNumber );

    vectorbarrier.TransferBarrier( attack->FireNumber );
    BoundingBox();
    if( attack->Burnout ) {
      attack->BurnLine[1] = attack->CurrentPoint;
      BurnOut();
      if( ! CrewIsActive ) {
        if( attack->BurnLine[1] > attack->BurnLine[0] ) {
          CrewIsActive = true;
          attack->BurnDelay = 0;
        }
      }
    }
  }
  else if( ! CrewIsActive ) {
    if( attack->Burnout ) {
      attack->BurnLine[1] = attack->CurrentPoint;
      BurnOut();
    }
  }

  if( Verbose > CallLevel )
    printf( "%*sfsxwattk:IndirectAttack:6\n", CallLevel, "" );
  CallLevel--;

  return CrewIsActive;
} //Attack::IndirectAttack

/*============================================================================
  Attack::ParallelAttack
  Perform direct attack.
*/
bool Attack::ParallelAttack( AttackData* atk, double TimeStep )
{ //Attack::ParallelAttack
  CallLevel++;
  if( Verbose > CallLevel )
    printf( "%*sfsxwattk:ParallelAttack:1\n", CallLevel, "" );

  bool    CrewIsActive = true, WriteLine;
  long    NumHullPts, HullPt, TempNum, PointCount;
  double* Hull1, * Hull2;

  double HorizPerimDist, DiagonalDist, SurfDist, LineRate, SuppressionDist;
  double FlankDist, LineDist = 0.0;
  double ratio, PreviousSpreadRatio, CuumSpreadDist;
  double newx, newy;
  long i, posit, FuelType;
  double ReplaceX2n, ReplaceY2n;
  double xdiff, ydiff, ExtendXn, ExtendYn;
  double OriginalTimeStep, NewPt2Dist, OriginalPt2Dist;
  celldata tempcell;
  crowndata tempcrown;
  grounddata tempground;
  attack = atk;

  //Alloc double number of points so that density control can rediscretize.
  if( (Hull2 = new double[GetNumPoints(attack->FireNumber) * 6]) == NULL ) {
    if( Verbose > CallLevel )
      printf( "%*sfsxwattk:ParallelAttack:1a\n", CallLevel, "" );
    CallLevel--;

    return false;
  }
  GetConvexHull( Hull2, &NumHullPts );
  if( (Hull1 = new double[GetNumPoints(attack->FireNumber) * 4]) == NULL ) {
    if( Hull2 ) delete[] Hull2;

    if( Verbose > CallLevel )
      printf( "%*sfsxwattk:ParallelAttack:1b\n", CallLevel, "" );
    CallLevel--;

    return false;
  }
  ExpandConvexHulls( Hull1, Hull2, NumHullPts );
  if( ! attack->Burnout ) HullDensityControl( Hull2, Hull1, &NumHullPts );
  FindHullPoint( Hull2, NumHullPts, &HullPt );
  if( ! AllocParallelLine(Hull1, NumHullPts, TimeStep) ) {
    if( Hull1 ) delete[] Hull1;
    if( Hull2 ) delete[] Hull2;

    if( Verbose > CallLevel )
      printf( "%*sfsxwattk:ParallelAttack:1c\n", CallLevel, "" );
    CallLevel--;

    return false;
  }

  if( attack->Burnout && attack->BurnLine[0] == -1 ) {
    if( attack->NumPoints > 1 ) attack->BurnLine[0] = attack->NumPoints - 1;
    else attack->BurnLine[0] = 0;
  }
  PreviousSpreadRatio = CuumSpreadDist = 0.0;
  xpt1 = attack->IndirectLine2[0];
  ypt1 = attack->IndirectLine2[1];
  fli1n = fli1 = Hull2[attack->CurrentPoint * 3 + 2];
  OriginalTimeStep = TimeStep;
  while( TimeStep > 0.0 ) {
    if( attack->Reverse ) {
      attack->NextPoint = attack->CurrentPoint - 1;
      if( attack->NextPoint < 0 ) attack->NextPoint = NumHullPts - 1;
    }
    else {
      attack->NextPoint = attack->CurrentPoint + 1;
      if( attack->NextPoint > NumHullPts - 1 ) attack->NextPoint = 0;
    }
    xpt2 = Hull1[attack->NextPoint * 2];
    ypt2 = Hull1[attack->NextPoint * 2 + 1];
    fli2 = Hull2[attack->NextPoint * 3 + 2];
    if( fli2 <= 0.0 ) {  //Can't suppress extinguished fire
      for( i = 0; i < NumHullPts; i++ ) {
        fli2n = Hull2[i * 3 + 2];
        if( fli2n > 0.0 ) break;
      }
      if( i == NumHullPts ) {  //Searched fire and no unburned points found
        CrewIsActive = false;
        OriginalTimeStep -= TimeStep;  //Set correct time elapsed

        break;
      }
    }
    xpt2n = Hull2[attack->NextPoint * 3];
    ypt2n = Hull2[attack->NextPoint * 3 + 1];
    fli2n = Hull2[attack->NextPoint * 3 + 2];

    if( OriginalTimeStep == TimeStep ) {  //First time through
      xdiff = xpt2 - xpt2n;
      ydiff = ypt2 - ypt2n;
      xpt1n = xpt1 - xdiff;
      ypt1n = ypt1 - ydiff;
    }

    OriginalPt2Dist = sqrt( pow2(xpt2 - xpt2n) + pow2(ypt2 - ypt2n) );
    if( PreviousSpreadRatio > 0.0 ) {  //Form new pro-rated xpt2
      CuumSpreadDist = OriginalPt2Dist * PreviousSpreadRatio;
      xpt2 = xpt2 - (xpt2 - xpt2n) * PreviousSpreadRatio;
      ypt2 = ypt2 - (ypt2 - ypt2n) * PreviousSpreadRatio;
      PreviousSpreadRatio = 0.0;
    }
    CellData( xpt1, ypt1, tempcell, tempcrown, tempground, &posit );
    FuelType = GetFuelConversion( tempcell.f );
    ros2 = GetPerimeter2Value( labs((long)fli2), (long)ROSVAL );
    if( FuelType > 0 ) {
      LineRate = crew[attack->CrewNum]->LineProduction[FuelType - 1] *
                 MetricResolutionConvert();
      if( ros2 == 0.0 ) {   //Create temporary false point for calcs
        xpt2n = xpt2 - (xpt1 - xpt1n);
        ypt2n = ypt2 - (ypt1 - ypt1n);
      }
    }
    else {
      attack->CurrentPoint = attack->NextPoint;
      xdiff = xpt1 - xpt2;
      ydiff = ypt1 - ypt2;
      if( pow2(xdiff) + pow2(ydiff) > 0.0 ) {
        WriteLine = true;
        if( attack->NumPoints >= NumInsertPoints )
          WriteLine =
                     AllocParallelLine( Hull1, NumHullPts, OriginalTimeStep );
        if( WriteLine ) {
          attack->IndirectLine1[attack->NumPoints * 2] = xpt2;
          attack->IndirectLine1[attack->NumPoints * 2 + 1] = ypt2;
          attack->NumPoints++;
        }
      }
      xpt1 = xpt2;
      ypt1 = ypt2;
      xpt1n = xpt2n;
      ypt1n = ypt2n;
      ros1 = ros2;
      ros1n = ros2n;
      fli1 = fli2;
      fli1n = fli2n;

      continue;
    }
    if( LineDist < 1e-9 ) {
      LineDist = LineRate * TimeStep;
      //Don't prosecute attack with such small line construction.
      if( LineDist < 0.001 ) {
        if( Hull1 ) delete[] Hull1;
        if( Hull2 ) delete[] Hull1;

        if( Verbose > CallLevel )
          printf( "%*sfsxwattk:ParallelAttack:1d\n", CallLevel, "" );
        CallLevel--;

        return true;
      }
      attack->LineBuilt += ( LineDist / MetricResolutionConvert() );
    }
    DiagonalDist = sqrt( pow2(xpt1 - xpt2n) + pow2(ypt1 - ypt2n) );
    HorizPerimDist = sqrt( pow2(xpt1 - xpt2) + pow2(ypt1 - ypt2) );
    ConvertLandData( tempcell.s, tempcell.a );
    SurfDist = CalculateSlopeDist( xpt1, ypt1, xpt2n, ypt2n );
    LineDist *= DiagonalDist / SurfDist;
    CalcChordArcRatio( xpt2n, ypt2n );
    LineDist *= ChordArcRatio;
    FlankDist = sqrt( pow2(xpt1n - xpt1) + pow2(ypt1n - ypt1) );

    if( fli2 <= 0.0 ) {  //On extinguished point
      if( SurfDist - LineDist > 1e-3 ) {
        xpt2n = xpt1 - ( xpt1 - xpt2n ) * LineDist / SurfDist;
        ypt2n = ypt1 - ( ypt1 - ypt2n ) * LineDist / SurfDist;
        TimeStep = 0.0;
      }
      else {
        TimeStep *= ( 1.0 - SurfDist / LineDist );
        LineDist -= SurfDist;
        attack->CurrentPoint = attack->NextPoint;
        if( TimeStep < 1e-6 ) TimeStep = 0.0;
      }
      xdiff = xpt1 - xpt2n;
      ydiff = ypt1 - ypt2n;
      if( pow2(xdiff) + pow2(ydiff) > 0.0 ) {
        WriteLine = true;
        if( attack->NumPoints >= NumInsertPoints )
          WriteLine = AllocParallelLine( Hull1, NumHullPts,
                                         OriginalTimeStep );
        if( WriteLine ) {
          attack->IndirectLine1[attack->NumPoints * 2] = xpt2n;
          attack->IndirectLine1[attack->NumPoints * 2 + 1] = ypt2n;
          attack->IndirectLine2[0] = xpt2n;
          attack->IndirectLine2[1] = ypt2n;
          attack->NumPoints++;
        }
      }
      xpt1 = xpt2n;
      ypt1 = ypt2n;
      xpt1n = xpt2n;
      ypt1n = ypt2n;
      ros1 = ros2;
      ros1n = ros2n;
      fli1 = fli2;
      fli1n = fli2n;

      continue;
    }

    if( (posit = ProblemQuad()) > 0 ) {
      if( posit == 1 ) {
        HorizPerimDist = sqrt( pow2(xpt1 - xpt2n) + pow2(ypt1 - ypt2n) );
        if( HorizPerimDist > 0.0 ) {
          if( LineDist >= HorizPerimDist ) {
            TimeStep = 0.0;
            ReplaceX2n = newx = xpt2n;
            ReplaceY2n = newy = ypt2n;
          }
          else if( LineDist < HorizPerimDist ) {
            newx = xpt1 - ( xpt1 - xpt2n ) * LineDist / HorizPerimDist;
            newy = ypt1 - ( ypt1 - ypt2n ) * LineDist / HorizPerimDist;
            TimeStep = 0.0;
          }
        }
        else {
          newx = xpt2n;
          newy = ypt2n;
          TimeStep = 0.0;
        }
        ReplaceX2n = newx;
        ReplaceY2n = newy;
      }
      else {
        TimeStep = 0.0;
        ReplaceX2n = newx = xpt2n;
        ReplaceY2n = newy = ypt2n;
      }
    }
    else if( LineDist < DiagonalDist ) {
      if( xpt1 == xpt1n && ypt1 == ypt1n ) {
        xpt1n = xpt1 - ( xpt2 - xpt2n );  //Form new pt1n from dims of pt2n
        ypt1n = ypt1 - ( ypt2 - ypt2n );
      }
      IterateIntersection( LineDist, xpt2n, ypt2n, xpt1, ypt1, xpt1n, ypt1n,
                           &newx, &newy );
      ReplaceX2n = newx;
      ReplaceY2n = newy;
      TimeStep = 0.0;
    }
    else if( LineDist > DiagonalDist ) {
      if( LineDist > HorizPerimDist ) ratio = LineDist * 2.0;
      //Else extend linedist out from x1,y1 past x2,y2 by double LineDist.
      else ratio = HorizPerimDist * 2.0;

      ExtendXn = xpt1n - ( xpt1n - xpt2n ) * ( ratio / HorizPerimDist );
      ExtendYn = ypt1n - ( ypt1n - ypt2n ) * ( ratio / HorizPerimDist );

      if( IterateIntersection(LineDist, ExtendXn, ExtendYn, xpt1, ypt1, xpt2n,
                              ypt2n, &newx, &newy) ) {
        if( Cross(newx, newy, xpt1, ypt1, xpt2, ypt2, xpt2n, ypt2n,
                  &ReplaceX2n, &ReplaceY2n) ) {
          newx = ReplaceX2n;
          newy = ReplaceY2n;
        }
        else {
          ReplaceX2n = newx = xpt2n;
          ReplaceY2n = newy = ypt2n;
        }
      }
      else {
        ReplaceX2n = newx;
        ReplaceY2n = newy;
      }
      SuppressionDist = sqrt( pow2(ReplaceX2n - xpt1) +
                              pow2(ReplaceY2n - ypt1) );
      if( SuppressionDist > LineDist ) SuppressionDist = LineDist;
      TimeStep *= ( 1.0 - SuppressionDist / LineDist );
      LineDist -= SuppressionDist;
    }
    else {  //Equal
      ReplaceX2n = newx = xpt2n;
      ReplaceY2n = newy = ypt2n;
      TimeStep = 0.0;
    }
    xdiff = xpt1 - ReplaceX2n;
    ydiff = ypt1 - ReplaceY2n;
    SuppressionDist = sqrt( pow2(xdiff) + pow2(ydiff) );
    if( SuppressionDist >= FlankDist / 2.0 ) {
      WriteLine = true;
      if( attack->NumPoints >= NumInsertPoints )
        WriteLine = AllocParallelLine( Hull1, NumHullPts, OriginalTimeStep );
      if( WriteLine ) {
        attack->IndirectLine1[attack->NumPoints * 2] = ReplaceX2n;
        attack->IndirectLine1[attack->NumPoints * 2 + 1] = ReplaceY2n;
        attack->NumPoints++;
      }
    }
    if( TimeStep > 0.0 ) {
      attack->CurrentPoint = attack->NextPoint;
      NewPt2Dist = sqrt( pow2(newx - xpt2) + pow2(newy - ypt2) );
      if( OriginalPt2Dist < 1e-9 ) PreviousSpreadRatio = 1.0;
      else {
        CuumSpreadDist += NewPt2Dist;
        PreviousSpreadRatio = CuumSpreadDist / OriginalPt2Dist;
        if( PreviousSpreadRatio > 1.0 ) PreviousSpreadRatio = 1.0;
      }
      xpt1n = xpt2n;
      ypt1n = ypt2n;
      ros1 = ros2;
      ros1n = ros2n;
      fli1 = fli2;
      fli1n = fli2n;
    }
    xpt1 = attack->IndirectLine2[0] = newx;
    ypt1 = attack->IndirectLine2[1] = newy;
  }
  if( attack->NumPoints > 1 ) {
    vectorbarrier.AllocBarrier( attack->NumPoints );
    for( PointCount = 0; PointCount < attack->NumPoints; PointCount++) {
      vectorbarrier.SetBarrierVertex( PointCount,
                                      attack->IndirectLine1[PointCount * 2],
                                  attack->IndirectLine1[PointCount * 2 + 1] );
    }
    vectorbarrier.BufferBarrier( 1.2 );
    if( attack->LineNumber <= -1 ) {
      attack->LineNumber = GetNewFires();
      IncNewFires( 1 );
    }
    vectorbarrier.TransferBarrier( attack->LineNumber );
    TempNum = attack->FireNumber;
    attack->FireNumber = attack->LineNumber;
    BoundingBox();
    attack->FireNumber = TempNum;
    //--------------------------------------------------------------
    //Burnout Now Conducted in FarProc3 like Indirect Attack
    //--------------------------------------------------------------

    if( attack->Burnout ) {
      attack->BurnLine[1] = attack->NumPoints - 1;
      if( ! CrewIsActive ) {
        if( attack->BurnLine[1] > attack->BurnLine[0] ) {
          attack->BurnDelay = 0;
        }
      }
    }
  }
  if( ! CrewIsActive ) {
    if( attack->Burnout ) {
      attack->BurnLine[1] = attack->NumPoints - 1;
      BurnOut();
    }
  }
  attack->AttackTime += OriginalTimeStep;
  //Save attack->CurrentPoint from fire perimeter so that it can be located on
  //revision.
  attack->CurrentPoint = attack->NumPoints - 1;
  if( Hull1 ) delete[] Hull1;
  if( Hull2 ) delete[] Hull2;

  if( Verbose > CallLevel )
    printf( "%*sfsxwattk:ParallelAttack:2\n", CallLevel, "" );
  CallLevel--;

  return CrewIsActive;
} //Attack::ParallelAttack

//============================================================================
void Attack::ConvertLandData( short slope, short aspect )
{ //Attack::ConvertLandData
  ld.SlopeConvert( slope );
  ld.AspectConvert( aspect );
} //Attack::ConvertLandData

/*============================================================================
  Attack::CalculateSlopeDist
  Calculates horizontal distance from slope, aspect and coords of 2 pts.
*/
double Attack::CalculateSlopeDist( double x1, double y1, double x2,
                                   double y2 )
{ //Attack::CalculateSlopeDist
  double Theta;
  double Angle;
  double hx, hy, ds;
  double Mx = -1.0, My = -1.0;
  double slopef = ( (double) ld.ld.slope / 180.0 ) * PI;
  double xt, yt, HorizDist;

  xt = x1 - x2;
  yt = y1 - y2;
  HorizDist = sqrt( pow2(xt) + pow2(yt) );

  if( ld.ld.slope == 0 || HorizDist < 1e-9 ) return HorizDist;

  //Calculate angles on horizontal coordinate system.
  Angle = atan2( -xt, -yt );
  if( Angle < 0.0 ) Angle += 2.0 * PI;
  Theta = ld.ld.aspectf - Angle;

  //Calculate angles on local surface coordinate system.
  Theta = -atan2( cos(Theta) / cos(slopef), sin(Theta) ) + PI / 2.0;

  //Calculate distance difference.
  ds = HorizDist * cos( Theta ) * ( 1.0 - cos(slopef) );

  //If quadrants 1 and 3.
  if( (ld.ld.aspectf <= PI / 2.0) ||
      (ld.ld.aspectf > PI && ld.ld.aspectf <= 3.0 * PI / 2.0) ) {
    if( cos(Angle) != 0.0 || Angle == PI ) {
      if( tan(Angle) < 0.0 ) {
        if( tan(Angle) < tan(ld.ld.aspectf - PI / 2.0) ) My = 1.0;
        else Mx = 1.0;
      }
    }
  }
  else {  //If quadrants 2 and 4
    if( cos(Angle) != 0.0 ) {
      if( tan(Angle) > 0.0 && Angle != PI ) {
        if( tan(Angle) > tan(ld.ld.aspectf - PI / 2.0) ) My = 1.0;
        else Mx = 1.0;
      }
    }
  }

  //Transform x and y components to local surface coordinates.
  hx = fabs( xt ) - Mx * fabs( ds * sin(ld.ld.aspectf) );
  hy = fabs( yt ) - My * fabs( ds * cos(ld.ld.aspectf) );

  return sqrt( pow2(hx) + pow2(hy) );
} //Attack::CalculateSlopeDist

//============================================================================
void Attack::BoundingBox()
{ //Attack::BoundingBox
  double xpt, ypt, Xlo, Xhi, Ylo, Yhi;
  long   NumFire = attack->FireNumber;
  long   NumPoint = GetNumPoints( NumFire );

  Xlo = Xhi = GetPerimeter1Value( NumFire, 0, XCOORD );
  Ylo = Yhi = GetPerimeter1Value( NumFire, 0, YCOORD );
  for( int i = 1; i < NumPoint; i++ ) {
    xpt = GetPerimeter1Value( NumFire, i, XCOORD );
    ypt = GetPerimeter1Value( NumFire, i, YCOORD );
    if( xpt < Xlo ) Xlo = xpt;
    if( xpt > Xhi ) Xhi = xpt;
    if( ypt < Ylo ) Ylo = ypt;
    if( ypt > Yhi ) Yhi = ypt;
  }
  SetPerimeter1( NumFire, NumPoint, Xlo, Xhi );
  SetFireChx( NumFire, NumPoint, Ylo, Yhi );
} //Attack::BoundingBox

//============================================================================
void Attack::InsertPerimeterPoint( double newx, double newy, double nros,
                                   double nfli, double nrcx, long InsertType )
{ //Attack::InsertPerimeterPoint
  long   i, j, TestPoint;
  long   numpts = GetNumPoints( attack->FireNumber );
  long   numfire = attack->FireNumber;
  double xpt, ypt, ros, fli, rcx;
  double ProportionalDist, x2, y2;

  if( nfli == 0.0 ) nfli = -0.001;
  if( attack->Reverse ) TestPoint = attack->CurrentPoint;
  else TestPoint = attack->NextPoint;
  j = 0;
  attack->IndirectLine2 = new double[(numpts + 1) * NUMDATA];
  switch ( InsertType ) {
    case 1:
      ProportionalDist = sqrt( pow2(newx - xpt1n) + pow2(newy - ypt1n) ) /
                         sqrt( pow2(xpt2n - xpt1n) + pow2(ypt2n - ypt1n) );
      x2 = xpt1 - ( xpt1 - xpt2 ) * ProportionalDist;
      y2 = ypt1 - ( ypt1 - ypt2 ) * ProportionalDist;
      break;
    case 2:
      x2 = xpt1;
      y2 = ypt1;
      break;
    case 3:
      x2 = xpt1;
      y2 = ypt1;
      break;
  }
  for( i = 0; i < numpts; i++ ) {
    GetPerimeter2( i, &xpt, &ypt, &ros, &fli, &rcx );
    if( i == TestPoint ) {
      attack->IndirectLine2[i * 4] = x2;
      attack->IndirectLine2[i * 4 + 1] = y2;
      attack->IndirectLine2[i * 4 + 2] = ros;
      attack->IndirectLine2[i * 4 + 3] = fabs( fli );
      attack->IndirectLine2[i * 4 + 4] = rcx;
      j = 1;
    }
    attack->IndirectLine2[(i + j) * 4] = xpt;
    attack->IndirectLine2[(i + j) * 4 + 1] = ypt;
    attack->IndirectLine2[(i + j) * 4 + 2] = ros;
    attack->IndirectLine2[(i + j) * 4 + 3] = fli;
    attack->IndirectLine2[(i + j) * 4 + 4] = rcx;
  }
  if( j == 0 ) {
    attack->IndirectLine2[i * 4] = x2;
    attack->IndirectLine2[i * 4 + 1] = y2;
    attack->IndirectLine2[i * 4 + 2] = ros;
    attack->IndirectLine2[i * 4 + 3] = fabs( fli );
    attack->IndirectLine2[i * 4 + 4] = rcx;
  }

  numpts++;
  AllocPerimeter2( numpts );
  for( i = 0; i < numpts; i++ ) {
    xpt = attack->IndirectLine2[i * 4];
    ypt = attack->IndirectLine2[i * 4 + 1];
    ros = attack->IndirectLine2[i * 4 + 2];
    fli = attack->IndirectLine2[i * 4 + 3];
    rcx = attack->IndirectLine2[i * 4 + 4];
    SetPerimeter2( i, xpt, ypt, ros, fli, rcx );
  }
  j = 0;
  numpts--;
  for( i = 0; i < numpts; i++ ) {
    if( i == TestPoint ) {
      attack->IndirectLine2[i * 4] = newx;
      attack->IndirectLine2[i * 4 + 1] = newy;
      attack->IndirectLine2[i * 4 + 2] = nros;
      attack->IndirectLine2[i * 4 + 3] = -fabs( nfli );  //Always negative
      attack->IndirectLine2[i * 4 + 4] = nfli;  //Always negative
      j = 1;
    }
    attack->IndirectLine2[(i + j) * 4] =
                                     GetPerimeter1Value( numfire, i, XCOORD );
    attack->IndirectLine2[(i + j) * 4 + 1] =
                                     GetPerimeter1Value( numfire, i, YCOORD );
    attack->IndirectLine2[(i + j) * 4 + 2] =
                                     GetPerimeter1Value( numfire, i, ROSVAL );
    attack->IndirectLine2[(i + j) * 4 + 3] =
                                     GetPerimeter1Value( numfire, i, FLIVAL );
    attack->IndirectLine2[(i + j) * 4 + 4] =
                                     GetPerimeter1Value( numfire, i, RCXVAL );
  }
  if( j == 0 ) {
    attack->IndirectLine2[i * 4] = newx;
    attack->IndirectLine2[i * 4 + 1] = newy;
    attack->IndirectLine2[i * 4 + 2] = nros;
    attack->IndirectLine2[i * 4 + 3] = -fabs( nfli );
    attack->IndirectLine2[i * 4 + 4] = nrcx;
  }
  numpts++;
  AllocPerimeter1( numfire, numpts + 1 );
  SetNumPoints( numfire, numpts );
  for( i = 0; i < numpts; i++ ) {
    xpt = attack->IndirectLine2[i * 4];
    ypt = attack->IndirectLine2[i * 4 + 1];
    ros = attack->IndirectLine2[i * 4 + 2];
    fli = attack->IndirectLine2[i * 4 + 3];
    rcx = attack->IndirectLine2[i * 4 + 4];
    SetPerimeter1( numfire, i, xpt, ypt );
    SetFireChx( numfire, i, ros, fli );
    SetReact( numfire, i, rcx );
  }
  delete[] attack->IndirectLine2;
  attack->IndirectLine2 = 0;
} //Attack::InsertPerimeterPoint

//============================================================================
bool Attack::IterateIntersection( double LineDist, double extendx,
                                  double extendy, double xstart,
                                  double ystart, double xend, double yend,
                                  double* newx, double* newy )
{ //Attack::IterateIntersection
  bool   NeedNewPoint = true;
  double CloseDist = sqrt( pow2(extendx - xstart) + pow2(extendy - ystart) );
  double TotalDist = sqrt( pow2(extendx - xend) + pow2(extendy - yend) );
  double StartDist = sqrt( pow2(xstart - xend) + pow2(ystart - yend) );
  double NewTotalDist;
  double SlideDist;
  double NewLineDist = LineDist;
  double oldx, oldy, inc;
  double Tolerance;
  long NumIter = 0;

  Tolerance = fabs( CloseDist - NewLineDist ) / 10.0;
  if( Tolerance > 0.1 ) Tolerance = 0.1;
  else if( Tolerance < 1e-9 ) Tolerance = 0.1;

  oldx = extendx;
  oldy = extendy;
  inc = TotalDist / 4.0;
  NewTotalDist = TotalDist - inc * 2.0;
  //Reconvert line distance to slopeing distance.
  LineDist *= CalculateSlopeDist( xstart, ystart, extendx, extendy ) /
                                                                    CloseDist;
  while( fabs(CloseDist - NewLineDist) > Tolerance ) {
    NumIter++;
    *newx = oldx - ( oldx - xend ) * ( 1.0 - NewTotalDist / TotalDist );
    *newy = oldy - ( oldy - yend ) * ( 1.0 - NewTotalDist / TotalDist );
    CloseDist = sqrt( pow2(*newx - xstart) + pow2(*newy - ystart) );
    SlideDist = sqrt( pow2(*newx - xend) + pow2(*newy - yend) );
    NewLineDist = LineDist * CloseDist /
                           CalculateSlopeDist( xstart, ystart, *newx, *newy );
    if( CloseDist == NewLineDist ) break;
    else {
      if( CloseDist < NewLineDist ) {
        NewTotalDist += inc;
        *newx = oldx;
        *newy = oldy;
      }
      else {
        NewTotalDist -= inc;
        oldx = *newx;
        oldy = *newy;
        inc /= 2.0;
      }
    }
    if( SlideDist < Tolerance ) {
      if( NewLineDist <= StartDist ) {
        *newx = xend - ( xend - xstart ) * ( 1.0 - LineDist / StartDist );
        *newy = yend - ( yend - ystart ) * ( 1.0 - LineDist / StartDist );
        NeedNewPoint = false;
        break;
      }
    }
    if( inc < 0.001 ) {
      *newx = xend;
      *newy = yend;
      NeedNewPoint = false;
      break;
    }
  }

  //Compute *newx,*newy if fails condition in while loop 1st time.
  if( NumIter == 0 ) {
    *newx = oldx - ( oldx - xend ) * ( 1.0 - NewTotalDist / TotalDist );
    *newy = oldy - ( oldy - yend ) * ( 1.0 - NewTotalDist / TotalDist );
  }

  return NeedNewPoint;
} //Attack::IterateIntersection

//============================================================================
void Attack::ConductBurnout( AttackData* atk )
{ //Attack::ConductBurnout
  attack = atk;

  if( attack->Burnout ) {
    if( attack->BurnLine[0] == -1 ) {
      if( attack->NumPoints > 1 ) attack->BurnLine[0] = attack->NumPoints - 1;
      else attack->BurnLine[0] = 0;
    }
    attack->BurnLine[1] = attack->NumPoints - 1;
    BurnOut();
  }
} //Attack::ConductBurnout

//============================================================================
void Attack::BurnOut()
{ //Attack::BurnOut
  long   i, j, k;
  long   NewFire, NewFirstPoint;
  long   LagPoint, NewPoints;
  long   start = attack->BurnLine[0];
  long   end = attack->BurnLine[1];
  double Delay;
  double xpt, ypt, xptl, yptl, xptn, yptn;
  double xbuf1, ybuf1;
  double xbuf2, ybuf2;
  double DistRes, OldOffset = LineOffset;

  Delay = (double) attack->BurnDelay * MetricResolutionConvert();
  if( end < 0 ) return;

  do {
    //Largest is 2.0m, smallest is 0.5.
    LineOffset = ( (double) (rand() % 22490) + 7500.0 ) / 15000.0;
  } while( fabs(LineOffset - OldOffset) < 0.5 );

  if( start == end ) start -= 1;
  if( start < 0 ) start = 0;
  NewFirstPoint = end;

  i = 1;
  LagPoint = 0;
  xpt2 = xpt2n = attack->IndirectLine1[end * 2];
  ypt2 = ypt2n = attack->IndirectLine1[end * 2 + 1];
  do {
    xpt1 = attack->IndirectLine1[(end - i) * 2];
    ypt1 = attack->IndirectLine1[(end - i) * 2 + 1];
    LineRate = sqrt( pow2(xpt2n - xpt1) + pow2(ypt2n - ypt1) );
    if( LineRate > Delay ) {
      xpt2n = xpt2n - ( xpt2n - xpt1 ) * Delay / LineRate;
      ypt2n = ypt2n - ( ypt2n - ypt1 ) * Delay / LineRate;
      LagPoint = i;
      NewFirstPoint = end - i - 1;
      if( NewFirstPoint < 0 ) NewFirstPoint = 0;
      break;
    }
    else {
      Delay -= LineRate;
      xpt2n = xpt1;
      ypt2n = ypt1;
    }
    i++;
  } while( (end - i) >= start );

  if( Delay > LineRate ) return;
  end -= LagPoint;
  if( end <= start ) return;

  /* start */
  long   AddPoints = 0;
  double TotalDist, IncDist, CuumDist;
  //Make a little finer.
  DistRes = GetDistRes() / 1.2 * MetricResolutionConvert();
  TotalDist = 0.0;
  xpt = xptl = attack->IndirectLine1[start * 2];
  ypt = yptl = attack->IndirectLine1[start * 2 + 1];
  if( start > 0 ) {
    xpt = attack->IndirectLine1[(start + 1) * 2];
    ypt = attack->IndirectLine1[(start + 1) * 2 + 1];
    xpt = xpt - ( xpt - xptl ) * 0.5;
    ypt = ypt - ( ypt - yptl ) * 0.5;
  }
  for( i = start; i <= end; i++ ) {
    if( i < end ) {
      xptn = attack->IndirectLine1[(i + 1) * 2];
      yptn = attack->IndirectLine1[(i + 1) * 2 + 1];
    }
    else if( LagPoint > 0 ) {
      xptn = xpt2n;
      yptn = ypt2n;
    }
    else break;
    CuumDist = sqrt( pow2(xpt - xptn) + pow2(ypt - yptn) );
    TotalDist += CuumDist;
    AddPoints += (long) ( CuumDist / DistRes );
    xpt = xptn;
    ypt = yptn;
  }
  NewPoints = end - start + 1 + AddPoints;
  if( LagPoint > 0 ) NewPoints += 1;

  if( NewPoints < 2 ) return;    //Can't have fewer points than 2
  /* end */

  xpt = xptl;
  ypt = yptl;
  if( start > 0 ) {
    xpt = attack->IndirectLine1[(start + 1) * 2];
    ypt = attack->IndirectLine1[(start + 1) * 2 + 1];
    xpt = xpt - ( xpt - xptl ) * 0.5;
    ypt = ypt - ( ypt - yptl ) * 0.5;
  }

  AllocPerimeter1( GetNewFires(), 2 * NewPoints + 1 );
  k = 0;
  for( i = start; i <= end + 1; i++ ) {
    j = i + 1;
    if( j <= end ) {
      xptn = attack->IndirectLine1[j * 2];
      yptn = attack->IndirectLine1[j * 2 + 1];
    }
    else if( LagPoint > 0 ) {
      xptn = xpt2n;
      yptn = ypt2n;
    }
    else break;

    BufferBurnout( xptl, yptl, xpt, ypt, xptn, yptn, &xbuf1, &ybuf1, &xbuf2,
                   &ybuf2 );
    if( attack->BurnDirection == 1 ) {
      SetPerimeter1( GetNewFires(), 2 * NewPoints - 1 - k, xbuf2, ybuf2 );
      SetFireChx( GetNewFires(), 2 * NewPoints - 1 - k, 0.0, -1.0 );
      SetPerimeter1( GetNewFires(), k, xbuf1, ybuf1 );
      SetFireChx( GetNewFires(), k, 0.0, 0.0 );
    }
    else {
      SetPerimeter1( GetNewFires(), 2 * NewPoints - 1 - k, xbuf1, ybuf1 );
      SetFireChx( GetNewFires(), 2 * NewPoints - 1 - k, 0.0, 0.0 );
      SetPerimeter1( GetNewFires(), k, xbuf2, ybuf2 );
      SetFireChx( GetNewFires(), k, 0.0, -1.0 );
    }

    k++;
    xptl = xpt;
    yptl = ypt;

    IncDist = pow2( xpt - xptn ) + pow2( ypt - yptn );
    if( IncDist > pow2(DistRes) ) {
      IncDist = sqrt( IncDist );
      xpt = xpt - ( xpt - xptn ) * DistRes / IncDist;
      ypt = ypt - ( ypt - yptn ) * DistRes / IncDist;
      i--;
    }
    else {
      xpt = xptn;
      ypt = yptn;
      if( LagPoint == 0 ) i++;
    }
  }

  if( k < NewPoints ) {  //No
    return;
  }
  SetNumPoints( GetNewFires(), 2 * k );
  SetInout( GetNewFires(), 1 );
  NewFire = attack->FireNumber;      //Save to restore after BoundingBox
  attack->FireNumber = GetNewFires();
  IncNewFires( 1 );
  BoundingBox();
  attack->FireNumber = NewFire;
  attack->BurnLine[0] = NewFirstPoint;
} //Attack::BurnOut

//============================================================================
void Attack::BufferBurnout( double xptl, double yptl, double xpt, double ypt,
                            double xptn, double yptn, double* xbuf1,
                            double* ybuf1, double* xbuf2, double* ybuf2 )
{ //Attack::BufferBurnout
  double xdiffl, xdiffn, ydiffl, ydiffn, tempx, tempy;
  double xdiff, ydiff, dist, distl, distn;
  double Sign;
  double DistanceResolution;
  double A1, A2, A3, DR;

  //Can't reduce width because center.
  DistanceResolution = GetDistRes() * 0.6 * MetricResolutionConvert();
  // of buffer is calculated line.
  if( attack->BurnDirection == 0 )  //Left
    Sign = 1.0;
  else Sign = -1.0;
  xdiffl = xpt - xptl;
  ydiffl = ypt - yptl;
  xdiffn = xpt - xptn;
  ydiffn = ypt - yptn;
  distl = sqrt( xdiffl * xdiffl + ydiffl * ydiffl );
  distn = sqrt( xdiffn * xdiffn + ydiffn * ydiffn );

  if( distl > 0.0 && distn > 0.0 ) {
    if( distl < distn ) {
      tempx = xpt - xdiffn * distl / distn;
      tempy = ypt - ydiffn * distl / distn;
      xdiff = xptl - tempx;
      ydiff = yptl - tempy;
    }
    else {
      if( distn < distl ) {
        tempx = xpt - xdiffl * distn / distl;
        tempy = ypt - ydiffl * distn / distl;
        xdiff = tempx - xptn;
        ydiff = tempy - yptn;
      }
      else {
        xdiff = xptl - xptn;
        ydiff = yptl - yptn;
      }
    }
  }
  else {
    xdiff = xptl - xptn;
    ydiff = yptl - yptn;
  }
  dist = sqrt( xdiff * xdiff + ydiff * ydiff );
  if( dist < 1e-9 ) dist = 1.0;
  if( fabs(xdiffl) < 1e-9 && fabs(ydiffl) < 1e-9 ) A1 = 0.0;
  else A1 = atan2( (xdiffl), (ydiffl) );
  if( fabs(xdiff) < 1e-9 && fabs(ydiff) < 1e-9 ) A2 = 0.0;
  else A2 = atan2( (xdiff), (ydiff) );
  A3 = cos( A1 ) * cos( A2 ) + sin( A1 ) * sin( A2 );
  if( fabs(A3) > 1e-2 && distl > 1e-9 ) DR = DistanceResolution / fabs( A3 );
  else DR = DistanceResolution;
  //Perpendicular to xpt,ypt.
  *xbuf1 = xpt + Sign * ( DR + LineOffset ) / dist * ydiff;
  *ybuf1 = ypt - Sign * ( DR + LineOffset ) / dist * xdiff;
  //Perpendicular to xpt,ypt.
  *xbuf2 = xpt + Sign * LineOffset / dist * ydiff;
  *ybuf2 = ypt - Sign * LineOffset / dist * xdiff;
} //Attack::BufferBurnout

//============================================================================
double Attack::pow2( double input ) { return input * input; }

//============================================================================
bool Attack::Cross( double xpt1, double ypt1, double xpt2, double ypt2,
                    double xpt1n, double ypt1n, double xpt2n, double ypt2n,
                    double* newx, double* newy )
{ //Attack::Cross
  double xdiff1, ydiff1, xdiff2, ydiff2, ycept1, ycept2;
  double slope1, slope2, ycommon, xcommon;
  bool   BadIntersection = false;

  xdiff1 = xpt2 - xpt1;
  ydiff1 = ypt2 - ypt1;
  if( fabs(xdiff1) < 1e-9 ) xdiff1 = 0.0;
  if( xdiff1 != 0.0 ) {
    slope1 = ydiff1 / xdiff1;
    ycept1 = ypt1 - ( slope1 * xpt1 );
  }
  else slope1 = ydiff1;   //SLOPE NON-ZERO
  xdiff2 = xpt2n - xpt1n;
  ydiff2 = ypt2n - ypt1n;
  if( fabs(xdiff2) < 1e-9 ) xdiff2 = 0.0;
  if( xdiff2 != 0.0 ) {
    slope2 = ydiff2 / xdiff2;
    ycept2 = ypt1n - ( slope2 * xpt1n );
  }
  else slope2 = ydiff2;   //SLOPE NON-ZERO
  if( slope1 == slope2 ) { }
	else
	{
		if (xdiff1 != 0.0 && xdiff2 != 0.0)
		{
			xcommon = (ycept2 - ycept1) / (slope1 - slope2);
			*newx = xcommon;
			ycommon = ycept1 + slope1 * xcommon;
			*newy = ycommon;
			if ((*newx > xpt1 && *newx < xpt2) || (*newx<xpt1 && *newx>xpt2))
			{
				if ((*newx > xpt1n && *newx < xpt2n) ||
					(*newx<xpt1n && *newx>xpt2n))
					BadIntersection = true;
			}
		}
		else
		{
			if (xdiff1 == 0.0 && xdiff2 != 0.0)
			{
				ycommon = slope2 * xpt1 + ycept2;
				*newx = xpt1;
				*newy = ycommon;
				if ((*newy > ypt1 && *newy < ypt2) ||
					(*newy<ypt1 && *newy>ypt2))
				{
					if ((*newx > xpt1n && *newx < xpt2n) ||
						(*newx<xpt1n && *newx>xpt2n))
						BadIntersection = true;
				}
			}
			else
			{
				if (xdiff1 != 0.0 && xdiff2 == 0.0)
				{
					ycommon = slope1 * xpt1n + ycept1;
					*newx = xpt1n;
					*newy = ycommon;
					if ((*newy > ypt1n && *newy < ypt2n) ||
						(*newy<ypt1n && *newy>ypt2n))
					{
						if ((*newx > xpt1 && *newx < xpt2) ||
							(*newx<xpt1 && *newx>xpt2))
							BadIntersection = true;
					}
				}
			}
		}
	}

	return BadIntersection;
} //Attack::Cross


long Attack::FindHullPoint(double* Hull, long NumHullPts, long* HullPt)
{
	bool RecordMinDist;
	long i, j, k, AbsoluteMinPoint;
	double dist, distl, distn;
	double AbsoluteMinDist = -1.0, DirectMinDist = -1.0;
	double startx, starty;

	for (i = 0; i < NumHullPts; i++)
	{
		xpt1 = Hull[i * 3];
		ypt1 = Hull[i * 3 + 1];
		if (xpt1 == attack->IndirectLine2[0] &&
			ypt1 == attack->IndirectLine2[1])
		{
			*HullPt = attack->CurrentPoint = i;
			DirectMinDist = 1.0;		// prevent exiting to external part of fire
			break;
		}
		else
		{
			RecordMinDist = true;
			startx = attack->IndirectLine2[0];
			starty = attack->IndirectLine2[1];
			dist = pow2(xpt1 - startx) + pow2(ypt1 - starty);
			if (AbsoluteMinDist < 0.0 || dist < AbsoluteMinDist)
			{
				AbsoluteMinDist = dist;
				AbsoluteMinPoint = i;
			}
			if (DirectMinDist < 0.0 || dist < DirectMinDist)
			{
				startx = startx - (startx - xpt1) * 0.01;	// offset a little so don't intersect dead pts
				starty = starty - (starty - ypt1) * 0.01;
				xpt2 = GetPerimeter2Value(0, XCOORD);
				ypt2 = GetPerimeter2Value(0, YCOORD);
				for (j = 1; j <= GetNumPoints(attack->FireNumber); j++)
				{
					k = j;
					if (j == GetNumPoints(attack->FireNumber))
						k = 0;
					xpt2n = GetPerimeter2Value(k, XCOORD);
					ypt2n = GetPerimeter2Value(k, YCOORD);
					// test to see if crosses fire if so then don't use this point
					if (Cross(startx, starty, xpt1, ypt1, xpt2, ypt2, xpt2n,
							ypt2n, &xpt1n, &ypt1n))
					{
						RecordMinDist = false;

						break;
					}
					xpt2 = xpt2n;
					ypt2 = ypt2n;
				}
				if (RecordMinDist)
				{
					if (DirectMinDist < 0.0 || dist < DirectMinDist)
					{
						DirectMinDist = dist;
						*HullPt = attack->CurrentPoint = i;  // use closest point
					}
				}
			}
		}
	}
	if (DirectMinDist < 0.0)	// if cannot see directly outside
	{
		*HullPt = attack->CurrentPoint = AbsoluteMinPoint;
		attack->NumPoints = 0;
		attack->IndirectLine2[0] = GetPerimeter1Value(attack->FireNumber,
									AbsoluteMinPoint, XCOORD);
		attack->IndirectLine2[1] = GetPerimeter1Value(attack->FireNumber,
									AbsoluteMinPoint, YCOORD);
		attack->IndirectLine2[2] = -1.0;	 // reset indirect line
		attack->BurnLine[0] = -1;
		if (attack->IndirectLine1)
			delete[] attack->IndirectLine1;//free(attack->IndirectLine1);
		attack->IndirectLine1 = 0;
		attack->LineNumber = (long)-1.0;			// set new fire number for barrier
	}

	if (attack->IndirectLine2[2] < 0.0)
	{
		attack->IndirectLine2[2] = 1;
		return attack->CurrentPoint;
	}

	i = attack->CurrentPoint + 1;
	if (i > NumHullPts - 1)
		i = 0;
	xpt1 = Hull[i * 3];
	ypt1 = Hull[i * 3 + 1];
	distn = pow2(xpt1 - attack->IndirectLine2[0]) +
		pow2(ypt1 -
			attack->IndirectLine2[1]);			// dist to point
	distl = pow2(xpt1 - Hull[attack->CurrentPoint * 3]) +
		pow2(ypt1 -
			Hull[attack->CurrentPoint * 3 + 1]);  		// total dist
	if (distn < distl)  								 	// on this segment
	{
		if (attack->Reverse)
			*HullPt = attack->CurrentPoint = i;
	}
	else if (distn > distl)
	{
		//if(!attack->Reverse)
		//{	attack->CurrentPoint-=1;
		//	if(attack->CurrentPoint<0)
		//     	attack->CurrentPoint+=NumHullPts;
		*HullPt = attack->CurrentPoint;

		//}
	}

	return attack->CurrentPoint;
}


bool Attack::AllocParallelLine(double* Hull, long NumHullPts, double TimeStep)
{
	long i, j, NewNumPts;
	double* TempLine = 0;
	double Rate, MaxRate = -1.0, MaxDist;
	double xptl, yptl, xpt, ypt;

	// determine maximum line prod rate for dimensioning array
	for (i = 0; i < 51; i++)
	{
		Rate = crew[attack->CrewNum]->LineProduction[i];
		if (Rate > MaxRate)
			MaxRate = Rate;
	}

	// calcualate distance along Hull line
	MaxDist = pow2(TimeStep * MaxRate * MetricResolutionConvert());  // maximum horizontal distance of line production squared
	Rate = 0.0;
	xptl = Hull[attack->CurrentPoint * 2];
	yptl = Hull[attack->CurrentPoint * 2 + 1];
	for (i = 1; i < NumHullPts; i++)
	{
		if (attack->Reverse)
		{
			j = attack->CurrentPoint - i;
			if (j < 0)
				j += NumHullPts;
		}
		else
		{
			j = attack->CurrentPoint + i;
			if (j > NumHullPts - 1)
				j -= NumHullPts;
		}
		xpt = Hull[j * 2];
		ypt = Hull[j * 2 + 1];
		Rate += pow2(xptl - xpt) + pow2(yptl - ypt);
		xptl = xpt;
		yptl = ypt;
		if (Rate > MaxDist)
			break;
	}
	NewNumPts = attack->NumPoints + i + 4;

	// transfer and save existing parallel line points
	if (attack->NumPoints > 0 && attack->IndirectLine1)
	{
		if ((TempLine = new double[attack->NumPoints * 2]) != NULL)//(double *) calloc(attack->NumPoints*2, sizeof(double)))!=NULL)
		{
			for (i = 0; i < attack->NumPoints; i++)
			{
				TempLine[i * 2] = attack->IndirectLine1[i * 2];
				TempLine[i * 2 + 1] = attack->IndirectLine1[i * 2 + 1];
			}
			delete[] attack->IndirectLine1;//free(attack->IndirectLine1);
			attack->IndirectLine1 = 0;
		}
		else
			return false;
	}

	// Reallocate IndirectLine2 and transfer existing points back from TempLine
	if ((attack->IndirectLine1 = new double[NewNumPts * 2]) != NULL)//(double *) calloc(NewNumPts*2, sizeof(double)))!=NULL)
	{
		NumInsertPoints = NewNumPts;			  // record number of points
		for (i = 0; i < attack->NumPoints; i++)
		{
			attack->IndirectLine1[i * 2] = TempLine[i * 2];
			attack->IndirectLine1[i * 2 + 1] = TempLine[i * 2 + 1];
		}
		if (TempLine)
			delete[] TempLine;//free(TempLine);
		if (attack->NumPoints == 0)
		{
			attack->IndirectLine1[0] = attack->IndirectLine2[0] = Hull[attack->CurrentPoint * 2]; //attack->IndirectLine2[0];
			attack->IndirectLine1[1] = attack->IndirectLine2[1] = Hull[attack->CurrentPoint * 2 + 1]; //attack->IndirectLine2[1];
			attack->NumPoints++;
		}
	}
	else
		return false;

	return true;
}


void Attack::GetConvexHull(double* Hull, long* NewHullPts)
{
	long i;
	long FireNum = attack->FireNumber;
	long NumPoints = GetNumPoints(attack->FireNumber);
	long NumHullPts;
	long NorthPt, SouthPt, EastPt, WestPt;
	double xpt, ypt, Xlo, Xhi, Ylo, Yhi;

	Xlo = Xhi = GetPerimeter1Value(FireNum, 0, XCOORD);
	Ylo = Yhi = GetPerimeter1Value(FireNum, 0, YCOORD);
	NorthPt = 0;
	SouthPt = 0;
	EastPt = 0;
	WestPt = 0;
	for (i = 1; i < NumPoints; i++)
	{
		xpt = GetPerimeter1Value(FireNum, i, XCOORD);
		ypt = GetPerimeter1Value(FireNum, i, YCOORD);
		if (xpt < Xlo)
		{
			Xlo = xpt;
			WestPt = i;
		}
		if (xpt > Xhi)
		{
			Xhi = xpt;
			EastPt = i;
		}
		if (ypt < Ylo)
		{
			Ylo = ypt;
			SouthPt = i;
		}
		if (ypt > Yhi)
		{
			Yhi = ypt;
			NorthPt = i;
		}
	}
	SetPerimeter1(FireNum, NumPoints, Xlo, Xhi);
	SetFireChx(FireNum, NumPoints, Ylo, Yhi);

	NumHullPts = 0;
	FindHullQuadrant(Hull, &NumHullPts, WestPt, SouthPt, PI);
	FindHullQuadrant(Hull, &NumHullPts, SouthPt, EastPt, PI / 2.0);
	FindHullQuadrant(Hull, &NumHullPts, EastPt, NorthPt, 0.0);
	FindHullQuadrant(Hull, &NumHullPts, NorthPt, WestPt, 3.0 * PI / 2.0);
	*NewHullPts = NumHullPts;
}


void Attack::FindHullQuadrant(double* Hull, long* NumPts, long StartPt,
	long EndPt, double RefAngle)
{
	long i, j, Mult, ConvexStartPt, ConvexEndPt;
	long ConPt, ConOrder;
	long FireNum = attack->FireNumber;
	long NumPoints = GetNumPoints(attack->FireNumber);
	long NumHullPts = *NumPts;
	double xpt, ypt;
	double AngleOut, AngleIn, Angle, DiffAngle;
	APolygon ap;

	Hull[NumHullPts * 3] = xpt1 = GetPerimeter1Value(FireNum, StartPt, XCOORD);
	Hull[NumHullPts * 3 + 1] = ypt1 = GetPerimeter1Value(FireNum, StartPt,
										YCOORD);
	if (GetPerimeter2Value(StartPt, FLIVAL) < 0.0)
		Mult = -1;
	else
		Mult = 1;
	Hull[NumHullPts * 3 + 2] = StartPt * Mult;
	NumHullPts++;
	ap.startx = GetPerimeter1Value(FireNum, StartPt, XCOORD);
	ap.starty = GetPerimeter1Value(FireNum, StartPt, YCOORD);
	AngleOut = RefAngle;
	xpt = GetPerimeter1Value(FireNum, EndPt, XCOORD);
	ypt = GetPerimeter1Value(FireNum, EndPt, YCOORD);
	AngleIn = ap.direction(xpt, ypt);
	DiffAngle = AngleOut - AngleIn;
	if (DiffAngle < 0.0)
		DiffAngle += 2.0 * PI;
	ConvexStartPt = StartPt + 1;
	ConvexEndPt = EndPt;
	if (ConvexEndPt < ConvexStartPt)
		ConvexEndPt += NumPoints - ConvexStartPt + StartPt + 1;
	ConOrder = -1;
	for (j = ConvexStartPt; j < ConvexEndPt; j++)
	{
		i = j;
		if (i > NumPoints - 1)
			i -= NumPoints;
		xpt = GetPerimeter1Value(FireNum, i, XCOORD);
		ypt = GetPerimeter1Value(FireNum, i, YCOORD);
		Angle = AngleOut - ap.direction(xpt, ypt);
		if (Angle < 0.0)
			Angle += 2.0 * PI;
		if (Angle < DiffAngle)
		{
			DiffAngle = Angle;
			ConPt = i;
			ConOrder = j;
		}
		if (j == ConvexEndPt - 1 && ConOrder > -1)
		{
			if (ConOrder < ConvexEndPt)
			{
				j = ConOrder;
				ap.startx = GetPerimeter1Value(FireNum, ConPt, XCOORD);
				ap.starty = GetPerimeter1Value(FireNum, ConPt, YCOORD);
				xpt = GetPerimeter1Value(FireNum, EndPt, XCOORD);
				ypt = GetPerimeter1Value(FireNum, EndPt, YCOORD);
				AngleIn = ap.direction(xpt, ypt);
				DiffAngle = AngleOut - AngleIn;
				if (DiffAngle < 0.0)
					DiffAngle += 2.0 * PI;
			}
			if (GetPerimeter2Value(ConPt, FLIVAL) < 0.0)
				Mult = -1;
			else
				Mult = 1;
			Hull[NumHullPts * 3] = xpt1 = GetPerimeter1Value(FireNum, ConPt,
											XCOORD);
			Hull[NumHullPts * 3 + 1] = ypt1 = GetPerimeter1Value(FireNum,
												ConPt, YCOORD);
			Hull[NumHullPts * 3 + 2] = ConPt * Mult;
			NumHullPts++;
			ConOrder = -1;
		}
	}
	*NumPts = NumHullPts;
}


void Attack::ExpandConvexHulls(double* Hull1, double* Hull2, long NumHullPts)
{
	long i, j, k, ConPt;
	double Distance;
	double xptl, yptl;
	double xptn, yptn;
	double xpt, ypt, xh, yh, fh, oldx, oldy;
	double firstx, firsty, xdiff, ydiff;
	double xdiffl, ydiffl, xdiffn, ydiffn;
	double tempx, tempy, distl, distn, dist;
	double A1, A2, A3, DR;

	/*
	FILE *newfile=fopen("hulls1.vct", "a");
	fprintf(newfile, "%s\n", "10");
	for(i=0; i<NumHullPts; i++)
	{	xh=ConvertEastingOffsetToUtm(Hull2[i*3]);
		yh=ConvertNorthingOffsetToUtm(Hull2[i*3+1]);
	fprintf(newfile, "%lf %lf\n", xh, yh);
	}
	fprintf(newfile, "%s\n", "END");
	fclose(newfile);
	*/

	Distance = ((double) attack->FireDist + GetDistRes() / 2.0) * MetricResolutionConvert();
	j = NumHullPts - 1;
	xptl = Hull2[j * 3];
	yptl = Hull2[j * 3 + 1];
	xpt = firstx = Hull2[0];
	ypt = firsty = Hull2[1];
	for (i = 0; i < NumHullPts; i++)
	{
		k = i + 1;
		if (k > NumHullPts - 1)
		{
			xptn = firstx;
			yptn = firsty;
		}
		else
		{
			xptn = Hull2[k * 3];
			yptn = Hull2[k * 3 + 1];
		}
		xdiffl = xpt - xptl;
		ydiffl = ypt - yptl;
		xdiffn = xpt - xptn;
		ydiffn = ypt - yptn;
		distl = sqrt(xdiffl * xdiffl + ydiffl * ydiffl);
		distn = sqrt(xdiffn * xdiffn + ydiffn * ydiffn);

		if (distl > 0.0 && distn > 0.0)
		{
			if (distl < distn)
			{
				tempx = xpt - xdiffn * distl / distn;
				tempy = ypt - ydiffn * distl / distn;
				xdiff = xptl - tempx;
				ydiff = yptl - tempy;
			}
			else
			{
				if (distn < distl)
				{
					tempx = xpt - xdiffl * distn / distl;
					tempy = ypt - ydiffl * distn / distl;
					xdiff = tempx - xptn;
					ydiff = tempy - yptn;
				}
				else
				{
					xdiff = xptl - xptn;
					ydiff = yptl - yptn;
				}
			}
		}
		else
		{
			xdiff = xptl - xptn;
			ydiff = yptl - yptn;
		}

		dist = sqrt(xdiff * xdiff + ydiff * ydiff);
		if (dist < 1e-9) //==0.0)
			dist = 1.0;
		if (fabs(xdiffl) < 1e-9 && fabs(ydiffl) < 1e-9)
			A1 = 0.0;
		else
			A1 = atan2((xdiffl), (ydiffl));
		if (fabs(xdiff) < 1e-9 && fabs(ydiff) < 1e-9)
			A2 = 0.0;
		else
			A2 = atan2((xdiff), (ydiff));
		A3 = cos(A1) * cos(A2) + sin(A1) * sin(A2);
		if (fabs(A3) > 1e-2 && distl > 1e-9)
			DR = Distance / fabs(A3);
		else
			DR = Distance;

		ConPt = labs((long) Hull2[i * 3 + 2]);
		xh = GetPerimeter2Value(ConPt, XCOORD);
		yh = GetPerimeter2Value(ConPt, YCOORD);
		fh = GetPerimeter2Value(ConPt, FLIVAL);
		oldx = xpt;
		oldy = ypt;
		if (fh < 0.0)
		{
			xpt = xh;
			ypt = yh;
		}
		Hull2[i * 3] = xpt - DR / dist * ydiff;
		Hull2[i * 3 + 1] = ypt + DR / dist * xdiff;
		xdiffl = xpt - xh;
		ydiffl = ypt - yh;
		Hull1[i * 2] = Hull2[i * 3] - xdiffl;
		Hull1[i * 2 + 1] = Hull2[i * 3 + 1] - ydiffl;
		/*
			xbuf1=xpt-DistanceResolution/dist*ydiff;	// perpendicular to xpt,ypt
			ybuf1=ypt+DistanceResolution/dist*xdiff;
			xbuf2=xpt+DistanceResolution/dist*ydiff; 	// perpendicular to xpt,ypt
			ybuf2=ypt-DistanceResolution/dist*xdiff;
			*/

		xptl = oldx;//xpt;
		yptl = oldy;//ypt;
		xpt = xptn;
		ypt = yptn;
	}


	/*     FILE *newfile=fopen("hulls.vct", "a");
		 fprintf(newfile, "%s\n", "10");
		 for(i=0; i<NumHullPts; i++)
		 {	xh=ConvertEastingOffsetToUtm(Hull2[i*3]);
		 	yh=ConvertNorthingOffsetToUtm(Hull2[i*3+1]);
			fprintf(newfile, "%lf %lf\n", xh, yh);
		 }
		 fprintf(newfile, "%s\n", "END");
		 fprintf(newfile, "%s\n", "20");
		 for(i=0; i<NumHullPts; i++)
		 {	xh=ConvertEastingOffsetToUtm(Hull1[i*2]);
		 	yh=ConvertNorthingOffsetToUtm(Hull1[i*2+1]);
			fprintf(newfile, "%lf %lf\n", xh, yh);
		 }
		 fprintf(newfile, "%s\n", "END");
		fclose(newfile);
	*/
}


void Attack::HullDensityControl(double* Hull1, double* Hull2, long* numhullpts)
{
	bool NewPointAdded;
	long i, j, k, NumHullPts = *numhullpts;
	double xpt, ypt, xpt1, ypt1, xpt2, ypt2, fli, fli1;
	double dist, PerimRes;

	PerimRes = GetPerimRes() * 2.0 * MetricResolutionConvert();
	do
	{
		NewPointAdded = false;
		for (i = 0; i < NumHullPts; i++)
		{
			j = i + 1;
			if (j >= NumHullPts)
				j = 0;
			dist = sqrt(pow2(Hull1[i * 3] - Hull1[j * 3]) +
					pow2(Hull1[i * 3 + 1] - Hull1[j * 3 + 1]));
			if (dist > PerimRes)
			{
				xpt1 = Hull1[i * 3] - (Hull1[i * 3] - Hull1[j * 3]) / 2.0;
				ypt1 = Hull1[i * 3 + 1] -
					(Hull1[i * 3 + 1] - Hull1[j * 3 + 1]) /
					2.0;
				fli1 = (long) ((Hull1[i * 3 + 2] + Hull1[j * 3 + 2]) / 2.0);
				xpt2 = Hull2[i * 2] - (Hull2[i * 2] - Hull2[j * 2]) / 2.0;
				ypt2 = Hull2[i * 2 + 1] -
					(Hull2[i * 2 + 1] - Hull2[j * 2 + 1]) /
					2.0;
				for (k = i + 1; k < NumHullPts; k++)
				{
					xpt = Hull1[k * 3];
					ypt = Hull1[k * 3 + 1];
					fli = Hull1[k * 3 + 2];
					Hull1[k * 3] = xpt1;
					Hull1[k * 3 + 1] = ypt1;
					Hull1[k * 3 + 2] = fli1;
					xpt1 = xpt;
					ypt1 = ypt;
					fli1 = fli;
					xpt = Hull2[k * 2];
					ypt = Hull2[k * 2 + 1];
					Hull2[k * 2] = xpt2;
					Hull2[k * 2 + 1] = ypt2;
					xpt2 = xpt;
					ypt2 = ypt;
				}
				Hull1[NumHullPts * 3] = xpt1;
				Hull1[NumHullPts * 3 + 1] = ypt1;
				Hull1[k * 3 + 2] = fli1;
				Hull2[NumHullPts * 2] = xpt2;
				Hull2[NumHullPts * 2 + 1] = ypt2;
				NumHullPts++;
				i++;						// force over newly added point
				if (NumHullPts >= GetNumPoints(attack->FireNumber) * 2 - 1)   // alloc safety
				{
					NewPointAdded = false;
					break;
				}
				else
					NewPointAdded = true;
			}
		}
	}
	while (NewPointAdded);

	*numhullpts = NumHullPts;
}


/*
void Attack::ReplaceDeadPoints()
{// replaces new with old point if fli was <0.0, indicating it was extinuished
	for(long i=0; i<GetNumPoints(attack->FireNumber); i++)
	 {    fli1=GetPerimeter2Value(i, FLIVAL);
		if(fli1<0.0)
		  {    GetPerimeter2(i, &xpt1, &ypt1, &ros1, &fli1);
		  	SetPerimeter1(attack->FireNumber, i, xpt1, ypt1);
			   SetFireChx(attack->FireNumber, i, ros1, fli1);
		  }
	 }
}
*/

//----------------------------------------------------------------------------
//Global Supression Access Functions
//----------------------------------------------------------------------------

static long NumAttacks = 0;
static long AttackCounter = 0;
AttackData* FirstAttack;
AttackData* NextAttack;
AttackData* CurAttack;
AttackData* LastAttack;
AttackData* Reassignment = 0;
char AttackLog[MAX_CUR_DIR_STR_LEN+14];

void LoadAttacks(AttackData attackdata)
{
	// function only for loading attacks from bookmark
	if (NumAttacks == 0)
	{
		FirstAttack = new AttackData;//(AttackData *) calloc(1, sizeof(AttackData));
		CurAttack = FirstAttack;
		memcpy(FirstAttack, &attackdata, sizeof(AttackData));
	}
	memcpy(CurAttack, &attackdata, sizeof(AttackData));
	NextAttack = new AttackData;//(AttackData *) calloc(1, sizeof(AttackData));
	CurAttack->next = (AttackData *) NextAttack;
	if (NumAttacks == 0)
		FirstAttack->next = (AttackData *) NextAttack;
	NumAttacks++;
	CurAttack = NextAttack;
}

/*============================================================================
  SetupIndirectAttack
  indirect attack constructor
*/
long SetupIndirectAttack( long crewnum, double* startpt, long numpts )
{ //SetupIndirectAttack
  long i;

  for( i = 0; i <= NumAttacks; i++ ) {
    if( NumAttacks == 0 ) {
      if( (FirstAttack = new AttackData) != NULL ) {
        CurAttack = FirstAttack;
        if( AttackCounter == 0 ) {
          GetCurDir( AttackLog, MAX_CUR_DIR_STR_LEN );
          //JWB: Mod'd AttackLog to init space on stack to have
          //     MAX_CUR_DIR_STR_LEN + 14 chars.
          strcat( AttackLog, "grndattk.log" );
          remove( AttackLog );
        }
      }
      else return 0;
    }
    else if( i == 0 ) CurAttack = FirstAttack;
    else CurAttack = NextAttack;

    if( i < NumAttacks ) NextAttack = (AttackData *) CurAttack->next;
  }

  if( (NextAttack = new AttackData) != NULL ) {
    CurAttack->next = (AttackData *) NextAttack;
    CurAttack->IndirectLine1 = 0;
    CurAttack->CrewNum = crewnum;
    CurAttack->Suspended = 0;
    CurAttack->AttackNumber = ++AttackCounter;
    CurAttack->Burnout = 0;
    CurAttack->AttackTime = 0.0;
    CurAttack->LineBuilt = 0.0;
    if( ResetIndirectAttack(CurAttack, startpt, numpts) ) ++NumAttacks;
    else {
      delete NextAttack;
      AttackCounter--;

      return 0;
    }
  }
  else return 0;

  return AttackCounter;
} //SetupIndirectAttack

//============================================================================
long ResetIndirectAttack( AttackData* atk, double* coords, long numpts )
{ //ResetIndirectAttack
  if( atk->AttackTime > 0.0 ) {
    WriteAttackLog( atk, 0, 0, 0 );
    atk->AttackTime = 0.0;
    atk->LineBuilt = 0.0;
  }
  atk->NumPoints = numpts;
  atk->Indirect = 1;
  if( atk->IndirectLine1 ) delete[] atk->IndirectLine1;
  if( (atk->IndirectLine1 = new double[numpts * 2]) != NULL ) {
    for( long i = 0; i < numpts; i++ ) {
      //20100928:JWB: I think the coords are supposed to be *relative*,
      //              so subtracting edge values here. 
      //         atk->IndirectLine1[i * 2] = coords[i * 2];
      //         atk->IndirectLine1[i * 2 + 1] = coords[i * 2 + 1];
      atk->IndirectLine1[i * 2] = coords[i * 2] - GetWestUtm() + GetLoEast();
      atk->IndirectLine1[i * 2 + 1] =
                             coords[i * 2 + 1] - GetSouthUtm() + GetLoNorth();
    }
    atk->CurrentPoint = 0;
    atk->FireNumber = -1;
    atk->BurnLine[0] = -1;  //Use -1 as flag for reset
  }
  else {
    atk->IndirectLine1 = 0;

    return 0;
  }

  return 1;
} //ResetIndirectAttack

/*============================================================================
  SetupDirectAttack
  Direct attack constructor.
  Find nearst vertex on all fires, && establish direction for line building.
*/
long SetupDirectAttack( long crewnum, long FireNum, double* coords )
{ //SetupDirectAttack
  CallLevel++;

  if( Verbose > CallLevel )
    printf( "%*sfsxwattk:SetupDirectAttack:1 NumAttacks=%ld\n",
            CallLevel, "", NumAttacks );

  long i;
  for( i = 0; i <= NumAttacks; i++ ) {
    if( NumAttacks == 0 ) {
      if( (FirstAttack = new AttackData) != NULL ) {
        CurAttack = FirstAttack;
        if( AttackCounter == 0 ) {
          GetCurDir( AttackLog, MAX_CUR_DIR_STR_LEN );
          //JWB: Mod'd AttackLog to init space on stack to have
          //     MAX_CUR_DIR_STR_LEN + 14 chars.
          strcat( AttackLog, "grndattk.log" );
          remove( AttackLog );
        }
      }
      else {
        CallLevel--;
        return 0;
      }
    }
    else if( i == 0 ) CurAttack = FirstAttack;
    else CurAttack = NextAttack;

    if( i < NumAttacks ) NextAttack = (AttackData *) CurAttack->next;
  }

  if( Verbose > CallLevel )
    printf( "%*sfsxwattk:SetupDirectAttack:2\n", CallLevel, "" );

  if( (NextAttack = new AttackData) != NULL ) {
    CurAttack->next = (AttackData *) NextAttack;
    CurAttack->AttackNumber = ++AttackCounter;
    CurAttack->CrewNum = crewnum;
    CurAttack->Suspended = 0;
    CurAttack->AttackTime = 0.0;
    CurAttack->LineBuilt = 0.0;
    CurAttack->IndirectLine1 = 0;
    CurAttack->IndirectLine2 = 0;
    CurAttack->NumPoints = 0;
    CurAttack->Reverse = 0;
    CurAttack->Burnout = 0;
    if( ResetDirectAttack(CurAttack, FireNum, coords) ) ++NumAttacks;
    else {
      AttackCounter--;
      delete[] NextAttack;

      CallLevel--;
      return 0;
    }
  }
  else {
    CallLevel--;
    return 0;
  }

  CallLevel--;
  return AttackCounter;
} //SetupDirectAttack

//============================================================================
long ResetDirectAttack(AttackData* atk, long FireNum, double* coords)
{
	long i;
	double xpt1, ypt1;
	double xdist, ydist, hdist, mindist = 0;
	double startpointx, startpointy, endpointx, endpointy;
	bool first = true;

	if (atk->AttackTime > 0.0)
	{
		WriteAttackLog(atk, 0, 0, 0);
		atk->AttackTime = 0.0;
		atk->LineBuilt = 0.0;
	}
	if (GetInputMode() == PARALLELLOCATION ||
		GetInputMode() == RELOCATEPARALLEL)
		atk->Indirect = 2;
	else
		atk->Indirect = 0;
	atk->Burnout = 0;
	atk->BurnLine[0] = -1;		// use -1 as flag for reset
	atk->CurrentPoint = -1;		// number of vertex on Indirect Line, not used here
	atk->LineNumber = -1;
	atk->FireNumber = FireNum;
	startpointx = coords[0];   	// transfer coords to local vars
	startpointy = coords[1];
	endpointx = coords[2];
	endpointy = coords[3];
	for (i = 0; i < GetNumPoints(CurAttack->FireNumber); i++)
	{
		xpt1 = GetPerimeter1Value(CurAttack->FireNumber, i, XCOORD);
		ypt1 = GetPerimeter1Value(CurAttack->FireNumber, i, YCOORD);
		xdist = pow2(xpt1 - startpointx);
		ydist = pow2(ypt1 - startpointy);
		hdist = xdist + ydist;
		if (first)
		{
			mindist = hdist;
			atk->CurrentPoint = i;
			first = false;
		}
		else if (hdist < mindist)
		{
			mindist = hdist;
			atk->CurrentPoint = i;
		}
	}
	first = true;
	for (i = 0; i < GetNumPoints(atk->FireNumber); i++)
	{
		xpt1 = GetPerimeter1Value(CurAttack->FireNumber, i, XCOORD);
		ypt1 = GetPerimeter1Value(CurAttack->FireNumber, i, YCOORD);
		xdist = pow2(xpt1 - endpointx);
		ydist = pow2(ypt1 - endpointy);
		hdist = xdist + ydist;
		if (first && i != CurAttack->CurrentPoint)
		{
			mindist = hdist;
			atk->NextPoint = i;
			first = 0;
		}
		else if (hdist < mindist && i != CurAttack->CurrentPoint)
		{
			mindist = hdist;
			atk->NextPoint = i;
		}
	}
	atk->Reverse = false;
	if (GetInout(atk->FireNumber) == 1 || GetInout(atk->FireNumber) == 2)
	{
		if (atk->NextPoint > atk->CurrentPoint)
		{
			if (atk->NextPoint -
				atk->CurrentPoint >
				(double) GetNumPoints(atk->FireNumber) /
				2.0)
			{
				atk->Reverse = true;
				//			else
				//  				atk->Reverse=false;
			}
		}
		else if (atk->CurrentPoint -
			atk->NextPoint <
			(double) GetNumPoints(atk->FireNumber) /
			2.0)
		{
			atk->Reverse = true;
			//  		  	else
			//  					atk->Reverse=false;
		}
	}
	else
		return 0;

	// 	allocate 2 points for xpt2 & ypt2 for determining start point later

	if (atk->IndirectLine1)
		delete[] atk->IndirectLine1;//free(atk->IndirectLine1);
	atk->IndirectLine1 = 0;
	if (atk->IndirectLine2)
		delete[] atk->IndirectLine2;//free(atk->IndirectLine2);
	atk->IndirectLine2 = 0;
	if (atk->Indirect == 0) 		  // direct attack
	{
		if ((atk->IndirectLine1 = new double[3]) != NULL)//(double *) calloc(3, sizeof(double)))!=NULL)
		{
			atk->IndirectLine1[0] = GetPerimeter1Value(atk->FireNumber,
										atk->CurrentPoint, XCOORD);
			atk->IndirectLine1[1] = GetPerimeter1Value(atk->FireNumber,
										atk->CurrentPoint, YCOORD);
			atk->IndirectLine1[2] = -1.0;
		}
		else
		{
			atk->IndirectLine1 = 0;

			return 0;
		}
	}
	else						  // parallel attack
	{
		if ((atk->IndirectLine2 = new double[3]) != NULL)//(double *) calloc(3, sizeof(double)))!=NULL)
		{
			atk->IndirectLine2[0] = GetPerimeter1Value(atk->FireNumber,
										atk->CurrentPoint, XCOORD);
			atk->IndirectLine2[1] = GetPerimeter1Value(atk->FireNumber,
										atk->CurrentPoint, YCOORD);
			atk->IndirectLine2[2] = -1.0;
		}
		else
		{
			atk->IndirectLine1 = 0;
			atk->IndirectLine2 = 0;
			return 0;
		}
	}

	return 1;
}

long GetNumAttacks()
{
	// returns the number of active attacks
	return NumAttacks;
}

long GetFireNumberForAttack(long AttackCounter)
{
	// returns fire number associated with a given attack
	for (long i = 0; i < NumAttacks; i++)
	{
		if (i == 0)
			CurAttack = FirstAttack;
		NextAttack = (AttackData *) CurAttack->next;
		if (CurAttack->AttackNumber == AttackCounter)
			return CurAttack->FireNumber;
		CurAttack = NextAttack;
	}

	return -1;
}


AttackData* GetAttackForFireNumber(long NumFire, long StartAttackNum,
	long* LastAttackNumber)
{
	for (long i = 0; i < NumAttacks; i++)
	{
		if (i == 0)
			CurAttack = FirstAttack;
		NextAttack = (AttackData *) CurAttack->next;
		if (i >= StartAttackNum)
		{
			if (CurAttack->FireNumber == NumFire && CurAttack->Suspended == 0)
			{
				*LastAttackNumber = i;

				return CurAttack;
			}
		}
		CurAttack = NextAttack;
	}

	return 0;
}

void SetNewFireNumberForAttack(long oldnumfire, long newnumfire)
{
	for (long i = 0; i < NumAttacks; i++)
	{
		if (i == 0)
		{
			CurAttack = FirstAttack;
			NextAttack = (AttackData *) CurAttack->next;
		}
		if (CurAttack->FireNumber == oldnumfire)
		{
			if (GetInout(newnumfire) == 3)
			{
				if (CurAttack->Indirect == 1)
					CurAttack->FireNumber = newnumfire;
			}
			else if (CurAttack->Indirect != 1)
				CurAttack->FireNumber = newnumfire;
		}
		else if (CurAttack->Indirect == 2)  		   // if parallel attack only
		{
			if (CurAttack->LineNumber == oldnumfire)
				CurAttack->LineNumber = newnumfire;
		}
		CurAttack = NextAttack;
		NextAttack = (AttackData *) CurAttack->next;
	}
}

long GetNumCrews()
{
	return NumCrews;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

AttackData* GetAttackByOrder(long OrdinalAttackNum, bool IndirectOnly)
{
	// retrieves indirect attack in order
	for (long i = 0; i < NumAttacks; i++)
	{
		if (i == 0)
			CurAttack = FirstAttack;
		else
			CurAttack = NextAttack;
		NextAttack = (AttackData *) CurAttack->next;
		if (i == OrdinalAttackNum)
		{
			if (IndirectOnly)
			{
				if (CurAttack->Indirect > 0 && CurAttack->Suspended == 0)
					return CurAttack;
				else
					return 0;
			}
			else
				return CurAttack;
		}
	}

	return 0;
}


AttackData* GetAttack(long AttackCounter)
{
	// Get Attack Instance, Attack Number is sequential based on
	for (long i = 0; i < NumAttacks; i++)
	{
		if (i == 0)
		{
			CurAttack = FirstAttack;
			NextAttack = (AttackData *) CurAttack->next;
		}
		if (CurAttack->AttackNumber == AttackCounter)
			return CurAttack;
		else
		{
			CurAttack = NextAttack;
			NextAttack = (AttackData *) CurAttack->next;
		}
	}

	return 0;
}


void CancelAttack(long AttackCounter)
{
	//make sure Attack Number is "1-based" NOT "0-based"
	for (long i = 0; i < NumAttacks; i++)
	{
		if (i == 0)
		{
			CurAttack = FirstAttack;
			NextAttack = (AttackData *) CurAttack->next;
		}
		if (CurAttack->AttackNumber == AttackCounter)
		{
			if (i == 0)
				FirstAttack = (AttackData *) CurAttack->next;
			else
				LastAttack->next = (AttackData *) NextAttack;
			WriteAttackLog(CurAttack, 0, 0, 0);
			if (CurAttack->IndirectLine1)
				delete[] CurAttack->IndirectLine1;//free(CurAttack->IndirectLine1);
			CurAttack->IndirectLine1 = 0;
			if (CurAttack->Indirect == 2 && CurAttack->IndirectLine2)
				delete[] CurAttack->IndirectLine2;//free(CurAttack->IndirectLine2);
			CurAttack->IndirectLine2 = 0;
			delete CurAttack;//free(CurAttack);
			NumAttacks--;
			if (NumAttacks == 0)
				delete NextAttack;//free(NextAttack);
			break;
		}
		else
		{
			LastAttack = CurAttack;
			CurAttack = NextAttack;
			NextAttack = (AttackData *) CurAttack->next;
		}
	}
}


AttackData* GetReassignedAttack()
{
	return Reassignment;
}

void ReassignAttack(AttackData* atk)
{
	Reassignment = atk;
}


void FreeAllAttacks()
{
	for (long i = 0; i < NumAttacks; i++)
	{
		if (i == 0)
		{
			CurAttack = FirstAttack;
			NextAttack = (AttackData *) CurAttack->next;
		}
		WriteAttackLog(CurAttack, 0, 0, 0);
		if (CurAttack->IndirectLine1)
			delete[] CurAttack->IndirectLine1;//free(CurAttack->IndirectLine1);
		CurAttack->IndirectLine1 = 0;
		if (CurAttack->Indirect == 2 && CurAttack->IndirectLine2)	// if parallel attack
			delete[] CurAttack->IndirectLine2;//free(CurAttack->IndirectLine2);
		CurAttack->IndirectLine2 = 0;
		delete CurAttack;//free(CurAttack);
		CurAttack = NextAttack;
		NextAttack = (AttackData *) CurAttack->next;
	}
	if (NumAttacks > 0)
	{
		delete CurAttack;//free(CurAttack);
		NumAttacks = 0;
	}
	AttackCounter = 0;
}

//============================================================================
void WriteAttackLog( AttackData* atk, long type, long var1, long var2 )
{ //WriteAttackLog
  FILE* atklog;
  char AttackType[32];

  memset( AttackType, 0x0, sizeof(AttackType) );
  if( (atklog = fopen(AttackLog, "a")) != NULL ) {
    if( atk ) {
      switch( atk->Indirect ) {
        case 0: sprintf( AttackType, "%s", "Direct" );
                break;
        case 1: sprintf( AttackType, "%s", "Indirect" );
                break;
        case 2: sprintf( AttackType, "%s", "Parallel" );
                break;
      }
    }

    switch( type ) {
      case 0: //End of attack
              fprintf( atklog,
                       "%s, %s, %ld %s, %ld %s, $%ld since last instruction\n",
                       GetCrew(atk->CrewNum)->CrewName, AttackType,
                       (long) atk->AttackTime, "mins", (long) atk->LineBuilt,
                       "m",
                    (long)(GetCrew(atk->CrewNum)->Cost * atk->AttackTime / 60.0
                           + GetCrew(atk->CrewNum)->FlatFee) );
              break;
      case 1: //Reassign attack crew
              fprintf( atklog, "%s reassigned %s\n", crew[var1]->CrewName,
                               AttackType );
              break;
      case 2: //Crew moved to different compound crew
              fprintf( atklog, "%s moved to %s\n", crew[var1]->CrewName,
                               crew[var2]->CrewName );
              break;
      case 3:
              fprintf( atklog,
                       "%s exceeded flamelength limit, direct attack\n",
                       GetCrew(atk->CrewNum)->CrewName );
              break;
      case 4: //Crew removed from group
              fprintf( atklog, "%s removed from group %s\n",
                               crew[var1]->CrewName, crew[var2]->CrewName );
              break;
      case 5: //End of attack
              fprintf( atklog,
                   "%s, %s, %s, %ld %s, %ld %s, $%ld since last instruction\n",
                    GetCrew(atk->CrewNum)->CrewName, "Crew Added", AttackType,
                    (long) atk->AttackTime, "mins", (long) atk->LineBuilt, "m",
                   (long) (GetCrew(atk->CrewNum)->Cost * atk->AttackTime / 60.0
                           + GetCrew(atk->CrewNum)->FlatFee ));
              break;
    }
    fclose( atklog );
  }
} //WriteAttackLog


bool LoadCrews(char* FileName, bool AppendList)
{
	FILE* crewfile;
	char garbage[256] = "", data[256] = "";
	char ch[2] = "";
	double RateMult, rate, test;
	long fuelnumber, crewnumber;

	if ((crewfile = fopen(FileName, "r")) != NULL)
	{
		long i, j;
		j = GetNumCrews();
		if (!AppendList)
		{
			for (i = 0; i < j; i++)
				FreeCrew(0);
		}

		fgets(garbage, 255, crewfile);
		do
		{
			if (feof(crewfile))
				break;
			strncpy(ch, garbage, 1);
			if (strcmp(ch, "#"))
				break;
			crewnumber = SetNewCrew();
			if (crewnumber == -1)
				return false;
			crew[crewnumber] = GetCrew(crewnumber);
			crew[crewnumber]->Compound = -1;
			memset(crew[crewnumber]->CrewName, 0x0,
				sizeof(crew[crewnumber]->CrewName));
			for (i = 1; i < (int)strlen(garbage) - 2; i++)
				strncat(crew[crewnumber]->CrewName, &garbage[i], 1);
			fgets(garbage, 255, crewfile);
			sscanf(garbage, "%s", data);
			if (!strcmp(strupr(data), "CHAINS_PER_HOUR"))
			{
				crew[crewnumber]->Units = 3;
				RateMult = 0.3048 * 1.1;
			}
			else if (!strcmp(strupr(data), "METERS_PER_MINUTE"))
			{
				crew[crewnumber]->Units = 1;
				RateMult = 1.0;
			}
			else if (!strcmp(strupr(data), "FEET_PER_MINUTE"))
			{
				crew[crewnumber]->Units = 2;
				RateMult = 0.3048;
			}
			fgets(garbage, 255, crewfile);
			sscanf(garbage, "%s %lf", data, &test);
			if (!strcmp(strupr(data), "FLAME_LIMIT"))
				crew[crewnumber]->FlameLimit = test;
			if (crew[crewnumber]->Units == 2 || crew[crewnumber]->Units == 3)
				crew[crewnumber]->FlameLimit *= 0.3048;
			do
			{
				fgets(garbage, 255, crewfile);
				sscanf(garbage, "%ld %lf", &fuelnumber, &rate);
				if (fuelnumber < 51)
					crew[crewnumber]->LineProduction[fuelnumber - 1] = rate * RateMult;
				else if (fuelnumber == 99)
					break;
			}
			while (!feof(crewfile));
			fgets(garbage, 255, crewfile);
			sscanf(garbage, "%s", data);
			if (!strcmp(strupr(data), "COST_PER_HOUR"))
			{
				sscanf(garbage, "%s %lf", data, &test);
				crew[crewnumber]->Cost = test;
				fgets(garbage, 255, crewfile);
			}
			else
				crew[crewnumber]->Cost = -1.0;
		}
		while (!feof(crewfile));
		fclose(crewfile);
	}
	else
		return false;

	return true;
}

/*
bool LoadCrews(char *FileName, bool AppendList)
{
	FILE* crewfile;
	 char garbage[256];
	 char ch[2]="";
	 double RateMult, rate;
	 long fuelnumber, crewnumber;

	if((crewfile=fopen(FileName, "r"))!=NULL)
	{    long i, j;
		  j=GetNumCrews();
		  if(!AppendList)
		  {    for(i=0; i<j; i++)
				  FreeCrew(0);
		  }

		  ch[0]=getc(crewfile);
		  do
	 	{    ch[0]=getc(crewfile);
			   if(feof(crewfile))
			   	break;
			   crewnumber=SetNewCrew();
		  	if(crewnumber==-1)
				return false;
			   crew[crewnumber]=GetCrew(crewnumber);
			   crew[crewnumber]->Compound=-1;
//  		  	for(i=0; i<51; i++)		// initialize LineProduction Rates
//     			crew[crewnumber]->LineProduction[i]=0.0;
//  			 fscanf(crewfile, "%1c", ch);
//     		fscanf(crewfile, "%1c", ch);
			memset(crew[crewnumber]->CrewName, 0x0, sizeof(crew[crewnumber]->CrewName));
		 	do
		 	{    strcat(crew[crewnumber]->CrewName, ch);
			   	ch[0]=getc(crewfile);
				//fscanf(crewfile, "%1c", ch);
			  } while(strcmp(ch, "#"));
		 	fscanf(crewfile, "%s", garbage);
			  if(!strcmp(garbage, "CHAINS_PER_HOUR"))
			  {    crew[crewnumber]->Units=3;
			   	RateMult=0.27709;
			   }
			  else if(!strcmp(garbage, "METERS_PER_MINUTE"))
			  {    crew[crewnumber]->Units=1;
			   	RateMult=1.0;
			   }
			  else if(!strcmp(garbage, "FEET_PER_MINUTE"))
			  {    crew[crewnumber]->Units=2;
			   	RateMult=0.3048;
			   }
			fscanf(crewfile, "%s", garbage);
			  if(!strcmp(strupr(garbage), "FLAME_LIMIT"))
			  	fscanf(crewfile, "%lf", &crew[crewnumber]->FlameLimit);
			  //else if(!strcmp(strupr(garbage), "SLOPE_LIMIT"))
			  //	fscanf(crewfile, "%lf", &crew[crewnumber]->SlopeLimit);
		 	//fscanf(crewfile, "%s", garbage);
			  //if(!strcmp(strupr(garbage), "SLOPE_LIMIT"))
			  //	fscanf(crewfile, "%lf", &crew[crewnumber]->SlopeLimit);
			  //else if(!strcmp(strupr(garbage), "FLAME_LIMIT"))
			  //	fscanf(crewfile, "%lf", &crew[crewnumber]->FlameLimit);
			if(crew[crewnumber]->Units==2 || crew[crewnumber]->Units==3)
			  	crew[crewnumber]->FlameLimit*=0.3048;
			  do
			{    fscanf(crewfile, "%ld", &fuelnumber);
				if(fuelnumber<51)
					{	fscanf(crewfile, "%lf", &rate);
					crew[crewnumber]->LineProduction[fuelnumber-1]=rate*RateMult;
					}
					else if(fuelnumber==99)
						break;
			} while(!feof(crewfile));
			   ch[0]=getc(crewfile);
			   ch[0]=getc(crewfile);
		}while(!strncmp(ch, "#", 1)); //(!feof(crewfile));
		fclose(crewfile);
	 }
	 else
		  return false;

	 return true;
}
*/

Crew* GetCrew(long CrewNumber)
{
	if (crew[CrewNumber])
		return crew[CrewNumber];

	return 0;
}


long SetNewCrew()
{
	// doesn't require crew number
	if ((crew[NumCrews] = new Crew) != NULL)//(Crew *) calloc(1, sizeof(Crew)))!=NULL)
		return NumCrews++;

	return -1;
}

long SetCrew(long CrewNumber)
{
	// requires crew number for allocation
	if ((crew[CrewNumber] = new Crew) != NULL)//(Crew *) calloc(1, sizeof(Crew)))!=NULL)
		return NumCrews++;

	return -1;
}


void FreeAllCrews()
{
	long i;

	for (i = 0; i < NumCrews; i++)
		FreeCrew(0);
	NumCrews = 0;
}


void FreeCrew(long CrewNumber)
{
	long i;
	Crew* crw;

	if (crew[CrewNumber])
	{
		crw = crew[CrewNumber];
		for (i = CrewNumber; i < NumCrews - 1; i++)
			crew[i] = crew[i + 1];
		NumCrews--;
		crew[NumCrews] = crw;
		delete crew[NumCrews];//free(crew[NumCrews]);
		crew[NumCrews] = 0;
	}
}


static long const MaxNumCompoundCrews = 200;
CompoundCrew* compoundcrew[200];
static long NumCompoundCrews = 0;

long GetNumCompoundCrews()
{
	return NumCompoundCrews;
}

/*
long SetCompoundCrew(long CrewNumber, char *crewname)
{
	 if(NumCompoundCrews>=MaxNumCompoundCrews)
	 	return -1;

	if((compoundcrew[CrewNumber]=new CompoundCrew)!=NULL)//(CompoundCrew *) calloc(1, sizeof(CompoundCrew)))!=NULL)
	 {    compoundcrew[CrewNumber]->CrewIndex=new long[COMPOUNDINC];//(long *) calloc(COMPOUNDINC, sizeof(long));
		compoundcrew[CrewNumber]->Multiplier=new double[COMPOUNDINC];//(double *) calloc(COMPOUNDINC, sizeof(double));
		  compoundcrew[CrewNumber]->NumTotalCrews=COMPOUNDINC;
		  compoundcrew[CrewNumber]->NumCurrentCrews=0;
		  if((compoundcrew[CrewNumber]->CompCrew=SetNewCrew())>-1)
		  {	memset(crew[compoundcrew[CrewNumber]->CompCrew]->CrewName, 0x0,
			   	sizeof(crew[compoundcrew[CrewNumber]->CompCrew]->CrewName));
			   strcpy(crew[compoundcrew[CrewNumber]->CompCrew]->CrewName, crewname);
			   crew[compoundcrew[CrewNumber]->CompCrew]->Compound=CrewNumber;
		  }

	 	return NumCompoundCrews++;
	 }

	 return -1;
}
*/

long SetCompoundCrew(long GroupNumber, char* crewname, long ExistingCrewNumber)
{
	if (NumCompoundCrews >= MaxNumCompoundCrews)
		return -1;

	long Val = 0;

	if (GroupNumber >= NumCompoundCrews)
	{
		if ((compoundcrew[GroupNumber] = new CompoundCrew) == NULL)
			return -1;
		compoundcrew[GroupNumber]->CrewIndex = new long[COMPOUNDINC];//(long *) calloc(COMPOUNDINC, sizeof(long));
		compoundcrew[GroupNumber]->Multiplier = new double[COMPOUNDINC];//(double *) calloc(COMPOUNDINC, sizeof(double));
		compoundcrew[GroupNumber]->NumTotalCrews = COMPOUNDINC;
		compoundcrew[GroupNumber]->NumCurrentCrews = 0;
		Val = 1;
	}

	if (ExistingCrewNumber == -1)
	{
		if ((compoundcrew[GroupNumber]->CompCrew = SetNewCrew()) == -1)
			return -1;
	}
	else
		compoundcrew[GroupNumber]->CompCrew = ExistingCrewNumber;
	memset(crew[compoundcrew[GroupNumber]->CompCrew]->CrewName, 0x0,
		sizeof(crew[compoundcrew[GroupNumber]->CompCrew]->CrewName));
	strcpy(crew[compoundcrew[GroupNumber]->CompCrew]->CrewName, crewname);
	crew[compoundcrew[GroupNumber]->CompCrew]->Compound = GroupNumber;

	if (Val == 0)
		return NumCompoundCrews;

	return NumCompoundCrews++;
}


bool AddToCompoundCrew(long CrewNumber, long NewCrew, double Mult)
{
	long* tempindx;
	double* tempmult;
	long numcur = compoundcrew[CrewNumber]->NumCurrentCrews;

	if (numcur == compoundcrew[CrewNumber]->NumTotalCrews)
	{
		if ((tempindx = new long[numcur]) == NULL)	//if((tempindx=(long *) calloc(numcur, sizeof(long)))==NULL)
			return false;
		if ((tempmult = new double[numcur]) == NULL) //((tempmult=(long *) calloc(numcur, sizeof(double)))==NULL)
			return false;
		memcpy(tempindx, compoundcrew[CrewNumber]->CrewIndex,
			numcur * sizeof(long));
		memcpy(tempmult, compoundcrew[CrewNumber]->Multiplier,
			numcur * sizeof(double));
		delete[] compoundcrew[CrewNumber]->CrewIndex;//free(compoundcrew[CrewNumber]->CrewIndex);
		delete[] compoundcrew[CrewNumber]->Multiplier; //free(compoundcrew[CrewNumber]->Multiplier);
		compoundcrew[CrewNumber]->CrewIndex = new long[numcur + COMPOUNDINC];	// (long *) calloc(numcur+COMPOUNDINC, sizeof(long));
		compoundcrew[CrewNumber]->Multiplier = new double[numcur + COMPOUNDINC];	// (double *) calloc(numcur+COMPOUNDINC, sizeof(double));
		compoundcrew[CrewNumber]->NumTotalCrews = numcur + COMPOUNDINC;
		memcpy(compoundcrew[CrewNumber]->CrewIndex, tempindx,
			numcur * sizeof(long));
		memcpy(compoundcrew[CrewNumber]->Multiplier, tempmult,
			numcur * sizeof(double));
		delete[] tempindx;//free(tempindx);
		delete[] tempmult;//free(tempmult);
	}
	compoundcrew[CrewNumber]->CrewIndex[numcur] = NewCrew;
	compoundcrew[CrewNumber]->Multiplier[numcur] = Mult;
	compoundcrew[CrewNumber]->NumCurrentCrews++;

	return true;
}

void CalculateCompoundRates(long CrewNumber)
{
	double Rate, Cost, MaxLen, FlatFee;
	long i, j;
	long numcur = compoundcrew[CrewNumber]->NumCurrentCrews;

	MaxLen = 0.0;
	for (i = 0; i < 51; i++)
	{
		Rate = 0.0;
		Cost = 0.0;
		FlatFee = 0.0;
		for (j = 0; j < numcur; j++)
		{
			Rate += crew[compoundcrew[CrewNumber]->CrewIndex[j]]->LineProduction[i] * compoundcrew[CrewNumber]->Multiplier[j];
			Cost += crew[compoundcrew[CrewNumber]->CrewIndex[j]]->Cost;
			FlatFee += crew[compoundcrew[CrewNumber]->CrewIndex[j]]->FlatFee;
		}
		crew[compoundcrew[CrewNumber]->CompCrew]->LineProduction[i] = Rate;
		crew[compoundcrew[CrewNumber]->CompCrew]->Cost = Cost;
		crew[compoundcrew[CrewNumber]->CompCrew]->FlatFee = FlatFee;
	}
	for (j = 0; j < numcur; j++)
	{
		if (crew[compoundcrew[CrewNumber]->CrewIndex[j]]->FlameLimit > MaxLen)
			MaxLen = crew[compoundcrew[CrewNumber]->CrewIndex[j]]->FlameLimit;
	}
	crew[compoundcrew[CrewNumber]->CompCrew]->FlameLimit = MaxLen;
	crew[compoundcrew[CrewNumber]->CompCrew]->Units = 1;
}


CompoundCrew* GetCompoundCrew(long CrewNumber)
{
	return compoundcrew[CrewNumber];
}

void FreeCompoundCrew(long CrewNumber)
{
	long i, j, OldCompCrew;
	CompoundCrew* crw;

	crw = compoundcrew[CrewNumber];
	OldCompCrew = compoundcrew[CrewNumber]->CompCrew;
	for (i = CrewNumber; i < NumCompoundCrews - 1; i++)
	{
		compoundcrew[i] = compoundcrew[i + 1];
		if (compoundcrew[i]->CompCrew > OldCompCrew)
			compoundcrew[i]->CompCrew--;
		for (j = 0; j < compoundcrew[i]->NumCurrentCrews; j++)
		{
			if (compoundcrew[i]->CrewIndex[j] > OldCompCrew)
				compoundcrew[i]->CrewIndex[j]--;
		}
	}
	AttackData* atk;
	for (i = 0; i < GetNumAttacks(); i++)
	{
		atk = GetAttackByOrder(i, false);
		if (atk->CrewNum > OldCompCrew)
			atk->CrewNum--;
	}
	NumCompoundCrews--;
	compoundcrew[NumCompoundCrews] = crw;

	for (i = 0; i < GetNumCrews(); i++)
	{
		if (crew[i]->Compound > CrewNumber)
			crew[i]->Compound--;
	}

	if (compoundcrew[NumCompoundCrews])
	{
		FreeCrew(compoundcrew[NumCompoundCrews]->CompCrew);
		delete[] compoundcrew[NumCompoundCrews]->CrewIndex;//free(compoundcrew[NumCompoundCrews]->CrewIndex);
		delete[] compoundcrew[NumCompoundCrews]->Multiplier; //free(compoundcrew[NumCompoundCrews]->Multiplier);
		delete compoundcrew[NumCompoundCrews]; //free(compoundcrew[NumCompoundCrews]);
	}
	compoundcrew[NumCompoundCrews] = 0;
}

void FreeAllCompoundCrews()
{
	long i;

	for (i = 0; i < NumCompoundCrews; i++)
	{
		if (compoundcrew[i])
		{
			delete[] compoundcrew[i]->CrewIndex; //free(compoundcrew[i]->CrewIndex);
			delete[] compoundcrew[i]->Multiplier; //free(compoundcrew[i]->Multiplier);
			delete compoundcrew[i]; //free(compoundcrew[i]);
		}
		compoundcrew[i] = 0;
	}
	NumCompoundCrews = 0;
}

bool LoadCompoundCrews(char* FileName)
{
	long NumCompCrews;
	long CrewNum, NumCrews;
	long i, j, * Index;
	char Name[256] = "";
	double* Mult;
	FILE* CurrentFile;

	if ((CurrentFile = fopen(FileName, "rb")) != NULL)
	{
		fread(&NumCompCrews, sizeof(long), 1, CurrentFile);
		for (i = 0; i < NumCompCrews; i++)
		{
			fread(&CrewNum, sizeof(long), 1, CurrentFile);
			CrewNum = SetNewCrew();
			crew[CrewNum]->Compound = i;
			fread(Name, sizeof(char), 256, CurrentFile);
			SetCompoundCrew(i, Name, CrewNum);
			fread(&NumCrews, sizeof(long), 1, CurrentFile);	// numtotal
			Index = new long[NumCrews];
			Mult = new double[NumCrews];
			fread(&NumCrews, sizeof(long), 1, CurrentFile);	// numcurrent
			fread(Index, sizeof(long), NumCrews, CurrentFile);
			fread(Mult, sizeof(double), NumCrews, CurrentFile);
			for (j = 0; j < NumCrews; j++)
				AddToCompoundCrew(i, Index[j], Mult[j]);
			delete[] Index;
			delete[] Mult;
			CalculateCompoundRates(i);
		}
		fclose(CurrentFile);
	}
	else
		return false;

	return true;
}


void RemoveFromCompoundCrew(long CompNumber, long CrewNumber)
{
	long i;

	//j=CrewNumber;//compoundcrew[CompNumber]->CrewIndex[CrewNumber];

	WriteAttackLog(0, 4, GetCompoundCrew(CompNumber)->CrewIndex[CrewNumber],
		GetCompoundCrew(CompNumber)->CompCrew);
	compoundcrew[CompNumber]->NumCurrentCrews--;
	for (i = CrewNumber; i < compoundcrew[CompNumber]->NumCurrentCrews; i++)
	{
		compoundcrew[CompNumber]->CrewIndex[i] = compoundcrew[CompNumber]->CrewIndex[i + 1];
		compoundcrew[CompNumber]->Multiplier[i] = compoundcrew[CompNumber]->Multiplier[i + 1];
	}
	compoundcrew[CompNumber]->CrewIndex[compoundcrew[CompNumber]->NumCurrentCrews] = 0;
	compoundcrew[CompNumber]->Multiplier[compoundcrew[CompNumber]->NumCurrentCrews] = 0;
}
