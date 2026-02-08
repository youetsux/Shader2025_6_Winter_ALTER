#pragma once

//超簡単球体コライダークラス
class SphereCollider
{
public:
	SphereCollider(float radius);
	float GetRadius() { return (radius_); } //インライン定義
private:
	float radius_;//半径
};