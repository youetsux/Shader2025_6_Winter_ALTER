# Step 6 ── デバッグ UI とパラメータ調整

## 学習目標

- 「見えないものを可視化してデバッグする」習慣を身につける
- シャドウマップの内容を ImGui ウィンドウで直接確認する
- バイアス・解像度・正射影サイズのトレードオフを実際に触って体験する

---

## 理論：なぜデバッグ UI が重要なのか

シャドウマップは「深度値だけが入った目に見えないテクスチャ」です。  
影がおかしいとき、何が原因かを調べるには**中身を直接見る**のが最速です。

```
影が全面真っ黒  → バイアスが小さすぎる / 正射影サイズが小さすぎる
影が全くでない  → シャドウマップが更新されていない / UV変換のミス
影の輪郭がギザギザ → シャドウマップの解像度が低い / PCF未適用
影がシーン外にある → 正射影サイズが大きすぎて精度不足
```

これらをすべてソースコードの数値で試すのは非効率です。  
ImGui のスライダーでリアルタイムに調整できるようにしましょう。

---

## 変更内容

### 変更ファイル：`Stage.h`

調整パラメータをメンバ変数として追加します。

```cpp
// ========== シャドウ調整パラメータ ==========
float shadowBias_;       // シャドウアクネ防止バイアス
float lightOrthoSize_;   // 正射影の幅・高さ
bool  showShadowMap_;    // シャドウマップ可視化フラグ
// ============================================
```

`Stage::Stage()` コンストラクタで初期化します。

```cpp
shadowBias_     = 0.005f;
lightOrthoSize_ = 20.0f;
showShadowMap_  = false;
```

---

### 変更ファイル：`Direct3D.h`

`GetLightProjectionMatrix()` に外からサイズを渡せるよう変更します。

**変更前：**
```cpp
DirectX::XMMATRIX GetLightProjectionMatrix();
```

**変更後：**
```cpp
DirectX::XMMATRIX GetLightProjectionMatrix(float orthoSize = 20.0f);
```

---

### 変更ファイル：`Direct3D.cpp`

```cpp
DirectX::XMMATRIX Direct3D::GetLightProjectionMatrix(float orthoSize)
{
    return XMMatrixOrthographicLH(orthoSize, orthoSize, 1.0f, 50.0f);
}
```

---

### 変更ファイル：`Stage.cpp`

`Stage::Update()` でライト VP 行列を計算している箇所に `lightOrthoSize_` を渡します。

```cpp
XMMATRIX lightP = Direct3D::GetLightProjectionMatrix(lightOrthoSize_);
```

`Stage::Draw()` の ImGui 部分を以下に置き換えます。

```cpp
// ========================================
// ImGui デバッグ UI
// ========================================

// --- Light Type ボタン（既存） ---
ImGui::Separator();
ImGui::Text("=== Light Type ===");
if (ImGui::Button("Directional")) { lightType_ = 0; }
ImGui::SameLine();
if (ImGui::Button("Point"))       { lightType_ = 1; }
ImGui::SameLine();
ImGui::Text("Current: %s", lightType_ == 0 ? "Directional" : "Point");

// --- シャドウパラメータ調整 ---
ImGui::Separator();
ImGui::Text("=== Shadow Parameters ===");

ImGui::SliderFloat("Shadow Bias",      &shadowBias_,     0.0001f, 0.05f, "%.4f");
ImGui::SliderFloat("Light Ortho Size", &lightOrthoSize_, 5.0f,    50.0f, "%.1f");

// --- シャドウマップ可視化 ---
ImGui::Separator();
ImGui::Checkbox("Show ShadowMap", &showShadowMap_);
if (showShadowMap_)
{
    ID3D11ShaderResourceView* pSRV = Direct3D::GetShadowMapSRV();
    if (pSRV != nullptr)
    {
        ImGui::Text("Shadow Map (1024x1024):");
        // ImGui::Image でテクスチャを表示
        // キャストが必要（ImTextureID = void*）
        ImGui::Image((ImTextureID)pSRV, ImVec2(256, 256));
    }
    else
    {
        ImGui::Text("ShadowMap SRV: null");
    }
}

// --- ライト情報 ---
ImGui::Separator();
ImGui::Text("=== Light Information ===");
XMFLOAT4 lightPos = Direct3D::GetLightPos();
if (lightType_ == 0)
{
    ImGui::Text("Directional Light Direction:");
}
else
{
    ImGui::Text("Point Light Position:");
}
ImGui::Text("  X: %.2f, Y: %.2f, Z: %.2f", lightPos.x, lightPos.y, lightPos.z);
ImGui::Text("  Control: WASD + Up/Down");

ImGui::Separator();
```

また `shadowBias_` を HLSL に渡すために、`CONSTANTBUFFER_STAGE` に追加します。

**`Stage.h` の `CONSTANTBUFFER_STAGE` に追記：**
```cpp
struct CONSTANTBUFFER_STAGE
{
    XMFLOAT4   lightPosition;
    XMFLOAT4   eyePosition;
    int        lightType;
    float      shadowBias;  // ← 追加
    XMFLOAT2   _pad;        // _pad を float3 → float2 に変更
    XMFLOAT4X4 matLightVP;
};
```

**`Stage.cpp` の `Update()` で送信：**
```cpp
cb.shadowBias = shadowBias_;
```

**`Simple3D.hlsl` の `cbuffer gStage` に追記：**
```hlsl
cbuffer gStage : register(b1)
{
    float4             lightPosition;
    float4             eyePosition;
    int                lightType;
    float              shadowBias;  // ← 追加
    float2             _pad;        // float3 → float2 に変更
    row_major float4x4 matLightVP;
};
```

`Simple3D.hlsl` のシャドウ判定で `bias` の固定値を `shadowBias` に変えます。

```hlsl
// 変更前
float bias = 0.005;

// 変更後
float bias = shadowBias;
```

---

## 確認方法

### シャドウマップの可視化

1. ImGui の「Show ShadowMap」チェックボックスをオンにする
2. 256×256 のグレースケール画像が表示される
3. **白いほど遠い（ライトから遠い）、黒いほど近い**
4. モデルのシルエットが正しく写っているか確認する

### バイアスの調整

| 症状 | 原因 | 対処 |
|------|------|------|
| 縞模様の影（シャドウアクネ） | バイアスが小さすぎる | `Shadow Bias` を大きくする |
| 影が浮いて見える（ピーターパン現象） | バイアスが大きすぎる | `Shadow Bias` を小さくする |

### 正射影サイズの調整

| 症状 | 原因 | 対処 |
|------|------|------|
| シーン外に影が出ない | サイズが小さすぎる | `Light Ortho Size` を大きくする |
| 影がぼんやりしている | サイズが大きすぎて精度不足 | `Light Ortho Size` を小さくする |

---

## 発展課題

余裕があれば以下に挑戦してみましょう。

### PCF（Percentage Closer Filtering）の適用

`SampleCmpLevelZero` に複数のサンプル点を使って結果を平均することで、  
影のエッジを滑らかにできます。

```hlsl
// 9サンプルの PCF
float shadow = 0.0;
float texelSize = 1.0 / 1024.0; // シャドウマップの解像度に合わせる
for (int x = -1; x <= 1; x++)
{
    for (int y = -1; y <= 1; y++)
    {
        float2 offset = float2(x, y) * texelSize;
        shadow += g_shadowMap.SampleCmpLevelZero(
            g_shadowSampler, shadowUV + offset, currentDepth - bias);
    }
}
shadow /= 9.0; // 9サンプルの平均
```

### シャドウマップ解像度の変更

`Direct3D::InitShadowMap()` の引数を変えて解像度の違いを比較してみましょう。

| 解像度 | 品質 | VRAM使用量 |
|--------|------|-----------|
| 512×512 | 粗い | 1MB |
| 1024×1024 | 標準 | 4MB |
| 2048×2048 | きれい | 16MB |
| 4096×4096 | 非常にきれい | 64MB |

---

## まとめ：実装完了

おめでとうございます！デプスシャドウマップの実装が完了しました。

```
実装したもの：
・ライト視点のビュー行列・正射影行列の計算（Step1）
・シャドウマップ用テクスチャ（DSV + SRV）の作成（Step2）
・深度だけ書くシャドウシェーダー（Step3）
・2パス描画（シャドウパス + メインパス）（Step4）
・Simple3D.hlsl への影判定追加（Step5）
・ImGui によるリアルタイムパラメータ調整（Step6）
```

次のテーマとして以下があります。
- **点光源のシャドウマップ**（キューブシャドウマップ）
- **カスケードシャドウマップ**（遠景の影を高精度にする）
- **ソフトシャドウ**（PCSSなど）
