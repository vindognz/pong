#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <stdio.h>

#define WIN_W 800
#define WIN_H 600

#define PADDLE_W  20.0f
#define PADDLE_H  100.0f
#define BALL_SIZE 20.0f

#define PADDLE_SPEED 400.0f

static const char *vert_src =
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "uniform mat4 uProjection;\n"
    "uniform vec2 uPos;\n"
    "uniform vec2 uSize;\n"
    "void main()\n"
    "{\n"
    "\tgl_Position = uProjection * vec4(aPos * uSize + uPos, 0.0, 1.0);\n"
    "}\n";

static const char *frag_src =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec3 uColor;\n"
    "void main()\n"
    "{\n"
    "\tFragColor = vec4(uColor, 1.0);\n"
    "}\n";

static GLuint compile_shader(GLenum type, const char *src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        fprintf(stderr, "Shader compile error:\n%s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;    
}

static GLuint create_program(const char *vsrc, const char *fsrc) {
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vsrc);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fsrc);

    if (!vs || !fs) {
        glDeleteShader(vs);
        glDeleteShader(fs);
        return 0;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, sizeof(log), NULL, log);
        fprintf(stderr, "Program link error:\n%s\n", log);
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

static void ortho(float *m, float left, float right, float top, float bottom) {
    for (int i = 0; i < 16; i++) {
        m[i] = 0.0f;
    }

    m[0]  = 2.0f / (right - left);
    m[5]  = 2.0f / (top - bottom);
    m[10] = -1.0f;
    m[12] = -(right + left) / (right - left);
    m[13] = -(top + bottom) / (top - bottom);
    m[15] = 1.0f;
}

static GLint pos_loc, size_loc, color_loc;

static void draw_rect(float x, float y, float w, float h, float r, float g, float b) {
    glUniform2f(pos_loc, x, y);
    glUniform2f(size_loc, w, h);
    glUniform3f(color_loc, r, g, b);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

int main(void) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window *window = SDL_CreateWindow("Pong", WIN_W, WIN_H, SDL_WINDOW_OPENGL);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GLContext ctx = SDL_GL_CreateContext(window);
    if (!ctx) {
        fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return 1;
    }

    if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress)) {
        fprintf(stderr, "Failed to load GL functions\n");
        return 1;
    }

    SDL_GL_SetSwapInterval(1); /* vsync */

    printf("GL version: %s\n", (const char *)glGetString(GL_VERSION));

    GLuint program = create_program(vert_src, frag_src);
    if (!program) {
        return 1;
    }
    printf("Shader program OK (id %u)\n", program);


    float proj[16];
    ortho(proj, 0.0f, (float) WIN_W, (float) WIN_H, 0.0f);

    GLint proj_loc = glGetUniformLocation(program, "uProjection");
    if (proj_loc < 0) {
        fprintf(stderr, "uProjection not found");
    }

    glUseProgram(program);
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, proj);


    pos_loc = glGetUniformLocation(program, "uPos");
    size_loc = glGetUniformLocation(program, "uSize");
    color_loc = glGetUniformLocation(program, "uColor");

    float x = WIN_W / 2.0f;
    float y = WIN_H / 2.0f;
    float vx = 300.0f; // pixels per second

    float left_y =  WIN_H / 2.0f;
    float right_y = WIN_H / 2.0f;

    Uint64 last = SDL_GetTicksNS();
    

    float vertices[] = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f,  0.5f,
    };

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);


    int running = 1;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = 0;
            }

            if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.key == SDLK_ESCAPE) {
                    running = 0;
                }
            }
        }

        Uint64 now = SDL_GetTicksNS();
        float dt = (float) (now - last) / 1000000000.0f;
        last = now;

        x += vx * dt;
        if (x + (BALL_SIZE / 2.0f) > WIN_W) {
            x = WIN_W - (BALL_SIZE / 2.0f);
            vx = -vx;
        }
        if (x - (BALL_SIZE / 2.0f) < 0.0f) {
            x = (BALL_SIZE / 2.0f);
            vx = -vx;
        }


        const bool *keys = SDL_GetKeyboardState(NULL);
        
        if (keys[SDL_SCANCODE_W])    left_y  += PADDLE_SPEED * dt;
        if (keys[SDL_SCANCODE_S])    left_y  -= PADDLE_SPEED * dt;
        if (keys[SDL_SCANCODE_UP])   right_y += PADDLE_SPEED * dt;
        if (keys[SDL_SCANCODE_DOWN]) right_y -= PADDLE_SPEED * dt;

        // keep the paddles on screen
        float min_y = PADDLE_H / 2.0f;
        float max_y = WIN_H - PADDLE_H / 2.0f;

        if (left_y  < min_y) left_y  = min_y;
        if (left_y  > max_y) left_y  = max_y;
        if (right_y < min_y) right_y = min_y;
        if (right_y > max_y) right_y = max_y;


        glClearColor(0.2f, 0.2f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
        glBindVertexArray(vao);

        draw_rect(30.0f, left_y, PADDLE_W, PADDLE_H, 1.0f, 1.0f, 1.0f);
        draw_rect(WIN_W - 30.0f, right_y, PADDLE_W, PADDLE_H, 1.0f, 1.0f, 1.0f);
        draw_rect(x, y, BALL_SIZE, BALL_SIZE, 1.0f, 0.8f, 0.2f);

        SDL_GL_SwapWindow(window);
    }

    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    SDL_GL_DestroyContext(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}