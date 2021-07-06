static const float PI = 3.141592;
static const float Epsilon = 0.00001;

float3 GammaCorrection(float3 color)
{
    float Gamma = 1.0f / 2.2f;

    float3 correctedColor;
    correctedColor = pow(color, float3(Gamma, Gamma, Gamma));

    return correctedColor;
}

float3 UnGammaCorrection(float3 color)
{
    float Gamma = 2.2f / 1.0f;

    float3 correctedColor;
    correctedColor = pow(color, float3(Gamma, Gamma, Gamma));

    return correctedColor;
}


// Shlick's approximation of the Fresnel factor.
float3 FresnelSchlick(float3 F0, float cosTheta)
{
	return F0 + (float3(1,1,1) - F0) * pow(1.0 - cosTheta, 5.0);
}

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
float NDFGGX(float cosLh, float roughness)
{
	float alpha = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;

	float result = alphaSq / (PI * denom * denom);

	if (isinf(result))
		result = 1.f;
	if (isnan(result))
		result = 0.f;
	return result;

	//float shininess = (1 - roughness)*256;
	//return (shininess + 8.0f) * pow(cosLh, shininess) / 8.0f;

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

float3 DirLightCalculation(float3 lightDir, float3 lightStrength,
    float3 lightOut, float3 normal, float cosLo, float3 F0, float3 albedo, float metallic, float roughness, float shadow)
{
	float3 Li = -lightDir;
	float3 Lradiance = lightStrength;

	//Li와 Lo사이의 하프벡터
	float3 Lh = normalize(Li + lightOut);

	//Normal과 다양한 빛 벡터 사이의 각을 계산한다.
	float cosLi = max(0.0, dot(normal, Li));
	float cosLh = max(0.0, dot(normal, Lh));

	//direct light에서의 fresnel 계산
	float3 F = FresnelSchlick(F0, max(0.0, dot(Lh, lightOut)));
	//Specular BRDF에서의 normal 분포도 계산
	float D = NDFGGX(cosLh, roughness);
	// Calculate geometric attenuation for specular BRDF.
	float G = GASchlickGGX(cosLi, cosLo, roughness);

	// Diffuse scattering happens due to light being refracted multiple times by a dielectric medium.
	// Metals on the other hand either reflect or absorb energy, so diffuse contribution is always zero.
	// To be energy conserving we must scale diffuse BRDF contribution based on Fresnel factor & metalness.
	float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metallic);

	// Lambert diffuse BRDF.
	// We don't scale by 1/PI for lighting & material units to be more convenient.
	// See: https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
	float3 diffuseBRDF = kd * albedo;

	// Cook-Torrance specular microfacet BRDF.
	float3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * cosLo);

	// Total contribution for this light.
	return (diffuseBRDF + specularBRDF) * Lradiance * cosLi * shadow;
}

float3 PointLightCalculation(float3 lightPos, float3 lightStrength, float falloffStart,
	float falloffEnd, float3 objectWorldCoord,
	float3 lightOut, float3 normal, float cosLo, float3 F0, float3 albedo, float metallic, float roughness)
{
	float3 fromObjectToPoint = lightPos - objectWorldCoord;
	float distance = length(fromObjectToPoint);

	if (distance > falloffEnd)
		return float3(0.f,0.f,0.f);

	float3 Li = normalize(fromObjectToPoint);
	float3 Lradiance = lightStrength;

	//Li와 Lo사이의 하프벡터
	float3 Lh = normalize(Li + lightOut);

	//Normal과 다양한 빛 벡터 사이의 각을 계산한다.
	float cosLi = max(0.0, dot(normal, Li));
	float cosLh = max(0.0, dot(normal, Lh));

	//direct light에서의 fresnel 계산
	float3 F = FresnelSchlick(F0, max(0.0, dot(Lh, lightOut)));
	//Specular BRDF에서의 normal 분포도 계산
	float D = NDFGGX(cosLh, roughness);
	// Calculate geometric attenuation for specular BRDF.
	float G = GASchlickGGX(cosLi, cosLo, roughness);

	// Diffuse scattering happens due to light being refracted multiple times by a dielectric medium.
	// Metals on the other hand either reflect or absorb energy, so diffuse contribution is always zero.
	// To be energy conserving we must scale diffuse BRDF contribution based on Fresnel factor & metalness.
	float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metallic);

	// Lambert diffuse BRDF.
	// We don't scale by 1/PI for lighting & material units to be more convenient.
	// See: https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
	float3 diffuseBRDF = kd * albedo;

	// Cook-Torrance specular microfacet BRDF.
	float3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * cosLo);

	float att = saturate((falloffEnd - distance)
		/ (falloffEnd - falloffStart));

	// Total contribution for this light.
	return (diffuseBRDF + specularBRDF) * Lradiance * cosLi * att;
}

float3 SpotLightCalculation(float3 lightPos, float3 lightDir, float3 lightStrength, float falloffStart,
	float falloffEnd, float spotPower,  float3 objectWorldCoord,
	float3 lightOut, float3 normal, float cosLo, float3 F0, float3 albedo, float metallic, float roughness)
{
	float3 fromObjectToPoint = lightPos - objectWorldCoord;
	float distance = length(fromObjectToPoint);

	if (distance > falloffEnd)
		return float3(0.f, 0.f, 0.f);

	float3 Li = normalize(fromObjectToPoint);
	float3 Lradiance = lightStrength;

	//Li와 Lo사이의 하프벡터
	float3 Lh = normalize(Li + lightOut);

	//Normal과 다양한 빛 벡터 사이의 각을 계산한다.
	float cosLi = max(0.0, dot(normal, Li));
	float cosLh = max(0.0, dot(normal, Lh));

	//direct light에서의 fresnel 계산
	float3 F = FresnelSchlick(F0, max(0.0, dot(Lh, lightOut)));
	//Specular BRDF에서의 normal 분포도 계산
	float D = NDFGGX(cosLh, roughness);
	// Calculate geometric attenuation for specular BRDF.
	float G = GASchlickGGX(cosLi, cosLo, roughness);

	// Diffuse scattering happens due to light being refracted multiple times by a dielectric medium.
	// Metals on the other hand either reflect or absorb energy, so diffuse contribution is always zero.
	// To be energy conserving we must scale diffuse BRDF contribution based on Fresnel factor & metalness.
	float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metallic);

	// Lambert diffuse BRDF.
	// We don't scale by 1/PI for lighting & material units to be more convenient.
	// See: https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
	float3 diffuseBRDF = kd * albedo;

	// Cook-Torrance specular microfacet BRDF.
	float3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * cosLo);

	float att = saturate((falloffEnd - distance)
		/ (falloffEnd - falloffStart));

	float spotFactor = pow(max(dot(-Li, lightDir), 0.0f),
		spotPower);

	// Total contribution for this light.
	return (diffuseBRDF + specularBRDF) * Lradiance * cosLi * att * spotFactor;
}