struct PSINPUT
{
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD0;
	float4 position : SV_POSITION;
	float4 worldpos : TEXCOORD1;
};

float4 main(PSINPUT In) : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}