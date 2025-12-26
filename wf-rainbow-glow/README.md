# Wayfire Glow Decoration Plugin

A Hyprland-style glowing window borders plugin for Wayfire compositor, implemented in GLSL.

![Glow Effect Preview](preview.png)

## Features

- **Glowing borders**: Soft, customizable glow effect around windows
- **Active/Inactive colors**: Different colors for focused and unfocused windows
- **Gradient support**: Smooth color gradients with configurable angle
- **Rounded corners**: Respects window corner radius
- **Animated effects**: Subtle pulsing and gradient animations
- **GPU-accelerated**: Pure GLSL shader implementation for smooth performance

## Requirements

- Wayfire >= 0.8.0
- wlroots >= 0.17
- meson >= 0.51.0
- GLM (OpenGL Mathematics library)
- OpenGL ES 3.0+

## Building

```bash
# Clone the repository
git clone https://github.com/yourusername/wayfire-glow-decoration.git
cd wayfire-glow-decoration

# Build with meson
meson setup build
ninja -C build

# Install
sudo ninja -C build install
```

## Configuration

Add to your `~/.config/wayfire.ini`:

```ini
[core]
plugins = ... glow-decoration

[glow-decoration]
# Primary glow color for active windows (Hyprland blue)
active_color = 0.33 0.55 0.83 1.0

# Glow color for inactive windows
inactive_color = 0.4 0.4 0.45 1.0

# Secondary color for gradient effect
gradient_color_2 = 0.55 0.33 0.83 1.0

# How far the glow extends (pixels)
glow_radius = 8.0

# Glow brightness (0.1 - 2.0)
glow_intensity = 0.7

# Solid border width (pixels)
border_width = 2.0

# Corner radius (pixels)
corner_radius = 10.0

# Animation speed multiplier
animation_speed = 2.0

# Enable gradient between two colors
enable_gradient = true

# Gradient angle in degrees
gradient_angle = 45.0
```

## Color Presets

### Hyprland Default (Blue)
```ini
active_color = 0.33 0.55 0.83 1.0
gradient_color_2 = 0.55 0.33 0.83 1.0
```

### Catppuccin Mocha (Mauve)
```ini
active_color = 0.80 0.52 0.90 1.0
gradient_color_2 = 0.95 0.55 0.66 1.0
```

### Nord
```ini
active_color = 0.53 0.75 0.82 1.0
gradient_color_2 = 0.71 0.56 0.68 1.0
```

### Tokyo Night
```ini
active_color = 0.48 0.51 0.78 1.0
gradient_color_2 = 0.73 0.49 0.72 1.0
```

### Dracula
```ini
active_color = 0.74 0.58 0.98 1.0
gradient_color_2 = 1.0 0.47 0.66 1.0
```

### Gruvbox
```ini
active_color = 0.98 0.74 0.18 1.0
gradient_color_2 = 0.92 0.36 0.16 1.0
```

### Pure Rainbow (animated)
To use the rainbow shader variant, you'll need to modify the source to use `glow_rainbow_fragment_shader` instead of `glow_fragment_shader` in the compilation step.

## How It Works

The plugin uses a GLSL fragment shader that:

1. Calculates the signed distance from each pixel to a rounded rectangle representing the window
2. Renders a solid border at the edge of the window
3. Applies an exponential falloff glow effect beyond the border
4. Optionally blends two colors in a gradient pattern
5. Adds subtle animation effects (pulsing, gradient movement)

The shader runs on the GPU for maximum performance and smooth 60fps+ rendering.

## Tips

- Set `glow_intensity` to 0 to disable the soft glow and only show solid borders
- Set `border_width` to 0 to only show the glow without a solid line
- Set `animation_speed` to 0 to disable all animations
- Higher `glow_radius` values create a more diffuse, softer glow
- Use complementary colors for `active_color` and `gradient_color_2` for vibrant effects

## Troubleshooting

**Plugin doesn't load:**
- Check that Wayfire can find the plugin: `ls $(pkg-config --variable=plugindir wayfire)`
- Verify the plugin is listed in `[core] plugins`

**No glow visible:**
- Increase `glow_intensity` and `glow_radius`
- Check that `active_color` alpha is not 0

**Performance issues:**
- Reduce `glow_radius` for less fragment shader work
- Disable `enable_gradient` for simpler color calculations

## License

MIT License - See LICENSE file for details.

## Credits

Inspired by Hyprland's beautiful glowing borders effect.
