/*============================================================================
  Point.h

  This is a C++ header file for the Point class, a component of
  FarsiteCL - RFL version.

  Based on Version 4.1.0 (12/21/2004) by Mark Finney.

  Revisions by John Benoit - Apr 2010

  See LICENSE.TXT file for license information.
  ============================================================================
*/
/*============================================================================
  Point
  This class describes a 2-dimensional point.
  * Data members are purposely made public for increased speed.
*/
#ifndef POINT_H
#define POINT_H

#define POINT_CLOCKWISE        -1
#define POINT_COUNTERCLOCKWISE  1
#define POINT_STRAIGHT_LINE     0
#define POINT_EPSILON           1e-6

class Point
{ //Point
public:
  double x, y;

  Point();
  Point( double x, double y );
  int    GetOrientation( Point &PA, Point &PB );
  double CalcDist( Point &P );
  double CalcDistSq( Point &P );
  //Sep2010: Below: first version okay on MacBook, but g++ in Ubuntu likes the
  //'unqualified' version. If you compile with the first version of the line,
  //g++ complains: 'extra qualification ... on member ...'.
  //Point& Point::operator=( const Point &RHS ); 
  Point& operator=( const Point &RHS );
};//Point

#endif