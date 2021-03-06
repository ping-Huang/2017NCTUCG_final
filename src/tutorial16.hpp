#ifndef T16
#define T16

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>

#include "../include/GL/glew.h"
#include "../include/GL/freeglut.h"
#include "../include/glm/glm.hpp"
#include "../include/glm/gtc/matrix_transform.hpp"

struct PackedVertex {
	glm::vec3 position;
	glm::vec2 uv;
	glm::vec3 normal;
	bool operator<(const PackedVertex that) const {
		return memcmp((void*)this, (void*)&that, sizeof(PackedVertex))>0;
	};
};

class tutorial16
{
	public:

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;

	std::vector<unsigned short> indices;
	std::vector<glm::vec3> indexed_vertices;
	std::vector<glm::vec2> indexed_uvs;
	std::vector<glm::vec3> indexed_normals;

	// Constructor
	tutorial16() {}
	
	bool loadOBJ(const char * path, std::vector<glm::vec3> & out_vertices, std::vector<glm::vec2> & out_uvs, std::vector<glm::vec3> & out_normals)
	{
		printf("Loading OBJ file %s...\n", path);

		std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
		std::vector<glm::vec3> temp_vertices;
		std::vector<glm::vec2> temp_uvs;
		std::vector<glm::vec3> temp_normals;


		FILE * file = fopen(path, "r");
		if (file == NULL) {
			printf("Impossible to open the file ! Are you in the right path ? See Tutorial 1 for details\n");
			getchar();
			return false;
		}

		while (1) {

			char lineHeader[128];
			// read the first word of the line
			int res = fscanf(file, "%s", lineHeader);
			if (res == EOF)
				break; // EOF = End Of File. Quit the loop.

					   // else : parse lineHeader

			if (strcmp(lineHeader, "v") == 0) {
				glm::vec3 vertex;
				fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
				temp_vertices.push_back(vertex);
			}
			else if (strcmp(lineHeader, "vt") == 0) {
				glm::vec2 uv;
				fscanf(file, "%f %f\n", &uv.x, &uv.y);
				uv.y = -uv.y; // Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you want to use TGA or BMP loaders.
				temp_uvs.push_back(uv);
			}
			else if (strcmp(lineHeader, "vn") == 0) {
				glm::vec3 normal;
				fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
				temp_normals.push_back(normal);
			}
			else if (strcmp(lineHeader, "f") == 0) {
				std::string vertex1, vertex2, vertex3;
				unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
				int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
				if (matches != 9) {
					printf("File can't be read by our simple parser :-( Try exporting with other options\n");
					fclose(file);
					return false;
				}
				vertexIndices.push_back(vertexIndex[0]);
				vertexIndices.push_back(vertexIndex[1]);
				vertexIndices.push_back(vertexIndex[2]);
				uvIndices.push_back(uvIndex[0]);
				uvIndices.push_back(uvIndex[1]);
				uvIndices.push_back(uvIndex[2]);
				normalIndices.push_back(normalIndex[0]);
				normalIndices.push_back(normalIndex[1]);
				normalIndices.push_back(normalIndex[2]);
			}
			else {
				// Probably a comment, eat up the rest of the line
				char stupidBuffer[1000];
				fgets(stupidBuffer, 1000, file);
			}

		}

		// For each vertex of each triangle
		for (unsigned int i = 0; i<vertexIndices.size(); i++) {

			// Get the indices of its attributes
			unsigned int vertexIndex = vertexIndices[i];
			unsigned int uvIndex = uvIndices[i];
			unsigned int normalIndex = normalIndices[i];

			// Get the attributes thanks to the index
			glm::vec3 vertex = temp_vertices[vertexIndex - 1];
			glm::vec2 uv = temp_uvs[uvIndex - 1];
			glm::vec3 normal = temp_normals[normalIndex - 1];

			// Put the attributes in buffers
			out_vertices.push_back(vertex);
			out_uvs.push_back(uv);
			out_normals.push_back(normal);

		}
		fclose(file);
		return true;
	}
	
	bool getSimilarVertexIndex_fast( PackedVertex & packed, std::map<PackedVertex, unsigned short> & VertexToOutIndex, unsigned short & result ) 
	{
		std::map<PackedVertex, unsigned short>::iterator it = VertexToOutIndex.find(packed);
		if (it == VertexToOutIndex.end()) {
			return false;
		}
		else {
			result = it->second;
			return true;
		}
	}

	void indexVBO(	std::vector<glm::vec3> & in_vertices,	std::vector<glm::vec2> & in_uvs,	std::vector<glm::vec3> & in_normals,	std::vector<unsigned short> & out_indices,	std::vector<glm::vec3> & out_vertices,	std::vector<glm::vec2> & out_uvs,	std::vector<glm::vec3> & out_normals)
	{
		std::map<PackedVertex, unsigned short> VertexToOutIndex;

		// For each input vertex
		for (unsigned int i = 0; i<in_vertices.size(); i++) {

			PackedVertex packed = { in_vertices[i], in_uvs[i], in_normals[i] };


			// Try to find a similar vertex in out_XXXX
			unsigned short index;
			bool found = getSimilarVertexIndex_fast(packed, VertexToOutIndex, index);

			if (found) { // A similar vertex is already in the VBO, use it instead !
				out_indices.push_back(index);
			}
			else { // If not, it needs to be added in the output data.
				out_vertices.push_back(in_vertices[i]);
				out_uvs.push_back(in_uvs[i]);
				out_normals.push_back(in_normals[i]);
				unsigned short newindex = (unsigned short)out_vertices.size() - 1;
				out_indices.push_back(newindex);
				VertexToOutIndex[packed] = newindex;
			}
		}
	}

	void RenderShadowMap16(GLuint depthBuffer, GLuint depthProgram)
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK); // Cull back-facing triangles -> draw only front-facing triangles
		glBindFramebuffer(GL_FRAMEBUFFER, depthBuffer);
		//glBindFramebuffer(GL_FRAMEBUFFER, NULL);
		//glViewport(0, 0, 512, 512);

		//glEnable(GL_CULL_FACE);
		//glCullFace(GL_BACK); // Cull back-facing triangles -> draw only front-facing triangles

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(depthProgram);

		glm::vec3 lightInvDir = glm::vec3(0.5f, 2, 2);

		// Compute the MVP matrix from the light's point of view
		glm::mat4 depthProjectionMatrix = glm::ortho<float>(-10, 10, -10, 10, -10, 20);
		glm::mat4 depthViewMatrix = glm::lookAt(lightInvDir, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		// or, for spot light :
		/*glm::vec3 lightPos(5, 20, 20);
		glm::mat4 depthProjectionMatrix = glm::perspective<float>(45.0f, 1.0f, 2.0f, 50.0f);
		glm::mat4 depthViewMatrix = glm::lookAt(lightPos, lightPos-lightInvDir, glm::vec3(0,1,0));*/

		indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);


		GLuint vertexbuffer;
		glGenBuffers(1, &vertexbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3), &indexed_vertices[0], GL_STATIC_DRAW);

		GLuint uvbuffer;
		glGenBuffers(1, &uvbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size() * sizeof(glm::vec2), &indexed_uvs[0], GL_STATIC_DRAW);

		GLuint normalbuffer;
		glGenBuffers(1, &normalbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		glBufferData(GL_ARRAY_BUFFER, indexed_normals.size() * sizeof(glm::vec3), &indexed_normals[0], GL_STATIC_DRAW);

		// Generate a buffer for the indices as well
		GLuint elementbuffer;
		glGenBuffers(1, &elementbuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);

		glm::mat4 depthModelMatrix = glm::mat4(1.0);
		glm::mat4 depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(glGetUniformLocation(depthProgram, "MVP"), 1, GL_FALSE, &depthMVP[0][0]);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,  // The attribute we want to configure
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
		);

		//glm::vec3 Camera_Pos;
		//Camera_Pos[0] = cameraDistance * glm::sin(glm::radians(cameraPitch)) * glm::cos(glm::radians(cameraYaw));
		//Camera_Pos[1] = cameraDistance * glm::cos(glm::radians(cameraPitch));
		//Camera_Pos[2] = cameraDistance * glm::sin(glm::radians(cameraPitch)) * glm::sin(glm::radians(cameraYaw));

		//glm::vec3 cameraPos = glm::vec3(0.0, 8.0, 36);

		//glm::mat4 model(1.0f);
		//model = glm::translate(model, LightCenter);
		//model = glm::rotate(model, roty, glm::vec3(0, 1, 0));
		//model = glm::rotate(model, rotz, glm::vec3(0, 0, 1));
		//model = glm::scale(model, glm::vec3(width * 0.5, height * 0.5, 1));

		//glm::mat4 view = glm::lookAt(cameraPos, cameraPos + (LightCenter - cameraPos), glm::vec3(0, 1, 0));
		//
		//glm::mat4 proj = glm::ortho<float>(-10, 10, -10, 10, -10, 20);

		///*glm::mat4 proj = glm::perspective(glm::radians(45.0), 1.0, 0.0001, 1000.0);*/

		//glm::mat4 MVP = proj * view * model;

		//glUniformMatrix4fv(glGetUniformLocation(depthProgram, "MVP"), 1, GL_FALSE, &MVP[0][0]);

		//GLuint vertexPositionLocation = glGetAttribLocation(depthProgram, "position");	

		//glBindBuffer(GL_ARRAY_BUFFER, lightRectBuffer);
		//glVertexAttribPointer(vertexPositionLocation, 3, GL_FLOAT, false, 0, 0);
		//glEnableVertexAttribArray(vertexPositionLocation);

		////glDrawArrays(GL_TRIANGLES, 0, 6);

		//model = glm::mat4(1.0f);
		//model = glm::translate(model, glm::vec3(0, 0, 25));

		//MVP = proj * view * model;

		//glUniformMatrix4fv(glGetUniformLocation(depthProgram, "MVP"), 1, GL_FALSE, &MVP[0][0]);

		//glBindBuffer(GL_ARRAY_BUFFER, teapotBuffer);
		//glVertexAttribPointer(vertexPositionLocation, 3, GL_FLOAT, false, 0, 0);
		//glEnableVertexAttribArray(vertexPositionLocation);
		//glDrawArrays(GL_TRIANGLES, 0, teapot.position.size() / 3);

		//glDisableVertexAttribArray(vertexPositionLocation);

		glBindFramebuffer(GL_FRAMEBUFFER, NULL);
		glUseProgram(NULL);
	}
};
#endif // !T16