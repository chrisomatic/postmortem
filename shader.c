#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <GL/glew.h>

#include "shader.h"

GLuint program;
GLuint sampler;

static void shader_add(GLuint program, GLenum shader_type, const char* shader_file_path);

void shader_load_all()
{
    shader_build_program(&program,
        "shaders/basic.vert.glsl",
        "shaders/basic.frag.glsl"
    );

    // Get uniform locations
    sampler = glGetUniformLocation(program, "sampler");
}

void shader_deinit()
{
    glDeleteProgram(program);
}

void shader_build_program(GLuint* p, const char* vert_shader_path, const char* frag_shader_path)
{
	*p = glCreateProgram();

    shader_add(*p, GL_VERTEX_SHADER,  vert_shader_path);
    shader_add(*p, GL_FRAGMENT_SHADER,frag_shader_path);

	glLinkProgram(*p);

	GLint success;
    glGetProgramiv(*p, GL_LINK_STATUS, &success);
	if (!success)
    {
        GLchar info[1000+1] = {0};
		glGetProgramInfoLog(*p, 1000, NULL, info);
		fprintf(stderr, "Error linking shader program: '%s'\n", info);
        exit(1);
	}

    glValidateProgram(*p);
    glGetProgramiv(*p, GL_VALIDATE_STATUS, &success);
    if (!success) {
        GLchar info[1000+1] = {0};
        glGetProgramInfoLog(*p, 1000, NULL, info);
        fprintf(stderr, "Invalid shader program: '%s'\n", info);
        exit(1);
    }

}

static int read_file(const char* filepath, char* ret_buf, uint32_t max_buffer_size)
{
    FILE* fp = fopen(filepath,"r");
    
    if(!fp)
        return -1;

    int c;
    int i = 0;

    for(;;)
    {
        c = fgetc(fp);
        if(c == EOF)
            break;

        ret_buf[i++] = c;

        if(i >= max_buffer_size)
            break;
    }
    return i;
}
static void shader_add(GLuint program, GLenum shader_type, const char* shader_file_path)
{
    // create
	GLuint shader_id = glCreateShader(shader_type);
    if (!shader_id)
    {
        fprintf(stderr, "Error creating shader type %d\n", shader_type);
        return;
    }

    char* buf = calloc(MAX_SHADER_LEN+1,sizeof(char));
    if(!buf)
    {
        fprintf(stderr, "Failed to malloc shader buffer\n");
        return;
    }

    // load
    int len = read_file(shader_file_path,buf, MAX_SHADER_LEN);
    if(len == 0)
    {
        printf("Read zero bytes from shader file.\n");
        free(buf);
        return;
    }

	// compile
	printf("Compiling shader: %s (size: %d bytes)\n", shader_file_path, len);

	glShaderSource(shader_id, 1, (const char**)&buf, NULL);
	glCompileShader(shader_id);

	// validate
	GLint success;
	glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
	if (!success)
    {
        GLchar info[1000+1] = {0};
		glGetShaderInfoLog(shader_id, 1000, NULL, info);
		fprintf(stderr,"Error compiling shader type %d: '%s'\n", shader_type, info);
        free(buf);
        return;
	}

	glAttachShader(program, shader_id);

    free(buf);
}

void shader_set_int(GLuint program, const char* name, int i)
{
    glUniform1i(glGetUniformLocation(program, name), i);
}

void shader_set_float(GLuint program, const char* name, float f)
{
    glUniform1f(glGetUniformLocation(program, name), f);
}

void shader_set_vec3(GLuint program, const char* name, float x, float y, float z)
{
    glUniform3f(glGetUniformLocation(program, name), x,y,z);
}
