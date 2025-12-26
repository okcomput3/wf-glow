#pragma once

#include <wayfire/plugin.hpp>
#include <wayfire/output.hpp>
#include <wayfire/view.hpp>
#include <wayfire/scene.hpp>
#include <wayfire/signal-definitions.hpp>
#include <wayfire/per-output-plugin.hpp>
#include <GLES3/gl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <map>
#include <memory>
#include <chrono>

namespace wf {
namespace glow_decoration {

struct glow_program_t {
    GLuint program = 0;
    GLuint vertex_shader = 0;
    GLuint fragment_shader = 0;
    bool compiled = false;
    
    // Uniform locations
    GLint u_resolution = -1;
    GLint u_border_box = -1;
    GLint u_glow_color = -1;
    GLint u_glow_color_2 = -1;
    GLint u_glow_radius = -1;
    GLint u_glow_intensity = -1;
    GLint u_border_width = -1;
    GLint u_time = -1;
    GLint u_enable_gradient = -1;
    GLint u_gradient_angle = -1;
    GLint u_corner_radius = -1;
    
    bool compile_shader(GLuint shader, const char* source);
    bool link_program();
    bool compile_shaders();
    void use();
    void destroy();
};

struct glow_config_t {
    glm::vec4 active_color{1.0f, 0.5f, 0.0f, 1.0f};
    glm::vec4 inactive_color{0.3f, 0.3f, 0.3f, 1.0f};
    glm::vec4 gradient_color_2{0.0f, 0.5f, 1.0f, 1.0f};
    float glow_radius = 20.0f;
    float glow_intensity = 1.0f;
    float border_width = 2.0f;
    float animation_speed = 1.0f;
    bool enable_gradient = false;
    float gradient_angle = 45.0f;
    float corner_radius = 10.0f;
};

extern glow_program_t g_glow_program;
extern glow_config_t g_config;

/**
 * The glow decoration render node.
 * Inherits from node_t and renders the glow effect.
 */
class glow_decoration_node_t : public wf::scene::node_t {
  public:
    wayfire_view view;
    bool is_active = false;
    float animation_time = 0.0f;
    
    // Track previous bounding box for damage when geometry changes
    wf::geometry_t prev_bbox{0, 0, 0, 0};
    
    // Signal connection for geometry changes
    wf::signal::connection_t<wf::view_geometry_changed_signal> on_geometry_changed;
    
    glow_decoration_node_t(wayfire_view v);
    
    void set_active(bool active);
    void set_animation_time(float time);
    
    std::string stringify() const override;
    wf::geometry_t get_bounding_box() override;
    void gen_render_instances(
        std::vector<wf::scene::render_instance_uptr>& instances,
        wf::scene::damage_callback push_damage,
        wf::output_t *output) override;
};

class glow_decoration_t : public wf::per_output_plugin_instance_t {
  public:
    void init() override;
    void fini() override;
    
  private:
    wf::option_wrapper_t<wf::color_t> opt_active_color{"glow-decoration/active_color"};
    wf::option_wrapper_t<wf::color_t> opt_inactive_color{"glow-decoration/inactive_color"};
    wf::option_wrapper_t<double> opt_glow_radius{"glow-decoration/glow_radius"};
    wf::option_wrapper_t<double> opt_glow_intensity{"glow-decoration/glow_intensity"};
    wf::option_wrapper_t<double> opt_border_width{"glow-decoration/border_width"};
    wf::option_wrapper_t<double> opt_animation_speed{"glow-decoration/animation_speed"};
    wf::option_wrapper_t<bool> opt_enable_gradient{"glow-decoration/enable_gradient"};
    wf::option_wrapper_t<double> opt_gradient_angle{"glow-decoration/gradient_angle"};
    wf::option_wrapper_t<wf::color_t> opt_gradient_color_2{"glow-decoration/gradient_color_2"};
    wf::option_wrapper_t<double> opt_corner_radius{"glow-decoration/corner_radius"};
    
    std::map<wayfire_view, std::shared_ptr<glow_decoration_node_t>> decorations;
    wayfire_view focused_view = nullptr;
    wl_event_source *animation_timer = nullptr;
    
    wf::signal::connection_t<wf::view_mapped_signal> on_view_mapped;
    wf::signal::connection_t<wf::view_unmapped_signal> on_view_unmapped;
    wf::signal::connection_t<wf::view_focus_request_signal> on_focus_request;
    
    void update_config();
    void update_focus();
    void update_animation();
    void add_decoration(wayfire_view view);
    void remove_decoration(wayfire_view view);
};

} // namespace glow_decoration
} // namespace wf