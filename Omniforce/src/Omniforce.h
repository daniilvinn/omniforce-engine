#pragma once

// Engine API

// Core
#include <Core/Application.h>
#include <Core/Subsystem.h>
#include <Core/Utils.h>
#include <Log/Logger.h>
#include <Threading/JobSystem.h>
#include <Threading/ConditionalLock.h>

#include <Core/Events/ApplicationEvents.h>
#include <Core/Events/KeyEvents.h>
#include <Core/Events/MouseEvents.h>

#include <Core/Input/Input.h>
#include <Core/Timer.h>

// Renderer
#include <Renderer/Renderer.h>
#include <Renderer/Swapchain.h>
#include <Renderer/DeviceBuffer.h>
#include <Renderer/ShaderLibrary.h>
#include <Renderer/Shader.h>
#include <Renderer/Pipeline.h>
#include <Renderer/Image.h>
#include <Renderer/DescriptorSet.h>
#include <Renderer/Mesh.h>

// Scene
#include <Scene/Scene.h>
#include <Scene/Entity.h>
#include <Scene/Component.h>
#include <Scene/SceneRenderer.h>

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