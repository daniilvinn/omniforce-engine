#pragma once

#include <Foundation/Common.h>

#include <vector>
#include <algorithm>
#include <limits>
#include <span>

#include <glm/glm.hpp>

namespace Omni {
	struct KDTreeNode {
		glm::vec3 point;
		uint32_t index;
		Scope<KDTreeNode> left;
		Scope<KDTreeNode> right;

		KDTreeNode(const glm::vec3& pt, uint32_t idx) : point(pt), index(idx), left(nullptr), right(nullptr) {}
	};

	class KDTree {
	public:
		KDTree() : root(nullptr) {}

		void BuildFromPointSet(const std::vector<std::pair<glm::vec3, uint32_t>>& points) {
			std::vector<std::pair<glm::vec3, uint32_t>> pointsCopy = points;
			root = BuildFromPointSetRecursive(pointsCopy, 0);
		}

		uint32_t ClosestPoint(const glm::vec3& target, uint32_t targetIndex) const {
			return nearestNeighborRec(root.get(), target, targetIndex, 0, std::numeric_limits<float>::max(), 0);
		}

		std::vector<uint32_t> ClosestPointSet(const glm::vec3& target, float maxDistance, uint32_t targetIndex) const {
			std::vector<uint32_t> result;
			findPointsWithinDistanceRec(root.get(), target, maxDistance, targetIndex, 0, result);
			return result;
		}

	private:
		Scope<KDTreeNode> root;

		Scope<KDTreeNode> BuildFromPointSetRecursive(std::vector<std::pair<glm::vec3, uint32_t>>& points, int depth) {
			if (points.empty()) {
				return nullptr;
			}

			int axis = depth % 3;
			std::nth_element(points.begin(), points.begin() + points.size() / 2, points.end(),
				[axis](const std::pair<glm::vec3, uint32_t>& a, const std::pair<glm::vec3, uint32_t>& b) {
					return a.first[axis] < b.first[axis];
				});

			size_t medianIndex = points.size() / 2;
			Scope<KDTreeNode> node = std::make_unique<KDTreeNode>(points[medianIndex].first, points[medianIndex].second);

			std::vector<std::pair<glm::vec3, uint32_t>> leftPoints(points.begin(), points.begin() + medianIndex);
			std::vector<std::pair<glm::vec3, uint32_t>> rightPoints(points.begin() + medianIndex + 1, points.end());

			node->left = BuildFromPointSetRecursive(leftPoints, depth + 1);
			node->right = BuildFromPointSetRecursive(rightPoints, depth + 1);

			return node;
		}

		uint32_t nearestNeighborRec(KDTreeNode* node, const glm::vec3& target, uint32_t targetIndex, int depth, float bestDist, uint32_t bestIndex) const {
			if (node == nullptr) {
				return bestIndex;
			}

			float dist = glm::distance(node->point, target);
			if (dist < bestDist && node->index != targetIndex) {
				bestDist = dist;
				bestIndex = node->index;
			}

			int axis = depth % 3;
			KDTreeNode* nextNode = (target[axis] < node->point[axis]) ? node->left.get() : node->right.get();
			KDTreeNode* otherNode = (target[axis] < node->point[axis]) ? node->right.get() : node->left.get();

			bestIndex = nearestNeighborRec(nextNode, target, targetIndex, depth + 1, bestDist, bestIndex);
			if (std::fabs(target[axis] - node->point[axis]) < bestDist) {
				bestIndex = nearestNeighborRec(otherNode, target, targetIndex, depth + 1, bestDist, bestIndex);
			}

			return bestIndex;
		}

		void findPointsWithinDistanceRec(KDTreeNode* node, const glm::vec3& target, float maxDistance, uint32_t targetIndex, int depth, std::vector<uint32_t>& result) const {
			if (node == nullptr) {
				return;
			}

			float dist = glm::distance(node->point, target);
			if (dist <= maxDistance && node->index != targetIndex) {
				result.push_back(node->index);
			}

			int axis = depth % 3;
			KDTreeNode* nextNode = (target[axis] < node->point[axis]) ? node->left.get() : node->right.get();
			KDTreeNode* otherNode = (target[axis] < node->point[axis]) ? node->right.get() : node->left.get();

			findPointsWithinDistanceRec(nextNode, target, maxDistance, targetIndex, depth + 1, result);
			if (std::fabs(target[axis] - node->point[axis]) < maxDistance) {
				findPointsWithinDistanceRec(otherNode, target, maxDistance, targetIndex, depth + 1, result);
			}
		}
	};

}