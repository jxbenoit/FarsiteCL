/*============================================================================
  FSXWUTIL.CPP - Utility functions for FARSITE
  Clip loops, Merge Fires, Rediscretize etc.

  Copyright 1994, 1995, 1996
  Mark A. Finney, Systems for Environmental Management

  See LICENSE.TXT file for license information.
  ============================================================================
*/
#include<string.h>
#include<iostream>
#include"globals.h"
#include"fsglbvar.h"
#include"fsxwattk.h"
#include"fsxpfront.h"
#include"fsairatk.h"
#include"rand2.h"
#include"Point.h"
#include"PerimeterPoint.h"

//============================================================================
AreaPerimeter::AreaPerimeter()
{  //AreaPerimeter::AreaPerimeter
  area = 0.0;
  perimeter = 0.0;
  sperimeter = 0.0;
  cuumslope[0] = 0.0;
  cuumslope[1] = 0.0;
} //AreaPerimeter::AreaPerimeter

//============================================================================
AreaPerimeter::~AreaPerimeter() {}

//============================================================================
void AreaPerimeter::arp( long CurrentFire )
{
	// calculates area and perimeter as a planimeter (with triangles)
	long i, j, numx;
	double xpt1, ypt1, xpt2, ypt2, aangle, zangle;
	double ediff, e1, e2, newarea;
	double hdist, gdist, xdiff, ydiff, DiffAngle;

	area = 0.0;
	perimeter = 0.0;
	sperimeter = 0.0; 	  // private AreaPerimeter-class members
	numx = GetNumPoints(CurrentFire);
	if (numx > 0)
	{
		startx = GetPerimeter2Value(0, 0);     // must use old perim array
		starty = GetPerimeter2Value(0, 1);     // new array not merged or clipped yet
		e1 = GetElev(0);
		i = 0;
		while (i < numx)
		{
			i++;
			xpt1 = GetPerimeter2Value(i, 0);
			ypt1 = GetPerimeter2Value(i, 1);
			zangle = direction(xpt1, ypt1); 	   // reference angle
			if (zangle != 999.9)	// make sure that startx,starty!=x[0]y[0]
				break;
		}
		e2 = GetElev(i);
		xdiff = xpt1 - startx;
		ydiff = ypt1 - starty;
		ediff = fabs(e1 - e2);
		hdist = sqrt(pow2(xdiff) + pow2(ydiff));
		gdist = sqrt(pow2(ediff) + pow2(hdist));
		perimeter = hdist;
		sperimeter = gdist;
		i++;
		for (j = i; j < numx; j++)
		{
			xpt2 = GetPerimeter2Value(j, 0);
			ypt2 = GetPerimeter2Value(j, 1);
			e2 = GetElev(j);
			xdiff = xpt2 - xpt1;
			ydiff = ypt2 - ypt1;
			ediff = fabs(e1 - e2);
			hdist = sqrt(pow2(xdiff) + pow2(ydiff));
			gdist = sqrt(pow2(ediff) + pow2(hdist));
			perimeter += hdist;
			sperimeter += gdist;
			newarea = .5 * (startx * ypt1 -
				xpt1 * starty +
				xpt1 * ypt2 -
				xpt2 * ypt1 +
				xpt2 * starty -
				startx * ypt2);
			newarea = fabs(newarea);
			aangle = direction(xpt2, ypt2);
			if (aangle != 999.9)
			{
				DiffAngle = aangle - zangle;
				if (DiffAngle > PI)
					DiffAngle = -(2.0 * PI - DiffAngle);
				else if (DiffAngle < -PI)
					DiffAngle = (2.0 * PI + DiffAngle);
				if (DiffAngle > 0.0)
					area -= newarea;
				else if (DiffAngle < 0.0)
					area += newarea;
				zangle = aangle;
			}
			xpt1 = xpt2;
			ypt1 = ypt2;
			e1 = e2;
		}
		xdiff = startx - xpt1;
		ydiff = starty - ypt1;
		e2 = GetElev(0);
		ediff = fabs(e1 - e2);
		hdist = sqrt(pow2(xdiff) + pow2(ydiff));
		gdist = sqrt(pow2(ediff) + pow2(hdist));
		perimeter += hdist;
		sperimeter += gdist;
		area /= (10000.0 * pow2(MetricResolutionConvert()));		// ha always
		perimeter /= (1000.0 * MetricResolutionConvert());  	 		// km always
		sperimeter /= (1000.0 * MetricResolutionConvert());			// km always
	}
}


double APolygon::direction(double xpt1, double ypt1)
{
	// calculates sweep direction for angle determination
	double zangle = 999.9, xdiff, ydiff;

	xdiff = xpt1 - startx;
	ydiff = ypt1 - starty;
	if (fabs(xdiff) < 1e-9)
		xdiff = 0.0;
	if (fabs(ydiff) < 1e-9)
		ydiff = 0.0;

	if (xdiff != 0.0)
	{
		zangle = atan(ydiff / xdiff);
		if (xdiff > 0.0)
			zangle = (PI / 2.0) - zangle;
		else
			zangle = (3.0 * PI / 2.0) - zangle;
	}
	else
	{
		if (ydiff >= 0.0)
			zangle = 0;
		else if (ydiff < 0.0)
			zangle = PI;
	}

	return zangle;
}



long APolygon::Overlap(long CurrentFire)
{
	// determines if point is inside or outside a fire polygon (CurrentFire)
	long NumVertex = GetNumPoints(CurrentFire);
	long count = 0, count1, count2, inside = 0;
	double Aangle = 0.0, Bangle;
	double CuumAngle = 0.0, DiffAngle;
	double Sxpt, Sypt;

	while (count < NumVertex)    // make sure that startx,starty!=x[0]y[0]
	{
		Sxpt = GetPerimeter1Value(CurrentFire, count, XCOORD);
		Sypt = GetPerimeter1Value(CurrentFire, count, YCOORD);
		Aangle = direction(Sxpt, Sypt);
		count++;
		if (Aangle != 999.9)
			break;
	}
	for (count1 = count; count1 <= NumVertex; count1++)
	{
		if (count1 == NumVertex)
			count2 = count - 1;
		else
			count2 = count1;
		Sxpt = GetPerimeter1Value(CurrentFire, count2, XCOORD);
		Sypt = GetPerimeter1Value(CurrentFire, count2, YCOORD);
		Bangle = direction(Sxpt, Sypt);
		if (Bangle != 999.9)
		{
			DiffAngle = Bangle - Aangle;
			if (DiffAngle > PI)
				DiffAngle = -(2.0 * PI - DiffAngle);
			else if (DiffAngle < -PI)
				DiffAngle = (2.0 * PI + DiffAngle);
			CuumAngle -= DiffAngle;
			Aangle = Bangle;
		}
	}
	if (fabs(CuumAngle) > PI)   	 // if absolute value of CuumAngle is>PI
		inside = 1; 			   // then point is inside polygon

	return inside;
}


void CompareRect::DetermineHiLo(double xpt, double ypt)
{
	// find bounding box for fire polygon
	if (xpt < Xlo)
		Xlo = xpt;
	else if (xpt > Xhi)
		Xhi = xpt;
	if (ypt < Ylo)
		Ylo = ypt;
	else if (ypt > Yhi)
		Yhi = ypt;
}


void CompareRect::WriteHiLo( long FireNum )
{
  // write bounding box for fire polygon
  if( GetNumPoints(FireNum) ) {
    SetPerimeter1( FireNum, GetNumPoints(FireNum), Xlo, Xhi );
    SetFireChx( FireNum, GetNumPoints(FireNum), Ylo, Yhi );  // not really firechx, but write to same positions
	}
}


void CompareRect::InitRect(long NumFire)
{
	// get bounding rectangle for fire polygon
	long NumPoints = GetNumPoints(NumFire);

	// load hi for lo and vice-versa to initiate for comparison
	Xhi = GetPerimeter1Value(NumFire, NumPoints, XCOORD);
	Xlo = GetPerimeter1Value(NumFire, NumPoints, YCOORD);
	Yhi = GetPerimeter1Value(NumFire, NumPoints, ROSVAL);
	Ylo = GetPerimeter1Value(NumFire, NumPoints, FLIVAL);
}

/*============================================================================
  CompareRect::BoundCross
*/
bool CompareRect::BoundCross( long Fire1, long Fire2 )
{ //CompareRect::BoundCross
  CallLevel++;
  if( Verbose >= CallLevel )
    printf( "%*sfsxwutil:CompareRect:BoundCross:1\n", CallLevel,"" );

  //Find out if bounding rectangles on 2 fires cross each other.
  Xlo = GetPerimeter1Value( Fire1, GetNumPoints(Fire1), XCOORD );
  Xhi = GetPerimeter1Value( Fire1, GetNumPoints(Fire1), YCOORD );
  Ylo = GetPerimeter1Value( Fire1, GetNumPoints(Fire1), ROSVAL );
  Yhi = GetPerimeter1Value( Fire1, GetNumPoints(Fire1), FLIVAL );
  xlo = GetPerimeter1Value( Fire2, GetNumPoints(Fire2), XCOORD );
  xhi = GetPerimeter1Value( Fire2, GetNumPoints(Fire2), YCOORD );
  ylo = GetPerimeter1Value( Fire2, GetNumPoints(Fire2), ROSVAL );
  yhi = GetPerimeter1Value( Fire2, GetNumPoints(Fire2), FLIVAL );
  //XLO=0.0; XHI=0.0; YLO=0.0; YHI=0.0;

  if( Verbose >= CallLevel )
    printf( "%*sfsxwutil:CompareRect:BoundCross:2 Xlo=%lf Xhi=%lf Ylo=%lf "
            "Yhi=%lf xlo=%lf xhi=%lf ylo=%lf yhi=%lf\n",
            CallLevel,"", Xlo, Xhi, Ylo, Yhi, xlo, xhi, ylo, yhi );
  CallLevel--;

  if( ! XOverlap() ) return false;
  else if( ! YOverlap() ) return false;

  return true;
} //CompareRect::BoundCross


bool CompareRect::MergeInAndOutOK1(long Fire1, long Fire2)
{
	// check to see if bounding rectangles allow mergers between inward and outward burning fires
	double XDiff1, XDiff2;
	double YDiff1, YDiff2;

	XDiff1 = Xhi - Xlo;
	XDiff2 = xhi - xlo;
	YDiff1 = Yhi - Ylo;
	YDiff2 = yhi - ylo;

	if (GetInout(Fire2) == 2)
	{
		if (XDiff1 >= XDiff2)
		{
			//return false;
			if (XDiff1 <= YDiff1 || YDiff1 >= YDiff2)    // if overlap dimension is smallest ||
				return false;   				   // both dimensions overlap
		}
		else if (YDiff1 >= YDiff2)
		{
			//return false;
			if (YDiff1 <= XDiff1 || XDiff1 >= XDiff2)
				return false;
		}
	}
	else if (GetInout(Fire1) == 2)
	{
		if (XDiff2 >= XDiff1)
		{
			//return false;
			if (XDiff2 <= YDiff2 || YDiff2 >= YDiff1)
				return false;
		}
		else if (YDiff2 >= YDiff1)
		{
			//return false;
			if (YDiff2 <= XDiff2 || XDiff2 >= XDiff1)
				return false;
		}
	}

	return true;
}


bool Intersections::MergeInAndOutOK2(long Fire1, long Fire2)
{
	// check to see if outward burning fire is inside another outward fire
	bool MergeOK = false;
	long i, j, NumFires, NumPoints;
	long TargetFire = -1;

	if (GetInout(Fire1) == 3 || GetInout(Fire2) == 3)
		return true;

	if (GetInout(Fire1) == 1)
		TargetFire = Fire1;
	if (GetInout(Fire2) == 1)
	{
		if (TargetFire == Fire1)
			return true;		// means that both are outward burning
		TargetFire = Fire2;
	}

	NumFires = GetNumFires();
	NumPoints = GetNumPoints(TargetFire);
	for (i = 0; i < NumFires; i++)
	{
		if (TargetFire == i)
			continue;
		else if (GetInout(i) != 1)
			continue;
		for (j = 0; j < NumPoints; j++)
		{
			startx = GetPerimeter1Value(TargetFire, j, XCOORD);
			starty = GetPerimeter1Value(TargetFire, j, YCOORD);
			if (Overlap(i))
			{
				MergeOK = true;
				break;
			}
		}
		if (MergeOK)
			break;
	}

	return MergeOK;
}


bool CompareRect::XOverlap()
{
	// computations for determining overlapping bounding rectangles, X-Dimension
	bool RectOverlap = 0;
	if (Xlo >= xlo && Xlo <= xhi)
	{
		XLO = Xlo;
		RectOverlap = 1;
	}
	else if (xlo >= Xlo && xlo <= Xhi)
	{
		XLO = xlo;
		RectOverlap = 1;
	}
	if (Xhi <= xhi && Xhi >= xlo)
	{
		XHI = Xhi;
		RectOverlap = 1;
	}
	else if (xhi <= Xhi && xhi >= Xlo)
	{
		XHI = xhi;
		RectOverlap = 1;
	}

	return RectOverlap;
}


bool CompareRect::YOverlap()
{
	// computations for determining overlapping bounding rectangles, Y-Dimension
	bool RectOverlap = 0;
	if (Ylo >= ylo && Ylo <= yhi)
	{
		YLO = Ylo;
		RectOverlap = 1;
	}
	else if (ylo >= Ylo && ylo <= Yhi)
	{
		YLO = ylo;
		RectOverlap = 1;
	}
	if (Yhi <= yhi && Yhi >= ylo)
	{
		YHI = Yhi;
		RectOverlap = 1;
	}
	else if (yhi <= Yhi && yhi >= Ylo)
	{
		YHI = yhi;
		RectOverlap = 1;
	}

	return RectOverlap;
}

//============================================================================
Intersections::Intersections()
{ //Intersections::Intersections
  crossout = 0;
  intersect = 0;
  interpoint = 0;
  crossnumalloc = 0;
  intersectnumalloc = 0;
  interpointnumalloc = 0;
  newisectnumalloc = 0;
  NewIsect = 0;
  AltIsect = 0;
  NumCrossThreads = 0;
  crossthread = 0;
} //Intersections::Intersections

//============================================================================
Intersections::~Intersections()
{ //Intersections::~Intersections
  FreeCrossout();
  FreeInterPoint();
  FreeIntersection();
  FreeNewIntersection();
  CloseCrossThreads();
} //Intersections::~Intersections


void Intersections::ResetIntersectionArrays()
{
	FreeSwap(); 					//XUtilities 	 member
	FreeCrossout(); 			  //Intersections member
	FreeInterPoint();
	FreeIntersection();
	FreeNewIntersection();
}


void Intersections::FindMergeSpans(long FireNum)
{
	// find vertices on fire polygons that are in overlapping rectangle
	// IF THIS FUNCTION IS USED, REPLACE USE OF XLO,XHI,YLO,YHI IN "FindExternalPt()"!!!!!!!

	double xpt, ypt;
	short First = 0, Second = 0;
	SpanAStart = 0;
	SpanAEnd = GetNumPoints(FireNum);

	for (long i = 0; i < GetNumPoints(FireNum); i++)
	{
		xpt = GetPerimeter1Value(FireNum, i, XCOORD);
		ypt = GetPerimeter1Value(FireNum, i, YCOORD);
		if (xpt <= XHI && xpt >= XLO)
		{
			if (ypt <= YHI && ypt >= YLO)
			{
				if (!First)
				{
					if (i > 0)  			  // don't start at beginning
					{
						SpanAStart = i - 1;   // start span at point before inside point
						First = 1;  		// first has started
						Second = 1; 		// search for second
					}
					else	  // if origin of fire inside box
					{
						//SpanAStart=0; 	// search whole fire
						//SpanAEnd=GetNumPoints(FireNum);
						break;
					}
				}
			}
			else if (Second)
			{
				SpanAEnd = i;
				//break;
			}
		}
		else if (Second)
		{
			SpanAEnd = i;
			//break;
		}
	}
	//	if(i>GetNumPoints(FireNum))  // if hit end of fire array and still inside
	//		SpanAEnd=GetNumPoints(FireNum);
}


void CompareRect::ExchangeRect(long FireNum)
{
	// determine combined bounding rectangle for merged fires
	if (xlo < Xlo)
		Xlo = xlo;
	if (xhi > Xhi)
		Xhi = xhi;
	if (ylo < Ylo)
		Ylo = ylo;
	if (yhi > Yhi)
		Yhi = yhi;
	WriteHiLo(FireNum);		// rewrite bounding rectangle to first perimeter
}


//============================================================================
void Intersections::AllocIntersection( long Number )
{ //Intersections::AllocIntersection
  //Allocate intersetion points array.
  if( Number <= 0 ) return;

  if( Number >= intersectnumalloc ) {
    FreeIntersection();
    nmemb = 2 * Number;
    if( (intersect = new long[nmemb]) == NULL )
      intersect = 0;
    else {
      intersectnumalloc = Number;
      memset( intersect, 0x0, nmemb * sizeof(long) );
    }
  }
  else
    memset( intersect, 0x0, intersectnumalloc * 2 * sizeof(long) );
} //Intersections::AllocIntersection

//============================================================================
void Intersections::FreeIntersection()
{ //Intersections::FreeIntesection
  if( intersect )
    delete [] intersect;
  intersect = 0;
  intersectnumalloc = 0;
} //Intersections::FreeIntesection

//============================================================================
void Intersections::AllocInterPoint( long Number )
{ //Intersections::AllocInterPoint
  if( Number <= 0 ) return;
  if( Number >= interpointnumalloc ) {
    FreeInterPoint();
    nmemb = NUMDATA * Number;
    if( (interpoint = new double[nmemb]) == NULL )
      interpoint = 0;
    else {
      interpointnumalloc = Number;
      memset( interpoint, 0x0, nmemb * sizeof(double) );
    }
  }
  else
    memset( interpoint, 0x0, interpointnumalloc * NUMDATA * sizeof(double) );
} //Intersections::AllocInterPoint

//============================================================================
void Intersections::FreeInterPoint()
{ //Intersections::FreeInterPoint
  if( interpoint )
    delete [] interpoint;
  interpoint = 0;
  interpointnumalloc = 0;
} //Intersections::FreeInterPoint

void Intersections::GetIntersection(long Number, long* XOrder, long* YOrder)
{
	Number *= 2;
	*XOrder = intersect[Number];
	*YOrder = intersect[++Number];
}

void Intersections::SetIntersection(long Number, long XOrder, long YOrder)
{
	Number *= 2;
	intersect[Number] = XOrder;
	intersect[++Number] = YOrder;
}

//============================================================================
void Intersections::GetInterPointCoord( long Number, double* x, double* y )
{ //Intersections::GetInterPointCoord
	Number *= NUMDATA;
	*x = interpoint[Number];
	*y = interpoint[++Number];
} //Intersections::GetInterPointCoord

//============================================================================
void Intersections::GetInterPoint( long Number, Point *Pt )
{ //Intersections::GetInterPoint
  Number *= NUMDATA;
  Pt->Set( interpoint[Number], interpoint[++Number] );
} //Intersections::GetInterPoint

void Intersections::GetInterPointFChx(long Number, double* Ros, double* Fli,
	double* Rct)
{
	Number *= NUMDATA;
	*Ros = interpoint[Number + 2];
	*Fli = interpoint[Number + 3];
	*Rct = interpoint[Number + 4];
}

void Intersections::SetInterPoint(long Number, double XCoord, double YCoord,
	double Ros, double Fli, double Rct)
{
	Number *= NUMDATA;
	if (XCoord == 0 || YCoord == 0)
		XCoord = YCoord = 1;
	interpoint[Number] = XCoord;
	interpoint[++Number] = YCoord;
	interpoint[++Number] = Ros;
	interpoint[++Number] = Fli;
	interpoint[++Number] = Rct;
}

/*============================================================================
  StandardizePolygon::FindExternalPoint
  Matches xcoord with bounding x coordinate to ensure outside starting point
  for crosscompare.
*/
long StandardizePolygon::FindExternalPoint( long CurrentFire, long type )
{ //StandardizePolygon::FindExternalPoint
  bool FirstTime = true;
  long i, nump, inout, coord1, coord2, OutPoint = 0, mult = 1, Reverse = 1;
  double testpt1, testpt2, max1, max2;

  nump = GetNumPoints( CurrentFire );
  inout = GetInout( CurrentFire );
  max1 = GetPerimeter1Value( CurrentFire, nump, type );
  switch( type ) {
    case 0:   //Using west as max1
      coord1 = 0;
      coord2 = 1;
      if( inout == 1 )
        max2 = GetPerimeter1Value( CurrentFire, nump, 3 );     //North max
      else max2 = GetPerimeter1Value( CurrentFire, nump, 2 );  //South max
        mult = 1;
        break;
    case 1:   //Using east as max1
      coord1 = 0;
      coord2 = 1;
      if( inout == 1 )
        max2 = GetPerimeter1Value( CurrentFire, nump, 2 );     //South max
      else max2 = GetPerimeter1Value( CurrentFire, nump, 3 );  //North max
        mult = -1;
        break;
    case 2:   //Using south as max1
      coord1 = 1;
      coord2 = 0;
      if( inout == 1 )
        max2 = GetPerimeter1Value( CurrentFire, nump, 0 );     //West max
      else max2 = GetPerimeter1Value( CurrentFire, nump, 1 );  //East max
      mult = -1;
      break;
    case 3:   //Using north as max1
      coord1 = 1;
      coord2 = 0;
      if( inout == 1 )
        max2 = GetPerimeter1Value( CurrentFire, nump, 1 );     //East max
      else max2 = GetPerimeter1Value( CurrentFire, nump, 0 );  //East max
      mult = 1;
      break;
  }

  if( inout == 2 ) Reverse = -1;
  mult *= Reverse;
  max2 *= mult;
  for( i = 0; i < nump; i++ ) {
    testpt1 = GetPerimeter1Value( CurrentFire, i, coord1 );
    testpt2 = GetPerimeter1Value( CurrentFire, i, coord2 ) * mult;
    if( testpt1 == max1 ) {
      if( FirstTime ) {
        OutPoint = i;
        max2 = testpt2;
        FirstTime = false;
      }
      else if( testpt2 <= max2 ) {
        OutPoint = i;
        max2 = testpt2;
      }
    }
  }

  return OutPoint;
} //StandardizePolygon::FindExternalPoint


void Intersections::CheckIllogicalExpansions(long CurrentFire)
{
	// determines if point is inside or outside a fire polygon (CurrentFire)
	// tests for each perim1 pt if inside perim2 polygon
	long NumVertex = GetNumPoints(CurrentFire);
	long count, count1, count2;//, inside;
	long i, j, k;
	double Aangle, Bangle;
	double CuumAngle, DiffAngle;
	double Sxpt, Sypt, xptl, yptl, xptn, yptn;
	long n, isect1, isect2;
	bool Continue;
	double xpt2, ypt2, xpt2l, ypt2l, xpt2n, ypt2n;
	double yceptl, yceptn, slopel, slopen;
	double xdiffl, ydiffl, xdiffn, ydiffn;


	for (i = 0; i < NumVertex; i++)
	{
		startx = GetPerimeter1Value(CurrentFire, i, XCOORD);
		starty = GetPerimeter1Value(CurrentFire, i, YCOORD);
		count = 0;
		CuumAngle = 0.0;
		while (count < NumVertex)    // make sure that startx,starty!=x[0]y[0]
		{
			Sxpt = GetPerimeter2Value(count, 0);
			Sypt = GetPerimeter2Value(count, 1);
			Aangle = direction(Sxpt, Sypt);
			count++;
			if (Aangle != 999.9)
				break;
		}
		for (count1 = count; count1 <= NumVertex; count1++)
		{
			if (count1 == NumVertex)
				count2 = count - 1;
			else
				count2 = count1;
			Sxpt = GetPerimeter2Value(count2, 0);
			Sypt = GetPerimeter2Value(count2, 1);
			Bangle = direction(Sxpt, Sypt);
			if (Bangle != 999.9)
			{
				DiffAngle = Bangle - Aangle;
				if (DiffAngle > PI)
					DiffAngle = -(2.0 * PI - DiffAngle);
				else if (DiffAngle < -PI)
					DiffAngle = (2.0 * PI + DiffAngle);
				CuumAngle -= DiffAngle;
				Aangle = Bangle;
			}
		}
		if ((GetInout(CurrentFire) == 1 && fabs(CuumAngle) > PI) || 		// if absolute value of CuumAngle is>PI then inside polygon
			(GetInout(CurrentFire) == 2 && fabs(CuumAngle) < PI))
		{
			j = i - 1;
			k = i + 1;
			if (j < 0)
				j = NumVertex - 1;
			if (k > NumVertex - 1)
				k = 0;
			Continue = false;
			for (n = 0;
				n < numcross;
				n++) // check for intersections involving point
			{
				GetIntersection(n, &isect1, &isect2);
				if (isect1 == j)
				{
					if (k != 0)
						i = k;
					Continue = true;
					break;
				}
				else if (isect1 == i)
				{
					if (k < NumVertex - 1 && k != 0)
						i = k + 1;
					Continue = true;
					break;
				}
				else if (isect1 == k)
				{
					if (k < NumVertex - 2 && k != 0)
						i = k + 2;
					Continue = true;
					break;
				}
			}
			if (Continue)
				continue;

			if (GetPerimeter1Value(CurrentFire, i, ROSVAL) == 0.0)
				continue;

			xptl = GetPerimeter1Value(CurrentFire, j, XCOORD);
			yptl = GetPerimeter1Value(CurrentFire, j, YCOORD);
			xptn = GetPerimeter1Value(CurrentFire, k, XCOORD);
			yptn = GetPerimeter1Value(CurrentFire, k, YCOORD);

			xpt2 = GetPerimeter2Value(i, XCOORD);
			ypt2 = GetPerimeter2Value(i, YCOORD);
			xpt2l = GetPerimeter2Value(j, XCOORD);
			ypt2l = GetPerimeter2Value(j, YCOORD);
			xpt2n = GetPerimeter2Value(k, XCOORD);
			ypt2n = GetPerimeter2Value(k, YCOORD);

			xdiffl = xpt2l - xpt2;
			ydiffl = ypt2l - ypt2;
			if (xdiffl != 0.0)
			{
				slopel = ydiffl / xdiffl;
				yceptl = yptl - (slopel * xptl); //  ycept from xptl on perim1
			}
			else
				slopel = 0.0;
			xdiffn = xpt2n - xpt2;
			ydiffn = ypt2n - ypt2;
			if (xdiffn != 0.0)
			{
				slopen = ydiffn / xdiffn;
				yceptn = yptn - (slopen * xptn); //  ycept from xptn on perim1
			}
			else
				slopen = 0.0;
			Continue = true;
			if (xdiffl != 0.0 && xdiffn != 0.0 && slopel != slopen)
			{
				Sxpt = (yceptn - yceptl) / (slopel - slopen); //xptl-((xpt2l-xpt2)/(xpt2l-xpt2n))*((xptl-xptn)/(xpt2l-xpt2n));
				Sypt = slopel * Sxpt + yceptl;
				Continue = false;
			}
			else if (xdiffl == 0.0 && xdiffn != 0.0)
			{
				Sypt = slopen * xptl + yceptn;
				Sxpt = xptl;
				Continue = false;
			}
			else if (xdiffl != 0.0 && xdiffn == 0.0)
			{
				Sypt = slopel * xptn + yceptl;
				Sxpt = xptn;
				Continue = false;
			}
			if (!Continue)
			{
				if (Sxpt == startx && Sypt == starty)
				{
					if (k != 0)
						i = k;
				}
				else
				{
					SetPerimeter1(CurrentFire, i, Sxpt, Sypt);
					if (k < NumVertex - 2 && k != 0)
						i = k + 2;
					else
						i = NumVertex;
				}
			}
		}
	}
}


bool Intersections::SwapBarriersAndFires()
{
	long i, j;
	bool NoneAhead;

	for (i = 0; i < GetNumFires(); i++)
	{
		if (GetInout(i) > 1)
		{
			NoneAhead = true;
			for (j = i + 1; j < GetNumFires(); j++)
			{
				if (GetInout(j) == 1)
				{
					NoneAhead = false;
					if (SwapFirePerims(i, j) > 0)
					{
						if (GetNumAttacks() > 0)
						{
							SetNewFireNumberForAttack(j, i);
							SetNewFireNumberForAttack(i, j);
						}
						if (GetNumAirAttacks() > 0)
							SetNewFireNumberForAirAttack(i, j);
					}
				}
			}
			if (NoneAhead)
				return true;
		}
	}

	return true;
}

/*============================================================================
  Intersections::CrossesWithBarriers
  Determines overlap between fires and barriers
*/
void Intersections::CrossesWithBarriers( long FireNum )
{ //Intersections::CrossesWithBarriers
  CallLevel++;

  if( Verbose >= CallLevel )
    printf( "%*sfsxwutil:Intersections::CrossesWithBarriers:1 SpanAEnd=%ld\n",
            CallLevel,"", SpanAEnd );

  if( GetInout(FireNum) == 0 ) {
    CallLevel--;
    return;
  }

  for( long i = 0; i < GetNumFires(); i++ ) {
    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::CrossesWithBarriers:1a "
              "GetInout(%ld)=%ld\n", CallLevel, "", i, GetInout(i) );

    if( GetInout(i) < 3 ) continue;
    else if( BoundCross(FireNum, i) ) {
      if( Verbose >= CallLevel )
        printf( "%*sfsxwutil:Intersections::CrossesWithBarriers:1a1\n",
                CallLevel,"" );

      SpanAStart = 0;
      SpanAEnd = GetNumPoints( FireNum );
      SpanBEnd = GetNumPoints( i );
      CrossCompare( &FireNum, i ); //Won't change FireNum because no rediscretizing
    }
  }

  if( Verbose >= CallLevel )
    printf( "%*sfsxwutil:Intersections::CrossesWithBarriers:2 SpanAEnd=%ld\n",
            CallLevel,"", SpanAEnd );

  CallLevel--;
} //Intersections::CrossesWithBarriers

/*============================================================================
  Intersections::CrossFires
  If bounding rectangles overlap, then search for crosspoints.
*/
void Intersections::CrossFires( int checkmergers, long* firenum )
{ //Intersections::CrossFires
  long type, startchex, chex, i;
  long TotalFires, CurrentFire = 0, NextFire, OldFire;
  bool CheckOutwardOnly = false;
  long InwardFiresPresent = 0;
  double flitest;

  CallLevel++;

  if( Verbose >= CallLevel )
    printf( "%*sfsxwutil:Intersections::CrossFires:1 "
            "Checking for fire crosspoints....   "
            "numpts[%d]=%ld numfires=%ld\n",
            CallLevel, "", 0, GetNumPoints(0), numfires );

  if( checkmergers == 0 ) {  //If checking crosses within a single fire
    TotalFires = *firenum + 1;  //Total fires is 1
    startchex = 0;              //Start checking
  }
  else { //If checking for mergers
    if( GetInout(0) == 3 ) SwapBarriersAndFires();
    TotalFires = GetNumFires();
    startchex = 1;    //Start checking after 1st fire
    CheckOutwardOnly = true;
  } //For each separate fire

  if( Verbose >= CallLevel )
    printf( "%*sfsxwutil:Intersections::CrossFires:2 "
            "numpts[%d]=%ld\n",
            CallLevel, "", 0, GetNumPoints(0) );

  for( CurrentFire = *firenum; CurrentFire < TotalFires; CurrentFire++ ) {
    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::CrossFires:2a %ld %ld "
              "numpts[%d]=%ld\n",
              CallLevel, "", CurrentFire, TotalFires, 0,
              GetNumPoints(0) );

    chex = CurrentFire + startchex;
    if( checkmergers ) {
      if( Verbose >= CallLevel )
        printf( "%*sfsxwutil:Intersections::CrossFires:2a1\n", CallLevel,"" );

      chex = 0;
      OldFire = CurrentFire;     //Save temp fire for post-frontal
      if( CheckOutwardOnly ) {
        if( GetInout(CurrentFire) > 1 ) {
          if( GetInout(CurrentFire) == 2 ) InwardFiresPresent = 1;
        }
        if( CurrentFire == TotalFires - 1 ) { //If last fire
          CheckOutwardOnly = false;
          if( InwardFiresPresent > 0 ) {
            CurrentFire = -1;
            continue;
          }
        }
        if( GetInout(CurrentFire) > 1 ) continue;
      }
      else if( GetInout(CurrentFire) != 2 ) continue;

      //---------------------------------------------------------------------
      //Don't merge inward fires with negative fli vertices, avoid bad crosses
      //---------------------------------------------------------------------
      else {
        for( i = 0; i < GetNumPoints(CurrentFire); i++ ) {
          flitest = GetPerimeter1Value( CurrentFire, i, FLIVAL );
          if( flitest < 0.0 ) {
            RePositionFire( &CurrentFire );
            if( CheckPostFrontal(GETVAL) && OldFire != CurrentFire )
              SetNewFireNumber( OldFire, CurrentFire,
                                post.AccessReferenceRingNum(1, GETVAL) );

            break;
          }
        }
        if( flitest < 0.0 ) continue;
      }

      //---------------------------------------------------------------------
      //Don't check crosses for merged fires.
      if( (GetInout(CurrentFire)) == 0 ) continue; 

      RePositionFire( &CurrentFire );
      if( CheckPostFrontal(GETVAL) && OldFire != CurrentFire )
        SetNewFireNumber( OldFire, CurrentFire,
                          post.AccessReferenceRingNum(1, GETVAL) );

    }
    else {
      if( Verbose >= CallLevel )
        printf( "%*sfsxwutil:Intersections::CrossFires:2a2\n", CallLevel,"" );

      if( GetInout(CurrentFire) == 3 ) {
        RePositionFire( &CurrentFire );
        continue;
      }
      else if( GetInout(CurrentFire) > 0 )
        RemoveDuplicatePoints( CurrentFire );
    }

    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::CrossFires:2b numpts[%d]=%ld\n",
              CallLevel, "", 0, GetNumPoints(0) );

    //Don't check crosses for merged fires.
    if( (GetInout(CurrentFire)) == 0 ) continue;

    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::CrossFires:2c numpts[%d]=%ld\n",
              CallLevel, "", 0, GetNumPoints(0) );

    OldFire = CurrentFire;  //Store old fire number for later use
    do {                    //Check for all mergers with "CurrentFire" fire
      NoCrosses = 0;        //CurrentFire number of fire mergers

      if( Verbose >= CallLevel )
        printf( "%*sfsxwutil:Intersections::CrossFires:2c1 numpts[%d]=%ld\n",
                CallLevel, "", 0, GetNumPoints(0) );

      //For same and other fire depending on check.
      for( NextFire = chex; NextFire < TotalFires; NextFire++ ) {
        if( Verbose >= CallLevel )
          printf( "%*sfsxwutil:Intersections::CrossFires:2c1a %ld %ld "
                  "numpts[%d]=%ld GetInout(%ld)=%ld GetInout(%ld)=%ld\n",
                  CallLevel, "", NextFire, TotalFires, 0, GetNumPoints(0),
                  CurrentFire, GetInout(CurrentFire), NextFire,
                  GetInout(NextFire) );

        if( GetInout(CurrentFire) == 0 ) {
          NoCrosses = 0;  //Exit do
          break;          //Exit for
        }
        if( GetInout(NextFire) == 0 ) continue;  //Terminate check if no fire

        if( Verbose >= CallLevel )
          printf( "%*sfsxwutil:Intersections::CrossFires:2c1b "
                  "numpts[%d]=%ld\n",
                  CallLevel, "", 0, GetNumPoints(0) );

        if( checkmergers ) {
          if( CheckOutwardOnly ) {
            if( NextFire < CurrentFire + 1 && GetInout(NextFire) < 3 )
              continue;
          }
          else if( GetInout(CurrentFire) == 2 && NextFire == 0 ) {
            //Only allow mergers with barriers.
            if( GetInout(NextFire) < 3 ) continue;
          }

          if( CurrentFire == NextFire ) continue;
          else if( GetInout(NextFire) == 2 ) {
            if( CheckOutwardOnly ) continue;
            else if( GetInout(CurrentFire) == 2 ) {
              continue;    //Don't check crosses between inward burning fires
            }
            else if( CurrentFire == 0 )
              continue;    //Don't merge fire 0 with inward burning fires
          }
        }

        if( Verbose >= CallLevel )
          printf( "%*sfsxwutil:Intersections::CrossFires:2c1c "
                  "numpts[%d]=%ld\n",
                  CallLevel, "", 0, GetNumPoints(0) );

        //If searching within a fire.
        if( CurrentFire == NextFire ) {
          if( Verbose >= CallLevel )
            printf( "%*sfsxwutil:Intersections::CrossFires:2c1c1 "
                    "SpanAEnd=%ld numpts[%d]=%ld NumFires=%ld\n",
                    CallLevel, "", SpanAEnd, 0, GetNumPoints(0),
                    GetNumFires() );

          SpanAStart = 0;
          SpanAEnd = GetNumPoints( CurrentFire );
          SpanBEnd = GetNumPoints( NextFire );

          if( Verbose >= CallLevel )
            printf( "%*sfsxwutil:Intersections::CrossFires:2c1c2 "
                    "SpanAEnd=%ld numpts[%d]=%ld\n",
                    CallLevel, "", SpanAEnd, 0, GetNumPoints(0) );

          for( type = 0; type < 4; type++ ) {
            if( Verbose >= CallLevel )
              printf( "%*sfsxwutil:Intersections::CrossFires:2c1c2a "
                      "numpts[%d]=%ld\n",
                      CallLevel, "", 0, GetNumPoints(0) );

            ReorderPerimeter( CurrentFire,
                              FindExternalPoint(CurrentFire, type) );

            if( Verbose >= CallLevel )
              printf( "%*sfsxwutil:Intersections::CrossFires:2c1c2b "
                      " numpts[%d]=%ld\n",
                      CallLevel, "", 0, GetNumPoints(0) );

            //If it doesn't work, reorder and try again.
            if( CrossCompare(&CurrentFire, NextFire) ) break;

            if( Verbose >= CallLevel )
              printf( "%*sfsxwutil:Intersections::CrossFires:2c1c2c "
                      "numpts[%d]=%ld\n",
                      CallLevel, "", 0, GetNumPoints(0) );
          }
          *firenum = TotalFires = CurrentFire; //Update totalfires && *firenum

          if( Verbose >= CallLevel )
            printf( "%*sfsxwutil:Intersections::CrossFires:2c1c3 "
                    "numpts[%d]=%ld\n",
                    CallLevel, "", 0, GetNumPoints(0) );
        }     //Force exit from for-loops
        //If searching between fires & bounding rectangles overlap.
        else if( BoundCross(CurrentFire, NextFire) ) {
          if( Verbose >= CallLevel )
            printf( "%*sfsxwutil:Intersections::CrossFires:2c1c4 "
                    "numpts[%d]=%ld\n",
                    CallLevel, "", 0, GetNumPoints(0) );

          if( MergeInAndOutOK2(CurrentFire, NextFire) ) {
            if( Verbose >= CallLevel )
              printf( "%*sfsxwutil:Intersections::CrossFires:2c1c4a "
                      "SpanAEnd=%ld\n",
                      CallLevel, "", SpanAEnd );

            SpanAStart = 0;
            SpanAEnd = GetNumPoints( CurrentFire );
            SpanBEnd = GetNumPoints( NextFire );

            if( Verbose >= CallLevel )
              printf( "%*sfsxwutil:Intersections::CrossFires:2c1c4b\n "
                      "SpanAEnd=%ld numpts[%d]=%ld\n",
                      CallLevel, "", SpanAEnd, (int)CurrentFire,
                      GetNumPoints(CurrentFire) );

            //Compare the fires pt by pt.
            CrossCompare( &CurrentFire, NextFire );

            if( Verbose >= CallLevel )
              printf( "%*sfsxwutil:Intersections::CrossFires:2c1c4c "
                      "numpts[%d]=%ld\n",
                      CallLevel, "", (int)CurrentFire,
                      GetNumPoints(CurrentFire) );

            TotalFires = GetNewFires();

            if( Verbose >= CallLevel )
              printf( "%*sfsxwutil:Intersections::CrossFires:2c1c4d\n",
                      CallLevel, "" );
          }
        }

        if( Verbose >= CallLevel )
          printf( "%*sfsxwutil:Intersections::CrossFires:2c1d "
                  "numpts[%d]=%ld\n",
                  CallLevel, "", 0, GetNumPoints(0) );
      }
      if( Verbose >= CallLevel )
        printf( "%*sfsxwutil:Intersections::CrossFires:2c2 numpts[%d]=%ld\n",
                CallLevel, "", 0, GetNumPoints(0) );

    } while( NoCrosses > 0 );

    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::CrossFires:2d "
              "numpts[%d]=%ld\n",
              CallLevel, "", 0, GetNumPoints(0) );

    //Restore old fire number as current number before ++.
    CurrentFire = OldFire;
  }

  if( Verbose >= CallLevel )
    printf( "%*sfsxwutil:Intersections::CrossFires:3 "
            "Done checking crosspoints  numpts[%d]=%ld\n",
            CallLevel, "", 0, GetNumPoints(0) );

  CallLevel--;
} //Intersections::CrossFires

//============================================================================
bool Intersections::AllocCrossThreads()
{ //Intersections::AllocCrossThreads
  if( NumCrossThreads == GetMaxThreads() ) return true;

  CloseCrossThreads();
  crossthread = new CrossThread[ GetMaxThreads() ];

  if( ! crossthread ) {
    NumCrossThreads = 0;

    return false;
  }

  NumCrossThreads = GetMaxThreads();

  return true;
} //Intersections::AllocCrossThreads

//============================================================================
void Intersections::CloseCrossThreads()
{ //Intersections::CloseCrossThreads
  FreeCrossThreads();
} //Intersections::CloseCrossThreads

//============================================================================
void Intersections::FreeCrossThreads()
{ //Intersections::FreeCrossThreads
  if( crossthread )
    delete[] crossthread;

  crossthread = 0;
  NumCrossThreads = 0;
} //Intersections::FreeCrossThreads

//============================================================================
bool Intersections::EliminateCrossPoints( long CurrentFire )
{ //Intersections::EliminateCrossPoints
  bool Modified = false;
  long i, j, BadPt;
  double xpt, ypt, xptn, yptn;
  double xdiff, ydiff, dist, offset;

  //Dithers duplicate points that cross;
  long NumPts = GetNumPoints( CurrentFire );
  for( i = 0; i < numcross; i++ ) {
    BadPt = -1;
    if( intersect[i * 2] < 0 ) BadPt = labs( intersect[i * 2] );
    if( BadPt < 0 && labs(intersect[i*2]) > labs(intersect[i * 2 + 1]) )
      continue;
    if( BadPt >= 0 ) {
      Modified = true;
      xpt = GetPerimeter1Value( CurrentFire, BadPt, XCOORD );
      ypt = GetPerimeter1Value( CurrentFire, BadPt, YCOORD );
      j = BadPt + 1;
      if( j > NumPts - 1 ) j -= NumPts;
      xptn = GetPerimeter1Value( CurrentFire, j, XCOORD );
      yptn = GetPerimeter1Value( CurrentFire, j, YCOORD );
      xdiff = xpt - xptn;
      ydiff = ypt - yptn;
      dist = sqrt( pow2(xdiff) + pow2(ydiff) );

      //Basically zero, so get rid of the point, this will avoid endless loop.
      if( dist < 1e-4 ) {
        //20200410 JWB: Replaced use of GetPerimeter1Address(NumFire,0) &
        //              memcpy() here with PerimeterPoint::DeletePoint().
        DeletePoint( CurrentFire, BadPt );
        NumPts--;

        for( j = i + 1; j < numcross; j++ ) {
          if( labs(intersect[j * 2]) == BadPt ||
              labs(intersect[j * 2 + 1]) == BadPt ) {
            intersect[j * 2] = 0;
            intersect[j * 2 + 1] = 0;
          }
          else {
            if( labs(intersect[j * 2]) > BadPt ) {
              if( intersect[j * 2] < 0 ) intersect[j * 2] += 1;
              else intersect[j * 2] -= 1;
            }
            if( labs(intersect[j * 2 + 1]) > BadPt ) {
              if( intersect[j * 2 + 1] < 0 ) intersect[j * 2 + 1] += 1;
              else intersect[j * 2 + 1] -= 1;
            }
          }
        }
      }
      //Not zero distance between points, so just dither it by 0.1m or ft.
      else {
        /*
        //Randomize distance offset.
        xoffset = ( (double)(rand2(&idum)*100) ) / 999.0;
        xoffset *= dist;
        if( xoffset > 0.1 ) xoffset = 0.1;
        //Randomize distance offset.
        yoffset = ( (double)(rand2(&idum)*100) ) / 999.0;
        yoffset *= dist;
        if( yoffset > 0.1 ) yoffset = 0.1;
        //Randomize x-direction offset.
        xa = ( (double)((rand2(&idum)*20)) / 20.0 + 1.0 ) * PI;
        //Randomize y-direction offset.
        ya = ( (double)((rand2(&idum)*20)) / 20.0 + 1.0 )*PI;
        xpt = xpt - xoffset * cos( xa );
        // * xdiff / dist;
        ypt= ypt - yoffset * sin( ya );
        // * ydiff / dist;
        */
        offset = 0.1 * dist;
        j = 0;
        while( offset > 0.1 ) {
          offset *= 0.1;
          j++;
          if( j > 50 ) break;
        };
        xpt -= offset * xdiff / dist;
        ypt -= offset * ydiff / dist;
        SetPerimeter1( CurrentFire, BadPt, xpt, ypt );
        for( j = i + 1; j < numcross; j++ ) {
          if( labs(intersect[j * 2]) == BadPt ||
              labs(intersect[j * 2 + 1]) == BadPt ) {
            intersect[j * 2] = labs( intersect[j * 2] );
            intersect[j * 2 + 1] = labs( intersect[j * 2 + 1] );
          }
        }
      }
    }
  }

  if( NumPts < GetNumPoints(CurrentFire) )
    SetNumPoints( CurrentFire, NumPts );

  return Modified;
} //Intersections::EliminateCrossPoints

//============================================================================
bool Intersections::CrossCompare( long *CurrentFire, long NextFire )
{ //Intersections::CrossCompare
  bool Result = true, Repeat = false;
  long i, j, k, m, begin, end, threadct;
  long range, cxx, NewFires1, NewFires2, * PostFires; //For post frontal stuff
  long inside = 0, NumCheckReverse = 0, CheckReverse = 0, begincross = 0;
  long TestCross = -1, NumTimes = 0, PriorNumPoints;
  double fract, interval, ipart, D1, angle1, angle2;
  double xpt, ypt, crxpt, crypt, crxptn, cryptn, newx, newy;

  CallLevel++;

  long CrossMemAlloc = GetNumPoints( *CurrentFire )
                       + GetNumPoints( NextFire );

  if( Verbose >= CallLevel )
    printf( "%*sfsxwutil:Intersections::CrossCompare:1 numpts[%d]=%ld\n",
            CallLevel, "", 0, GetNumPoints(0) );

  AllocIntersection( CrossMemAlloc );  //Allocate array of intersection orders
  AllocInterPoint( CrossMemAlloc );    //Allocate array of intersection points
  AllocCrossThreads();

  if( Verbose >= CallLevel )
    printf( "%*sfsxwutil:Intersections::CrossCompare:2 "
            "numpts[%d]=%ld  crossthread[%d].GetNumCross()=%ld\n",
            CallLevel, "", 0, GetNumPoints(0), 0,
            crossthread[0].GetNumCross() );

  do {
    Repeat = false;

    interval = ( (double) GetNumPoints(*CurrentFire) ) /
               ( (double) NumCrossThreads );
    fract = modf( interval, &ipart );
    range = (long) interval;
    if( fract > 0.0 ) range++;

    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::CrossCompare:2a "
              "numpts[%d]=%ld %ld %f crossthread[%d].GetNumCross()=%ld\n",
              CallLevel, "", 0, GetNumPoints(0), NumCrossThreads, interval,
              0, crossthread[0].GetNumCross() );

    begin = threadct = 0;
    for( i = 0; i < NumCrossThreads; i++ ) {
      end = begin + range;
      if( begin >= GetNumPoints(*CurrentFire) ) continue;
      if( end > GetNumPoints(*CurrentFire) )
        end = GetNumPoints( *CurrentFire );
      crossthread[i].SpanAStart = begin;
      crossthread[i].SpanAEnd = end;

      threadct++;
      begin = end;
    }

    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::CrossCompare:2b "
              "numpts[%d]=%ld crossthread[%d].GetNumCross()=%ld\n",
              CallLevel, "",0 , GetNumPoints(0), 0,
              crossthread[0].GetNumCross() );

    for( j = 0; j < threadct; j++ )
      crossthread[j].StartCrossThread( j, *CurrentFire, NextFire );

    numcross = 0;

    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::CrossCompare:2c "
              "numpts[%d]=%ld crossthread[%d].GetNumCross()=%ld "
              "numcross=%ld CurrentFire=%ld\n",
              CallLevel, "", 0, GetNumPoints(0), 0,
              crossthread[0].GetNumCross(), numcross, *CurrentFire );

    j = 0;
    for( i = 0; i < threadct; i++ ) j += crossthread[i].GetNumCross();

    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::CrossCompare:2d "
              "numpts[%d]=%ld numcross=%ld\n",
              CallLevel, "", 0, GetNumPoints(0), numcross );

    if( j >= CrossMemAlloc ) {
      CrossMemAlloc += j;
      AllocIntersection( CrossMemAlloc ); //Alloc array of intersection orders
      AllocInterPoint( CrossMemAlloc );   //Alloc array of intersection points
    }

    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::CrossCompare:2e "
              "numpts[%d]=%ld numcross=%ld threadct=%ld\n",
              CallLevel, "", 0, GetNumPoints(0), numcross, threadct );

    for( i = 0; i < threadct; i++ ) {
      if( Verbose >= CallLevel )
        printf( "%*sfsxwutil:Intersections::CrossCompare:2e1 "
                "j=%ld\n",
                CallLevel, "", j );

      j = crossthread[i].GetNumCross();

      if( Verbose >= CallLevel )
        printf( "%*sfsxwutil:Intersections::CrossCompare:2e2 "
                "j=%ld numcross=%ld\n",
                CallLevel, "", j, numcross );

      memcpy( &intersect[numcross * 2], crossthread[i].GetIsect(),
              j * 2 * sizeof(long) );
      memcpy( &interpoint[numcross * NUMDATA], crossthread[i].GetIpoint(),
              j * NUMDATA * sizeof(double) );
      numcross += j;
    }

    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::CrossCompare:2f "
              "numpts[%d]=%ld numcross=%ld\n",
              CallLevel, "", 0, GetNumPoints(0), numcross );

    if( numcross > 0 && *CurrentFire == NextFire ) {
      if( EliminateCrossPoints(*CurrentFire) ) {
        numcross = 0;
        Repeat = true;
        continue;
      }
      else Repeat = false;
    }

    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::CrossCompare:2g numpts[%d]=%ld\n",
              CallLevel, "", 0, GetNumPoints(0) );

    if( numcross > 0 && NumTimes > 0 && *CurrentFire == NextFire ) {
      if(GetInout(*CurrentFire) == 2 ) {
        for( k = 0; k < numcross; k++ ) {
          GetIntersection( k, &i, &j );
          if( CheckReverse == 0 ) {
            GetInterPointCoord( k, &newx, &newy );
            m = j + 1;
            if( m > GetNumPoints(*CurrentFire) - 1 ) m = 0;
            xpt = GetPerimeter1Value( *CurrentFire, i, XCOORD );
            ypt = GetPerimeter1Value( *CurrentFire, i, YCOORD );
            crxpt = GetPerimeter1Value( *CurrentFire, j, XCOORD );
            crypt = GetPerimeter1Value( *CurrentFire, j, YCOORD );
            crxptn = GetPerimeter1Value( *CurrentFire, m, XCOORD );
            cryptn = GetPerimeter1Value( *CurrentFire, m, YCOORD );

            startx = newx;
            starty = newy;
            angle1 = direction( xpt, ypt );
            angle2 = direction( crxptn, cryptn );
            D1 = sin( angle2 ) * cos( angle1 ) -
                 cos( angle2 ) * sin( angle1 );
            if( D1 >= 0.0 || (pow2(newx - xpt) + pow2(newy - ypt)) < 1e-12 ) {
              startx = xpt - ( xpt - crxpt ) / 2.0;
              starty = ypt - ( ypt - crypt ) / 2.0;
              if( ! Overlap(*CurrentFire) ) inside = 1;
              else inside = -1;
            }
            else inside = -1;  //Don't check on mirror of cross order
            begincross = i + 1;
            if( begincross > GetNumPoints(*CurrentFire) - 1 ) begincross = 0;
            TestCross = i;
            CheckReverse = 1;
          }
          else if( i == TestCross ) CheckReverse *= -1;
        }
      }
    }

    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::CrossCompare:2h numpts[%d]=%ld\n",
              CallLevel, "", 0, GetNumPoints(0) );

    if( numcross > 0 && *CurrentFire == NextFire ) {
      Repeat = false;
      if( NumTimes == 0 ) {  //Do this always for inward and outward fires
        if( Verbose >= CallLevel )
          printf( "%*sfsxwutil:Intersections::CrossCompare:2h1 "
                  "numpts[%d]=%ld\n",
                  CallLevel, "", 0, GetNumPoints(0) );

        OrganizeCrosses( *CurrentFire );

        if( Verbose >= CallLevel )
          printf( "%*sfsxwutil:Intersections::CrossCompare:2h2 "
                  "numpts[%d]=%ld\n",
                  CallLevel, "", 0, GetNumPoints(0) );

        PriorNumPoints = GetNumPoints( *CurrentFire );

        if( Verbose >= CallLevel )
          printf( "%*sfsxwutil:Intersections::CrossCompare:2h3 "
                  "numpts[%d]=%ld\n",
                  CallLevel, "", 0, GetNumPoints(0) );

        numcross = 0;
        Repeat = true;
      }
      else if( GetInout(*CurrentFire) == 2 && inside * CheckReverse == 1 ) {
        if( Verbose >= CallLevel )
          printf( "%*sfsxwutil:Intersections::CrossCompare:2h4 "
                  "numpts[%d]=%ld\n",
                  CallLevel, "", 0, GetNumPoints(0) );

        //Reorder without origin inside loop.
        ReorderPerimeter( *CurrentFire, begincross );
        numcross = 0;
        Repeat = true;
        if( ++NumCheckReverse >= GetNumPoints(*CurrentFire) ) {
          CheckReverse = -1;  //Don't repeat it
          inside = 0;   //Prevent repeat
        }
        //Must repeat until 1/2 vertices examined in case of twin spike.
        else CheckReverse = 0;
      }
      else Repeat = false;
      NumTimes = 1;
    }

    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::CrossCompare:2i numpts[%d]=%ld\n",
              CallLevel, "", 0, GetNumPoints(0) );
  } while( Repeat == true );

  if( Verbose >= CallLevel )
    printf( "%*sfsxwutil:Intersections::CrossCompare:3 numpts[%d]=%ld\n",
            CallLevel, "", 0, GetNumPoints(0) );

  //-------------------------------------------------------------
  //NEW ALGORITHM PRESERVES ONLY OUTER PERIMETER
  //-------------------------------------------------------------
  //Below: was !=0 but now prevents single crossovers, that shouldn't
  //happen anyway??!!
  if( numcross > 0 ) {
    if(*CurrentFire == NextFire && numcross >= CrossMemAlloc ) {
      if( GetInout(*CurrentFire) == 1 ) {
        FindOuterFirePerimeter( *CurrentFire );
        rediscretize( CurrentFire, true );
        numcross = 0;
      }
      //Eliminate by forcing into trap for non-even numcrosses.
      else numcross = 1;
    }
  }
  if( Verbose >= CallLevel )
    printf( "%*sfsxwutil:Intersections::CrossCompare:4 numpts[%d]=%ld\n",
            CallLevel, "", 0, GetNumPoints(0) );

  //Below: was !=0 but now prevents single crossovers, that shouldn't
  //happen anyway??!!
  if( numcross > 0 ) {
    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::CrossCompare:4a\n", CallLevel, "" );

    //cxx = (long) (numcross / 2.0); original JAS!
    cxx = ( long ) ( numcross / 2 ); //modified JAS!
    if( cxx * 2 != numcross && GetInout(NextFire) < 3 ) {
      if( Verbose >= CallLevel )
        printf( "%*sfsxwutil:Intersections::CrossCompare:4a1 "
                "numcross=%ld cxx=%ld test=%ld\n",
                CallLevel, "", numcross, cxx, (long)(numcross/2.0) );
       // || numcross>=CrossMemAlloc)  //trap for odd number of cross points
      SetNumPoints( NextFire, 0 );    //and overly large number of crosses on
      SetInout( NextFire, 0 );        //spaghetti fronts
      IncSkipFires( 1 );
      numcross = 0;
    }
    //Else if merging two fires then....
    else if( *CurrentFire != NextFire && GetInout(NextFire) < 3 ) {
      //Determine and write combined bounding rectangle.
      ExchangeRect( *CurrentFire );
      //Change order of intersected points.
      OrganizeIntersections( *CurrentFire );
      //Tally the number of times a fire has merged.
      NoCrosses++;
    }

    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::CrossCompare:4b numpts[%d]=%ld\n",
              CallLevel, "", 0, GetNumPoints(0) );

    if( numcross > 0 ) {
      if( *CurrentFire == NextFire ) {  //Added 6/25/1995
        if( Verbose >= CallLevel )
          printf( "%*sfsxwutil:Intersections::CrossCompare:4b1\n",
                  CallLevel, "" );

        writenum = 1; //Make sure perimeter is written in CleanPerimeter

        NewFires1 = GetNewFires();

        if( Verbose >= CallLevel )
          printf( "%*sfsxwutil:Intersections::CrossCompare:4b2 "
                  "numpts[%d]=%ld numcross=%ld\n",
                  CallLevel, "", 0, GetNumPoints(0), numcross );

        CleanPerimeter( *CurrentFire );      // REMOVE IF USING OLD ALGORITHM

        if( Verbose >= CallLevel )
          printf( "%*sfsxwutil:Intersections::CrossCompare:4b3 "
                  "numpts[%d]=%ld\n",
                  CallLevel, "", 0, GetNumPoints(0) );

        if( CheckPostFrontal(GETVAL) && GetNumPoints(*CurrentFire) > 0 ) {
          if( Verbose >= CallLevel )
            printf( "%*sfsxwutil:Intersections::CrossCompare:4b3a "
                    "\n", CallLevel, "" );

          NewFires2 = GetNewFires();
          if( (PostFires = new long[NewFires2 - NewFires1 + 1]) != NULL ) {
            if( Verbose >= CallLevel )
              printf( "%*sfsxwutil:Intersections::CrossCompare:4b3a1 "
                      "\n", CallLevel, "" );

            PostFires[0] = *CurrentFire;
            for( i = 1; i < (NewFires2 - NewFires1 + 1); i++ )
              PostFires[i] = NewFires1 + i - 1;

            if( Verbose >= CallLevel )
              printf( "%*sfsxwutil:Intersections::CrossCompare:4b3a2 "
                      "\n", CallLevel, "" );

            post.CorrectFireRing( numcross, intersect, interpoint, PostFires,
                                  NewFires2 - NewFires1 + 1, PriorNumPoints );

            if( Verbose >= CallLevel )
              printf( "%*sfsxwutil:Intersections::CrossCompare:4b3a3 "
                      "\n", CallLevel, "" );

            delete[] PostFires;
          }
        }

        if( Verbose >= CallLevel )
          printf( "%*sfsxwutil:Intersections::CrossCompare:4b4 "
                  "numpts[%d]=%ld\n",
                  CallLevel, "", 0, GetNumPoints(0) );

        if( GetNumPoints(*CurrentFire) > 0 ) { // SET TO 0
          rediscretize( CurrentFire, false );  //REMOVE IF USING OLD ALGORITHM
          if( CheckPostFrontal(GETVAL) && NextFire != *CurrentFire )
            SetNewFireNumber( NextFire, *CurrentFire,
                              post.AccessReferenceRingNum(1, GETVAL) );
        }

      }
      else {
        if( Verbose >= CallLevel )
          printf( "%*sfsxwutil:Intersections::CrossCompare:4b6\n",
                  CallLevel, "" );

        if( GetInout(NextFire) == 3 ) {  // if merging fire with barrier
          if( Verbose >= CallLevel )
            printf( "%*sfsxwutil:Intersections::CrossCompare:4b6a\n",
                    CallLevel, "" );
          OrganizeIntersections( *CurrentFire );
          MergeBarrier( CurrentFire, NextFire );
        }
        else {
          if( Verbose >= CallLevel )
            printf( "%*sfsxwutil:Intersections::CrossCompare:4b6b\n",
                    CallLevel, "" );
          if( GetInout(*CurrentFire) == 2 ) {
            if( GetNumPoints(*CurrentFire) < GetNumPoints(NextFire) )
              writenum = 1;
          }
          if( ! MergeFire(CurrentFire, NextFire) ) Result = false;
          Result = false;
        }

        if( Verbose >= CallLevel )
          printf( "%*sfsxwutil:Intersections::CrossCompare:4b7\n",
                  CallLevel, "" );
      }
    }

    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::CrossCompare:4c\n", CallLevel, "" );
  }
  //If no crosspoints, and searching within same fire.
  else if( *CurrentFire == NextFire ) {
    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::CrossCompare:4d numpts[%d]=%ld\n",
              CallLevel, "", 0, GetNumPoints(0) );

    if( GetNumPoints(*CurrentFire) > 0 ) {  //If points in fire
      tranz( *CurrentFire, 0 );
      if( Verbose >= CallLevel )
        printf( "%*sfsxwutil:Intersections::CrossCompare:4d1 "
                "numpts[%d]=%ld\n",
                CallLevel, "", 0, GetNumPoints(0) );

      rediscretize( CurrentFire, false );
      if( Verbose >= CallLevel )
        printf( "%*sfsxwutil:Intersections::CrossCompare:4d2 "
                "numpts[%d]=%ld\n",
                CallLevel, "", 0, GetNumPoints(0) );

      if( CheckPostFrontal(GETVAL) && NextFire != *CurrentFire )
        SetNewFireNumber( NextFire, *CurrentFire,
                          post.AccessReferenceRingNum(1, GETVAL) );
    }
    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::CrossCompare:4e numpts[%d]=%ld\n",
              CallLevel, "", 0, GetNumPoints(0) );
  }         //No crosses and searching between fires
  else if( GetInout(*CurrentFire) == 1 && GetInout(NextFire) == 1 ) {
    CheckEnvelopedFires( *CurrentFire, NextFire );
  }
  else if( GetInout(NextFire) == 3 ) {
    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::CrossCompare:4h\n", CallLevel, "" );

    if( GetInout(*CurrentFire) == 1 )
      CheckEnvelopedFires( *CurrentFire, NextFire );
    else if( GetInout(*CurrentFire) == 2 )
      MergeBarrierNoCross( CurrentFire, NextFire );
  }
  if( Verbose >= CallLevel )
    printf( "%*sfsxwutil:Intersections::CrossCompare:5 numpts[%d]=%ld\n",
            CallLevel, "", 0, GetNumPoints(0) );

  CallLevel--;

  return Result;
} // Intersections::CrossCompare

//============================================================================
void Intersections::CheckEnvelopedFires( long Fire1, long Fire2 )
{ //Intersections::CheckEnvelopedFires
  long i, j, k, m;
  long NumPts1, NumPts2;
  long NumSmall, FireSmall, FireBig;
  long Inside1, Inside2;
  double XLo1, XLo2, XHi1, XHi2;
  double YLo1, YLo2, YHi1, YHi2;
  double fli;

  NumPts1 = GetNumPoints(Fire1);
  NumPts2 = GetNumPoints(Fire2);
  XLo1 = GetPerimeter1Value(Fire1, NumPts1, XCOORD);
  XHi1 = GetPerimeter1Value(Fire1, NumPts1, YCOORD);
  YLo1 = GetPerimeter1Value(Fire1, NumPts1, XCOORD);
  YHi1 = GetPerimeter1Value(Fire1, NumPts1, YCOORD);
  XLo2 = GetPerimeter1Value(Fire2, NumPts2, XCOORD);
  XHi2 = GetPerimeter1Value(Fire2, NumPts2, YCOORD);
  YLo2 = GetPerimeter1Value(Fire2, NumPts2, XCOORD);
  YHi2 = GetPerimeter1Value(Fire2, NumPts2, YCOORD);
  Inside1 = 0;
  Inside2 = 1;

  // fires must overlap to get to this point, thus must find out which
  // one is smaller and could fit inside larger one
  if( (XHi1 - XLo1) * (YHi1 - YLo1) < (XHi2 - XLo2) * (YHi2 - YLo2) ) {
    NumSmall = NumPts1;
    FireSmall = Fire1;
    FireBig = Fire2;
  }
  else {
    NumSmall = NumPts2;
    FireSmall = Fire2;
    FireBig = Fire1;
  }

  for( i = 0; i < NumSmall; i++ ) {
    startx = GetPerimeter1Value(FireSmall, i, XCOORD);
    starty = GetPerimeter1Value(FireSmall, i, YCOORD);
    fli = GetPerimeter1Value(FireSmall, i, FLIVAL);
    if( fli >= 0.0 )    // don't test for extinguished points
      Inside1 = Overlap(FireBig);
    else
      continue;
    if( Inside1 ) {
      Inside2 = 0;
      for( j = 0; j < GetNewFires(); j++ ) {
        if( GetInout(j) == 2 ) {
          //NumPtsIn=GetNumPoints(j);
          if( BoundCross(FireBig, j) ) {    // if(overlap of outer and inner)
            Inside1 = 0;
            for( m = 0; m < GetNumPoints(j); m++ ) {
              startx = GetPerimeter1Value(j, m, XCOORD);
              starty = GetPerimeter1Value(j, m, YCOORD);
              Inside1 = Overlap(FireBig);

              if( Inside1 ) break;
            }

            if( Inside1 ) {  //!MergeInAndOutOK(FireBig, j))   // if(Inward is inside big one)
              for( k = 0; k < NumSmall; k++ ) {
                fli = GetPerimeter1Value(FireSmall, k, FLIVAL);
                if( fli > 0.0 ) {    // no points on barren ground
                  startx = GetPerimeter1Value(FireSmall, k, XCOORD);
                  starty = GetPerimeter1Value(FireSmall, k, YCOORD);
                  Inside2 = Overlap(j);

                  if( Inside2 ) break;
                }
              }
            }
            else continue;
          }
        }

        if( Inside2 ) break;
      }
      i = NumSmall;    // force exit from for loop
    }
  }

  if( ! Inside2 ) {
    SetInout(FireSmall, 0);
    SetNumPoints(FireSmall, 0);
    IncSkipFires(1);
    if( CheckPostFrontal(GETVAL) ) {
      SetNewFireNumber( FireSmall, -1,
                        post.AccessReferenceRingNum(1, GETVAL) );
    }
  }
} //Intersections::CheckEnvelopedFires

/*============================================================================
  StandardizePolygon::ReorderPerimeter
  Reorders points in CurrentFire from NewBeginning because startpoint is
  inside loop or crossover.
*/
void StandardizePolygon::ReorderPerimeter( long CurrentFire,
                                           long NewBeginning )
{ //StandardizePolygon::ReorderPerimeter
  long count, newcount = 0;
  double xpt, ypt, ros, fli, rcx;

  if( NewBeginning == 0 ) return;

  //If less than NewBeginning # of pts in perim2.
  if( GetPerimeter2Value(GETVAL, GETVAL) < GetNumPoints(CurrentFire) ) {
    FreePerimeter2();
    AllocPerimeter2( GetNumPoints(CurrentFire) + 1 );
  }

  //Read early points into perim2.
  for( count = 0; count < NewBeginning; count++) {
    xpt = GetPerimeter1Value( CurrentFire, count, XCOORD );
    ypt = GetPerimeter1Value( CurrentFire, count, YCOORD );
    ros = GetPerimeter1Value( CurrentFire, count, ROSVAL );
    fli = GetPerimeter1Value( CurrentFire, count, FLIVAL );
    rcx = GetPerimeter1Value( CurrentFire, count, RCXVAL );
    SetPerimeter2(count, xpt, ypt, ros, fli, rcx);
  }
  //Read remaining points to difft position in perim1.
  for( count = NewBeginning; count < GetNumPoints(CurrentFire); count++ ) {
    xpt = GetPerimeter1Value( CurrentFire, count, XCOORD );
    ypt = GetPerimeter1Value( CurrentFire, count, YCOORD );
    ros = GetPerimeter1Value( CurrentFire, count, ROSVAL );
    fli = GetPerimeter1Value( CurrentFire, count, FLIVAL );
    rcx = GetPerimeter1Value( CurrentFire, count, RCXVAL );
    SetPerimeter1( CurrentFire, newcount, xpt, ypt );
    SetFireChx( CurrentFire, newcount, ros, fli );
    SetReact( CurrentFire, newcount, rcx );
    ++newcount;
  }
  //Read early points at end.
  for( count = 0; count < NewBeginning; count++) {
    GetPerimeter2( count, &xpt, &ypt, &ros, &fli, &rcx );
    SetPerimeter1( CurrentFire, newcount, xpt, ypt );
    SetFireChx( CurrentFire, newcount, ros, fli );
    SetReact( CurrentFire, newcount, rcx );
    ++newcount;
  }
} //StandardizePolygon::ReorderPerimeter


void StandardizePolygon::RemoveDuplicatePoints(long CurrentFire)
{
	long i, j, k, orient, NumPoints;
	double xpt, ypt, ros, negfli, fli, xptl, yptl, xptn, yptn;
	double dist, distn;
	double xlo, xhi, ylo, yhi, rcx;

	NumPoints = GetNumPoints(CurrentFire);
	if (NumPoints == 0)
		return;

	//FreePerimeter2();
	AllocPerimeter2(NumPoints + 1);
	j = 0;
	negfli = 1.0;
	xptl = xlo = xhi = GetPerimeter1Value(CurrentFire, NumPoints - 1, XCOORD);
	yptl = ylo = yhi = GetPerimeter1Value(CurrentFire, NumPoints - 1, YCOORD);
	for (i = 0; i < NumPoints; i++)
	{
		xpt = GetPerimeter1Value(CurrentFire, i, XCOORD);
		ypt = GetPerimeter1Value(CurrentFire, i, YCOORD);
		dist = pow2(xpt - xptl) + pow2(ypt - yptl);
		k = i + 1;
		if (k > NumPoints - 1)
			k -= NumPoints;
		xptn = GetPerimeter1Value(CurrentFire, k, XCOORD);
		yptn = GetPerimeter1Value(CurrentFire, k, YCOORD);
		distn = pow2(xptn - xptl) + pow2(yptn - yptl);
    if( dist > 1e-12 && distn > 1e-12 ) {
      Point P1( xptl, yptl );
      Point P2( xpt, ypt );
      Point P3( xptn, yptn );
      orient = P1.GetOrientation( P2, P3 );
      if( orient == 0 && distn <= dist ) {
				xpt = xptn;
				ypt = yptn;
				negfli = GetPerimeter1Value(CurrentFire, i, FLIVAL);
				i++;
			}
			ros = GetPerimeter1Value(CurrentFire, i, ROSVAL);
			fli = GetPerimeter1Value(CurrentFire, i, FLIVAL);
			rcx = GetPerimeter1Value(CurrentFire, i, RCXVAL);
			if (negfli < 0.0 && fli >= 0.0) 			  // don't remove suppressed points
				fli = negfli;
			negfli = 1.0;
			SetPerimeter2(j, xpt, ypt, ros, fli, rcx);
			j++;
			xptl = xpt;
			yptl = ypt;
			if (xpt < xlo)
				xlo = xpt;
			if (xpt > xhi)
				xhi = xpt;
			if (ypt < ylo)
				ylo = ypt;
			if (ypt > yhi)
				yhi = ypt;
		}
	}
	if (j > 0 && j < NumPoints)
	{
		SetPerimeter2(j, xlo, xhi, ylo, yhi, 0.0);
		SwapFirePerims(CurrentFire, -(j + 1));
		SetNumPoints(CurrentFire, j);
	}
}


void StandardizePolygon::BoundingBox(long NumFire)
{
	double xpt, ypt, Xlo, Xhi, Ylo, Yhi;
	long NumPoint = GetNumPoints(NumFire);

	Xlo = Xhi = GetPerimeter1Value(NumFire, 0, 0);
	Ylo = Yhi = GetPerimeter1Value(NumFire, 0, 1);
	for (int i = 1; i < NumPoint; i++)
	{
		xpt = GetPerimeter1Value(NumFire, i, 0);
		ypt = GetPerimeter1Value(NumFire, i, 1);
		if (xpt < Xlo)
			Xlo = xpt;
		else
		{
			if (xpt > Xhi)
				Xhi = xpt;
		}
		if (ypt < Ylo)
			Ylo = ypt;
		else
		{
			if (ypt > Yhi)
				Yhi = ypt;
		}
	}
	SetPerimeter1(NumFire, NumPoint, Xlo, Xhi);
	SetFireChx(NumFire, NumPoint, Ylo, Yhi);
}


void Intersections::MergeBarrierNoCross(long* CurrentFire, long NextFire)
{
	// merge active fire front with vector fire barrier
	long i, NumPts, inside;
	double ros, fli;

	NumPts = GetNumPoints(*CurrentFire);
	for (i = 0; i < NumPts; i++)
	{
		startx = GetPerimeter1Value(*CurrentFire, i, XCOORD);
		starty = GetPerimeter1Value(*CurrentFire, i, YCOORD);
		fli = GetPerimeter1Value(*CurrentFire, i, FLIVAL);
		if (fli >= 0.0)
		{
			inside = Overlap(NextFire);
			if (inside)
			{
				ros = GetPerimeter1Value(*CurrentFire, i, ROSVAL);
				if (fli == 0.0)
					fli = 1.0;
				SetFireChx(*CurrentFire, i, ros, -fli);
			}
		}
	}
	//rediscretize(CurrentFire);
}


void Intersections::MergeBarrier(long* CurrentFire, long NextFire)
{
	// neutralize points intersecting and falling inside barrier
	long i, j, Start, End, NumPts, NumNewPoints, inside;
	double xpt, ypt, ros, fli, rcx, dist;

	//if(numcross<2) // eliminated because allows points to leak across barriers
	//     return;

	NumPts = GetNumPoints(*CurrentFire);
	AllocPerimeter2(NumPts + numcross + 1);
	Start = NumNewPoints = 0;
	for (i = 0; i <= numcross; i++) 		 // for each cross
	{
		if (i < numcross)
			End = GetSpan(i, 0);		 // use start of crossed span 1 as end point
		else
			End = NumPts;
		for (j = Start;
			j <= End;
			j++)  	 // transfer from beginning of perim1 to end
		{
			startx = xpt = GetPerimeter1Value(*CurrentFire, j, XCOORD);
			starty = ypt = GetPerimeter1Value(*CurrentFire, j, YCOORD);
			ros = GetPerimeter1Value(*CurrentFire, j, ROSVAL);
			fli = GetPerimeter1Value(*CurrentFire, j, FLIVAL);
			rcx = GetPerimeter1Value(*CurrentFire, j, RCXVAL);
			if (j < NumPts && fli >= 0.0)
			{
				inside = Overlap(NextFire);
				if (inside)
				{
					if (fli > 0.0)
						fli *= -1.0;
					if (fli == 0.0)
						fli = -1.0;
				}
			}
			else
				inside = 0;
			SetPerimeter2(NumNewPoints++, xpt, ypt, ros, fli, rcx);
		}
		Start = End + 1;			   // set new start point
		if (i < numcross)
		{
			GetInterPointCoord(i, &xpt, &ypt);
			dist = pow2(xpt - startx) + pow2(ypt - starty);
			if (dist > 0.1) 		// don't allow duplicate points
			{
				GetInterPointFChx(i, &ros, &fli, &rcx);
				if (fli > 0.0)
					fli *= -1.0;
				else if (fli == 0.0)
					fli = -1.0;
				SetPerimeter2(NumNewPoints++, xpt, ypt, ros, fli, rcx);
				startx = xpt;    // in case of multiple crosses on same span
				starty = ypt;
			}
		}
	}
	if (NumNewPoints > (NumPts + 1))
	{
		AllocPerimeter1(*CurrentFire, NumNewPoints);
	}
	tranz(*CurrentFire, NumNewPoints);
	SetNumPoints(*CurrentFire, NumNewPoints - 1);
}


void Intersections::OrganizeIntersections(long CurrentFire)
{
	bool Reverse;//, Continue;
	long i, j, k, m, n;
	long samechek1, samechek2;
	double x1, y1, x2, y2, rcx1, rcx2;
	double xpt, ypt, ros1, fli1, ros2, fli2, diff1, diff2;

	for (i = 0; i < numcross; i++)
	{
		samechek1 = GetSpan(i, 0);
		j = i + 1;
		while (j < numcross)
		{
			samechek2 = GetSpan(j, 0);
			if (samechek2 != samechek1)
				break;
			j++;
		}
		if (j >= numcross && samechek2 != samechek1)
			break;
		if (j - i == 1)
			continue;

		xpt = GetPerimeter1Value(CurrentFire, samechek1, XCOORD);
		ypt = GetPerimeter1Value(CurrentFire, samechek1, YCOORD);
		do
		{
			Reverse = false;
			for (k = i; k < j; k++)
			{
				GetInterPointCoord(k, &x1, &y1);//ipoints[k*4];
				diff1 = pow2(xpt - x1) + pow2(ypt - y1);
				for (m = k + 1; m < j; m++)
				{
					GetInterPointCoord(m, &x2, &y2);
					diff2 = pow2(xpt - x2) + pow2(ypt - y2);
					if (diff2 < diff1)
					{
						Reverse = true;
						GetInterPointFChx(m, &ros2, &fli2, &rcx2);
						GetInterPointFChx(k, &ros1, &fli1, &rcx1);
						SetInterPoint(k, x2, y2, ros2, fli2, rcx2);
						SetInterPoint(m, x1, y1, ros1, fli1, rcx1);
						GetIntersection(m, &n, &samechek2);
						GetIntersection(k, &n, &samechek1);
						SetIntersection(m, n, samechek1);
						SetIntersection(k, n, samechek2);
						k = j + 1;	// force break out of outer for statement
						break;
					}
				}
			}
		}
		while (Reverse);
		i = j - 1;
	}
}

/*============================================================================
  Intersections::OrganizeCrosses
  Reorders intersections and adds points between multiple intersections on a
  span.
*/
void Intersections::OrganizeCrosses( long CurrentFire )
{ //Intersections::OrganizeCrosses
  long StartCheck, NextCheck, SwitchCheck1, SwitchCheck2;
  long Isect1a, Isect1b, Isect2a, Isect2b;
  bool CheckFound = false, CrossFound = false;
  double point1x, point2x, point1y, point2y;
  double xpt1, ypt1, ros1, fli1, rcx1;
  double xpt2, ypt2, ros2, fli2, rcx2;
  double xpt3, ypt3, ros3, fli3, rcx3;
  double xdiff, ydiff, dist1, dist2;

  CallLevel++;

  if( Verbose >= CallLevel )
    printf( "%*sfsxwutil:Intersections::OrganizeCrosses:1 "
            "numpts[%d]=%ld numcross=%ld\n",
            CallLevel, "", 0, GetNumPoints(0), numcross );

  for( StartCheck = 0; StartCheck < numcross; StartCheck++ ) {
    NextCheck = StartCheck;
    GetIntersection( StartCheck, &Isect1a, &Isect1b );
    GetIntersection( ++NextCheck, &Isect2a, &Isect2b );
    while( Isect1a == Isect2a ) {
      GetIntersection( ++NextCheck, &Isect2a, &Isect2b );
      CheckFound = true;
      CrossFound = true;
    };
    if( CheckFound ) {
      CheckFound = false;
      NextCheck--;
      xpt1 = GetPerimeter1Value( CurrentFire, Isect1a, XCOORD );
      ypt1 = GetPerimeter1Value( CurrentFire, Isect1a, YCOORD );
      for( SwitchCheck1 = StartCheck; SwitchCheck1 < NextCheck;
           SwitchCheck1++ ) {
        GetIntersection( SwitchCheck1, &Isect1a, &Isect1b );
        GetInterPointCoord( SwitchCheck1, &point1x, &point1y );
        xdiff = pow2( xpt1 - point1x );
        ydiff = pow2( ypt1 - point1y );
        dist1 = xdiff + ydiff;
        for( SwitchCheck2 = SwitchCheck1 + 1; SwitchCheck2 <= NextCheck;
             SwitchCheck2++ ) {
          GetIntersection( SwitchCheck2, &Isect2a, &Isect2b );
          GetInterPointCoord( SwitchCheck2, &point2x, &point2y );
          xdiff = pow2( xpt1 - point2x );
          ydiff = pow2( ypt1 - point2y );
          dist2 = xdiff + ydiff;
          if( dist2 < dist1 ) {
            dist1 = dist2;
            SetIntersection( SwitchCheck1, Isect2a, Isect2b );
            GetInterPointFChx( SwitchCheck2, &ros2, &fli2, &rcx2 );
            SetInterPoint( SwitchCheck1, point2x, point2y, ros2, fli2, rcx2 );
            SetIntersection( SwitchCheck2, Isect1a, Isect1b );
            GetInterPointFChx( SwitchCheck1, &ros2, &fli2, &rcx2 );
            SetInterPoint( SwitchCheck2, point1x, point1y, ros2, fli2, rcx2 );
            Isect1a = Isect2a;
            Isect1b = Isect2b;
            point1x = point2x;
            point1y = point2y;
          }
        }
      }
      StartCheck = NextCheck;
    }
  }

  if( Verbose >= CallLevel )
    printf( "%*sfsxwutil:Intersections::OrganizeCrosses:2 numpts[%d]=%ld\n",
            CallLevel, "", 0, GetNumPoints(0) );

  if( CrossFound ) {
    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::OrganizeCrosses:2a "
              "numpts[%d]=%ld\n",
              CallLevel, "", 0, GetNumPoints(0) );

    AllocPerimeter2( GetNumPoints(CurrentFire) + 2 * numcross );

    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::OrganizeCrosses:2b "
              "numpts[%d]=%ld numcross=%ld\n",
              CallLevel, "", 0, GetNumPoints(0), numcross );

    long NumNewPoints = 0, NumPointsWritten = 0, NextPoint;
    for( StartCheck = 0; StartCheck < numcross; StartCheck++ ) {
      NextCheck = StartCheck + 1;
      if( NextCheck > numcross - 1 ) NextCheck = 0;
      GetIntersection( StartCheck, &Isect1a, &Isect1b );
      GetIntersection( NextCheck, &Isect2a, &Isect2b );
      for( NextPoint = NumPointsWritten; NextPoint <= Isect2a; NextPoint++ ) {
        xpt1 = GetPerimeter1Value( CurrentFire, NextPoint, XCOORD );
        ypt1 = GetPerimeter1Value( CurrentFire, NextPoint, YCOORD );
        ros1 = GetPerimeter1Value( CurrentFire, NextPoint, ROSVAL );
        fli1 = GetPerimeter1Value( CurrentFire, NextPoint, FLIVAL );
        rcx1 = GetPerimeter1Value( CurrentFire, NextPoint, RCXVAL );
        SetPerimeter2( NextPoint + NumNewPoints, xpt1, ypt1, ros1, fli1,
                       rcx1 );
      }
      NumPointsWritten = NextPoint;
      if( Isect1a == Isect2a ) {
        GetInterPointCoord( StartCheck, &xpt1, &ypt1 );
        GetInterPointFChx( StartCheck, &ros1, &fli1, &rcx1 );
        GetInterPointCoord( NextCheck, &xpt2, &ypt2 );
        GetInterPointFChx( NextCheck, &ros2, &fli2, &rcx2 );
        xpt3 = xpt2 - ( xpt2 - xpt1 ) / 2.0;
        ypt3 = ypt2 - ( ypt2 - ypt1 ) / 2.0;
        dist1 = sqrt( pow2(xpt2 - xpt1) + pow2(ypt2 - ypt1) );
        if( dist1 > 1e-8 ) {
          ros3 = ( ros1 + ros2 ) / 2.0;
          fli3 = ( fabs(fli1) + fabs(fli2) ) / 2.0;
          rcx3 = ( rcx1 + rcx2 ) / 2.0;
          if( fli1 < 0.0 || fli2 < 0.0 ) fli3 *= -1.0;
          SetPerimeter2( NumPointsWritten + NumNewPoints, xpt3, ypt3, ros3,
                         fli3, rcx3 );
          NumNewPoints++;
        }
        else {
          dist1 += 1;
          xpt3 += dist1;
        }
      }
    }

    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::OrganizeCrosses:2c "
              "numpts[%d]=%ld NumNewPoints=%ld\n",
              CallLevel, "", 0, GetNumPoints(0), NumNewPoints );

    for( NextPoint = NumPointsWritten; NextPoint <= GetNumPoints(CurrentFire);
         NextPoint++ ) {
      xpt1 = GetPerimeter1Value( CurrentFire, NextPoint, XCOORD );
      ypt1 = GetPerimeter1Value( CurrentFire, NextPoint, YCOORD );
      ros1 = GetPerimeter1Value( CurrentFire, NextPoint, ROSVAL );
      fli1 = GetPerimeter1Value( CurrentFire, NextPoint, FLIVAL );
      rcx1 = GetPerimeter1Value( CurrentFire, NextPoint, RCXVAL );
      SetPerimeter2( NextPoint + NumNewPoints, xpt1, ypt1, ros1, fli1, rcx1 );
    }

    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::OrganizeCrosses:2d "
              "numpts[%d]=%ld\n",
              CallLevel, "", 0, GetNumPoints(0) );

    NumPointsWritten = NextPoint;

    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::OrganizeCrosses:2e "
              "numpts[%d]=%ld\n",
              CallLevel, "", 0, GetNumPoints(0) );

    AllocPerimeter1( CurrentFire, NumPointsWritten + NumNewPoints );

    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::OrganizeCrosses:2f "
              "numpts[%d]=%ld\n",
              CallLevel, "", 0, GetNumPoints(0) );

    tranz( CurrentFire, NumPointsWritten + NumNewPoints );

    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::OrganizeCrosses:2g numpts[%d]=%ld "
              "NumPointsWritten=%ld NumNewPoints=%ld\n",
              CallLevel, "", 0, GetNumPoints(0), NumPointsWritten,
              NumNewPoints );

    SetNumPoints( CurrentFire, NumPointsWritten + NumNewPoints - 1 );

    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:Intersections::OrganizeCrosses:2h "
              "numpts[%d]=%ld\n",
              CallLevel, "", 0, GetNumPoints(0) );
  }

  if( Verbose >= CallLevel )
    printf( "%*sfsxwutil:Intersections::OrganizeCrosses:3 numpts[%d]=%ld\n",
            CallLevel, "", 0, GetNumPoints(0) );
  CallLevel--;
} //Intersections::OrganizeCrosses

/*
void Intersections::OrganizeIntersections(long CurrentFire)
{// reorders intersections and interpoints for intersections between fires
 // to order they would be encountered
	 long inc, inc1, inc2, samechek1, samechek2, xchek;
	 double xpt, ypt, ros1, fli1, ros2, fli2, xdiff, ydiff;
	 double samepoint1x=0, samepoint1y=0, samepoint2x=0, samepoint2y=0;
	double dist1, dist2;

	 for(inc=0; inc<numcross; inc++)
	{    samechek1=GetSpan(inc, 0);    // reverses order of spike intersections where two fires merge
		inc2=inc+1; 				  // and form internal fires
		  if(inc2>numcross-1)
			  	break;
  		samechek2=GetSpan(inc2, 0);
  		if(samechek2==samechek1)
  		{    GetInterPointCoord(inc, &samepoint1x, &samepoint1y);
  			GetInterPointCoord(inc2, &samepoint2x, &samepoint2y);
  			xpt=GetPerimeter1Value(CurrentFire, samechek1, XCOORD);
  			ypt=GetPerimeter1Value(CurrentFire, samechek1, YCOORD);
  			xdiff=pow2(xpt-samepoint1x);
  			ydiff=pow2(ypt-samepoint1y);
  			dist1=xdiff+ydiff;
  			xdiff=pow2(xpt-samepoint2x);
  			ydiff=pow2(ypt-samepoint2y);
  			dist2=xdiff+ydiff;
  			if(dist1>dist2)
  			{	do
  				{    if(++inc2<numcross)
  						samechek2=GetSpan(inc2, 0);
  					else
							 	break;
	 			} while(samechek2==samechek1);
				inc1=inc;			// start switching here, end switching at inc2-1
				inc=inc2-1;   	// go on to next intersection in for loop
				while(inc2>inc1)
				{    inc2--;
					GetIntersection(inc2, &xchek, &samechek2);		// reorder addresses and intersection points
					GetIntersection(inc1, &xchek, &samechek1);
					GetInterPointCoord(inc2, &samepoint2x, &samepoint2y);
					GetInterPointCoord(inc1, &samepoint1x, &samepoint1y);
					GetInterPointFChx(inc2, &ros2, &fli2);
					GetInterPointFChx(inc1, &ros1, &fli1);
					SetIntersection(inc1, xchek, samechek2);
					SetIntersection(inc2, xchek, samechek1);
					SetInterPoint(inc1, samepoint2x, samepoint2y, ros2, fli2);
					SetInterPoint(inc2, samepoint1x, samepoint1y, ros1, fli1);
					inc1++;
				}
			}
		}
	}
}
*/


bool Intersections::MergeFire( long* CurrentFire, long NextFire )
{ //Intersections::MergeFire
  // clips loops and merges fires, finds and stores enclaves
  long xsect1 = 0, xsect2 = 0, xsect3 = 0, xsect4 = 0, xsect5, xsect6,
       xsect7, xsect8;
  long inc = 0, outcross = 0, offcheck = 0;
  long fxend = 0, bxend = 0, Firstin = 0, Secondin = 0;
  long numchek = 0, chekct = 0, newnump = 0, crosstype = 0;
  long freadstart = 0, freadend, breadstart = 0, breadend = 0;
  long forewrite = 0, backwrite = 0, xwrite = 0;
  long* PostFires;
  double diff1, diff2;

  noffset1 = 0; noffset2 = 0;
  if( *CurrentFire == NextFire ) {   // if merging loops on one fire
    /*------------------------------------------------------------------//
    // ##### Eliminated because based on point density 12/23/1994 #####
    if(GetInout(*CurrentFire)==1) { // for outward fires only
      diff2=GetNumPoints(*CurrentFire)/2;
      for(numchek=0;numchek<numcross;numchek++) {
        GetIntersection(numchek, &xsect1, &xsect2);
        diff1=xsect2-xsect1;
        if(diff1>diff2) {
          if(numchek!=0) {
            GetIntersection(numchek-1, &xsect3, &xsect4);
            diff1=xsect4-xsect3;
            if(xsect1==xsect3) {
              if(diff1>0 && diff1<diff2)
                noffset1=xsect4+1;
              else
                noffset1=xsect1+1;
            }
            else
              noffset1=xsect1+1;
          }
          else
            noffset1=xsect1+1;  // offsets for array addresses
            noffset2=GetNumPoints(*CurrentFire);
            noffset2=noffset2-noffset1;
            GetIntersection(++numchek, &xsect3, &xsect4);
            if(xsect3==xsect1) {
              while(xsect3==xsect1) {
                GetIntersection(++inc+numchek, &xsect3, &xsect6);
              }
            }
            else {
              if(xsect4==xsect2) {
                while(xsect4==xsect2) {
                  //xsect4=intersect[++inc+numchek][1];
                  GetIntersection(++inc+numchek, &xsect5, &xsect4);
                }
              }
            }
            outcross=inc+numchek; // offset for numchek
            inc=0;
            numchek--;
          }
        }
      }
    --------------------------------------------------------------------*/
    long NumExistingFires = GetNewFires();
    numchek = 0;
    AllocSwap(GetNumPoints(*CurrentFire));
    while (numchek < numcross) {    // for all line crosses
      offcheck = (numchek + outcross);
      GetOffcheck(&offcheck);
      xsect1 = intercode(offcheck, 0);
      xsect2 = intercode(offcheck, 1);  // =0 the first time through by default
      fxend = offcheck;
      offcheck = (++numchek + outcross);
      GetOffcheck(&offcheck);
      xsect3 = intercode(offcheck, 0);
      xsect4 = intercode(offcheck, 1);
      if( xsect3 != xsect2 ) {    // if two crosses in a row
        xsect5 = xsect3;
        xsect6 = xsect4;
        if( xsect5 != xsect1 && xsect6 != xsect2 ) { // if only one cross on 1-2 span
          xsect7 = xsect1;
          xsect8 = xsect2;
          while( xsect5 < xsect8 ) {
            offcheck = (++numchek + outcross);
            GetOffcheck(&offcheck);
            xsect5 = intercode(offcheck, 0);
            xsect6 = intercode(offcheck, 1);
            if( xsect8 <= xsect6 ) {
              xsect7 = xsect5;
              xsect8 = xsect6;
            }
          }

          while (xsect6 != xsect7) {
            offcheck = (++numchek + outcross);
            GetOffcheck(&offcheck);
            xsect6 = intercode(offcheck, 1);
          }

          if( xsect5 == xsect2 && xsect6 == xsect1 ) {
            offcheck = (numchek + outcross - 1);   // BACKS UP ONE IN INTERSECT[] TO ELIMINATE SINGLE CROSS-OVERS
            GetOffcheck(&offcheck);
            xsect7 = intercode(offcheck, 0);
            xsect8 = intercode(offcheck, 1);
            if( xsect7 > xsect4 )
              xsect4 = xsect7;    //  TEST 12/29/1994
            if( xsect7 == xsect4 && xsect8 == xsect3 ) {  // IF DOUBLE LOOP WITH INTERNAL FIRE
              if( xsect4 - xsect3 > 2 ) {     // if internal fire is > minimum point number 10
                backwrite = 0;
                breadstart = xsect3 + 1;
                bxend = offcheck;
                breadend = xsect4;
                xwrite = backwrite;
                readnum = *CurrentFire;
                writenum = GetNewFires();
                MergeWrite(bxend, breadstart, breadend, &xwrite);
                backwrite = xwrite;
                AllocPerimeter1(GetNewFires(), xwrite + 1);
                SetInout(GetNewFires(), 2);
                SetNumPoints(GetNewFires(), xwrite);
                SwapTranz(GetNewFires(), xwrite);
                BoundaryBox(xwrite);
                IncNewFires(1);
              }
            }
            else {
              fxend = -1;
            }
          }
          else {
            fxend = -1;
          }
          freadend = xsect1;
          xwrite = forewrite;
          readnum = *CurrentFire;
          writenum = NextFire;
          MergeWrite(fxend, freadstart, freadend, &xwrite);
          forewrite = xwrite;
          offcheck = (1 + numchek + outcross);
          GetOffcheck(&offcheck);
          xsect7 = intercode(offcheck, 0);
          if( xsect7 != xsect5 )
            freadstart = xsect5 + 1;
          else
            freadstart = -1;
        }
        else {   // more than one cross on 1-2 span
          if( xsect5 == xsect1 ) {   // if spike loop
            while( xsect3 == xsect1 ) {
              offcheck = (++numchek + outcross);
              GetOffcheck(&offcheck);
              xsect3 = intercode(offcheck, 0);
            }
            offcheck = (--numchek + outcross);
            GetOffcheck(&offcheck);
            xsect3 = intercode(offcheck, 0);
            xsect4 = intercode(offcheck, 1);
            fxend = offcheck;
            xsect5 = xsect3;
            xsect7 = xsect3;
            xsect6 = xsect4;
            xsect8 = xsect4;
            while( xsect5 < xsect8 ) {    // traps reverse spike
              offcheck = (++numchek + outcross);
              GetOffcheck(&offcheck);
              xsect5 = intercode(offcheck, 0);
              xsect6 = intercode(offcheck, 1);
              if( xsect8 <= xsect6 ) {
                xsect8 = xsect6;
                xsect7 = xsect5;
              }
            }
            while( xsect6 != xsect7 ) {
              offcheck = (++numchek + outcross);
              GetOffcheck(&offcheck);
              xsect6 = intercode(offcheck, 1);
            }

            /*if(xsect5>xsect4)
              { xsect4=xsect5;    // TEST 12/29/1994
              inc=-1;
              }*/
            if( xsect5 == xsect4 && xsect6 == xsect3 ) {
              if( xsect4 - xsect3 > 2 ) {       // if internal fire is > minimum point number 10
                backwrite = 0;
                breadstart = xsect3 + 1 + inc;  // +inc is TEST
                bxend = offcheck;
                breadend = xsect4;
                xwrite = backwrite;
                readnum = *CurrentFire;
                writenum = GetNewFires();
                MergeWrite(bxend, breadstart, breadend, &xwrite);
                backwrite = xwrite;
                AllocPerimeter1(GetNewFires(), xwrite + 1);
                SetInout(GetNewFires(), 2);
                SetNumPoints(GetNewFires(), xwrite);
                SwapTranz(GetNewFires(), xwrite);
                BoundaryBox(xwrite);
                IncNewFires(1);
                inc = 0;      // inc is TEST
              }
            }
            else fxend = -1;

            freadend = xsect1;
            xwrite = forewrite;
            readnum = *CurrentFire;
            writenum = NextFire;
            MergeWrite(fxend, freadstart, freadend, &xwrite);
            forewrite = xwrite;
            offcheck = (1 + numchek + outcross);
            GetOffcheck(&offcheck);
            xsect5 = intercode(offcheck, 0);
            if( xsect5 != xsect8 )
              freadstart = xsect8 + 1;
            else
              freadstart = -1;
          }
          else {    // spike starts on 1-2 span, "alternate spike loop"
            while( xsect6 == xsect2 ) {
              offcheck = (++numchek + outcross);
              GetOffcheck(&offcheck);
              xsect6 = intercode(offcheck, 1);
            }
            offcheck = (--numchek + outcross);
            GetOffcheck(&offcheck);
            xsect5 = intercode(offcheck, 0);
            xsect6 = intercode(offcheck, 1);
            xsect7 = xsect5;
            xsect8 = xsect6;
            while( xsect5 < xsect8 ) {
              offcheck = (++numchek + outcross);
              GetOffcheck(&offcheck);
              xsect5 = intercode(offcheck, 0);
              xsect6 = intercode(offcheck, 1);
              if( xsect8 <= xsect6 ) {
                xsect8 = xsect6;
                xsect7 = xsect5;
              }
            }
            while( xsect6 != xsect7 ) {
              offcheck = (++numchek + outcross);
              GetOffcheck(&offcheck);
              xsect6 = intercode(offcheck, 1);
            }
            /*  if(xsect5>xsect4)
                {  xsect4=xsect5;    // TEST 12/29/1994
                  inc=-1;
              }*/
            if( xsect5 == xsect4 && xsect6 == xsect3 ) {
              if( xsect4 - xsect3 > 2 ) {    // if internal fire is > minimum point number 20
                backwrite = 0;
                breadstart = xsect3 + 1 + inc;  // +inc is TEST
                bxend = offcheck;
                breadend = xsect4;
                xwrite = backwrite;
                readnum = *CurrentFire;
                writenum = GetNewFires();
                MergeWrite( bxend, breadstart, breadend, &xwrite );
                backwrite = xwrite;
                AllocPerimeter1(GetNewFires(), xwrite + 1);
                SetInout(GetNewFires(), 2);
                SetNumPoints(GetNewFires(), xwrite);
                SwapTranz(GetNewFires(), xwrite);
                BoundaryBox(xwrite);
                IncNewFires(1);
                inc = 0;    // inc is TEST
              }
            }
            else fxend = -1;

            freadend = xsect1;
            xwrite = forewrite;
            readnum = *CurrentFire;
            writenum = NextFire;
            MergeWrite(fxend, freadstart, freadend, &xwrite);
            forewrite = xwrite;
            offcheck = (1 + numchek + outcross);
            GetOffcheck(&offcheck);
            xsect7 = intercode(offcheck, 0);
            if( xsect7 != xsect5 )
              freadstart = xsect8 + 1;
            else
              freadstart = -1;
          }
        }
        offcheck = (++numchek + outcross);
        GetOffcheck(&offcheck);
      }
      else {     // clip single loop
        freadend = xsect1;   //freadstart=0 by default or other freadstart
        xwrite = forewrite;
        readnum = *CurrentFire;
        writenum = NextFire;
        MergeWrite(fxend, freadstart, freadend, &xwrite);
        forewrite = xwrite;
        offcheck = (++numchek + outcross);
        GetOffcheck(&offcheck);
        xsect5 = intercode(offcheck, 0);
        if( offcheck != numcross && xsect5 != xsect3 )    // no twin spike loop on xsect3 span
          freadstart = xsect3 + 1;
        else
          freadstart = -1;
      }
    }
    freadend = GetNumPoints(*CurrentFire) - 1;
    fxend = -1;
    xwrite = forewrite;
    readnum = *CurrentFire;
    writenum = NextFire;
    MergeWrite(fxend, freadstart, freadend, &xwrite);
    forewrite = xwrite;
    offcheck = (++numchek + outcross);
    GetOffcheck(&offcheck);
    OldNumPoints = GetNumPoints(*CurrentFire);    // store old number of points for rediscretize
    if( GetInout(*CurrentFire) == 2 ) {
      SetNumPoints(*CurrentFire, forewrite);
      //FreeSwap();    // must go before rediscretize which also uses swapperim
      rediscretize(CurrentFire, true);
    }
    else {
      if( forewrite > 8 || forewrite > ((double) OldNumPoints) / 2.0 ) {   // normal exit from MergeFire
        SetNumPoints(*CurrentFire, forewrite);
        //FreeSwap();      // must go before rediscretize which also uses swapperim
        rediscretize(CurrentFire, true);
      }
      else {    // reset new inward fires
        for( numchek = NumExistingFires; numchek < GetNewFires(); numchek++ ) {
          SetNumPoints(numchek, 0);
          SetInout(numchek, 0);
        }
        //-----------------------------------------------------
        //----- New Sequence Finds Only OuterPerim
        FindOuterFirePerimeter(*CurrentFire);
        //FreeSwap(); 	  // must go before rediscretize which also uses swapperim
        rediscretize(CurrentFire, false);
        //-----------------------------------------------------

        //-----------------------------------------------------
        //----- Old Sequence Leaves Fire In WITHOUT LOOP CLIPPING
        //FreeSwap();
        //SetNewFires(NumExistingFires);
        //-----------------------------------------------------

        return false;
      }
    }
  }      // if merging two different fires
  else if( numcross == 2 ) {    // only two intersections between fires, no internal fires
    writenum = *CurrentFire;
    //FreePerimeter2();
    AllocPerimeter2( 2 * GetNumPoints(*CurrentFire) +
                     2 * GetNumPoints(NextFire));// be safe and allocate enough for both arrays
    GetIntersection(numchek, &xsect1, &xsect2);
    GetIntersection(++numchek, &xsect3, &xsect4);
    startx = GetPerimeter1Value(*CurrentFire, 0, XCOORD);
    starty = GetPerimeter1Value(*CurrentFire, 0, YCOORD);
    Firstin = Overlap(NextFire);
    startx = GetPerimeter1Value(NextFire, 0, XCOORD);
    starty = GetPerimeter1Value(NextFire, 0, YCOORD);
    Secondin = Overlap(*CurrentFire);
    if( GetInout(*CurrentFire) == 2 ) {
      if( Secondin ) Secondin = 0;
      else Secondin = 1;
    }
    else if( GetInout(NextFire) == 2 ) {
      if( Firstin ) Firstin = 0;
      else Firstin = 1;
    }
    if( ! Firstin ) {    // origin of 1st fire is not inside
      if( ! Secondin ) crosstype = 1;    // origin of 2nd fire is not inside
      else crosstype = 2;                // 2nd only
    }
    else {
      if( ! Secondin ) crosstype = 3;    // 1st only
      else crosstype = 4;                // both origins within overlap
    }

    switch( crosstype ) {
      case 1:
        xwrite = forewrite;    // if both origins NOT within overlap
        freadstart = 0;
        freadend = xsect1;
        fxend = numchek - 1;
        readnum = *CurrentFire;
        MergeWrite(fxend, freadstart, freadend, &xwrite);
        freadstart = xsect2 + 1;
        freadend = GetNumPoints(NextFire) - 1;
        fxend = -1;
        readnum = NextFire;
        MergeWrite(fxend, freadstart, freadend, &xwrite);
        freadstart = 0;
        freadend = xsect4;
        fxend = numchek;
        readnum = NextFire;
        MergeWrite(fxend, freadstart, freadend, &xwrite);
        freadstart = xsect3 + 1;
        freadend = GetNumPoints(*CurrentFire) - 1;
        fxend = -1;
        readnum = *CurrentFire;
        MergeWrite(fxend, freadstart, freadend, &xwrite);
        forewrite = xwrite;
        break;

      case 2:
        xwrite = forewrite = 0;    // if only origin on fire2 is within overlap
        freadstart = 0;
        freadend = xsect1;
        fxend = numchek - 1;
        readnum = *CurrentFire;
        MergeWrite(fxend, freadstart, freadend, &xwrite);
        freadstart = xsect2 + 1;
        freadend = xsect4;
        fxend = numchek;
        readnum = NextFire;
        MergeWrite(fxend, freadstart, freadend, &xwrite);
        freadstart = xsect3 + 1;
        freadend = GetNumPoints(*CurrentFire) - 1;
        fxend = -1;
        readnum = *CurrentFire;
        MergeWrite(fxend, freadstart, freadend, &xwrite);
        forewrite = xwrite;
        break;

      case 3:
        xwrite = forewrite;    // if only origin on fire1 is within overlap
        freadstart = xsect1 + 1;
        freadend = xsect3;
        fxend = numchek;
        readnum = *CurrentFire;
        MergeWrite(fxend, freadstart, freadend, &xwrite);
        freadstart = xsect4 + 1;
        freadend = GetNumPoints(NextFire) - 1;
        fxend = -1;
        readnum = NextFire;
        MergeWrite(fxend, freadstart, freadend, &xwrite);
        freadstart = 0;
        freadend = xsect2;
        fxend = numchek - 1;
        readnum = NextFire;
        MergeWrite(fxend, freadstart, freadend, &xwrite);
        forewrite = xwrite;
        break;

      case 4:
        xwrite = forewrite;    // if both fire origins within overlap
        freadstart = xsect1 + 1;
        freadend = xsect3;
        fxend = numchek;
        readnum = *CurrentFire;
        MergeWrite(fxend, freadstart, freadend, &xwrite);
        freadstart = xsect4 + 1;
        freadend = xsect2;  				   
        fxend = numchek - 1;
        readnum = NextFire;
        MergeWrite(fxend, freadstart, freadend, &xwrite);
        forewrite = xwrite;
        break;
    }

    /*20200402 JWB:
      Added this check for if there are fire rings.
      Not sure if this is correct, but the code will seg-fault
      if MergeFireRings() is called when there are no fire rings.
    */
    if( GetNumRings() ) { //20200402 Check added
      if( CheckPostFrontal(GETVAL) ) {
        PostFires = new long[2];
        PostFires[0] = *CurrentFire;
        PostFires[1] = NextFire;
        post.MergeFireRings( PostFires, 2, intersect, interpoint, numcross,
                             forewrite );
        delete[] PostFires;
      }
    }                     //20200402 Check added

    OldNumPoints = GetNumPoints(*CurrentFire);	// store old number of points for rediscretize
    SetNumPoints(*CurrentFire, forewrite);
    if( GetInout(*CurrentFire) == 1 && GetInout(NextFire) == 1 )
      SetInout(*CurrentFire, 1);
    else
      SetInout(*CurrentFire, 2);
    SetInout(NextFire, 0);
    IncSkipFires(1);
    SetNumPoints(NextFire, 0);
    if( GetNumAttacks() > 0 )
      SetNewFireNumberForAttack(NextFire, *CurrentFire);
    if( GetNumAirAttacks() > 0 )
      SetNewFireNumberForAirAttack(NextFire, *CurrentFire);

    NextFire = *CurrentFire;
    rediscretize(CurrentFire, true);
    if( CheckPostFrontal(GETVAL) && NextFire != *CurrentFire )
      SetNewFireNumber( NextFire, *CurrentFire,
                        post.AccessReferenceRingNum(1, GETVAL) );
  }
  else {   // merging 2 fires with internal loops formed
    long SpanStart, SpanEnd, SpanNext, trychek, Target;
    double XStart, YStart, XTest, YTest, XTarget, YTarget, NXTarget,
           NYTarget;
    double DistToNext = 0, DistToLast = 0, NDistToLast = 0, diff3 = 0,
           diff4 = 0, area;
    long readfire = 0, inside = 0, outward = 0, ctfire1 = 0, ctfire2 = 0;
    long endloop = -1;
    long P2NumAlloc = GetNumPoints(*CurrentFire) + GetNumPoints(NextFire) +
                      numcross;
    long SwapNumAlloc = GetNumPoints(*CurrentFire) + GetNumPoints(NextFire);
    long i, NewFires1, NewFires2;

    NewFires1 = GetNewFires();    // bookkeeping for postfrontal stuff
    AllocCrossout(numcross);      // array for holding status of crosspoints 1=used
    //FreePerimeter2();           // must GlobalFree perim2 because tranz not called
    AllocPerimeter2(P2NumAlloc);  // allocate enough memory for combined number of points
    AllocSwap(SwapNumAlloc);      // allocate enough memory for combined number of points
    startx = GetPerimeter1Value(*CurrentFire, 0, XCOORD);
    starty = GetPerimeter1Value(*CurrentFire, 0, YCOORD);
    inside = Overlap(NextFire);
    ctfire2 = *CurrentFire;

    if( GetInout(NextFire) == 2 ) {
      if( inside ) inside = 0;
      else inside = 1;
    }
    if( ! inside ) {    // if first point on fire1 is within 2nd fire
      SpanEnd = GetSpan(numchek, 0);
      freadstart = 0; freadend = SpanEnd; fxend = numchek; ctfire1 = *CurrentFire;  // fxend=numchek;
      readnum = *CurrentFire; writenum = ctfire2;
      MergeWrite(fxend, freadstart, freadend, &xwrite);
      SetCrossout(numchek, 1);
      chekct++;
    }
    else {              // first point not within 2nd fire
      readfire = 1;
      endloop = 0;
    }

    while( chekct < numcross ) {
      do {
        readfire = abs(readfire - 1);
        if( readfire )
          ctfire1 = NextFire;    // SWITCH FIRE PERIMETER TO READ FROM
        else
          ctfire1 = *CurrentFire;
        if( readfire && fxend == -1 )    // IF READING FIRE 2 FROM ORIGIN OF ARRAY
          SpanStart = -1;
        else {
          SpanStart = GetSpan(numchek, readfire);
          GetInterPointCoord(numchek, &XStart, &YStart);
          if( SpanStart < GetNumPoints(ctfire1) - 1 )
            Target = SpanStart + 1;    // Target is the next point on fire
          else
            Target = 0;
          XTarget = GetPerimeter1Value(ctfire1, Target, XCOORD);
          YTarget = GetPerimeter1Value(ctfire1, Target, YCOORD);
          diff1 = pow2(XStart - XTarget);
          diff2 = pow2(YStart - YTarget);
          DistToNext = DistToLast = sqrt(diff1 + diff2);
        }

        SpanEnd = 2500000;
        trychek = 0;
        do {
          if( trychek != numchek ) {
            if( GetCrossout(trychek) == 0 ) {    // IF NEXT INTERSECT HASN'T ALREADY BEEN USED
              SpanNext = GetSpan(trychek, readfire);
              //if(Target==0 && SpanNext==0)  // IF READING FROM LAST POINT IN ARRAY AND CROSS IS AT ZERO
              //  SpanStart=-1;
              if( SpanNext >= SpanStart && SpanNext <= SpanEnd ) {
                if( SpanNext == SpanStart ) {    // IF NEXT INTERSECT IS >= TO CURRENT INTERSECT
                  GetInterPointCoord(trychek, &XTest, &YTest);
                  diff1 = pow2(XTest - XTarget);
                  diff2 = pow2(YTest - YTarget);
                  diff1 = sqrt(diff1 + diff2);
                  diff3 = pow2(XTest - XStart);
                  diff4 = pow2(YTest - YStart);
                  diff2 = sqrt(diff3 + diff4);
                  if( diff1 <= DistToNext && diff2 <= DistToLast ) {
                    SpanEnd = SpanNext;
                    numchek = trychek;
                    DistToLast = diff2;
                  }
                }
                else if( SpanNext == SpanEnd ) {    // && NDistToLast!=-1
                  GetInterPointCoord( trychek, &XTest, &YTest );
                  NXTarget = GetPerimeter1Value( ctfire1, SpanNext, XCOORD );
                  NYTarget = GetPerimeter1Value( ctfire1, SpanNext, YCOORD );
                  diff1 = pow2( XTest - NXTarget );
                  diff2 = pow2( YTest - NYTarget );
                  diff1 = sqrt( diff1 + diff2 );
                  if( diff1 <= NDistToLast ) {
                    SpanEnd = SpanNext;
                    numchek = trychek;
                    NDistToLast = diff1;
                  }
                }
                else {
                  SpanEnd = SpanNext;
                  numchek = trychek;
                  GetInterPointCoord( trychek, &XTest, &YTest );
                  NXTarget = GetPerimeter1Value( ctfire1, SpanNext, XCOORD );
                  NYTarget = GetPerimeter1Value( ctfire1, SpanNext, YCOORD );
                  diff1 = pow2( XTest - NXTarget );
                  diff2 = pow2( YTest - NYTarget );
                  NDistToLast = sqrt( diff1 + diff2 );
                }
              }
            }
          }
          trychek++;
        } while( trychek < numcross );

        if( SpanEnd == 2500000 ) {    // if no match for above spanend
          SpanEnd = GetNumPoints(ctfire1) - 1;
          fxend = -1;
          readfire = abs(readfire - 1);    // DON'T SWITCH FIRES YET
        }
        else
          fxend = numchek;

        freadstart = SpanStart + 1;
        freadend = SpanEnd;
        readnum = ctfire1;
        writenum = ctfire2;

        if( writenum == *CurrentFire ) {
          if( xwrite + freadend - freadstart < P2NumAlloc )
            MergeWrite(fxend, freadstart, freadend, &xwrite);
          else {
            /*FindOuterFirePerimeter(*CurrentFire);
              NumPts=GetNumPoints(*CurrentFire);
              Xlo=GetPerimeter1Value(*CurrentFire, NumPts, 0);    // use OldNumPoints from perim1[count]
              Xhi=GetPerimeter1Value(*CurrentFire, NumPts, 1);
              Ylo=GetPerimeter1Value(*CurrentFire, NumPts, 2);
              Yhi=GetPerimeter1Value(*CurrentFire, NumPts, 3);
              SetPerimeter2(NumPts, Xlo, Xhi, Ylo, Yhi);
              tranz(*CurrentFire, NumPts+1);
              SetNumPoints(*CurrentFire, NumPts);
              FindOuterFirePerimeter(NextFire);
              NumPts=GetNumPoints(NextFire);
              Xlo=GetPerimeter1Value(NextFire, NumPts, 0);      // use OldNumPoints from perim1[count]
              Xhi=GetPerimeter1Value(NextFire, NumPts, 1);
              Ylo=GetPerimeter1Value(NextFire, NumPts, 2);
              Yhi=GetPerimeter1Value(NextFire, NumPts, 3);
              SetPerimeter2(NumPts, Xlo, Xhi, Ylo, Yhi);
              tranz(*CurrentFire, NumPts+1);
              SetNumPoints(*CurrentFire, NumPts);
            */
            numchek = endloop;
            chekct = numcross;
            outward = 1;
            xwrite = 0;
            fxend = -1;
            newnump = 0;
            //tranz(*CurrentFire, 0);   // must transfer to perim2 for rediscretize
          }     // won't rediscretize if newnump==0
        }
        else {
          if( xwrite + freadend - freadstart < SwapNumAlloc )
            MergeWrite(fxend, freadstart, freadend, &xwrite);
          else {
            numchek = endloop;
            if( ! outward ) newnump = 0;
            outward = 1;
            chekct = numcross;
            xwrite = 0;
            fxend = -1;
          }
        }

        if( GetCrossout(numchek) == 0 ) {
          SetCrossout(numchek, 1);
          chekct++;
        }
        if( fxend == -1 && readfire )    // different breakout criteria for outside first fire
          break;
      } while( numchek != endloop );

      if( outward == 0 ) {    // if outward fire has not been found
        if( xwrite > 0 )
          area = arp(2, xwrite);
        else area = 0;

        if( area > 0.0 ) {
          outward = 1;       // outside fire has been identified, there can be only 1 outward fire
          newnump = xwrite;  // after merger
          ctfire2 = GetNewFires();
        }
        else if( area < 0.0 ) {     // this was an inward fire, but
          if( xwrite > 2 ) {        // don't write very small "irrelevant" enclaves
            if( GetInout(*CurrentFire) == 2 || GetInout(NextFire) == 2 ) {
              outward = 1;         // outside fire has been identified, there can be only 1 outward fire
              newnump = xwrite;    // after merger
              ctfire2 = GetNewFires();
              SetInout(*CurrentFire, 2);
            }
            else {
              AllocPerimeter1(GetNewFires(), xwrite + 1);
              tranz(GetNewFires(), xwrite);    // transfer points from perim2 to newfire array
              BoundaryBox(xwrite);
              SetNumPoints(GetNewFires(), xwrite);    // because this is an inward burning fire
              SetInout(GetNewFires(), 2);
              IncNewFires(1);
            }
          }
        }
      }
      else if( xwrite > 2 ) {    // don't write very small "irrelevant" enclaves
        AllocPerimeter1(GetNewFires(), xwrite + 1);
        SwapTranz(GetNewFires(), xwrite);
        BoundaryBox(xwrite);
        SetNumPoints(GetNewFires(), xwrite);
        SetInout(GetNewFires(), 2);
        IncNewFires(1);
        ctfire2 = GetNewFires();
      }

      if( chekct < numcross ) {
        numchek = -1;
        xwrite = 0;
        do {
          inside = GetCrossout(++numchek);    // find next loop, inward or outward
        } while( inside );
        endloop = numchek;
        inside = 1;    // reset inside for all but possibly first fire
        readfire = 1;
      }
    }

    i = 0;
    if( CheckPostFrontal(GETVAL) ) {
      NewFires2 = GetNewFires();
      if( (PostFires = new long[(NewFires2 - NewFires1 + 2)]) != NULL ) {
        /*20200402 JWB:
          Added this check for if there are fire rings.
          Not sure if this is correct, but the code will seg-fault
          if MergeFireRings() is called when there are no fire rings.
        */
        if( GetNumRings() ) { //20200402 Check added
          PostFires[0] = *CurrentFire;
          PostFires[1] = NextFire;
          for( i = 2; i < (NewFires2 - NewFires1 + 2); i++ )
            PostFires[i] = NewFires1 + i - 2;
          post.MergeFireRings( PostFires, NewFires2 - NewFires1 + 2,
                               intersect, interpoint, numcross, newnump );
          delete[] PostFires;//GlobalFree(PostFires);
        }                    //20200402 Check added
      }
    }

    OldNumPoints = GetNumPoints(*CurrentFire);    // store old number of points for rediscretize
    if( newnump > P2NumAlloc )
      newnump = 0;
    else if( newnump > 0 )
      SetNumPoints(*CurrentFire, newnump);
    SetNumPoints(NextFire, 0);
    SetInout(NextFire, 0);
    IncSkipFires(1);

    if( GetNumAttacks() > 0 )
      SetNewFireNumberForAttack(NextFire, *CurrentFire);
    if( GetNumAirAttacks() > 0 )
      SetNewFireNumberForAirAttack(NextFire, *CurrentFire);

    NextFire = *CurrentFire;
    if( newnump > 0 )
      rediscretize(CurrentFire, true);    // only rediscretize outer fire
    if( CheckPostFrontal(GETVAL) && NextFire != *CurrentFire )
      SetNewFireNumber( NextFire, *CurrentFire,
                        post.AccessReferenceRingNum(1, GETVAL) );
  }

  return true;
} //Intersections::MergeFire


void Intersections::BoundaryBox(long NumPoints)
{
	// determines bounding box for new fire
	double xpt, ypt, Xlo, Xhi, Ylo, Yhi;

	Xlo = Xhi = GetPerimeter1Value(GetNewFires(), 0, XCOORD);
	Ylo = Yhi = GetPerimeter1Value(GetNewFires(), 0, YCOORD);
	for (int i = 1; i < NumPoints; i++)
	{
		xpt = GetPerimeter1Value(GetNewFires(), i, XCOORD);
		ypt = GetPerimeter1Value(GetNewFires(), i, YCOORD);
		if (xpt < Xlo)
			Xlo = xpt;
		else
		{
			if (xpt > Xhi)
				Xhi = xpt;
		}
		if (ypt < Ylo)
			Ylo = ypt;
		else
		{
			if (ypt > Yhi)
				Yhi = ypt;
		}
	}
	SetPerimeter1(GetNewFires(), NumPoints, Xlo, Xhi);
	SetFireChx(GetNewFires(), NumPoints, Ylo, Yhi);
}


void Intersections::AllocCrossout(long Number)
{
	if (Number)
	{
		long i;

		if (Number >= crossnumalloc)
		{
			FreeCrossout();
			nmemb = Number;
			//if((crossout=(long *) GlobalAlloc(GMEM_FIXED, nmemb*sizeof(long)))!=NULL)
			if ((crossout = new long[nmemb]) != NULL)
			{
				for (i = 0; i < Number; i++)
					crossout[i] = 0;
				crossnumalloc = Number;
			}
			else
				crossout = 0;
		}
		else
		{
			for (i = 0; i < crossnumalloc; i++)
				crossout[i] = 0;
		}
	}
}

void Intersections::FreeCrossout()
{
	if (crossout)
		delete[] crossout;//GlobalFree(crossout);
	crossout = 0;
	crossnumalloc = 0;
}

long Intersections::GetCrossout(long Number)
{
	return crossout[Number];
}

void Intersections::SetCrossout(long Number, long Value)
{
	crossout[Number] = Value;
}


long Intersections::GetSpan(long Number, long ReadFire)
{
	long SpanBlank, Span;

	if (ReadFire)
		GetIntersection(Number, &SpanBlank, &Span);
	else
		GetIntersection(Number, &Span, &SpanBlank);
	SpanBlank = 0;

	return Span;
}

/*============================================================================
  Intersections::arp
  Calculates fire area for determining orientation of fire perimeter
  if positive, then outward burning, if negative inward burning.
*/
double Intersections::arp( int PerimNum, long count )
{ //Intersections::arp
  long count1, count2 = 0, NumPoints = 0;
  double xpt1, ypt1, xpt2, ypt2, aangle, zangle;
  double area = 0.0, DiffAngle, newarea;

  switch( PerimNum ) {
    case 1:
      NumPoints = GetNumPoints( count );
      if( ! NumPoints ) break;
      startx = GetPerimeter1Value( count, 0, XCOORD );
      starty = GetPerimeter1Value( count, 0, YCOORD );
      while( count2 < NumPoints ) {
        count2++;
        xpt1 = GetPerimeter1Value( count, count2, XCOORD );
        ypt1 = GetPerimeter1Value( count, count2, YCOORD );
        zangle = direction( xpt1, ypt1 );  // reference angle
        if( zangle != 999.9 ) break;  // make sure that startx,starty!=x[0]y[0]
      }
      break;
    case 2:
      NumPoints = count;
      if( ! NumPoints ) break;
      startx = GetPerimeter2Value( 0, 0 );
      starty = GetPerimeter2Value( 0, 1 );
      while( count2 < NumPoints ) {
        count2++;
        xpt1 = GetPerimeter2Value( count2, 0 );
        ypt1 = GetPerimeter2Value( count2, 1 );
        zangle = direction( xpt1, ypt1 );  // reference angle
        if( zangle != 999.9 )	break;   //make sure that startx,starty!=x[0]y[0]
      }
      break;
  }
  count2++;
  if( NumPoints < 3 ) return area;    // don't work on line fires

  for( count1 = count2; count1 < NumPoints; count1++ ) {
    switch( PerimNum ) {
      case 1:
        xpt2 = GetPerimeter1Value( count, count1, XCOORD );
        ypt2 = GetPerimeter1Value( count, count1, YCOORD );
        break;
      case 2:
        xpt2 = GetPerimeter2Value( count1, 0 );
        ypt2 = GetPerimeter2Value( count1, 1 );
        break;
    }
    newarea = fabs( 0.5 * (startx * ypt1 - xpt1 * starty + xpt1 * ypt2 -
                           xpt2 * ypt1 + xpt2 * starty - startx * ypt2) );
    aangle = direction( xpt2, ypt2 );
    if( aangle != 999.9 ) {
      DiffAngle = aangle - zangle;
      if( DiffAngle > PI ) DiffAngle = -( 2.0 * PI - DiffAngle );
      else if( DiffAngle < -PI ) DiffAngle = ( 2.0 * PI + DiffAngle );
      if( DiffAngle > 0.0 ) area -= newarea;
      else if( DiffAngle < 0.0 ) area += newarea;
      zangle = aangle;
    }
    xpt1 = xpt2;
    ypt1 = ypt2;
  }

  return area;
} //Intersections::arp


void Intersections::GetOffcheck(long* Offcheck)
{
	if (*Offcheck >= numcross)
		*Offcheck -= (numcross * (long) (*Offcheck / numcross));
}

long Intersections::intercode(long offcheck, long xypt)
{
	// translates crosspoint into offset coordinates to avoid 1st element in array
	long xsect;

	xsect = GetSpan(offcheck, xypt);
	if (noffset1 > 0)
	{
		if (xsect < noffset1)
			xsect = xsect + noffset2;
		else
			xsect = xsect - noffset1;
	}
	return xsect;
}



void Intersections::MergeWrite(long xend, long readstart, long readend,
	long* xwrite)
{
	// writes segments of fire perimeter to perimeter2 array[][]
	// or to perimeter 1 array if the fire is a new inward burning fire polygon
	long countx, count1, inc;
	double xpt, ypt, ros1, fli1, xpti, ypti, rcx1;

	if (readstart >= 0)   /* to eliminate twin spike loops */
	{
		inc = *xwrite;
		if (GetInout(writenum) != 0)	/* if existing fire */
		{
			for (countx = readstart;
				countx <= readend;
				countx++)  /* write points between loops */
			{
				if (countx >= noffset2)
					count1 = countx - noffset2;	/* decodes the offset array address */
				else
					count1 = countx + noffset1;
				xpt = GetPerimeter1Value(readnum, count1, XCOORD);
				ypt = GetPerimeter1Value(readnum, count1, YCOORD);
				if (countx == readstart && inc != 0)
				{
					xpti = GetPerimeter2Value(--inc, 0);	// check against previous point
					ypti = GetPerimeter2Value(inc, 1);
					if (xpt == xpti && ypt == ypti)
						*xwrite = inc;
					else
						inc++;
				}
				ros1 = GetPerimeter1Value(readnum, count1, ROSVAL);
				fli1 = GetPerimeter1Value(readnum, count1, FLIVAL);
				rcx1 = GetPerimeter1Value(readnum, count1, RCXVAL);
				SetPerimeter2(*xwrite, xpt, ypt, ros1, fli1, rcx1);
				inc++;
				*xwrite = inc;
			}
			if (xend >= 0)					// unless at end of array, write last crosspoint
			{
				GetInterPointCoord(xend, &xpti, &ypti);
				if (xpti != xpt || ypti != ypt)		// check against intersection point
				{
					GetInterPointFChx(xend, &ros1, &fli1, &rcx1);
					SetPerimeter2(*xwrite, xpti, ypti, ros1, fli1, rcx1);    // must write XPTI & YPTI here not XPT & YPT !!!!!!!!
					inc++;
					*xwrite = inc;
				}
			}
		}
		else	   // new fire front, inward burning
		{
			for (countx = readstart;
				countx <= readend;
				countx++)  // write points between loops
			{
				if (countx >= noffset2)
					count1 = countx - noffset2;	// decodes the offset array address
				else
					count1 = countx + noffset1;
				xpt = GetPerimeter1Value(readnum, count1, XCOORD);
				ypt = GetPerimeter1Value(readnum, count1, YCOORD);
				if (countx == readstart && inc != 0)
				{
					xpti = swapperim[--inc * NUMDATA];	// check against previous point
					ypti = swapperim[inc * NUMDATA + 1];
					if (xpt == xpti && ypt == ypti)
						*xwrite = inc;
					else
						inc++;
				}
				ros1 = GetPerimeter1Value(readnum, count1, ROSVAL);
				fli1 = GetPerimeter1Value(readnum, count1, FLIVAL);
				rcx1 = GetPerimeter1Value(readnum, count1, RCXVAL);
				SetSwap(*xwrite, xpt, ypt, ros1, fli1, rcx1);	// must write XPTI here not XPT!!!!!!!!
				*xwrite = ++inc;
			}
			if (xend >= 0)						// unless at end of array, write last crosspoint
			{
				GetInterPointCoord(xend, &xpti, &ypti);
				if (xpti != xpt || ypti != ypt)		// check against intersection point
				{
					GetInterPointFChx(xend, &ros1, &fli1, &rcx1);
					SetSwap(*xwrite, xpti, ypti, ros1, fli1, rcx1);	// must write XPTI here not XPT!!!!!!!!
					*xwrite = ++inc;
				}
			}
		}
	}
}


//============================================================================
XUtilities::XUtilities()
{ //XUtilities::XUtilities
  swapperim = 0;
  swapnumalloc = 0;
} //XUtilities::XUtilities


//============================================================================
XUtilities::~XUtilities()
{ //XUtilities::XUtilities
  FreeSwap();
} //XUtilities::XUtilities

//============================================================================
void XUtilities::AllocSwap( long NumPoint )
{ //XUtilities::AllocSwap
  if( NumPoint ) {
    if( NumPoint >= swapnumalloc ) {
      FreeSwap();
      nmemb = NUMDATA * NumPoint; // dimension swap array to 5X original
      if( (swapperim = new double[nmemb]) == NULL ) {
        swapperim = 0;
        NumPoint = -1;  // debugging
      }
      swapnumalloc = NumPoint;
    }
  }
} //XUtilities::AllocSwap

//============================================================================
void XUtilities::FreeSwap()
{ //XUtilities::FreeSwap
  if( swapperim ) delete[] swapperim;
  swapnumalloc = 0;
  swapperim = 0;
} //XUtilities::FreeSwap

//============================================================================
void XUtilities::SetSwap( long NumPoint, double xpt, double ypt, double ros,
                          double fli, double rcx )
{ //XUtilities::SetSwap
  if( NumPoint < swapnumalloc && NumPoint >= 0 ) {
    NumPoint *= NUMDATA;
    swapperim[NumPoint] = xpt;
    swapperim[++NumPoint] = ypt;
    swapperim[++NumPoint] = ros;
    swapperim[++NumPoint] = fli;
    swapperim[++NumPoint] = rcx;
  }
	else NumPoint = -1;  //debugging
} //XUtilities::SetSwap

//============================================================================
void XUtilities::SetSwap( long NumPoint, PerimeterPoint &Pt )
{ //XUtilities::SetSwap
  if( NumPoint < swapnumalloc && NumPoint >= 0 )
    Pt.Get( &swapperim[NumPoint*NUMDATA] );
	else NumPoint = -1;  //debugging
} //XUtilities::SetSwap

void XUtilities::GetSwap(long NumPoint, double* xpt, double* ypt, double* ros,
	double* fli, double* rcx)
{
	NumPoint *= NUMDATA;
	*xpt = swapperim[NumPoint];
	*ypt = swapperim[++NumPoint];
	*ros = swapperim[++NumPoint];
	*fli = swapperim[++NumPoint];
	*rcx = swapperim[++NumPoint];
}


void XUtilities::RePositionFire(long* firenum)
{
	long FireCount, FireNum;//, NumPoints;

	FireCount = 0;
	do  			   				  		// checks for merged fires
	{
		FireNum = GetInout(FireCount);  	  		// and overwrites arrays
		if (FireNum == 0)
			break;
		FireCount++;
	}
	while (FireCount <= *firenum);
	if (FireCount < *firenum)
	{
		SwapFirePerims(FireCount, *firenum);
		if (GetNumAttacks() > 0)				   // update fire number associated with attack
			SetNewFireNumberForAttack(*firenum, FireCount);
		if (GetNumAirAttacks() > 0)
			SetNewFireNumberForAirAttack(*firenum, FireCount);
		*firenum = FireCount;
	}
}


void XUtilities::RestoreDeadPoints(long firenum)
{
	long i, nump = GetNumPoints(firenum);
	double xpt1, ypt1, ros1, fli1, rcx1;
	//double xhi, xlo, yhi, ylo;

	//xhi=xlo=GetPerimeter1Value(firenum, 0, XCOORD);
	//yhi=ylo=GetPerimeter1Value(firenum, 0, YCOORD);
	for (i = 0; i < nump; i++)
	{
		fli1 = GetPerimeter2Value(i, FLIVAL);
		if (fli1 < 0.0)
		{
			GetPerimeter2(i, &xpt1, &ypt1, &ros1, &fli1, &rcx1);
			SetPerimeter1(firenum, i, xpt1, ypt1);
			SetFireChx(firenum, i, ros1, fli1);
			SetReact(firenum, i, rcx1);
		}
		else
		{
			xpt1 = GetPerimeter1Value(firenum, i, XCOORD);
			ypt1 = GetPerimeter1Value(firenum, i, YCOORD);
		}
		//if(xpt1<xlo)
		//	xlo=xpt1;
		//if(xpt1>xhi)  	  // cant do "else if" because of vertical lines
		//	xhi=xpt1;
		//if(ypt1<ylo)
		//	ylo=ypt1;
		//if(ypt1>yhi)  	  // cant do "else if" becuause of horizontal lines
		//	yhi=ypt1;
	}
	//SetPerimeter1(firenum, nump, xlo, xhi);    // reset bounding box
	//SetFireChx(firenum, nump, ylo, yhi);
}

/*============================================================================
  XUtilities::rediscretize
  Adds midpoint between vertices exceeding maximum resolution.
*/
void XUtilities::rediscretize( long* firenum, bool Reorder )
{ //XUtilities::rediscretize
  CallLevel++;
  if( Verbose >= CallLevel )
    printf( "%*sfsxwutil:XUtilities::rediscretize:1 numpts[%d]=%ld\n",
            CallLevel, "", 0, GetNumPoints(0) );

  long   i, firetype, firet, nump, newnump, count = 0, count1, count2 = 0;
  double xpt = 0, ypt = 0, xptn = 0, yptn = 0, newx, newy, ros1 = 0,
         ros2 = 0, avgros, avgrcx;
  double xdiff, ydiff, xyhyp = 0.01, fli1 = 0, fli2 = 0, avgfli, rcx1, rcx2;
  double MaxDistSquared = pow2( GetPerimRes() * MetricResolutionConvert() );
  if( Verbose >= CallLevel )
    printf( "%*sfsxwutil:XUtilities::rediscretize:2 MaxDistSquared=%lf\n",
            CallLevel, "", MaxDistSquared );

  firetype = GetInout( *firenum );
  //INIIALIZE EXTINGUISHED NUMBER OF POINTS.
  ExNumPts = newnump = nump = GetNumPoints( *firenum );

  //Search for extinguished fires in array sequence.
  do {  //Checks for merged fires
    firet = GetInout( count );  //And overwrites arrays
    if( firet == 0 ) break;
    count++;
  } while( count <= *firenum );

  if( count > *firenum ) count = *firenum;
  if( Reorder == false ) {
    if( GetInout(*firenum) == 2 ) {  //Inward burning
      if( *firenum >= GetNumFires() && count < GetNumFires() )
        count = *firenum;
    }
  }
  if( Verbose >= CallLevel )
    printf( "%*sfsxwutil:XUtilities::rediscretize:3 numpts[%d]=%ld\n",
            CallLevel, "", 0, GetNumPoints(0) );

  double Xlo, Xhi, Ylo, Yhi;
  bool   FirstTime = true;
  newnump *= 4;     //Dimension swap array to 4X original
  AllocSwap( newnump );

  //If internal fire && smaller than 10 points.
  if( firetype == 2 && nump < 3 ) {
    firetype = 0;  //Then eliminate the fire
    count2 = 0;
    //XUtilities.ExNumPts, stores extinguished array size for
    //dist meth check 2.
    ExNumPts = nump;
    IncSkipFires( 1 );
  }
  else {
    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:XUtilities::rediscretize:4a "
              "count=%ld count2=%ld\n",
              CallLevel, "", count, count2 );

    for( count1 = 0; count1 <= nump; count1++ ) {
      if( count1 < nump ) {
        if( FirstTime ) {
          GetPerimeter2( count1, &xpt, &ypt, &ros1, &fli1, &rcx1 );
          if( xpt <= GetLoEast() ) continue;
          if( xpt >= GetHiEast() ) continue;
          if( ypt <= GetLoNorth() ) continue;
          if( ypt >= GetHiNorth() ) continue;
          Xlo = Xhi = xpt;
          Ylo = Yhi = ypt;
          FirstTime = false;
        }
        GetPerimeter2( count1, &xptn, &yptn, &ros2, &fli2, &rcx2 );
        //Added to check for zero points 5/31/1995.
        if( xptn <= GetLoEast() ) continue;
        if( xptn >= GetHiEast() ) continue;
        if( yptn <= GetLoNorth() ) continue;
        if( yptn >= GetHiNorth() ) continue;
        if( xptn < Xlo ) Xlo = xptn;  //Determine bounding boxes
        else if( xptn > Xhi ) Xhi = xptn;
        if( yptn < Ylo ) Ylo = yptn;
        else if( yptn > Yhi ) Yhi = yptn;
      }
      else GetSwap( 0, &xptn, &yptn, &ros2, &fli2, &rcx2 );

      if( firetype < 3 ) {  //Don't check resolution of barriers
        xdiff = xpt - xptn;
        ydiff = ypt - yptn;
        xyhyp = pow2( xdiff ) + pow2( ydiff );  //Hypotenuse distance b'n pts
      }
      if( xyhyp >= MaxDistSquared ) {  //Adds points to line segment
        newx = xpt - xdiff / 2.0;  //Adds points to line segment
        newy = ypt - ydiff / 2.0;
        SetSwap( count2, xpt, ypt, ros1, fli1, rcx1 );
        count2++;
        avgros = ( ros1 + ros2 ) / 2.0;
        avgfli = ( fabs(fli1) + fabs(fli2) ) / 2.0;
        if( fli1 < 0.0 && fli2 < 0.0 ) avgfli *= -1.0;
        else if( fli1 <= 0.0 || fli2 <= 0.0 ) {
          startx = newx;
          starty = newy;
          for( i = 0; i < GetNumFires(); i++ ) {
            if( GetInout(i) < 3 ) continue;
            if( Overlap(i) ) {
              avgfli *= -1.0;
              if( avgfli == 0.0 ) avgfli = -1.0;
            }
          }
        }
        avgrcx = ( rcx1 + rcx2 ) / 2.0;
        SetSwap( count2, newx, newy, avgros, avgfli, avgrcx );
        count2++;
      }
      else if( xyhyp > 1e-12 ) {
        SetSwap( count2, xpt, ypt, ros1, fli1, rcx1 );
        count2++;
      }
      xpt = xptn;
      ypt = yptn;
      ros1 = ros2;
      fli1 = fli2;
      rcx1 = rcx2;
    }      //COORDINATES AS LAST POINT

    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:XUtilities::rediscretize:4b "
              "count=%ld count2=%ld\n",
              CallLevel, "", count, count2 );
  }
  if( Verbose >= CallLevel )
    printf( "%*sfsxwutil:XUtilities::rediscretize:5 numpts[%d]=%ld\n",
            CallLevel, "", 0, GetNumPoints(0) );

  //If external fire && smaller than 4 points, can happen with precis. loss.
  if( firetype == 1 && count2 < 3 ) {
    //XXUtilities.ExNumPts, stores extinguished array size for
    //dist meth check 2.
    ExNumPts = nump;
    firetype = 0;  //Then eliminate the fire
    count2 = 0;
    IncSkipFires( 1 );
  }
  if( Verbose >= CallLevel )
    printf( "%*sfsxwutil:XUtilities::rediscretize:6 "
            "numpts[%d]=%ld count=%ld count2=%ld\n",
            CallLevel, "", 0, GetNumPoints(0), count, count2 );

  SetNumPoints( count, count2 );
  if( Verbose >= CallLevel )
    printf( "%*sfsxwutil:XUtilities::rediscretize:7 numpts[%d]=%ld\n",
            CallLevel, "", 0, GetNumPoints(0) );

  SetInout( count, firetype );
  if( count2 != 0 ) {
    SetSwap( count2, Xlo, Xhi, Ylo, Yhi, 0.0 );
    count2++;
    AllocPerimeter1( count, count2 );
    SwapTranz( count, count2 );
  }
  if( *firenum != count ) {  //If write points to new array
    if( GetNumAttacks() > 0 )
      SetNewFireNumberForAttack( *firenum, count );
    if( GetNumAirAttacks() > 0 )
      SetNewFireNumberForAirAttack( *firenum, count );
    SetInout( *firenum, 0 );      //Reset fire direction
    SetNumPoints( *firenum, 0 );  //Resent number of points in fire
    *firenum = count;             //Update new fire number
  }

  if( Verbose >= CallLevel )
    printf( "%*sfsxwutil:XUtilities::rediscretize:8 numpts[%d]=%ld\n",
            CallLevel, "", 0, GetNumPoints(0) );
  CallLevel--;
} //XUtilities::rediscretize


void XUtilities::SwapTranz(long writefire, long nump)
{
	// swap transfer file contents to perimeter1 array[][][]
	double xpt, ypt, ros, fli, rcx;

	for (long count1 = 0; count1 < nump; count1++)
	{
		GetSwap(count1, &xpt, &ypt, &ros, &fli, &rcx);
		SetPerimeter1(writefire, count1, xpt, ypt);
		SetFireChx(writefire, count1, ros, fli);
		SetReact(writefire, count1, rcx);
	}
}

/*============================================================================
  XUtilities::tranz
  Transfers points between arrays
*/
void XUtilities::tranz( long count, long nump )
{ //XUtilities::tranz
  CallLevel++;
  if( Verbose >= CallLevel )
    printf( "%*sfsxwutil:XUtilities::tranz:1\n",
            CallLevel, "" );

  long   ct1;
  double xpt = 0, ypt = 0, ros = 0, fli = 0, rcx;

  if( nump == 0 ) {
    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:XUtilities::tranz:1a\n",
              CallLevel, "" );

    nump = OldNumPoints = GetNumPoints( count );
    if( nump > 0 ) {
      if( ! SwapFirePerims(-1, count) ) {  //Backup method
        AllocPerimeter2( nump );
        for( ct1 = 0; ct1 < nump; ct1++ ) {
          //xpt = GetPerimeter1Value( count, ct1, XCOORD );
          //ypt = GetPerimeter1Value( count, ct1, YCOORD );
          //ros = GetPerimeter1Value( count, ct1, ROSVAL );
          //fli = GetPerimeter1Value( count, ct1, FLIVAL );
          //rcx = GetPerimeter1Value( count, ct1, RCXVAL );
          //SetPerimeter2( ct1, xpt, ypt, ros, fli, rcx );
          PerimeterPoint Pt;
          GetPerimeter1Point( count, ct1, &Pt );
          SetPerimeter2( ct1, Pt );
        }
      }
    }
    else nump = -1;  //For debugging

    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:XUtilities::tranz:1b\n",
              CallLevel, "" );
  }
  else {
    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:XUtilities::tranz:1c count=%ld nump=%ld\n",
              CallLevel, "", count, nump );

    if( ! SwapFirePerims(count, -nump) ) {

      if( Verbose >= CallLevel )
        printf( "%*sfsxwutil:XUtilities::tranz:1c1\n",
                CallLevel, "" );

      for( ct1 = 0; ct1 < nump; ct1++ ) {  //Backup method
        GetPerimeter2( ct1, &xpt, &ypt, &ros, &fli, &rcx );
        SetPerimeter1( count, ct1, xpt, ypt );
        SetFireChx( count, ct1, ros, fli );
        SetReact( count, ct1, rcx );
      }
    }

    if( Verbose >= CallLevel )
      printf( "%*sfsxwutil:XUtilities::tranz:1d\n",
              CallLevel, "" );
  }

  if( Verbose >= CallLevel )
    printf( "%*sfsxwutil:XUtilities::tranz:2\n",
            CallLevel, "" );
  CallLevel--;
} //XUtilities::tranz
