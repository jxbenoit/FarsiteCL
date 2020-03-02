/*============================================================================
  crossthread.cpp
  FARSITE 4.0
  10/20/2000
  Mark A. Finney (USDA For. Serv., Fire Sciences Laboratory)

  Contains functions for threading the comparisons of crossing segements of
  a fire perimeter (declarations in fsx4.h).

  See LICENSE.TXT file for license information.
  ============================================================================
*/
#include<string.h>
#include"globals.h"
#include"fsx4.h"
#include"fsglbvar.h"

//============================================================================
CrossThread::CrossThread()
{ //CrossThread::CrossThread
  NumCross = 0;
  NumAlloc = 0;
  intersect = 0;
  interpoint = 0;
  hXEvent = 0;
  ThreadOrder = -1;
  CurrentFire = NextFire = 0;
} //CrossThread::CrossThread

//============================================================================
CrossThread::~CrossThread()
{ //CrossThread::~CrossThread
  FreeCross();
} //CrossThread::~CrossThread

//============================================================================
bool CrossThread::AllocCross( long Num )
{ //CrossThread::AllocCross
  if( Num < NumAlloc ) return true;

  FreeCross();
  if( (intersect = new long[Num * 2]) == NULL ) return false;
  if( (interpoint = new double[Num * NUMDATA]) == NULL ) {
    FreeCross();
    return false;
  }
  NumAlloc = Num;
  NumCross = 0;

  return true;
} //CrossThread::AllocCross


bool CrossThread::ReallocCross(long Num)
{
	long OldNum, OldNumCross;
	long* tempisect;
	double* tempipt;

	if( intersect ) {
		tempisect = new long[NumAlloc * 2];
		if( tempisect == NULL ) return false;
		memcpy(tempisect, intersect, NumAlloc * 2 * sizeof(long));
	}
	if( interpoint ) {
		tempipt = new double[NumAlloc * NUMDATA];
		if( tempipt == NULL ) {
			delete[] tempisect;

			return false;
		}
		memcpy(tempipt, interpoint, NumAlloc * NUMDATA * sizeof(double));
	}
	OldNum = NumAlloc;
	OldNumCross = NumCross;
	FreeCross();
	AllocCross(Num);
	memcpy(intersect, tempisect, OldNum * 2 * sizeof(long));
	memcpy(interpoint, tempipt, OldNum * NUMDATA * sizeof(double));
	NumCross = OldNumCross;
	delete[] tempisect;
	delete[] tempipt;

	return true;
}

//============================================================================
void CrossThread::FreeCross()
{ //CrossThread::FreeCross
  if( intersect ) delete[] intersect;
  if( interpoint ) delete[] interpoint;
  intersect = 0;
  interpoint = 0;
  NumAlloc = 0;
  NumCross = 0;
} //CrossThread::FreeCross

//============================================================================
void CrossThread::StartCrossThread( long threadorder, long currentfire,
                                    long nextfire )
{ //CrossThread::StartCrossThread
  CallLevel++;

  CurrentFire = currentfire;
  NextFire = nextfire;

  if( Verbose >= CallLevel )
    printf( "%*scrossthread:CrossThread::StartCrossThread:1 %ld\n",
              CallLevel, "", SpanAEnd );

  static_cast <CrossThread*> (this)->CrossCompare();

  if( Verbose >= CallLevel )
    printf( "%*scrossthread:CrossThread::StartCrossThread:2\n",
            CallLevel, "" );

  CallLevel--;
} //CrossThread::StartCrossThread

//============================================================================
void CrossThread::SetInterPoint( long Number, double XCoord, double YCoord,
                                 double Ros, double Fli, double Rct )
{ //CrossThread::SetInterPoint
  if( Number >= NumAlloc ) ReallocCross( Number * 2 );

  Number *= NUMDATA;
  if( XCoord == 0 || YCoord == 0 ) XCoord = YCoord = 1;
  interpoint[Number] = XCoord;
  interpoint[++Number] = YCoord;
  interpoint[++Number] = Ros;
  interpoint[++Number] = Fli;
  interpoint[++Number] = Rct;
} //CrossThread::SetInterPoint

/*============================================================================
  CrossThread::CrossCompare
  Determines which points crossed into already burned areas.
*/
void CrossThread::CrossCompare()
{ //CrossThread::CrossCompare
  CallLevel++;

  long i, j, k, m, n, dup1, dup2;
  double xpt, ypt, xptl, yptl, xptn, yptn, newx, newy;
  double crxpt, crypt, crxptl, cryptl, crxptn, cryptn;
  double ros1, ros2, ros3, fli1, fli2, fli3, fli4;
  double rcx1, rcx2, rcx3, xg, yg;

  if( Verbose >= CallLevel )
    printf( "%*scrossthread:CrossThread:CrossCompare:1 %ld NumCross=%ld\n",
            CallLevel, "", SpanAEnd, NumCross );

  long CrossMemAlloc = GetNumPoints( CurrentFire ) + GetNumPoints( NextFire );

  if( Verbose >= CallLevel )
    printf( "%*scrossthread:CrossThread:CrossCompare:2 NumCross=%ld\n",
            CallLevel, "", NumCross );

  NumCross = 0;
  SpanBEnd = GetNumPoints( NextFire );
  AllocCross( CrossMemAlloc );   //Allocate array of intersection orders

  if( Verbose >= CallLevel )
    printf( "%*scrossthread:CrossThread:CrossCompare:3 "
            "NumCross=%ld SpanAStart=%ld SpanAEnd=%ld SpanBEnd=%ld "
            "CurrentFire=%ld NextFire=%ld\n",
            CallLevel, "", NumCross, SpanAStart, SpanAEnd, SpanBEnd,
            CurrentFire, NextFire );

  //For each point in a given fire.
  for( i = SpanAStart; i < SpanAEnd; i++ ) {
    xpt = GetPerimeter1Value( CurrentFire, i, XCOORD );
    ypt = GetPerimeter1Value( CurrentFire, i, YCOORD );

    if( Verbose >= CallLevel )
      printf( "%*scrossthread:CrossThread:CrossCompare:3a "
              "i=%ld SpanAEnd=%ld SpanBEnd=%ld\n",
              CallLevel, "", i, SpanAEnd, SpanBEnd );

    if( i < GetNumPoints(CurrentFire) - 1 ) k = i + 1;
    else k = 0;
    xptn = GetPerimeter1Value( CurrentFire, k, XCOORD );
    yptn = GetPerimeter1Value( CurrentFire, k, YCOORD );

    //FOR ALL FIRES CHECK FOR ALL CROSSES FROM THE FIRST ELEMENT.
    for( j = 0; j < SpanBEnd; j++ ) {
      crxpt = GetPerimeter1Value( NextFire, j, XCOORD );
      crypt = GetPerimeter1Value( NextFire, j, YCOORD );
      if( j != GetNumPoints(NextFire) - 1 ) m = j + 1;
      else m = 0;

      //Don't check for crosses on same or adjacent points within same array.
      if( CurrentFire == NextFire )
        if( i == j || i == m || k == j || k == m ) continue;

      crxptn = GetPerimeter1Value( NextFire, m, XCOORD );
      cryptn = GetPerimeter1Value( NextFire, m, YCOORD );

      //Flag for forcing check of crosspt only within bounds of each span.
      dup1 = dup2 = 1;
      bool crossyn = Cross( xpt, ypt, xptn, yptn, crxpt, crypt,
                            crxptn, cryptn, &newx, &newy, &dup1, &dup2 );
      if( Verbose >= CallLevel )
        printf( "%*scrossthread:CrossThread:CrossCompare:3a1 crossyn=%d\n",
                CallLevel, "", crossyn );

      bool ptcross1 = false;
      bool ptcross2 = false;
      bool SinglePtCross = false;

      if( dup1 < 0 && dup2 < 0 ) {
        if( CurrentFire == NextFire ) ptcross1 = ptcross2 = false;
        else ptcross1 = true; //Ignore cross if only at one point
        SinglePtCross = true;
      }
      else if( dup1 < 0 ) {  //If cross duplicates xpt, ypt
        n = i - 1;
        if( n < 0 ) n = GetNumPoints( CurrentFire ) - 1;
        xptl = GetPerimeter1Value( CurrentFire, n, XCOORD );
        yptl = GetPerimeter1Value( CurrentFire, n, YCOORD );
        dup1 = 1;
        dup2 = 0;
        if( ! Cross(xptl, yptl, xptn, yptn, crxpt, crypt, crxptn, cryptn,
                    &xg, &yg, &dup1, &dup2) )
          ptcross1 = true;
        else if( dup1 < 0 ) ptcross1 = true;
        else SinglePtCross = true;
      }
      else if( dup2 < 0 ) {     //If cross duplicates crxpt, crypt
        n = j - 1;
        if( n < 0 ) n = GetNumPoints( NextFire ) - 1;
        crxptl = GetPerimeter1Value( NextFire, n, XCOORD );
        cryptl = GetPerimeter1Value( NextFire, n, YCOORD );
        dup1 = 0;
        dup2 = 1;
        if( ! Cross(xpt, ypt, xptn, yptn, crxptl, cryptl, crxptn, cryptn,
                    &xg, &yg, &dup1, &dup2) )
          ptcross2 = true;  //Means this is just a single pt cross
        else if( dup1 < 0 ) ptcross2 = true;
        else SinglePtCross = true;
      }

      if( crossyn ) {
        if( ptcross1 || ptcross2 ) continue;  //Just a single pt cross
        if( SinglePtCross ) {
          if( CurrentFire == NextFire )
            //Point of order i in first fire, j on second fire.
            SetIntersection( NumCross, -i, -j );
          else
            //Point of order i in first fire, j on second fire.
            SetIntersection( NumCross, i, j );
        }
        else
          //Point of order i in first fire, j on second fire.
          SetIntersection( NumCross, i, j );
        ros1 = GetPerimeter1Value( CurrentFire, i, ROSVAL );
        fli1 = GetPerimeter1Value( CurrentFire, i, FLIVAL );
        rcx1 = GetPerimeter1Value( CurrentFire, i, RCXVAL );
        ros2 = GetPerimeter1Value( CurrentFire, k, ROSVAL );
        fli2 = GetPerimeter1Value( CurrentFire, k, FLIVAL );
        rcx2 = GetPerimeter1Value( CurrentFire, k, RCXVAL );
        //Average rate of spread for perimeter segment.
        ros1 = ( ros1 + ros2 ) / 2.0;
        //Average fireline intensity for perimeter segment.
        fli3 = ( fabs(fli1) + fabs(fli2) ) / 2.0;
        if( fli1 < 0.0 || fli2 < 0.0 ) fli3 *= -1.0;
        fli1 = fli3;
        //Average rate of spread for perimeter segment.
        rcx1 = ( rcx1 + rcx2 ) / 2.0;
        ros2 = GetPerimeter1Value( NextFire, j, ROSVAL );
        fli2 = GetPerimeter1Value( NextFire, j, FLIVAL );
        rcx2 = GetPerimeter1Value( NextFire, j, RCXVAL );
        ros3 = GetPerimeter1Value( NextFire, m, ROSVAL );
        fli3 = GetPerimeter1Value( NextFire, m, FLIVAL );
        rcx3 = GetPerimeter1Value( NextFire, m, RCXVAL );
        //Average rate of spread for perimeter segment.
        ros2 = ( ros2 + ros3 ) / 2.0;
        //Average rate of spread for perimeter segment.
        fli4 = ( fabs(fli2) + fabs(fli3) ) / 2.0;
        if( fli2 < 0.0 || fli3 < 0.0 ) fli4 *= -1.0;
        fli2 = fli4;
        //Average rate of spread for perimeter segment.
        rcx2 = ( rcx2 + rcx3 ) / 2.0;
        ros3 = ( ros1 + ros2 ) / 2.0; //ROS for intersection point
        fli4 = ( fabs(fli1) + fabs(fli2) ) / 2.0; //FLI for intersection point
        if( fli1 < 0.0 && fli2 < 0.0 ) fli4 *= -1.0;
        fli3 = fli4;
        rcx3 = ( rcx1 + rcx2 ) / 2.0; //ROS for intersection point
        //Write intersected points.
        SetInterPoint( NumCross, newx, newy, ros3, fli3, rcx3 );
        if( GetInout(CurrentFire) == 2 && GetInout(NextFire) == 1 ) {
          if( fli2 < 0.0 ) {   //Check for stationary points on outward fire
            NumCross = 0; //If found dont merge with inward burning fire
          }
        }
        NumCross++;  //Allow outward fire to get only outward fire perimeter

        if( Verbose >= CallLevel )
          printf( "%*scrossthread:CrossThread:CrossCompare:3a1a "
                  "NumCross=%ld\n",
                  CallLevel, "", NumCross );
      }
    }  //End of j loop
  }  //End of i loop

for( i = SpanAStart; i < SpanAEnd; i++ ) {                       //AAA
  xpt = GetPerimeter1Value( CurrentFire, i, XCOORD );            //AAA
  ypt = GetPerimeter1Value( CurrentFire, i, YCOORD );            //AAA

  for( long j = SpanAStart; j < SpanAEnd; j++ ) {                 //AAA
    if( i == j || j - 1 == i || j + 1 == i ) continue; //Same point //AAA
    long iLast = i - 1;                                        //AAA
    if( iLast < SpanAStart ) iLast = SpanAEnd;                   //AAA
    long jLast = j - 1;                                        //AAA
    if( jLast < SpanAStart ) jLast = SpanAEnd;                   //AAA

    double xptiLast = GetPerimeter1Value( CurrentFire, iLast, XCOORD ); //AAA
    double yptiLast = GetPerimeter1Value( CurrentFire, iLast, YCOORD ); //AAA
    double xptj = GetPerimeter1Value( CurrentFire, j, XCOORD ); //AAA
    double yptj = GetPerimeter1Value( CurrentFire, j, YCOORD ); //AAA
    double xptjLast = GetPerimeter1Value( CurrentFire, jLast, XCOORD ); //AAA
    double yptjLast = GetPerimeter1Value( CurrentFire, jLast, YCOORD ); //AAA

    double mi = (ypt - yptiLast) / (xpt - xptiLast);             //AAA
    double mj = (yptj - yptjLast) / (xptj - xptjLast);             //AAA
    double bi = ypt - mi * xpt;                                    //AAA
    double bj = yptj - mj * xptj;                                    //AAA

    if( mi != mj ) { //If these are NOT parallel lines....         //AAA
      double x_intersect = (bj - bi) / (mi - mj);                 //AAA
      if( ((x_intersect >= xptiLast && x_intersect <= xpt) ||    //AAA
          (x_intersect >= xpt && x_intersect <= xptiLast)) &&     //AAA
          ((x_intersect >= xptjLast && x_intersect <= xptj) ||    //AAA
          (x_intersect >= xptj && x_intersect <= xptjLast))      //AAA
        ) {    //AAA
      }                                                            //AAA
    }                                                              //AAA
    else {  //Parallel lines found                                 //AAA
        printf( "PARALLEL: xpt=%lf ypt=%lf xptiLast=%lf yptiLast=%lf\n", //AAA
                xpt, ypt, xptiLast, yptiLast );                         //AAA
        printf( "           xptj=%lf yptj=%lf xptjLast=%lf yptjLast=%lf\n", //AAA
                xptj, yptj, xptjLast, yptjLast );                         //AAA
    }                                                              //AAA
  }                                                                //AAA
}                                                                //AAA

  if( Verbose >= CallLevel )
    printf( "%*scrossthread:CrossThread:CrossCompare:4 NumCross=%ld\n",
            CallLevel, "", NumCross );

  CallLevel--;
} //CrossThread::CrossCompare

//============================================================================
long CrossThread::GetNumCross() { return NumCross; }

//============================================================================
long* CrossThread::GetIsect() { return intersect; }

//============================================================================
double* CrossThread::GetIpoint() { return interpoint; }

//============================================================================
void CrossThread::SetIntersection( long Number, long XOrder, long YOrder )
{ //CrossThread::SetIntersection
  if( Number >= NumAlloc ) ReallocCross( Number * 2 );

  Number *= 2;
  intersect[Number] = XOrder;
  intersect[++Number] = YOrder;
} //CrossThread::SetIntersection

/*============================================================================
  CrossThread::Cross
  Checks if two line segments intersect. If they intersect, the intersection
  point is returned in the parameter list.
  Returns true if the segments intersect, false if not.
*/
bool CrossThread::Cross( double xpt1, double ypt1, double xpt2, double ypt2,
                         double xpt1n, double ypt1n,
                         double xpt2n, double ypt2n,
                         double *newx, double *newy, long *dup1, long *dup2 )
{ //CrossThread::Cross
  bool OnSpan1 = false, OnSpan2 = false;
  if( *dup1 != 0 ) OnSpan1 = true;  //Success only if crosspt is within Span1
  if( *dup2 != 0 ) OnSpan2 = true;  //Success only if crosspt is within Span2

  //Compute the slopes & y-intercepts for the two line segments.
  double ycept1, ycept2;
  double slope1, slope2;
  double xdiff1 = xpt2 - xpt1;
  double ydiff1 = ypt2 - ypt1;
  if( fabs(xdiff1) < 1e-9 ) {
    xdiff1 = 0.0;
    slope1 = 1.0;
    ycept1 = xpt1;
  }
  else {
    slope1 = ydiff1 / xdiff1;
    ycept1 = ypt1 - ( slope1 * xpt1 );
  }
  double xdiff2 = xpt2n - xpt1n;
  double ydiff2 = ypt2n - ypt1n;
  if( fabs(xdiff2) < 1e-9 ) {
    xdiff2 = 0.0;
    slope2 = 1.0;  //SLOPE NON-ZERO
    ycept2 = xpt1n;
  }
  else {
    slope2 = ydiff2 / xdiff2;
    ycept2 = ypt1n - ( slope2 * xpt1n );
  }

  bool intersection = false;

  *dup1 = *dup2 = 0;
  if( fabs(slope1 - slope2) < 1e-9 ) {    //If slopes are nearly parallel...
    if( fabs(ycept1 - ycept2) < 1e-9 ) {    //If line segments are colinear...
      if( xdiff1 == 0.0 && xdiff2 == 0.0 ) {  //If line segs are vertical...
        if( OnSpan2 ) {
          //If ypt1 is between the points of the other line segment
          //(i.e. the segments overlap)...
          if( (ypt1 <= ypt1n && ypt1 > ypt2n) ||
              (ypt1 >= ypt1n && ypt1 < ypt2n) ) {
            *dup1 = -1;
            intersection = true;
            *newx = xpt1;
            *newy = ypt1;
          }
        }
        if( OnSpan1 ) {
          //If ypt1n is between the points of the other line segment
          //(i.e. the segments overlap)...
          if( (ypt1n <= ypt1 && ypt1n > ypt2) ||
              (ypt1n >= ypt1 && ypt1n < ypt2) ) {
            *dup2 = -1;
            intersection = true;
            *newx = xpt1n;
            *newy = ypt1n;
          }
        }
      }
      else {  //Else if the line segs are NOT vertical...
        if( OnSpan2 ) {
          //If xpt1 is between the points of the other line segment
          //(i.e. the segments overlap)...
          if( (xpt1 <= xpt1n && xpt1 > xpt2n) ||
              (xpt1 >= xpt1n && xpt1 < xpt2n) ) {
            *dup1 = -1;
            intersection = true;
            *newx = xpt1;
            *newy = ypt1;
          }
        }
        if( OnSpan1 ) {
          //If xpt1n is between the points of the other line segment
          //(i.e. the segments overlap)...
          if( (xpt1n <= xpt1 && xpt1n > xpt2) ||
              (xpt1n >= xpt1 && xpt1n < xpt2) ) {
            *dup2 = -1;
            intersection = true;
            *newx = xpt1n;
            *newy = ypt1n;
          }
        }
      }
    }
  }
  else {  //Else if slopes are NOT parallel...
    if( xdiff1 != 0.0 && xdiff2 != 0.0 ) {    //If neither line is vertical...
      //Find the intersection point of the two lines containing the line
      //segments. The intersection *may* be within the line segments.
      *newx = ( ycept2 - ycept1 ) / ( slope1 - slope2 );
      *newy = ycept1 + slope1 * ( *newx );

      if( OnSpan1 && OnSpan2 ) {
        //Check if the intersection point is within both line segments.
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
    else if( xdiff1 == 0.0 && xdiff2 != 0.0 ) {  //If one line is vertical...
      *newx = xpt1;
      *newy = slope2 * xpt1 + ycept2;
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
    else if( xdiff2 == 0.0 && xdiff1 != 0.0 ) {  //If other line vertical...
      *newx = xpt1n;
      *newy = slope1 * xpt1n + ycept1;
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
      if( CalcDistSq(*newx - xpt1, *newy - ypt1) < 1e-18 ) *dup1 = -1;
      if( CalcDistSq(*newx - xpt1n, *newy - ypt1n) < 1e-18 ) *dup2 = -1;
    }
  }

  return intersection;
} //CrossThread::Cross
