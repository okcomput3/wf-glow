#include "glow-decoration.hpp"
#include "shaders.hpp"

#include <wayfire/core.hpp>
#include <wayfire/output.hpp>
#include <wayfire/view.hpp>
#include <wayfire/signal-definitions.hpp>
#include <wayfire/render-manager.hpp>
#include <wayfire/toplevel-view.hpp>
#include <wayfire/scene.hpp>
#include <wayfire/scene-render.hpp>
#include <wayfire/scene-operations.hpp>
#include <chrono>

namespace wf {
namespace glow_decoration {

glow_program_t g_glow_program;
glow_config_t g_config;
static std::chrono::steady_clock::time_point g_start_time = std::chrono::steady_clock::now();

// Shader compilation
bool glow_program_t::compile_shader(GLuint shader, const char* source) {
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        LOGE("Glow shader compile error: ", log);
        return false;
    }
    return true;
}

bool glow_program_t::link_program() {
    glLinkProgram(program);
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    
    if (!success) {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        LOGE("Glow program link error: ", log);
        return false;
    }
    return true;
}

bool glow_program_t::compile_shaders() {
    if (compiled) return true;
    
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    program = glCreateProgram();
    
    if (!compile_shader(vertex_shader, glow_vertex_shader)) return false;
    if (!compile_shader(fragment_shader, glow_fragment_shader)) return false;
    
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    
    if (!link_program()) return false;
    
    u_resolution = glGetUniformLocation(program, "u_resolution");
    u_border_box = glGetUniformLocation(program, "u_border_box");
    u_glow_color = glGetUniformLocation(program, "u_glow_color");
    u_glow_color_2 = glGetUniformLocation(program, "u_glow_color_2");
    u_glow_radius = glGetUniformLocation(program, "u_glow_radius");
    u_glow_intensity = glGetUniformLocation(program, "u_glow_intensity");
    u_border_width = glGetUniformLocation(program, "u_border_width");
    u_time = glGetUniformLocation(program, "u_time");
    u_enable_gradient = glGetUniformLocation(program, "u_enable_gradient");
    u_gradient_angle = glGetUniformLocation(program, "u_gradient_angle");
    u_corner_radius = glGetUniformLocation(program, "u_corner_radius");
    
    compiled = true;
    LOGI("Glow decoration shaders compiled");
    return true;
}

void glow_program_t::use() {
    glUseProgram(program);
}

void glow_program_t::destroy() {
    if (program) glDeleteProgram(program);
    if (vertex_shader) glDeleteShader(vertex_shader);
    if (fragment_shader) glDeleteShader(fragment_shader);
    program = vertex_shader = fragment_shader = 0;
    compiled = false;
}

// Render instance
class glow_render_instance_t : public wf::scene::render_instance_t {
    std::shared_ptr<glow_decoration_node_t> self;
    wf::scene::damage_callback push_damage;
    wf::signal::connection_t<wf::scene::node_damage_signal> on_damage;
    
  public:
    glow_render_instance_t(std::shared_ptr<glow_decoration_node_t> node,
                           wf::scene::damage_callback push_damage_cb,
                           wf::output_t *output)
        : self(node), push_damage(push_damage_cb) {
        
        on_damage = [=] (wf::scene::node_damage_signal *ev) {
            push_damage(ev->region);
        };
        self->connect(&on_damage);
    }
    
    void schedule_instructions(
        std::vector<wf::scene::render_instruction_t>& instructions,
        const wf::render_target_t& target,
        wf::region_t& damage) override {
        
        auto bbox = self->get_bounding_box();
        wf::region_t our_region{bbox};
        our_region &= damage;
        
        if (!our_region.empty()) {
            instructions.push_back(wf::scene::render_instruction_t{
                .instance = this,
                .target = target,
                .damage = std::move(our_region),
            });
        }
    }
    
    void render(const wf::scene::render_instruction_t& instr) override {
        auto& node = self;
        if (!node->view || !node->view->is_mapped()) {
            return;
        }
        
        if (!g_glow_program.compiled) {
            if (!g_glow_program.compile_shaders()) {
                return;
            }
        }

            // Skip if fully transparent
    if (node->opacity <= 0.0f) {
        return;
    }
        
        auto view_bbox = node->view->get_bounding_box();
        float glow_r = g_config.glow_radius;
        
        // Geometry for the quad (expanded by glow radius)
        wf::geometry_t geom = {
            static_cast<int>(view_bbox.x - glow_r),
            static_cast<int>(view_bbox.y - glow_r),
            static_cast<int>(view_bbox.width + 2 * glow_r),
            static_cast<int>(view_bbox.height + 2 * glow_r)
        };
        
        auto& target = instr.target;
        
        // Target geometry defines the viewport - coordinates are relative to this
        auto fb_geom = target.geometry;
        float fb_w = static_cast<float>(fb_geom.width);
        float fb_h = static_cast<float>(fb_geom.height);
        
        // Convert from output coordinates to framebuffer-relative coordinates
        float rel_x = static_cast<float>(geom.x - fb_geom.x);
        float rel_y = static_cast<float>(geom.y - fb_geom.y);
        float rel_w = static_cast<float>(geom.width);
        float rel_h = static_cast<float>(geom.height);
        
        // Convert to NDC (-1 to 1)
        float left   = (rel_x / fb_w) * 2.0f - 1.0f;
        float right  = ((rel_x + rel_w) / fb_w) * 2.0f - 1.0f;
        float top    = (rel_y / fb_h) * 2.0f - 1.0f;
        float bottom = ((rel_y + rel_h) / fb_h) * 2.0f - 1.0f;
        
        // Vertices in NDC
        float vertices[] = {
            left,  top,
            right, top,
            right, bottom,
            left,  bottom,
        };
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        
        g_glow_program.use();
        
        // Pass framebuffer resolution
        glUniform2f(g_glow_program.u_resolution, fb_w, fb_h);
        
        // Pass border box in framebuffer-relative coordinates
        glUniform4f(g_glow_program.u_border_box,
                    static_cast<float>(view_bbox.x - fb_geom.x),
                    static_cast<float>(view_bbox.y - fb_geom.y),
                    static_cast<float>(view_bbox.width),
                    static_cast<float>(view_bbox.height));
        
glm::vec4 color = node->is_active ? g_config.active_color : g_config.inactive_color;
color.a *= node->opacity;  // Apply fade opacity
glUniform4fv(g_glow_program.u_glow_color, 1, glm::value_ptr(color));

glm::vec4 grad_color = g_config.gradient_color_2;
grad_color.a *= node->opacity;  // Apply fade opacity to gradient too
glUniform4fv(g_glow_program.u_glow_color_2, 1, glm::value_ptr(grad_color));
        
        glUniform1f(g_glow_program.u_glow_radius, g_config.glow_radius);
        glUniform1f(g_glow_program.u_glow_intensity, g_config.glow_intensity);
        glUniform1f(g_glow_program.u_border_width, g_config.border_width);
        glUniform1f(g_glow_program.u_time, node->animation_time);
        glUniform1i(g_glow_program.u_enable_gradient, g_config.enable_gradient ? 1 : 0);
        glUniform1f(g_glow_program.u_gradient_angle, g_config.gradient_angle);
        glUniform1f(g_glow_program.u_corner_radius, g_config.corner_radius);
        
        GLuint vao, vbo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
        
        // Position attribute only (2 floats per vertex)
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        /*
        glEnable(GL_SCISSOR_TEST);
        for (auto& box : instr.damage) {
            auto sbox = wlr_box_from_pixman_box(box);
            // Convert scissor coordinates to framebuffer-relative
            int scissor_x = sbox.x - fb_geom.x;
            int scissor_y = sbox.y - fb_geom.y;
            // OpenGL scissor has Y=0 at bottom, so flip
            glScissor(scissor_x, fb_geom.height - scissor_y - sbox.height,
                      sbox.width, sbox.height);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }
        glDisable(GL_SCISSOR_TEST);
        */
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glBindVertexArray(0);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }
    
    void presentation_feedback(wf::output_t*) override {}
    void compute_visibility(wf::output_t*, wf::region_t&) override {}
};

// Node implementation
glow_decoration_node_t::glow_decoration_node_t(wayfire_view v) 
    : node_t(false), view(v) {
}

void glow_decoration_node_t::set_active(bool active) {
    if (is_active != active) {
        is_active = active;
        auto bbox = get_bounding_box();
        wf::scene::node_damage_signal ev;
        ev.region = wf::region_t{bbox};
        emit(&ev);
    }
}

void glow_decoration_node_t::set_animation_time(float time) {
    animation_time = time;
    auto bbox = get_bounding_box();
    wf::scene::node_damage_signal ev;
    ev.region = wf::region_t{bbox};
    emit(&ev);
}

std::string glow_decoration_node_t::stringify() const {
    return "glow-decoration " + (view ? view->get_title() : "null");
}

wf::geometry_t glow_decoration_node_t::get_bounding_box() {
    if (!view || !view->is_mapped()) {
        return {0, 0, 0, 0};
    }
    
    auto bbox = view->get_bounding_box();
    int expand = static_cast<int>(g_config.glow_radius + g_config.border_width);
    
    return {
        bbox.x - expand,
        bbox.y - expand,
        bbox.width + 2 * expand,
        bbox.height + 2 * expand
    };
}

void glow_decoration_node_t::gen_render_instances(
    std::vector<wf::scene::render_instance_uptr>& instances,
    wf::scene::damage_callback push_damage,
    wf::output_t *output) {
    
    // Use node_t's shared_from_this and cast
    auto self_ptr = std::dynamic_pointer_cast<glow_decoration_node_t>(
        wf::scene::node_t::shared_from_this());
    instances.push_back(std::make_unique<glow_render_instance_t>(
        self_ptr, push_damage, output));
}

// Plugin implementation
void glow_decoration_t::update_config() {
    auto to_vec4 = [](const wf::color_t& c) -> glm::vec4 {
        return glm::vec4(c.r, c.g, c.b, c.a);
    };
    
    g_config.active_color = to_vec4(opt_active_color);
    g_config.inactive_color = to_vec4(opt_inactive_color);
    g_config.glow_radius = opt_glow_radius;
    g_config.glow_intensity = opt_glow_intensity;
    g_config.border_width = opt_border_width;
    g_config.animation_speed = opt_animation_speed;
    g_config.enable_gradient = opt_enable_gradient;
    g_config.gradient_angle = opt_gradient_angle;
    g_config.gradient_color_2 = to_vec4(opt_gradient_color_2);
    g_config.corner_radius = opt_corner_radius;
    
    for (auto& [view, node] : decorations) {
        auto bbox = node->get_bounding_box();
        wf::scene::node_damage_signal ev;
        ev.region = wf::region_t{bbox};
        node->emit(&ev);
    }
}

void glow_decoration_t::update_focus() {
    for (auto& [view, node] : decorations) {
        node->set_active(view == focused_view);
    }
}

void glow_decoration_t::update_animation() {
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - g_start_time).count();
    
    const float DELAY = 1.0f;
    const float FADE_DURATION = 0.5f;
    
    for (auto& [view, node] : decorations) {
        if (view && view->is_mapped()) {
            node->set_animation_time(elapsed * g_config.animation_speed);
            
            float age = elapsed - node->creation_time;
            if (age < DELAY) {
                node->opacity = 0.0f;
            } else {
                float fade_progress = (age - DELAY) / FADE_DURATION;
                node->opacity = std::min(1.0f, fade_progress);
            }
        }
    }
}
void glow_decoration_t::add_decoration(wayfire_view view) {
    if (!view || decorations.count(view)) {
        return;
    }
    
    // Skip CSD (client-side decorated) windows
    auto toplevel = toplevel_cast(view);
    if (toplevel && !toplevel->should_be_decorated()) {
        return;
    }
    
    auto node = std::make_shared<glow_decoration_node_t>(view);
    node->set_active(view == focused_view);

    // Record creation time using global start
    auto now = std::chrono::steady_clock::now();
    node->creation_time = std::chrono::duration<float>(now - g_start_time).count();
    
    auto view_node = view->get_root_node();
    wf::scene::add_front(view_node, node);
    
    decorations[view] = node;
    
    LOGD("Added glow decoration for: ", view->get_title());
}
void glow_decoration_t::remove_decoration(wayfire_view view) {
    auto it = decorations.find(view);
    if (it != decorations.end()) {
        wf::scene::remove_child(it->second);
        decorations.erase(it);
    }
    
    if (focused_view == view) {
        focused_view = nullptr;
    }
}

void glow_decoration_t::init() {
    update_config();
    
    auto reload = [this]() { update_config(); };
    
    opt_active_color.set_callback(reload);
    opt_inactive_color.set_callback(reload);
    opt_glow_radius.set_callback(reload);
    opt_glow_intensity.set_callback(reload);
    opt_border_width.set_callback(reload);
    opt_animation_speed.set_callback(reload);
    opt_enable_gradient.set_callback(reload);
    opt_gradient_angle.set_callback(reload);
    opt_gradient_color_2.set_callback(reload);
    opt_corner_radius.set_callback(reload);
    
    on_view_mapped = [this](wf::view_mapped_signal *ev) {
        if (toplevel_cast(ev->view)) {
            add_decoration(ev->view);
        }
    };
    output->connect(&on_view_mapped);
    
    on_view_unmapped = [this](wf::view_unmapped_signal *ev) {
        remove_decoration(ev->view);
    };
    output->connect(&on_view_unmapped);
    
    on_focus_request = [this](wf::view_focus_request_signal *ev) {
        focused_view = ev->view;
        update_focus();
    };
    output->connect(&on_focus_request);
    
    for (auto& view : wf::get_core().get_all_views()) {
        if (view->get_output() == output && toplevel_cast(view)) {
            add_decoration(view);
        }
    }
    
    auto loop = wl_display_get_event_loop(wf::get_core().display);
    animation_timer = wl_event_loop_add_timer(loop, [](void *data) -> int {
        auto self = static_cast<glow_decoration_t*>(data);
        self->update_animation();
        wl_event_source_timer_update(self->animation_timer, 16);
        return 0;
    }, this);
    wl_event_source_timer_update(animation_timer, 16);
    
    LOGI("Glow decoration plugin initialized");
}

void glow_decoration_t::fini() {
    if (animation_timer) {
        wl_event_source_remove(animation_timer);
        animation_timer = nullptr;
    }
    
    for (auto& [view, node] : decorations) {
        wf::scene::remove_child(node);
    }
    decorations.clear();
    
    if (g_glow_program.compiled) {
        g_glow_program.destroy();
    }
    
    LOGI("Glow decoration plugin finalized");
}

DECLARE_WAYFIRE_PLUGIN(wf::per_output_plugin_t<glow_decoration_t>);

} // namespace glow_decoration
} // namespace wf
