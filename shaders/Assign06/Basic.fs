#version 430 core
// Change to 410 for macOS

layout(location=0) out vec4 out_color;
 
in vec4 vertexColor; // Now interpolated across face
in vec4 interPos;
in vec3 interNormal;

struct PointLight {
	vec4 pos;
	vec4 color;
};

uniform PointLight light;

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

    vec3 specularColor = vec3(light.color)*diffCoefficient*calculateSpecular(N, L, V, shininess, vec3(1.0));

    out_color = vec4(diffColor + specularColor, 1.0);
}