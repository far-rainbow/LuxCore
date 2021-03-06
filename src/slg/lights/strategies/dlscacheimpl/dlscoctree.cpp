/***************************************************************************
 * Copyright 1998-2018 by authors (see AUTHORS.txt)                        *
 *                                                                         *
 *   This file is part of LuxCoreRender.                                   *
 *                                                                         *
 * Licensed under the Apache License, Version 2.0 (the "License");         *
 * you may not use this file except in compliance with the License.        *
 * You may obtain a copy of the License at                                 *
 *                                                                         *
 *     http://www.apache.org/licenses/LICENSE-2.0                          *
 *                                                                         *
 * Unless required by applicable law or agreed to in writing, software     *
 * distributed under the License is distributed on an "AS IS" BASIS,       *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
 * See the License for the specific language governing permissions and     *
 * limitations under the License.                                          *
 ***************************************************************************/

#include <algorithm>

#if defined(_OPENMP)
#include <omp.h>
#endif

#include <boost/format.hpp>
#include <boost/circular_buffer.hpp>

#include "luxrays/core/geometry/bbox.h"
#include "slg/samplers/sobol.h"
#include "slg/lights/strategies/dlscacheimpl/dlscacheimpl.h"
#include "slg/lights/strategies/dlscacheimpl/dlscoctree.h"

using namespace std;
using namespace luxrays;
using namespace slg;

//------------------------------------------------------------------------------
// DLSCOctree
//------------------------------------------------------------------------------

DLSCOctree::DLSCOctree(const BBox &bbox, const float r, const float normAngle, const u_int md) :
	worldBBox(bbox), maxDepth(md), entryRadius(r), entryRadius2(r * r),
	entryNormalCosAngle(cosf(Radians(normAngle))) {
	worldBBox.Expand(MachineEpsilon::E(worldBBox));
}

DLSCOctree::~DLSCOctree() {
}

void DLSCOctree::Add(DLSCacheEntry *cacheEntry) {
	const Vector entryRadiusVector(entryRadius, entryRadius, entryRadius);
	const BBox entryBBox(cacheEntry->p - entryRadiusVector, cacheEntry->p + entryRadiusVector);

	AddImpl(&root, worldBBox, cacheEntry, entryBBox, DistanceSquared(entryBBox.pMin,  entryBBox.pMax));
}

const DLSCacheEntry *DLSCOctree::GetEntry(const Point &p, const Normal &n,
		const bool isVolume) const {
	return GetEntryImpl(&root, worldBBox, p, n, isVolume);
}

void DLSCOctree::GetAllNearEntries(vector<DLSCacheEntry *> &entries,
		const Point &p, const Normal &n,
		const bool isVolume,
		const float radius) const {
	const Vector radiusVector(radius, radius, radius);
	const BBox bbox(p - radiusVector, p + radiusVector);

	return GetAllNearEntriesImpl(entries, &root, worldBBox,
			p, n, isVolume,
			bbox, radius * radius);
}

BBox DLSCOctree::ChildNodeBBox(u_int child, const BBox &nodeBBox,
	const Point &pMid) const {
	BBox childBound;

	childBound.pMin.x = (child & 0x4) ? pMid.x : nodeBBox.pMin.x;
	childBound.pMax.x = (child & 0x4) ? nodeBBox.pMax.x : pMid.x;
	childBound.pMin.y = (child & 0x2) ? pMid.y : nodeBBox.pMin.y;
	childBound.pMax.y = (child & 0x2) ? nodeBBox.pMax.y : pMid.y;
	childBound.pMin.z = (child & 0x1) ? pMid.z : nodeBBox.pMin.z;
	childBound.pMax.z = (child & 0x1) ? nodeBBox.pMax.z : pMid.z;

	return childBound;
}

void DLSCOctree::AddImpl(DLSCOctreeNode *node, const BBox &nodeBBox,
	DLSCacheEntry *entry, const BBox &entryBBox,
	const float entryBBoxDiagonal2, const u_int depth) {
	// Check if I have to store the entry in this node
	if ((depth == maxDepth) ||
			DistanceSquared(nodeBBox.pMin, nodeBBox.pMax) < entryBBoxDiagonal2) {
		node->entries.push_back(entry);
		return;
	}

	// Determine which children the item overlaps
	const Point pMid = .5 * (nodeBBox.pMin + nodeBBox.pMax);

	const bool x[2] = {
		entryBBox.pMin.x <= pMid.x,
		entryBBox.pMax.x > pMid.x
	};
	const bool y[2] = {
		entryBBox.pMin.y <= pMid.y,
		entryBBox.pMax.y > pMid.y
	};
	const bool z[2] = {
		entryBBox.pMin.z <= pMid.z,
		entryBBox.pMax.z > pMid.z
	};

	const bool overlap[8] = {
		bool(x[0] & y[0] & z[0]),
		bool(x[0] & y[0] & z[1]),
		bool(x[0] & y[1] & z[0]),
		bool(x[0] & y[1] & z[1]),
		bool(x[1] & y[0] & z[0]),
		bool(x[1] & y[0] & z[1]),
		bool(x[1] & y[1] & z[0]),
		bool(x[1] & y[1] & z[1])
	};

	for (u_int child = 0; child < 8; ++child) {
		if (!overlap[child])
			continue;

		// Allocated the child node if required
		if (!node->children[child])
			node->children[child] = new DLSCOctreeNode();

		// Add the entry to each overlapping child
		const BBox childBBox = ChildNodeBBox(child, nodeBBox, pMid);
		AddImpl(node->children[child], childBBox,
				entry, entryBBox, entryBBoxDiagonal2, depth + 1);
	}
}

const DLSCacheEntry *DLSCOctree::GetEntryImpl(const DLSCOctreeNode *node, const BBox &nodeBBox,
	const Point &p, const Normal &n, const bool isVolume) const {
	// Check if I'm inside the node bounding box
	if (!nodeBBox.Inside(p))
		return NULL;

	// Check every entry in this node
	for (auto entry : node->entries) {
		if ((DistanceSquared(p, entry->p) <= entryRadius2) &&
				(isVolume == entry->isVolume) && 
				(isVolume || (Dot(n, entry->n) >= entryNormalCosAngle))) {
			// I have found a valid entry
			return entry;
		}
	}

	// Check the children too
	const Point pMid = .5 * (nodeBBox.pMin + nodeBBox.pMax);
	for (u_int child = 0; child < 8; ++child) {
		if (node->children[child]) {
			const BBox childBBox = ChildNodeBBox(child, nodeBBox, pMid);

			const DLSCacheEntry *entry = GetEntryImpl(node->children[child], childBBox,
					p, n, isVolume);
			if (entry) {
				// I have found a valid entry
				return entry;
			}
		}
	}

	return NULL;
}

void DLSCOctree::GetAllNearEntriesImpl(vector<DLSCacheEntry *> &entries,
		const DLSCOctreeNode *node, const BBox &nodeBBox,
		const Point &p, const Normal &n,
		const bool isVolume,
		const BBox areaBBox,
		const float areaRadius2) const {
	// Check if I overlap the node bounding box
	if (!nodeBBox.Overlaps(areaBBox))
		return;

	// Check every entry in this node
	for (auto entry : node->entries) {
		if ((DistanceSquared(p, entry->p) <= areaRadius2) &&
				(isVolume == entry->isVolume) && 
				(isVolume || (Dot(n, entry->n) >= entryNormalCosAngle))) {
			// I have found a valid entry but I avoid to insert duplicates
			if (find(entries.begin(), entries.end(), entry) == entries.end())
				entries.push_back(entry);
		}
	}

	// Check the children too
	const Point pMid = .5 * (nodeBBox.pMin + nodeBBox.pMax);
	for (u_int child = 0; child < 8; ++child) {
		if (node->children[child]) {
			const BBox childBBox = ChildNodeBBox(child, nodeBBox, pMid);

			GetAllNearEntriesImpl(entries, node->children[child], childBBox,
					p, n, isVolume,
					areaBBox, areaRadius2);
		}
	}
}
