#version 430 core
// Change to 410 for macOS

layout(location=0) out vec4 out_color;
 
in vec4 vertexColor; // Now interpolated across face
in vec4 interPos;
in vec3 interNormal;

uniform float metallic;
uniform float roughness;
const float PI = 3.14159265359;

struct PointLight {
	vec4 pos;
	vec4 color;
};

uniform PointLight light;

vec3 getFresnelAtAngleZero(vec3 albedo, float metallic) {
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	return F0;
}

vec3 getFresnel(vec3 F0, vec3 L, vec3 H) {
	float cosAngle = max(0, dot(L, H));
	return F0 + (1 - F0) * pow((1 - max(0, cosAngle)), 5);
}

float getNDF(vec3 H, vec3 N, float roughness) {
	float a = pow(roughness, 2);
	return pow(a, 2) / (PI * pow(pow(dot(N, H), 2) * pow(a, 2) + 1, 2));
}

float getSchlickGeo(vec3 B, vec3 N, float roughness) {
	float k = pow(roughness + 1, 2) / 8;
	return dot(N, B) / (dot(N, B) * (1-k) + k);
}

float getGF(vec3 L, vec3 V, vec3 N, float roughness) {
	float GL = getSchlickGeo(L, N, roughness);
	float GV = getSchlickGeo(V, N, roughness);

	return GL * GV;
}

vec3 calculateSpecular(vec3 N, vec3 L, vec3 V, float shininess, vec3 specularColorOfSurface) {
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), shininess);
    return specularColorOfSurface * spec;
}

void main()
{	
	float shininess = 10.0;

	vec3 N = normalize(interNormal);
	vec3 L = normalize(vec3(light.pos) - vec3(interPos));
	float diffCoefficient = max(0, dot(N, L));
	vec3 diffColor = vec3(diffCoefficient * vertexColor * light.color);
    vec3 V = normalize(-vec3(interPos));

    vec3 specularColor = calculateSpecular(N, L, V, shininess, vec3(1.0));

	vec3 F0 = getFresnelAtAngleZero(vec3(vertexColor), metallic);
	vec3 H = normalize(L + V);
	vec3 F = getFresnel(F0, L, H);
	
	vec3 kS = F;
	vec3 kD = (1.0 - kS) * (1.0 - metallic) * vec3(vertexColor) / PI;

	float NDF = getNDF(H, N, roughness);
	float G = getGF(L, V, N, roughness);
	kS = kS * NDF * G;
	kS = kS / ((4.0 * max(0, dot(N,L)) * max(0, dot(N,V))) + 0.0001);

	vec3 finalColor = (kD + kS)*vec3(light.color)*max(0, dot(N,L));

	out_color = vec4(finalColor, 1.0);
}