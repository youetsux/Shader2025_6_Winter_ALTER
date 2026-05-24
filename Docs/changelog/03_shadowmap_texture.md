# 変更ログ：シャドウマップ用テクスチャ（DSV/SRV）を追加

## 目的
ライトから見たZ値を書き込む深度テクスチャと、シェーダーで読むためのSRVを作成する。

---

## 変更ファイル

### Engine/Direct3D.h

#### 追加した宣言
```cpp
HRESULT InitShadowMap(int width, int height);
ID3D11ShaderResourceView* GetShadowMapSRV();
```

### Engine/Direct3D.cpp

#### 追加した変数
```cpp
int screenWidth;
int screenHeight;
ID3D11Texture2D*          pShadowMapTexture;
ID3D11DepthStencilView*   pShadowMapDSV;
ID3D11ShaderResourceView* pShadowMapSRV;
```

#### Initialize() の変更
- 冒頭で `screenWidth` / `screenHeight` を保存
- `InitShader()` 後に `InitShadowMap(1024, 1024)` を呼ぶ

#### Release() の変更
- `SAFE_RELEASE(pShadowMapSRV)`
- `SAFE_RELEASE(pShadowMapDSV)`
- `SAFE_RELEASE(pShadowMapTexture)` を追加

#### InitShadowMap() の実装内容
| 項目 | 値 |
|---|---|
| Texture2D フォーマット | `DXGI_FORMAT_R32_TYPELESS` |
| BindFlags | `D3D11_BIND_DEPTH_STENCIL \| D3D11_BIND_SHADER_RESOURCE` |
| DSV フォーマット | `DXGI_FORMAT_D32_FLOAT` |
| SRV フォーマット | `DXGI_FORMAT_R32_FLOAT` |

---

## 元に戻す方法

```bash
git checkout HEAD~1 -- Engine/Direct3D.h Engine/Direct3D.cpp
```

---

## 対応するコミット

chapter3: シャドウマップ用テクスチャ・DSV・SRV を追加
