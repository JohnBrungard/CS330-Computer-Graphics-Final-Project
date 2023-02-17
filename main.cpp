#include <Camera.h> // Camera class from LearnOpenGL.com
#include <iostream>             // cout, cerr
#include <cstdlib>              // EXIT_FAILURE
//#include <GL/glew.h>            // GLEW library
#include <GLFW/glfw3.h>         // GLFW library
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>      // Image loading Utility functions
// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ShapeGenerator.h>
#include "ShapeData.h"

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Final Project"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbos[2];     // Handle for the vertex buffer object & EBO
        GLuint nIndices;    // Number of indices of the mesh
        GLuint nLightIndices; // Number of indices to create light sources.
        GLuint sphereVBO{}, sphereVAO; // Handle for the sphere vbo/vao
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;

    // Triangle mesh data
    GLMesh gMesh;

    // Texture id
    GLuint gTextureId;

    // Scales texture/normal coordinates
    glm::vec2 gUVScale(2.0f, 2.0f);

    // Shader programs
    GLuint gProgramId;
    GLuint gLampProgramId;

    // Toggles ortho/perspective view
    bool ortho = false;

    // camera
    Camera gCamera(glm::vec3(0.4f, 0.5f, 2.5f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    // Subject position and scale
    glm::vec3 gPosition(1.0f, 0.1f, 0.0f);
    glm::vec3 gScale(0.36f);

    // Scene and light color
    glm::vec3 gObjectColor(1.f, .2f, 0.0f);
    glm::vec3 gLightColor(1.0f, 1.0f, 0.95f);
    // Light position and scale
    glm::vec3 gLightPosition(0.0f, 3.25f, 2.5f);
    glm::vec3 gLightScale(0.3f);
    // Fill light position and scale
    glm::vec3 gFillLightColor(1.0f, 1.0f, 1.0f);
    glm::vec3 gFillLightPosition(0.0f, 3.25f, -2.5f);
    glm::vec3 gFillLightScale(0.3f);
    // Lamp animation
    bool gIsLampOrbiting = false;

    // sphere creation variables
    GLuint sphereNumIndices;
    GLuint sphereVertexArrayObjectID;
    GLuint sphereIndexByteOffset;
    const uint NUM_FLOATS_PER_VERTICE = 9;
    const uint VERTEX_BYTE_SIZE = NUM_FLOATS_PER_VERTICE * sizeof(float);
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreateMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);


/* Vertex Shader Source Code*/
const GLchar* vertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position; // Vertex data from Vertex Attrib Pointer 0
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate; // For mapping texture to vertex locations

//Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // transforms vertices to clip coordinates
    vertexTextureCoordinate = textureCoordinate;

    vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)
    vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
}
);


/* Fragment Shader Source Code*/
const GLchar* fragmentShaderSource = GLSL(440,
    in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;

out vec4 fragmentColor;

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPosition;
uniform sampler2D uTexture;
uniform vec2 uvScale;
uniform vec3 fillLightColor;
uniform vec3 fillLightPos;

void main()
{
    /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

    //Calculate Ambient lighting*/
    float ambientStrength = 0.2f; // Set ambient or global lighting strength
    vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

    // Calculate Ambient lighting for fill light
    float fillAmbientStrength = 0.2f;
    vec3 fillAmbient = fillAmbientStrength * fillLightColor;

    //Calculate Diffuse lighting*/
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse = impact * lightColor; // Generate diffuse light color

    //Calculate Diffuse lighting for fill
    vec3 fillLightDirection = normalize(fillLightPos - vertexFragmentPos);
    float fillImpact = max(dot(norm, fillLightDirection), 0.0);
    vec3 fillDiffuse = fillImpact * fillLightColor; // Generate diffuse light color

    //Calculate Specular lighting*/
    float specularIntensity = 0.3f; // Set specular light strength
    float highlightSize = 1.0f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
    //Calculate specular component
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    vec3 specular = specularIntensity * specularComponent * lightColor;

    // Calculate Specular fill lighting
    float fillSpecularIntensity = 0.3f;
    vec3 fillReflectDir = reflect(-fillLightDirection, norm);// Calculate reflection vector
    float fillSpecularComponent = pow(max(dot(viewDir, fillReflectDir), 0.0), highlightSize);
    vec3 fillSpecular = fillSpecularIntensity * fillSpecularComponent * fillLightColor;

    // Texture holds the color to be used for all three components
    vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

    // Calculate phong result
    vec3 phong = (ambient + diffuse + specular + fillAmbient + fillDiffuse + fillSpecular) * textureColor.xyz;

    fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
}
);

/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);


/* Lamp Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

void main()
{
    fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}

int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the mesh
    UCreateMesh(gMesh); // Calls the function to create the Vertex Buffer Object

    // Create the shader programs
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
        return EXIT_FAILURE;
    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
        return EXIT_FAILURE;

    // Load texture
    const char* texFilename = "..\\resources\\textures\\texture.png";
    if (!UCreateTexture(texFilename, gTextureId))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;



        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(gMesh);

    // Release texture
    UDestroyTexture(gTextureId);

    // Release shader programs
    UDestroyShaderProgram(gProgramId);
    UDestroyShaderProgram(gLampProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 2.5f;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);

    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        if (!ortho) {
            ortho = true;
        }
        else {
            ortho = false;
        }
    }

    // Pause and resume lamp orbiting
    static bool isLKeyDown = false;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && !gIsLampOrbiting)
        gIsLampOrbiting = true;
    else if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS && gIsLampOrbiting)
        gIsLampOrbiting = false;
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
}

// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        if (action == GLFW_PRESS)
            cout << "Left mouse button pressed" << endl;
        else
            cout << "Left mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        if (action == GLFW_PRESS)
            cout << "Middle mouse button pressed" << endl;
        else
            cout << "Middle mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        if (action == GLFW_PRESS)
            cout << "Right mouse button pressed" << endl;
        else
            cout << "Right mouse button released" << endl;
    }
    break;

    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}


// Function called to render a frame
void URender()
{
    // Lamp orbits around the origin
    const float angularVelocity = glm::radians(45.0f);
    if (gIsLampOrbiting)
    {
        glm::vec4 newPosition = glm::rotate(angularVelocity * gDeltaTime, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(gLightPosition, 1.0f);
        gLightPosition.x = newPosition.x;
        gLightPosition.y = newPosition.y;
        gLightPosition.z = newPosition.z;

        // Vec3 changes the trajectory of the orbit for the fill light.
        glm::vec4 fillNewPosition = glm::rotate(angularVelocity * gDeltaTime, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(gFillLightPosition, 1.0f);
        gFillLightPosition.x = fillNewPosition.x;
        gFillLightPosition.y = fillNewPosition.y;
        gFillLightPosition.z = fillNewPosition.z;
    }

    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.3f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = glm::translate(gPosition) * glm::scale(gScale);

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Creates a perspective/ortho projection
    glm::mat4 projection;
    if (ortho) {
        projection = glm::ortho(-2.15f, 2.15f, -2.15f, 2.15f, 0.1f, 100.0f);
    }
    else {
        projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    GLint objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // References the fill light position and attributes
    GLint fillLightColorLoc = glGetUniformLocation(gProgramId, "fillLightColor");
    GLint fillLightPositionLoc = glGetUniformLocation(gProgramId, "fillLightPos");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    // Pass the fill light data to the shader uniform
    glUniform3f(fillLightColorLoc, gFillLightColor.r, gFillLightColor.g, gFillLightColor.b);
    glUniform3f(fillLightPositionLoc, gFillLightPosition.x, gFillLightPosition.y, gFillLightPosition.z);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, gMesh.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle

    // LAMP: draw lamp
    //----------------
    glUseProgram(gLampProgramId);

    model = glm::translate(gLightPosition) * glm::scale(gLightScale);

    // Reference matrix uniforms from the Lamp Shader program
    modelLoc = glGetUniformLocation(gLampProgramId, "model");
    viewLoc = glGetUniformLocation(gLampProgramId, "view");
    projLoc = glGetUniformLocation(gLampProgramId, "projection");

    // Pass matrix data to the Lamp Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, gMesh.nLightIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle

    // LAMP: draw fill lamp
    //---------------------
    glUseProgram(gLampProgramId);

    model = glm::translate(gFillLightPosition) * glm::scale(gFillLightScale);

    // Reference matrix uniforms from the Lamp Shader program
    modelLoc = glGetUniformLocation(gLampProgramId, "model");
    viewLoc = glGetUniformLocation(gLampProgramId, "view");
    projLoc = glGetUniformLocation(gLampProgramId, "projection");

    // Pass matrix data to the Lamp Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, gMesh.nLightIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle

    // setup to draw sphere
    glUseProgram(gProgramId);
    glBindVertexArray(gMesh.sphereVAO);
    model = model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.3f, 0.239f, 0.0f));
    model = glm::scale(model, glm::vec3(0.13f)); // Make it a smaller sphere
    // lightingShader.setMat4("model", model);
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // draw sphere
    glDrawElements(GL_TRIANGLES, sphereNumIndices, GL_UNSIGNED_SHORT, (void*)sphereIndexByteOffset);


    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, gTextureId);



    // Deactivate the Vertex Array Object & Shader program
    glBindVertexArray(0);
    glUseProgram(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}


// Implements the UCreateMesh function
void UCreateMesh(GLMesh& mesh)
{
    /* MUG DRAWN USING GEOGEBRA.
     * USE LINK FOR REFERENCE TO POINTS DUE TO COMPLEXITY.
     * LINK: https://www.geogebra.org/3d/uqdn8zdx
     */


     // Position and Texture data
    GLfloat verts[] = {
        // Vertex Positions         //Normal Coords                 // Texture Coords

        /* MUG BASE */

        // Index 0
        0.0f, 0.0f, 0.0f,           0.125f, 0.0f,  0.0f,          0.25f, 0.001f, // A
        // Index 1
        1.0f, 0.0f, 0.0f,           0.016f, 0.0f, 0.0f,           0.03125f, 0.001f, // B
        // Index 2
        0.92f, 0.0f, 0.38f,         0.031f, 0.0f, 0.25f,          0.0625f, 0.001f, // C
        // Index 3
        0.71f, 0.0f, 0.71f,         0.047f, 0.0f, 0.5f,           0.09375f, 0.001f, // D
        // Index 4
        0.38f, 0.0f, 0.92f,         0.063f, 0.0f, 0.75f,          0.125f, 0.001f, // E
        // Index 5
        0.0f, 0.0f, 1.0f,           0.078f, 0.0f, 1.0f,           0.15625f, 0.001f, // F
        // Index 6
        -0.38f, 0.0f, 0.92f,        0.094f, 0.0f, 0.75f,          0.1875f, 0.001f, // G
        // Index 7
        -0.71f, 0.0f, 0.71f,        0.109f, 0.0f, 0.5f,           0.21875f, 0.001f, // H
        // Index 8
        -0.92f, 0.0f, 0.38f,        0.125f, 0.0f, 0.25f,          0.25f, 0.001f, // I
        // Index 9
        -1.0f, 0.0f, 0.0f,          0.141f, 0.0f, 0.0f,           0.28125f, 0.001f, // J
        // Index 10
        -0.92f, 0.0f, -0.38f,       0.156f, 0.0f, -0.25f,         0.3125f, 0.001f, // K
        // Index 11
        -0.71f, 0.0f, -0.71f,       0.172f, 0.0f, -0.5f,          0.34375f, 0.001f, // L
        // Index 12
        -0.38f, 0.0f, -0.92f,       0.188f, 0.0f, -0.75f,         0.375f, 0.001f, // M
        // Index 13
        0.0f, 0.0f, -1.0f,          0.203f, 0.0f, -1.0f,          0.40625f, 0.001f, // N
        // Index 14
        0.38f, 0.0f, -0.92f,        0.219f, 0.0f, -0.75f,         0.4375f, 0.001f, // O
        //Index 15
        0.71f, 0.0f, -0.71f,        0.234f, 0.0f, -0.5f,          0.46875, 0.001f, // P
        // Index 16
        0.92f, 0.0f, -0.38f,        0.249f, 0.0f, -0.25f,         0.5f, 0.001f, // Q

        /* MUG TOP */

        // Index 17
        0.0f, 2.0f, 0.0f,           0.125f, 0.249f, 0.0f,         0.25f, 0.499f, // A1
        // Index 18
        1.0f, 2.0f, 0.0f,           0.016f, 0.249f, 0.0f,         0.3125f, 0.499f, // B1
        // Index 19
        0.92f, 2.0f, 0.38f,         0.031f, 0.249f, 0.25f,        0.0625f, 0.499f, // C1
        // Index 20
        0.71f, 2.0f, 0.71f,         0.047f, 0.249f, 0.5f,         0.09375f, 0.499f, // D1
        // Index 21
        0.38f, 2.0f, 0.92f,         0.063f, 0.249f, 0.75f,        0.125f, 0.499f, // E1
        // Index 22
        0.0f, 2.0f, 1.0f,           0.078f, 0.249f, 1.0f,         0.15625f, 0.499f, // F1
        // Index 23
        -0.38f, 2.0f, 0.92f,        0.094f, 0.249f, 0.75f,        0.1875f, 0.499f, // G1
        // Index 24
        -0.71f, 2.0f, 0.71f,        0.109f, 0.249f, 0.5f,         0.21875f, 0.499f, // H1
        // Index 25
        -0.92f, 2.0f, 0.38f,        0.125f, 0.249f, 0.25f,        0.25f, 0.499f, // I1
        // Index 26
        -1.0f, 2.0f, 0.0f,          0.141f, 0.249f, 0.0f,         0.28125f, 0.499f, // J1
        // Index 27
        -0.92f, 2.0f, -0.38f,       0.156f, 0.249f, -0.25f,       0.3125f, 0.499f, // K1
        // Index 28
        -0.71f, 2.0f, -0.71f,       0.172f, 0.249f, -0.5f,        0.34375f, 0.499f, // L1
        // Index 29
        -0.38f, 2.0f, -0.92f,       0.188f, 0.249f, -0.75f,       0.375f, 0.499f, // M1
        // Index 30
        0.0f, 2.0f, -1.0f,          0.203f, 0.249f, -1.0f,        0.40625f, 0.499f, // N1
        // Index 31
        0.38f, 2.0f, -0.92f,        0.219f, 0.249f, -0.75f,       0.4375f, 0.499f, // O1
        //Index 32
        0.71f, 2.0f, -0.71f,        0.234f, 0.249f, -0.5f,        0.46875, 0.499f, // P1
        // Index 33
        0.92f, 2.0f, -0.38f,        0.249f, 0.249f, -0.25f,       0.5f, 0.499f, // Q1

        /* TORUS FRONT (BIG) */

        // Index 34
        1.0001f, 1.65f, 0.11f,      0.0f, 0.249f, 1.0f,           0.001, 0.499f, // R1
        // Index 35
        1.26f, 1.67f, 0.11f,        0.063f, 0.219f, 1.0f,         0.125f, 0.4375f, // R2
        // Index 36
        1.58f, 1.53f, 0.11f,        0.125f, 0.188f, 1.0f,         0.25f, 0.375f, // R3
        // Index 37
        1.82f, 1.24f, 0.11f,        0.188f, 0.156f, 1.0f,         0.375f, 0.3125f, // R4
        // Index 38
        1.86f, 1.0f, 0.11f,         0.249f, 0.125f, 1.0f,         0.499f, 0.25f, // R5
        // Index 39
        1.82f, 0.76f, 0.11f,        0.188f, 0.094f, 1.0f,         0.375f, 0.1875f, // R6
        // Index 40
        1.58f, 0.47f, 0.11f,        0.125f, 0.063f, 1.0f,         0.25f, 0.125f, // R7
        // Index 41
        1.26f, 0.33f, 0.11f,        0.063f, 0.031f, 1.0f,         0.125f, 0.0625f, // R8
        // Index 42
        1.0001f, 0.35f, 0.11f,      0.0f, 0.0f, 1.0f,             0.001f, 0.001f, // R9

        /* TORUS FRONT (SMALL) */

        // Index 43
        0.99f, 1.475f,0.11f,        0.0f, 0.234f, 1.0f,           0.001f, 0.46875f, // S1
        // Index 44
        1.19f, 1.51f, 0.11f,        0.063f, 0.203f, 1.0f,         0.125f, 0.40625f, // S2
        // Index 45
        1.47f, 1.4f, 0.11f,         0.125f, 0.172f, 1.0f,         0.25f, 0.34375f, // S3
        // Index 46
        1.66f, 1.17f, 0.11f,        0.188f, 0.148f, 1.0f,         0.375f, 0.29688f, // S4
        // Index 47
        1.695f, 1.0f, 0.11f,        0.249f, 0.125f, 1.0f,         0.499f, 0.25f, // S5
        // Index 48
        1.66f, 0.83f, 0.11f,        0.188f, 0.109f, 1.0f,         0.375f, 0.21875f, // S6
        // Index 49
        1.47f, 0.6f, 0.11f,         0.125f, 0.078f, 1.0f,         0.25f, 0.15625f, // S7
        // Index 50
        1.19f, 0.49f, 0.11f,        0.063f, 0.047f, 1.0f,         0.125f, 0.09375f, // S8
        // Index 51
        0.99f, 0.525f,0.11f,        0.0f, 0.016f, 1.0f,           0.001f, 0.03125f, // S9

        /* TORUS BACK (BIG) */

        // Index 52
        0.99f, 1.65f, -0.11f,       0.0f, 0.249f, -1.0f,          0.001, 0.5f, // T1
        // Index 53
        1.26f, 1.67f, -0.11f,       0.063, 0.219f, -1.0f,         0.125f, 0.4375f, // T2 (or TII according to graph issues...)
        // Index 54
        1.58f, 1.53f, -0.11f,       0.125f, 0.188f, -1.0f,        0.25f, 0.375f, // T3
        // Index 55
        1.82f, 1.24f, -0.11f,       0.188f, 0.156f, -1.0f,        0.375f, 0.3125f, // T4
        // Index 56
        1.86f, 1.0f, -0.11f,        0.249f, 0.125f, -1.0f,        0.499f, 0.25f, // T5
        // Index 57
        1.82f, 0.76f, -0.11f,       0.188f, 0.094f, -1.0f,        0.375f, 0.1875f, // T6
        // Index 58
        1.58f, 0.47f, -0.11f,       0.125f, 0.063f, -1.0f,        0.25f, 0.125f, // T7
        // Index 59
        1.26f, 0.33f, -0.11f,       0.063f, 0.031f, -1.0f,        0.125f, 0.0625f, // T8
        // Index 60
        0.99f, 0.35f, -0.11f,       0.0f, 0.0f, -1.0f,            0.001f, 0.0f, // T9

        /* TORUS BACK (SMALL) */

        // Index 61
        0.99f, 1.475f, -0.11f,      0.0f, 0.234f, -1.0f,          0.001, 0.46875f, // U1
        // Index 62
        1.19f, 1.51f, -0.11f,       0.063f, 0.203f, -1.0f,        0.125f, 0.40625f, // U2
        // Index 63
        1.47f, 1.4f, -0.11f,        0.125f, 0.172f, -1.0f,        0.25f, 0.34375f, // U3
        // Index 64
        1.66f, 1.17f, -0.11f,       0.188f, 0.148f, -1.0f,        0.375f, 0.29688f, // U4
        // Index 65
        1.695f, 1.0f, -0.11f,       0.249f, 0.125f, -1.0f,        0.5f, 0.25f, // U5 or UV according to graph issues
        // Index 66
        1.66f, 0.83f, -0.11f,       0.188f, 0.109f, -1.0f,        0.375f, 0.21875f, // U6
        // Index 67
        1.47f, 0.6f, -0.11f,        0.125f, 0.078f, -1.0f,        0.25f, 0.15625f, // U7
        // Index 68
        1.19f, 0.49f, -0.11f,       0.063f, 0.047f, -1.0f,        0.125f, 0.09375f, // U8
        // Index 69
        0.99f, 0.525f, -0.11f,      0.0f, 0.016f, -1.0f,          0.001f, 0.03125f,  // U9

        /* PLANE */

        //Index 70
        3.0f, -0.001f, 5.0f,        0.25f, 0.25f, 0.25f,          0.5f, 0.5f, // Lower right
        //Index 71
        3.0f, -0.001f, -1.25f,      0.25f, 0.498f, -0.5f,         0.5f, 0.999f, // Top right
        // Index 72
        -7.0f, -0.001f, 5.0f,       0.001f, 0.25f, 0.25f,         0.001f, 0.5f, // Lower left
        // Index 73
        -7.0f, -0.001f, -1.25f,     0.001f, 0.498f, -0.5f,        0.001f, 0.999f,  // Top left

        /* CYLINDER BASE */

        // Index 74
        -4.0f, 0.0f, 0.0f,          0.375f, 0.1f,  0.0f,          0.75f, 0.001f, // A
        // Index 75
        -3.25f, 0.0f, 0.0f,         0.266f, 0.1f, 0.0f,           0.53125f, 0.001f, // B
        // Index 76
        -3.31f, 0.0f, 0.285f,       0.281f, 0.1f, 0.25f,          0.5625f, 0.001f, // C
        // Index 77
        -3.468f, 0.0f, 0.533f,      0.297f, 0.1f, 0.5f,           0.59375f, 0.001f, // D
        // Index 78
        -3.715f, 0.0f, 0.69f,       0.313f, 0.1f, 0.75f,          0.625f, 0.001f, // E
        // Index 79
        -4.0f, 0.0f, 0.75f,         0.328f, 0.1f, 1.0f,           0.65625f, 0.001f, // F
        // Index 80
        -4.285f, 0.0f, 0.69f,       0.344f, 0.1f, 0.75f,          0.6875f, 0.001f, // G
        // Index 81
        -4.533f, 0.0f, 0.533f,      0.359f, 0.1f, 0.5f,           0.71875f, 0.001f, // H
        // Index 82
        -4.69f, 0.0f, 0.285f,       0.375f, 0.1f, 0.25f,          0.75f, 0.001f, // I
        // Index 83
        -4.75f, 0.0f, 0.0f,         0.391f, 0.1f, 0.0f,           0.78125f, 0.001f, // J
        // Index 84
        -4.69f, 0.0f, -0.285f,      0.406f, 0.1f, -0.25f,         0.8125f, 0.001f, // K
        // Index 85
        -4.533f, 0.0f, -0.533f,     0.422f, 0.1f, -0.5f,          0.84375f, 0.001f, // L
        // Index 86
        -4.285f, 0.0f, -0.69f,      0.438f, 0.1f, -0.75f,         0.875f, 0.001f, // M
        // Index 87
        -4.0f, 0.0f, -0.75f,        0.453f, 0.1f, -1.0f,          0.90625f, 0.001f, // N
        // Index 88
        -3.715f, 0.0f, -0.69f,      0.469f, 0.1f, -0.75f,         0.9375f, 0.001f, // O
        //Index 89
        -3.468f, 0.0f, -0.533f,     0.484f, 0.1f, -0.5f,          0.96875, 0.001f, // P
        // Index 90
        -3.31f, 0.0f, -0.285f,      0.499f, 0.1f, -0.25f,         0.999f, 0.001f, // Q

        /* CYLINDER TOP */

        // Index 91
        -4.0f, 1.5f, 0.0f,          0.375f, 0.249f, 0.0f,         0.75f, 0.499f, // A1
        // Index 92
        -3.25f, 1.5f, 0.0f,         0.266f, 0.249f, 0.0f,         0.53125f, 0.499f, // B1
        // Index 93
        -3.31f, 1.5f, 0.285f,       0.281f, 0.249f, 0.25f,        0.5625f, 0.499f, // C1
        // Index 94
        -3.468f, 1.5f, 0.533f,      0.297f, 0.249f, 0.5f,         0.59375f, 0.499f, // D1
        // Index 95
        -3.715f, 1.5f, 0.69f,       0.313f, 0.249f, 0.75f,        0.625f, 0.499f, // E1
        // Index 96
        -4.0f, 1.5f, 0.75f,         0.328f, 0.249f, 1.0f,         0.65625f, 0.499f, // F1
        // Index 97
        -4.285f, 1.5f, 0.69f,       0.344f, 0.249f, 0.75f,        0.6875f, 0.499f, // G1
        // Index 98
        -4.533f, 1.5f, 0.533f,      0.359f, 0.249f, 0.5f,         0.71875f, 0.499f, // H1
        // Index 99
        -4.69f, 1.5f, 0.285f,       0.375f, 0.249f, 0.25f,        0.75f, 0.499f, // I1
        // Index 100
        -4.75f, 1.5f, 0.0f,         0.391f, 0.249f, 0.0f,         0.78125f, 0.499f, // J1
        // Index 101
        -4.69f, 1.5f, -0.285f,      0.406f, 0.249f, -0.25f,       0.8125f, 0.499f, // K1
        // Index 102
        -4.533f, 1.5f, -0.533f,     0.422f, 0.249f, -0.5f,        0.84375f, 0.499f, // L1
        // Index 103
        -4.285f, 1.5f, -0.69f,      0.438f, 0.249f, -0.75f,       0.875f, 0.499f, // M1
        // Index 104
        -4.0f, 1.5f, -0.75f,        0.453f, 0.249f, -1.0f,        0.90625f, 0.499f, // N1
        // Index 105
        -3.715f, 1.5f, -0.69f,      0.469f, 0.249f, -0.75f,       0.9375f, 0.499f, // O1
        //Index 106
        -3.468f, 1.5f, -0.533f,     0.484f, 0.249f, -0.5f,        0.96875, 0.499f, // P1
        // Index 107
        -3.31f, 1.5f, -0.285f,      0.499f, 0.249f, -0.25f,       0.999f, 0.499f, // Q1

    };

    // Index data to share position data
    GLushort indices[] = {
        /* MUG BASE */

        0, 1, 2,    // ABC
        0, 2, 3,    // ACD
        0, 3, 4,    // ADE
        0, 4, 5,    // AEF
        0, 5, 6,    // AFG
        0, 6, 7,    // AGH
        0, 7, 8,    // AHI
        0, 8, 9,    // AIJ
        0, 9, 10,   // AJK
        0, 10, 11,  // AKL
        0, 11, 12,  // ALM
        0, 12, 13,  // AMN
        0, 13, 14,  // ANO
        0, 14, 15,  // AOP
        0, 15, 16,  // APQ
        0, 1, 16,   // ABQ

        /* MUG BODY */

        1, 2, 18,   // BCB1
        2, 18, 19,  // CB1C1
        2, 3, 19,   // CDC1
        3, 19, 20,  // DC1D1
        3, 4, 20,   // DED1
        4, 20, 21,  // ED1E1
        4, 5, 21,   // EFE1
        5, 21, 22,  // FE1F1
        5, 6, 22,   // FGF1
        6, 22, 23,  // GF1G1
        6, 7, 23,   // GHG1
        7, 23, 24,  // HG1H1
        7, 8, 24,   // HIH1
        8, 24, 25,  // IH1I1
        8, 9, 25,   // IJI1
        9, 25, 26,  // JI1J1
        9, 10, 26,  // JKJ1
        10, 26, 27, // KJ1K1
        10, 11, 27, // KLK1
        11, 27, 28, // LK1L1
        11, 12, 28, // LML1
        12, 28, 29, // ML1M1
        12, 13, 29, // MNM1
        13, 29, 30, // NM1N1
        13, 14, 30, // NON1
        14, 30, 31, // ON1O1
        14, 15, 31, // OPO1
        15, 31, 32, // PO1P1
        15, 16, 32, // PQP1
        16, 32, 33, // QP1Q1
        16, 1, 33,  // QBQ1
        1, 18, 33,  // BB1Q1

        /* OUTER TORUS */

        34, 35, 52, // R1R2T1
        35, 52, 53, // R2T1T2 
        35, 36, 53, // R2R3T2
        36, 53, 54, // R3T2T3
        36, 37, 54, // R3R4T3
        37, 54, 55, // R4T3T4
        37, 38, 55, // R4R5T4
        38, 55, 56, // R5T4T5
        38, 39, 56, // R5R6T5
        39, 56, 57, // R6T5T6
        39, 40, 57, // R6R7T6
        40, 57, 58, // R7T6T7
        40, 41, 58, // R7R8T7
        41, 58, 59, // R8T7T8
        41, 42, 59, // R8R9T8
        42, 59, 60, // R9T8T9

        /* FRONT SIDE TORUS */

        34, 35, 43, // R1R2S1
        35, 43, 44, // R2S1S2
        35, 36, 44, // R2R3S2
        36, 44, 45, // R3S2S3
        36, 37, 45, // R3R4S3
        37, 45, 46, // R4S3S4
        37, 38, 46, // R4R5S4
        38, 46, 47, // R5S4S5
        38, 39, 47, // R5R6S5
        39, 47, 48, // R6S5S6
        39, 40, 48, // R6R7S6
        40, 48, 49, // R7S6S7
        40, 41, 49, // R7R8S7
        41, 49, 50, // R8S7S8
        41, 42, 50, // R8R9S8
        42, 50, 51, // R9S8S9


        /* BACK SIDE TORUS */

        52, 53, 61, // T1T2U1
        53, 61, 62, // T2U1U2
        53, 54, 62, // T2T3U2
        54, 62, 63, // T3U2U3
        54, 55, 63, // T3T4U3
        55, 63, 64, // T4U3U4
        55, 56, 64, // T4T5U4
        56, 64, 65, // T5U4U5
        56, 57, 65, // T5T6U5
        57, 65, 66, // T6U5U6
        57, 58, 66, // T6T7U6
        58, 66, 67, // T7U6U7
        58, 59, 67, // T7T8U7
        59, 67, 68, // T8U7U8
        59, 60, 68, // T8T9U8
        60, 68, 69, // T9U8U9

        /* INNER TORUS */

        43, 44, 61, // S1S2U1
        44, 61, 62, // S2U1U2
        44, 45, 62, // S2S3U2
        45, 62, 63, // S3U2U3
        45, 46, 63, // S3S4U3
        46, 63, 64, // S4U3U4
        46, 47, 64, // S4S5U4
        47, 64, 65, // S5U4U5
        47, 48, 65, // S5S6U5
        48, 65, 66, // S6U5U6
        48, 49, 66, // S6S7U6
        49, 66, 67, // S7U6U7
        49, 50, 67, // S7S8U7
        50, 67, 68, // S8U7U8
        50, 51, 68, // S8S9U8
        51, 68, 69, // S9U8U9

        /* INSIDE HANDLE TORUS */

        34, 43, 52, // R1S1T1
        43, 52, 61, // S1T1U1
        42, 51, 60, // R9S9T9
        51, 60, 69, // S9T9U9

        /* PLANE */

        70, 71, 72, // Lower right of Plane
        71, 72, 73, // Top left of Plane

        /* CYLINDER BASE*/

        74, 75, 76,    // ABC
        74, 76, 77,    // ACD
        74, 77, 78,    // ADE
        74, 78, 79,    // AEF
        74, 79, 80,    // AFG
        74, 80, 81,    // AGH
        74, 81, 82,    // AHI
        74, 82, 83,    // AIJ
        74, 83, 84,    // AJK
        74, 84, 85,    // AKL
        74, 85, 86,    // ALM
        74, 86, 87,    // AMN
        74, 87, 88,    // ANO
        74, 88, 89,    // AOP
        74, 89, 90,    // APQ
        74, 75, 90,    // ABQ

        /* CYLINDER TOP */

        91, 92, 93,   // A1B1C1
        91, 93, 94,   // A1C1D1
        91, 94, 95,   // A1D1E1
        91, 95, 96,   // A1E1F1
        91, 96, 97,   // A1F1G1
        91, 97, 98,   // A1G1H1
        91, 98, 99,   // A1H1I1
        91, 99, 100,  // A1I1J1
        91, 100, 101, // A1J1K1
        91, 101, 102, // A1K1L1
        91, 102, 103, // A1L1M1
        91, 103, 104, // A1M1N1
        91, 104, 105, // A1N1O1
        91, 105, 106, // A1O1P1
        91, 106, 107, // A1P1Q1
        91, 92, 107,  // A1B1Q1 

        /* CYLINDER BODY */

        75, 76, 92,   // BCB1
        76, 92, 93,   // CB1C1
        76, 77, 93,   // CDC1
        77, 93, 94,   // DC1D1
        77, 78, 94,   // DED1
        78, 94, 95,   // ED1E1
        78, 79, 95,   // EFE1
        79, 95, 96,   // FE1F1
        79, 80, 96,   // FGF1
        80, 96, 97,   // GF1G1
        80, 81, 97,   // GHG1
        81, 97, 98,   // HG1H1
        81, 82, 98,   // HIH1
        82, 98, 99,   // IH1I1
        82, 83, 99,   // IJI1
        83, 99, 100,  // JI1J1
        83, 84, 100,  // JKJ1
        84, 100, 101, // KJ1K1
        84, 85, 101,  // KLK1
        85, 101, 102, // LK1L1
        85, 86, 102,  // LML1
        86, 102, 103, // ML1M1
        86, 87, 103,  // MNM1
        87, 103, 104, // NM1N1
        87, 88, 104,  // NON1
        88, 104, 105, // ON1O1
        88, 89, 105,  // OPO1
        89, 105, 106, // PO1P1
        89, 90, 106,  // PQP1
        90, 106, 107, // QP1Q1
        90, 75, 107,  // QBQ1
        75, 92, 107,  // BB1Q1

    };

    GLuint lightIndices[] =
    {
        /* HEXADECAGON CIRCLE */

        17, 18, 19, // A1B1C1
        17, 19, 20, // A1C1D1
        17, 20, 21, // A1D1E1
        17, 21, 22, // A1E1F1
        17, 22, 23, // A1F1G1
        17, 23, 24, // A1G1H1
        17, 24, 25, // A1H1I1
        17, 25, 26, // A1I1J1
        17, 26, 27, // A1J1K1
        17, 27, 28, // A1K1L1
        17, 28, 29, // A1L1M1
        17, 29, 30, // A1M1N1
        17, 30, 31, // A1N1O1
        17, 31, 32, // A1O1P1
        17, 32, 33, // A1P1Q1
        17, 18, 33, // A1B1Q1       
    };

    // Signifies the floats per coordinate system
    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    // creates sphere object
    ShapeData sphere = ShapeGenerator::makeSphere();

    glGenVertexArrays(1, &mesh.sphereVAO);
    glGenBuffers(1, &mesh.sphereVBO);
    glBindVertexArray(mesh.sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphere.vertexBufferSize() + sphere.indexBufferSize(), 0, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)(sizeof(float) * 3));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)(sizeof(float) * 6));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.sphereVBO);

    GLsizeiptr currentOffset = 0;
    glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sphere.vertexBufferSize(), sphere.vertices);
    currentOffset += sphere.vertexBufferSize();
    sphereIndexByteOffset = currentOffset;
    glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sphere.indexBufferSize(), sphere.indices);
    sphereNumIndices = sphere.numIndices;

    // Creates vao for holding the vbo containing vertex/indice data
    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Creates lighting buffer
    mesh.nLightIndices = sizeof(lightIndices) / sizeof(lightIndices[0]);
    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers for mesh data
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(2);

}


void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, mesh.vbos);
}

/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}

void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}

// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}
