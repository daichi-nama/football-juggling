#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

//////////////////////////////////
// constants
//////////////////////////////////

static constexpr double FPS        = 60.0;
static constexpr float RADIUS      = 0.76f;
static constexpr float INITIAL_POS = 3.0f;

static const glm::vec3 LIGHT_POS = glm::vec3(5.0f, 20.0f, 5.0f);
static constexpr float SHININESS = 100.0f;

static const glm::vec3 CELL_POS[9] = {
    glm::vec3(-2.0f, 0.0f, -2.0f), glm::vec3(0.0f, 0.0f, -2.0f), glm::vec3(2.0f, 0.0f, -2.0f),
    glm::vec3(-2.0f, 0.0f, 0.0f),  glm::vec3(0.0f, 0.0f, 0.0f),  glm::vec3(2.0f, 0.0f, 0.0f),
    glm::vec3(-2.0f, 0.0f, 2.0f),  glm::vec3(0.0f, 0.0f, 2.0f),  glm::vec3(2.0f, 0.0f, 2.0f),
};
static const glm::vec3 UNIT_RECTANGLE_POS[4]
    = {glm::vec3(-1.0f, 0.0f, -1.0f), glm::vec3(-1.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 1.0f),
       glm::vec3(1.0f, 0.0f, -1.0f)};
static const glm::vec2 UNIT_RECTANGLE_UV[4]
    = {glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 0.0f)};
static const unsigned int UNIT_RECTANGLE_INDEX[2][3] = {{0, 1, 2}, {3, 2, 0}};

static const glm::vec3 WHITE      = glm::vec3(1.0f, 1.0f, 1.0f);
static const glm::vec3 RED        = glm::vec3(1.0f, 0.0f, 0.0f);
static const glm::vec3 SPEC_COLOR = glm::vec3(0.8f, 0.8f, 0.8f);
static const glm::vec3 AMBI_COLOR = glm::vec3(0.3f, 0.3f, 0.3f);

static const std::string SHADER_DIRECTORY         = "../src/shaders/";
static const std::string DATA_DIRECTORY           = "../data/";
static const std::string COLOR_VERT_SHADER_FILE   = SHADER_DIRECTORY + "color.vert";
static const std::string COLOR_FRAG_SHADER_FILE   = SHADER_DIRECTORY + "color.frag";
static const std::string TEXTURE_VERT_SHADER_FILE = SHADER_DIRECTORY + "texture.vert";
static const std::string TEXTURE_FRAG_SHADER_FILE = SHADER_DIRECTORY + "texture.frag";
static const std::string RENDER_VERT_SHADER_FILE  = SHADER_DIRECTORY + "render.vert";
static const std::string RENDER_FRAG_SHADER_FILE  = SHADER_DIRECTORY + "render.frag";
static const std::string GRASS_TEX_FILE           = DATA_DIRECTORY + "grass.jpg";
static const std::string BALL_OBJ_FILE            = DATA_DIRECTORY + "Football.obj";

//////////////////////////////////
// global variables
//////////////////////////////////

static int g_win_width         = 900;
static int g_win_height        = 900;
static std::string g_win_title = "Football Juggling Game";
static glm::mat4 g_proj_mat
    = glm::perspective(45.0f, (float)g_win_width / (float)g_win_height, 0.1f, 1000.0f);
static glm::mat4 g_view_mat = glm::lookAt(glm::vec3(0.0f, 5.0f, 6.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                                          glm::vec3(0.0f, 1.0f, 0.0f));

//////////////////////////////////
// classes
//////////////////////////////////

struct Vertex1 {
    Vertex1(const glm::vec3& position_, const glm::vec3& color_)
        : position(position_), color(color_) {}

    glm::vec3 position;
    glm::vec3 color;
};

struct Vertex2 {
    Vertex2(const glm::vec3& position_, const glm::vec2& texcoord_)
        : position(position_), texcoord(texcoord_) {}

    glm::vec3 position;
    glm::vec2 texcoord;
};

struct Vertex3 {
    Vertex3(const glm::vec3& position_, const glm::vec3& normal_, const glm::vec3& diffuse_)
        : position(position_), normal(normal_), diffuse(diffuse_) {}

    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 diffuse;
};

class RenderObject {
   protected:
    static GLuint compile_shader(const std::string& filename, GLuint type) {
        GLuint shader_id = glCreateShader(type);

        std::ifstream reader;
        size_t codeSize;
        std::string code;

        reader.open(filename.c_str(), std::ios::in);
        if (!reader.is_open()) {
            fprintf(stderr, "Failed to load a shader: %s\n", filename.c_str());
            std::exit(1);
        }

        reader.seekg(0, std::ios::end);
        codeSize = reader.tellg();
        code.resize(codeSize);
        reader.seekg(0);
        reader.read(&code[0], codeSize);
        reader.close();

        const char* codeChars = code.c_str();
        glShaderSource(shader_id, 1, &codeChars, NULL);
        glCompileShader(shader_id);

        GLint compileStatus;
        glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compileStatus);
        if (compileStatus == GL_FALSE) {
            fprintf(stderr, "Failed to compile a shader!\n");

            GLint log_length;
            glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);
            if (log_length > 0) {
                GLsizei length;
                std::string err_msg;
                err_msg.resize(log_length);
                glGetShaderInfoLog(shader_id, log_length, &length, &err_msg[0]);

                fprintf(stderr, "[ ERROR ] %s\n", err_msg.c_str());
                fprintf(stderr, "%s\n", code.c_str());
            }
            std::exit(1);
        }

        return shader_id;
    }

    void build_shader_program(const std::string& vert_shader_file,
                              const std::string& frag_shader_file) {
        GLuint vert_shader_id = compile_shader(vert_shader_file, GL_VERTEX_SHADER);
        GLuint frag_shader_id = compile_shader(frag_shader_file, GL_FRAGMENT_SHADER);

        m_program_id = glCreateProgram();
        glAttachShader(m_program_id, vert_shader_id);
        glAttachShader(m_program_id, frag_shader_id);
        glLinkProgram(m_program_id);

        GLint linkState;
        glGetProgramiv(m_program_id, GL_LINK_STATUS, &linkState);
        if (linkState == GL_FALSE) {
            fprintf(stderr, "Failed to link shaders!\n");

            GLint log_length;
            glGetProgramiv(m_program_id, GL_INFO_LOG_LENGTH, &log_length);
            if (log_length > 0) {
                GLsizei length;
                std::string err_msg;
                err_msg.resize(log_length);
                glGetProgramInfoLog(m_program_id, log_length, &length, &err_msg[0]);

                fprintf(stderr, "[ ERROR ] %s\n", err_msg.c_str());
            }
            std::exit(1);
        }

        glUseProgram(0);
    }

    void load_texture(const std::string& filename) {
        int tex_width, tex_height, channels;
        unsigned char* bytes
            = stbi_load(filename.c_str(), &tex_width, &tex_height, &channels, STBI_rgb_alpha);
        if (!bytes) {
            fprintf(stderr, "Failed to load image file: %s\n", filename.c_str());
            std::exit(1);
        }

        glGenTextures(1, &m_texture_id);
        glBindTexture(GL_TEXTURE_2D, m_texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_width, tex_height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     bytes);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(bytes);
    }

    void load_obj(const std::string& obj_file, const std::string& mtl_file_dir,
                  std::vector<Vertex3>& vertices, std::vector<unsigned int>& indices) {
        tinyobj::ObjReaderConfig reader_config;
        reader_config.mtl_search_path = mtl_file_dir;
        tinyobj::ObjReader reader;
        if (!reader.ParseFromFile(obj_file, reader_config)) {
            if (!reader.Error().empty()) {
                fprintf(stderr, "TinyObjReader: %s", reader.Error().c_str());
            }
            std::exit(1);
        }
        // if (!reader.Warning().empty()) {
        //     printf("TinyObjReader: %s", reader.Warning().c_str());
        // }

        auto& attrib    = reader.GetAttrib();
        auto& shapes    = reader.GetShapes();
        auto& materials = reader.GetMaterials();

        for (size_t s = 0; s < shapes.size(); s++) {
            size_t index_offset = 0;
            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
                size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
                for (size_t v = 0; v < fv; v++) {
                    glm::vec3 position, normal, diffuse;
                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                    position = glm::vec3(attrib.vertices[3 * size_t(idx.vertex_index) + 0],
                                         attrib.vertices[3 * size_t(idx.vertex_index) + 1],
                                         attrib.vertices[3 * size_t(idx.vertex_index) + 2]);
                    if (idx.normal_index >= 0) {
                        normal = glm::vec3(attrib.normals[3 * size_t(idx.normal_index) + 0],
                                           attrib.normals[3 * size_t(idx.normal_index) + 1],
                                           attrib.normals[3 * size_t(idx.normal_index) + 2]);
                    }
                    diffuse = glm::vec3(materials[shapes[s].mesh.material_ids[f]].diffuse[0],
                                        materials[shapes[s].mesh.material_ids[f]].diffuse[1],
                                        materials[shapes[s].mesh.material_ids[f]].diffuse[2]);

                    indices.push_back((unsigned int)vertices.size());
                    vertices.push_back(Vertex3(position, normal, diffuse));
                }
                index_offset += fv;
            }
        }
    }

    void init_vao1(const std::vector<Vertex1>& vertices, const std::vector<unsigned int>& indices) {
        glGenVertexArrays(1, &m_vao_id);
        glBindVertexArray(m_vao_id);

        glGenBuffers(1, &m_vbo_id);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo_id);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex1) * vertices.size(), vertices.data(),
                     GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex1),
                              (void*)offsetof(Vertex1, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex1),
                              (void*)offsetof(Vertex1, color));

        glGenBuffers(1, &m_ibo_id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo_id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), indices.data(),
                     GL_STATIC_DRAW);

        m_buffer_size = (GLsizei)indices.size();

        glBindVertexArray(0);
    }

    void init_vao2(const std::vector<Vertex2>& vertices, const std::vector<unsigned int>& indices) {
        glGenVertexArrays(1, &m_vao_id);
        glBindVertexArray(m_vao_id);

        glGenBuffers(1, &m_vbo_id);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo_id);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex2) * vertices.size(), vertices.data(),
                     GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex2),
                              (void*)offsetof(Vertex2, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2),
                              (void*)offsetof(Vertex2, texcoord));

        glGenBuffers(1, &m_ibo_id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo_id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), indices.data(),
                     GL_STATIC_DRAW);

        m_buffer_size = (GLsizei)indices.size();

        glBindVertexArray(0);
    }

    void init_vao3(const std::vector<Vertex3>& vertices, const std::vector<unsigned int>& indices) {
        glGenVertexArrays(1, &m_vao_id);
        glBindVertexArray(m_vao_id);

        glGenBuffers(1, &m_vbo_id);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo_id);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex3) * vertices.size(), vertices.data(),
                     GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3),
                              (void*)offsetof(Vertex3, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3),
                              (void*)offsetof(Vertex3, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3),
                              (void*)offsetof(Vertex3, diffuse));

        glGenBuffers(1, &m_ibo_id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo_id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), indices.data(),
                     GL_STATIC_DRAW);

        m_buffer_size = (GLsizei)indices.size();

        glBindVertexArray(0);
    }

    void draw_elements1(const glm::mat4& mvp_mat) {
        glUseProgram(m_program_id);
        glBindVertexArray(m_vao_id);
        GLuint uid = glGetUniformLocation(m_program_id, "u_mvp_mat");
        glUniformMatrix4fv(uid, 1, GL_FALSE, glm::value_ptr(mvp_mat));
        glDrawElements(m_mode, m_buffer_size, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glUseProgram(0);
    }

    void draw_elements2(const glm::mat4& mvp_mat) {
        glUseProgram(m_program_id);
        glBindVertexArray(m_vao_id);
        GLuint uid = glGetUniformLocation(m_program_id, "u_mvp_mat");
        glUniformMatrix4fv(uid, 1, GL_FALSE, glm::value_ptr(mvp_mat));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texture_id);
        uid = glGetUniformLocation(m_program_id, "u_texture");
        glUniform1i(uid, 0);
        glDrawElements(m_mode, m_buffer_size, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    }

    void draw_elements3(const glm::mat4& mv_mat, const glm::mat4& mvp_mat,
                        const glm::mat4& norm_mat, const glm::mat4& light_mat,
                        const glm::vec3& light_pos, float shininess) {
        glUseProgram(m_program_id);
        glBindVertexArray(m_vao_id);
        GLuint uid;
        uid = glGetUniformLocation(m_program_id, "u_mv_mat");
        glUniformMatrix4fv(uid, 1, GL_FALSE, glm::value_ptr(mv_mat));
        uid = glGetUniformLocation(m_program_id, "u_mvp_mat");
        glUniformMatrix4fv(uid, 1, GL_FALSE, glm::value_ptr(mvp_mat));
        uid = glGetUniformLocation(m_program_id, "u_norm_mat");
        glUniformMatrix4fv(uid, 1, GL_FALSE, glm::value_ptr(norm_mat));
        uid = glGetUniformLocation(m_program_id, "u_light_mat");
        glUniformMatrix4fv(uid, 1, GL_FALSE, glm::value_ptr(light_mat));
        uid = glGetUniformLocation(m_program_id, "u_light_pos");
        glUniform3fv(uid, 1, glm::value_ptr(light_pos));
        uid = glGetUniformLocation(m_program_id, "u_specColor");
        glUniform3fv(uid, 1, glm::value_ptr(SPEC_COLOR));
        uid = glGetUniformLocation(m_program_id, "u_ambiColor");
        glUniform3fv(uid, 1, glm::value_ptr(AMBI_COLOR));
        uid = glGetUniformLocation(m_program_id, "u_shininess");
        glUniform1f(uid, shininess);
        glDrawElements(GL_TRIANGLES, m_buffer_size, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glUseProgram(0);
    }

   protected:
    GLuint m_vao_id       = 0;
    GLuint m_vbo_id       = 0;
    GLuint m_ibo_id       = 0;
    GLsizei m_buffer_size = 0;
    GLuint m_texture_id   = 0;
    GLuint m_program_id   = 0;
    GLuint m_mode         = 0;
};

class Ground : public RenderObject {
   public:
    Ground() {
        m_mode = GL_TRIANGLES;
    }

    void init() {
        unsigned int idx = 0;
        std::vector<Vertex2> vertices;
        std::vector<unsigned int> indices;
        for (int i = 0; i < 3; i++) {
            Vertex2 v(UNIT_RECTANGLE_POS[UNIT_RECTANGLE_INDEX[0][i]],
                      6.0f * UNIT_RECTANGLE_UV[UNIT_RECTANGLE_INDEX[0][i]]);
            vertices.push_back(v);
            indices.push_back(idx++);
        }
        for (int i = 0; i < 3; i++) {
            Vertex2 v(UNIT_RECTANGLE_POS[UNIT_RECTANGLE_INDEX[1][i]],
                      6.0f * UNIT_RECTANGLE_UV[UNIT_RECTANGLE_INDEX[1][i]]);
            vertices.push_back(v);
            indices.push_back(idx++);
        }
        init_vao2(vertices, indices);
        load_texture(GRASS_TEX_FILE);
        build_shader_program(TEXTURE_VERT_SHADER_FILE, TEXTURE_FRAG_SHADER_FILE);
    }

    void draw() {
        glm::mat4 model_mat = glm::mat4(1.0f);
        model_mat           = glm::translate(model_mat, glm::vec3(0.0f, -RADIUS * 2, 0.0f));
        model_mat           = glm::scale(model_mat, glm::vec3(32.0f));
        glm::mat4 mvp_mat   = g_proj_mat * g_view_mat * model_mat;
        draw_elements2(mvp_mat);
    }
};

class Grid : public RenderObject {
   public:
    Grid() {
        m_mode = GL_LINES;
    }

    void init() {
        unsigned int idx = 0;
        std::vector<Vertex1> vertices;
        std::vector<unsigned int> indices;
        int index_four[5] = {
            0, 1, 2, 3, 0,
        };
        for (int j = 0; j < 9; ++j) {
            for (int i = 0; i < 4; ++i) {
                for (int k = 0; k < 2; ++k) {
                    glm::vec3 pos = UNIT_RECTANGLE_POS[index_four[i + k]] + get_pos(j);
                    Vertex1 v(pos, WHITE);
                    vertices.push_back(v);
                    indices.push_back(idx++);
                }
            }
        }
        init_vao1(vertices, indices);
        build_shader_program(COLOR_VERT_SHADER_FILE, COLOR_FRAG_SHADER_FILE);
    }

    void draw() {
        glm::mat4 model_mat = glm::mat4(1.0f);
        glm::mat4 mvp_mat   = g_proj_mat * g_view_mat * model_mat;
        draw_elements1(mvp_mat);
    }

   private:
    glm::vec3 get_pos(int pos_idx) {
        return CELL_POS[pos_idx] + glm::vec3(0.0f, -RADIUS, 0.0f);
    }
};

enum class GameMode {
    BEFORE_START,
    FALLING,
    JUGGLING,
    FAILED,
};

class Tile : public RenderObject {
   public:
    Tile(const glm::vec3& color, int pos_idx = 4) : m_color(color), m_pos_idx(pos_idx) {
        m_mode = GL_TRIANGLES;
    }

    void init() {
        unsigned int idx = 0;
        std::vector<Vertex1> vertices;
        std::vector<unsigned int> indices;
        for (int i = 0; i < 3; i++) {
            Vertex1 v(UNIT_RECTANGLE_POS[UNIT_RECTANGLE_INDEX[0][i]], m_color);
            vertices.push_back(v);
            indices.push_back(idx++);
        }
        for (int i = 0; i < 3; i++) {
            Vertex1 v(UNIT_RECTANGLE_POS[UNIT_RECTANGLE_INDEX[1][i]], m_color);
            vertices.push_back(v);
            indices.push_back(idx++);
        }
        init_vao1(vertices, indices);
        build_shader_program(COLOR_VERT_SHADER_FILE, COLOR_FRAG_SHADER_FILE);
    }

    void draw() {
        glm::mat4 model_mat = glm::mat4(1.0f);
        model_mat           = glm::translate(model_mat, get_pos());
        glm::mat4 mvp_mat   = g_proj_mat * g_view_mat * model_mat;
        draw_elements1(mvp_mat);
    }

    void set_pos_number(int pos_idx) {
        m_pos_idx = pos_idx;
    }

    void set_pos_number_by_key(char key) {
        switch (key) {
            case 'Q':
                m_pos_idx = 0;
                break;
            case 'W':
                m_pos_idx = 1;
                break;
            case 'E':
                m_pos_idx = 2;
                break;
            case 'A':
                m_pos_idx = 3;
                break;
            case 'S':
                m_pos_idx = 4;
                break;
            case 'D':
                m_pos_idx = 5;
                break;
            case 'Z':
                m_pos_idx = 6;
                break;
            case 'X':
                m_pos_idx = 7;
                break;
            case 'C':
                m_pos_idx = 8;
                break;
            default:
                break;
        }
    }

    int get_pos_idx() const {
        return m_pos_idx;
    }

   private:
    glm::vec3 get_pos() {
        return CELL_POS[m_pos_idx] + glm::vec3(0.0f, -RADIUS, 0.0f);
    }

   private:
    glm::vec3 m_color;
    int m_pos_idx = 4;
};

class Ball : public RenderObject {
   public:
    Ball() {
        reset();
        m_mode = GL_TRIANGLES;
    }

    float get_falling_pos() const {
        return m_falling_pos;
    }

    bool is_fallen() const {
        return m_rev_angle >= 180.0f;
    }

    int get_next_pos_idx() const {
        return m_next_pos_idx;
    }

    void set_dest() {
        m_last_pos_idx    = m_next_pos_idx;
        m_rev_angle       = 0.0f;
        m_next_pos_idx    = get_random_next_pos_idx(m_last_pos_idx);
        m_rev_angular_vel = get_random_rev_angular_vel();
        m_rot_angular_vel = get_random_rot_angular_vel();
        calc_rotation();
    }

    void reset() {
        m_falling_pos     = INITIAL_POS;
        m_last_pos_idx    = 4;
        m_next_pos_idx    = 4;
        m_rot_angle       = 0.0f;
        m_rev_angle       = 0.0f;
        m_rot_angular_vel = 1.0f;
        m_rev_angular_vel = 2.25f;
    }

    static void calc_bounds(glm::vec3& min_bound, glm::vec3& max_bound,
                            const std::vector<Vertex3>& vertices) {
        float large_val = 100000.0f;
        min_bound       = glm::vec3(+large_val);
        max_bound       = glm::vec3(-large_val);
        for (const auto& v : vertices) {
            for (int i = 0; i < 3; ++i) {
                min_bound[i] = std::min(min_bound[i], v.position[i]);
                max_bound[i] = std::max(max_bound[i], v.position[i]);
            }
        }
    }

    static float calc_radius(const glm::vec3& min_bound, const glm::vec3& max_bound) {
        return (max_bound.x - min_bound.x) * 0.5f;
    }

    static glm::vec3 calc_center(const glm::vec3& min_bound, const glm::vec3& max_bound) {
        return (min_bound + max_bound) * 0.5f;
    }

    void init() {
        std::vector<Vertex3> vertices;
        std::vector<unsigned int> indices;
        load_obj(BALL_OBJ_FILE, DATA_DIRECTORY, vertices, indices);
        glm::vec3 min_bound, max_bound;
        calc_bounds(min_bound, max_bound, vertices);
        float radius = calc_radius(min_bound, max_bound);
        m_to_center  = calc_center(min_bound, max_bound);
        m_scale      = RADIUS / radius;
        init_vao3(vertices, indices);
        build_shader_program(RENDER_VERT_SHADER_FILE, RENDER_FRAG_SHADER_FILE);
    }

    void draw_before_starting() {
        glm::mat4 trans_mat, adj_mat;
        trans_mat = adj_mat = glm::mat4(1.0f);
        trans_mat           = glm::translate(trans_mat, glm::vec3(0.0f, m_falling_pos, 0.0f));

        // Move the center of the ball to the origin
        adj_mat = glm::scale(adj_mat, glm::vec3(m_scale));
        adj_mat = glm::translate(adj_mat, -m_to_center);

        glm::mat4 mv_mat    = g_view_mat * trans_mat * adj_mat;
        glm::mat4 mvp_mat   = g_proj_mat * g_view_mat * trans_mat * adj_mat;
        glm::mat4 norm_mat  = glm::transpose(glm::inverse(mv_mat));
        glm::mat4 light_mat = g_view_mat;

        draw_elements3(mv_mat, mvp_mat, norm_mat, light_mat, LIGHT_POS, SHININESS);
    }

    void draw_during_game() {
        glm::vec3 last_pos  = get_pos_from_idx(m_last_pos_idx);
        glm::mat4 model_mat = glm::mat4(1.0f);
        model_mat           = glm::translate(model_mat, m_rotation_center);
        model_mat           = glm::rotate(model_mat, glm::radians(m_rev_angle), m_rotation_axis);
        model_mat           = glm::translate(model_mat, last_pos - m_rotation_center);
        model_mat           = glm::rotate(model_mat, glm::radians(-m_rev_angle), m_rotation_axis);
        model_mat = glm::rotate(model_mat, glm::radians(m_rot_angle), glm::vec3(1.0f, 0.0f, 0.0f));
        // Move the center of the ball to the origin
        model_mat = glm::scale(model_mat, glm::vec3(m_scale));
        model_mat = glm::translate(model_mat, -m_to_center);

        glm::mat4 mv_mat    = g_view_mat * model_mat;
        glm::mat4 mvp_mat   = g_proj_mat * g_view_mat * model_mat;
        glm::mat4 norm_mat  = glm::transpose(glm::inverse(mv_mat));
        glm::mat4 light_mat = g_view_mat;

        draw_elements3(mv_mat, mvp_mat, norm_mat, light_mat, LIGHT_POS, SHININESS);
    }

    void update_fall() {
        m_falling_pos -= 0.1f;
    }

    void update_juggle() {
        m_rot_angle += m_rot_angular_vel;
        if (m_rot_angle >= 360.0f) {
            m_rot_angle = 0.0f;
        }
        m_rev_angle += m_rev_angular_vel;
    }

   private:
    int get_random_next_pos_idx(int last_pos_idx) const {
        int result;
        while (1) {
            result = rand() % 9;
            if (result != last_pos_idx) {
                return result;
            }
        }
    }

    float get_random_rev_angular_vel() const {
        const int num = rand();
        switch (num % 4) {
            case 0:
                return 2.25f;
                break;
            case 1:
                return 2.5f;
                break;
            case 2:
                return 3.0f;
                break;
            case 3:
                return 3.6f;
                break;
            default:
                fprintf(stderr, "Bug in 'get_random_rev_angular_vel'");
                std::exit(1);
                break;
        }
    }

    float get_random_rot_angular_vel() const {
        const int num = rand();
        switch (num % 4) {
            case 0:
                return 1.0f;
                break;
            case 1:
                return 5.0f;
                break;
            case 2:
                return 10.0f;
                break;
            case 3:
                return 20.0f;
                break;
            default:
                fprintf(stderr, "Bug in 'get_random_rot_angular_vel'");
                std::exit(1);
                break;
        }
    }

    glm::vec3 get_pos_from_idx(int idx) const {
        return CELL_POS[idx];
    }

    void calc_rotation() {
        glm::vec3 next_pos  = get_pos_from_idx(m_next_pos_idx);
        glm::vec3 last_pos  = get_pos_from_idx(m_last_pos_idx);
        glm::vec3 direction = next_pos - last_pos;
        m_rotation_axis     = glm::cross(direction, glm::vec3(0.0f, -1.0f, 0.0f));
        m_rotation_center   = (last_pos + next_pos) * 0.5f;
    }

   private:
    float m_falling_pos;

    int m_last_pos_idx;
    int m_next_pos_idx;

    float m_rot_angle;  // rotation angle
    float m_rev_angle;  // revolution angle
    float m_rot_angular_vel;
    float m_rev_angular_vel;

    glm::vec3 m_to_center;
    float m_scale;

    glm::vec3 m_rotation_axis;
    glm::vec3 m_rotation_center;
};

class GameManager {
   public:
    GameManager()
        : m_game_mode(GameMode::BEFORE_START), m_tile(WHITE), m_red_tile(RED), m_count(0) {}

    void init() {
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        m_ball.init();
        m_grid.init();
        m_ground.init();
        m_tile.init();
        m_red_tile.init();
    }

    void main_loop() {
        m_grid.draw();
        m_ground.draw();
        m_tile.draw();

        if (m_game_mode == GameMode::BEFORE_START) {
            m_ball.draw_before_starting();
        } else if (m_game_mode == GameMode::FALLING) {
            m_ball.draw_before_starting();
            if (m_ball.get_falling_pos() < 0.0f) {
                m_game_mode = GameMode::JUGGLING;
                printf("%d\n", ++m_count);
                m_ball.set_dest();
            }
            m_ball.update_fall();
        } else if (m_game_mode == GameMode::JUGGLING) {
            m_ball.draw_during_game();
            if (is_failure()) {
                m_game_mode = GameMode::FAILED;
                printf(
                    "Failed!\n"
                    "Score: %d\n"
                    "Press space to restart.\n\n",
                    m_count);
            } else if (m_ball.is_fallen()) {
                printf("%d\n", ++m_count);
                m_ball.set_dest();
            }
            m_ball.update_juggle();
        } else if (m_game_mode == GameMode::FAILED) {
            m_ball.draw_during_game();
            m_red_tile.set_pos_number(m_ball.get_next_pos_idx());
            m_red_tile.draw();
        }
    }

    void keyboard_event(GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS && m_game_mode == GameMode::JUGGLING) {
            m_tile.set_pos_number_by_key(key);
        }
        if (action == GLFW_PRESS && (char)key == ' '
            && (m_game_mode == GameMode::BEFORE_START || m_game_mode == GameMode::FAILED)) {
            m_game_mode = GameMode::FALLING;
            reset();
        }
    }

   private:
    bool is_failure() {
        if (!m_ball.is_fallen()) {
            return false;
        }
        if (m_ball.get_next_pos_idx() == m_tile.get_pos_idx()) {
            return false;
        }
        return true;
    }

    void reset() {
        m_ball.reset();
        m_tile.set_pos_number(4);
        m_count = 0;
    }

   private:
    GameMode m_game_mode;

    Ball m_ball;
    Grid m_grid;
    Tile m_tile;
    Tile m_red_tile;
    Ground m_ground;

    int m_count;
};

static GameManager g_game;

void resize_gl(GLFWwindow* window, int width, int height) {
    g_win_width  = width;
    g_win_height = height;
    glfwSetWindowSize(window, g_win_width, g_win_height);
    int render_buffer_width, render_buffer_height;
    glfwGetFramebufferSize(window, &render_buffer_width, &render_buffer_height);
    glViewport(0, 0, render_buffer_width, render_buffer_height);
    g_proj_mat = glm::perspective(45.0f, (float)g_win_width / (float)g_win_height, 0.1f, 1000.0f);
}

void keyboard_event(GLFWwindow* window, int key, int scancode, int action, int mods) {
    g_game.keyboard_event(window, key, scancode, action, mods);
}

void print_how_to_play() {
    printf(
        "\n"
        "-------------------------------------------------------\n"
        "                Football Juggling Game\n"
        "-------------------------------------------------------\n\n"
        "---- How To Play ----\n\n"
        "Move the white tile so that the soccer ball won't fall.\n\n"
        "  Top left      - Q\n"
        "  Top center    - W\n"
        "  Top right     - E\n"
        "  Center left   - A\n"
        "  Center        - S\n"
        "  Center right  - D\n"
        "  Bottom left   - Z\n"
        "  Bottom center - X\n"
        "  Bottom right  - C\n\n"
        "Your score will be displayed on the console.\n\n"
        "Press space to start.\n\n");
}

//////////////////////////////////
// main function
//////////////////////////////////

int main(int argc, char** argv) {
    std::srand((unsigned int)time(NULL));

    if (glfwInit() == GL_FALSE) {
        fprintf(stderr, "Initialization failed!\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window
        = glfwCreateWindow(g_win_width, g_win_height, g_win_title.c_str(), NULL, NULL);

    if (window == NULL) {
        fprintf(stderr, "Window creation failed!");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyboard_event);

    const int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        fprintf(stderr, "Failed to load OpenGL 3.x/4.x libraries!\n");
        return 1;
    }

    glfwSetWindowSizeCallback(window, resize_gl);

    g_game.init();

    print_how_to_play();

    double prev_time = glfwGetTime();
    while (glfwWindowShouldClose(window) == GL_FALSE) {
        const double current_time = glfwGetTime();
        if (current_time - prev_time >= 1.0 / FPS) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            g_game.main_loop();
            glfwSwapBuffers(window);
            glfwPollEvents();
            prev_time = current_time;
        }
    }
}
