/*============================================================================
  PerimeterPoint.cpp

  This is a C++ class file for the PerimeterPoint class, a component of
  FarsiteCL - RFL version.

  Based on Version 4.1.0 (12/21/2004) by Mark Finney.

  Revisions by John Benoit - Apr 2010

  See LICENSE.TXT file for license information.
  ============================================================================
*/
#include"PerimeterPoint.h"

//============================================================================
PerimeterPoint::PerimeterPoint() : Point() {}

//============================================================================
PerimeterPoint::PerimeterPoint( double x, double y ) : Point( x, y )
{ //PerimeterPoint::PerimeterPoint
} //PerimeterPoint::PerimeterPoint

//============================================================================
void   PerimeterPoint::Get( double *A )
{ //PerimeterPoint::Get
  *A = x;
  *(++A) = y;
  *(++A) = ROS;
  *(++A) = FLI;
  *(++A) = RCX;
} //PerimeterPoint::Get

//============================================================================
PerimeterPoint& PerimeterPoint::operator=( const PerimeterPoint &RHS )
{ //PerimeterPoint::operator=
  this->x = RHS.x;
  this->y = RHS.y;
  this->ROS = RHS.ROS;
  this->FLI = RHS.FLI;
  this->RCX = RHS.RCX;

  return *this;
} //PerimeterPoint::operator=