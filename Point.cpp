/*============================================================================
  Point.cpp

  This is a C++ class file for the Point class, a component of
  FarsiteCL - RFL version.

  Based on Version 4.1.0 (12/21/2004) by Mark Finney.

  Revisions by John Benoit - Apr 2010

  See LICENSE.TXT file for license information.
  ============================================================================
*/
#include<math.h> //For sqrt
#include<iostream> //For sqrt
#include"Point.h"

//============================================================================
Point::Point() { X = Y = 0.0; }

//============================================================================
Point::Point( double X, double Y )
{ //Point::Point
  this->X = X;
  this->Y = Y;
} //Point::Point

//============================================================================
void Point::Set( double X, double Y )
{ //Point::Set
  this->X = X;
  this->Y = Y;
} //Point::Set
//
//============================================================================
Point::~Point()
{ //Point::~Point
} //Point::~Point

/*============================================================================
  Point::GetOrientation
  Given two other points, returns POINT_COUNTERCLOCKWISE (1) if Point A is
  'to the left of' Point B, returns POINT_CLOCKWISE (-1) if Point B is
  'to the right of' Point A, or returns POINT_STRAIGHT_LINE (0) if the points
  are in a straight line.
  This simply treats the two given points as vectors, with origin at the Point
  this function is called on. Then it computes the 'z' component of the cross
  product of these vectors, which is negative if Points A & B are clockwise
  about the Point this function is called on, positive if they are
  counterclockwise, and 0 if they are in a straight line.
*/
int Point::GetOrientation( Point &PA, Point &PB )
{ //Point::GetOrientation
  double Ax = PA.X - X, Ay = PA.Y - Y;
  double Bx = PB.X - X, By = PB.Y - Y;

  double det = Ax * By - Ay * Bx;
  if( det > POINT_EPSILON ) return POINT_COUNTERCLOCKWISE;
  else if( det < -POINT_EPSILON ) return POINT_CLOCKWISE;
  else return POINT_STRAIGHT_LINE;
} //Point::GetOrientation

/*============================================================================
  Point::CalcDist
  Calculates the distance from this Point to the given Point.
*/
double Point::CalcDist( Point &P )
{ //Point::CalcDist
  double xdiff = P.X - X;
  double ydiff = P.Y - Y;

  return sqrt( xdiff * xdiff + ydiff * ydiff );
} //Point::CalcDist

/*============================================================================
  Point::CalcDistSq
  Calculates the distance SQUARED from this Point to the given Point.
*/
double Point::CalcDistSq( Point &P )
{ //Point::CalcDistSq
  double xdiff = P.X - X;
  double ydiff = P.Y - Y;

  return xdiff * xdiff + ydiff * ydiff;
} //Point::CalcDistSq

//============================================================================
Point& Point::operator=( const Point &RHS )
{ //Point::operator=
  this->X = RHS.X;
  this->Y = RHS.Y;

  return *this;
} //Point::operator=
//
//============================================================================
Point& Point::operator=( const Point *RHS )
{ //Point::operator=
  this->X = RHS->X;
  this->Y = RHS->Y;

  return *this;
} //Point::operator=
