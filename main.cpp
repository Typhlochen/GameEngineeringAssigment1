/**
* Author: Tyler Chen
* Assignment: Simple 2D Scene
* Date due: 2024-09-28, 11:58pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH = 640 * 2,
WINDOW_HEIGHT = 480 * 2;

constexpr float BG_RED = 0.827f,
BG_GREEN = 0.827f,
BG_BLUE = 0.827f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr GLint NUMBER_OF_TEXTURES = 1, // to be generated, that is
LEVEL_OF_DETAIL = 0, // mipmap reduction image level
TEXTURE_BORDER = 0; // this value MUST be zero

// source: google images
constexpr char BS_SPRITE_FILEPATH[] = "BlueShell.png",
MARIO_SPRITE_FILEPATH[] = "Mario.png",
TRACK_SPRITE_FILEPATH[] = "Track.png";

constexpr glm::vec3 INIT_SCALE = glm::vec3(5.0f, 5.98f, 0.0f);

constexpr float ROT_INCREMENT = 1.0f;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

glm::mat4 g_view_matrix,
g_model_matrix,
g_model_matrix_2,
g_background,
g_projection_matrix;

glm::vec3 g_rotation_kimi = glm::vec3(0.0f, 0.0f, 0.0f),
g_rotation_totsuko = glm::vec3(0.0f, 0.0f, 0.0f);

GLuint g_bs_texture_id,
g_mario_texture_id,
g_track_texture_id;

float g_previous_ticks = 0.0f;

float horizontal_movement = 0.0f;
float vertical_movement = 0.0f;

float theta = 0.0f;

float rotation_amount = 0.0f;

constexpr float ANGLE = glm::radians(45.0);


GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}


void initialise()
{
    // Initialise video and joystick subsystems
    SDL_Init(SDL_INIT_VIDEO);

    g_display_window = SDL_CreateWindow("Cursed by First!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_model_matrix = glm::mat4(1.0f);
    g_model_matrix_2 = glm::mat4(1.0f);
    g_background = glm::mat4(1.0f);
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_bs_texture_id = load_texture(BS_SPRITE_FILEPATH);
    g_mario_texture_id = load_texture(MARIO_SPRITE_FILEPATH);
    g_track_texture_id = load_texture(TRACK_SPRITE_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_app_status = TERMINATED;
        }
    }
}


void update()
{
    float g_ticks = (float)SDL_GetTicks() / 1000.0f;  // get the current number of g_ticks
    float g_delta_time = g_ticks - g_previous_ticks;       // the delta time is the difference from the last frame
    g_previous_ticks = g_ticks;

    rotation_amount += ANGLE * g_delta_time;
    theta += 1.0f * g_delta_time;
    horizontal_movement += glm::cos(theta) * g_delta_time;
    vertical_movement += glm::sin(theta) * g_delta_time;

    g_background = glm::mat4(1.0f);
    g_model_matrix = glm::mat4(1.0f);
    g_model_matrix_2 = glm::mat4(1.0f);
    g_model_matrix_2 = glm::translate(g_model_matrix_2, glm::vec3(0.0f, -0.7f, 0.0f));
    g_model_matrix_2 = glm::translate(g_model_matrix_2, glm::vec3(3.0f * horizontal_movement, vertical_movement, 0.0f));
    g_model_matrix_2 = glm::scale(g_model_matrix_2, glm::vec3(glm::sin(theta/0.5f) * 0.5f + 1.0f, glm::sin(theta/0.5f) * 0.5f + 1.0f, 0.0f));
    g_model_matrix = glm::translate(g_model_matrix_2, glm::vec3(2.0f * glm::sin(theta), 2.0f * glm::cos(theta), 0.0f));
    g_model_matrix = glm::rotate(g_model_matrix, rotation_amount, glm::vec3(0.0f, 0.0f, 1.0f));
    g_background = glm::rotate(g_background, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    g_background = glm::scale(g_background, glm::vec3(7.5f, 10.0f, 0.0f));
}


void draw_object(glm::mat4& object_g_model_matrix, GLuint& object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}


void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] =
    {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] =
    {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false,
        0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
        false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(g_background, g_track_texture_id);
    draw_object(g_model_matrix, g_bs_texture_id);
    draw_object(g_model_matrix_2, g_mario_texture_id);

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
