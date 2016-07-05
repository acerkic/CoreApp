Texture2D ColorTexture : register(t0);
SamplerState LinearSampler : register(s0);


struct VS_OUTPUT
{
	float4 position: SV_Position;
	float2 tex : TEXCOORDS0;
	float3 normal : NORMAL;
	float3 light :LIGHT;
};



float4 main(in VS_OUTPUT input) : SV_TARGET
{
	//normalize world space and light vectors
	float3 n = normalize(input.normal);
	float3 l = normalize(input.light);


	//calculate the amount of light reaching this fragment
	float4 Illumination = max(dot(n, l), 0) + 0.2f;

	//determine the color properties of the texture 
	float4 SurfaceColor = ColorTexture.Sample(LinearSampler, input.tex);
	
	return float4(SurfaceColor * Illumination);
}