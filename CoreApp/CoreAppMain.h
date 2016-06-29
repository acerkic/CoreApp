#pragma once

#include "Common\StepTimer.h"
#include "Common\DeviceResources.h"
#include "SimpleMath.h"
#include "Model.h"
#include "Effects.h"
#include "CommonStates.h"

// Renders Direct2D and 3D content on the screen.
namespace CoreApp
{
	class CoreAppMain : public DX::IDeviceNotify
	{
	public:
		CoreAppMain(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		~CoreAppMain();
		void CreateWindowSizeDependentResources();
		void Update();
		bool Render();

		// IDeviceNotify
		virtual void OnDeviceLost();
		virtual void OnDeviceRestored();

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// Rendering loop timer.
		DX::StepTimer m_timer;
		float rotation;
		std::unique_ptr<DirectX::Model> model;
		std::unique_ptr<DirectX::IEffectFactory> factory;
		std::unique_ptr<DirectX::CommonStates> states;

		DirectX::SimpleMath::Matrix view;
		DirectX::SimpleMath::Matrix proj;
		DirectX::SimpleMath::Matrix world;

		
	};
}