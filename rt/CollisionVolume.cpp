//
//  CollisionVolume.cpp
//  MiRay/rt
//
//  Created by Damir Sagidullin on 08.08.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#include "CollisionVolume.h"

using namespace mr;

// ------------------------------------------------------------------------ //

CollisionVolume::CollisionVolume(size_t nReserveTrangles)
	: m_root(NULL)
	, m_nTraceCount(0)
	, m_matTransformation(Matrix::Identity)
	, m_matInvTransformation(Matrix::Identity)
{
	m_triangles.reserve(nReserveTrangles);
	m_aabb.ClearBounds();
}

// ------------------------------------------------------------------------ //

void CollisionVolume::SetTransformation(const Matrix & m)
{
	m_matTransformation = m;
	m_matInvTransformation.Inverse(m);

	m_aabb = m_root.BoundingBox();
	m_aabb.Transform(m);
}

// ------------------------------------------------------------------------ //

void CollisionVolume::AddTriangle(const CollisionTriangle & t)
{
	if (!t.IsDegenerate())
		m_triangles.push_back(t);
}

void CollisionVolume::Build(byte nMaxNodesLevel)
{
	if (nMaxNodesLevel == 0)
	{
		while ((nMaxNodesLevel < 32) && ((size_t)(1 << nMaxNodesLevel) < m_triangles.size()))
			nMaxNodesLevel++;
	}
	
	if (nMaxNodesLevel > 0)
	{
		CollisionNode::PolygonArray polygons;
		polygons.reserve(m_triangles.size());
		for (CollisionTriangleArray::iterator it = m_triangles.begin(), itEnd = m_triangles.end(); it != itEnd; ++it)
		{
			polygons.push_back(CollisionNode::Polygon(&(*it)));
			polygons.back().AddVertex(it->Vertex(0).pos);
			polygons.back().AddVertex(it->Vertex(1).pos);
			polygons.back().AddVertex(it->Vertex(2).pos);
		}
		
		m_root.Create(nMaxNodesLevel - 1, polygons);
		m_aabb = m_root.BoundingBox();
		m_aabb.Transform(m_matTransformation);
	}
}

// ------------------------------------------------------------------------ //

bool CollisionVolume::TraceRay(const Vec3 & vFrom, const Vec3 & vTo, TraceResult & tr)
{
	m_nTraceCount++;
	const CollisionTriangle * pSkipTriangle = tr.pTriangle;

	CollisionRay ray(vFrom.GetTransformedCoord(m_matInvTransformation), vTo.GetTransformedCoord(m_matInvTransformation), tr);

	const CollisionNode * pStackNodes[64]; // nMaxNodesLevel must be less than 64
	const CollisionNode ** ppTopNode = pStackNodes;
	*ppTopNode++ = &m_root;
	
	while (ppTopNode > pStackNodes)
	{
		const CollisionNode * pNode = *(--ppTopNode);
		if (!ray.TestIntersection(pNode->Center(), pNode->Extents()))
			continue;

		if (pNode->Child(0))
		{
			assert(pNode->Child(0) && pNode->Child(1));
			if (ray.Origin()[pNode->Axis()] > pNode->Dist())
			{
				*ppTopNode++ = pNode->Child(0);
				*ppTopNode++ = pNode->Child(1);
			}
			else
			{
				*ppTopNode++ = pNode->Child(1);
				*ppTopNode++ = pNode->Child(0);
			}
		}
		else
		{
//			for (std::vector<CollisionTriangle *>::const_iterator it = pNode->Triangles().begin(), itEnd = pNode->Triangles().end(); it != itEnd; ++it)
//			{
//				CollisionTriangle * pTriangle = *it;
			for (CollisionTriangle * pTriangle : pNode->Triangles())
			{
//				if (pTriangle->CheckTraceCount(m_nTraceCount) && pTriangle->TraceRay(ray, tr))
				if (tr.pTriangle != pTriangle && pTriangle->TraceRay(ray, tr))
				{
					assert(tr.pTriangle == pTriangle);
				}
			}
			
			if (tr.pTriangle != pSkipTriangle && pNode->BoundingBox().Test(ray.End()))
//			if (tr.pTriangle != pSkipTriangle)
				break;
		}
	}

	if (tr.pTriangle == pSkipTriangle)
		return false;

	tr.pVolume = this;
	tr.pos = ray.End().GetTransformedCoord(m_matTransformation);
	tr.localPos = ray.End();
	tr.localDir = ray.Direction();

	return true;
}