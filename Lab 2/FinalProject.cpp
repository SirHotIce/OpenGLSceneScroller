#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"
#include <windows.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include "DebugCallback.h"
#include "ImGuiFileDialog.h"
#include "InitShader.h"
#include "LoadMesh.h"
#include "LoadTexture.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <random>

bool clearScreen = true;
bool enableDepthTest = true;
glm::vec3 camera_position = glm::vec3(0.0f, 0.0f, 1.0f);

float camera_fov = glm::radians(45.0f);
float near_clip = 0.1f;
float far_clip = 100.0f;

int window_width = 2000;
int window_height = 1200;
float aspect_ratio = 1.0f;

GLuint shaderProgram = -1;

struct ImportedMesh {
    int id; // Unique identifier
    std::string name;
    std::string meshFile;
    std::string textureFile;
    std::string normalMapFile;
    bool hasTexture = false;
    bool hasNormalMap = false;
    glm::vec4 tintColor = glm::vec4(1.0f);
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f); // Roll, Pitch, Yaw
    glm::vec3 scale = glm::vec3(1.0f);
    MeshData mesh;
    GLuint textureID = -1;
    GLuint normalMapID = -1;
};

std::vector<ImportedMesh> importedMeshes;
char meshNameInput[256] = "";
std::string meshFileInput = "";
std::string textureFileInput = "";
std::string normalMapFileInput = "";
bool enableTexture = false;
bool enableNormalMap = false;
glm::vec4 meshTintColor(1.0f);

int generate_unique_id() {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(1, 1000000);
    return dist(rng);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    window_width = width;
    window_height = height;
    glViewport(0, 0, width, height);
    aspect_ratio = static_cast<float>(width) / static_cast<float>(height);
}

void draw_mesh_import_window() {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Always);
    ImGui::Begin("Import Mesh", nullptr, ImGuiWindowFlags_NoResize);

    // Set a default value if the buffer is empty
    if (meshNameInput[0] == '\0') { // Check if the buffer is empty
        strncpy_s(meshNameInput, sizeof(meshNameInput), "Untitled Object", _TRUNCATE);
    }


    // Render the InputText with the current value
    ImGui::InputText("Mesh Name", meshNameInput, sizeof(meshNameInput));


    IGFD::FileDialogConfig fileDialogConfig;
    fileDialogConfig.path = ".";
    fileDialogConfig.fileName = "";
    fileDialogConfig.countSelectionMax = 1;
    fileDialogConfig.flags = ImGuiFileDialogFlags_None;

    // Mesh file 
    if (ImGui::Button("Select Mesh File")) {
        ImGuiFileDialog::Instance()->OpenDialog("ChooseMeshFile", "Select Mesh File", ".obj,.fbx,.dae", fileDialogConfig);
    }
    ImGui::SameLine();
    ImGui::Text("%s", meshFileInput.c_str());

    // Texture file
    if (ImGui::Button("Select Texture File")) {
        ImGuiFileDialog::Instance()->OpenDialog("ChooseTextureFile", "Select Texture File", ".png,.jpg,.bmp", fileDialogConfig);
    }
    ImGui::SameLine();
    ImGui::Text("%s", textureFileInput.c_str());

    // Normal map 
    if (ImGui::Button("Select Normal Map File")) {
        ImGuiFileDialog::Instance()->OpenDialog("ChooseNormalMapFile", "Select Normal Map File", ".png,.jpg,.bmp", fileDialogConfig);
    }
    ImGui::SameLine();
    ImGui::Text("%s", normalMapFileInput.c_str());

    ImGui::Checkbox("Enable Texture", &enableTexture);
    ImGui::Checkbox("Enable Normal Map", &enableNormalMap);
    ImGui::ColorEdit4("Mesh Tint", glm::value_ptr(meshTintColor));

    if (ImGui::Button("Import Mesh") && meshFileInput != "") {
        ImportedMesh newMesh;
        newMesh.id = generate_unique_id(); // Assign unique ID
        newMesh.name = meshNameInput;
        newMesh.meshFile = meshFileInput;
        newMesh.hasTexture = enableTexture;
        newMesh.hasNormalMap = enableNormalMap;
        newMesh.tintColor = meshTintColor;

        // Load the mesh
        newMesh.mesh = LoadMesh(meshFileInput);

        // Load the texture if enabled
        if (enableTexture && !textureFileInput.empty()) {
            newMesh.textureID = LoadTexture(textureFileInput);
        }

        // Load the normal map if enabled
        if (enableNormalMap && !normalMapFileInput.empty()) {
            newMesh.normalMapID = LoadTexture(normalMapFileInput);
        }

        importedMeshes.push_back(newMesh);
    }

    ImGui::End();

    // Handle mesh file selection
    if (ImGuiFileDialog::Instance()->Display("ChooseMeshFile", 32, ImVec2(500, 500), ImVec2(800, 800))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            meshFileInput = ImGuiFileDialog::Instance()->GetFilePathName();
        }
        ImGuiFileDialog::Instance()->Close();
    }

    // Handle texture file selection
    if (ImGuiFileDialog::Instance()->Display("ChooseTextureFile", 32, ImVec2(500, 500), ImVec2(800, 800))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            textureFileInput = ImGuiFileDialog::Instance()->GetFilePathName();
        }
        ImGuiFileDialog::Instance()->Close();
    }

    // Handle normal map file selection
    if (ImGuiFileDialog::Instance()->Display("ChooseNormalMapFile", 32, ImVec2(500, 500), ImVec2(800, 800))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            normalMapFileInput = ImGuiFileDialog::Instance()->GetFilePathName();
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

void draw_hierarchy_window() {
    ImGui::SetNextWindowPos(ImVec2(1590, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_Always);
    ImGui::Begin("Hierarchy", nullptr, ImGuiWindowFlags_NoResize);
    IGFD::FileDialogConfig fileDialogConfig;
    fileDialogConfig.path = ".";
    fileDialogConfig.fileName = "";
    fileDialogConfig.countSelectionMax = 1;
    fileDialogConfig.flags = ImGuiFileDialogFlags_None;

    for (size_t i = 0; i < importedMeshes.size(); ++i) {
        ImportedMesh& mesh = importedMeshes[i]; // Reference to the current mesh

        // Create a unique identifier for the CollapsingHeader
        std::string headerLabel = mesh.name + " (ID: " + std::to_string(mesh.id) + ")";
        if (ImGui::CollapsingHeader(headerLabel.c_str())) {
            ImGui::PushID(mesh.id); // Ensure the controls inside are uniquely identified

           
            // Update texture file
            static char textureBuffer[512];
            strncpy_s(textureBuffer, mesh.textureFile.c_str(), sizeof(textureBuffer));
            if (ImGui::Button("Select New Texture")) {
                ImGuiFileDialog::Instance()->OpenDialog("SelectNewTexture", "Select Texture", ".png,.jpg,.bmp", fileDialogConfig);
            }
            ImGui::SameLine();
            ImGui::Text("%s", mesh.textureFile.c_str());
            if (ImGuiFileDialog::Instance()->Display("SelectNewTexture",32, ImVec2(500,500),ImVec2(800,800))) {
                if (ImGuiFileDialog::Instance()->IsOk()) {
                    mesh.textureFile = ImGuiFileDialog::Instance()->GetFilePathName();
                    mesh.textureID = LoadTexture(mesh.textureFile); // Reload the texture
                    mesh.hasTexture = true; // Enable texture usage
                }
                ImGuiFileDialog::Instance()->Close();
            }

            // Update normal map
            static char normalMapBuffer[512];
            strncpy_s(normalMapBuffer, mesh.normalMapFile.c_str(), sizeof(normalMapBuffer));
            if (ImGui::Button("Select New Normal Map")) {
                ImGuiFileDialog::Instance()->OpenDialog("SelectNewNormalMap", "Select Normal Map", ".png,.jpg,.bmp",fileDialogConfig);
            }
            ImGui::SameLine();
            ImGui::Text("%s", mesh.normalMapFile.c_str());
            if (ImGuiFileDialog::Instance()->Display("SelectNewNormalMap", 32, ImVec2(500, 500), ImVec2(800, 800))) {
                if (ImGuiFileDialog::Instance()->IsOk()) {
                    mesh.normalMapFile = ImGuiFileDialog::Instance()->GetFilePathName();
                    mesh.normalMapID = LoadTexture(mesh.normalMapFile); // Reload the normal map
                    mesh.hasNormalMap = true; // Enable normal map usage
                }
                ImGuiFileDialog::Instance()->Close();
            }

            // Texture and normal map toggles
            ImGui::Checkbox("Use Texture", &mesh.hasTexture);
            ImGui::Checkbox("Use Normal Map", &mesh.hasNormalMap);

            // Transformation controls
            ImGui::DragFloat3("Position", glm::value_ptr(mesh.position), 0.1f);
            ImGui::DragFloat3("Rotation (Roll, Pitch, Yaw)", glm::value_ptr(mesh.rotation), 0.1f, -6.28, 6.28);
            ImGui::DragFloat3("Scale", glm::value_ptr(mesh.scale), 0.1f);
            ImGui::ColorEdit4("Tint", glm::value_ptr(mesh.tintColor));

            // Delete button
            if (ImGui::Button("Delete")) {
                importedMeshes.erase(importedMeshes.begin() + i);
                ImGui::PopID(); // Cleanup before breaking the loop
                break;          // Prevent invalid iterator access after deletion
            }

            ImGui::PopID(); // End the unique identifier scope
        }
    }

    ImGui::End();
}


void draw_gui(GLFWwindow* window) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::SetWindowFontScale(1.2);
    draw_mesh_import_window();
    draw_hierarchy_window();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void display(GLFWwindow* window) {
    if (clearScreen) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    glm::mat4 V = glm::lookAt(camera_position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 P = glm::perspective(camera_fov, aspect_ratio, near_clip, far_clip);

    for (auto& mesh : importedMeshes) {
        glm::mat4 T = glm::translate(mesh.position);
        glm::mat4 S = glm::scale(mesh.scale);
        glm::mat4 R = glm::rotate(mesh.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) *
            glm::rotate(mesh.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::rotate(mesh.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 M = T * S * R;  // Change to TSR (Translate-Scale-Rotate)
        glm::mat4 PVM = P * V * M;

        glUseProgram(shaderProgram);

        glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), mesh.hasTexture ? 1 : 0);
        if (mesh.hasTexture) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.textureID);
            glUniform1i(glGetUniformLocation(shaderProgram, "diffuse_tex"), 0);
        }


        glUniform4fv(glGetUniformLocation(shaderProgram, "tintColor"), 1, glm::value_ptr(mesh.tintColor));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "PVM"), 1, false, glm::value_ptr(PVM));

        glBindVertexArray(mesh.mesh.mVao);
        mesh.mesh.DrawMesh();
    }

    draw_gui(window);
    glfwSwapBuffers(window);
}

void reload_shader() {
    std::string vs = "shaders/f_vs.glsl";
    std::string fs = "shaders/f_fs.glsl";

    GLuint new_shader = InitShader(vs.c_str(), fs.c_str());
    if (new_shader != -1) {
        if (shaderProgram != -1) {
            glDeleteProgram(shaderProgram);
        }
        shaderProgram = new_shader;
    }
}

void init() {
    glewInit();
    RegisterDebugCallback();

    glClearColor(0.35f, 0.35f, 0.35f, 1.0f);
    if (enableDepthTest)
        glEnable(GL_DEPTH_TEST);

    reload_shader();
}

int main(void) {
    GLFWwindow* window;

    if (!glfwInit()) {
        return -1;
    }

#ifdef _DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    window = glfwCreateWindow(window_width, window_height, "OpenGL Scene Scroller", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwMakeContextCurrent(window);
    init();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    while (!glfwWindowShouldClose(window)) {
        display(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}
