/*============================================================================
  PerimeterPoint.cpp

  This is a C++ class file for the PerimeterPoint class, a component of
  FarsiteCL - RFL version.

  Based on Version 4.1.0 (12/21/2004) by Mark Finney.

  Revisions by John Benoit - Apr 2010

  See LICENSE.TXT file for license information.
  ============================================================================
*/
#include<iostream>
#include"PerimeterPoint.h"

//============================================================================
PerimeterPoint::PerimeterPoint() : Point() {}

//============================================================================
PerimeterPoint::PerimeterPoint( double x, double y ) : Point( x, y )
{ //PerimeterPoint::PerimeterPoint
} //PerimeterPoint::PerimeterPoint

//============================================================================
PerimeterPoint::PerimeterPoint( PerimeterPoint *P )
{ //PerimeterPoint::PerimeterPoint
  this->Point::operator=( P );  //Call base class (Point) = operator
  this->ROS = P->ROS;
  this->FLI = P->FLI;
  this->React = P->React;
} //PerimeterPoint::PerimeterPoint

//============================================================================
PerimeterPoint::~PerimeterPoint()
{ //PerimeterPoint::~PerimeterPoint
} //PerimeterPoint::~PerimeterPoint

//============================================================================
PerimeterPoint& PerimeterPoint::operator=( const PerimeterPoint &RHS )
{ //PerimeterPoint::operator=
  this->Point::operator=( RHS );  //Call base class (Point) = operator
  this->ROS = RHS.ROS;
  this->FLI = RHS.FLI;
  this->React = RHS.React;

  return *this;
} //PerimeterPoint::operator=

//============================================================================
void   PerimeterPoint::Set( PerimeterPoint &From )
{ //PerimeterPoint::Set
  Point::operator=( From );  //Call the base class (Point) copy operator
  this->ROS = From.ROS;
  this->FLI = From.FLI;
  this->React = From.React;
} //PerimeterPoint::Set
//
//============================================================================
void   PerimeterPoint::Set( PerimeterPoint *From )
{ //PerimeterPoint::Set
  Point::Set( From->GetX(), From->GetY() );
  this->ROS = From->ROS;
  this->FLI = From->FLI;
  this->React = From->React;
} //PerimeterPoint::Set

//============================================================================
void   PerimeterPoint::Get( double *Values )
{ //PerimeterPoint::Get
  *Values = GetX();
  *(++Values) = GetY();
  *(++Values) = ROS;
  *(++Values) = FLI;
  *(++Values) = React;
} //PerimeterPoint::Get

//============================================================================
double PerimeterPoint::Get( int ValueType )
{ //PerimeterPoint::Get
  switch ( ValueType ) {
    case   X_VAL: return GetX();
    case   Y_VAL: return GetY();
    case ROS_VAL: return ROS;
    case FLI_VAL: return FLI;
    case RCX_VAL: return React;
  }

  return 0;
} //PerimeterPoint::Get

//============================================================================
void PerimeterPoint::SetLoc( double X, double Y )
{ //PerimeterPoint::SetLoc
  Point::Set( X, Y );
} //PerimeterPoint::SetLoc

//============================================================================
void PerimeterPoint::SetCharacteristics( double ROS, double FLI )
{ //PerimeterPoint::SetCharacteristics
  this->ROS = ROS;
  this->FLI = FLI;
} //PerimeterPoint::SetCharacteristics

//============================================================================
void PerimeterPoint::SetReact( double React )
{ //PerimeterPoint::SetReact
  this->React = React;
} //PerimeterPoint::SetReact

//============================================================================
void PerimeterPoint::Set( double X, double Y, double ROS, double FLI,
                          double React )
{ //PerimeterPoint::Set
  Point::Set( X, Y );
  this->ROS = ROS;
  this->FLI = FLI;
  this->React = React;
} //PerimeterPoint::Set

//============================================================================
//PerimeterPoint::Set
//Note: Values MUST be in this order: X, Y, ROS, FLI, React
void   PerimeterPoint::Set( double *Values )
{ //PerimeterPoint::Set
  Point::Set( *Values, *(++Values) );
  this->ROS = *(++Values);
  this->FLI = *(++Values);
  this->React = *(++Values);
} //PerimeterPoint::Set

   void   SetFLI( double Value );
//============================================================================
//PerimeterPoint::SetFLI
void   PerimeterPoint::SetFLI( double Value )
{ //PerimeterPoint::SetFLI
  this->FLI = Value;
} //PerimeterPoint::SetFLI
