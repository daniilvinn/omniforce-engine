#pragma once

#include "VulkanCommon.h"

#include <Renderer/DescriptorSet.h>	

namespace Omni {

	class VulkanDescriptorSet : public DescriptorSet {
	public:
		VulkanDescriptorSet(const DescriptorSetSpecification& spec);
		~VulkanDescriptorSet();

		void Destroy() override;

		VkDescriptorSet Raw() const { return m_DescriptorSet; }
		VkDescriptorSetLayout RawLayout() const { return m_Layout; }

		void Write(uint16 binding, uint16 array_element, Shared<Image> image, Shared<ImageSampler> sampler) override;
		void Write(uint16 binding, uint16 array_element, Shared<DeviceBuffer> buffer, uint64 size, uint64 offset) override;

	private:
		VkDescriptorSet m_DescriptorSet;
		VkDescriptorSetLayout m_Layout;
	};

}