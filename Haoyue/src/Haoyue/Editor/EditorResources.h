#pragma once

#include "Haoyue/Renderer/Texture.h"

namespace Haoyue{

	class EditorResources
	{
	public:
		// Components
		inline static Ref<Texture2D> TransformIcon = nullptr;
		inline static Ref<Texture2D> AnimationIcon = nullptr;
		inline static Ref<Texture2D> AudioIcon = nullptr;
		inline static Ref<Texture2D> BoxColliderIcon = nullptr;
		inline static Ref<Texture2D> CameraIcon = nullptr;
		inline static Ref<Texture2D> CircleColliderIcon = nullptr;
		inline static Ref<Texture2D> ComponentIcon = nullptr;
		inline static Ref<Texture2D> ListenerIcon = nullptr;
		inline static Ref<Texture2D> RendererIcon = nullptr;
		inline static Ref<Texture2D> RigidBodyIcon = nullptr;
		inline static Ref<Texture2D> ScriptIcon = nullptr;
		inline static Ref<Texture2D> TextIcon = nullptr;

		// Icons
		inline static Ref<Texture2D> PlayIcon = nullptr;
		inline static Ref<Texture2D> PauseButtonIcon = nullptr;
		inline static Ref<Texture2D> StopButtonTex = nullptr;
		inline static Ref<Texture2D> CheckerboardTex = nullptr;
		inline static Ref<Texture2D> FileIcon = nullptr;
		inline static Ref<Texture2D> FolderIcon = nullptr;
		inline static Ref<Texture2D> FbxIcon = nullptr;
		inline static Ref<Texture2D> ObjIcon = nullptr;
		inline static Ref<Texture2D> WavIcon = nullptr;
		inline static Ref<Texture2D> CscIcon = nullptr;
		inline static Ref<Texture2D> PngIcon = nullptr;
		inline static Ref<Texture2D> MoonIcon = nullptr;
		inline static Ref<Texture2D> BackButtonIcon = nullptr;
		inline static Ref<Texture2D> ForwardButtonIcon = nullptr;
		inline static Ref<Texture2D> AssetIcon = nullptr;

		static void Init()
		{
			TransformIcon		= Texture2D::Create("Resources/editor/Components/Transform.png");
			AnimationIcon		= Texture2D::Create("Resources/editor/Components/Animation.png");
			AudioIcon			= Texture2D::Create("Resources/editor/Components/Audio.png");
			BoxColliderIcon		= Texture2D::Create("Resources/editor/Components/BoxCollider.png");
			CameraIcon			= Texture2D::Create("Resources/editor/Components/Camera.png");
			CircleColliderIcon	= Texture2D::Create("Resources/editor/Components/CircleCollider.png");
			ComponentIcon		= Texture2D::Create("Resources/editor/Components/Component.png");
			ListenerIcon		= Texture2D::Create("Resources/editor/Components/Listener.png");
			RendererIcon		= Texture2D::Create("Resources/editor/Components/Renderer.png");
			RigidBodyIcon		= Texture2D::Create("Resources/editor/Components/RigidBody.png");
			ScriptIcon			= Texture2D::Create("Resources/editor/Components/Script.png");
			TextIcon			= Texture2D::Create("Resources/editor/Components/Text.png");

			CheckerboardTex		= Texture2D::Create("Resources/editor/Checkerboard.tga");
			PlayIcon			= Texture2D::Create("Resources/editor/PlayButton.png");
			PauseButtonIcon		= Texture2D::Create("Resources/editor/PauseButton.png");
			StopButtonTex		= Texture2D::Create("Resources/editor/StopButton.png");
			FileIcon			= Texture2D::Create("Resources/editor/file.png");
			FolderIcon			= Texture2D::Create("Resources/editor/folder.png");
			FbxIcon				= Texture2D::Create("Resources/editor/fbx.png");
			ObjIcon				= Texture2D::Create("Resources/editor/obj.png");
			WavIcon				= Texture2D::Create("Resources/editor/wav.png");
			CscIcon				= Texture2D::Create("Resources/editor/csc.png");
			PngIcon				= Texture2D::Create("Resources/editor/png.png");
			MoonIcon			= Texture2D::Create("Resources/editor/Haoyue.png");
			BackButtonIcon		= Texture2D::Create("Resources/editor/btn_back.png");
			ForwardButtonIcon	= Texture2D::Create("Resources/editor/btn_fwrd.png");
			AssetIcon			= Texture2D::Create("Resources/editor/asset.png");
		}

		static void Shutdown()
        {
            TransformIcon.Reset();
            AnimationIcon.Reset();
            AudioIcon.Reset();
            BoxColliderIcon.Reset();
            CameraIcon.Reset();
            CircleColliderIcon.Reset();
            ComponentIcon.Reset();
            ListenerIcon.Reset();
            RendererIcon.Reset();
            RigidBodyIcon.Reset();
            ScriptIcon.Reset();
            TextIcon.Reset();

            CheckerboardTex.Reset();
          	PlayIcon.Reset();
            PauseButtonIcon.Reset();
            StopButtonTex.Reset();
            FileIcon.Reset();
            FolderIcon.Reset();
            FbxIcon.Reset();
            ObjIcon.Reset();
            WavIcon.Reset();
            CscIcon.Reset();
            PngIcon.Reset();
			MoonIcon.Reset();
            BackButtonIcon.Reset();
            ForwardButtonIcon.Reset();
            AssetIcon.Reset();
        }
	};
}