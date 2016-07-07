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
	CreateDeviceDependentResources();
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
	Size size = m_deviceResources->GetOutputSize();

	float aspectRatio = size.Width / size.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;
	if (aspectRatio < 1.0)
	{
		fovAngleY = 2.0f;
	}
	XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveRH(fovAngleY,aspectRatio, 0.1f, 1000);
	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();
	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);
	proj = projectionMatrix * orientationMatrix;

	world = Matrix::Identity * Matrix::CreateScale(.05f, .05f, .05f) * Matrix::CreateRotationY(rotation) * Matrix::CreateTranslation(0, -10, -30.f);

	XMStoreFloat4x4(&m_constantBufferData.WorldMatrix,world);

	static const XMVECTORF32 eye = {0.0f, 0.7f, 1.5f, 0.0f};
	static const XMVECTORF32 at = {0.0f, -.1f, 0.0f, 0.0f};
	static const XMVECTORF32 up = {0.0f, 1.0f, 0.0f, 0.0f};

	view = XMMatrixLookAtRH(eye, at, up);
	
	XMStoreFloat4x4(&m_constantBufferData.WorldViewProjMatrix, XMMatrixTranspose(world * view * proj));
}

void CoreAppMain::CreateDeviceDependentResources()
{
	factory = std::make_unique<EffectFactory>(m_deviceResources->GetD3DDevice());

	model = Model::CreateFromCMO(m_deviceResources->GetD3DDevice(), L"Z3.cmo", *factory);
	
	auto loadVSTask = DX::ReadDataAsync(L"demoVS.cso");
	auto loadPSTask = DX::ReadDataAsync(L"demoPS.cso");
	
	auto createVSTask = loadVSTask.then([this](const std::vector<byte>& filedata)
	{
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&filedata[0],
				filedata.size(),
				nullptr,
				&m_vertexShader
			)
		);

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				&filedata[0],
				filedata.size(),
				&m_inputLayout)
			);
		vsDone = true;
	});

		auto createPSTask = loadPSTask.then([this](const std::vector<byte>& filedata)
		{
			DX::ThrowIfFailed(
				m_deviceResources->GetD3DDevice()->CreatePixelShader(
					&filedata[0],
					filedata.size(),
					nullptr,
					&m_pixelShader
				)		
			);


			CD3D11_BUFFER_DESC constantBufferDesc(sizeof(StaticMeshTransforms), D3D11_BIND_CONSTANT_BUFFER);

			DX::ThrowIfFailed(
				m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBuffer
				)
			);

			psDone = true;
		});

	
		D3D11_SAMPLER_DESC samplerDesc = {};
	
		m_deviceResources->GetD3DDevice()->CreateSamplerState(&samplerDesc, m_sampler.GetAddressOf());
		
		

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
	if (!psDone && !vsDone)
		return true;

	world = Matrix::Identity * Matrix::CreateScale(.05f, .05f, .05f) * Matrix::CreateRotationY(rotation) * Matrix::CreateTranslation(0, -10, -20.f);
	XMStoreFloat4x4(&m_constantBufferData.WorldViewProjMatrix, XMMatrixTranspose(world * view * proj));
	XMStoreFloat4x4(&m_constantBufferData.WorldMatrix, world);
	
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
	
	context->UpdateSubresource1(
		m_constantBuffer.Get(),
		0,
		NULL,
		&m_constantBufferData,
		0,
		0,
		0
	);
	
	///Render meshes
	for (auto i = model->meshes.cbegin(); i != model->meshes.cend(); ++i)
	{
		ID3D11DepthStencilState* depthStencilState;
		ID3D11BlendState* blendState;
		
		//Every Mesh
		//Prep for rendering
		D3D11_BLEND_DESC desc = {};

		desc.RenderTarget[0].BlendEnable = D3D11_BLEND_ONE;
		desc.RenderTarget[0].SrcBlend = desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		desc.RenderTarget[0].DestBlend = desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		desc.RenderTarget[0].BlendOp = desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		m_deviceResources->GetD3DDevice()->CreateBlendState(&desc, &blendState);
		
		//Set blend state 
		context->OMSetBlendState(blendState, nullptr, 0xFFFFFFFF);
		
		D3D11_DEPTH_STENCIL_DESC Depthdesc = {};

		Depthdesc.DepthEnable = true;
		Depthdesc.DepthWriteMask =  D3D11_DEPTH_WRITE_MASK_ALL;
		Depthdesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

		Depthdesc.StencilEnable = false;
		Depthdesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
		Depthdesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

		Depthdesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		Depthdesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		Depthdesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		Depthdesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

		Depthdesc.BackFace = Depthdesc.FrontFace;
		 m_deviceResources->GetD3DDevice()->CreateDepthStencilState(&Depthdesc, &depthStencilState);
		
		//set the DepthStencil state
		context->OMSetDepthStencilState(depthStencilState, 0);
		///end of prep for Mesh rendering

		ID3D11RasterizerState *rast;
		D3D11_RASTERIZER_DESC cullDesc = {};

		cullDesc.CullMode = D3D11_CULL_BACK;
		cullDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
		cullDesc.DepthClipEnable = true;
		cullDesc.MultisampleEnable = true;

		m_deviceResources->GetD3DDevice()->CreateRasterizerState(&cullDesc, &rast);
		
		//Set Rasterizer State
		context->RSSetState(rast);
			
	/*	D3D11_SAMPLER_DESC samplerDesc = {};

		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

		samplerDesc.MaxAnisotropy = (m_deviceResources->GetD3DDevice()->GetFeatureLevel() > D3D_FEATURE_LEVEL_9_1) ? 16 : 2;

		samplerDesc.MaxLOD = FLT_MAX;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

		 m_deviceResources->GetD3DDevice()->CreateSamplerState(&samplerDesc, &m_sampler);*/
		// Set sampler state.
		
		//context->PSSetSamplers(0, 1, &m_sampler);

		ModelMesh* mesh = i->get();

		for (auto part = mesh->meshParts.cbegin(); part != mesh->meshParts.cend(); ++part)
		{
			
			//Every MeshPart
			ModelMeshPart * meshPart= (*part).get();

			context->IASetInputLayout(m_inputLayout.Get());

			context->IASetVertexBuffers(
				0,
				1,
				meshPart->vertexBuffer.GetAddressOf(),
				&meshPart->vertexStride,
				&meshPart->vertexOffset
			);

			context->IASetIndexBuffer(
				meshPart->indexBuffer.Get(),
				meshPart->indexFormat,
				meshPart->vertexOffset
			);

			//Get texture from somewhere
			/*ID3D11ShaderResourceView* textures[1] = { };

			deviceContext->PSSetShaderResources(0, 1, textures);*/
			
			context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
			context->PSSetShader(m_pixelShader.Get(), nullptr, 0);

			context->VSSetConstantBuffers1(0, 1, m_constantBuffer.GetAddressOf(), nullptr, 0);
			
			context->IASetPrimitiveTopology(meshPart->primitiveType);
			context->DrawIndexed(meshPart->indexCount, 0, 0);


			///use the meshpart to get the stuff to render here
			//			meshPart->Draw(m_deviceResources->GetD3DDeviceContext(), nullptr, m_inputLayout.Get());

		
		}
	}

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
