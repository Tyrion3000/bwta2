#include "BaseLocationGenerator.h"

#include "BaseLocationImpl.h"
#include "RegionImpl.h"
#include "Heap.h"

namespace BWTA
{
	const int MIN_CLUSTER_DIST = 6; // minerals less than this distance will be grouped into the same cluster
	const size_t MIN_RESOURCES = 3; // a cluster with less than this will be discarded
	const int MAX_INFLUENCE_DISTANCE_RADIUS = 12; // max radius distance from a resource to place a base

	// TODO review this method, probably we can optimize it
	void calculate_walk_distances_area(const BWAPI::Position& start, int width, int height, int max_distance, 
		RectangleArray<int>& distance_map)
	{
		distance_map.setTo(-1);

		Heap<BWAPI::Position, int> heap(true);
		const int maxX = start.x + width;
		const int maxY = start.y + height;
		for (int x = start.x; x < maxX; ++x) {
			for (int y = start.y; y < maxY; ++y) {
				heap.push(std::make_pair(BWAPI::Position(x, y), 0));
				distance_map[x][y] = 0;
			}
		}

		while (!heap.empty()) {
			BWAPI::Position pos = heap.top().first;
			int distance = heap.top().second;
			heap.pop();
			if (distance > max_distance && max_distance > 0) break;
			int min_x = std::max(pos.x - 1, 0);
			int max_x = std::min(pos.x + 1, MapData::mapWidthWalkRes - 1);
			int min_y = std::max(pos.y - 1, 0);
			int max_y = std::min(pos.y + 1, MapData::mapHeightWalkRes - 1);
			for (int ix = min_x; ix <= max_x; ++ix) {
				for (int iy = min_y; iy <= max_y; ++iy) {
					int f = std::abs(ix - pos.x) * 10 + std::abs(iy - pos.y) * 10;
					if (f > 10) { f = 14; }
					int v = distance + f;
					if (distance_map[ix][iy] > v) {
						heap.push(std::make_pair(pos, v));
						distance_map[ix][iy] = v;
					} else if (distance_map[ix][iy] == -1 && MapData::rawWalkability[ix][iy] == true) {
						heap.push(std::make_pair(BWAPI::Position(ix, iy), v));
						distance_map[ix][iy] = v;
					}
				}
			}
		}
	}

	std::vector<int> findNeighbors(const std::vector<unitTypeTilePos_t>& resources, const unitTypeTilePos_t& resource)
	{
		std::vector<int> retIndexes;
		for (size_t i = 0; i < resources.size(); ++i) {
			int dist = resources[i].pos.getApproxDistance(resource.pos);
			if (dist <= MIN_CLUSTER_DIST) retIndexes.emplace_back(i);
		}
		return retIndexes;
	}

	void detectBaseLocations(std::set<BaseLocation*>& baseLocations)
	{
		Timer timer;
		timer.start();

		// 1) cluster resources using DBSCAN algorithm
		// ===========================================================================

		std::vector<std::vector<unitTypeTilePos_t>> clusters;
		std::vector<bool> clustered(MapData::resources.size());
		std::vector<bool> visited(MapData::resources.size());
		int clusterID = -1;

		// for each unvisited resource
		for (size_t i = 0; i < MapData::resources.size(); ++i) {
			if (!visited[i]) {
				visited[i] = true;
				std::vector<int> neighbors = findNeighbors(MapData::resources, MapData::resources[i]);
				if (neighbors.size() >= MIN_RESOURCES) {
					// add resource to a new cluster
					clusters.emplace_back(std::vector<unitTypeTilePos_t> {MapData::resources[i]});
					++clusterID;
					clustered[i] = true;

					for (size_t j = 0; j < neighbors.size(); ++j) {
						size_t neighborID = neighbors[j];
						if (!visited[neighborID]) {
							visited[neighborID] = true;
							std::vector<int> neighbors2 = findNeighbors(MapData::resources, MapData::resources[neighborID]);
							if (neighbors2.size() >= MIN_RESOURCES) {
								neighbors.insert(neighbors.end(), neighbors2.begin(), neighbors2.end());
							}
						}
						// if neighbor is not yet a member of any cluster
						if (!clustered[neighborID]) { 
							// add neighbor to current cluster
							clusters[clusterID].emplace_back(MapData::resources[neighborID]);
							clustered[neighborID] = true;
						}
					}
				}
			}
		}
//		for (const auto& c : clusters) LOG("  - Cluster size: " << c.size());
		LOG(" - Found " << clusters.size() << " resource clusters in " << timer.stopAndGetTime() << " seconds");
		timer.start();

		// 2) compute a buildable map where a resource depot can be build (4x3 tiles)
		// ===========================================================================

		RectangleArray<bool> baseBuildMap(MapData::mapWidthTileRes, MapData::mapHeightTileRes);
		// baseBuildMap[x][y] is true if build_map[ix][iy] is true for all x<=ix<x+4 and all y<=yi<+3
		// 4 and 3 are the tile width and height of a command center/nexus/hatchery
		for (unsigned int x = 0; x < MapData::buildability.getWidth(); ++x) {
			for (unsigned int y = 0; y < MapData::buildability.getHeight(); ++y) {
				// If this tile is too close to the bottom or right of the map, set it to false
				if (x + 4 > MapData::buildability.getWidth() || y + 3 > MapData::buildability.getHeight()) {
					baseBuildMap[x][y] = false;
					continue;
				}

				baseBuildMap[x][y] = true; // by default is buildable

				unsigned int maxX = std::min(x + 4, MapData::buildability.getWidth());
				unsigned int maxY = std::min(y + 3, MapData::buildability.getHeight());
				for (unsigned int ix = x; ix < maxX; ++ix) {
					for (unsigned int iy = y; iy < maxY; ++iy) {
						// if we found one non buildable tile, all the area (4x3) is  not buildable
						if (!MapData::buildability[ix][iy]) {
							baseBuildMap[x][y] = false;
							break;
						}
					}
				}

				
			}
		}
		// Set build tiles too close to resources in any cluster to false in baseBuildMap
		for (const auto& cluster : clusters) {
			for (const auto& resource : cluster) {
				int x1 = resource.pos.x - 6;
				int y1 = resource.pos.y - 5;
				int x2 = resource.pos.x + resource.type.tileWidth() + 2;
				int y2 = resource.pos.y + resource.type.tileHeight() + 2;
				baseBuildMap.setRectangleTo(x1, y1, x2, y2, false);
			}
		}
		LOG(" - baseBuildMap computed in " << timer.stopAndGetTime() << " seconds");
		timer.start();

		// 3) with the clusters and baseBuildMap, we will try to find a base location for each cluster
		// ===========================================================================

		RectangleArray<int> tileScores(MapData::mapWidthTileRes, MapData::mapHeightTileRes);
		tileScores.setTo(0);
		const int maxWidth = static_cast<int>(baseBuildMap.getWidth()) - 1;
		const int maxHeight = static_cast<int>(baseBuildMap.getHeight()) - 1;
		const int maxSquareDistance = MAX_INFLUENCE_DISTANCE_RADIUS*MAX_INFLUENCE_DISTANCE_RADIUS*2;

		// copy of start locations
		std::deque<BWAPI::TilePosition> startLocations = MapData::startLocations;

		for (const auto& cluster : clusters) {
			int clusterMaxX = 0;
			int clusterMinX = maxWidth;
			int clusterMaxY = 0;
			int clusterMinY = maxHeight;
			for (const auto& resource : cluster) {
				// TODO add methods to update bounding boxes
				// bounding box of current resource
				int minX = std::max(resource.pos.x - MAX_INFLUENCE_DISTANCE_RADIUS, 0);
				int maxX = std::min(resource.pos.x + MAX_INFLUENCE_DISTANCE_RADIUS, maxWidth);
				int minY = std::max(resource.pos.y - MAX_INFLUENCE_DISTANCE_RADIUS, 0);
				int maxY = std::min(resource.pos.y + MAX_INFLUENCE_DISTANCE_RADIUS, maxHeight);

				// a geyser has more "weight" than minerals
				int scoreScale = resource.type == BWAPI::UnitTypes::Resource_Vespene_Geyser? 3 : 1;
				// updating score inside the bounding box
				for (int x = minX; x < maxX; ++x) {
					for (int y = minY; y < maxY; ++y) {
						if (baseBuildMap[x][y]) {
							int dx = resource.pos.x - x;
							int dy = resource.pos.y - y;
							// since we only need to compare distances the squareDistance is enough
							int squareDistance = dx*dx + dy*dy; 
							tileScores[x][y] += (maxSquareDistance - squareDistance) * scoreScale;
						}
					}
				}
				
				// update the bounding box of the cluster
				clusterMaxX = std::max(maxX, clusterMaxX);
				clusterMinX = std::min(minX, clusterMinX);
				clusterMaxY = std::max(maxY, clusterMaxY);
				clusterMinY = std::min(minY, clusterMinY);
			}
			BWAPI::TilePosition bestTile(-1, -1);
			int maxScore = 0;
			for (int x = clusterMinX; x < clusterMaxX; ++x)
			for (int y = clusterMinY; y < clusterMaxY; ++y) {
				if (tileScores[x][y] > maxScore) {
					maxScore = tileScores[x][y];
					bestTile = BWAPI::TilePosition(x, y);
				}
			}
			if (maxScore > 0) {
				BaseLocationImpl* b = new BaseLocationImpl(bestTile, cluster);
				baseLocations.insert(b);
				// check if it is a start location
				for (auto it = startLocations.begin(); it != startLocations.end(); ++it) {
					int distance = it->getApproxDistance(bestTile);
					if (distance < 10) {
						b->_isStartLocation = true;
						BWTA_Result::startlocations.insert(b);
						startLocations.erase(it);
						break;
					}
				}
			} else {
				LOG(" - [ERROR] No BaseLocation found for a cluster");
			}
		}
//		tileScores.saveToFile("logs/tileScores.txt", ',');
		if (!startLocations.empty()) LOG(" - [ERROR] " << startLocations.size() << " start locations not found.");


		LOG(" - Best baseLocations computed in " << timer.stopAndGetTime() << " seconds");
	}



	//attach resource pointers to base locations based on proximity (walk distance)
	void attachResourcePointersToBaseLocations(std::set<BWTA::BaseLocation*>& baseLocations)
	{
		RectangleArray<int> distanceMap(MapData::mapWidthWalkRes, MapData::mapHeightWalkRes);
		for (auto& b : baseLocations) {
			BWAPI::Position p(b->getTilePosition().x * 4, b->getTilePosition().y * 4);
			calculate_walk_distances_area(p, 16, 12, 10 * 4 * 10, distanceMap);
			BWTA::BaseLocationImpl* ii = static_cast<BWTA::BaseLocationImpl*>(b);
			
			for (auto geyser : BWAPI::Broodwar->getStaticGeysers()) {
				int x = geyser->getInitialTilePosition().x * 4 + 8;
				int y = geyser->getInitialTilePosition().y * 4 + 4;
				if (distanceMap[x][y] >= 0 && distanceMap[x][y] <= 4 * 10 * 10) {
					ii->geysers.insert(geyser);
				}
			}
			
			for (auto mineral : BWAPI::Broodwar->getStaticMinerals()) {
				int x = mineral->getInitialTilePosition().x * 4 + 4;
				int y = mineral->getInitialTilePosition().y * 4 + 2;
				if (distanceMap[x][y] >= 0 && distanceMap[x][y] <= 4 * 10 * 10) {
					ii->staticMinerals.insert(mineral);
				}
			}
		}
	}

	void calculateBaseLocationProperties()
	{
		RectangleArray<double> distanceMap; 
		for (auto& base : BWTA_Result::baselocations) {
			BaseLocationImpl* baseI = static_cast<BaseLocationImpl*>(base);

			// TODO this can be optimized only computing the distance between reachable base locations
			BWAPI::TilePosition baseTile = base->getTilePosition();
			getGroundDistanceMap(baseTile, distanceMap);
			// assume the base location is an island unless we can walk from this base location to another base location
			for (const auto& base2 : BWTA_Result::baselocations) {
				if (base == base2) {
					baseI->groundDistances[base2] = 0;
					baseI->airDistances[base2] = 0;
				} else {
					BWAPI::TilePosition base2Tile = base2->getTilePosition();
					if (baseI->_isIsland && isConnected(baseTile, base2Tile)) {
						baseI->_isIsland = false;
					}
					baseI->groundDistances[base2] = distanceMap[base2Tile.x][base2Tile.y];
					baseI->airDistances[base2] = baseTile.getDistance(base2Tile);
				}
			}

			// look if this base location is a start location
//			for (const auto& startLocation : MapData::startLocations) {
//				int distance = startLocation.getApproxDistance(base->getTilePosition());
//				if (distance < 10) {
//					baseI->_isStartLocation = true;
//					BWTA_Result::startlocations.insert(base);
//					break;
//				}
//			}

			// find what region this base location is in and tell that region about the base location
			BWAPI::WalkPosition baseWalkPos(base->getPosition());
			int baseRegionLabel = BWTA_Result::regionLabelMap[baseWalkPos.x][baseWalkPos.y];
			for (auto& r : BWTA_Result::regions) {
				if (r->getLabel() == baseRegionLabel) { // TODO I need a vector to map labelId to Region*
					baseI->region = r;
					static_cast<RegionImpl*>(r)->baseLocations.insert(base);
					break;
				}
			}

		}
	}
}