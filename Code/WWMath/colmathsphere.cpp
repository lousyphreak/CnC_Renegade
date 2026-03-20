/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : WWMath                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwmath/colmathsphere.cpp                     $*
 *                                                                                             *
 *                       Author:: Greg Hjelstrom                                               *
 *                                                                                             *
 *                     $Modtime:: 4/25/01 2:05p                                               $*
 *                                                                                             *
 *                    $Revision:: 6                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   CollisionMath::Intersection_Test -- Sphere - AAbox intersection                           *
 *   CollisionMath::Intersection_Test -- Sphere - OBBox intersection                           *
 *   CollisionMath::Overlap_Test -- Sphere - Point overlap test                                *
 *   CollisionMath::Overlap_Test -- sphere line overlap test                                   *
 *   CollisionMath::Overlap_Test -- sphere triangle overlap test                               *
 *   CollisionMath::Overlap_Test -- Sphere - Sphere overlap test                               *
 *   CollisionMath::Overlap_Test -- Sphere - AABox overlap test                                *
 *   CollisionMath::Overlap_Test -- Sphere - OBBox overlap test                                *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#include "colmath.h"
#include "aaplane.h"
#include "plane.h"
#include "lineseg.h"
#include "tri.h"
#include "sphere.h"
#include "aabox.h"
#include "obbox.h"
#include "wwdebug.h"

namespace
{

float Distance_Squared_To_Line_Segment(const Vector3 & point, const LineSegClass & line)
{
	const Vector3 closest = line.Find_Point_Closest_To(point);
	return (closest - point).Length2();
}

float Distance_Squared_To_Triangle(const Vector3 & point, const TriClass & tri)
{
	const Vector3 edge0 = *tri.V[1] - *tri.V[0];
	const Vector3 edge1 = *tri.V[2] - *tri.V[0];
	const Vector3 normal = *tri.N;
	const float plane_distance = Vector3::Dot_Product(point - *tri.V[0], normal);
	const Vector3 projected_point = point - plane_distance * normal;

	if (tri.Contains_Point(projected_point)) {
		return plane_distance * plane_distance;
	}

	LineSegClass edges[3] = {
		LineSegClass(*tri.V[0], *tri.V[1]),
		LineSegClass(*tri.V[1], *tri.V[2]),
		LineSegClass(*tri.V[2], *tri.V[0])
	};

	float best_distance_sq = Distance_Squared_To_Line_Segment(point, edges[0]);
	for (int i = 1; i < 3; ++i) {
		const float edge_distance_sq = Distance_Squared_To_Line_Segment(point, edges[i]);
		if (edge_distance_sq < best_distance_sq) {
			best_distance_sq = edge_distance_sq;
		}
	}

	return best_distance_sq;
}

} // namespace


// Sphere Intersection fucntions.  Does the sphere intersect the passed in object
/***********************************************************************************************
 * CollisionMath::Intersection_Test -- Sphere - AAbox intersection                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   4/25/2001  gth : Created.                                                                 *
 *=============================================================================================*/
bool CollisionMath::Intersection_Test(const SphereClass & sphere,const AABoxClass & box)
{
	Vector3 closest_point(
		WWMath::Clamp(sphere.Center.X, box.Center.X - box.Extent.X, box.Center.X + box.Extent.X),
		WWMath::Clamp(sphere.Center.Y, box.Center.Y - box.Extent.Y, box.Center.Y + box.Extent.Y),
		WWMath::Clamp(sphere.Center.Z, box.Center.Z - box.Extent.Z, box.Center.Z + box.Extent.Z)
	);

	return (closest_point - sphere.Center).Length2() <= sphere.Radius * sphere.Radius;
}


/***********************************************************************************************
 * CollisionMath::Intersection_Test -- Sphere - OBBox intersection                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   4/25/2001  gth : Created.                                                                 *
 *=============================================================================================*/
bool CollisionMath::Intersection_Test(const SphereClass & sphere,const OBBoxClass & box)
{
	Matrix3D tm(box.Basis,box.Center);
	Vector3 box_rel_center;
	Matrix3D::Inverse_Transform_Vector(tm,sphere.Center,&box_rel_center);
	Vector3 closest_point(
		WWMath::Clamp(box_rel_center.X, -box.Extent.X, box.Extent.X),
		WWMath::Clamp(box_rel_center.Y, -box.Extent.Y, box.Extent.Y),
		WWMath::Clamp(box_rel_center.Z, -box.Extent.Z, box.Extent.Z)
	);

	return (closest_point - box_rel_center).Length2() <= sphere.Radius * sphere.Radius;
}

// Sphere Overlap functions.  Where is operand B with respect to the sphere
/***********************************************************************************************
 * CollisionMath::Overlap_Test -- Sphere - Point overlap test                                  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   4/25/2001  gth : Created.                                                                 *
 *=============================================================================================*/
CollisionMath::OverlapType
CollisionMath::Overlap_Test(const SphereClass & sphere,const Vector3 & point)
{
	float r2 = (point - sphere.Center).Length2();
	if (r2 < sphere.Radius * sphere.Radius - COINCIDENCE_EPSILON) {
		return NEG;
	}
	if (r2 > sphere.Radius * sphere.Radius + COINCIDENCE_EPSILON) {
		return POS;
	}
	return ON;
}


/***********************************************************************************************
 * CollisionMath::Overlap_Test -- sphere line overlap test                                     *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   4/25/2001  gth : Created.                                                                 *
 *=============================================================================================*/
CollisionMath::OverlapType
CollisionMath::Overlap_Test(const SphereClass & sphere,const LineSegClass & line)
{
	int mask = 0;
	mask |= CollisionMath::Overlap_Test(sphere, line.Get_P0());
	mask |= CollisionMath::Overlap_Test(sphere, line.Get_P1());
	const OverlapType endpoint_result = eval_overlap_mask(mask);
	if (endpoint_result != POS) {
		return endpoint_result;
	}

	const float radius_sq = sphere.Radius * sphere.Radius;
	const float distance_sq = Distance_Squared_To_Line_Segment(sphere.Center, line);
	if (distance_sq < radius_sq - COINCIDENCE_EPSILON) {
		return BOTH;
	}
	if (distance_sq <= radius_sq + COINCIDENCE_EPSILON) {
		return ON;
	}

	return POS;
}


/***********************************************************************************************
 * CollisionMath::Overlap_Test -- sphere triangle overlap test                                 *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   4/25/2001  gth : Created.                                                                 *
 *=============================================================================================*/
CollisionMath::OverlapType
CollisionMath::Overlap_Test(const SphereClass & sphere,const TriClass & tri)
{
	int mask = 0;
	mask |= CollisionMath::Overlap_Test(sphere, *tri.V[0]);
	mask |= CollisionMath::Overlap_Test(sphere, *tri.V[1]);
	mask |= CollisionMath::Overlap_Test(sphere, *tri.V[2]);
	const OverlapType vertex_result = eval_overlap_mask(mask);
	if (vertex_result != POS) {
		return vertex_result;
	}

	const float radius_sq = sphere.Radius * sphere.Radius;
	const float distance_sq = Distance_Squared_To_Triangle(sphere.Center, tri);
	if (distance_sq < radius_sq - COINCIDENCE_EPSILON) {
		return BOTH;
	}
	if (distance_sq <= radius_sq + COINCIDENCE_EPSILON) {
		return ON;
	}

	return POS;
}


/***********************************************************************************************
 * CollisionMath::Overlap_Test -- Sphere - Sphere overlap test                                 *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   4/25/2001  gth : Created.                                                                 *
 *=============================================================================================*/
CollisionMath::OverlapType
CollisionMath::Overlap_Test(const SphereClass & sphere,const SphereClass & sphere2)
{
	CollisionMath::OverlapType retval = OUTSIDE;

	float radius	= sphere.Radius + sphere2.Radius;
	float dist2		= (sphere2.Center - sphere.Center).Length2();
	
	if (dist2 == 0 && sphere.Radius == sphere2.Radius) {
		retval = OVERLAPPED;
	} else if (dist2 <= radius * radius - COINCIDENCE_EPSILON) {
		retval = INSIDE;
	}

	return retval;
}


/***********************************************************************************************
 * CollisionMath::Overlap_Test -- Sphere - AABox overlap test                                  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   4/25/2001  gth : Created.                                                                 *
 *=============================================================================================*/
CollisionMath::OverlapType
CollisionMath::Overlap_Test(const SphereClass & sphere,const AABoxClass & aabox)
{
	// TODO: overlap function that detects containment?
	return ( Intersection_Test(sphere,aabox) ? BOTH : POS );
}


/***********************************************************************************************
 * CollisionMath::Overlap_Test -- Sphere - OBBox overlap test                                  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   4/25/2001  gth : Created.                                                                 *
 *=============================================================================================*/
CollisionMath::OverlapType
CollisionMath::Overlap_Test(const SphereClass & sphere,const OBBoxClass & obbox)
{
	// TODO: overlap function that detects containment?
	return ( Intersection_Test(sphere,obbox) ? BOTH : POS );
}


