#include"RasterizeRootSig.hlsl"

struct PixelIn
{
    float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3x3 TangentBasis : TBASIS;
    float2 TexC    : TEXCOORD;
    // nointerpolation is used so the index is not interpolated 
    // across the triangle.
    nointerpolation uint instID : INSTID;
};

static const float PI = 3.141592;
static const float Epsilon = 0.00001;
// Shlick's approximation of the Fresnel factor.
float3 FresnelSchlick(float3 F0, float cosTheta)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}


// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
float NDFGGX(float cosLh, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;

    float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
    return alphaSq / (PI * denom * denom);
}

// Single term for separable Schlick-GGX below.
float GASchlickG1(float cosTheta, float k)
{
    return cosTheta / (cosTheta * (1.0 - k) + k);
}


// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float GASchlickGGX(float cosLi, float cosLo, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
    return GASchlickG1(cosLi, k) * GASchlickG1(cosLo, k);
}

float4 PS(PixelIn pin) : SV_Target
{

    InstanceData instData = gInstanceData[pin.instID];

    float LerpValue = instData.LerpValue;
    LerpValue = LerpValue * LerpValue * LerpValue;
    float3 LearValueVector3 = float3(LerpValue, LerpValue, LerpValue);
    

    float3 Albedo0 = Textures[instData.MatIndex0].Sample(gsamLinearWrap, pin.TexC);
    float3 Albedo1 = Textures[instData.MatIndex1].Sample(gsamLinearWrap, pin.TexC);
    float3 Albedo = lerp(Albedo0, Albedo1, LearValueVector3);

    float Roughness0 = Textures[instData.MatIndex0 + 1].Sample(gsamLinearWrap, pin.TexC).x;
    float Roughness1 = Textures[instData.MatIndex1 + 1].Sample(gsamLinearWrap, pin.TexC).x;
    float Roughness = lerp(Roughness0, Roughness1, LerpValue);

    float Metallic0 = Textures[instData.MatIndex0 + 2].Sample(gsamLinearWrap, pin.TexC).x;
    float Metallic1 = Textures[instData.MatIndex1 + 2].Sample(gsamLinearWrap, pin.TexC).x;
    float Metallic = lerp(Metallic0, Metallic1, LerpValue);

    float Ao0 = Textures[instData.MatIndex0 + 3].Sample(gsamLinearWrap, pin.TexC).x;
    float Ao1 = Textures[instData.MatIndex1 + 3].Sample(gsamLinearWrap, pin.TexC).x;
    float Ao = lerp(Ao0, Ao1, LerpValue);

    float3 N0 = Textures[instData.MatIndex0 + 4].Sample(gsamLinearWrap, pin.TexC);
    float3 N1 = Textures[instData.MatIndex1 + 4].Sample(gsamLinearWrap, pin.TexC);
    float3 N = lerp(N0, N1, LearValueVector3);


    //Outgoing light direction(월드 스페이스 픽셀에서 눈으로의 벡터)
    float3 Lo = normalize(gEyePosW - pin.PosW);

    //Normal(픽셀에서의 노말)
    N = normalize(2.0 * N - 1.0);
    N = normalize(mul(N, pin.TangentBasis));

    //표면의 Normal과 Outgoing light의 각도
    float cosLo = max(0.0, dot(N, Lo));

    //Specular reflection vector(아직 용도는 잘 모르겠지만, Normal과 Lo를 통해서 스펙큘러가 가장 높게 나오는 빛의 벡터를 구한 것 같다
    float3 Lr = 2.0 * cosLo * N - Lo;

    //Frensnel reflentance at normal incidence(for metals use albedo color)
    //노말과 동일한 빛이 입사할 때 나오는 프레넬 반사
    float3 Fdielectric = float3(0.04, 0.04, 0.04);//유전체
    float3 F0 = lerp(Fdielectric, Albedo, Metallic);

    float3 directLighting = 0.0;
    for(uint i = 0; i< NUM_DIR_LIGHTS;i++)
    {
        float3 Li = -gLights[i].Direction;
        float3 Lradiance = gLights[i].Strength;
       
        //Li와 Lo사이의 하프벡터
        float3 Lh = normalize(Li + Lo);

        //Normal과 다양한 빛 벡터 사이의 각을 계산한다.
        float cosLi = max(0.0, dot(N, Li));
        float cosLh = max(0.0, dot(N, Lh));

        //direct light에서의 fresnel 계산
        float3 F = FresnelSchlick(F0, max(0.0, dot(Lh, Lo)));
        //Specular BRDF에서의 normal 분포도 계산
        float D = NDFGGX(cosLh, Roughness);
        // Calculate geometric attenuation for specular BRDF.
        float G = GASchlickGGX(cosLi, cosLo, Roughness);

        // Diffuse scattering happens due to light being refracted multiple times by a dielectric medium.
        // Metals on the other hand either reflect or absorb energy, so diffuse contribution is always zero.
        // To be energy conserving we must scale diffuse BRDF contribution based on Fresnel factor & metalness.
        float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), Metallic);

        // Lambert diffuse BRDF.
        // We don't scale by 1/PI for lighting & material units to be more convenient.
        // See: https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
        float3 diffuseBRDF = kd * Albedo;

        // Cook-Torrance specular microfacet BRDF.
        float3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * cosLo);

        // Total contribution for this light.
        directLighting += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
    }

    float3 Ambient = float3(0.03f, 0.03f, 0.03f) * Albedo * Ao;


    float3 color = directLighting + Ambient;

    //float Gamma = 1.0f / 2.2f;
    //color = color / (color + float3(1.0f, 1.0f, 1.0f));
    //color = pow(color, float3(Gamma, Gamma, Gamma));
    
    return float4(color, 1.0f);


   
}


