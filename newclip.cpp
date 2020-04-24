/*============================================================================
  newclip.cpp

  RFL version  
  Based on Version 4.1.0 (12/21/2004) by Mark Finney.

  See LICENSE.TXT file for license information.

  New loop-clipping functions for FARSITE.
  ============================================================================
*/
#include"fsglbvar.h"
#include"globals.h"
#include"Point.h"
#include"PerimeterPoint.h"
#include"fsxw.h"

//============================================================================
bool Intersections::TurningNumberOK( long CurrentFire, long StartPoint )
{ //Intersections::TurningNumberOK
  double xpt, ypt, midx, midy;
  double Tx1, Ty1, Tx2, Ty2;
  long i, j, k, l, m, NumPoints;
  long inside;

  NumPoints = GetNumPoints( CurrentFire );
  GetInterPointCoord( StartPoint, &xpt, &ypt );
  GetIntersection( StartPoint, &i, &j );

  for( m = 0; m < 4; m++ ) {
    switch ( m ) {
      case 0:
        k = i;
        l = j;
        break;
      case 1:
        k = i + 1;
        l = j;
        break;
      case 2:
        k = i;
        l = j + 1;
        break;
      case 3:
        k = i + 1;
        l = j + 1;
        break;
    }
    if( k > NumPoints ) k = 0;
    if( l > NumPoints ) l = 0;
    Tx1 = GetPerimeter1Value( CurrentFire, k, XCOORD );
    Ty1 = GetPerimeter1Value( CurrentFire, k, YCOORD );
    Tx2 = GetPerimeter1Value( CurrentFire, l, XCOORD );
    Ty2 = GetPerimeter1Value( CurrentFire, l, YCOORD );
    midx = Tx1 - ( Tx1 - Tx2 ) / 2.0;
    midy = Ty1 - ( Ty1 - Ty2 ) / 2.0;
    startx = xpt - ( xpt - midx ) / 2.0;
    starty = ypt - ( ypt - midy ) / 2.0;
    inside = Overlap( CurrentFire );
    if( ! inside ) return true;
  }

  return false;
} //Intersections::TurningNumberOK

//============================================================================
void Intersections::CleanPerimeter( long CurrentFire )
{ //Intersections::CleanPerimeter
  int Terminate;
  bool WritePerim = true;
  long i, icount, subcount;
  long StartPoint = -1, isect1, isect2;
  long LastStart = -1, isect1T, isect2T;

  CallLevel++;

  if( Verbose >= CallLevel )
    printf( "%*snewclip:Intersections::CleanPerimeter:1\n", CallLevel, "" );

  AllocNewIntersection( numcross );
  for( i = 0; i < numcross; i++ ) {
    GetIntersection( i, &isect1, &isect2 );
    SetNewIntersection( 2, i, isect1 + 1, isect2 + 1 );
  }

  if( Verbose >= CallLevel )
    printf( "%*snewclip:Intersections::CleanPerimeter:2 "
            "numpts[%d]=%ld writenum=%ld\n",
            CallLevel, "", 0, GetNumPoints(0), writenum );

  if( writenum == -1 ) WritePerim = false;

  do {
    if( Verbose >= CallLevel )
      printf( "%*snewclip:Intersections::CleanPerimeter:2a "
              "writenum=%ld\n",
              CallLevel, "", writenum );

    FindFirePerimeter( CurrentFire, StartPoint );

    if( ! WritePerim ) break;

    if( Verbose >= CallLevel )
      printf( "%*snewclip:Intersections::CleanPerimeter:2b "
              "numcross=%ld writenum=%ld\n",
              CallLevel, "", numcross, writenum );

    Terminate = 1;
    for( icount = 0; icount < numcross; icount++ ) {
      GetNewIntersection( 0, icount, &isect1, &isect2 );
      if( isect1 > 0 && isect1 < isect2 ) {
        if( LastStart == icount ) {
          SetNewIntersection( 0, icount, -isect1, -isect2 );
          for( subcount = icount; subcount < numcross; subcount++ ) {
            GetNewIntersection( 0, subcount, &isect1T, &isect2T );
            if( isect1T == isect2 && isect2T == isect1 ) {
              SetNewIntersection( 0, subcount, -isect1T, -isect2T );
              break;
            }
          }
          LastStart = -1;
        }
        else {
          if( TurningNumberOK(CurrentFire, icount) ) {
            StartPoint = LastStart = icount;
            Terminate = 0;
            break;
          }
          else {
            SetNewIntersection( 0, icount, -isect1, -isect2 );
            for( subcount = icount; subcount < numcross; subcount++ ) {
              GetNewIntersection( 0, subcount, &isect1T, &isect2T );
              if( isect1T == isect2 && isect2T == isect1 ) {
                SetNewIntersection( 0, subcount, -isect1T, -isect2T );
                break;
              }
            }
            LastStart = -1;
          }
        }
      }
    }

    if( Verbose >= CallLevel )
      printf( "%*snewclip:Intersections::CleanPerimeter:2c\n",
              CallLevel, "" );

  } while( ! Terminate );

  if( Verbose >= CallLevel )
    printf( "%*snewclip:Intersections::CleanPerimeter:3 "
            "numpts[%d]=%ld writenum=%ld\n",
            CallLevel, "", 0, GetNumPoints(0), writenum );

  if( WritePerim ) {
    if( writenum < 0 ) writenum *= -1;
    if( Verbose >= CallLevel )
      printf( "%*snewclip:Intersections::CleanPerimeter:3a "
              "numpts[%d]=%ld writenum=%ld\n",
              CallLevel, "", 0, GetNumPoints(0), writenum );

    SetNumPoints( CurrentFire, writenum );
    if( writenum == 0 ) {  // done here or in Intersect::CrossCompare
      SetInout( CurrentFire, 0 );
      IncSkipFires( 1 );
      if( CheckPostFrontal(GETVAL) )
        SetNewFireNumber( CurrentFire, -1,
                          post.AccessReferenceRingNum(1, GETVAL) );
    }
  }

  if( Verbose >= CallLevel )
    printf( "%*snewclip:Intersections::CleanPerimeter:4 numpts[%d]=%ld\n",
            CallLevel, "", 0, GetNumPoints(0) );

  CallLevel--;
} //Intersections::CleanPerimeter

//============================================================================
void Intersections::FindFirePerimeter( long CurrentFire, long StartPoint )
{ //Intersections::FindFirePerimeter
  double crossdist1, crossdist2, crossdist3, crossdist4, totaldist;
  double distl, outdist;
  long  crosscount, nextcount;
  long  CrossFound = 0, CrossSpan = -1, CrossNext = -1, CrossLastL = -1,
        CrossLastN = -1;
  long  icount, isect1, isect2, istop1 = -1, istop2 = -1;
  long  InternalLoop = -1;

  bool  Terminate = false, StartAtCross = false, StartAtReverse = false;
  bool  OffsetCrossPoint = false, EliminateTroubleFire = false;
  int   Reverse, Direction = 0, InOut;
  long  isect2T, isect1T, isect2M, isect1M;
  long  isect1A, isect2A, dup1, dup2, n;
  bool  Erase, MateFound, Exit, ptcross1, ptcross2, crossyn;

  CallLevel++;

  if( Verbose >= CallLevel )
    printf( "%*snewclip:Intersections::FindFirePerimeter:1 writenum=%ld\n",
            CallLevel, "", writenum );

  long  numpts = GetNumPoints( CurrentFire );
  long  writecount = 0, writelimit = numpts * 2;
  long  pointcount = 0, pointcountn = 1, pointcountl = numpts - 1,
        pointtoward;

  PerimeterPoint PrevPt, Pt, NextPt, Pt1;
  if( StartPoint != -1 ) {
    AllocSwap( GetNumPoints(CurrentFire) );
    GetNewIntersection( 0, StartPoint, &pointcountl, &pointcount );
    pointcountl -= 1;
    pointcount -= 1;
    istop1 = pointcountl;  //Says when to stop looping
    istop2 = pointcount;
    StartAtCross = true;

std::cerr<<"AAA newclip:Intersections:FindFirePerimeter:1a\n"; //AAA
    GetPerimeter1Point( CurrentFire, pointcountl, &PrevPt );
    Pt1 = PrevPt;
std::cerr<<"AAA newclip:Intersections:FindFirePerimeter:1b " //AAA
         << PrevPt.Get(PerimeterPoint::X_VAL) << " " //AAA
         << Pt1.Get(PerimeterPoint::X_VAL) << "\n"; //AAA
    pointcountn = pointcountl + 1;
    GetPerimeter1Point( CurrentFire, pointcountn, &NextPt );
std::cerr<<"AAA newclip:Intersections:FindFirePerimeter:1c " //AAA
         << NextPt.Get(PerimeterPoint::X_VAL) << "\n"; //AAA
    GetInterPoint( StartPoint, &Pt );
    CrossFound = 1;
    CrossLastL = pointcount;
    CrossLastN = pointcount + 1;

    pointcount = pointcountl;
    Pt1 = Pt;
    SetSwap( writecount++, Pt1.Get(PerimeterPoint::X_VAL),
                           Pt1.Get(PerimeterPoint::Y_VAL),
                           Pt1.Get(PerimeterPoint::ROS_VAL),
                           Pt1.GetFLI(),
                           Pt1.Get(PerimeterPoint::RCX_VAL) );
    InOut = 2;
  }
  else {
    AllocPerimeter2( writelimit );
std::cerr<<"AAA newclip:Intersections:FindFirePerimeter:1d\n"; //AAA
    GetPerimeter1Point( CurrentFire, pointcount, &Pt );
    GetPerimeter1Point( CurrentFire, pointcountl, &PrevPt );
    GetPerimeter1Point( CurrentFire, pointcountn, &NextPt );
    GetPerimeter1Point( CurrentFire, 0, &Pt1 );
std::cerr<<"AAA newclip:Intersections:FindFirePerimeter:1e " //AAA
         << Pt.Get(PerimeterPoint::X_VAL) << " " //AAA
         << Pt1.Get(PerimeterPoint::X_VAL) << "\n"; //AAA

    //202004201232 JWB: 'Corrected' this line: Pt1 = Pt;
    Pt1.SetLoc( Pt.GetX(), Pt.GetY() );

    SetPerimeter2( writecount++, Pt1.Get(PerimeterPoint::X_VAL),
                                 Pt1.Get(PerimeterPoint::Y_VAL),
                                 Pt1.Get(PerimeterPoint::ROS_VAL),
                                 Pt1.GetFLI(),
                                 Pt1.Get(PerimeterPoint::RCX_VAL) );
    InOut = 2;
    InOut = GetInout( CurrentFire );
  }

  if( Verbose >= CallLevel )
    printf( "%*snewclip:Intersections::FindFirePerimeter:2 "
            "writenum=%ld writecount=%ld\n",
            CallLevel, "", writenum, writecount );

  PerimeterPoint Pt2, Pt3, Pt4, Pt5;
  PerimeterPoint PtA, PtB, PtD;
  Point PtC, PtE, DummyPt;
  int orientation, concave;
  double mindistSq, testdistSq, mindist;
  do {
    if( Verbose >= CallLevel )
      printf( "%*snewclip:Intersections::FindFirePerimeter:2a "
              "StartPoint=%ld\n",
              CallLevel, "", StartPoint );

    orientation = PrevPt.GetOrientation( Pt, NextPt );

    if( orientation == POINT_CLOCKWISE ) concave = 1;
    else concave = 0;

    if( Verbose >= CallLevel )
      printf( "%*snewclip:Intersections::FindFirePerimeter:2b "
              "concave=%d orientation=%d\n",
              CallLevel, "", concave, orientation );

    if( InOut == 1 ) {
      if( CrossFound ) {
        if( orientation == POINT_CLOCKWISE ||
            orientation == POINT_STRAIGHT_LINE ) Reverse = 0;
        else Reverse = 1;
      }
      else {
        if( writecount == 1 && concave ) {
          Reverse = 1; //Can't have concave on first point.
          StartAtReverse = true;
        }
        else Reverse = 0;
      }
    }
    else {   //Inward burning fire
      if( CrossFound ) {
        if( orientation == POINT_CLOCKWISE ||
            orientation == POINT_STRAIGHT_LINE ) Reverse = 0;
        else if( InternalLoop < 0 ) Reverse = 1;
        else Reverse = 0;
      }
      else {
        if( writecount == 1 && concave ) {
          Reverse = 0;
          StartAtReverse = true;
        }
        else Reverse = 0;
      }
    }

    if( Reverse ) {
      Direction = Reverse;
      pointtoward = pointcountl;
      pointcountl = pointcountn;
      pointcountn = pointtoward;
      if( ! CrossFound ) {
        Pt5 = PrevPt;
        PtC.Set( Pt5.Get(PerimeterPoint::X_VAL),
                 Pt5.Get(PerimeterPoint::Y_VAL) );
      }
      else {
std::cerr<<"AAA newclip:Intersections:FindFirePerimeter:2a\n"; //AAA
        GetPerimeter1Point( CurrentFire, pointcountn, &NextPt );
        Pt5 = NextPt;
std::cerr<<"AAA newclip:Intersections:FindFirePerimeter:2b " //AAA
         << NextPt.Get(PerimeterPoint::X_VAL) << " " //AAA
         << Pt5.Get(PerimeterPoint::X_VAL) << "\n"; //AAA
        PtC.Set( Pt5.Get(PerimeterPoint::X_VAL),
                 Pt5.Get(PerimeterPoint::Y_VAL) );
        if( pointcountl != 0 && pointcountn > pointcountl ) Direction = 0;
      }
    }
    else {
      Pt5 = NextPt;
      PtC.Set( Pt5.Get(PerimeterPoint::X_VAL),
               Pt5.Get(PerimeterPoint::Y_VAL) );
      pointtoward = pointcountn;
    }

std::cerr<<"AAA newclip:Intersections:FindFirePerimeter:2c\n"; //AAA
    GetPerimeter1Point( CurrentFire, pointtoward, &Pt2 );
std::cerr<<"AAA newclip:Intersections:FindFirePerimeter:2d " //AAA
         << Pt2.Get(PerimeterPoint::X_VAL) << "\n"; //AAA
    Pt5.SetCharacteristics( Pt2.Get(PerimeterPoint::ROS_VAL),
                            Pt2.GetFLI() );
    Pt5.SetReact( Pt2.Get(PerimeterPoint::RCX_VAL) );
    mindistSq = Pt.CalcDistSq( PtC );

    testdistSq = 0.0;
    for( crosscount = 0; crosscount < numpts; crosscount++ ) {
      nextcount = crosscount + 1;
      if( nextcount == numpts ) nextcount = 0;
      if( crosscount == pointcount || crosscount == pointtoward ||
          nextcount == pointcount || nextcount == pointtoward )
        continue;
      long m = min( CrossLastL, CrossLastN );
      if( crosscount == m ) {
        if( CrossLastN == 0 ) {
          if( StartAtReverse ) {
            if( CrossLastL < 2 ) continue;
            else if( Direction == 0 ) continue;
          }
          else if( Direction == 1 ) continue;
        }
        else continue;
      }
      else if( CrossLastN == 0 && CrossLastL == numpts - 1 &&
               crosscount == numpts - 1 ) continue;
      else if( CrossLastL == 0 && CrossLastN == numpts - 1 &&
               crosscount == numpts - 1 ) continue;

std::cerr<<"AAA newclip:Intersections:FindFirePerimeter:2e\n"; //AAA
      GetPerimeter1Point( CurrentFire, crosscount, &PtA );
      GetPerimeter1Point( CurrentFire, nextcount, &PtB );
std::cerr<<"AAA newclip:Intersections:FindFirePerimeter:2f " //AAA
         << PtA.Get(PerimeterPoint::X_VAL) << " " //AAA
         << PtB.Get(PerimeterPoint::X_VAL) << "\n"; //AAA
      dup1 = dup2 = 1; //Want crosses only if on span1 && span2
      double dx = PtD.GetX(), dy = PtD.GetY();
      crossyn = Cross( Pt.GetX(), Pt.GetY(), PtC.GetX(), PtC.GetY(),
                       PtA.GetX(), PtA.GetY(), PtB.GetX(), PtB.GetY(),
                       &dx, &dy, &dup1, &dup2 );
      PtD.SetLoc( dx, dy );

      ptcross1 = ptcross2 = false;

      if( dup1 < 0 && dup2 < 0 )  //Indicates single pt crosses w/ single pt
        ptcross1 = ptcross2 = false;
      else if( dup1 < 0 ) {   //If cross duplicates xpt, ypt
        dup1 = 1;
        dup2 = 0;
        dx = DummyPt.GetX(), dy = DummyPt.GetY();
        if( ! Cross(PrevPt.GetX(), PrevPt.GetY(), PtC.GetX(), PtC.GetY(),
                    PtA.GetX(), PtA.GetY(), PtB.GetX(), PtB.GetY(),
                    &dx, &dy, &dup1, &dup2) )
          ptcross1 = true; //Means this is just a single pt cross
        else if( dup1 < 0 )
          ptcross1 = true; //Means this is just a single pt cross
        DummyPt.Set( dx, dy );
      }
      else if( dup2 < 0 ) {  //If cross duplicates crxpt, crypt
        n = crosscount - 1;
        if( n < 0 ) n = GetNumPoints( CurrentFire ) - 1;
std::cerr<<"AAA newclip:Intersections:FindFirePerimeter:2g\n"; //AAA
        GetPerimeter1Point( CurrentFire, n, &PrevPt );
std::cerr<<"AAA newclip:Intersections:FindFirePerimeter:2h " //AAA
         << PrevPt.Get(PerimeterPoint::X_VAL) << "\n"; //AAA
        dup1 = 0;
        dup2 = 1;
        dx = DummyPt.GetX(), dy = DummyPt.GetY();
        if( ! Cross(Pt.GetX(), Pt.GetY(), PtC.GetX(), PtC.GetY(),
                    PrevPt.GetX(), PrevPt.GetY(), PtB.GetX(), PtB.GetY(),
                    &dx, &dy, &dup1, &dup2) )
          ptcross2 = true; //Means this is just a single pt cross
        else if( dup2 < 0 )
          ptcross2 = true; //Means this is just a single pt cross
        DummyPt.Set( dx, dy );
      }

      if( crossyn ) {
        //Only single pt crosses, so no crosses to process.
        if( ptcross1 == true || ptcross2 == true ) crossyn = false;
      }

      if( crossyn ) {
        testdistSq = Pt.CalcDistSq( PtD );
        if( testdistSq < 1e-18 ) { //Same point
          if( ! CrossFound ) {
            CrossFound = 1;
            testdistSq = -1;
            CrossSpan = crosscount;
            CrossNext = nextcount;
            //------------------------------------------------
            //Offset cross point if same one, 10/20/2000.
            distl = PtA.CalcDist( PtB );
            if( distl > 0.02 ) outdist = 0.01;  //Maximum distance
            else if( distl > 0.000002 ) outdist = distl / 2.0;
            else outdist = 0.000001;
            outdist /= distl;

            double x = Pt.GetX() - outdist * ( PtA.GetX() - PtB.GetX() );
            double y = Pt.GetY() - outdist * ( PtA.GetY() - PtB.GetY() );
            PtD.SetLoc( x, y );
            //------------------------------------------------
            break;
          }
          continue;
        }

        if( testdistSq < mindistSq ) {
          crossdist1 = sqrt( mindistSq );
          mindistSq = testdistSq;
          mindist = sqrt( testdistSq );
          crossdist2 = crossdist1 - mindist;
          CrossFound = 1;
          CrossSpan = crosscount;
          CrossNext = nextcount;
          Pt5 = PtD;
std::cerr<<"AAA newclip:Intersections:FindFirePerimeter:2i\n"; //AAA
          GetPerimeter1Point( CurrentFire, crosscount, &Pt3 );
          GetPerimeter1Point( CurrentFire, nextcount, &Pt4 );
std::cerr<<"AAA newclip:Intersections:FindFirePerimeter:2j " //AAA
         << Pt3.Get(PerimeterPoint::X_VAL) << " " //AAA
         << Pt4.Get(PerimeterPoint::X_VAL) << "\n"; //AAA
          crossdist3 = PtA.CalcDist( PtB );
          crossdist4 = PtA.CalcDist( PtD );
          totaldist = crossdist3 + crossdist1;
          crossdist3 -= crossdist4;
          if( totaldist > 0.0 ) {
            Pt5.SetCharacteristics(
                      ( Pt1.Get(PerimeterPoint::ROS_VAL) * mindist +
                        Pt2.Get(PerimeterPoint::ROS_VAL) * crossdist2 +
                        Pt3.Get(PerimeterPoint::ROS_VAL) * crossdist3 +
                        Pt4.Get(PerimeterPoint::ROS_VAL) * crossdist4 )
                        / totaldist,
                      ( fabs(Pt1.GetFLI()) * mindist +
                        fabs(Pt2.GetFLI()) * crossdist2 +
                        fabs(Pt3.GetFLI()) * crossdist3 +
                        fabs(Pt4.GetFLI()) * crossdist4 )
                        / totaldist );
            Pt5.SetReact(
                        ( Pt1.Get(PerimeterPoint::RCX_VAL) * mindist +
                          Pt2.Get(PerimeterPoint::RCX_VAL) * crossdist2 +
                          Pt3.Get(PerimeterPoint::RCX_VAL) * crossdist3 +
                          Pt4.Get(PerimeterPoint::RCX_VAL) * crossdist4 )
                          / totaldist );
            if( (Pt1.GetFLI() < 0.0 ||
                 Pt2.GetFLI() < 0.0) &&
                (Pt3.GetFLI() < 0.0 ||
                 Pt4.GetFLI() < 0.0) )
              Pt5.SetFLI( -Pt5.GetFLI() );
          }
        }
      }
    }

    if( Verbose >= CallLevel )
      printf( "%*snewclip:Intersections::FindFirePerimeter:2c "
              "StartPoint=%ld CrossFound=%ld StartAtCross=%d "
              "writecount=%ld\n",
              CallLevel, "", StartPoint, CrossFound, StartAtCross,
              writecount );

    //If writing existing outer or inner polygon...
    if( StartPoint == -1 ) {
      if( CrossFound && StartAtCross ) {
        if( istop1 == -1 ) {
          istop1 = min( pointcount, pointtoward );
          istop2 = CrossSpan;
          writecount = 0;
        }
        else if( CrossSpan == istop1 &&
                 (pointcount == istop2 || pointtoward == istop2) ) {
          Terminate = 1;
        }
      }
      else SetPerimeter2( writecount++, Pt5.GetX(), Pt5.GetY(),
                          Pt5.Get(PerimeterPoint::ROS_VAL),
                          Pt5.GetFLI(),
                          Pt5.Get(PerimeterPoint::RCX_VAL) );
    }
    else {
      //If writing new inward polygon.
      if( CrossFound && CrossSpan == istop1 &&
          (pointcount == istop2 || pointtoward == istop2) )
        Terminate = 1;
      else if( writecount > numpts ) {
        Terminate = 1;
        for( icount = 0; icount < numcross; icount++ ) {
          GetNewIntersection( 0, icount, &isect1, &isect2 );
          isect1 -= 1;
          isect2 -= 1;
          if( (isect1 == istop2 && isect2 == istop1) ||
              (isect1 == istop1 && isect2 == istop2) ) {
            isect1 += 1;
            isect2 += 1;
            SetNewIntersection( 0, icount, -isect1, -isect2 );
          }
        }
        writecount = 0;
      }

      if( ! Terminate ) {
        SetSwap( writecount++, Pt5.GetX(), Pt5.GetY(),
                          Pt5.Get(PerimeterPoint::ROS_VAL),
                          Pt5.GetFLI(),
                          Pt5.Get(PerimeterPoint::RCX_VAL) );
        if( CrossFound )    //Only for inward fires after crossfound
          OffsetCrossPoint = true;
      }
    }

    if( CrossFound ) {
      if( testdistSq != 0.0 ) {   //If more crosses after last cross
        InternalLoop = -1;
        CrossFound = false;
        Erase = false;
        MateFound = false;
        Exit = false;
        for( icount = 0; icount < numcross; icount++ ) {
          GetNewIntersection( 0, icount, &isect1, &isect2 );
          isect2T = abs( isect2 ) - 1;
          isect1T = abs( isect1 ) - 1;
          if( ! MateFound ) {
            if( ((isect2T == CrossSpan || isect2T == CrossNext) &&
                 (isect1T == pointcount || isect1T == pointcountn)) ||
                ((isect1T == CrossSpan || isect1T == CrossNext) &&
                 (isect2T == pointcount || isect2T == pointcountn))) {
              MateFound = true;
              Erase = true;
              isect2M = isect1T;
              isect1M = isect2T;
              if( StartPoint == -1 && isect1 < 0 ) MateFound = false;
            }
          }
          else {
            if( isect1T == isect1M && isect2T == isect2M ) {
              Erase = true;
              Exit = true;
            }
          }
          if( Erase ) {
            if( InOut == 2 && isect1 < 0 ) { //If not 1st time at intersection
              GetNewIntersection( 1, icount, &isect1A, &isect2A );
              if( isect1A > 0 || isect2A > 0 ) { //If 2nd time at intersection
                CrossFound = true;
                SetNewIntersection( 1, icount, -isect1A, -isect2A );
                if( Exit && OffsetCrossPoint ) {
                  distl = Pt.CalcDist( Pt5 );
                  if( distl > 0.02 ) outdist = 0.01;  //Maximum distance
                  else if( distl > 0.000002 ) outdist = distl / 2.0;
                  else {
                    outdist = 0.000001;
                    CrossFound = false;
                    Exit = true;
                  }
                  double ratio = ( distl - outdist ) / distl;
                  SetSwap( writecount - 1,
                           Pt.GetX() - ratio * (Pt.GetX() - Pt5.GetX()),
                           Pt.GetY() - ratio * (Pt.GetY() - Pt5.GetY()),
                           Pt5.Get(PerimeterPoint::ROS_VAL),
                           Pt5.GetFLI(),
                           Pt5.Get(PerimeterPoint::RCX_VAL) );
                  OffsetCrossPoint = false;
                }
              }
              else  //NEW 1/11/1996 to try to exclude illogical enclaves
                Exit = true;
            }
            else {
              isect1T += 1;
              isect2T += 1;
              if( InOut == 2 ) {
                if( InternalLoop == -1 ) InternalLoop = icount;
                else {
                  if( icount == InternalLoop + 1 ) InternalLoop = icount;
                  else InternalLoop = -2;
                }  //NEW 1/11/1996 to try to exclude illogical enclaves
                SetNewIntersection( 1, icount, -isect2T, -isect1T );
              }
              SetNewIntersection( 0, icount, -isect1T, -isect2T );
              CrossFound = true;
            }
            Erase = false;
          }
          if( Exit ) break;
        }
        if( StartPoint > -1 && ! CrossFound ) {
          Terminate = 1;
          writecount = 0;
          GetNewIntersection( 0, StartPoint, &isect1, &isect2 );
          if( isect1 > 0 ) isect1 *= (long)-1.0;
          if( isect2 > 0 ) isect2 *= (long)-1.0;
          SetNewIntersection( 0, StartPoint, isect1, isect2 );
          for( icount = 0; icount < numcross; icount++ ) {
            GetNewIntersection( 0, icount, &isect1, &isect2 );
            isect1 -= 1;
            isect2 -= 1;
            if( isect1 == istop2 && isect2 == istop1 ) {
              isect1 += 1;
              isect2 += 1;
              SetNewIntersection( 0, icount, -isect1, -isect2 );
              break;
            }
          }
        }
        pointcountn = CrossNext;
        CrossLastN = pointtoward;   //Switch crossed spans
        pointtoward = pointcount;
        pointcountl = pointcount = CrossSpan;
        CrossLastL = pointtoward;
      }
      else {  //If crosses last time but not now
        CrossFound = 0;
        OffsetCrossPoint = false;
        CrossLastL = -1;
        CrossLastN = -1;
        pointcount = pointtoward;
        if( Reverse ) {
          pointcountl = pointcount + 1;
          pointcountn = pointcount - 1;
        }
        else {
          pointcountl = pointcount - 1;
          pointcountn = pointcount + 1;
        }
        Direction = Reverse;
        if( pointcountn == numpts ) pointcountn = 0;
        else if( pointcountn < 0 ) pointcountn = numpts - 1;
        if( pointcountl == numpts ) pointcountl = 0;
        else if( pointcountl < 0 ) pointcountl = numpts - 1;
      }
    }
    else {   //If no crosses last & this time through
      if( Direction ) {
        pointcountn--;
        pointcountl = pointcount;
        pointcount--;
      }
      else {
        pointcountn++;
        pointcountl = pointcount;
        pointcount++;
      }
      if( pointcountn == numpts ) pointcountn = 0;
      else if( pointcountn < 0 ) pointcountn = numpts - 1;
      if( pointcount == numpts ) pointcount = 0;
      else if( pointcount < 0 ) pointcount = numpts - 1;
    }
std::cerr<<"AAA newclip:Intersections:FindFirePerimeter:2k\n"; //AAA
    GetPerimeter1Point( CurrentFire, pointcountn, &NextPt );
std::cerr<<"AAA newclip:Intersections:FindFirePerimeter:2l " //AAA
         << NextPt.Get(PerimeterPoint::X_VAL) << "\n"; //AAA
    PrevPt = Pt;
    Pt = Pt1 = Pt5;
    if( StartPoint == -1 ) {
      if( pointcount == 0 && ! CrossFound ) Terminate = true;
      if( writecount >= writelimit ) {
        for( icount = 0; icount < numcross; icount++ ) {
          GetNewIntersection( 0, icount, &isect1, &isect2 );
          if( isect1 > 0 ) {
            isect1 *= -1;
            isect2 *= -1;
            SetNewIntersection( 0, icount, -isect1, -isect2 );
          }
        }
        Terminate = true;
        tranz( CurrentFire, 0 );
        writecount = numpts + 1;
        EliminateTroubleFire = true;
      }
    }
  } while( ! Terminate );

  if( Verbose >= CallLevel )
    printf( "%*snewclip:Intersections::FindFirePerimeter:3 "
            "writenum=%ld writecount=%ld\n",
            CallLevel, "", writenum, writecount );

  if( StartPoint == -1 ) {
    if( Verbose >= CallLevel )
      printf( "%*snewclip:Intersections::FindFirePerimeter:3a "
              "writecount=%ld\n",
              CallLevel, "", writecount );

    writenum = writecount - 1;  //writenum is Intersections::class data member
    if( InOut == 2 ) {
      if( arp(2, writenum) > 0.0 ) writenum *= -1;
      if( EliminateTroubleFire ) writenum = 0;
    }
  }
  else {
    if( Verbose >= CallLevel )
      printf( "%*snewclip:Intersections::FindFirePerimeter:3b\n",
              CallLevel, "" );

    AllocPerimeter1( GetNewFires(), writecount + 1 );
    SetNumPoints( GetNewFires(), writecount );
     //Set inward barrier to type 3.
    if( GetInout(CurrentFire) == 3 ) SetInout( GetNewFires(), 3 );
    else SetInout( GetNewFires(), 2 ); //Else set inward fire to type 2
    SwapTranz( GetNewFires(), writecount );
    BoundaryBox( writecount );
    //Only finish writing new fire.
    if( writecount > 2 && arp(1, GetNewFires()) < 0.0 )
      IncNewFires( 1 );   //If numpts>2 && inward fire
    else {
      SetNumPoints( GetNewFires(), 0 );
      SetInout( GetNewFires(), 0 );
    }
  }

  if( Verbose >= CallLevel )
    printf( "%*snewclip:Intersections::FindFirePerimeter:4 writenum=%ld\n",
            CallLevel, "", writenum );

  CallLevel--;
} //Intersections::FindFirePerimeter

//============================================================================
void Intersections::GetNewIntersection( long Alt, long count, long *isect1,
                                        long *isect2 )
{ //Intersections::GetNewIntersection
  if( Alt == 0 ) {
    if( NewIsect ) {
      count *= 2;
      *isect1 = NewIsect[count];
      *isect2 = NewIsect[count + 1];
    }
  }
  else if( AltIsect ) {
    count *= 2;
    *isect1 = AltIsect[count];
    *isect2 = AltIsect[count + 1];
  }
} //Intersections::GetNewIntersection

//============================================================================
void Intersections::SetNewIntersection( long Alt, long count, long isect1,
                                        long isect2 )
{ //Intersections::SetNewIntersection
  if( NewIsect && AltIsect ) {
    count *= 2;
    switch( Alt ) {
      case 0:
        NewIsect[count] = isect1;
        NewIsect[count + 1] = isect2;
        break;
      case 1:
        AltIsect[count] = isect1;
        AltIsect[count + 1] = isect2;
        break;
      case 2:
        NewIsect[count] = isect1;
        NewIsect[count + 1] = isect2;
        AltIsect[count] = isect1;
        AltIsect[count + 1] = isect2;
        break;
    }
  }
} //Intersections::SetNewIntersection

bool Intersections::AllocNewIntersection(long NumCross)
{
	if (NumCross > 0)
	{
		if (NumCross >= newisectnumalloc)
		{
			FreeNewIntersection();
			if ((NewIsect = new long[NumCross * 2]) != NULL)
			{
				if ((AltIsect = new long[NumCross * 2]) != NULL)
				{
					newisectnumalloc = NumCross;

					return true;
				}
				return false;
			}
			return false;                
		}
		return true;
	}
	NewIsect = 0;
	AltIsect = 0;

	return false;
}

bool Intersections::FreeNewIntersection()
{
	if (NewIsect)
		delete[] NewIsect;
	if (AltIsect)
		delete[] AltIsect;
	NewIsect = 0;
	AltIsect = 0;
	newisectnumalloc = 0;

	return true;
}

//-------------------------------------------------------------------------------------
//----------THIS VERSION WORKS 6/25/1995, SAVED WHILE MODIFICAITONS MADE---------------
//-------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------

void StandardizePolygon::FindOuterFirePerimeter(long CurrentFire)
{
	//double AngleForward, AngleBackward, Do, DiffAngle;
	double xpt, ypt, xptn, yptn, xptl, yptl, xl, yl;
	double xpta, ypta, xptb, yptb, xptc, yptc;
	double xptd, yptd, xpte, ypte;
	double ros1, fli1, ros2, fli2, ros3, fli3, ros4, fli4, ros5, fli5;
	double rcx1, rcx2, rcx3, rcx4, rcx5;
	double mindist, testdist, crossdist1, crossdist2, crossdist3, crossdist4,
		totaldist;
	double xdiff, ydiff, xdiffl, ydiffl, xdiffn, ydiffn, midx, midy;
	double tempx, tempy, distl, distn, outdist, PerpAngle;

	//bool FIRST;
	long n, dup1, dup2;
	long numpts = GetNumPoints(CurrentFire);
	long writecount = 0, writelimit = numpts*4;
	long pointcount = 0, pointcountn = 1, pointcountl = numpts - 1,
		pointtoward;
	long crosscount, nextcount; //intercount, intermatch,
	long CrossFound = 0, CrossSpan = -1, CrossNext = -1, CrossLastL = -1,
		CrossLastN = -1;
	bool Terminate = false;
	bool StartAtReverse = false, ptcross1, ptcross2, crossyn;
  int concave, Reverse, Direction = 0;

	//FreePerimeter2();
	AllocPerimeter2(writelimit);//+numcross);
	xpt = GetPerimeter1Value(CurrentFire, pointcount, XCOORD);
	ypt = GetPerimeter1Value(CurrentFire, pointcount, YCOORD);
	xptl = GetPerimeter1Value(CurrentFire, pointcountl, XCOORD);
	yptl = GetPerimeter1Value(CurrentFire, pointcountl, YCOORD);
	xptn = GetPerimeter1Value(CurrentFire, pointcountn, XCOORD);
	yptn = GetPerimeter1Value(CurrentFire, pointcountn, YCOORD);
	ros1 = GetPerimeter1Value(CurrentFire, 0, ROSVAL);
	fli1 = GetPerimeter1Value(CurrentFire, 0, FLIVAL);
	rcx1 = GetPerimeter1Value(CurrentFire, 0, RCXVAL);
	SetPerimeter2(writecount++, xpt, ypt, ros1, fli1, rcx1);
  do {
    Point P1( xptl, yptl );
    Point P2( xpt, ypt );
    Point P3( xptn,yptn );
    int orientation = P1.GetOrientation( P2, P3 );
		xdiffl = xpt - xptl;
		ydiffl = ypt - yptl;
		xdiffn = xpt - xptn;
		ydiffn = ypt - yptn;
		distl = sqrt(pow2(xdiffl) + pow2(ydiffl));
		distn = sqrt(pow2(xdiffn) + pow2(ydiffn));
    if( distl < distn ) {
			tempx = xpt - xdiffn * distl / distn;
			tempy = ypt - ydiffn * distl / distn;
			xdiff = xptl - tempx;
			ydiff = yptl - tempy;
			midx = xptl - xdiff / 2.0;
			midy = yptl - ydiff / 2.0;
			outdist = distn * 2.0;
    }
    else {
      if( distn < distl ) {
				tempx = xpt - xdiffl * distn / distl;
				tempy = ypt - ydiffl * distn / distl;
				xdiff = tempx - xptn;
				ydiff = tempy - yptn;
				midx = tempx - xdiff / 2.0;
				midy = tempy - ydiff / 2.0;
				outdist = distl * 2.0;
			}
			else
			{
				xdiff = xptl - xptn;
				ydiff = yptl - yptn;
				midx = xptl - xdiff / 2.0;
				midy = yptl - ydiff / 2.0;
				outdist = distn * 2.0;
			}
		}
		if (fabs(xdiff) < 1e-12)
			xdiff = 0.0;
		if (xdiff == 0.0)
		{
			if (ydiff > 0.0)
				PerpAngle = PI;
			else if (ydiff < 0.0)
				PerpAngle = 0.0;
		}
		else
			PerpAngle = atan2(ydiff, xdiff) + PI / 2.0;
		tempx = xpt + outdist * cos(PerpAngle);
		tempy = ypt + outdist * sin(PerpAngle);
		distn = pow2(tempx - xpt) + pow2(tempy - ypt);	  // distance from outpt to xpt,ypt
		distl = pow2(tempx - midx) + pow2(tempy - midy);  // distance from outpt to midx,midy
    if( distn > distl ) concave = 1;
    else concave = 0;

    if( GetInout(CurrentFire) == 1 ) {
      if( CrossFound ) {
        if( orientation == POINT_CLOCKWISE && concave ) Reverse = 0;  //keep it the same
        else if( orientation == POINT_STRAIGHT_LINE ) Reverse = 0;
        else Reverse = 1;  //Change direction
			}
			else {
				if( writecount == 1 && concave ) {
					Reverse = 1; 	// can't have concave on first point, because
					StartAtReverse = true;
				}
				else {				// always start at extreme point, which must be convex
					if ((orientation != POINT_CLOCKWISE && !concave) ||
						(orientation != POINT_COUNTERCLOCKWISE && concave))
						Reverse = 0;	// keep it the same
					else
						Reverse = 1;
				}
			}
		}
		else
		{
			if (CrossFound)
			{
				if (orientation == POINT_CLOCKWISE && concave)
					Reverse = 0;
				else if (orientation == POINT_STRAIGHT_LINE )
				{
					//distn=sqrt(pow2(xptl-xptn)+pow2(yptl-yptn));
					/*if(distn<distl)
									Reverse=0;
								else
									Reverse=1;		// change direction
								*/
					Reverse = 0;
				}
				else
					Reverse = 1;		// change direction
			}
			else
			{
				if (writecount == 1 && concave)
					Reverse = 1; 	// can't have concave on first point, because
				else				// always start at extreme point, which must be convex
				{
					if ((orientation != POINT_COUNTERCLOCKWISE && !concave) ||
						(orientation != POINT_CLOCKWISE && concave))
						Reverse = 0;
					else
						Reverse = 1;
				}
			}
		}
		if (Reverse)
		{
			Direction = Reverse;
			pointtoward = pointcountl;
			pointcountl = pointcountn;
			pointcountn = pointtoward;
			if (!CrossFound)
			{
				xptc = xpte = xptl;
				yptc = ypte = yptl;
			}
			else
			{
				xptc = xpte = xptn = GetPerimeter1Value(CurrentFire,
										pointcountn, XCOORD);
				yptc = ypte = yptn = GetPerimeter1Value(CurrentFire,
										pointcountn, YCOORD);
				if (pointcountl != 0 && pointcountn > pointcountl)
					//	Reverse=0;
					Direction = 0;
			}
		}
		else
		{
			xptc = xpte = xptn;
			yptc = ypte = yptn;
			pointtoward = pointcountn;
		}
		//Reverse=0;
		ros5 = ros2 = GetPerimeter1Value(CurrentFire, pointtoward, ROSVAL);
		fli5 = fli2 = GetPerimeter1Value(CurrentFire, pointtoward, FLIVAL);
		rcx5 = rcx2 = GetPerimeter1Value(CurrentFire, pointtoward, RCXVAL);
		crossdist1 = mindist = sqrt(pow2(xpt - xptc) + pow2(ypt - yptc));
		testdist = 0;
		for (crosscount = 0; crosscount < numpts; crosscount++)
		{
			nextcount = crosscount + 1;
			if (nextcount == numpts)
				nextcount = 0;
			if (crosscount == pointcount ||
				crosscount == pointtoward ||
				nextcount == pointcount ||
				nextcount == pointtoward)
				continue;
      long m = min( CrossLastL, CrossLastN );
			if( crosscount == m ) {
				if (CrossLastN == 0)
				{
					if (StartAtReverse)
					{
						if (CrossLastL < 2)
							continue;
						else if (Direction == 0)
							continue;
					}
					else
					{
						if (Direction == 1)
							continue;
					}
				}
				else
					continue;
			}
			else if (CrossLastN == 0 &&
				CrossLastL == numpts - 1 &&
				crosscount == numpts - 1)
				continue;
			else if (CrossLastL == 0 &&
				CrossLastN == numpts - 1 &&
				crosscount == numpts - 1)
				continue;
			xpta = GetPerimeter1Value(CurrentFire, crosscount, XCOORD);
			ypta = GetPerimeter1Value(CurrentFire, crosscount, YCOORD);
			xptb = GetPerimeter1Value(CurrentFire, nextcount, XCOORD);
			yptb = GetPerimeter1Value(CurrentFire, nextcount, YCOORD);
			dup1 = dup2 = 1; // want crosses only if on span1 && span2
			crossyn = Cross(xpt, ypt, xptc, yptc, xpta, ypta, xptb, yptb,
						&xptd, &yptd, &dup1, &dup2);

			ptcross1 = ptcross2 = false;
			if (dup1 < 0 && dup2 < 0)      // indicates single point crosses with single pt
				ptcross1 = ptcross2 = false;
			else if (dup1 < 0)  		// if cross duplicates xpt, ypt
			{
				dup1 = 1;
				dup2 = 0;
				if (!Cross(xptl, yptl, xptc, yptc, xpta, ypta, xptb, yptb,
						&xl, &yl, &dup1, &dup2))
					ptcross1 = true; // means this is just a single pt cross
				else if (dup1 < 0)
					ptcross1 = true; // means this is just a single pt cross
			}
			else if (dup2 < 0)     // if cross duplicates crxpt, crypt
			{
				n = crosscount - 1;
				if (n < 0)
					n = GetNumPoints(CurrentFire) - 1;
				xptl = GetPerimeter1Value(CurrentFire, n, XCOORD);
				yptl = GetPerimeter1Value(CurrentFire, n, YCOORD);
				dup1 = 0;
				dup2 = 1;
				if (!Cross(xpt, ypt, xptc, yptc, xptl, yptl, xptb, yptb, &xl,
						&yl, &dup1, &dup2))
					ptcross2 = true;   // means this is just a single pt cross
				else if (dup2 < 0)
					ptcross2 = true; // means this is just a single pt cross
			}
      if( crossyn ) {
				if (ptcross1 == true || ptcross2 == true)    // just a single pt cross
					crossyn = false;
			}

      if( crossyn ) {
        testdist = sqrt( pow2(xpt - xptd) + pow2(ypt - yptd) );
        if( testdist < 1e-9 ) { // same point
          if( ! CrossFound ) {
						CrossFound = 1;
						testdist = -1;
						CrossSpan = crosscount;
						CrossNext = nextcount;
						//------------------------------------------------
						// offset cross point if same one, 10/20/2000
						xdiffl = xpta - xptb;
						ydiffl = ypta - yptb;
						distl = sqrt(pow2(xdiffl) + pow2(ydiffl));
						if (distl > 0.02)
							outdist = 0.01;  // maximum distance
						else if (distl > 0.000002)
							outdist = distl / 2.0;
						else
							outdist = 0.000001;

						xdiffl = outdist * xdiffl / distl;
						ydiffl = outdist * ydiffl / distl;
						xptd = xpt - xdiffl;
						yptd = ypt - ydiffl;
						//------------------------------------------------
						break;
					}
					continue;
        }
        if( testdist < mindist ) {
          mindist = testdist;
					crossdist2 = crossdist1 - mindist;
					CrossFound = 1;
					CrossSpan = crosscount;
					CrossNext = nextcount;
					xpte = xptd;
					ypte = yptd;
					ros3 = GetPerimeter1Value(CurrentFire, crosscount, ROSVAL);
					fli3 = GetPerimeter1Value(CurrentFire, crosscount, FLIVAL);
					rcx3 = GetPerimeter1Value(CurrentFire, crosscount, RCXVAL);
					ros4 = GetPerimeter1Value(CurrentFire, nextcount, ROSVAL);
					fli4 = GetPerimeter1Value(CurrentFire, nextcount, FLIVAL);
					rcx4 = GetPerimeter1Value(CurrentFire, nextcount, RCXVAL);
					crossdist3 = sqrt(pow2(xpta - xptb) + pow2(ypta - yptb));
					crossdist4 = sqrt(pow2(xpta - xptd) + pow2(ypta - yptd));
					totaldist = crossdist3 + crossdist1;
					crossdist3 -= crossdist4;
					if (totaldist > 0.0)
					{
						ros5 = ros1 * mindist /
							totaldist +
							ros2 * crossdist2 /
							totaldist +
							ros3 * crossdist3 /
							totaldist +
							ros4 * crossdist4 /
							totaldist;
						fli5 = fabs(fli1) * mindist /
							totaldist +
							fabs(fli2) * crossdist2 /
							totaldist +
							fabs(fli3) * crossdist3 /
							totaldist +
							fabs(fli4) * crossdist4 /
							totaldist;
						rcx5 = rcx1 * mindist /
							totaldist +
							rcx2 * crossdist2 /
							totaldist +
							rcx3 * crossdist3 /
							totaldist +
							rcx4 * crossdist4 /
							totaldist;
						if ((fli1 < 0.0 || fli2 < 0.0) &&
							(fli3 < 0.0 || fli4 < 0.0))
							fli5 *= -1.0;
					}
				}
			}
		}
		SetPerimeter2(writecount++, xpte, ypte, ros5, fli5, rcx5);
		if (CrossFound) 	 // if crosses
		{
			if (testdist != 0.0)
			{
				pointcountn = CrossNext;
				CrossLastN = pointtoward;   	// switch crossed spans
				pointtoward = pointcount;
				pointcountl = pointcount = CrossSpan;
				CrossLastL = pointtoward;
			}
			else			// if crosses last time but not now
			{
				CrossFound = 0;
				CrossLastL = -1;//CrossSpan=-1;
				CrossLastN = -1;//CrossNext=-1;
				pointcount = pointtoward;
				if (Reverse) // if(Direction)
				{
					pointcountl = pointcount + 1;
					pointcountn = pointcount - 1;
				}
				else
				{
					pointcountl = pointcount - 1;
					pointcountn = pointcount + 1;
				}
				Direction = Reverse;
				if (pointcountn == numpts)
					pointcountn = 0;
				else if (pointcountn < 0)
					pointcountn = numpts - 1;
				if (pointcountl == numpts)
					pointcountl = 0;
				else if (pointcountl < 0)
					pointcountl = numpts - 1;
			}
		}
		else				// if no crosses last & this time through
		{
			if (Direction)
			{
				pointcountn--;
				pointcountl = pointcount;
				pointcount--;
			}
			else
			{
				pointcountn++;
				pointcountl = pointcount;
				pointcount++;
			}
			if (pointcountn == numpts)
				pointcountn = 0;
			else if (pointcountn < 0)
				pointcountn = numpts - 1;
			if (pointcount == numpts)
				pointcount = 0;
			else if (pointcount < 0)
				pointcount = numpts - 1;
		}
		xptn = GetPerimeter1Value(CurrentFire, pointcountn, XCOORD);
		yptn = GetPerimeter1Value(CurrentFire, pointcountn, YCOORD);
		xptl = xpt;
		yptl = ypt;
		xpt = xpte;
		ypt = ypte;
		ros1 = ros5;
		fli1 = fli5;
		if ((pointcount == 0 && !CrossFound) || writecount >= writelimit)
			Terminate = true;
	}
	while (!Terminate);
	SetNumPoints(CurrentFire, writecount - 1);
	//	rediscretize(&CurrentFire);
}


//============================================================================
bool StandardizePolygon::Cross( Point Pt1, Point Pt2,
                                Point NextPt1, Point NextPt2, Point *NewPt,
                                long *dup1, long *dup2 )
{ //StandardizePolygon::Cross
  double new_x, new_y;
  bool ret = Cross( Pt1.GetX(), Pt1.GetY(), Pt2.GetX(), Pt2.GetY(),
                    NextPt1.GetX(), NextPt1.GetY(),
                    NextPt2.GetX(), NextPt2.GetY(),
                    &new_x, &new_y, dup1, dup2 );
  NewPt->Set( new_x, new_y );

  return ret;
} //StandardizePolygon::Cross

//============================================================================
bool StandardizePolygon::Cross( double xpt1, double ypt1,
                                double xpt2, double ypt2,
                                double xpt1n, double ypt1n,
                                double xpt2n, double ypt2n,
                                double *newx, double *newy,
                                long *dup1, long *dup2 )
{ //StandardizePolygon::Cross
  double xdiff1, ydiff1, xdiff2, ydiff2, ycept1, ycept2;
  double slope1, slope2, ycommon, xcommon;
  bool intersection = false, OnSpan1, OnSpan2;

  if( *dup1 != 0 ) OnSpan1 = true;  // success only if crosspt is within Span1
  else OnSpan1 = false;
  if( *dup2 != 0 ) OnSpan2 = true;  // success only if crosspt is within Span2
  else OnSpan2 = false;

  xdiff1 = xpt2 - xpt1;
  ydiff1 = ypt2 - ypt1;
  if( fabs(xdiff1) < 1e-9 ) xdiff1 = 0.0;
  if( xdiff1 != 0.0 ) {
    slope1 = ydiff1 / xdiff1;
    ycept1 = ypt1 - ( slope1 * xpt1 );
  }
  else {
    slope1 = 1.0;
    ycept1 = xpt1;
  }
  xdiff2 = xpt2n - xpt1n;
  ydiff2 = ypt2n - ypt1n;
  if( fabs(xdiff2) < 1e-9 ) xdiff2 = 0.0;
  if( xdiff2 != 0.0 ) {
    slope2 = ydiff2 / xdiff2;
    ycept2 = ypt1n - ( slope2 * xpt1n );
  }
  else {
    slope2 = 1.0;  // SLOPE NON-ZERO
    ycept2 = xpt1n;
  }
  *dup1 = *dup2 = 0;

  if( fabs(slope1 - slope2) < 1e-9 ) {
    if( fabs(ycept1 - ycept2) < 1e-9 ) {
      if( xdiff1 == 0.0 && xdiff2 == 0.0 ) {
        if( OnSpan1 && OnSpan2 ) {
          if( (ypt1 <= ypt1n && ypt1 > ypt2n) ||
              (ypt1 >= ypt1n && ypt1 < ypt2n) ) {
            *dup1 = -1;
            intersection = true;
            *newx = xpt1;
            *newy = ypt1;
          }
          if( (ypt1n <= ypt1 && ypt1n > ypt2) ||
              (ypt1n >= ypt1 && ypt1n < ypt2) ) {
            *dup2 = -1;
            intersection = true;
            *newx = xpt1n;
            *newy = ypt1n;
          }
        }
        else if( OnSpan1 ) {
          if( (ypt1n <= ypt1 && ypt1n > ypt2) ||
              (ypt1n >= ypt1 && ypt1n < ypt2) ) {
            *dup2 = -1;
            intersection = true;
            *newx = xpt1n;
            *newy = ypt1n;
          }
        }
        else if( OnSpan2 ) {
          if( (ypt1 <= ypt1n && ypt1 > ypt2n) ||
              (ypt1 >= ypt1n && ypt1 < ypt2n) ) {
            *dup1 = -1;
            intersection = true;
            *newx = xpt1;
            *newy = ypt1;
          }
        }
      }
      else {
        if( OnSpan1 && OnSpan2 ) {
          if( (xpt1 <= xpt1n && xpt1 > xpt2n) ||
              (xpt1 >= xpt1n && xpt1 < xpt2n) ) {
            *dup1 = -1;
            intersection = true;
            *newx = xpt1;
            *newy = ypt1;
          }
          if( (xpt1n <= xpt1 && xpt1n > xpt2) ||
              (xpt1n >= xpt1 && xpt1n < xpt2) ) {
            *dup2 = -1;
            intersection = true;
            *newx = xpt1n;
            *newy = ypt1n;
          }
        }
        else if( OnSpan1 ) {
          if( (xpt1n <= xpt1 && xpt1n > xpt2) ||
              (xpt1n >= xpt1 && xpt1n < xpt2) ) {
            *dup2 = -1;
            intersection = true;
            *newx = xpt1n;
            *newy = ypt1n;
          }
        }
        else if( OnSpan2 ) {
          if( (xpt1 <= xpt1n && xpt1 > xpt2n) ||
              (xpt1 >= xpt1n && xpt1 < xpt2n) ) {
            *dup1 = -1;
            intersection = true;
            *newx = xpt1;
            *newy = ypt1;
          }
        }
      }
    }
  }
  else {
    if( xdiff1 != 0.0 && xdiff2 != 0.0 ) {
      xcommon = ( ycept2 - ycept1 ) / ( slope1 - slope2 );
      *newx = xcommon;
      ycommon = ycept1 + slope1 * xcommon;
      *newy = ycommon;
      if( OnSpan1 && OnSpan2 ) {
        if( (*newx >= xpt1 && *newx < xpt2) ||
            (*newx <= xpt1 && *newx > xpt2) ) {
          *dup1 = 1;
          if( (*newx >= xpt1n && *newx < xpt2n) ||
              (*newx <= xpt1n && *newx > xpt2n) ) {
            *dup2 = 1;
            intersection = true;
          }
        }
      }
      else if( OnSpan1 ) {
        if( (*newx >= xpt1 && *newx < xpt2) ||
            (*newx <= xpt1 && *newx > xpt2) ) {
          *dup1 = 1;
          intersection = true;
        }
      }
      else if( OnSpan2 ) {
        if( (*newx >= xpt1n && *newx < xpt2n) ||
            (*newx <= xpt1n && *newx > xpt2n) ) {
          *dup2 = 1;
          intersection = true;
        }
      }
    }
    else if( xdiff1 == 0.0 && xdiff2 != 0.0 ) {
      ycommon = slope2 * xpt1 + ycept2;
      *newx = xpt1;
      *newy = ycommon;
      if( OnSpan1 && OnSpan2 ) {
        if( (*newy >= ypt1 && *newy < ypt2) ||
            (*newy <= ypt1 && *newy > ypt2) ) {
          *dup1 = 1;
          if( (*newx >= xpt1n && *newx < xpt2n) ||
              (*newx <= xpt1n && *newx > xpt2n) ) {
            intersection = true;
            *dup2 = 1;
          }
        }
      }
      else if( OnSpan1 ) {
        if( (*newy >= ypt1 && *newy < ypt2) ||
          (*newy <= ypt1 && *newy > ypt2) ) {
          *dup1 = 1;
          intersection = true;
        }
      }
      else if( OnSpan2 ) {
        if( (*newx >= xpt1n && *newx < xpt2n) ||
            (*newx <= xpt1n && *newx > xpt2n) ) {
          intersection = true;
          *dup2 = 1;
        }
      }
    }
    else if( xdiff2 == 0.0 && xdiff1 != 0.0 ) {
      ycommon = slope1 * xpt1n + ycept1;
      *newx = xpt1n;
      *newy = ycommon;
      if( OnSpan1 && OnSpan2 ) {
        if( (*newy >= ypt1n && *newy < ypt2n) ||
            (*newy <= ypt1n && *newy > ypt2n) ) {
          *dup2 = 1;
          if( (*newx >= xpt1 && *newx < xpt2) ||
              (*newx <= xpt1 && *newx > xpt2) ) {
            intersection = true;
            *dup1 = 1;
          }
        }
      }
      else if( OnSpan1 ) {
        if( (*newx >= xpt1 && *newx < xpt2) ||
            (*newx <= xpt1 && *newx > xpt2) ) {
          intersection = true;
          *dup1 = 1;
        }
      }
      else if( OnSpan2 ) {
        if( (*newy >= ypt1n && *newy < ypt2n) ||
            (*newy <= ypt1n && *newy > ypt2n) ) {
          *dup2 = 1;
          intersection = true;
        }
      }
    }

    if( intersection ) {
      if( sqrt(pow2(*newx - xpt1) + pow2(*newy - ypt1)) < 1e-9 ) *dup1 = -1;
      if( sqrt(pow2(*newx - xpt1n) + pow2(*newy - ypt1n)) < 1e-9 ) *dup2 = -1;
    }
  }

  return intersection;
} //StandardizePolygon::Cross


void StandardizePolygon::DensityControl(long CurrentFire)
{
	long i;
	long NewPts = 0, NumPts = GetNumPoints(CurrentFire);
	double xpt, ypt, xptn, yptn, newx, newy, newrcx;
	double ros, fli, rosn, flin, newr, newf, rcx, rcxn;
	double dist, diffx, diffy, testdist2;

	testdist2 = pow2(GetDistRes() * MetricResolutionConvert());
	if (testdist2 == 0)
	{
		SwapFirePerims(-1, CurrentFire);
		return;
	}

	if (testdist2 > 25000.0)
		testdist2 = 25000.0;
	AllocPerimeter2(2 * NumPts);
	xpt = GetPerimeter1Value(CurrentFire, NumPts - 1, XCOORD);
	ypt = GetPerimeter1Value(CurrentFire, NumPts - 1, YCOORD);
	ros = GetPerimeter1Value(CurrentFire, NumPts - 1, ROSVAL);
	fli = GetPerimeter1Value(CurrentFire, NumPts - 1, FLIVAL);
	rcx = GetPerimeter1Value(CurrentFire, NumPts - 1, RCXVAL);
	for (i = 0; i < NumPts; i++)
	{
		xptn = GetPerimeter1Value(CurrentFire, i, XCOORD);
		yptn = GetPerimeter1Value(CurrentFire, i, YCOORD);
		rosn = GetPerimeter1Value(CurrentFire, i, ROSVAL);
		flin = GetPerimeter1Value(CurrentFire, i, FLIVAL);
		rcxn = GetPerimeter1Value(CurrentFire, i, RCXVAL);
		diffx = pow2(xpt - xptn);
		diffy = pow2(ypt - yptn);
		dist = diffx + diffy;
		if (dist > testdist2)
		{
			newx = xpt - (xpt - xptn) * 0.5;
			newy = ypt - (ypt - yptn) * 0.5;
			rosn = GetPerimeter1Value(CurrentFire, i, ROSVAL);
			flin = GetPerimeter1Value(CurrentFire, i, FLIVAL);
			newr = (ros + rosn) / 2.0;
			newf = (fli + flin) / 2.0;
			newrcx = (rcx + rcxn) / 2.0;
			SetPerimeter2(NewPts++, newx, newy, newr, newf, newrcx);
		}
		SetPerimeter2(NewPts++, xptn, yptn, rosn, flin, rcxn);
		xpt = xptn;
		ypt = yptn;
		ros = rosn;
		fli = flin;
	}
	SetNumPoints(CurrentFire, NewPts);
	/*

		long i, j, k, m;//count, count2, count3, count4;
		long NumPts=GetNumPoints(CurrentFire);
		double testdist1, testdist2, propdist;
		double xpt1, ypt1, xpt2, ypt2, xpt3, ypt3;
		double ros1, fli1, ros2, fli2;
		double newx, newy, newr, newf;

		//FreePerimeter2();
		AllocPerimeter2(2*NumPts);
		xpt1=GetPerimeter1Value(CurrentFire, 0, XCOORD);
		ypt1=GetPerimeter1Value(CurrentFire, 0, YCOORD);
		ros1=GetPerimeter1Value(CurrentFire, 0, ROSVAL);
		fli1=GetPerimeter1Value(CurrentFire, 0, FLIVAL);
		j=0;//count2=0;
		for(i=0; i<NumPts; i++)
		{    SetPerimeter2(j++, xpt1, ypt1, ros1, fli1);
			k=i+1;
			m=i+2;
			if(m==NumPts)
				m=0;
			else if(k==NumPts)
			{	k=0;
				m=1;
			}
			xpt2=GetPerimeter1Value(CurrentFire, k, XCOORD);
			ypt2=GetPerimeter1Value(CurrentFire, k, YCOORD);
			ros2=GetPerimeter1Value(CurrentFire, k, ROSVAL);
			fli2=GetPerimeter1Value(CurrentFire, k, FLIVAL);
			xpt3=GetPerimeter1Value(CurrentFire, m, XCOORD);
			ypt3=GetPerimeter1Value(CurrentFire, m, YCOORD);
			testdist1=sqrt(pow2(xpt1-xpt2)+pow2(ypt1-ypt2));
			testdist2=sqrt(pow2(xpt2-xpt3)+pow2(ypt2-ypt3));
			if(testdist2<testdist1/2.0)
			{    propdist=testdist2/testdist1;
				newx=xpt2-(xpt2-xpt1)*propdist;
				newy=ypt2-(ypt2-ypt1)*propdist;
				newr=ros2*propdist+ros1*(1.0-propdist);
				newf=fli2*propdist+fli1*(1.0-propdist);
				SetPerimeter2(j++, newx, newy, newr, newf);
			}
			xpt1=xpt2;
			ypt1=ypt2;
		}
		SetNumPoints(CurrentFire, j);
	*/
}


/*void StandardizePolygon::DensityControl(long CurrentFire)
{
	long count, count2, count3;
	long NumPts=GetNumPoints(CurrentFire);
	double testdist, mindist, newdist, totaldist=0.0, propdist;
	double xpt1, ypt1, xpt2, ypt2;
	double ros1, fli1, ros2, fli2;
	double newx, newy, newr, newf;
	double SegmentLength, SegDist, NumSegments, WholeSegments;

	xpt2=GetPerimeter1Value(CurrentFire, NumPts-1, 0);
	ypt2=GetPerimeter1Value(CurrentFire, NumPts-1, 1);
	xpt1=GetPerimeter1Value(CurrentFire, 0, 0);
	ypt1=GetPerimeter1Value(CurrentFire, 0, 1);
	mindist=totaldist=20.0; //sqrt(pow2(xpt1-xpt2)+pow2(ypt1-ypt2));
	for(count=1; count<NumPts; count++)
	{    xpt2=GetPerimeter1Value(CurrentFire, count, 0);
		ypt2=GetPerimeter1Value(CurrentFire, count, 1);
		testdist=sqrt(pow2(xpt1-xpt2)+pow2(ypt1-ypt2));
		if(testdist<mindist)
			mindist=testdist;
		totaldist+=testdist;
		xpt1=xpt2;
		ypt1=ypt2;
	}
	FreePerimeter2();
	AllocPerimeter2(totaldist/mindist+NumPts);
	xpt1=GetPerimeter1Value(CurrentFire, 0, 0);
	ypt1=GetPerimeter1Value(CurrentFire, 0, 1);
	ros1=GetPerimeter1Value(CurrentFire, 0, 2);
	fli1=GetPerimeter1Value(CurrentFire, 0, 3);
	count2=0;
	for(count=1; count<=NumPts; count++)
	{    SetPerimeter2(count2++, xpt1, ypt1, ros1, fli1);
		if(count==NumPts)
		{	xpt2=GetPerimeter1Value(CurrentFire, 0, 0);
			ypt2=GetPerimeter1Value(CurrentFire, 0, 1);
			ros2=GetPerimeter1Value(CurrentFire, 0, 2);
			fli2=GetPerimeter1Value(CurrentFire, 0, 3);
		}
		else
		{	xpt2=GetPerimeter1Value(CurrentFire, count, 0);
			ypt2=GetPerimeter1Value(CurrentFire, count, 1);
			ros2=GetPerimeter1Value(CurrentFire, count, 2);
			fli2=GetPerimeter1Value(CurrentFire, count, 3);
		}
		testdist=sqrt(pow2(xpt1-xpt2)+pow2(ypt1-ypt2));
		NumSegments=testdist/mindist;
		if(NumSegments>=1.75)
		{    newdist=0.0;
			SegDist=modf(NumSegments, &WholeSegments);
			if(SegDist<=0.25)
				WholeSegments-=1.0;
			SegmentLength=testdist/WholeSegments;
			for(count3=0; count3<NumSegments; count3++)
			{    newdist+=SegmentLength;
				propdist=SegmentLength/testdist;
				newx=xpt1-(xpt1-xpt2)*propdist;
				newy=ypt1-(ypt1-ypt2)*propdist;
				newr=ros1*propdist+ros2*(1.0-propdist);
				newf=fli1*propdist+fli2*(1.0-propdist);
				SetPerimeter2(count2++, newx, newy, newr, newf);
			};
		}
		xpt1=xpt2;
		ypt1=ypt2;
		ros1=ros2;
		fli1=fli2;
	}
	SetNumPoints(CurrentFire, count2);
}
*/

/*

void Intersections::ReverseSpikeIntersectionOrder(long CurrentFire)
{
	double xpt1, ypt1; //, xpt2, ypt2;
	double ros1, fli1; //ros2,  fli2;
	long count, count2, count3, count4, count5, numsame;
	long samecheck1, ycheck; //, samecheck2
	double samepoint1x=0, samepoint1y=0;//, samepoint2x=0, samepoint2y=0;
	double distmin, distcalc, xdiff, ydiff;

	typedef struct
	{
		long order;
		long number;
		long ycheck;
		double xpt, ypt, ros, fli;
		double dist;
	}  Distances;
	Distances* distances;

	for(count=0; count<numcross; count++)
	{	samecheck1=GetSpan(count, 0);
		count2=count+1;
		while(samecheck1==GetSpan(count2, 0))
			count2++;
		numsame=count2-count;
		if(numsame>1)
		{	distances=(Distances *) calloc(numsame, sizeof(Distances));
			xpt1=GetPerimeter1Value(CurrentFire, samecheck1, 0);
			ypt1=GetPerimeter1Value(CurrentFire, samecheck1, 1);
			for(count3=count; count3<count2; count3++)
			{	GetInterPointCoord(count3, &samepoint1x, &samepoint1y);
				GetInterPointFChx(count3, &ros1, &fli1);
				ycheck=GetSpan(count3, 1);
				xdiff=pow2(xpt1-samepoint1x);
				ydiff=pow2(ypt1-samepoint1y);
				distcalc=xdiff+ydiff;
				count4=count3-count;
				distances[count4].number=count3;
				distances[count4].dist=distcalc;
				distances[count4].xpt=samepoint1x;
				distances[count4].ypt=samepoint1y;
				distances[count4].ros=ros1;
				distances[count4].fli=fli1;
				distances[count4].ycheck=ycheck;
				if(count3==count)
				{	distmin=distcalc;
					distances[count4].order=1;
				}
				else if(distcalc<distmin)
				{	distmin=distcalc;
					distances[count4].order=1;
					for(count5=0; count5<count4-1; count5++)
					{	distances[count5].order+=1;
					}
				}
			}
			count5=1;
			for(count3=count; count3<count2; count3++)
			{    for(count4=0; count4<numsame; count4++)
				{	if(distances[count4].order==count5)
					{	SetIntersection(count3, samecheck1, distances[count4].ycheck);
						SetInterPoint(count3, distances[count4].xpt, distances[count4].ypt,
									  distances[count4].ros, distances[count4].fli);
						count5++;
						break;
					}
				}
			}
			free(distances);
		}
		count=count2-1;
	}
}

*/



/*
void Intersections::InfuseIntersectionsIntoPerimeter(CurrentFire)
{
	double xpt1, ypt1, ros1, fli1;
	double xpt1n, ypt1n, ros1n, fli1n;
	double xpta, ypta, rosa, flia;
	long pointcount, pointcountn=1, intercount=0;
	long xcount=0, xcount1, xcount2;
	long numpts=GetNumPoints(CurrentFire);

	FreePerimeter2();
	AllocPerimeter2(numpts+numcross);
	ReverseSpikeIntersectionOrder(CurrentFire);

	xpt1=GetPerimeter1Value(CurrentFire, pointcount, 0);
	ypt1=GetPerimeter1Value(CurrentFire, pointcount, 1);
	ros1=GetPerimeter1Value(CurrentFire, pointcount, 2);
	fli1=GetPerimeter1Value(CurrentFire, pointcount, 3);

	for(pointcount=0; pointcount<numpts; pointcount++)
	{    xpt1n=GetPerimeter1Value(CurrentFire, pointcountn, 0);
		ypt1n=GetPerimeter1Value(CurrentFire, pointcountn, 1);
		ros1n=GetPerimeter1Value(CurrentFire, pointcountn, 2);
		fli1n=GetPerimeter1Value(CurrentFire, pointcountn, 3);
		SetPerimeter2(pointcount+intercount, xpt1, ypt1, ros1, fli1);

		while(xcount1<=pointcount)
		{	GetIntersection(xcount, &xcount1, &xcount2);
			xcount++;
			if(xcount1==pointcount)
			{    intercount++;
				GetInterPointCoord(xcount, &xpta, &ypta);
				GetInterPointFChx(xcount, &rosa, &flia);
				SetPerimeter2(pointcount+intercount, xpt1, ypt1, ros1, fli1);
			}
		}
		xpt1=xpt1n;
		ypt1=ypt1n;
		ros1=ros1n;
		fli1=fli1n;
		pointcountn++;
	}
}
*/
