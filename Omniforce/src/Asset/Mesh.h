#pragma once

#include "Foundation/Macros.h"
#include "Asset/AssetBase.h"
#include "Renderer/DeviceBuffer.h"
#include "Renderer/DeviceBufferLayout.h"

#include <filesystem>

namespace Omni {

	class OMNIFORCE_API Mesh : public AssetBase {
	public: 
		Mesh(std::filesystem::path path, AssetHandle handle);
		~Mesh();
		
		static Shared<Mesh> Create(std::filesystem::path path, AssetHandle handle);
		void Destroy() override {};

		std::vector<uint8>* GetData() { return &m_Data; }
		std::vector<uint32>* GetIndices() { return &m_Indices; }
		DeviceBufferLayout* GetLayout() { return &m_BufferLayout; }

		bool Valid() const { return m_Valid; }
		

	private:
		std::vector<uint8> m_Data;
		std::vector<uint32> m_Indices;
		DeviceBufferLayout m_BufferLayout;

		bool m_Valid = false;

	};

}