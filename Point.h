/*============================================================================
  Point.h

  This is a C++ header file for the Point class, a component of
  FarsiteCL - RFL version.

  Based on Version 4.1.0 (12/21/2004) by Mark Finney.

  Revisions by John Benoit - Apr 2020

  See LICENSE.TXT file for license information.
  ============================================================================
*/
/*============================================================================
  Point
  This class describes a 2-dimensional point.
*/
#ifndef POINT_H
#define POINT_H

#define POINT_CLOCKWISE        -1
#define POINT_COUNTERCLOCKWISE  1
#define POINT_STRAIGHT_LINE     0
#define POINT_EPSILON           1e-6

class Point
{ //Point
  private:
    double X, Y;

  public:
    Point();
    Point( double X, double Y );
    ~Point();
    int    GetOrientation( Point &PA, Point &PB );
    double CalcDist( Point &P );
    double CalcDistSq( Point &P );
    Point& operator=( const Point *RHS );
    Point& operator=( const Point &RHS );
    double GetX() { return X; }
    double GetY() { return Y; }
    void   Set( double X, double Y );
};//Point

#endif
