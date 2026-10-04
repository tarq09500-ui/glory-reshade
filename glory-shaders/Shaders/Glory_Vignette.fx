// Glory Vignette: تظليل أطراف الشاشة
#include "Glory.fxh"

uniform float Amount   < ui_type = "slider"; ui_min = 0.0; ui_max = 1.0; ui_label = "Amount"; > = 0.35;
uniform float Radius   < ui_type = "slider"; ui_min = 0.2; ui_max = 1.5; ui_label = "Radius"; > = 0.9;
uniform float Softness < ui_type = "slider"; ui_min = 0.05; ui_max = 1.0; ui_label = "Softness"; > = 0.5;

float3 VignettePS(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
	float3 c = tex2D(Glory::BackBuffer, uv).rgb;
	float2 d = (uv - 0.5) * float2(BUFFER_WIDTH * BUFFER_RCP_HEIGHT, 1.0);
	float v = smoothstep(Radius, Radius - Softness, length(d));
	return lerp(c * (1.0 - Amount), c, v);
}

technique Glory_Vignette
{
	pass { VertexShader = GloryVS; PixelShader = VignettePS; }
}
