#ifndef GAME_HPP
#define GAME_HPP

#include "CG_Algorithms.hpp"
#include "FontRenderer.hpp"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include <sstream>

// =======================================================================
// ZOMBIE SHOOTER GAME ENGINE
// Implements complete gameplay mechanics with explicit Computer Graphics
// concepts: DDA, Bresenham, 2D Transformations, and Polygon Filling.
// =======================================================================

// Particle structure for blood / explosion effects
struct Particle {
    Point2D pos;
    float vx, vy;
    float life;      // 0.0 to 1.0
    float decay;
    Color color;
    int size;

    void update(float dt) {
        // 2D Translation: pos = pos + velocity * dt
        pos = CG::translate(pos, vx * dt, vy * dt);
        life -= decay * dt;
    }
};

// Bullet fired by the player
struct Bullet {
    Point2D pos;
    Point2D prev_pos; // For drawing DDA motion blur / tracer line
    float vx, vy;
    bool active;
    float radius;

    Bullet(Point2D p, float angle_rad, float speed = 750.0f)
        : pos(p), prev_pos(p), active(true), radius(3.5f) {
        // Velocity vector derived from rotation angle
        vx = std::cos(angle_rad) * speed;
        vy = std::sin(angle_rad) * speed;
    }

    void update(float dt, int screen_width) {
        prev_pos = pos;
        // 2D Translation of bullet
        pos = CG::translate(pos, vx * dt, vy * dt);

        if (pos.x > screen_width + 20 || pos.y < -20 || pos.y > 700) {
            active = false;
        }
    }

    void draw(Framebuffer& fb) const {
        if (!active) return;

        // ALGO: DDA Line used to draw laser bullet trail / tracer
        CG::draw_line_dda(fb, 
            static_cast<int>(prev_pos.x), static_cast<int>(prev_pos.y),
            static_cast<int>(pos.x), static_cast<int>(pos.y),
            Color::NeonOrange());

        // ALGO: Bresenham Circle used for glowing bullet tip
        CG::fill_circle_bresenham(fb, 
            static_cast<int>(pos.x), static_cast<int>(pos.y), 
            static_cast<int>(radius), Color::Yellow());
    }
};

// Zombie enemy walking from right to left
class Zombie {
public:
    Point2D pos;          // Center position
    float speed;
    int health;
    bool active;
    float walk_cycle;     // For limb rotation animation
    float scale_factor;

    Zombie(float start_x, float start_y, float spd = 65.0f)
        : pos(start_x, start_y), speed(spd), health(1), active(true),
          walk_cycle(static_cast<float>(rand() % 100)), scale_factor(1.0f) {}

    void update(float dt) {
        // ALGO: 2D Translation (moves horizontally towards left)
        pos = CG::translate(pos, -speed * dt, 0.0f);
        walk_cycle += dt * 6.0f;
    }

    void draw(Framebuffer& fb) const {
        if (!active) return;

        int cx = static_cast<int>(pos.x);
        int cy = static_cast<int>(pos.y);

        // 1. ZOMBIE TORSO: Scanline Polygon Fill
        // Define base 4-vertex polygon around (0,0)
        std::vector<Point2D> torso_local = {
            Point2D(-12.0f, -18.0f),
            Point2D( 12.0f, -18.0f),
            Point2D( 10.0f,  20.0f),
            Point2D(-10.0f,  20.0f)
        };

        // ALGO: 2D Translation of torso polygon to zombie position
        std::vector<Point2D> torso_world = CG::translate_polygon(torso_local, pos.x, pos.y);

        // ALGO: Scanline Polygon Fill Algorithm fills the zombie torso
        CG::fill_polygon_scanline(fb, torso_world, Color::ZombieDark());
        // Outline drawn with Bresenham line
        CG::draw_polygon(fb, torso_world, Color::ZombieSkin());

        // 2. ZOMBIE HEAD: Bresenham Circle
        int head_y = cy - 28;
        int head_radius = 12;
        // ALGO: Filled Bresenham circle for head
        CG::fill_circle_bresenham(fb, cx, head_y, head_radius, Color::ZombieSkin());
        CG::draw_circle_bresenham(fb, cx, head_y, head_radius, Color::Black());

        // Zombie Glowing Red Eyes (using Bresenham circles)
        CG::fill_circle_bresenham(fb, cx - 6, head_y - 2, 2, Color::Red());
        CG::fill_circle_bresenham(fb, cx - 1, head_y - 2, 2, Color::Red());

        // 3. ZOMBIE ARMS: Outstretched forward with 2D Rotation wobble
        float arm_angle = std::sin(walk_cycle) * 0.15f; // Slight menacing swing
        Point2D shoulder_left(pos.x - 6.0f, pos.y - 10.0f);
        Point2D hand_left(pos.x - 32.0f, pos.y - 8.0f);

        // ALGO: 2D Rotation applied to arm
        Point2D rotated_hand = CG::rotate(hand_left, shoulder_left, arm_angle);
        // ALGO: Bresenham Line for drawing outstretched arms
        CG::draw_line_bresenham(fb, 
            static_cast<int>(shoulder_left.x), static_cast<int>(shoulder_left.y),
            static_cast<int>(rotated_hand.x), static_cast<int>(rotated_hand.y),
            Color::ZombieSkin());

        // 4. ZOMBIE LEGS: Walking animation using 2D Rotation
        float leg_swing = std::sin(walk_cycle) * 0.35f;
        Point2D hip_left(pos.x - 5.0f, pos.y + 20.0f);
        Point2D foot_left(pos.x - 5.0f, pos.y + 40.0f);
        Point2D rotated_foot_1 = CG::rotate(foot_left, hip_left, leg_swing);

        Point2D hip_right(pos.x + 5.0f, pos.y + 20.0f);
        Point2D foot_right(pos.x + 5.0f, pos.y + 40.0f);
        Point2D rotated_foot_2 = CG::rotate(foot_right, hip_right, -leg_swing);

        // ALGO: Bresenham Line for legs
        CG::draw_line_bresenham(fb, 
            static_cast<int>(hip_left.x), static_cast<int>(hip_left.y),
            static_cast<int>(rotated_foot_1.x), static_cast<int>(rotated_foot_1.y),
            Color::ZombieDark());

        CG::draw_line_bresenham(fb, 
            static_cast<int>(hip_right.x), static_cast<int>(hip_right.y),
            static_cast<int>(rotated_foot_2.x), static_cast<int>(rotated_foot_2.y),
            Color::ZombieDark());
    }

    // Check hit test against bullet
    bool check_collision(const Point2D& bullet_pos, float bullet_rad) const {
        if (!active) return false;
        float dx = bullet_pos.x - pos.x;
        float dy = bullet_pos.y - pos.y;
        float dist_sq = dx * dx + dy * dy;
        float hit_rad = 24.0f + bullet_rad;
        return dist_sq <= (hit_rad * hit_rad);
    }
};

// Player standing on the left side behind the defense line
class Player {
public:
    Point2D pos;          // Base position (X is fixed behind line, Y can move)
    float aim_angle;      // Gun angle in radians
    float muzzle_timer;   // Flash timer after firing
    float speed;

    Player(float x, float y)
        : pos(x, y), aim_angle(0.0f), muzzle_timer(0.0f), speed(220.0f) {}

    void move_up(float dt, float min_y = 120.0f) {
        // ALGO: 2D Translation
        pos.y -= speed * dt;
        if (pos.y < min_y) pos.y = min_y;
    }

    void move_down(float dt, float max_y = 520.0f) {
        // ALGO: 2D Translation
        pos.y += speed * dt;
        if (pos.y > max_y) pos.y = max_y;
    }

    void aim_up(float dt) {
        aim_angle -= 1.8f * dt;
        if (aim_angle < -0.785f) aim_angle = -0.785f; // Limit to ~ -45 degrees
    }

    void aim_down(float dt) {
        aim_angle += 1.8f * dt;
        if (aim_angle > 0.785f) aim_angle = 0.785f;  // Limit to ~ +45 degrees
    }

    void reset_aim() {
        aim_angle = 0.0f;
    }

    void update(float dt) {
        if (muzzle_timer > 0.0f) {
            muzzle_timer -= dt;
        }
    }

    // Calculates the exact tip of the gun barrel using 2D Rotation and Translation
    Point2D get_gun_tip() const {
        Point2D shoulder(pos.x + 8.0f, pos.y - 6.0f);
        Point2D unrotated_tip(pos.x + 46.0f, pos.y - 6.0f);

        // ALGO: 2D Rotation about shoulder pivot
        return CG::rotate(unrotated_tip, shoulder, aim_angle);
    }

    void draw(Framebuffer& fb) const {
        int cx = static_cast<int>(pos.x);
        int cy = static_cast<int>(pos.y);

        // 1. PLAYER TORSO: Scanline Filled Polygon
        std::vector<Point2D> torso_local = {
            Point2D(-10.0f, -16.0f),
            Point2D( 12.0f, -16.0f),
            Point2D(  8.0f,  18.0f),
            Point2D(-10.0f,  18.0f)
        };
        // ALGO: 2D Translation
        std::vector<Point2D> torso_world = CG::translate_polygon(torso_local, pos.x, pos.y);
        // ALGO: Scanline Fill Polygon
        CG::fill_polygon_scanline(fb, torso_world, Color::Blue());
        CG::draw_polygon(fb, torso_world, Color::Cyan());

        // 2. PLAYER HEAD: Bresenham Circle
        int head_y = cy - 26;
        int head_r = 11;
        // ALGO: Bresenham Circle Fill and Outline
        CG::fill_circle_bresenham(fb, cx, head_y, head_r, Color(240, 200, 160));
        CG::draw_circle_bresenham(fb, cx, head_y, head_r, Color::White());

        // Tactical Visor / Eyes
        CG::draw_line_dda(fb, cx, head_y - 2, cx + 7, head_y - 2, Color::NeonCyan());

        // 3. PLAYER LEGS: Bresenham Line
        CG::draw_line_bresenham(fb, cx - 6, cy + 18, cx - 10, cy + 42, Color::MidGray());
        CG::draw_line_bresenham(fb, cx + 4, cy + 18, cx + 8,  cy + 42, Color::MidGray());

        // 4. GUN & ARM: 2D Rotation about shoulder pivot
        Point2D shoulder(pos.x + 8.0f, pos.y - 6.0f);

        // Gun barrel model (rectangle polygon)
        std::vector<Point2D> gun_local = {
            Point2D( 0.0f, -4.0f),
            Point2D(36.0f, -4.0f),
            Point2D(36.0f,  4.0f),
            Point2D( 0.0f,  4.0f)
        };

        // ALGO: 2D Translation to shoulder, then 2D Rotation around shoulder
        std::vector<Point2D> gun_positioned = CG::translate_polygon(gun_local, shoulder.x, shoulder.y);
        std::vector<Point2D> gun_rotated    = CG::rotate_polygon(gun_positioned, shoulder, aim_angle);

        // ALGO: Scanline Polygon Fill for Gun
        CG::fill_polygon_scanline(fb, gun_rotated, Color::GunMetal());
        CG::draw_polygon(fb, gun_rotated, Color::LightGray());

        // Gun stock / grip
        CG::draw_line_bresenham(fb, 
            static_cast<int>(shoulder.x), static_cast<int>(shoulder.y),
            static_cast<int>(shoulder.x - 4), static_cast<int>(shoulder.y + 12),
            Color::GunMetal());

        // 5. MUZZLE FLASH: 2D Scaling of triangular flash polygon
        if (muzzle_timer > 0.0f) {
            Point2D tip = get_gun_tip();
            std::vector<Point2D> flash_local = {
                Point2D( 0.0f,  0.0f),
                Point2D(18.0f, -10.0f),
                Point2D(12.0f,   0.0f),
                Point2D(18.0f,  10.0f)
            };

            // ALGO: 2D Scaling based on timer
            float scale_val = (muzzle_timer / 0.08f) * 1.3f;
            std::vector<Point2D> flash_scaled = CG::scale_polygon(flash_local, Point2D(0,0), scale_val, scale_val);

            // ALGO: 2D Translation and Rotation
            std::vector<Point2D> flash_trans = CG::translate_polygon(flash_scaled, tip.x, tip.y);
            std::vector<Point2D> flash_rot   = CG::rotate_polygon(flash_trans, tip, aim_angle);

            // ALGO: Scanline Fill for Muzzle Flash
            CG::fill_polygon_scanline(fb, flash_rot, Color::Yellow());
            CG::draw_polygon(fb, flash_rot, Color::Orange());
        }

        // Laser Aim Sight Line (drawn using DDA line!)
        Point2D tip = get_gun_tip();
        Point2D laser_end = CG::translate(tip, std::cos(aim_angle) * 700.0f, std::sin(aim_angle) * 700.0f);
        CG::draw_line_dda(fb, 
            static_cast<int>(tip.x), static_cast<int>(tip.y),
            static_cast<int>(laser_end.x), static_cast<int>(laser_end.y),
            Color(255, 30, 30, 80)); // Faint red laser guide
    }
};

// Main Game Controller
class Game {
public:
    enum State {
        MENU,
        PLAYING,
        GAME_OVER,
        VICTORY
    };

    State state;
    int score;
    int high_score;
    int kills;
    int lives;
    float defense_line_x;
    float spawn_timer;
    float spawn_interval;
    float defense_hit_flash; // Visual flash when zombie breaches line

    Player player;
    std::vector<Bullet> bullets;
    std::vector<Zombie> zombies;
    std::vector<Particle> particles;

    Game(int width = 960, int height = 600)
        : state(MENU), score(0), high_score(0), kills(0), lives(3),
          defense_line_x(170.0f), spawn_timer(0.0f), spawn_interval(2.0f),
          defense_hit_flash(0.0f),
          player(80.0f, height / 2.0f) {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
    }

    void restart() {
        score = 0;
        kills = 0;
        lives = 3;
        spawn_timer = 0.0f;
        spawn_interval = 2.0f;
        defense_hit_flash = 0.0f;
        bullets.clear();
        zombies.clear();
        particles.clear();
        player.pos = Point2D(80.0f, 300.0f);
        player.reset_aim();
        state = PLAYING;
    }

    void shoot_bullet() {
        if (state != PLAYING) return;

        Point2D gun_tip = player.get_gun_tip();
        bullets.emplace_back(gun_tip, player.aim_angle, 800.0f);
        player.muzzle_timer = 0.08f; // Trigger muzzle flash
    }

    void spawn_zombie(int screen_width, int screen_height) {
        float start_y = static_cast<float>(130 + rand() % (screen_height - 230));
        float speed = 55.0f + static_cast<float>(rand() % 40) + (kills * 1.5f); // Gradually gets faster
        zombies.emplace_back(static_cast<float>(screen_width + 30), start_y, speed);
    }

    void create_blood_splatter(Point2D pos, int count = 18) {
        for (int i = 0; i < count; ++i) {
            Particle p;
            p.pos = pos;
            float angle = static_cast<float>(rand() % 360) * (3.14159f / 180.0f);
            float spd = 60.0f + static_cast<float>(rand() % 160);
            p.vx = std::cos(angle) * spd;
            p.vy = std::sin(angle) * spd;
            p.life = 1.0f;
            p.decay = 1.5f + static_cast<float>(rand() % 100) / 100.0f;
            p.size = 2 + (rand() % 3);
            p.color = (rand() % 2 == 0) ? Color::BloodRed() : Color::ZombieSkin();
            particles.push_back(p);
        }
    }

    void update(float dt, int screen_width, int screen_height) {
        if (state != PLAYING) return;

        player.update(dt);

        if (defense_hit_flash > 0.0f) {
            defense_hit_flash -= dt * 3.0f;
        }

        // Spawn zombies periodically
        spawn_timer += dt;
        if (spawn_timer >= spawn_interval) {
            spawn_timer = 0.0f;
            spawn_zombie(screen_width, screen_height);
            // Slowly ramp up difficulty
            if (spawn_interval > 0.75f) {
                spawn_interval -= 0.02f;
            }
        }

        // Update Bullets
        for (auto& b : bullets) {
            b.update(dt, screen_width);
        }

        // Update Zombies
        for (auto& z : zombies) {
            z.update(dt);

            // Check if Zombie touches or crosses the Defense Line
            if (z.active && z.pos.x <= defense_line_x) {
                z.active = false;
                lives--;
                defense_hit_flash = 1.0f; // Flash defense line red

                create_blood_splatter(z.pos, 15);

                if (lives <= 0) {
                    lives = 0;
                    state = GAME_OVER;
                }
            }
        }

        // Collision Check: Bullets vs Zombies
        for (auto& b : bullets) {
            if (!b.active) continue;

            for (auto& z : zombies) {
                if (!z.active) continue;

                if (z.check_collision(b.pos, b.radius)) {
                    b.active = false;
                    z.active = false;
                    score += 100;
                    if (score > high_score) {
                        high_score = score;
                    }
                    kills++;
                    create_blood_splatter(z.pos, 22);
                    break;
                }
            }
        }

        // Cleanup inactive bullets & zombies
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
            [](const Bullet& b) { return !b.active; }), bullets.end());

        zombies.erase(std::remove_if(zombies.begin(), zombies.end(),
            [](const Zombie& z) { return !z.active; }), zombies.end());

        // Update Particles
        for (auto& p : particles) {
            p.update(dt);
        }
        particles.erase(std::remove_if(particles.begin(), particles.end(),
            [](const Particle& p) { return p.life <= 0.0f; }), particles.end());
    }

    // Helper: Draws heart icon for lives using Bresenham Circles and Scanline Fill Triangle
    void draw_heart(Framebuffer& fb, int x, int y, bool filled) const {
        if (filled) {
            // Heart top 2 lobes: filled circles
            CG::fill_circle_bresenham(fb, x - 5, y - 2, 5, Color::Red());
            CG::fill_circle_bresenham(fb, x + 5, y - 2, 5, Color::Red());

            // Bottom pointed lobe: scanline filled inverted triangle
            std::vector<Point2D> tri = {
                Point2D(static_cast<float>(x - 10), static_cast<float>(y - 1)),
                Point2D(static_cast<float>(x + 10), static_cast<float>(y - 1)),
                Point2D(static_cast<float>(x),      static_cast<float>(y + 11))
            };
            CG::fill_polygon_scanline(fb, tri, Color::Red());
        } else {
            // Depleted heart outline
            CG::draw_circle_bresenham(fb, x - 5, y - 2, 5, Color::DarkGray());
            CG::draw_circle_bresenham(fb, x + 5, y - 2, 5, Color::DarkGray());
            CG::draw_line_bresenham(fb, x - 10, y - 1, x, y + 11, Color::DarkGray());
            CG::draw_line_bresenham(fb, x + 10, y - 1, x, y + 11, Color::DarkGray());
        }
    }

    void draw(Framebuffer& fb) const {
        // Clear background with dark sci-fi / horror tone
        fb.clear(Color(18, 20, 26));

        // Draw ground / background grid pattern using DDA lines
        for (int y = 90; y < fb.height; y += 45) {
            CG::draw_line_dda(fb, 0, y, fb.width, y, Color(24, 28, 36));
        }

        // ===============================================================
        // 1. DEFENSE LINE (Crucial gameplay boundary)
        // ALGO: DDA Line Drawing Algorithm
        // ===============================================================
        Color line_col = (defense_hit_flash > 0.0f) ? Color::Red() : Color::NeonCyan();
        int def_x = static_cast<int>(defense_line_x);

        // Multi-line glow effect using DDA
        CG::draw_line_dda(fb, def_x - 1, 80, def_x - 1, fb.height - 20, line_col);
        CG::draw_line_dda(fb, def_x,     80, def_x,     fb.height - 20, Color::White());
        CG::draw_line_dda(fb, def_x + 1, 80, def_x + 1, fb.height - 20, line_col);

        // Hazard barricade posts along the line (Bresenham lines & circles)
        for (int y = 110; y < fb.height - 20; y += 60) {
            CG::fill_circle_bresenham(fb, def_x, y, 6, line_col);
            CG::draw_line_bresenham(fb, def_x - 12, y, def_x + 12, y, Color::GunMetal());
        }

        // ===============================================================
        // 2. PARTICLES (Blood splatter & explosion sparks)
        // ===============================================================
        for (const auto& p : particles) {
            // ALGO: Midpoint/Bresenham Circle for particles
            CG::fill_circle_bresenham(fb, 
                static_cast<int>(p.pos.x), static_cast<int>(p.pos.y), 
                p.size, p.color);
        }

        // ===============================================================
        // 3. BULLETS & TRACERS
        // ===============================================================
        for (const auto& b : bullets) {
            b.draw(fb);
        }

        // ===============================================================
        // 4. PLAYER STANDING BEHIND LINE
        // ===============================================================
        player.draw(fb);

        // ===============================================================
        // 5. ZOMBIES
        // ===============================================================
        for (const auto& z : zombies) {
            z.draw(fb);
        }

        // ===============================================================
        // 6. HEADS-UP DISPLAY (HUD) & UI
        // ===============================================================
        // Top banner bar
        std::vector<Point2D> top_bar = {
            Point2D(0.0f, 0.0f),
            Point2D(static_cast<float>(fb.width), 0.0f),
            Point2D(static_cast<float>(fb.width), 75.0f),
            Point2D(0.0f, 75.0f)
        };
        CG::fill_polygon_scanline(fb, top_bar, Color(12, 14, 18));
        CG::draw_line_dda(fb, 0, 75, fb.width, 75, Color::Cyan());

        // Display Score, High Score, Kills, and Lives
        FontRenderer::draw_text(fb, "SCORE:", 20, 24, Color::White(), 2);
        FontRenderer::draw_text(fb, std::to_string(score), 105, 24, Color::Yellow(), 2);

        FontRenderer::draw_text(fb, "HIGH SCORE:", 230, 24, Color::White(), 2);
        FontRenderer::draw_text(fb, std::to_string(high_score), 380, 24, Color::NeonCyan(), 2);

        FontRenderer::draw_text(fb, "KILLS:", 510, 24, Color::White(), 2);
        FontRenderer::draw_text(fb, std::to_string(kills), 595, 24, Color::Green(), 2);

        FontRenderer::draw_text(fb, "LIVES:", 710, 24, Color::White(), 2);
        for (int i = 0; i < 3; ++i) {
            draw_heart(fb, 800 + (i * 30), 32, (i < lives));
        }

        // Controls info bar at bottom
        FontRenderer::draw_text(fb, "CONTROLS: [ENTER / SPACE] SHOOT | [W/S] or [UP/DOWN] MOVE | [A/D] AIM GUN | [R] RESTART", 
            60, fb.height - 22, Color::LightGray(), 1);

        // ===============================================================
        // 7. MENUS & GAME OVER OVERLAYS
        // ===============================================================
        if (state == MENU) {
            // Semi-transparent overlay box
            std::vector<Point2D> menu_box = {
                Point2D(150.0f, 150.0f),
                Point2D(static_cast<float>(fb.width - 150), 150.0f),
                Point2D(static_cast<float>(fb.width - 150), 450.0f),
                Point2D(150.0f, 450.0f)
            };
            CG::fill_polygon_scanline(fb, menu_box, Color(15, 18, 25));
            CG::draw_polygon(fb, menu_box, Color::NeonCyan());

            FontRenderer::draw_text(fb, "ZOMBIE LINE DEFENSE", 240, 195, Color::Red(), 4);
            FontRenderer::draw_text(fb, "HOLD THE LINE AND SURVIVE THE HORDE", 230, 260, Color::Cyan(), 2);

            FontRenderer::draw_text(fb, ">> PRESS [ENTER] OR [SPACE] TO START <<", 215, 325, Color::Yellow(), 2);
            FontRenderer::draw_text(fb, "Stop zombies before they cross the defense line!", 245, 380, Color::LightGray(), 1);
            FontRenderer::draw_text(fb, "You have 3 lives. Good luck soldier!", 310, 405, Color::Orange(), 1);
        } else if (state == GAME_OVER) {
            std::vector<Point2D> over_box = {
                Point2D(170.0f, 160.0f),
                Point2D(static_cast<float>(fb.width - 170), 160.0f),
                Point2D(static_cast<float>(fb.width - 170), 450.0f),
                Point2D(170.0f, 450.0f)
            };
            CG::fill_polygon_scanline(fb, over_box, Color(25, 10, 10));
            CG::draw_polygon(fb, over_box, Color::Red());

            FontRenderer::draw_text(fb, "GAME OVER", 310, 195, Color::Red(), 5);
            FontRenderer::draw_text(fb, "THE DEFENSE LINE HAS FALLEN!", 270, 260, Color::White(), 2);
            FontRenderer::draw_text(fb, "FINAL SCORE: " + std::to_string(score) + "   HIGH SCORE: " + std::to_string(high_score), 
                260, 305, Color::Yellow(), 2);
            FontRenderer::draw_text(fb, "TOTAL ZOMBIES ELIMINATED: " + std::to_string(kills), 
                280, 340, Color::Green(), 2);

            FontRenderer::draw_text(fb, "PRESS [R] TO PLAY AGAIN", 315, 395, Color::NeonCyan(), 2);
        }
    }
};

#endif // GAME_HPP
