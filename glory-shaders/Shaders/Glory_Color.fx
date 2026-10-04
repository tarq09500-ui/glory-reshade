// Glory Color: سطوع، تباين، تشبع، vibrance، وتلوين
#include "Glory.fxh"

uniform float Brightness < ui_type = "slider"; ui_min = -0.5; ui_max = 0.5; ui_label = "Brightness"; > = 0.0;
uniform float Contrast   < ui_type = "slider"; ui_min = 0.5; ui_max = 2.0; ui_label = "Contrast"; > = 1.0;
uniform float Saturation < ui_type = "slider"; ui_min = 0.0; ui_max = 2.0; ui_label = "Saturation"; > = 1.0;
uniform float Vibrance   < ui_type = "slider"; ui_min = -1.0; ui_max = 1.0; ui_label = "Vibrance"; > = 0.0;
uniform float3 Tint      < ui_type = "color"; ui_label = "Tint"; > = float3(1.0, 1.0, 1.0);
uniform float TintAmount < ui_type = "slider"; ui_min = 0.0; ui_max = 1.0; ui_label = "Tint Amount"; > = 0.0;

float3 ColorPS(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
	float3 c = tex2D(Glory::BackBuffer, uv).rgb;
	c += Brightness;
	c = (c - 0.5) * Contrast + 0.5;
	float luma = dot(c, float3(0.2126, 0.7152, 0.0722));
	c = lerp(luma.xxx, c, Saturation);
	float mx = max(c.r, max(c.g, c.b));
	float mn = min(c.r, min(c.g, c.b));
	c = lerp(luma.xxx, c, 1.0 + Vibrance * (1.0 - (mx - mn)));
	c = lerp(c, c * Tint, TintAmount);
	return saturate(c);
}

technique Glory_Color
{
	pass { VertexShader = GloryVS; PixelShader = ColorPS; }
}
