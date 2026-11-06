#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/material.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>

struct Part {
    GLsizei first;
    GLsizei count;
    glm::vec3 color;
};

const char* vertexSource = R"(
#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
uniform mat4 uViewProjection;
out vec3 normal;
void main() {
    normal = aNormal;
    gl_Position = uViewProjection * vec4(aPosition, 1.0);
}
)";

const char* fragmentSource = R"(
#version 330 core
in vec3 normal;
uniform vec3 uColor;
out vec4 fragColor;
void main() {
    vec3 light = normalize(vec3(-0.4, 0.8, 0.6));
    float brightness = 0.45 + 0.55 * max(dot(normalize(normal), light), 0.0);
    fragColor = vec4(uColor * brightness, 1.0);
}
)";

GLuint makeShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[2048];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "Shader error: " << log << '\n';
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint makeProgram() {
    GLuint vertex = makeShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragment = makeShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!vertex || !fragment) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return 0;
    }
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[2048];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << "Program error: " << log << '\n';
        glDeleteProgram(program);
        program = 0;
    }
    return program;
}

bool loadModel(const char* path, std::vector<float>& vertices,
               std::vector<Part>& parts) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_PreTransformVertices
    );

    if (!scene || !scene->HasMeshes()) {
        std::cerr << "Could not load model: " << path << "\n"
                  << importer.GetErrorString() << '\n';
        return false;
    }

    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* mesh = scene->mMeshes[m];
        Part part = {static_cast<GLsizei>(vertices.size() / 6), 0,
                     glm::vec3(0.8f)};

        const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        aiColor4D color;
        if (material->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS ||
            material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            part.color = glm::vec3(color.r, color.g, color.b);
        }

        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) continue;
            for (unsigned int i = 0; i < 3; ++i) {
                unsigned int index = face.mIndices[i];
                const aiVector3D& p = mesh->mVertices[index];
                const aiVector3D n = mesh->HasNormals()
                    ? mesh->mNormals[index] : aiVector3D(0, 1, 0);
                vertices.insert(vertices.end(), {p.x, p.y, p.z, n.x, n.y, n.z});
                ++part.count;
            }
        }
        if (part.count) parts.push_back(part);
    }

    std::cout << "Loaded " << parts.size() << " mesh parts.\n";
    return !parts.empty();
}

int main() {
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed.\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1100, 700,
                                           "One Man Restaurant", nullptr, nullptr);
    if (!window) {
        std::cerr << "Could not create window.\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD initialization failed.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    GLuint program = makeProgram();
    if (!program) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::vector<float> vertices;
    std::vector<Part> parts;
    if (!loadModel(OFFICE_MODEL_PATH, vertices, parts)) {
        glDeleteProgram(program);
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                 vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);

    // Camera stays at world position (0, 0, 0), looking toward negative Z.
    const glm::vec3 cameraPosition(0.0f, 0.0f, 0.0f);
    const glm::mat4 view = glm::lookAt(
        cameraPosition,
        cameraPosition + glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0) continue;
        glViewport(0, 0, width, height);
        glClearColor(0.10f, 0.12f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const glm::mat4 projection = glm::perspective(
            glm::radians(70.0f), float(width) / float(height), 0.01f, 500.0f);
        const glm::mat4 viewProjection = projection * view;

        glUseProgram(program);
        glUniformMatrix4fv(glGetUniformLocation(program, "uViewProjection"),
                           1, GL_FALSE, glm::value_ptr(viewProjection));
        glBindVertexArray(vao);
        for (const Part& part : parts) {
            glUniform3fv(glGetUniformLocation(program, "uColor"),
                         1, glm::value_ptr(part.color));
            glDrawArrays(GL_TRIANGLES, part.first, part.count);
        }
        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
