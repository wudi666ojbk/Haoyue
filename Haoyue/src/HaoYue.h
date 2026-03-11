//
// Note:	this file is to be included in client applications ONLY
//			NEVER include this file anywhere in the engine codebase
//
#pragma once

#include "Haoyue/Core/Application.h"
#include "Haoyue/Core/Log.h"
#include "Haoyue/Core/Input.h"
#include "Haoyue/Core/Timestep.h"
#include "Haoyue/Core/Timer.h"

#include "Haoyue/Core/Events/Event.h"
#include "Haoyue/Core/Events/ApplicationEvent.h"
#include "Haoyue/Core/Events/KeyEvent.h"
#include "Haoyue/Core/Events/MouseEvent.h"

#include "Haoyue/Core/Math/AABB.h"
#include "Haoyue/Core/Math/Ray.h"

#include "imgui/imgui.h"

// --- Haoyue Render API ------------------------------
#include "Haoyue/Renderer/Renderer.h"
#include "Haoyue/Renderer/SceneRenderer.h"
#include "Haoyue/Renderer/RenderPass.h"
#include "Haoyue/Renderer/Framebuffer.h"
#include "Haoyue/Renderer/VertexBuffer.h"
#include "Haoyue/Renderer/IndexBuffer.h"
#include "Haoyue/Renderer/Pipeline.h"
#include "Haoyue/Renderer/Texture.h"
#include "Haoyue/Renderer/Shader.h"
#include "Haoyue/Renderer/Mesh.h"
#include "Haoyue/Renderer/Camera.h"
#include "Haoyue/Renderer/Material.h"
// ---------------------------------------------------

// Scenes
#include "Haoyue/Scene/Entity.h"
#include "Haoyue/Scene/Scene.h"
#include "Haoyue/Scene/SceneCamera.h"
#include "Haoyue/Scene/SceneSerializer.h"
#include "Haoyue/Scene/Components.h"