#include "stdafx.h"
#include "measuring.h"

#include "..\Processing\GeograpicLib\PolygonArea.hpp"
//const GeographicLib::Geodesic& geod = GeographicLib::Geodesic::WGS84;
GeographicLib::Geodesic geod(GeographicLib::Constants::WGS84_a(), GeographicLib::Constants::WGS84_f());
GeographicLib::PolygonArea poly(geod);

// *******************************************************
//		get_Length()
// *******************************************************
STDMETHODIMP CMeasuring::get_IsStopped(VARIANT_BOOL* retVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	*retVal = this->IsStopped() ? VARIANT_TRUE : VARIANT_FALSE;
	return S_OK;
}

// *******************************************************
//		UndoPoint()
// *******************************************************
STDMETHODIMP CMeasuring::get_PointCount(long* retVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	*retVal = points.size();
	return S_OK;
}

// *******************************************************
//		get_PointXY()
// *******************************************************
STDMETHODIMP CMeasuring::get_PointXY(long pointIndex, double* x, double* y, VARIANT_BOOL* retVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (pointIndex < 0 || pointIndex >= points.size())
	{
		// TODO: report error
		*retVal = VARIANT_FALSE;
	}
	else
	{
		*x = points[pointIndex]->Proj.x;
		*y = points[pointIndex]->Proj.y;
		*retVal = VARIANT_TRUE;
	}
	return S_OK;
}

// *******************************************************
//		MeasuringType()
// *******************************************************
STDMETHODIMP CMeasuring::get_MeasuringType(tkMeasuringType* retVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	*retVal = this->measuringType;
	return S_OK;
}
STDMETHODIMP CMeasuring::put_MeasuringType(tkMeasuringType newVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
	this->measuringType = newVal;
	return S_OK;
}

// *******************************************************
//		get_Length()
// *******************************************************
STDMETHODIMP CMeasuring::get_Length(double* retVal) 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	*retVal = this->GetDistance();
	return S_OK;
}

// *******************************************************
//		get_AreaWithClosingVertex()
// *******************************************************
STDMETHODIMP CMeasuring::get_AreaWithClosingVertex(double lastPointProjX, double lastPointProjY, double* retVal) 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	*retVal = this->GetArea(true, lastPointProjX, lastPointProjY);
	return S_OK;
}

// *******************************************************
//		get_Area()
// *******************************************************
STDMETHODIMP CMeasuring::get_Area(double* retVal) 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	*retVal = this->GetArea(false, 0.0, 0.0);
	return S_OK;
}

// *******************************************************
//		UndoPoint()
// *******************************************************
STDMETHODIMP CMeasuring::UndoPoint(VARIANT_BOOL* retVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	*retVal = VARIANT_FALSE;
	if (points.size() > 0) {
		delete points[points.size() - 1];
		points.pop_back();
		*retVal = VARIANT_TRUE;
		areaRecalcIsNeeded = true;
	}
	stopped = false;
	closedPoly = false;
	return S_OK;
}

// *******************************************************
//		Stop()
// *******************************************************
STDMETHODIMP CMeasuring::FinishMeasuring()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	this->stopped = true;
	return S_OK;
}

// *******************************************************
//		Clear()
// *******************************************************
STDMETHODIMP CMeasuring::Clear()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	for(size_t i = 0; i < points.size(); i++)
		delete points[i];
	this->points.clear();
	poly.Clear();
	return S_OK;
}

STDMETHODIMP CMeasuring::get_SegementLength(int segmentIndex, double* retVal) 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (segmentIndex < 0 || segmentIndex >= points.size() - 1)
	{
		// TODO: report error
	}
	else
	{
		int i = segmentIndex;
		if (transformationMode == tmNotDefined)
		{
			*retVal = points[i]->Proj.GetDistance(points[i + 1]->Proj);
		}
		else
		{
			IUtils* utils = GetUtils();
			utils->GeodesicDistance(points[i]->y, points[i]->x, points[i + 1]->y, points[i + 1]->x, retVal);
		}
	}
	return S_OK;
}

#pragma region Distance calculation
// *******************************************************
//		getDistance()
// *******************************************************
double CMeasuring::GetDistance() 
{
	if (transformationMode == tmNotDefined)
	{
		return GetEuclidianDistance();	// if there us undefined or incompatible projection; return distance on plane 
	}
	else
	{
		return GetGeodesicDistance();
	}
}

// *******************************************************
//		GetEuclidianDistance()
// *******************************************************
// in map units specified by current projection
double CMeasuring::GetEuclidianDistance()
{
	double dist = 0.0;
	if (points.size() > 0) 
	{
		for (size_t i = 0; i < points.size() - 1; i++)
		{
			dist += points[i]->Proj.GetDistance(points[i + 1]->Proj);
		}
	}
	return dist;
}

// *******************************************************
//		GetGeodesicDistance()
// *******************************************************
// in meters with decimal degrees as input
double CMeasuring::GetGeodesicDistance() 
{
	IUtils* utils = GetUtils();
	double dist = 0.0;
	if (points.size() > 0) 
	{
		for (size_t i = 0; i < points.size() - 1; i++)
		{
			double val;
			utils->GeodesicDistance(points[i]->y, points[i]->x, points[i + 1]->y, points[i + 1]->x, &val);
			dist += val;
		}
	}
	return dist;
}
#pragma endregion

#pragma region Calculate area

// *******************************************************
//		GetArea()
// *******************************************************
double CMeasuring::GetArea(bool closingPoint, double x, double y)
{
	if (transformationMode == tmNotDefined)
	{
		return GetEuclidianArea(closingPoint, x, y);   // x, y are projected
	}
	else
	{
		double xDeg = x, yDeg = y;
		TransformPoint(xDeg, yDeg);
		return GetGeodesicArea(closingPoint, xDeg, yDeg);	  // x, y are decimal degrees
	}
}

// *******************************************************
//		GetGeodesicArea()
// *******************************************************
// coordinates in decimal degress are expected
double CMeasuring::GetGeodesicArea(bool closingPoint, double x, double y)
{
	unsigned int result;
	double perimeter = 0.0, area = 0.0;
	if (points.size() > 1)
	{
		if (areaRecalcIsNeeded)
		{
			areaRecalcIsNeeded = false;
			poly.Clear();
			for(int i = 0; i < points.size(); i++)
			{
				poly.AddPoint(points[i]->y, points[i]->x);
			}
			result = poly.Compute(true, true, perimeter, area);
		}
		
		if (closingPoint)
		{
			//poly.AddPoint(y, x);
			result = poly.TestPoint(y, x, true, true, perimeter, area);
		}
		else
		{
			result = poly.Compute(true, true, perimeter, area);
		}
	}
	return area;
}

// *******************************************************
//		GetEuclidianArea()
// *******************************************************
double CMeasuring::GetEuclidianArea(bool closingPoint, double x, double y) 
{
	double val = 0.0;
	if (points.size() > 1)
	{
		VARIANT_BOOL vb;
		shape->Create(ShpfileType::SHP_POLYGON, &vb);		// this will clear points

		long pointIndex = -1;
		
		for (size_t i = 0; i < points.size(); i++)
		{
			shape->AddPoint(points[i]->Proj.x, points[i]->Proj.y, &pointIndex);
		}
		
		if (closingPoint)
			shape->AddPoint(x, y, &pointIndex);

		shape->AddPoint(points[0]->Proj.x, points[0]->Proj.y, &pointIndex);   // we need to close the poly
		shape->get_Area(&val);
	}
	return val;
}
#pragma endregion

// *******************************************************
//		SetProjection()
// *******************************************************
bool CMeasuring::SetProjection(IGeoProjection* projNew, IGeoProjection* projWGS84New, tkTransformationMode mode)
{
	if (projWGS84)
		projWGS84->Release();
	this->projWGS84 = projWGS84New;
	projWGS84->AddRef();
	
	// we need to clone a map projection, so that nobody will stop out transformation
	if (proj)
	{
		if (transformationMode == tmDoTransformation)
			proj->StopTransform();
	}
	VARIANT_BOOL vb;
	proj->CopyFrom(projNew, &vb);

	this->transformationMode = mode;
	if (transformationMode == tmDoTransformation)
	{
		proj->StartTransform(projWGS84, &vb);
		if (!vb) {
			transformationMode = tmNotDefined;
			return false;
		}
	}
	return (transformationMode != tmNotDefined);
}

// *******************************************************
//		AddPoint()
// *******************************************************
void CMeasuring::AddPoint(double xProj, double yProj, double xScreen, double yScreen) 
{
	closedPoly = false;
	if (stopped)
	{
		this->points.clear();
		this->stopped = false;
	}
	this->mousePoint.x = xScreen;
	this->mousePoint.y = yScreen;
	
	MeasurePoint* pnt = new MeasurePoint();
	pnt->Proj.x = xProj;
	pnt->Proj.y = yProj;
	points.push_back(pnt);	

	double x = xProj, y = yProj;
	if (TransformPoint(x, y))
	{
		pnt->x = x;
		pnt->y = y;
	}
	areaRecalcIsNeeded = true;
}

// *******************************************************
//		GetTransformedPoints()
// *******************************************************
// trasnforms input data to decimal degrees
bool CMeasuring::TransformPoint(double& x, double& y) {
	
	VARIANT_BOOL vb;
	switch (transformationMode)
	{
		case tkTransformationMode::tmDoTransformation:
			proj->Transform(&x, &y, &vb);
			return true;
		case tkTransformationMode::tmWgs84Complied:	
			return true;
	}
	return false;
}
