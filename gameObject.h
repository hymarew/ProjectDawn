#pragma once
#include "main.h"
#include "vector3.h"
#include "component.h"

class GameObject
{
public: // 外部からアクセスしやすくするか、getter/setterを作ります
	bool m_IsActive = false; // ★これを追加（最初はfalseで休ませておく）
	bool m_Destroy = false;

	int m_Layer = 1;	//描画順番レイヤー番号
	//カメラは0
	//実態オブジェクトは1
	//透明なオブジェクトは2
	//Uiや2D描画は3

	float m_CameraZ;	//ソート用Z


protected:
	Vector3 m_Position{ 0.0f,0.0f,0.0f };
	Vector3 m_Rotation{ 0.0f,0.0f,0.0f };
	Vector3 m_Scale{ 1.0f,1.0f,1.0f };

	std::list<Component*> m_Components;


	enum AxisType
	{
		AXIS_RIGHT,
		AXIS_UP,
		AXIS_FORWARD
	};

public:
	virtual void Init() {};
	virtual void Uninit() 
	{
		for (Component* component : m_Components)	//範囲for文
		{
			component->Uninit();
			delete component;

		}
	};
	virtual void Update(float dt)
	{
		for (Component* component : m_Components)
		{
			component->Update(dt);
		}
	}
	virtual void Draw() 
	{
		for (Component* component : m_Components)	//範囲for文
		{
			component->Draw();
		}
	};

	virtual void DrawShadow()
	{
		for (Component* component : m_Components)	//範囲for文
		{
			component->DrawShadow();
		}
	};

	virtual const char* GetName() { return "GameObject"; }



	Vector3 GetPosition() const { return m_Position; }
	Vector3 GetScale()    const { return m_Scale; }
	Vector3 GetRotation() const { return m_Rotation; }

	void SetPosition(const Vector3& Position) { m_Position = Position; }
	void SetRotation(const Vector3& Rotation) { m_Rotation = Rotation; }
	void SetScale(const Vector3& Scale) { m_Scale = Scale; }

	void SetCamera(const Vector3& Position) {}
	void SetDestroy() { m_Destroy = true; }

	int GetLayer() { return m_Layer; }

	float GetCameraZ() const { return m_CameraZ; }
	void CalcCameraZ(Vector3 CameraPosition, Vector3 CameraForward)
	{
		Vector3 direction = m_Position - CameraPosition;
		m_CameraZ = Vector3::dot(direction, CameraForward); // 内積
	}


	bool Destroy()
	{
		if (m_Destroy)
		{
			Uninit();
			delete this;
			return true;
		}
		else
		{
			return false;
		}
	}

	template <typename T>	//テンプレート化(通常引数でクラスは送れないけど、リストとかテンプレートを組み合わせると行ける)
	T* AddComponent(GameObject* Object)
	{
		T* component = new T(Object);
		component->Init();
		m_Components.push_back(component);

		return component;
	}

	//インスタンシング用
	template <typename T>
	T* GetComponent()
	{
		for (Component* component : m_Components)
		{
			T* target = dynamic_cast<T*>(component);
			if (target != nullptr) return target;
		}
		return nullptr;
	}

	// 回転情報から指定軸方向のベクトルを取得
	// 0 = Right(右)
	// 1 = Up(上)
	// 2 = Forward(前)
	Vector3 GetAxis(int axis)
	{
		// 回転行列を生成
		XMMATRIX rot = XMMatrixRotationRollPitchYaw(
			m_Rotation.x,
			m_Rotation.y,
			m_Rotation.z);

		// 指定軸の方向ベクトルを取得
		Vector3 vec;
		XMStoreFloat3((XMFLOAT3*)&vec, rot.r[axis]);

		return vec;
	}
	virtual Vector3 GetRight() { return GetAxis(AXIS_RIGHT); }	// 右方向ベクトルを取得
	virtual Vector3 GetUp() { return GetAxis(AXIS_UP); }	// 上方向ベクトルを取得
	virtual Vector3 GetForward() { return GetAxis(AXIS_FORWARD); }	// 前方向ベクトルを取得


};