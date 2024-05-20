#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

#include <vector>
#include <algorithm>
#include <limits>

#include <glm/glm.hpp>

namespace Omni {

	// Currently, only 3-dimensional tree

	struct KDTreeNode {
		glm::vec3 point;
		uint32 index;
		KDTreeNode* left;
		KDTreeNode* right;

		KDTreeNode(const glm::vec3& pt, uint32 idx) : point(pt), index(idx), left(nullptr), right(nullptr) {}
	};

	class KDTree {
	public:
		KDTree() : m_Root(nullptr) {}

		void AddPoint(const glm::vec3& point, uint32 index) {
			m_Root = AddPointRecursive(m_Root, point, index, 0);
		}

		uint32 NearestPoint(const glm::vec3& target, uint32 target_index) {
			return NearestPointRecursive(m_Root, target, target_index, 0, std::numeric_limits<float32>::max(), 0);
		}

		std::vector<uint32> NearestPoints(const glm::vec3& target, float32 maxDistance, uint32 targetIndex) {
			std::vector<uint32> result;
			FindNearestPointsRecursive(m_Root, target, maxDistance, targetIndex, 0, result);
			return result;
		}

	private:

		KDTreeNode* AddPointRecursive(KDTreeNode* node, const glm::vec3& point, uint32 index, uint32 depth) {
			if (node == nullptr) {
				return new KDTreeNode(point, index);
			}

			uint32 axis = depth % 3;
			if (point[axis] < node->point[axis]) {
				node->left = AddPointRecursive(node->left, point, index, depth + 1);
			}
			else {
				node->right = AddPointRecursive(node->right, point, index, depth + 1);
			}

			return node;
		}

		uint32 NearestPointRecursive(KDTreeNode* node, const glm::vec3& target, uint32 target_index, uint32 depth, float32 best_distance, uint32 best_index) {
			if (node == nullptr) {
				return best_index;
			}

			float32 dist = glm::distance(node->point, target);
			if (dist < best_distance && node->index != target_index) {
				best_distance = dist;
				best_index = node->index;
			}

			uint32 axis = depth % 3;
			KDTreeNode* nextNode = (target[axis] < node->point[axis]) ? node->left : node->right;
			KDTreeNode* otherNode = (target[axis] < node->point[axis]) ? node->right : node->left;

			best_index = NearestPointRecursive(nextNode, target, target_index, depth + 1, best_distance, best_index);
			if (std::fabs(target[axis] - node->point[axis]) < best_distance) {
				best_index = NearestPointRecursive(otherNode, target, target_index, depth + 1, best_distance, best_index);
			}

			return best_index;
		}

		void FindNearestPointsRecursive(KDTreeNode* node, const glm::vec3& target, float32 max_distance, uint32 target_index, uint32 depth, std::vector<uint32>& result) {
			if (node == nullptr) {
				return;
			}

			float32 dist = glm::distance(node->point, target);
			if (dist <= max_distance && node->index != target_index) {
				result.push_back(node->index);
			}

			uint32 axis = depth % 3;

			KDTreeNode* next_node = (target[axis] < node->point[axis]) ? node->left : node->right;
			KDTreeNode* other_node = (target[axis] < node->point[axis]) ? node->right : node->left;

			FindNearestPointsRecursive(next_node, target, max_distance, target_index, depth + 1, result);
			if (glm::abs(target[axis] - node->point[axis]) < max_distance) {
				FindNearestPointsRecursive(other_node, target, max_distance, target_index, depth + 1, result);
			}
		}

		KDTreeNode* m_Root;
	};

}