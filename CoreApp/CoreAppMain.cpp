#include "pch.h"
#include "CoreAppMain.h"

#include "Common\DirectXHelper.h"


using namespace CoreApp;
using namespace Windows::Foundation;
using namespace Windows::System::Threading;
using namespace Concurrency;
using namespace DirectX;
using namespace DirectX::SimpleMath;


// Loads and initializes application assets when the application is loaded.
CoreAppMain::CoreAppMain(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_deviceResources(deviceResources)
{
	// Register to be notified if the Device is lost or recreated
	m_deviceResources->RegisterDeviceNotify(this);

	CreateWindowSizeDependentResources();
	// TODO: Change the timer settings if you want something other than the default variable timestep mode.
	// e.g. for 60 FPS fixed timestep update logic, call:
	/*
	m_timer.SetFixedTimeStep(true);
	m_timer.SetTargetElapsedSeconds(1.0 / 60);
	*/
}

CoreAppMain::~CoreAppMain()
{
	// Deregister device notification
	m_deviceResources->RegisterDeviceNotify(nullptr);
}

// Updates application state when the window size changes (e.g. device orientation change)
void CoreAppMain::CreateWindowSizeDependentResources() 
{
	states = std::make_unique<CommonStates>(m_deviceResources->GetD3DDevice());
	factory = std::make_unique<EffectFactory>(m_deviceResources->GetD3DDevice());
	
	
	model = Model::CreateFromCMO(m_deviceResources->GetD3DDevice(), L"Z3.cmo", *factory );
	
	view = Matrix::CreatePerspectiveFieldOfView(XM_PI/4.f, m_deviceResources->GetOutputSize().Width/ m_deviceResources->GetOutputSize().Height,.1f, 200.1f);
	rotation = 0.0;
}



// Updates the application state once per frame.
void CoreAppMain::Update() 
{
	// Update scene objects.
	m_timer.Tick([&]()
	{
		// TODO: Replace this with your app's content update functions.
	
		rotation += 1.f * m_timer.GetElapsedSeconds();;
		world = Matrix::Identity * Matrix::CreateScale(.05f, .05f, .05f) * Matrix::CreateRotationY(rotation) * Matrix::CreateTranslation(0, -10, -30.f);

	});
}

// Renders the current frame according to the current application state.
// Returns true if the frame was rendered and is ready to be displayed.
bool CoreAppMain::Render() 
{
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
	{
		return false;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();

	// Reset the viewport to target the whole screen.
	auto viewport = m_deviceResources->GetScreenViewport();
	context->RSSetViewports(1, &viewport);

	// Reset render targets to the screen.
	ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
	context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());

	// Clear the back buffer and depth stencil view.
	context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), DirectX::Colors::CornflowerBlue);
	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	model->Draw(context, *states,world, view, proj);

	return true;
}

// Notifies renderers that device resources need to be released.
void CoreAppMain::OnDeviceLost()
{
	
}

// Notifies renderers that device resources may now be recreated.
void CoreAppMain::OnDeviceRestored()
{
	
	CreateWindowSizeDependentResources();
}
