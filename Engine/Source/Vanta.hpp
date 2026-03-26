//==============================================================================
// Vanta Engine - Client Application Header
//
// This header is meant to be included **only** in client applications using the Vanta Engine.
//
// **DO NOT** include this file in the engine's internal codebase.
//
// Including this file provides access to core engine functionality.
//
//==============================================================================

#pragma once

#include "Core/Application.hpp"
#include "Core/Log.hpp"
#include "Core/Input.hpp"
#include "Core/TimeStep.hpp"
#include "Core/Assert.hpp"

#include "Events/Event.hpp"
#include "Events/ApplicationEvent.hpp"
#include "Events/KeyEvent.hpp"
#include "Events/MouseEvent.hpp"

#include <imgui.h>

// ==== Vanta Render API ====
#include "Renderer/Renderer.hpp"
#include "Renderer/Framebuffer.hpp"
#include "Renderer/Buffer.hpp"
#include "Renderer/Texture.hpp"
#include "Renderer/Shader.hpp"
#include "Renderer/Mesh.hpp"
#include "Renderer/Camera.hpp"
#include "Renderer/Material.hpp"
// ==========================