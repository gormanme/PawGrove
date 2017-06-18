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
    float4 position : POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
};

struct PINPUT
{
    float4 position : SV_POSITION;
    float3 normal : TEXCOORD0;
    float2 tex : TEXCOORD1;
};

//Albedo means surface color :)
Texture2D albedo : register(t0);

SamplerState samplerState : register(s0);

PINPUT VShader(VINPUT input)
{
    PINPUT output = (PINPUT)0;

    output.position = mul(input.position, world);
    output.position = mul(output.position, view);
    output.position = mul(output.position, projection);

    output.normal = mul(float4(input.normal, 1), world).xyz;

    output.tex = input.tex;

    return output;
}


float4 PShader(PINPUT input) : SV_TARGET
{
    float4 finalColor = 0;

    float4 surfaceColor = albedo.Sample(samplerState, input.tex);

    //Do NdotL lighting for light
    float ambient = 0.6;
    float nDotL = saturate(dot((float3) - lightDir, normalize(input.normal)));
    float diffuse = nDotL * (1 - ambient);
    finalColor += diffuse * lightColor * surfaceColor;
    finalColor += ambient * lightColor * surfaceColor;

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