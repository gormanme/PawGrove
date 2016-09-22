cbuffer ConstantBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix projection;
    float4 lightDir;
    float4 lightColor;
    float4 outputColor;
}

struct VINPUT
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
};

struct PINPUT
{
    float4 position : SV_POSITION;
    float3 normal : TEXCOORD0;
};

PINPUT VShader(VINPUT input)
{
    PINPUT output = (PINPUT)0;

    output.position = mul(input.position, world);
    output.position = mul(output.position, view);
    output.position = mul(output.position, projection);

   // output.normal = mul(float4(input.normal, 1), world).xyz;

    return output;
}


float4 PShader(PINPUT input) : SV_TARGET
{
    float4 finalColor = 0;

    //Do NdotL lighting for light
    finalColor += saturate(dot((float3)lightDir, input.normal) * lightColor);

    finalColor.a = 1;
    return finalColor;
}


//--------------------------------------------------------------------------------------
// PSSolid - render a solid color
//--------------------------------------------------------------------------------------
float4 PSSolid(PINPUT input) : SV_Target
{
    return outputColor;
}