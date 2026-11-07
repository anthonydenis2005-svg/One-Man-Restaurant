
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace {

struct Part {
    GLint first = 0;
    GLsizei count = 0;
    glm::vec3 color{0.8f};
};

struct CameraState {
    float yaw = -90.0f;
    float pitch = 0.0f;
    float fieldOfView = 70.0f;
    bool rotating = false;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
};

void onMouseButton(GLFWwindow* window, int button, int action, int) {
    if (button != GLFW_MOUSE_BUTTON_MIDDLE) {
        return;
    }

    auto* camera = static_cast<CameraState*>(glfwGetWindowUserPointer(window));
    if (action == GLFW_PRESS) {
        camera->rotating = true;
        glfwGetCursorPos(window, &camera->lastMouseX, &camera->lastMouseY);
    } else if (action == GLFW_RELEASE) {
        camera->rotating = false;
    }
}

void onCursorMove(GLFWwindow* window, double mouseX, double mouseY) {
    auto* camera = static_cast<CameraState*>(glfwGetWindowUserPointer(window));
    if (!camera->rotating) {
        return;
    }

    constexpr float sensitivity = 0.15f;

    const float changeX = static_cast<float>(mouseX - camera->lastMouseX);
    const float changeY = static_cast<float>(mouseY - camera->lastMouseY);

    camera->lastMouseX = mouseX;
    camera->lastMouseY = mouseY;

    camera->yaw += changeX * sensitivity;
    camera->pitch = std::clamp(
        camera->pitch - changeY * sensitivity,
        -89.0f,
        89.0f
    );
}

void onScroll(GLFWwindow* window, double, double offsetY) {
    auto* camera = static_cast<CameraState*>(glfwGetWindowUserPointer(window));
    camera->fieldOfView = std::clamp(
        camera->fieldOfView - static_cast<float>(offsetY) * 3.0f,
        25.0f,
        100.0f
    );
}

void onFramebufferResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

std::filesystem::path findModelPath() {
    std::vector<std::filesystem::path> startingDirectories{
        std::filesystem::current_path()
    };

#ifdef _WIN32
    wchar_t executablePath[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(
        nullptr, executablePath, MAX_PATH
    );

    if (length > 0 && length < MAX_PATH) {
        startingDirectories.push_back(
            std::filesystem::path(executablePath).parent_path()
        );
    }
#endif

    for (auto directory : startingDirectories) {
        for (int level = 0; level < 6; ++level) {
            for (const char* filename : {"office.glb", "tutorial_donut1.glb"}) {
                const auto modelPath =
                    directory / "assets" / "models" / filename;

                if (std::filesystem::is_regular_file(modelPath)) {
                    return modelPath;
                }
            }

            if (directory == directory.root_path()) {
                break;
            }

            directory = directory.parent_path();
        }
    }

    return {};
}

GLuint compileShader(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success == GL_FALSE) {
        char log[2048] = {};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);

        std::cerr << "Shader compilation failed: " << log << '\n';
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint createProgram() {
    constexpr const char* vertexSource =
        "#version 330 core\n"
        "layout(location = 0) in vec3 aPosition;\n"
        "layout(location = 1) in vec3 aNormal;\n"
        "uniform mat4 uViewProjection;\n"
        "out vec3 vNormal;\n"
        "\n"
        "void main() {\n"
        "    vNormal = aNormal;\n"
        "    gl_Position = uViewProjection * vec4(aPosition, 1.0);\n"
        "}\n";

    constexpr const char* fragmentSource =
        "#version 330 core\n"
        "in vec3 vNormal;\n"
        "uniform vec3 uColor;\n"
        "out vec4 fragColor;\n"
        "\n"
        "void main() {\n"
        "    vec3 lightDirection = normalize(vec3(-0.5, 0.9, 0.6));\n"
        "    float brightness = 0.55 + 0.45 * max(\n"
        "        dot(normalize(vNormal), lightDirection), 0.0\n"
        "    );\n"
        "    fragColor = vec4(uColor * brightness, 1.0);\n"
        "}\n";

    const GLuint vertex = compileShader(GL_VERTEX_SHADER, vertexSource);

    if (!vertex) {
        return 0;
    }

    const GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    if (!fragment) {
        glDeleteShader(vertex);
        return 0;
    }

    const GLuint program = glCreateProgram();

    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (success == GL_FALSE) {
        char log[2048] = {};
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);

        std::cerr << "Shader linking failed: " << log << '\n';
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

bool loadOfficeModel(
    const std::filesystem::path& path,
    std::vector<float>& vertices,
    std::vector<Part>& parts
) {
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        path.string(),
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_PreTransformVertices
    );

    if (!scene || !scene->HasMeshes()) {
        std::cerr << "Failed to read " << path.string() << "\n"
                  << importer.GetErrorString() << '\n';
        return false;
    }

    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        const aiMesh* mesh = scene->mMeshes[meshIndex];

        Part part;
        part.first = static_cast<GLint>(vertices.size() / 6);

        if (mesh->mMaterialIndex < scene->mNumMaterials) {
            const aiMaterial* material =
                scene->mMaterials[mesh->mMaterialIndex];

            aiColor4D color;

            if (material->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS ||
                material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {

                part.color = glm::vec3(
                    color.r,
                    color.g,
                    color.b
                );
            }
        }

        for (unsigned int faceIndex = 0;
             faceIndex < mesh->mNumFaces;
             ++faceIndex) {

            const aiFace& face = mesh->mFaces[faceIndex];

            if (face.mNumIndices != 3) {
                continue;
            }

            for (unsigned int corner = 0; corner < 3; ++corner) {
                const unsigned int vertexIndex = face.mIndices[corner];

                const aiVector3D& position =
                    mesh->mVertices[vertexIndex];

                const aiVector3D normal = mesh->HasNormals()
                    ? mesh->mNormals[vertexIndex]
                    : aiVector3D(0.0f, 1.0f, 0.0f);

                vertices.insert(vertices.end(), {
                    position.x,
                    position.y,
                    position.z,
                    normal.x,
                    normal.y,
                    normal.z
                });

                ++part.count;
            }
        }

        if (part.count > 0) {
            parts.push_back(part);
        }
    }

    std::cout << "Loaded " << parts.size()
              << " mesh parts from " << path.string() << '\n';

    return !parts.empty();
}

} // namespace

int main() {
    const auto modelPath = findModelPath();

    std::vector<float> vertices;
    std::vector<Part> parts;

    if (modelPath.empty()) {
        std::cerr << "No GLB model found. Place office.glb or "
                     "tutorial_donut1.glb inside assets/models/.\n";
        return 1;
    }

    if (!loadOfficeModel(modelPath, vertices, parts)) {
        return 1;
    }

    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "Could not initialize GLFW.\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        1100,
        700,
        "One Man Restaurant - Office",
        nullptr,
        nullptr
    );

    if (!window) {
        std::cerr << "Could not create the OpenGL window.\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(
        reinterpret_cast<GLADloadproc>(glfwGetProcAddress)
    )) {
        std::cerr << "Could not initialize GLAD.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    CameraState camera;

    glfwSetWindowUserPointer(window, &camera);
    glfwSetFramebufferSizeCallback(window, onFramebufferResize);
    glfwSetMouseButtonCallback(window, onMouseButton);
    glfwSetCursorPosCallback(window, onCursorMove);
    glfwSetScrollCallback(window, onScroll);

    glEnable(GL_DEPTH_TEST);

    const GLuint program = createProgram();

    if (!program) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    GLuint vao = 0;
    GLuint vbo = 0;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
        vertices.data(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        6 * sizeof(float), nullptr
    );
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE,
        6 * sizeof(float),
        reinterpret_cast<const void*>(3 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    const GLint viewProjectionLocation =
        glGetUniformLocation(program, "uViewProjection");

    const GLint colorLocation =
        glGetUniformLocation(program, "uColor");

    const glm::vec3 cameraPosition(1.25f, 0.85f, 1.75f);

    std::cout << "Hold middle mouse and drag to rotate. "
                 "Scroll to zoom. Esc to exit.\n";

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        int width = 0;
        int height = 0;

        glfwGetFramebufferSize(window, &width, &height);

        if (width == 0 || height == 0) {
            continue;
        }

        glViewport(0, 0, width, height);

        const float yawRadians = glm::radians(camera.yaw);
        const float pitchRadians = glm::radians(camera.pitch);

        const glm::vec3 forward(
            std::cos(yawRadians) * std::cos(pitchRadians),
            std::sin(pitchRadians),
            std::sin(yawRadians) * std::cos(pitchRadians)
        );

        const glm::mat4 view = glm::lookAt(
            cameraPosition,
            cameraPosition + glm::normalize(forward),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        const glm::mat4 projection = glm::perspective(
            glm::radians(camera.fieldOfView),
            static_cast<float>(width) / static_cast<float>(height),
            0.01f,
            500.0f
        );

        const glm::mat4 viewProjection = projection * view;

        glClearColor(0.10f, 0.13f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);

        glUniformMatrix4fv(
            viewProjectionLocation,
            1,
            GL_FALSE,
            glm::value_ptr(viewProjection)
        );

        glBindVertexArray(vao);

        for (const Part& part : parts) {
            glUniform3fv(
                colorLocation,
                1,
                glm::value_ptr(part.color)
            );

            glDrawArrays(
                GL_TRIANGLES,
                part.first,
                part.count
            );
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
