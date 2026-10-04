// Glory Sharpen: تحديد الصورة
#include "Glory.fxh"

uniform float Strength < ui_type = "slider"; ui_min = 0.0; ui_max = 2.0; ui_label = "Sharpen Strength"; > = 0.5;

float3 SharpenPS(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
	float2 px = BUFFER_PIXEL_SIZE;
	float3 c = tex2D(Glory::BackBuffer, uv).rgb;
	float3 blur = tex2D(Glory::BackBuffer, uv + float2(px.x, 0)).rgb
	            + tex2D(Glory::BackBuffer, uv - float2(px.x, 0)).rgb
	            + tex2D(Glory::BackBuffer, uv + float2(0, px.y)).rgb
	            + tex2D(Glory::BackBuffer, uv - float2(0, px.y)).rgb;
	blur *= 0.25;
	return saturate(c + (c - blur) * Strength);
}

technique Glory_Sharpen
{
	pass { VertexShader = GloryVS; PixelShader = SharpenPS; }
}
