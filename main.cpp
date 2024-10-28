/**
* Author: Amalie Zard
* Assignment: Lunar Lander
* Date due: 2024-10-27, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define PLATFORM_COUNT 11

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_mixer.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include <cstdlib>
#include "Entity.h"
#include <random>
#include <string>


// ––––– STRUCTS AND ENUMS ––––– //
struct GameState
{
    Entity* player;
    Entity* platforms;
};

// ––––– CONSTANTS ––––– //
constexpr int WINDOW_WIDTH  = 640,
          WINDOW_HEIGHT = 480;

constexpr float BG_RED     = 0.1922f,
            BG_BLUE    = 0.549f,
            BG_GREEN   = 0.9059f,
            BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;
constexpr char SPRITESHEET_FILEPATH[] = "assets/rocket.png";
constexpr char PLATFORM_FILEPATH[]    = "assets/platformPack_tile027.png";
constexpr char FLOOR_FILEPATH[]    = "assets/floor.png";

constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL  = 0;
constexpr GLint TEXTURE_BORDER   = 0;

constexpr int CD_QUAL_FREQ    = 44100,
          AUDIO_CHAN_AMT  = 2,     // stereo
          AUDIO_BUFF_SIZE = 4096;

constexpr char BGM_FILEPATH[] = "assets/crypto.mp3",
           SFX_FILEPATH[] = "assets/bounce.wav";

constexpr int PLAY_ONCE = 0,
          NEXT_CHNL = -1,
          ALL_SFX_CHNL = -1;

constexpr float FRICTION = 0.5f;
constexpr float GRAVITY = -0.1f;

std::string g_game_message = "";
GLuint g_font_texture_id;
float g_end_game_timer = 0.0f;
const float END_GAME_DISPLAY_TIME = 2.0f;


Mix_Music *g_music;
Mix_Chunk *g_jump_sfx;

// ––––– GLOBAL VARIABLES ––––– //
GameState g_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

// ––––– GENERAL FUNCTIONS ––––– //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;
}



void initialise()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow("Rocket Lander",
                                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        WINDOW_WIDTH, WINDOW_HEIGHT,
                                        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif


    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    g_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_program.set_projection_matrix(g_projection_matrix);
    g_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_program.get_program_id());

    //glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
    glClearColor(1.0f, 0.75f, 0.8f, 1.0f);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint player_texture_id = load_texture(SPRITESHEET_FILEPATH);
    g_font_texture_id = load_texture("assets/font1.png");
    

    g_state.player = new Entity(
        player_texture_id,
        0.0f,
        glm::vec3(0.0f, 0.0f, 0.0f),
        3.0f,
        nullptr,
        0.0f,
        0,
        0,
        1,
        1,
        1.0f,
        1.0f,
        PLAYER,
        false
    );

    //  rocket at the top of the screen
    g_state.player->set_position(glm::vec3(0.0f, 3.75f, 0.0f));
    LOG("Initial Rocket Position Set To: " << g_state.player->get_position().x << ", " << g_state.player->get_position().y);
    
   
    GLuint floor_texture_id = load_texture(FLOOR_FILEPATH);
    GLuint platform_texture_id = load_texture(PLATFORM_FILEPATH);
    g_state.platforms = new Entity[PLATFORM_COUNT];


    for (int i = 0; i < 5; i++) // first 5 are normal platforms
    {
        g_state.platforms[i].set_texture_id(platform_texture_id);
        g_state.platforms[i].set_position(glm::vec3(i - PLATFORM_COUNT / 2.0f, -3.0f, 0.0f));
        g_state.platforms[i].set_width(1.0f);
        g_state.platforms[i].set_height(1.0f);
        g_state.platforms[i].set_entity_type(SAFE_ZONE);
        g_state.platforms[i].update(0.0f, NULL, NULL, 0);
    }

    for (int i = 5; i < 9; i++) {  //  5 to 9 are dangerous zones
        g_state.platforms[i].set_texture_id(floor_texture_id);
        g_state.platforms[i].set_position(glm::vec3((i - PLATFORM_COUNT / 2.0f)*1.0f, -3.0f, 0.0f));
        g_state.platforms[i].set_width(0.3f);
        g_state.platforms[i].set_height(0.3f);
        g_state.platforms[i].set_entity_type(DANGEROUS_ZONE);
        g_state.platforms[i].update(0.0f, NULL, NULL, 0);
    }

    for (int i = 8; i < PLATFORM_COUNT; i++) // last 2 platforms are normal
    {
        g_state.platforms[i].set_texture_id(platform_texture_id);
        g_state.platforms[i].set_position(glm::vec3(i - PLATFORM_COUNT / 2.0f, -3.0f, 0.0f));
        g_state.platforms[i].set_width(1.0f);
        g_state.platforms[i].set_height(1.0f);
        g_state.platforms[i].set_entity_type(SAFE_ZONE);
        g_state.platforms[i].update(0.0f, NULL, NULL, 0);
    }
    
}

void process_input() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            g_game_is_running = false;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);
    float horizontal_acceleration = 5.0f;

    if (key_state[SDL_SCANCODE_LEFT]) {
        g_state.player->set_acceleration(glm::vec3(-horizontal_acceleration, g_state.player->get_acceleration().y, 0.0f));
    } else if (key_state[SDL_SCANCODE_RIGHT]) {
        g_state.player->set_acceleration(glm::vec3(horizontal_acceleration, g_state.player->get_acceleration().y, 0.0f));
    } else {
        // to slow down horizontal movement gradually when no keys are pressed
        glm::vec3 velocity = g_state.player->get_velocity();
        float new_velocity_x = velocity.x - FRICTION * (velocity.x > 0 ? 1 : -1);
        
        // stop if the velocity is very low
        if (fabs(new_velocity_x) < 0.1f) new_velocity_x = 0.0f;
        
        g_state.player->set_velocity(glm::vec3(new_velocity_x, velocity.y, 0.0f));
        g_state.player->set_acceleration(glm::vec3(0.0f, g_state.player->get_acceleration().y, 0.0f));
    }
}

void update() {
    if (g_end_game_timer > 0.0f) {
        g_end_game_timer -= FIXED_TIMESTEP;
        if (g_end_game_timer <= 0.0f) {
            g_game_is_running = false;
        }
        return;
    }

    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    delta_time += g_accumulator;

    if (delta_time < FIXED_TIMESTEP) {
        g_accumulator = delta_time;
        return;
    }

    while (delta_time >= FIXED_TIMESTEP) {
        g_state.player->set_acceleration(glm::vec3(g_state.player->get_acceleration().x, GRAVITY, 0.0f));
        g_state.player->update(FIXED_TIMESTEP, NULL, g_state.platforms, PLATFORM_COUNT);
        delta_time -= FIXED_TIMESTEP;
    }

    g_accumulator = delta_time;

    constexpr float LEFT_BOUND = -5.0f;
    constexpr float RIGHT_BOUND = 5.0f;
    constexpr float LOWER_BOUND = -3.75f;

    if (g_state.player->get_position().x < LEFT_BOUND ||
        g_state.player->get_position().x > RIGHT_BOUND ||
        g_state.player->get_position().y < LOWER_BOUND) {
        
        g_game_message = "Mission Failed!";
        g_end_game_timer = END_GAME_DISPLAY_TIME;
        return;
    }

    for (int i = 0; i < PLATFORM_COUNT; i++) {
        if (g_state.player->check_collision(&g_state.platforms[i])) {
            if (g_state.platforms[i].get_entity_type() == SAFE_ZONE) {
                g_game_message = "Mission Accomplished!";
            } else if (g_state.platforms[i].get_entity_type() == DANGEROUS_ZONE) {
                g_game_message = "Mission Failed!";
            }
            g_end_game_timer = END_GAME_DISPLAY_TIME;
            return;
        }
    }
}

constexpr int FONTBANK_SIZE = 16;

void draw_text(ShaderProgram *program, GLuint font_texture_id, std::string text,
               float font_size, float spacing, glm::vec3 position)
{
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    for (int i = 0; i < text.size(); i++) {
        int spritesheet_index = (int) text[i];
        float offset = (font_size + spacing) * i;
        
        float u_coordinate = (float) (spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float) (spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        vertices.insert(vertices.end(), {
            offset + (-0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
        });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
        });
    }

    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);
    
    program->set_model_matrix(model_matrix);
    glUseProgram(program->get_program_id());
    
    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates.data());
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());
    
    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int) (text.size() * 6));
    
    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}


void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    g_state.player->render(&g_program);
    for (int i = 0; i < PLATFORM_COUNT; i++) {
        g_state.platforms[i].render(&g_program);
    }

    if (!g_game_message.empty()) {
        float font_size = 0.4f;
        float spacing = 0.05f;
        float text_length = g_game_message.size() * (font_size + spacing) / 2.0f;

        glm::vec3 text_position = glm::vec3(-text_length, 0.0f, 0.0f);
        draw_text(&g_program, g_font_texture_id, g_game_message, font_size, spacing, text_position);
    }

    SDL_GL_SwapWindow(g_display_window);
}



void shutdown()
{
    SDL_Quit();

    delete [] g_state.platforms;
    delete g_state.player;
}

// ––––– GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    initialise();

    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
