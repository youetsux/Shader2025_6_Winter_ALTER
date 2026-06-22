#include "TestScene.h"
#include "Engine/Input.h"
#include "Engine/SceneManager.h"
#include "Stage.h"
// ToDo03: Audio機能を使うためにインクルードする
#include "Engine/Audio.h"


TestScene::TestScene(GameObject* parent)
	:GameObject(parent, "TestScene")
{
}

TestScene::~TestScene()
{
}

void TestScene::Initialize()
{
	Instantiate<Stage>(this);

	// ToDo04: SEとBGMのファイルパスを指定してロードし、BGMを再生する
	// ToDo04a: Assets/Audio/SE/ と Assets/Audio/BGM/ フォルダにwavファイルを入れる
	file_path shotPath   = "Assets/Audio/SE/A1_02033.WAV";
	file_path damagePath = "Assets/Audio/SE/A1_02034.WAV";
	file_path stagePath  = "Assets/Audio/BGM/BGM1.wav";

	Audio::LoadSE("shot", shotPath);
	Audio::LoadSE("damage", damagePath);
	Audio::LoadBGM("stage", stagePath);

	Audio::PlayBGM("stage", true);
}

void TestScene::Update()
{
	//if (Input::IsKeyDown(DIK_SPACE)) {
	//	SceneManager* pSceneManager = (SceneManager*)FindObject("SceneManager");
	//	pSceneManager->ChangeScene(SCENE_ID_PLAY);
	//}
	//スペースキー押したら 
	// SceneManager::ChangeScene(SCENE_ID_PLAY); を呼び出してね



	// ToDo05: キー入力でSEを鳴らす・BGMを止める
	if (Input::IsKeyDown(DIK_SPACE))
	{
		Audio::PlaySE("shot");
	}

	if (Input::IsKeyDown(DIK_D))
	{
		Audio::PlaySE("damage");
	}

	if (Input::IsKeyDown(DIK_B))
	{
		Audio::StopBGM();
	}

	// ToDo10: key 1=full / 2=half / 3=mute / F=BGM fadeout(2sec)
	if (Input::IsKeyDown(DIK_1)) { Audio::SetMasterVolume(1.0f); }
	if (Input::IsKeyDown(DIK_2)) { Audio::SetMasterVolume(0.5f); }
	if (Input::IsKeyDown(DIK_3)) { Audio::SetMasterVolume(0.0f); }
	if (Input::IsKeyDown(DIK_F)) { Audio::FadeOutBGM(2.0f); }
}

void TestScene::Draw()
{
}

void TestScene::Release()
{
}
