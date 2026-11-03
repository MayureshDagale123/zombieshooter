#ifndef CG_ALGORITHMS_HPP
#define CG_ALGORITHMS_HPP

#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <iostream>

// =======================================================================
// COMPUTER GRAPHICS CORE PRIMITIVES AND ALGORITHMS
// Specially written for academic understanding & demonstration
// Algorithms included:
//   1. DDA Line Drawing Algorithm
//   2. Bresenham's Line Drawing Algorithm
//   3. Bresenham's / Midpoint Circle Drawing Algorithm
//   4. 2D Transformations (Translation, Rotation, Scaling)
//   5. Scanline Polygon Fill Algorithm
//   6. Boundary Fill & Flood Fill Algorithms
//   7. Cohen-Sutherland Line Clipping (Helper)
// =======================================================================

// Color representation (32-bit ARGB / XRGB format)
struct Color {
    uint8_t b, g, r, a;

    Color() : b(0), g(0), r(0), a(255) {}
    Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
        : b(blue), g(green), r(red), a(alpha) {}

    static Color Black()       { return Color(0, 0, 0); }
    static Color White()       { return Color(255, 255, 255); }
    static Color Red()         { return Color(255, 50, 50); }
    static Color Green()       { return Color(50, 255, 50); }
    static Color Blue()        { return Color(60, 140, 255); }
    static Color Yellow()      { return Color(255, 230, 0); }
    static Color Cyan()        { return Color(0, 240, 255); }
    static Color Magenta()     { return Color(255, 0, 200); }
    static Color Orange()      { return Color(255, 140, 0); }
    static Color DarkGray()    { return Color(30, 32, 40); }
    static Color MidGray()     { return Color(80, 85, 95); }
    static Color LightGray()   { return Color(180, 185, 195); }
    static Color ZombieSkin()  { return Color(85, 170, 85); }
    static Color ZombieDark()  { return Color(45, 110, 45); }
    static Color BloodRed()    { return Color(200, 20, 20); }
    static Color GunMetal()    { return Color(110, 120, 135); }
    static Color NeonCyan()    { return Color(20, 255, 240); }
    static Color NeonOrange()  { return Color(255, 120, 20); }

    bool operator==(const Color& other) const {
        return r == other.r && g == other.g && b == other.b;
    }
    bool operator!=(const Color& other) const {
        return !(*this == other);
    }
};

// 2D Point structure
struct Point2D {
    float x;
    float y;

    Point2D() : x(0.0f), y(0.0f) {}
    Point2D(float _x, float _y) : x(_x), y(_y) {}
};

// Framebuffer class for direct pixel manipulation
class Framebuffer {
public:
    int width;
    int height;
    std::vector<uint32_t> pixels; // 0x00RRGGBB format for Win32 GDI DIBSection

    Framebuffer(int w, int h) : width(w), height(h), pixels(w * h, 0) {}

    void clear(Color color = Color::Black()) {
        uint32_t c = (color.r << 16) | (color.g << 8) | color.b;
        std::fill(pixels.begin(), pixels.end(), c);
    }

    // Basic putpixel function with boundary clipping
    inline void put_pixel(int x, int y, Color color) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            pixels[y * width + x] = (color.r << 16) | (color.g << 8) | color.b;
        }
    }

    inline Color get_pixel(int x, int y) const {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            uint32_t p = pixels[y * width + x];
            return Color((p >> 16) & 0xFF, (p >> 8) & 0xFF, p & 0xFF);
        }
        return Color::Black();
    }
};

namespace CG {

    // ===================================================================
    // 1. DDA (DIGITAL DIFFERENTIAL ANALYZER) LINE ALGORITHM
    // Concept:
    //   Computes step = max(|dx|, |dy|)
    //   x_inc = dx / step, y_inc = dy / step
    //   Plots pixel at rounded (x, y) at each step.
    // ===================================================================
    inline void draw_line_dda(Framebuffer& fb, int x1, int y1, int x2, int y2, Color color) {
        int dx = x2 - x1;
        int dy = y2 - y1;

        int steps = std::max(std::abs(dx), std::abs(dy));

        if (steps == 0) {
            fb.put_pixel(x1, y1, color);
            return;
        }

        float x_inc = static_cast<float>(dx) / static_cast<float>(steps);
        float y_inc = static_cast<float>(dy) / static_cast<float>(steps);

        float x = static_cast<float>(x1);
        float y = static_cast<float>(y1);

        for (int i = 0; i <= steps; ++i) {
            fb.put_pixel(static_cast<int>(std::round(x)), static_cast<int>(std::round(y)), color);
            x += x_inc;
            y += y_inc;
        }
    }

    // ===================================================================
    // 2. BRESENHAM'S LINE DRAWING ALGORITHM (Generalized for all 8 octants)
    // Concept:
    //   Integer-only math for maximum precision and speed.
    //   Uses a decision parameter 'pk' to choose between adjacent pixels.
    // ===================================================================
    inline void draw_line_bresenham(Framebuffer& fb, int x1, int y1, int x2, int y2, Color color) {
        int dx = std::abs(x2 - x1);
        int dy = std::abs(y2 - y1);
        int sx = (x1 < x2) ? 1 : -1;
        int sy = (y1 < y2) ? 1 : -1;
        int err = dx - dy;

        while (true) {
            fb.put_pixel(x1, y1, color);

            if (x1 == x2 && y1 == y2) break;

            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x1 += sx;
            }
            if (e2 < dx) {
                err += dx;
                y1 += sy;
            }
        }
    }

    // ===================================================================
    // 3. BRESENHAM'S / MIDPOINT CIRCLE ALGORITHM
    // Concept:
    //   Uses 8-way symmetry: for every point (x, y) on circle centered at origin,
    //   there are 8 symmetric points: (x,y), (y,x), (-x,y), (-y,x),
    //   (-x,-y), (-y,-x), (x,-y), (y,-x).
    //   Decision parameter P_k = 1 - r (or 3 - 2r in Bresenham formulation).
    // ===================================================================
    inline void draw_circle_bresenham(Framebuffer& fb, int xc, int yc, int radius, Color color) {
        int x = 0;
        int y = radius;
        int d = 3 - 2 * radius; // Initial decision parameter

        auto plot8 = [&](int px, int py) {
            fb.put_pixel(xc + px, yc + py, color);
            fb.put_pixel(xc - px, yc + py, color);
            fb.put_pixel(xc + px, yc - py, color);
            fb.put_pixel(xc - px, yc - py, color);
            fb.put_pixel(xc + py, yc + px, color);
            fb.put_pixel(xc - py, yc + px, color);
            fb.put_pixel(xc + py, yc - px, color);
            fb.put_pixel(xc - py, yc - px, color);
        };

        plot8(x, y);

        while (y >= x) {
            x++;
            if (d > 0) {
                y--;
                d = d + 4 * (x - y) + 10;
            } else {
                d = d + 4 * x + 6;
            }
            plot8(x, y);
        }
    }

    // Filled circle helper using horizontal scan spans (built from Bresenham circle symmetry)
    inline void fill_circle_bresenham(Framebuffer& fb, int xc, int yc, int radius, Color color) {
        int x = 0;
        int y = radius;
        int d = 3 - 2 * radius;

        auto draw_span = [&](int y_val, int x1, int x2) {
            if (x1 > x2) std::swap(x1, x2);
            for (int curr_x = x1; curr_x <= x2; ++curr_x) {
                fb.put_pixel(curr_x, y_val, color);
            }
        };

        while (y >= x) {
            draw_span(yc + y, xc - x, xc + x);
            draw_span(yc - y, xc - x, xc + x);
            draw_span(yc + x, xc - y, xc + y);
            draw_span(yc - x, xc - y, xc + y);

            x++;
            if (d > 0) {
                y--;
                d = d + 4 * (x - y) + 10;
            } else {
                d = d + 4 * x + 6;
            }
        }
    }

    // ===================================================================
    // 4. 2D TRANSFORMATIONS (Translation, Rotation, Scaling)
    // All transformations use standard affine 2D mathematics.
    // ===================================================================

    // Point Translation: x' = x + tx, y' = y + ty
    inline Point2D translate(const Point2D& p, float tx, float ty) {
        return Point2D(p.x + tx, p.y + ty);
    }

    // Point Rotation around a pivot (xc, yc) by angle theta (in radians):
    // x' = xc + (x - xc)*cos(theta) - (y - yc)*sin(theta)
    // y' = yc + (x - xc)*sin(theta) + (y - yc)*cos(theta)
    inline Point2D rotate(const Point2D& p, const Point2D& pivot, float angle_rad) {
        float s = std::sin(angle_rad);
        float c = std::cos(angle_rad);

        float dx = p.x - pivot.x;
        float dy = p.y - pivot.y;

        return Point2D(
            pivot.x + (dx * c - dy * s),
            pivot.y + (dx * s + dy * c)
        );
    }

    // Point Scaling relative to a fixed point (pivot):
    // x' = xc + (x - xc)*sx
    // y' = yc + (y - yc)*sy
    inline Point2D scale(const Point2D& p, const Point2D& pivot, float sx, float sy) {
        return Point2D(
            pivot.x + (p.x - pivot.x) * sx,
            pivot.y + (p.y - pivot.y) * sy
        );
    }

    // Polygon Translation
    inline std::vector<Point2D> translate_polygon(const std::vector<Point2D>& poly, float tx, float ty) {
        std::vector<Point2D> result;
        result.reserve(poly.size());
        for (const auto& pt : poly) {
            result.push_back(translate(pt, tx, ty));
        }
        return result;
    }

    // Polygon Rotation around pivot
    inline std::vector<Point2D> rotate_polygon(const std::vector<Point2D>& poly, const Point2D& pivot, float angle_rad) {
        std::vector<Point2D> result;
        result.reserve(poly.size());
        for (const auto& pt : poly) {
            result.push_back(rotate(pt, pivot, angle_rad));
        }
        return result;
    }

    // Polygon Scaling around pivot
    inline std::vector<Point2D> scale_polygon(const std::vector<Point2D>& poly, const Point2D& pivot, float sx, float sy) {
        std::vector<Point2D> result;
        result.reserve(poly.size());
        for (const auto& pt : poly) {
            result.push_back(scale(pt, pivot, sx, sy));
        }
        return result;
    }

    // Draw Polygon Outline using Bresenham lines
    inline void draw_polygon(Framebuffer& fb, const std::vector<Point2D>& poly, Color color) {
        if (poly.size() < 2) return;
        for (size_t i = 0; i < poly.size(); ++i) {
            size_t next = (i + 1) % poly.size();
            draw_line_bresenham(fb, 
                static_cast<int>(std::round(poly[i].x)), 
                static_cast<int>(std::round(poly[i].y)),
                static_cast<int>(std::round(poly[next].x)), 
                static_cast<int>(std::round(poly[next].y)), 
                color);
        }
    }

    // ===================================================================
    // 5. SCANLINE POLYGON FILL ALGORITHM
    // Concept:
    //   1. Find y_min and y_max of polygon vertices.
    //   2. For each scanline from y_min to y_max:
    //      - Find intersections with all polygon edges.
    //      - Sort intersection x-coordinates in ascending order.
    //      - Fill pixels between pairs of intersections: (x0, x1), (x2, x3), etc.
    // ===================================================================
    inline void fill_polygon_scanline(Framebuffer& fb, const std::vector<Point2D>& poly, Color color) {
        if (poly.size() < 3) return;

        // Step 1: Find bounding y range
        float ymin_f = poly[0].y;
        float ymax_f = poly[0].y;
        for (size_t i = 1; i < poly.size(); ++i) {
            if (poly[i].y < ymin_f) ymin_f = poly[i].y;
            if (poly[i].y > ymax_f) ymax_f = poly[i].y;
        }

        int ymin = static_cast<int>(std::ceil(ymin_f));
        int ymax = static_cast<int>(std::floor(ymax_f));

        // Clip to framebuffer height
        ymin = std::max(0, ymin);
        ymax = std::min(fb.height - 1, ymax);

        // Step 2: Loop over every scanline y
        for (int y = ymin; y <= ymax; ++y) {
            std::vector<float> intersections;

            for (size_t i = 0; i < poly.size(); ++i) {
                size_t next = (i + 1) % poly.size();
                Point2D p1 = poly[i];
                Point2D p2 = poly[next];

                // Ensure p1 is lower y than p2
                if (p1.y > p2.y) std::swap(p1, p2);

                // Check if current scanline intersects edge (half-open interval [p1.y, p2.y) to handle vertices)
                if (y >= p1.y && y < p2.y) {
                    if (std::abs(p2.y - p1.y) > 0.0001f) {
                        float x = p1.x + (static_cast<float>(y) - p1.y) * (p2.x - p1.x) / (p2.y - p1.y);
                        intersections.push_back(x);
                    }
                }
            }

            // Step 3: Sort intersections ascending
            std::sort(intersections.begin(), intersections.end());

            // Step 4: Fill between consecutive pairs of intersections (Parity rule)
            for (size_t i = 0; i + 1 < intersections.size(); i += 2) {
                int x_start = static_cast<int>(std::ceil(intersections[i]));
                int x_end   = static_cast<int>(std::floor(intersections[i + 1]));

                // Clip to framebuffer width
                x_start = std::max(0, x_start);
                x_end   = std::min(fb.width - 1, x_end);

                for (int x = x_start; x <= x_end; ++x) {
                    fb.put_pixel(x, y, color);
                }
            }
        }
    }

    // ===================================================================
    // 6. BOUNDARY FILL & FLOOD FILL ALGORITHMS (Stack-based for safety)
    // Concept:
    //   Boundary fill: Replaces pixels with fill_color until boundary_color is met.
    //   Flood fill: Replaces pixels of old_color with new_color.
    // ===================================================================
    inline void boundary_fill_4(Framebuffer& fb, int start_x, int start_y, Color fill_color, Color boundary_color) {
        if (start_x < 0 || start_x >= fb.width || start_y < 0 || start_y >= fb.height) return;

        Color current = fb.get_pixel(start_x, start_y);
        if (current == fill_color || current == boundary_color) return;

        std::vector<std::pair<int, int>> stack;
        fb.put_pixel(start_x, start_y, fill_color);
        stack.push_back({start_x, start_y});

        while (!stack.empty()) {
            auto [x, y] = stack.back();
            stack.pop_back();

            const int dx[4] = {1, -1, 0, 0};
            const int dy[4] = {0, 0, 1, -1};

            for (int i = 0; i < 4; ++i) {
                int nx = x + dx[i];
                int ny = y + dy[i];

                if (nx >= 0 && nx < fb.width && ny >= 0 && ny < fb.height) {
                    Color c = fb.get_pixel(nx, ny);
                    if (c != boundary_color && c != fill_color) {
                        fb.put_pixel(nx, ny, fill_color);
                        stack.push_back({nx, ny});
                    }
                }
            }
        }
    }

    inline void flood_fill_4(Framebuffer& fb, int start_x, int start_y, Color new_color, Color old_color) {
        if (start_x < 0 || start_x >= fb.width || start_y < 0 || start_y >= fb.height) return;
        if (new_color == old_color) return;
        if (fb.get_pixel(start_x, start_y) != old_color) return;

        std::vector<std::pair<int, int>> stack;
        fb.put_pixel(start_x, start_y, new_color);
        stack.push_back({start_x, start_y});

        while (!stack.empty()) {
            auto [x, y] = stack.back();
            stack.pop_back();

            const int dx[4] = {1, -1, 0, 0};
            const int dy[4] = {0, 0, 1, -1};

            for (int i = 0; i < 4; ++i) {
                int nx = x + dx[i];
                int ny = y + dy[i];

                if (nx >= 0 && nx < fb.width && ny >= 0 && ny < fb.height) {
                    if (fb.get_pixel(nx, ny) == old_color) {
                        fb.put_pixel(nx, ny, new_color);
                        stack.push_back({nx, ny});
                    }
                }
            }
        }
    }

} // namespace CG

#endif // CG_ALGORITHMS_HPP
