cbuffer StaticMeshTransforms
{
	matrix WorldMatrix;
	matrix WorldViewProjMatrix;
};

cbuffer LightParameters
{

	float3 LightPositionWS;
	float4 LightColor;
};

struct VS_INPUT
{
	float3 position : POSITION;
	float2 tex :TEXCOORDS0;
	float3 normal :NORMAL;
};


struct VS_OUTPUT
{
	float4 position : SV_Position;
	float2 tex : TEXCOORDS0;
	float3 normal :NORMAL;
	float3 light :LIGHT;
};

VS_OUTPUT main( in VS_INPUT input ) 
{
	VS_OUTPUT output;
	
	//generate the clip space
	output.position = mul(float4(input.position, 1.0f), WorldViewProjMatrix);

	//generate the world space normal vector
	output.normal = mul(input.normal, (float3x3)WorldMatrix);

	//find world space position of vertex
	float3 PositionWS = mul(float4(input.position, 1.0f), WorldMatrix).xyz;


	//calculate the world space light vector
	output.light = LightPositionWS - PositionWS;
	
	//pass through the texture
	output.tex = input.tex;

	return output;
}