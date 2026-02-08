#include "TestScene.h"
#include "Engine/Input.h"
#include "Engine/SceneManager.h"
#include "Stage.h"

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
}

void TestScene::Update()
{
	//if (Input::IsKeyDown(DIK_SPACE)) {
	//	SceneManager* pSceneManager = (SceneManager*)FindObject("SceneManager");
	//	pSceneManager->ChangeScene(SCENE_ID_PLAY);
	//}
	//スペースキー押したら 
	// SceneManager::ChangeScene(SCENE_ID_PLAY); を呼び出してね
}

void TestScene::Draw()
{
}

void TestScene::Release()
{
}
