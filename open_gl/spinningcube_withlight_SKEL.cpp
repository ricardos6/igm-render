// Copyright (C) 2020 Emilio J. Padrón
// Released as Free Software under the X11 License
// https://spdx.org/licenses/X11.html

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

// GLM library to deal with matrix operations
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::perspective
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp> // glm::mat4

#include "textfile_ALT.h"

// img textures library
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int gl_width  = 640;
int gl_height = 480;

void glfw_window_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void render(double);

GLuint shader_program = 0; // shader program to set render pipeline
GLuint vao            = 0; // Vertext Array Object to set input data
GLint model_location, view_location, proj_location,
    normal_to_world_location; // Uniforms for transformation matrices
GLint view_pos_location;      // Uniform for camera position
GLint light_pos1_location, light_amb1_location, light_diff1_location,
    light_spec1_location, light_pos2_location, light_amb2_location,
    light_diff2_location, light_spec2_location; // Uniform for light position
GLint material_shin_location, material_diff_location,
    material_spec_location; // Uniform for material properties
GLint cam_pos_location;     // Uniform for camera position
GLuint texture_maps[2];     // The diffuse and specular maps

// Shader names
const char *vertexFileName   = "spinningcube_withlight_vs_SKEL.glsl";
const char *fragmentFileName = "spinningcube_withlight_fs_SKEL.glsl";

// Cameras
glm::vec3 camera_pos(5.0f, 5.0f, 5.0f);

// Lighting
glm::vec3 light_pos1(-2.0f, 4.0f, -1.0f);
glm::vec3 light_pos2(5.0f, 5.0f, 5.0f);
glm::vec3 light_ambient(0.2f, 0.2f, 0.2f);
glm::vec3 light_diffuse(0.5f, 0.5f, 0.5f);
glm::vec3 light_specular(1.0f, 1.0f, 1.0f);

// Material
const GLfloat material_shininess = 64.0f;

int main() {
    // start GL context and O/S window using the GLFW helper library
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW3\n");
        return 1;
    }

    GLFWwindow *window
        = glfwCreateWindow(gl_width, gl_height, "My spinning cube", NULL, NULL);
    if (!window) {
        fprintf(stderr, "ERROR: could not open window with GLFW3\n");
        glfwTerminate();
        return 1;
    }
    glfwSetWindowSizeCallback(window, glfw_window_size_callback);
    glfwMakeContextCurrent(window);

    // start GLEW extension handler
    // glewExperimental = GL_TRUE;
    glewInit();

    // get version info
    const GLubyte *vendor    = glGetString(GL_VENDOR);   // get vendor string
    const GLubyte *renderer  = glGetString(GL_RENDERER); // get renderer string
    const GLubyte *glversion = glGetString(GL_VERSION);  // version as a string
    const GLubyte *glslversion
        = glGetString(GL_SHADING_LANGUAGE_VERSION); // version as a string
    printf("Vendor: %s\n", vendor);
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", glversion);
    printf("GLSL version supported %s\n", glslversion);
    printf("Starting viewport: (width: %d, height: %d)\n", gl_width, gl_height);

    // Enable Depth test: only draw onto a pixel if fragment closer to viewer
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS); // set a smaller value as "closer"

    // Vertex Shader
    char *vertex_shader = textFileRead(vertexFileName);

    // Fragment Shader
    char *fragment_shader = textFileRead(fragmentFileName);

    // Shaders compilation
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader, NULL);
    free(vertex_shader);
    glCompileShader(vs);

    int success;
    char infoLog[512];
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vs, 512, NULL, infoLog);
        printf("ERROR: Vertex Shader compilation failed!\n%s\n", infoLog);

        return (1);
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader, NULL);
    free(fragment_shader);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fs, 512, NULL, infoLog);
        printf("ERROR: Fragment Shader compilation failed!\n%s\n", infoLog);

        return (1);
    }

    // Create program, attach shaders to it and link it
    shader_program = glCreateProgram();
    glAttachShader(shader_program, fs);
    glAttachShader(shader_program, vs);
    glLinkProgram(shader_program);

    glValidateProgram(shader_program);
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program, 512, NULL, infoLog);
        printf("ERROR: Shader Program linking failed!\n%s\n", infoLog);

        return (1);
    }

    // Release shader objects
    glDeleteShader(vs);
    glDeleteShader(fs);

    // Vertex Array Object
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Cube to be rendered
    //
    //          0        3
    //       7        4 <-- top-right-near
    // bottom
    // left
    // far ---> 1        2
    //       6        5
    //

    const GLfloat vertex_positions[] = {
        -0.25f, -0.25f, -0.25f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, // 1
        -0.25f, 0.25f, -0.25f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,  // 0
        0.25f, -0.25f, -0.25f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,  // 2

        0.25f, 0.25f, -0.25f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,  // 3
        0.25f, -0.25f, -0.25f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, // 2
        -0.25f, 0.25f, -0.25f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // 0

        0.25f, -0.25f, -0.25f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // 2
        0.25f, 0.25f, -0.25f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // 3
        0.25f, -0.25f, 0.25f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // 5

        0.25f, 0.25f, 0.25f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // 4
        0.25f, -0.25f, 0.25f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // 5
        0.25f, 0.25f, -0.25f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // 3

        0.25f, -0.25f, 0.25f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,  // 5
        0.25f, 0.25f, 0.25f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,   // 4
        -0.25f, -0.25f, 0.25f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // 6

        -0.25f, 0.25f, 0.25f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,  // 7
        -0.25f, -0.25f, 0.25f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // 6
        0.25f, 0.25f, 0.25f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,   // 4

        -0.25f, -0.25f, 0.25f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // 6
        -0.25f, 0.25f, 0.25f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // 7
        -0.25f, -0.25f, -0.25f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // 1

        -0.25f, 0.25f, -0.25f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // 0
        -0.25f, -0.25f, -0.25f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // 1
        -0.25f, 0.25f, 0.25f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // 7

        0.25f, -0.25f, -0.25f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,  // 2
        0.25f, -0.25f, 0.25f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,   // 5
        -0.25f, -0.25f, -0.25f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // 1

        -0.25f, -0.25f, 0.25f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,  // 6
        -0.25f, -0.25f, -0.25f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // 1
        0.25f, -0.25f, 0.25f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,   // 5

        0.25f, 0.25f, 0.25f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // 4
        0.25f, 0.25f, -0.25f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // 3
        -0.25f, 0.25f, 0.25f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // 7

        -0.25f, 0.25f, -0.25f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // 0
        -0.25f, 0.25f, 0.25f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,  // 7
        0.25f, 0.25f, -0.25f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,  // 3

        // SECOND CUBE

        1.5f, 1.5f, 1.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, // 1
        1.5f, 2.5f, 1.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // 0
        2.5f, 1.5f, 1.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, // 2

        2.5f, 2.5f, 1.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, // 3
        2.5f, 1.5f, 1.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, // 2
        1.5f, 2.5f, 1.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // 0

        2.5f, 1.5f, 1.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // 2
        2.5f, 2.5f, 1.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // 3
        2.5f, 1.5f, 2.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // 5

        2.5f, 2.5f, 2.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // 4
        2.5f, 1.5f, 2.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // 5
        2.5f, 2.5f, 1.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // 3

        2.5f, 1.5f, 2.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // 5
        2.5f, 2.5f, 2.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // 4
        1.5f, 1.5f, 2.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // 6

        1.5f, 2.5f, 2.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, // 7
        1.5f, 1.5f, 2.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // 6
        2.5f, 2.5f, 2.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // 4

        1.5f, 1.5f, 2.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // 6
        1.5f, 2.5f, 2.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // 7
        1.5f, 1.5f, 1.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // 1

        1.5f, 2.5f, 1.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // 0
        1.5f, 1.5f, 1.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // 1
        1.5f, 2.5f, 2.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // 7

        2.5f, 1.5f, 1.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, // 2
        2.5f, 1.5f, 2.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // 5
        1.5f, 1.5f, 1.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // 1

        1.5f, 1.5f, 2.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, // 6
        1.5f, 1.5f, 1.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // 1
        2.5f, 1.5f, 2.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // 5

        2.5f, 2.5f, 2.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // 4
        2.5f, 2.5f, 1.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // 3
        1.5f, 2.5f, 2.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // 7

        1.5f, 2.5f, 1.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // 0
        1.5f, 2.5f, 2.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // 7
        2.5f, 2.5f, 1.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f  // 3
    };

    // Vertex Buffer Object (for vertex coordinates)
    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_positions), vertex_positions,
        GL_STATIC_DRAW);

    // Vertex attributes
    // 0: vertex position (x, y, z)
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // 1: vertex normals (x, y, z)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
        (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 2: image maps coords (x, y)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
        (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Unbind vbo (it was conveniently registered by VertexAttribPointer)
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Unbind vao
    glBindVertexArray(0);

    //  - Model matrix
    model_location = glGetUniformLocation(shader_program, "model");

    // - View matrix
    view_location = glGetUniformLocation(shader_program, "view");

    // - Projection matrix
    proj_location = glGetUniformLocation(shader_program, "projection");

    // - Normal matrix: normal vectors from local to world coordinates
    normal_to_world_location
        = glGetUniformLocation(shader_program, "normal_to_world");

    // - Camera position
    view_pos_location = glGetUniformLocation(shader_program, "view_pos");

    // - Light data 1
    light_pos1_location
        = glGetUniformLocation(shader_program, "light.position");
    light_amb1_location = glGetUniformLocation(shader_program, "light.ambient");
    light_diff1_location
        = glGetUniformLocation(shader_program, "light.diffuse");
    light_spec1_location
        = glGetUniformLocation(shader_program, "light.specular");

    // - Light data 2
    light_pos2_location
        = glGetUniformLocation(shader_program, "light.position");
    light_amb2_location = glGetUniformLocation(shader_program, "light.ambient");
    light_diff2_location
        = glGetUniformLocation(shader_program, "light.diffuse");
    light_spec2_location
        = glGetUniformLocation(shader_program, "light.specular");

    // - Material data
    material_shin_location
        = glGetUniformLocation(shader_program, "material.shininess");

    // - Texture maps
    glGenTextures(2, texture_maps);

    // Diffuse map in GL_TEXTURE0 var
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_maps[0]);
    int imageWidth, imageHeight, ImageNrChannels;
    stbi_set_flip_vertically_on_load(1);
    unsigned char *image
        = stbi_load("diffuse_map.jpg", &imageWidth, &imageHeight, &ImageNrChannels, 0);
    if (image) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_RGB,
            GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        printf("cant load diffuse map\n");
    }
    stbi_image_free(image);

    // Diffuse map in GL_TEXTURE1 var
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture_maps[1]);
    image = stbi_load("specular_map.jpg", &imageWidth, &imageHeight, &ImageNrChannels, 0);
    if (image) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, GL_RGB,
            GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        printf("cant load specular map\n");
    }
    stbi_image_free(image);

    // Render loop
    while (!glfwWindowShouldClose(window)) {

        processInput(window);

        render(glfwGetTime());

        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}

void render(double currentTime) {
    float f = (float)currentTime * 0.3f;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, gl_width, gl_height);

    glUseProgram(shader_program);
    glBindVertexArray(vao);

    glm::mat4 model_matrix, view_matrix, proj_matrix, normal_matrix;

    // proj matrix
    proj_matrix = glm::perspective(
        glm::radians(40.0f), (float)gl_width / (float)gl_height, 0.1f, 1000.0f);
    glUniformMatrix4fv(proj_location, 1, GL_FALSE, glm::value_ptr(proj_matrix));

    // view matrix
    view_matrix = glm::lookAt(camera_pos, // pos
        glm::vec3(0.0f, 0.0f, 0.0f),      // target
        glm::vec3(0.0f, 1.0f, 0.0f));     // up
    glUniformMatrix4fv(view_location, 1, GL_FALSE, glm::value_ptr(view_matrix));

    // moving cube
    model_matrix = glm::translate(glm::mat4(1.f), glm::vec3(0.0f, 0.0f, -4.0f));
    model_matrix = glm::translate(model_matrix,
        glm::vec3(sinf(2.1f * f) * 0.5f, cosf(1.7f * f) * 0.5f,
            sinf(1.3f * f) * cosf(1.5f * f) * 2.0f));
    model_matrix = glm::rotate(model_matrix,
        glm::radians((float)currentTime * 45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model_matrix = glm::rotate(model_matrix,
        glm::radians((float)currentTime * 81.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(
        model_location, 1, GL_FALSE, glm::value_ptr(model_matrix));

    // Matriz normal to world
    normal_matrix = glm::transpose(glm::inverse(glm::mat3(model_matrix)));
    glUniformMatrix3fv(
        normal_to_world_location, 1, GL_FALSE, glm::value_ptr(normal_matrix));

    // luz1
    glUniform3fv(light_pos1_location, 1, glm::value_ptr(light_pos1));
    glUniform3fv(light_amb1_location, 1, glm::value_ptr(light_ambient));
    glUniform3fv(light_diff1_location, 1, glm::value_ptr(light_diffuse));
    glUniform3fv(light_spec1_location, 1, glm::value_ptr(light_specular));

    // luz2
    glUniform3fv(light_pos2_location, 1, glm::value_ptr(light_pos2));
    glUniform3fv(light_amb2_location, 1, glm::value_ptr(light_ambient));
    glUniform3fv(light_diff2_location, 1, glm::value_ptr(light_diffuse));
    glUniform3fv(light_spec2_location, 1, glm::value_ptr(light_specular));

    // material
    glUniform1f(material_shin_location, material_shininess);

    // camera pos
    glUniform3fv(cam_pos_location, 1, glm::value_ptr(camera_pos));

    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDrawArrays(GL_TRIANGLES, 36, 36);
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, 1);
}

// Callback function to track window size and update viewport
void glfw_window_size_callback(GLFWwindow *window, int width, int height) {
    gl_width  = width;
    gl_height = height;
    printf("New viewport: (width: %d, height: %d)\n", width, height);
}
