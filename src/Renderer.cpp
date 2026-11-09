#include "Renderer.h"
#include "Camera.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <cstdint>
#include <iostream>
#include <limits>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincodec.h>
#endif

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace {

// Compile shader code and print an error if OpenGL rejects it.
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

// The vertex shader positions the model and the fragment shader handles its colors and lighting.
GLuint createProgram() {
    constexpr const char* vertexSource =
        "#version 330 core\n"
        "layout(location = 0) in vec3 aPosition;\n"
        "layout(location = 1) in vec3 aNormal;\n"
        "layout(location = 2) in vec2 aTexCoord;\n"
        "layout(location = 3) in vec2 aPaperUV;\n"
        "uniform mat4 uViewProjection;\n"
        "uniform mat4 uModel;\n"
        "out vec3 vNormal;\n"
        "out vec2 vPaperUV;\n"
        "out vec2 vTexCoord;\n"
        "\n"
        "void main() {\n"
        "    vNormal = mat3(uModel) * aNormal;\n"
        "    vPaperUV = aPaperUV;\n"
        "    vTexCoord = aTexCoord;\n"
        "    gl_Position = uViewProjection * uModel * vec4(aPosition, 1.0);\n"
        "}\n";

    constexpr const char* fragmentSource =
        "#version 330 core\n"
        "in vec3 vNormal;\n"
        "in vec2 vTexCoord;\n"
        "in vec2 vPaperUV;\n"
        "uniform vec3 uColor;\n"
        "uniform sampler2D uBaseTexture;\n"
        "uniform int uHasTexture;\n"
        "uniform int uOverridePaper;\n"
        "out vec4 fragColor;\n"
        "\n"
        "void main() {\n"
        "    vec3 lightDirection = normalize(vec3(-0.5, 0.9, 0.6));\n"
        "    float brightness = 0.55 + 0.45 * max(\n"
        "        dot(normalize(vNormal), lightDirection), 0.0\n"
        "    );\n"
        "    vec3 baseColor = uColor;\n"
        "    if (uOverridePaper != 0) {\n"
        "        baseColor = texture(uBaseTexture, vec2(vPaperUV.x, 1.0 - vPaperUV.y)).rgb;\n"
        "    } else if (uHasTexture != 0) {\n"
        "        baseColor *= texture(uBaseTexture, vec2(vTexCoord.x, 1.0 - vTexCoord.y)).rgb;\n"
        "    }\n"
        "    fragColor = vec4(baseColor * brightness, 1.0);\n"
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


#ifdef _WIN32
// On Windows, this converts PNG bytes into RGBA pixels for OpenGL.
bool decodePngWithWic(
    const std::vector<unsigned char>& png,
    std::vector<unsigned char>& rgba,
    unsigned int& width,
    unsigned int& height
) {
    const HRESULT comStatus = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool uninitializeCom = SUCCEEDED(comStatus);

    IWICImagingFactory* factory = nullptr;
    IWICStream* stream = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* converter = nullptr;
    bool success = false;

    do {
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
            CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)))) break;
        if (FAILED(factory->CreateStream(&stream))) break;
        if (png.size() > static_cast<size_t>(std::numeric_limits<DWORD>::max())) break;
        if (FAILED(stream->InitializeFromMemory(
            const_cast<BYTE*>(png.data()), static_cast<DWORD>(png.size())))) break;
        if (FAILED(factory->CreateDecoderFromStream(stream, nullptr,
            WICDecodeMetadataCacheOnLoad, &decoder))) break;
        if (FAILED(decoder->GetFrame(0, &frame))) break;
        if (FAILED(factory->CreateFormatConverter(&converter))) break;
        if (FAILED(converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone, nullptr, 0.0,
            WICBitmapPaletteTypeCustom))) break;
        UINT w = 0, h = 0;
        if (FAILED(converter->GetSize(&w, &h)) || w == 0 || h == 0) break;
        if (static_cast<uint64_t>(w) * h * 4 > std::numeric_limits<UINT>::max()) break;
        rgba.resize(static_cast<size_t>(w) * h * 4);
        if (FAILED(converter->CopyPixels(nullptr, w * 4,
            static_cast<UINT>(rgba.size()), rgba.data()))) break;
        width = w;
        height = h;
        success = true;
    } while (false);

    if (converter) converter->Release();
    if (frame) frame->Release();
    if (decoder) decoder->Release();
    if (stream) stream->Release();
    if (factory) factory->Release();
    if (uninitializeCom) CoUninitialize();
    return success;
}
#endif

} // namespace

// Upload the model textures and vertices, then prepare the OpenGL shaders.
bool initializeRenderer(
    RendererState& renderer,
    const std::vector<float>& vertices,
    const std::vector<TextureImage>& images,
    const std::filesystem::path& modelPath
) {
    // First upload the textures that were embedded in the GLB.
    renderer.textures.resize(images.size(), 0);
    for (size_t i = 0; i < images.size(); ++i) {
        std::vector<unsigned char> pixels = images[i].rgbaPixels;
        unsigned int textureWidth = images[i].width;
        unsigned int textureHeight = images[i].height;
#ifdef _WIN32
        if (pixels.empty() && !images[i].encodedBytes.empty() &&
            !decodePngWithWic(images[i].encodedBytes, pixels, textureWidth, textureHeight)) {
            std::cerr << "Failed to decode embedded texture " << i << ".\\n";
            continue;
        }
#else
        if (!images[i].encodedBytes.empty()) {
            std::cerr << "Embedded image decoding requires Windows WIC in this version.\\n";
        }
#endif
        if (pixels.empty() || !textureWidth || !textureHeight) continue;
        GLuint& texture = renderer.textures[i];
        glGenTextures(1, &texture);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
            static_cast<GLsizei>(textureWidth),
            static_cast<GLsizei>(textureHeight), 0,
            GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "Loaded embedded texture " << i << " ("
                  << textureWidth << "x" << textureHeight << ").\\n";
    }

    // These extra PNG files are used as the random paperwork images on the clipboard.
    const std::filesystem::path textureRoot = modelPath.parent_path().parent_path() / "textures";
    std::filesystem::path clipboardDirectory = textureRoot / "Clipboard";
    if (!std::filesystem::is_directory(clipboardDirectory)) {
        clipboardDirectory = textureRoot / "clipboard";
    }
    if (std::filesystem::is_directory(clipboardDirectory)) {
        std::vector<std::filesystem::path> files;
        for (const auto& item : std::filesystem::directory_iterator(clipboardDirectory)) {
            if (!item.is_regular_file()) continue;
            std::string extension = item.path().extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (extension == ".png") files.push_back(item.path());
        }
        std::sort(files.begin(), files.end());
        for (const auto& filePath : files) {
#ifdef _WIN32
            std::ifstream stream(filePath, std::ios::binary);
            if (!stream) continue;
            std::vector<unsigned char> png(
                (std::istreambuf_iterator<char>(stream)),
                std::istreambuf_iterator<char>()
            );
            std::vector<unsigned char> rgba;
            unsigned int w = 0, h = 0;
            if (png.empty() || !decodePngWithWic(png, rgba, w, h)) {
                std::cerr << "Could not read paper texture: " << filePath.string() << '\n';
                continue;
            }
            GLuint texture = 0;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                         static_cast<GLsizei>(w), static_cast<GLsizei>(h),
                         0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glGenerateMipmap(GL_TEXTURE_2D);
            renderer.paperTextures.push_back(texture);
#endif
        }
    }
    std::cout << "Loaded " << renderer.paperTextures.size() << " clipboard PNG textures.\n";

    // Depth testing keeps objects behind other objects from drawing over them.
    glEnable(GL_DEPTH_TEST);

    const GLuint program = createProgram();


    if (!program) {
        return false;
    }

    renderer.program = program;
    GLuint& vao = renderer.vao;
    GLuint& vbo = renderer.vbo;
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

    // Each vertex has 10 floats: position (3), normal (3), UV (2), and paper UV (2).
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        10 * sizeof(float), nullptr
    );
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE,
        10 * sizeof(float),
        reinterpret_cast<const void*>(3 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(
        2, 2, GL_FLOAT, GL_FALSE,
        10 * sizeof(float),
        reinterpret_cast<const void*>(6 * sizeof(float))
    );
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(
        3, 2, GL_FLOAT, GL_FALSE,
        10 * sizeof(float),
        reinterpret_cast<const void*>(8 * sizeof(float))
    );
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    // Save shader uniform locations so we can update them while drawing each frame.
    renderer.viewProjectionLocation =
        glGetUniformLocation(program, "uViewProjection");

    renderer.modelLocation = glGetUniformLocation(program, "uModel");
    renderer.paperOverrideLocation = glGetUniformLocation(program, "uOverridePaper");
    renderer.colorLocation =
        glGetUniformLocation(program, "uColor");
    renderer.texturedLocation =
        glGetUniformLocation(program, "uHasTexture");
    renderer.textureLocation =
        glGetUniformLocation(program, "uBaseTexture");
    return true;
}

// Move the clipboard toward the camera as the animation progress goes from 0 to 1.
glm::mat4 clipboardModelTransform(const BoundingBox& originalBounds, float progress) {
    const glm::vec3 originalCenter = (originalBounds.min + originalBounds.max) * 0.5f;
    const float t = std::clamp(progress, 0.0f, 1.0f);
    // Ease the movement so the clipboard does not start and stop abruptly.
    const float eased = t * t * (3.0f - 2.0f * t);
    const float yaw = glm::radians(kInitialCameraYaw);
    const float pitch = glm::radians(kInitialCameraPitch);
    const glm::vec3 forward(
        std::cos(yaw) * std::cos(pitch),
        std::sin(pitch),
        std::sin(yaw) * std::cos(pitch)
    );
    // The destination controls the final position of the raised clipboard.
    const glm::vec3 destination = getCameraPosition() + forward * 0.80f;
    const glm::vec3 center = destination + glm::vec3(0.0f, -0.95f * (1.0f - eased), 0.0f);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), center);
    model = glm::rotate(model, glm::radians(64.579f), glm::vec3(1.0f, 0.0f, 0.0f));
    //model = glm::scale(model, glm::vec3(1.85f));

    return glm::translate(model, -originalCenter);
}

// Draw every model part, moving the clipboard only when it is open or animating.
void renderFrame(
    const RendererState& renderer,
    const std::vector<Part>& parts,
    const std::vector<InteractiveObject>& interactiveObjects,
    const glm::mat4& viewProjection,
    const GameState& gameState, int width, int height
) {
    glm::mat4 clipboardModel(1.0f);
    if (gameState.clipboardViewState != ClipboardViewState::OnDesk) {
        for (const InteractiveObject& object : interactiveObjects) {
            if (object.name != "Clipboard") continue;
            clipboardModel = clipboardModelTransform(object.bounds, gameState.clipboardProgress);
            break;
        }
    }

    const GLuint program = renderer.program;
    const GLuint vao = renderer.vao;
    const GLint viewProjectionLocation = renderer.viewProjectionLocation;
    const GLint colorLocation = renderer.colorLocation;
    const GLint texturedLocation = renderer.texturedLocation;
    const GLint textureLocation = renderer.textureLocation;

        glViewport(0, 0, width, height);
        // --- FEATURE #1: FLASH BACKGROUND RED IF ALARM IS ACTIVE ---
        if (gameState.alarmActive) {
            float redPulse = 0.5f + 0.5f * std::sin(static_cast<float>(glfwGetTime()) * 10.0f);
            glClearColor(redPulse, 0.10f, 0.10f, 1.0f);
        }
        else {
            glClearColor(0.10f, 0.13f, 0.17f, 1.0f);
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);

        glUniformMatrix4fv(
            viewProjectionLocation,
            1,
            GL_FALSE,
            glm::value_ptr(viewProjection)
        );

        glActiveTexture(GL_TEXTURE0);
        glUniform1i(textureLocation, 0);
        glBindVertexArray(vao);

        // Each part uses its own color and texture, but clipboard parts share one transform.
        for (const Part& part : parts) {
            const bool animatedClipboard = part.interactiveName == "Clipboard" &&
                gameState.clipboardViewState != ClipboardViewState::OnDesk;
            const glm::mat4 model = animatedClipboard ? clipboardModel : glm::mat4(1.0f);
            glUniformMatrix4fv(renderer.modelLocation, 1, GL_FALSE, glm::value_ptr(model));

            GLuint texture = 0;
            if (part.baseColorTextureIndex >= 0 &&
                static_cast<size_t>(part.baseColorTextureIndex) < renderer.textures.size()) {
                texture = renderer.textures[part.baseColorTextureIndex];
            }
            // Only the clipboard paper gets replaced with the selected PNG.
            const bool overridePaper = part.isClipboardPaper &&
                gameState.selectedPaperTexture >= 0 &&
                static_cast<size_t>(gameState.selectedPaperTexture) < renderer.paperTextures.size();
            if (overridePaper) texture = renderer.paperTextures[gameState.selectedPaperTexture];

            glBindTexture(GL_TEXTURE_2D, texture);
            glUniform1i(texturedLocation, texture != 0 ? 1 : 0);
            glUniform1i(renderer.paperOverrideLocation, overridePaper ? 1 : 0);
            glUniform3fv(colorLocation, 1, glm::value_ptr(part.color));
            glDrawArrays(GL_TRIANGLES, part.first, part.count);
        }
}

// Free the textures, shaders, and buffers before the window closes.
void destroyRenderer(const RendererState& renderer) {
    const GLuint vbo = renderer.vbo;
    const GLuint vao = renderer.vao;
    const GLuint program = renderer.program;
    for (GLuint texture : renderer.textures) {
        if (texture != 0) glDeleteTextures(1, &texture);
    }
    for (GLuint texture : renderer.paperTextures) {
        if (texture != 0) glDeleteTextures(1, &texture);
    }
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
}
