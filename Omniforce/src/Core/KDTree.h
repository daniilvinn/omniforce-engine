#pragma once

#include <Foundation/Macros.h>
#include <Foundation/Types.h>

#include <vector>
#include <algorithm>
#include <limits>

#include <glm/glm.hpp>

namespace Omni {

	// Currently, only 3-dimensional tree
	// TODO: fix memory leak
	struct KDTreeNode {
		glm::vec3 point;
		uint32 index;
		KDTreeNode* left;
		KDTreeNode* right;

		KDTreeNode(const glm::vec3& pt, uint32 idx) : point(pt), index(idx), left(nullptr), right(nullptr) {}
	};

	class KDTree {
	public:
		KDTree() : m_RootNode(nullptr) {}

		void AddPoint(const glm::vec3& point, uint32_t index) {
			if (!SearchPoint(point, index)) {
				m_RootNode = insertRec(m_RootNode, point, index, 0);
			}
		}

		uint32_t FindNearestPoint(const glm::vec3& target, uint32_t target_index) const {
			return FindNearestPointRecursive(m_RootNode, target, target_index, 0, std::numeric_limits<float32>::max(), 0);
		}

		std::vector<uint32_t> FindNearestPoints(const glm::vec3& target, float32 maxDistance, uint32_t targetIndex) const {
			std::vector<uint32_t> result;
			FindNearestPointsRecursive(m_RootNode, target, maxDistance, targetIndex, 0, result);
			return result;
		}

	private:
		KDTreeNode* insertRec(KDTreeNode* node, const glm::vec3& point, uint32_t index, uint32 depth) {
			if (node == nullptr) {
				return new KDTreeNode(point, index);
			}

			uint32 axis = depth % 3;
			if (point[axis] < node->point[axis]) {
				node->left = insertRec(node->left, point, index, depth + 1);
			}
			else {
				node->right = insertRec(node->right, point, index, depth + 1);
			}

			return node;
		}

		bool SearchPoint(const glm::vec3& point, uint32_t index) {
			return SearchPointRecursive(m_RootNode, point, index, 0);
		}

		bool SearchPointRecursive(KDTreeNode* node, const glm::vec3& point, uint32_t index, uint32 depth) {
			if (node == nullptr) {
				return false;
			}
			if (node->point == point && node->index == index) {
				return true;
			}

			uint32 axis = depth % 3;
			if (point[axis] < node->point[axis]) {
				return SearchPointRecursive(node->left, point, index, depth + 1);
			}
			else {
				return SearchPointRecursive(node->right, point, index, depth + 1);
			}
		}

		uint32_t FindNearestPointRecursive(KDTreeNode* node, const glm::vec3& target, uint32_t targetIndex, uint32 depth, float32 best_distance, uint32_t best_index) const {
			if (node == nullptr) {
				return best_index;
			}

			float32 dist = glm::distance(node->point, target);
			if (dist < best_distance && node->index != targetIndex) {
				best_distance = dist;
				best_index = node->index;
			}

			uint32 axis = depth % 3;
			KDTreeNode* nextNode = (target[axis] < node->point[axis]) ? node->left : node->right;
			KDTreeNode* otherNode = (target[axis] < node->point[axis]) ? node->right : node->left;

			best_index = FindNearestPointRecursive(nextNode, target, targetIndex, depth + 1, best_distance, best_index);
			if (std::fabs(target[axis] - node->point[axis]) < best_distance) {
				best_index = FindNearestPointRecursive(otherNode, target, targetIndex, depth + 1, best_distance, best_index);
			}

			return best_index;
		}

		void FindNearestPointsRecursive(KDTreeNode* node, const glm::vec3& target, float32 max_distance, uint32_t target_index, uint32 depth, std::vector<uint32_t>& result) const {
			if (node == nullptr) {
				return;
			}

			float32 dist = glm::distance(node->point, target);
			if (dist <= max_distance && node->index != target_index) {
				result.push_back(node->index);
			}

			uint32 axis = depth % 3;

			KDTreeNode* nextNode = (target[axis] < node->point[axis]) ? node->left : node->right;
			KDTreeNode* otherNode = (target[axis] < node->point[axis]) ? node->right : node->left;

			FindNearestPointsRecursive(nextNode, target, max_distance, target_index, depth + 1, result);
			if (glm::abs(target[axis] - node->point[axis]) < max_distance) {
				FindNearestPointsRecursive(otherNode, target, max_distance, target_index, depth + 1, result);
			}
		}

		KDTreeNode* m_RootNode;
	};

}