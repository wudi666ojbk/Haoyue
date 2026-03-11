#pragma once

#include "Haoyue/Renderer/Texture.h"

namespace Haoyue{

	class EditorResources
	{
	public:
		// Components


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
			MoonIcon			= Texture2D::Create("Resources/editor/Moon.png");
			BackButtonIcon		= Texture2D::Create("Resources/editor/btn_back.png");
			ForwardButtonIcon	= Texture2D::Create("Resources/editor/btn_fwrd.png");
			AssetIcon			= Texture2D::Create("Resources/editor/asset.png");
		}

		static void Shutdown()
        {
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