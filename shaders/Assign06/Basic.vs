#version 430 core
// Change to 410 for macOS

layout(location=0) in vec3 position;
layout(location=1) in vec4 color;
layout(location=2) in vec3 normal;

out vec4 vertexColor;
out vec4 interPos;
out vec3 interNormal;
uniform mat4 modelMat;
uniform mat4 viewMat;
uniform mat4 projMat;
uniform mat3 normMat;

void main()
{		
	// Get position of vertex (object space)
	vec4 objPos = vec4(position, 1.0);

	// For now, just pass along vertex position (no transformations)
	gl_Position = projMat * viewMat * modelMat * objPos;

	interPos = viewMat * modelMat * objPos;
	interNormal = normMat * normal;

	// Output per-vertex color
	vertexColor = color;
}
