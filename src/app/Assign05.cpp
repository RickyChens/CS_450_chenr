#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <GL/glew.h>					
#include <GLFW/glfw3.h>
#include "glm/glm.hpp"
#include "MeshData.hpp"
#include "MeshGLData.hpp"
#include "GLSetup.hpp"
#include "Shader.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "glm/gtc/matrix_transform.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/transform.hpp"
#include "glm/gtx/string_cast.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "Utility.hpp"

using namespace std;

float rotAngle = 0.0f;
glm::vec3 eye = glm::vec3(0, 0, 1);
glm::vec3 lookAt = glm::vec3(0, 0, 0);
glm::vec2 mousePos;

glm::mat4 makeLocalRotate(glm::vec3 offset, glm::vec3 axis, float angle){
	glm::mat4 modelMat(1.0);

	modelMat = glm::translate(glm::vec3(offset)) * 
			   glm::rotate(glm::radians(angle), axis) * 
			   glm::translate(glm::vec3(-offset)) * modelMat;

	return modelMat;
}

 static void mouse_position_callback(GLFWwindow* window, double xpos, double ypos) {
	//Getting relative pos
	glm::vec2 relMouse = mousePos - glm::vec2(xpos, ypos);

	// Getting window width and height
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	if (width > 0 && height > 0) {
		glm::vec2 scaledRelativeMouse = glm::vec2((double)relMouse.x / width, (double)relMouse.y / height);

		glm::mat4 relX = makeLocalRotate(eye, glm::vec3(0, 1, 0), 30.0f * scaledRelativeMouse.x);
		lookAt = glm::vec3(relX * glm::vec4(lookAt, 1.0));

		glm::mat4 relY = makeLocalRotate(eye, glm::cross(lookAt-eye, glm::vec3(0, 1, 0)), 30.0f * scaledRelativeMouse.y);
		lookAt = glm::vec3(relY * glm::vec4(lookAt, 1.0));
	}

	mousePos = glm::vec2(xpos, ypos);
 }


glm::mat4 makeRotateZ(glm::vec3 offset) {
	glm::mat4 modelMat(1.0);
	modelMat = glm::translate(glm::vec3(offset)) * 
			   glm::rotate(glm::radians(rotAngle), glm::vec3(0,0,1)) * 
			   glm::translate(glm::vec3(-offset)) * modelMat;

	return modelMat;
}

void renderScene(vector<MeshGL> &allMeshes, aiNode *node,
				glm::mat4 parentMat, GLint modelMatLoc, int level) {
	
	aiMatrix4x4 mat = node->mTransformation;
	glm::mat4 nodeT;
	aiMatToGLM4(mat, nodeT);
	glm::mat4 modelMat = parentMat*nodeT;

	glm::vec3 pos = glm::vec3(modelMat[3].x, modelMat[3].y, modelMat[3].z);
	glm::mat4 R = makeRotateZ(pos);
	glm::mat4 tmpModel = R * modelMat;

	glUniformMatrix4fv(modelMatLoc, 1, GL_FALSE, glm::value_ptr(tmpModel));

	for (int i = 0; i < node->mNumMeshes; i++) {
		int index = node->mMeshes[i];
		drawMesh(allMeshes.at(index));
	}

	for (int i = 0; i < node->mNumChildren; i++) {
		renderScene(allMeshes, node->mChildren[i], modelMat, modelMatLoc, level + 1);
	}
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_ESCAPE) {
        	glfwSetWindowShouldClose(window, GL_TRUE);
		}	
		else if (key == GLFW_KEY_J) {
			rotAngle += 1.0f;
		}
		else if (key == GLFW_KEY_K) {
			rotAngle -= 1.0f;
		}
		else if (key == GLFW_KEY_W) {
			glm::vec3 direction = glm::normalize(lookAt-eye);
			eye += direction * 0.1f;
			lookAt += direction * 0.1f;
		}
		else if (key == GLFW_KEY_S) {
			glm::vec3 direction = glm::normalize(lookAt-eye);
			eye += direction * -0.1f;
			lookAt += direction * -0.1f;
		}
		else if (key == GLFW_KEY_D) {
			eye += glm::normalize(glm::cross(lookAt - eye, glm::vec3(0, 1, 0))) * 0.1f;
			lookAt += glm::normalize(glm::cross(lookAt - eye, glm::vec3(0, 1, 0))) * 0.1f;
		}
		else if (key == GLFW_KEY_A) {
			eye += glm::normalize(glm::cross(lookAt - eye, glm::vec3(0, 1, 0))) * -0.1f;
			lookAt += glm::normalize(glm::cross(lookAt - eye, glm::vec3(0, 1, 0))) * -0.1f;
		}
    }
}

void extractMeshData(aiMesh *mesh, Mesh &m) {
	m.vertices.clear();
	m.indices.clear();

	for (int i = 0; i < mesh->mNumVertices; i++) {
		Vertex vertex;

		vertex.position = glm::vec3(mesh->mVertices[i].x , mesh->mVertices[i].y , mesh->mVertices[i].z);
		vertex.color = glm::vec4(1.0, 0.0, 0.0, 1.0);

		m.vertices.push_back(vertex);
	}
	for (int i = 0; i < mesh->mNumFaces; i++) {
			aiFace face = mesh->mFaces[i];

			for (int k = 0; k < face.mNumIndices; k++) {
				m.indices.push_back(face.mIndices[k]);
			}
		}
}

// Create very simple mesh: a quad (4 vertices, 6 indices, 2 triangles)
void createSimpleQuad(Mesh &m) {
	// Clear out vertices and elements
	m.vertices.clear();
	m.indices.clear();

	// Create four corners
	Vertex upperLeft, upperRight; 
	Vertex lowerLeft, lowerRight;

	// Set positions of vertices
	// Note: glm::vec3(x, y, z)
	upperLeft.position = glm::vec3(-0.5, 0.5, 0.0);
	upperRight.position = glm::vec3(0.5, 0.5, 0.0);
	lowerLeft.position = glm::vec3(-0.5, -0.5, 0.0);
	lowerRight.position = glm::vec3(0.5, -0.5, 0.0);

	// Set vertex colors (red, green, blue, white)
	// Note: glm::vec4(red, green, blue, alpha)
	upperLeft.color = glm::vec4(1.0, 0.0, 0.0, 1.0);
	upperRight.color = glm::vec4(0.0, 1.0, 0.0, 1.0);
	lowerLeft.color = glm::vec4(0.0, 0.0, 1.0, 1.0);
	lowerRight.color = glm::vec4(1.0, 1.0, 1.0, 1.0);

	// Add to mesh's list of vertices
	m.vertices.push_back(upperLeft);
	m.vertices.push_back(upperRight);	
	m.vertices.push_back(lowerLeft);
	m.vertices.push_back(lowerRight);
	
	// Add indices for two triangles
	m.indices.push_back(0);
	m.indices.push_back(3);
	m.indices.push_back(1);

	m.indices.push_back(0);
	m.indices.push_back(2);
	m.indices.push_back(3);
}

void createSimplePentagon(Mesh &m) {
	// Clear out vertices and elements
	m.vertices.clear();
	m.indices.clear();

	// Create four corners
	Vertex upperLeft, upperRight;
	Vertex lowerLeft, lowerRight;
	Vertex farLeft;

	// Set positions of vertices
	// Note: glm::vec3(x, y, z)
	farLeft.position = glm::vec3(-0.75, 0.0, 0.0);
	upperLeft.position = glm::vec3(-0.5, 0.5, 0.0);
	upperRight.position = glm::vec3(0.5, 0.5, 0.0);
	lowerLeft.position = glm::vec3(-0.5, -0.5, 0.0);
	lowerRight.position = glm::vec3(0.5, -0.5, 0.0);

	// Set vertex colors (red, green, blue, white)
	// Note: glm::vec4(red, green, blue, alpha)
	farLeft.color = glm::vec4(1.0, 1.0, 0.0, 1.0);
	upperLeft.color = glm::vec4(1.0, 0.0, 0.0, 1.0);
	upperRight.color = glm::vec4(0.0, 1.0, 0.0, 1.0);
	lowerLeft.color = glm::vec4(0.0, 0.0, 1.0, 1.0);
	lowerRight.color = glm::vec4(1.0, 1.0, 1.0, 1.0);

	// Add to mesh's list of vertices
	m.vertices.push_back(upperLeft);
	m.vertices.push_back(upperRight);	
	m.vertices.push_back(lowerLeft);
	m.vertices.push_back(lowerRight);
	m.vertices.push_back(farLeft);
	
	// Add indices for two triangles
	m.indices.push_back(0);
	m.indices.push_back(3);
	m.indices.push_back(1);

	m.indices.push_back(0);
	m.indices.push_back(2);
	m.indices.push_back(3);

	m.indices.push_back(0);
	m.indices.push_back(4);
	m.indices.push_back(2);
}

// Main 
int main(int argc, char **argv) {
	// Are we in debugging mode?
	bool DEBUG_MODE = true;

	// GLFW setup
	// Switch to 4.1 if necessary for macOS
	GLFWwindow* window = setupGLFW("Assign05: chenr", 4, 3, 800, 800, DEBUG_MODE);

	// GLEW setup
	setupGLEW(window);

	// Check OpenGL version
	checkOpenGLVersion();

	// Set up debugging (if requested)
	if(DEBUG_MODE) checkAndSetupOpenGLDebugging();

	// Set the background color to a shade of blue
	glClearColor(0.5f, 0.9f, 0.7f, 1.0f);	

	// Setting Key Callback
	glfwSetKeyCallback(window, key_callback);

	// get mouse pos
	double mx, my;
	glfwGetCursorPos(window, &mx, &my);
	mousePos = glm::vec2(mx, my);

	glfwSetCursorPosCallback(window, mouse_position_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Create and load shaders
	GLuint programID = 0;
	try {		
		// Load vertex shader code and fragment shader code
		string vertexCode = readFileToString("./shaders/Assign05/Basic.vs");
		string fragCode = readFileToString("./shaders/Assign05/Basic.fs");

		// Print out shader code, just to check
		if(DEBUG_MODE) printShaderCode(vertexCode, fragCode);

		// Create shader program from code
		programID = initShaderProgramFromSource(vertexCode, fragCode);
	}
	catch (exception e) {		
		// Close program
		cleanupGLFW(window);
		exit(EXIT_FAILURE);
	}

	string modelPath = "sampleModels/bunnyteatime.glb";
	// Creating importer and grabbing command line
	Assimp::Importer importer;
	if (argc >= 2) {
        modelPath = argv[1];
    }

	const aiScene *scene = importer.ReadFile(modelPath, aiProcess_Triangulate | aiProcess_FlipUVs |
											  			 aiProcess_GenNormals | aiProcess_JoinIdenticalVertices);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		cerr << "Error: " << importer.GetErrorString() << endl;
		exit(1);
	}
	
	vector<MeshGL> meshGLVector;
	for (int i = 0; i < scene->mNumMeshes; i++) {
		Mesh mesh;
		MeshGL meshGL;

		extractMeshData(scene->mMeshes[i], mesh);
		createMeshGL(mesh, meshGL);
		meshGLVector.push_back(meshGL);
	}

	/*
	// Create simple quad
	Mesh m;
	createSimplePentagon(m);

	// Create OpenGL mesh (VAO) from data
	MeshGL mgl;
	createMeshGL(m, mgl);
	*/

	// Enable depth testing
	glEnable(GL_DEPTH_TEST);

	GLint modelMatLoc = glGetUniformLocation(programID, "modelMat");
	GLint viewMatLoc = glGetUniformLocation(programID, "viewMat");
	GLint projMatLoc = glGetUniformLocation(programID, "projMat");

	while (!glfwWindowShouldClose(window)) {
		// Set viewport size
		int fwidth, fheight;
		glfwGetFramebufferSize(window, &fwidth, &fheight);
		glViewport(0, 0, fwidth, fheight);

		// Clear the framebuffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use shader program
		glUseProgram(programID);

		glm::mat4 viewMat = glm::lookAt(eye, lookAt, glm::vec3(0,1,0));
		glUniformMatrix4fv(viewMatLoc, 1, GL_FALSE, glm::value_ptr(viewMat));

		float aspectRatio = (fheight > 0) ? (float)fwidth / (float)fheight : 1.0f;

		glm::mat4 projMat = glm::perspective(glm::radians(90.0f), aspectRatio, 0.01f, 50.0f);
		glUniformMatrix4fv(projMatLoc, 1, GL_FALSE, glm::value_ptr(projMat));

		// Draw object
		//drawMesh(mgl);
		for (int i = 0; i < meshGLVector.size(); i++) {
			renderScene(meshGLVector, scene->mRootNode, glm::mat4(1.0), modelMatLoc, 0);
		}

		// Swap buffers and poll for window events		
		glfwSwapBuffers(window);
		glfwPollEvents();

		// Sleep for 15 ms
		this_thread::sleep_for(chrono::milliseconds(15));
	}

	// Clean up mesh
	//cleanupMesh(mgl);

	for (int i = 0; i < meshGLVector.size(); i++) {
			cleanupMesh(meshGLVector[i]);
	}
	meshGLVector.clear();

	// Clean up shader programs
	glUseProgram(0);
	glDeleteProgram(programID);
		
	// Destroy window and stop GLFW
	cleanupGLFW(window);

	return 0;
}
