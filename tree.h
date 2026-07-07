#pragma once
#include "gameObject.h"
#include <d3d11.h>

struct MODEL;

// LOD0〜2を保持するモデルセット
struct TreeModel
{
    MODEL* lod[3] = { nullptr, nullptr, nullptr };
};

// 樹木の種類 × バリアント（Tree_05/08/10 × A/B/C = 9種類）
enum class TreeType
{
    Tree05A = 0,
    Tree05B = 1,
    Tree05C = 2,
    Tree08A = 3,
    Tree08B = 4,
    Tree08C = 5,
    Tree10A = 6,
    Tree10B = 7,
    Tree10C = 8,
    Count   = 9,
};

class Tree : public GameObject
{
private:
    //--------------------------------------------------------------------
    // 静的リソース（全インスタンス共有）
    //--------------------------------------------------------------------
    // 幹用シェーダー
    static ID3D11InputLayout*     m_VertexLayout;
    static ID3D11VertexShader*    m_VertexShader;
    static ID3D11PixelShader*     m_PixelShader;

    // 葉用シェーダー（風アニメーション + Alpha Clip）
    static ID3D11InputLayout*     m_LeafVertexLayout;
    static ID3D11VertexShader*    m_LeafVertexShader;
    static ID3D11PixelShader*     m_LeafPixelShader;

    // 風アニメーション用定数バッファ
    static ID3D11Buffer*          m_WindBuffer;

    // ラスタライザ
    static ID3D11RasterizerState* m_RsCullBack;
    static ID3D11RasterizerState* m_RsCullNone;

    static bool m_IsLoaded;

    // 9種類 × LOD0-2 のモデルセット
    static TreeModel m_TreeModels[(int)TreeType::Count];

    //--------------------------------------------------------------------
    // インスタンス固有
    //--------------------------------------------------------------------
    TreeType m_TreeType    = TreeType::Tree08A;
    float    m_Hp          = 100.0f;
    bool     m_IsDestroyed = false;

    //--------------------------------------------------------------------
    // 内部ヘルパー
    //--------------------------------------------------------------------
    bool IsLeafMaterial(const char* name) const;
    int  SelectLOD(float cameraDistSq) const;

public:
    void Init()           override;
    void Uninit()         override;
    void Update(float dt) override;
    void Draw()           override;
    void DrawShadow()     override;
    const char* GetName() override { return "Tree"; }

    void     SetTreeType(TreeType type) { m_TreeType = type; }
    TreeType GetTreeType() const        { return m_TreeType; }
    float    GetHp()       const        { return m_Hp; }
    bool     IsDestroyed() const        { return m_IsDestroyed; }

    void UpdateWind(float time);  // 風バッファ更新（Draw内部で自動呼び出し）
    void TakeDamage(float dmg);   // 将来：破壊処理

    static void ReleaseShaders();
};
