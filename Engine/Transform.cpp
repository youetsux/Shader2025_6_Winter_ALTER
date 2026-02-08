#include "Transform.h"

Transform::Transform()
	:matTranslate_(XMMatrixIdentity()),
	matRotate_(XMMatrixIdentity()),
	matScale_(XMMatrixIdentity()),
	pParent_(nullptr)
{
	position_ = XMFLOAT3(0.0f, 0.0f, 0.0f);
	rotate_ = XMFLOAT3(0.0f, 0.0f, 0.0f);
	scale_ = XMFLOAT3(1.0f, 1.0f, 1.0f);
}

Transform::~Transform()
{

}

void Transform::Calculation()
{
	matTranslate_ = XMMatrixTranslation(position_.x, position_.y, position_.z);
	matRotate_ = XMMatrixRotationRollPitchYaw(XMConvertToRadians(rotate_.x),
		XMConvertToRadians(rotate_.y),
		XMConvertToRadians(rotate_.z));
	matScale_ = XMMatrixScaling(scale_.x, scale_.y, scale_.z);
}

XMMATRIX Transform::GetWorldMatrix()
{
	if (pParent_ != nullptr)
	{
		return matScale_ * matRotate_ * matTranslate_ * pParent_->GetWorldMatrix();
	}
	else
		return matScale_ * matRotate_ * matTranslate_;
}

XMMATRIX Transform::GetNormalMatrix()
{
	// World の逆転置(平行移動成分の無視）
	XMMATRIX world = GetWorldMatrix();
	XMMATRIX invWorld = XMMatrixInverse(nullptr, world);
	return XMMatrixTranspose(invWorld);
}
