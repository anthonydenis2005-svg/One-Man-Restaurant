#include "ModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

bool beginsWith(const std::string& name, const std::string& prefix) {
    if (name.size() < prefix.size()) return false;
    for (size_t i = 0; i < prefix.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(name[i])) !=
            std::tolower(static_cast<unsigned char>(prefix[i]))) return false;
    }
    return true;
}

// Names are taken from the current main-branch office model.
// Both the alarm switch and its box activate the same interaction target.
std::string interactionGroup(const std::string& name) {
    if (name == "Board" || name == "ClipPaper") return "Clipboard";
    if (name == "FireAlarm" || name == "FireAlarmSwitch") return "FireAlarmBox";

    static const std::vector<std::string> interactivePrefixes{"Clipboard", "FireAlarmBox"};
    for (const auto& prefix : interactivePrefixes) {
        if (beginsWith(name, prefix)) return prefix;
    }
    return {};
}

void extendBounds(BoundingBox& box, const glm::vec3& position) {
    box.min = glm::min(box.min, position);
    box.max = glm::max(box.max, position);
}

BoundingBox emptyBounds() {
    const float hi = std::numeric_limits<float>::max();
    const float lo = std::numeric_limits<float>::lowest();
    return {glm::vec3(hi), glm::vec3(lo)};
}

} // namespace

std::filesystem::path findModelPath() {
    std::vector<std::filesystem::path> startingDirectories{std::filesystem::current_path()};

#ifdef _WIN32
    wchar_t executablePath[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
    if (length > 0 && length < MAX_PATH)
        startingDirectories.push_back(std::filesystem::path(executablePath).parent_path());
#endif

    for (auto directory : startingDirectories) {
        for (int level = 0; level < 6; ++level) {
            for (const char* filename : {"office.glb", "tutorial_donut1.glb"}) {
                const auto modelPath = directory / "assets" / "models" / filename;
                if (std::filesystem::is_regular_file(modelPath)) return modelPath;
            }
            if (directory == directory.root_path()) break;
            directory = directory.parent_path();
        }
    }
    return {};
}

bool loadOfficeModel(
    const std::filesystem::path& path,
    std::vector<float>& vertices,
    std::vector<Part>& parts,
    std::vector<TextureImage>& images,
    std::vector<InteractiveObject>& interactiveObjects
) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path.string(),
        aiProcess_Triangulate | aiProcess_GenSmoothNormals);
    if (!scene || !scene->HasMeshes() || !scene->mRootNode) {
        std::cerr << "Failed to read " << path.string() << "\n"
                  << importer.GetErrorString() << '\n';
        return false;
    }

    vertices.clear();
    parts.clear();
    images.clear();
    interactiveObjects.clear();

    // Read embedded GLB textures, whether compressed or supplied as raw pixels.
    images.resize(scene->mNumTextures);
    for (unsigned int i = 0; i < scene->mNumTextures; ++i) {
        const aiTexture* texture = scene->mTextures[i];
        if (!texture) continue;
        if (texture->mHeight == 0) {
            const auto* bytes = reinterpret_cast<const unsigned char*>(texture->pcData);
            images[i].encodedBytes.assign(bytes, bytes + texture->mWidth);
        } else {
            images[i].width = texture->mWidth;
            images[i].height = texture->mHeight;
            images[i].rgbaPixels.resize(static_cast<size_t>(texture->mWidth) * texture->mHeight * 4);
            for (size_t pixel = 0; pixel < static_cast<size_t>(texture->mWidth) * texture->mHeight; ++pixel) {
                const aiTexel& texel = texture->pcData[pixel];
                images[i].rgbaPixels[pixel * 4 + 0] = texel.r;
                images[i].rgbaPixels[pixel * 4 + 1] = texel.g;
                images[i].rgbaPixels[pixel * 4 + 2] = texel.b;
                images[i].rgbaPixels[pixel * 4 + 3] = texel.a;
            }
        }
    }

    std::map<std::string, BoundingBox> groups;
    std::function<void(const aiNode*, const aiMatrix4x4&, const std::string&)> visit;
    visit = [&](const aiNode* node, const aiMatrix4x4& parent, const std::string& parentGroup) {
        const aiMatrix4x4 transform = parent * node->mTransformation;
        aiMatrix3x3 normalTransform(transform);
        normalTransform.Inverse().Transpose();
        const std::string nodeName = node->mName.C_Str();
        const std::string currentGroup = interactionGroup(nodeName).empty()
            ? parentGroup : interactionGroup(nodeName);

        for (unsigned int nodeMesh = 0; nodeMesh < node->mNumMeshes; ++nodeMesh) {
            const unsigned int meshIndex = node->mMeshes[nodeMesh];
            if (meshIndex >= scene->mNumMeshes) continue;
            const aiMesh* mesh = scene->mMeshes[meshIndex];
            const std::string name = nodeName.empty()
                ? std::string(mesh->mName.C_Str()) : nodeName;
            const std::string groupName = interactionGroup(name).empty()
                ? currentGroup : interactionGroup(name);

            Part part;
            part.name = name;
            part.interactiveName = groupName;
            part.isClipboardPaper = name == "ClipPaper";
            part.first = static_cast<GLint>(vertices.size() / 10);

            if (mesh->mMaterialIndex < scene->mNumMaterials) {
                const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
                aiColor4D color;
                if (material->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS ||
                    material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
                    part.color = glm::vec3(color.r, color.g, color.b);
                aiString materialName;
                if (material->Get(AI_MATKEY_NAME, materialName) == AI_SUCCESS &&
                    std::string(materialName.C_Str()) == "Board" &&
                    part.baseColorTextureIndex == -1)
                    part.color = glm::vec3(0.72f, 0.54f, 0.35f);
                aiString texturePath;
                if (material->GetTexture(aiTextureType_BASE_COLOR, 0, &texturePath) == AI_SUCCESS ||
                    material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
                    const aiTexture* texture = scene->GetEmbeddedTexture(texturePath.C_Str());
                    if (texture) {
                        for (unsigned int index = 0; index < scene->mNumTextures; ++index) {
                            if (scene->mTextures[index] == texture) {
                                part.baseColorTextureIndex = static_cast<int>(index);
                                break;
                            }
                        }
                    }
                }
            }

            BoundingBox meshBounds = emptyBounds();
            for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
                const aiVector3D transformed = transform * mesh->mVertices[i];
                extendBounds(meshBounds, glm::vec3(transformed.x, transformed.y, transformed.z));
            }

            const float paperWidth = std::max(0.000001f, meshBounds.max.x - meshBounds.min.x);
            const float paperLength = std::max(0.000001f, meshBounds.max.z - meshBounds.min.z);

            for (unsigned int faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
                const aiFace& face = mesh->mFaces[faceIndex];
                if (face.mNumIndices != 3) continue;
                for (unsigned int corner = 0; corner < 3; ++corner) {
                    const unsigned int vertexIndex = face.mIndices[corner];
                    const aiVector3D transformed = transform * mesh->mVertices[vertexIndex];
                    aiVector3D normal = mesh->HasNormals()
                        ? normalTransform * mesh->mNormals[vertexIndex]
                        : aiVector3D(0.0f, 1.0f, 0.0f);
                    normal.Normalize();
                    const aiVector3D uv = mesh->HasTextureCoords(0)
                        ? mesh->mTextureCoords[0][vertexIndex]
                        : aiVector3D(0.0f, 0.0f, 0.0f);
                    const float paperU = (transformed.x - meshBounds.min.x) / paperWidth;
                    const float paperV = (meshBounds.max.z - transformed.z) / paperLength;
                    vertices.insert(vertices.end(), {
                        transformed.x, transformed.y, transformed.z,
                        normal.x, normal.y, normal.z,
                        uv.x, uv.y, paperU, paperV
                    });
                    ++part.count;
                }
            }

            if (part.count > 0) {
                part.center = (meshBounds.min + meshBounds.max) * 0.5f;
                part.modelWidth = std::max(0.000001f, meshBounds.max.x - meshBounds.min.x);
                parts.push_back(part);
                if (!groupName.empty()) {
                    auto result = groups.emplace(groupName, emptyBounds());
                    result.first->second.min = glm::min(result.first->second.min, meshBounds.min);
                    result.first->second.max = glm::max(result.first->second.max, meshBounds.max);
                }
            }
        }

        for (unsigned int i = 0; i < node->mNumChildren; ++i)
            visit(node->mChildren[i], transform, currentGroup);
    };

    visit(scene->mRootNode, aiMatrix4x4(), {});
    for (const auto& entry : groups) {
        interactiveObjects.push_back({entry.first, entry.second});
        std::cout << "Interactive bounds: " << entry.first << '\n';
    }
    std::cout << "Loaded " << parts.size() << " mesh parts from " << path.string() << '\n';
    return !parts.empty();
}
