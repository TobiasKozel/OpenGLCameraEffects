#pragma once
#include "glad/glad.h"

#include <glm.hpp>


#include <string>
#include <fstream>
#include <iostream>
#include <vector>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Shader.h"
#include "Texture.h"
#include "Mesh.h"
#include "../util/Util.h"
#include <set>


class Model  {
	// Bounding Box
    float bmin[3] = { std::numeric_limits<float>::max() };
    float bmax[3] = { -std::numeric_limits<float>::max() };
    std::vector<std::shared_ptr<Mesh>> meshes;
	
public:
    NO_COPY(Model)
    std::string directory;
    bool gammaCorrection;

    Model(std::string const &path, bool gamma = false) : gammaCorrection(gamma) {
        loadModel(platformPath(path));
    }

    // draws the model, and thus all its meshes
    void draw(Shader &shader) {
    	for (auto &i : meshes) {
            i->Draw(shader);
    	}
    }
    
private:
    void loadModel(std::string const &path) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> tinyMaterials;
        std::vector<Mesh::Material> convertedMaterials;
        std::map<std::string, std::shared_ptr<Texture>> textures;

        directory = path.substr(0, path.find_last_of('/'));
        std::string warn;
        std::string err;
        bool ret = tinyobj::LoadObj(
            &attrib, &shapes, &tinyMaterials, &warn, &err,
            path.c_str() , directory.c_str(), false
        );

        if (!warn.empty()) { std::cout << "WARN: " << warn << std::endl; }
        if (!err.empty()) { std::cerr << err << std::endl; }

        if (!ret) {
            std::cerr << "Failed to load " << path << std::endl;
            return;
        }

    	// Create materials and load Textures
        for (auto &i : tinyMaterials) {
            Mesh::Material material;
            material.color = { i.diffuse[0], i.diffuse[1] , i.diffuse[2], i.dissolve };
            material.name = i.name;
            if (i.diffuse_texname.length() == 0) {
                convertedMaterials.push_back(material);
                continue;;
            }
        	
            // Only load the texture if it is not already loaded
            if (textures.find(i.diffuse_texname) != textures.end()) {
                material.colorTexture = textures.find(i.diffuse_texname)->second;
                convertedMaterials.push_back(material);
	            continue;
            }
        	
            int w, h, channels;
            std::string texture_filename = directory + "/" + i.diffuse_texname;

            unsigned char* image = stbi_load(
                texture_filename.c_str(), &w, &h,
                &channels, STBI_default
            );
        	
            if (image == nullptr) {
                std::cerr << "Unable to load texture: " << texture_filename << "\n";
                return;
            }
        	
            TextureConfig conf;
            conf.name = i.diffuse_texname;
            conf.pixels = image;
            if (channels == 4) { conf.internalFormat = conf.format = GL_RGBA; }
            if (channels == 1) { conf.internalFormat = conf.format = GL_RED; }
            std::shared_ptr<Texture> tex(new Texture(w, h, conf));

            stbi_image_free(image);
        	
            textures.insert(std::make_pair(i.diffuse_texname, tex));
            material.colorTexture = tex;
            convertedMaterials.push_back(material);
        }

        for (size_t s = 0; s < shapes.size(); s++) {
            size_t vertexIndex = 0;
            std::set<int> materialIds;
            std::vector<Mesh::Vertex> vertices;
            std::vector<unsigned int> indices;
        	
            indices.reserve(shapes[s].mesh.indices.size());
            vertices.reserve(shapes[s].mesh.indices.size());

        	// Iterate over each face
            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
                const unsigned char faceVerts = shapes[s].mesh.num_face_vertices[f];
            	// Iterate each vert of the face
                for (size_t v = 0; v < faceVerts; v++) {
                    Mesh::Vertex vertex = {};
                    tinyobj::index_t idx = shapes[s].mesh.indices[vertexIndex + v];
                    vertex.Position.x = attrib.vertices[0 + 3 * idx.vertex_index];
                    vertex.Position.y = attrib.vertices[1 + 3 * idx.vertex_index];
                    vertex.Position.z = attrib.vertices[2 + 3 * idx.vertex_index];
                    vertex.Normal.x = attrib.normals[0 + 3 * idx.normal_index];
                    vertex.Normal.y = attrib.normals[1 + 3 * idx.normal_index];
                    vertex.Normal.z = attrib.normals[2 + 3 * idx.normal_index];
                    vertex.TexCoords.x = attrib.texcoords[0 + 2 * idx.texcoord_index];
                    // For some reason the v coordinate needs to be flipped
                    vertex.TexCoords.y = 1 - attrib.texcoords[1 + 2 * idx.texcoord_index];
                    vertices.push_back(vertex);
                    indices.push_back(vertexIndex + v); // well that's kinda useless
                }
            	// Move to the next face
                vertexIndex += faceVerts;
                materialIds.insert(shapes[s].mesh.material_ids[f]);
            }
            std::vector<Mesh::Material> materials;
        	for (auto &i : materialIds) {
                materials.push_back(convertedMaterials[i]);
        	}
            std::shared_ptr<Mesh> mesh(new Mesh(vertices, indices, materials[0]));
            meshes.push_back(mesh);
        }
    }

    static bool FileExists(const std::string& abs_filename) {
        FILE* fp = fopen(abs_filename.c_str(), "rb");
        if (fp) {
            fclose(fp);
            return true;
        }
        return false;
    }
};
