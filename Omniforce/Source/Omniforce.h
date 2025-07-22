#pragma once

// Engine API

// Foundation
#include <Foundation/Common.h>

// Core
#include <Core/Application.h>
#include <Core/Subsystem.h>
#include <Core/Utils.h>
#include <Core/Events/ApplicationEvents.h>
#include <Core/Events/KeyEvents.h>
#include <Core/Events/MouseEvents.h>
#include <Core/Input/Input.h>
#include <Core/EngineConfig.h>

#include <Threading/JobSystem.h>

// RHI
#include <RHI/Renderer.h>
#include <RHI/Swapchain.h>
#include <RHI/DeviceBuffer.h>
#include <RHI/ShaderLibrary.h>
#include <RHI/Shader.h>
#include <RHI/Pipeline.h>
#include <RHI/Image.h>
#include <RHI/DescriptorSet.h>
#include <DebugUtils/DebugRenderer.h>

// Scene
#include <Scene/Scene.h>
#include <Scene/Entity.h>
#include <Scene/Component.h>

// Rendering
#include <Rendering/ISceneRenderer.h>
#include <Rendering/Raster/RasterSceneRenderer.h>
#include <Rendering/PathTracing/PathTracingSceneRenderer.h>
#include <Rendering/Mesh.h>

// Asset
#include <Asset/AssetManager.h>
#include <Asset/AssetBase.h>
#include <Asset/AssetType.h>
#include <Asset/Material.h>
#include <Asset/Importers/ImageImporter.h>
#include <Asset/Importers/ModelImporter.h>
#include <Asset/AssetCompressor.h>
#include <Asset/AssetFile.h>
#include <Asset/OFRController.h>
#include <Asset/Model.h>

// File system
#include <Filesystem/Filesystem.h>

// Scripts
#include <Scripting/ScriptEngine.h>

// ImGui
#include <imgui.h>

// Entry Point
#include <Core/EntryPoint.h>