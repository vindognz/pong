#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <stdio.h>

static const char *vert_src =
	"#version 330 core\n"
	"layout (location = 0) in vec2 aPos;\n"
	"void main()\n"
	"{\n"
	"\tgl_Position = vec4(aPos, 0.0, 1.0);\n"
	"}\n";

static const char *frag_src =
	"#version 330 core\n"
	"out vec4 FragColor;\n"
	"void main()\n"
	"{\n"
	"\tFragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
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

int main(void)
{
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_Window *window = SDL_CreateWindow("Pong", 800, 600, SDL_WINDOW_OPENGL);
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

		glClearColor(0.2f, 0.2f, 0.4f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		SDL_GL_SwapWindow(window);
	}

    glDeleteProgram(program);
	SDL_GL_DestroyContext(ctx);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}