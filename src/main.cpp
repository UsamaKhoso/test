#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "audio/AudioEngine.h"
#include "math/Mat4.h"
#include "math/Vec3.h"

namespace {

constexpr float kPi = 3.1415926535f;
constexpr float kGravity = -18.0f;
constexpr float kArenaSize = 48.0f;
constexpr float kTileSize = 6.0f;

struct Player {
    Vec3 position{0.0f, 1.2f, 10.0f};
    Vec3 velocity{0.0f, 0.0f, 0.0f};
    float yaw{0.0f};
    float cameraYaw{0.0f};
    float health{140.0f};
    float stamina{120.0f};
    float mana{80.0f};
    float attackCooldown{0.0f};
    float spellCooldown{0.0f};
    float dashCooldown{0.0f};
    bool onGround{true};
};

struct Projectile {
    Vec3 position;
    Vec3 velocity;
    float ttl{4.0f};
    bool fromPlayer{false};
    float damage{12.0f};
};

struct Enemy {
    enum class BehaviorState { Dormant, Patrol, Chase, Attack, Recover, Enraged };

    Vec3 position{0.0f, 0.6f, 0.0f};
    Vec3 velocity{0.0f, 0.0f, 0.0f};
    Vec3 facing{0.0f, 0.0f, 1.0f};
    std::vector<Vec3> patrolPoints;
    std::size_t patrolIndex{0};
    BehaviorState state{BehaviorState::Patrol};
    float stateTimer{0.0f};
    float health{90.0f};
    float maxHealth{90.0f};
    float aggressionRadius{14.0f};
    float attackRange{2.4f};
    float rangedRange{16.0f};
    float attackCooldown{0.0f};
    float moveSpeed{5.5f};
    float projectileSpeed{12.0f};
    bool isBoss{false};
    float enragedThreshold{0.35f};
};

struct ArenaTile {
    Vec3 center;
    bool isWall{false};
};

struct GameWorld {
    std::vector<ArenaTile> tiles;
    std::vector<Enemy> enemies;
    std::vector<Projectile> projectiles;
    float dayNightTimer{0.0f};
};

struct TitleMenu {
    std::vector<std::string> options{"BEGIN QUEST", "ARENA TRAINING", "QUIT"};
    int selection{0};
    float pulse{0.0f};
    float inputCooldown{0.0f};
    float confirmCooldown{0.0f};
};

enum class GameState { Title, Playing, Victory, Defeat };

void errorCallback(int code, const char* description) {
    std::cerr << "GLFW error (" << code << "): " << description << "\n";
}

float clampf(float value, float minValue, float maxValue) {
    return std::max(minValue, std::min(value, maxValue));
}

float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

Vec3 lerpVec(const Vec3& a, const Vec3& b, float t) {
    return a + (b - a) * t;
}

void drawCube(float size) {
    float half = size * 0.5f;
    glBegin(GL_QUADS);

    glColor3f(0.7f, 0.4f, 0.2f);
    glVertex3f(-half, -half, half);
    glVertex3f(half, -half, half);
    glVertex3f(half, half, half);
    glVertex3f(-half, half, half);

    glColor3f(0.3f, 0.5f, 0.9f);
    glVertex3f(-half, -half, -half);
    glVertex3f(-half, half, -half);
    glVertex3f(half, half, -half);
    glVertex3f(half, -half, -half);

    glColor3f(0.4f, 0.7f, 0.4f);
    glVertex3f(-half, -half, -half);
    glVertex3f(-half, -half, half);
    glVertex3f(-half, half, half);
    glVertex3f(-half, half, -half);

    glColor3f(0.9f, 0.6f, 0.3f);
    glVertex3f(half, -half, -half);
    glVertex3f(half, half, -half);
    glVertex3f(half, half, half);
    glVertex3f(half, -half, half);

    glColor3f(0.7f, 0.7f, 0.9f);
    glVertex3f(-half, half, -half);
    glVertex3f(-half, half, half);
    glVertex3f(half, half, half);
    glVertex3f(half, half, -half);

    glColor3f(0.2f, 0.2f, 0.2f);
    glVertex3f(-half, -half, -half);
    glVertex3f(half, -half, -half);
    glVertex3f(half, -half, half);
    glVertex3f(-half, -half, half);

    glEnd();
}

void drawPrism(float radius, float height) {
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(0.8f, 0.2f, 0.2f);
    glVertex3f(0.0f, height * 0.5f, 0.0f);
    for (int i = 0; i <= 12; ++i) {
        float angle = (static_cast<float>(i) / 12.0f) * kPi * 2.0f;
        glVertex3f(std::cos(angle) * radius, -height * 0.5f, std::sin(angle) * radius);
    }
    glEnd();

    glBegin(GL_TRIANGLE_FAN);
    glColor3f(0.1f, 0.1f, 0.1f);
    glVertex3f(0.0f, -height * 0.5f, 0.0f);
    for (int i = 0; i <= 12; ++i) {
        float angle = (static_cast<float>(i) / 12.0f) * kPi * 2.0f;
        glVertex3f(std::cos(angle) * radius, -height * 0.5f, std::sin(angle) * radius);
    }
    glEnd();
}

void drawGroundTile(const ArenaTile& tile) {
    float half = kTileSize * 0.5f;
    glBegin(GL_QUADS);
    if (tile.isWall) {
        glColor3f(0.25f, 0.25f, 0.3f);
    } else {
        glColor3f(0.15f, 0.35f, 0.15f);
    }
    glVertex3f(tile.center.x - half, 0.0f, tile.center.z - half);
    glVertex3f(tile.center.x + half, 0.0f, tile.center.z - half);
    glVertex3f(tile.center.x + half, 0.0f, tile.center.z + half);
    glVertex3f(tile.center.x - half, 0.0f, tile.center.z + half);
    glEnd();

    if (tile.isWall) {
        glPushMatrix();
        glTranslatef(tile.center.x, 1.5f, tile.center.z);
        glScalef(1.0f, 3.0f, 1.0f);
        drawCube(kTileSize * 0.6f);
        glPopMatrix();
    }
}

void drawBillboard(float size, float r, float g, float b) {
    float half = size * 0.5f;
    glBegin(GL_QUADS);
    glColor3f(r, g, b);
    glVertex3f(-half, 0.0f, 0.0f);
    glVertex3f(half, 0.0f, 0.0f);
    glVertex3f(half, size, 0.0f);
    glVertex3f(-half, size, 0.0f);
    glEnd();
}

struct Glyph {
    std::array<std::string, 7> rows;
};

const std::unordered_map<char, Glyph>& glyphMap() {
    static const std::unordered_map<char, Glyph> glyphs = {
        {'A', {{" ### ", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"}}},
        {'B', {{"#### ", "#   #", "#   #", "#### ", "#   #", "#   #", "#### "}}},
        {'C', {{" ### ", "#   #", "#    ", "#    ", "#    ", "#   #", " ### "}}},
        {'D', {{"#### ", "#   #", "#   #", "#   #", "#   #", "#   #", "#### "}}},
        {'E', {{"#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#####"}}},
        {'F', {{"#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#    "}}},
        {'G', {{" ### ", "#   #", "#    ", "#  ##", "#   #", "#   #", " ### "}}},
        {'H', {{"#   #", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"}}},
        {'I', {{" ### ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", " ### "}}},
        {'J', {{"  ###", "   #", "   #", "   #", "   #", "#  #", " ## "}}},
        {'K', {{"#   #", "#  # ", "# #  ", "##   ", "# #  ", "#  # ", "#   #"}}},
        {'L', {{"#    ", "#    ", "#    ", "#    ", "#    ", "#    ", "#####"}}},
        {'M', {{"#   #", "## ##", "# # #", "#   #", "#   #", "#   #", "#   #"}}},
        {'N', {{"#   #", "##  #", "# # #", "#  ##", "#   #", "#   #", "#   #"}}},
        {'O', {{" ### ", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "}}},
        {'P', {{"#### ", "#   #", "#   #", "#### ", "#    ", "#    ", "#    "}}},
        {'Q', {{" ### ", "#   #", "#   #", "#   #", "# # #", "#  # ", " ## #"}}},
        {'R', {{"#### ", "#   #", "#   #", "#### ", "# #  ", "#  # ", "#   #"}}},
        {'S', {{" ####", "#    ", "#    ", " ### ", "    #", "    #", "#### "}}},
        {'T', {{"#####", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  "}}},
        {'U', {{"#   #", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "}}},
        {'V', {{"#   #", "#   #", "#   #", "#   #", " # # ", " # # ", "  #  "}}},
        {'W', {{"#   #", "#   #", "#   #", "# # #", "# # #", "## ##", "#   #"}}},
        {'X', {{"#   #", "#   #", " # # ", "  #  ", " # # ", "#   #", "#   #"}}},
        {'Y', {{"#   #", "#   #", " # # ", "  #  ", "  #  ", "  #  ", "  #  "}}},
        {'Z', {{"#####", "    #", "   # ", "  #  ", " #   ", "#    ", "#####"}}},
        {'0', {{" ### ", "#   #", "#  ##", "# # #", "##  #", "#   #", " ### "}}},
        {'1', {{"  #  ", " ##  ", "  #  ", "  #  ", "  #  ", "  #  ", " ### "}}},
        {'2', {{" ### ", "#   #", "    #", "   # ", "  #  ", " #   ", "#####"}}},
        {'3', {{" ### ", "#   #", "    #", " ### ", "    #", "#   #", " ### "}}},
        {'4', {{"   # ", "  ## ", " # # ", "#  # ", "#####", "   # ", "   # "}}},
        {'5', {{"#####", "#    ", "#    ", "#### ", "    #", "#   #", " ### "}}},
        {'6', {{" ### ", "#   #", "#    ", "#### ", "#   #", "#   #", " ### "}}},
        {'7', {{"#####", "    #", "   # ", "  #  ", " #   ", " #   ", " #   "}}},
        {'8', {{" ### ", "#   #", "#   #", " ### ", "#   #", "#   #", " ### "}}},
        {'9', {{" ### ", "#   #", "#   #", " ####", "    #", "#   #", " ### "}}},
        {' ', {{"     ", "     ", "     ", "     ", "     ", "     ", "     "}}},
        {'-', {{"     ", "     ", "     ", " ### ", "     ", "     ", "     "}}},
        {':', {{"     ", "  #  ", "     ", "     ", "  #  ", "     ", "     "}}},
        {'!', {{"  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "     ", "  #  "}}},
        {'.', {{"     ", "     ", "     ", "     ", "     ", " ##  ", " ##  "}}},
        {'/', {{"    #", "    #", "   # ", "  #  ", " #   ", "#    ", "#    "}}},
    };
    return glyphs;
}

void begin2D(int width, int height) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, static_cast<double>(width), static_cast<double>(height), 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

void end2D() {
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawString2D(float x, float y, float scale, const std::string& text) {
    float cursorX = x;
    float cursorY = y;

    const auto& glyphs = glyphMap();
    for (char ch : text) {
        char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        auto it = glyphs.find(upper);
        if (it == glyphs.end()) {
            cursorX += scale * 6.0f;
            continue;
        }
        const Glyph& glyph = it->second;
        glBegin(GL_QUADS);
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (glyph.rows[row][col] != ' ') {
                    float gx = cursorX + col * scale;
                    float gy = cursorY + row * scale;
                    glVertex2f(gx, gy);
                    glVertex2f(gx + scale, gy);
                    glVertex2f(gx + scale, gy + scale);
                    glVertex2f(gx, gy + scale);
                }
            }
        }
        glEnd();
        cursorX += scale * 6.0f;
    }
}

void updateWindowTitle(GLFWwindow* window, const Player& player, const GameWorld& world, float elapsedSeconds) {
    std::ostringstream oss;
    oss << "Arcane Vanguard | HP " << std::fixed << std::setprecision(0) << player.health
        << " | STA " << player.stamina << " | MP " << player.mana
        << " | Enemies " << world.enemies.size() << " | Time " << std::setprecision(1)
        << elapsedSeconds << "s";
    glfwSetWindowTitle(window, oss.str().c_str());
}

void buildArena(GameWorld& world) {
    world.tiles.clear();
    int count = static_cast<int>(kArenaSize / kTileSize);
    float start = -kArenaSize * 0.5f + kTileSize * 0.5f;
    for (int x = 0; x < count; ++x) {
        for (int z = 0; z < count; ++z) {
            ArenaTile tile;
            tile.center = Vec3{start + x * kTileSize, 0.0f, start + z * kTileSize};
            tile.isWall = (x == 0 || z == 0 || x == count - 1 || z == count - 1);
            if (!tile.isWall && ((x + z) % 5 == 0)) {
                tile.isWall = true;
            }
            world.tiles.push_back(tile);
        }
    }
}

Vec3 findClosestGroundPoint(const Vec3& position) {
    float half = kArenaSize * 0.5f - 1.0f;
    Vec3 clamped = position;
    clamped.x = clampf(clamped.x, -half, half);
    clamped.z = clampf(clamped.z, -half, half);
    if (clamped.y < 0.6f) clamped.y = 0.6f;
    return clamped;
}

void spawnEnemy(GameWorld& world, const Vec3& position, bool boss = false) {
    Enemy enemy;
    enemy.position = position;
    enemy.patrolPoints = {
        position,
        position + Vec3{2.5f, 0.0f, 1.5f},
        position + Vec3{-2.5f, 0.0f, -1.5f}};
    enemy.patrolIndex = 0;
    enemy.health = enemy.maxHealth = boss ? 420.0f : 110.0f;
    enemy.aggressionRadius = boss ? 24.0f : 12.0f;
    enemy.attackRange = boss ? 4.0f : 2.5f;
    enemy.rangedRange = boss ? 30.0f : 16.0f;
    enemy.moveSpeed = boss ? 4.2f : 5.6f;
    enemy.projectileSpeed = boss ? 18.0f : 11.0f;
    enemy.isBoss = boss;
    enemy.enragedThreshold = boss ? 0.45f : 0.2f;
    world.enemies.push_back(enemy);
}

void populateEnemies(GameWorld& world) {
    world.enemies.clear();
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-kArenaSize * 0.35f, kArenaSize * 0.35f);
    for (int i = 0; i < 14; ++i) {
        Vec3 pos{dist(rng), 0.6f, dist(rng)};
        spawnEnemy(world, pos, false);
    }
    spawnEnemy(world, Vec3{-8.0f, 0.6f, -12.0f}, true);
    spawnEnemy(world, Vec3{10.0f, 0.6f, 14.0f}, true);
}

Vec3 horizontalDirection(const Vec3& from, const Vec3& to) {
    Vec3 dir = to - from;
    dir.y = 0.0f;
    if (length(dir) < 0.001f) return Vec3{0.0f, 0.0f, 0.0f};
    return normalize(dir);
}

void applyDamage(Enemy& enemy, float amount) {
    enemy.health -= amount;
    if (enemy.health < 0.0f) enemy.health = 0.0f;
    enemy.state = Enemy::BehaviorState::Recover;
    enemy.stateTimer = 0.5f;
}

void updatePlayer(Player& player, GameWorld& world, GLFWwindow* window, float dt) {
    Vec3 forward{std::sin(player.yaw), 0.0f, -std::cos(player.yaw)};
    Vec3 right{forward.z, 0.0f, -forward.x};

    Vec3 input{0.0f, 0.0f, 0.0f};
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) input += forward;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) input -= forward;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) input -= right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) input += right;

    bool isDashing = false;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && player.dashCooldown <= 0.0f && length(input) > 0.0f && player.stamina >= 12.0f) {
        player.dashCooldown = 1.25f;
        player.stamina -= 12.0f;
        isDashing = true;
    }

    float targetSpeed = isDashing ? 16.0f : 7.0f;
    if (length(input) > 0.0f) {
        input = normalize(input);
        Vec3 desired = input * targetSpeed;
        player.velocity.x = lerpf(player.velocity.x, desired.x, 0.18f);
        player.velocity.z = lerpf(player.velocity.z, desired.z, 0.18f);
        player.stamina = std::min(player.stamina + 15.0f * dt, 120.0f);
    } else {
        player.velocity.x = lerpf(player.velocity.x, 0.0f, 0.12f);
        player.velocity.z = lerpf(player.velocity.z, 0.0f, 0.12f);
        player.stamina = std::min(player.stamina + 22.0f * dt, 120.0f);
    }

    if (player.onGround && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && player.stamina >= 8.0f) {
        player.velocity.y = 9.5f;
        player.onGround = false;
        player.stamina -= 8.0f;
    }

    player.velocity.y += kGravity * dt;
    player.position += player.velocity * dt;

    if (player.position.y <= 0.6f) {
        player.position.y = 0.6f;
        player.velocity.y = 0.0f;
        player.onGround = true;
    }

    player.position = findClosestGroundPoint(player.position);

    if (player.dashCooldown > 0.0f) player.dashCooldown -= dt;
    if (player.attackCooldown > 0.0f) player.attackCooldown -= dt;
    if (player.spellCooldown > 0.0f) player.spellCooldown -= dt;

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        player.yaw -= dt * 1.5f;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        player.yaw += dt * 1.5f;
    }

    player.cameraYaw = lerpf(player.cameraYaw, player.yaw, 0.08f);

    bool meleePressed = glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS;
    bool spellPressed = glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS;

    if (meleePressed && player.attackCooldown <= 0.0f) {
        player.attackCooldown = 0.55f;
        player.stamina = std::max(0.0f, player.stamina - 5.0f);
        for (auto& enemy : world.enemies) {
            float dist = length(enemy.position - player.position);
            if (dist < 2.6f) {
                applyDamage(enemy, enemy.isBoss ? 24.0f : 38.0f);
                enemy.velocity += horizontalDirection(enemy.position, player.position) * -6.0f;
            }
        }
    }

    if (spellPressed && player.spellCooldown <= 0.0f && player.mana >= 12.0f) {
        player.spellCooldown = 0.9f;
        player.mana -= 12.0f;
        Vec3 direction = normalize(Vec3{std::sin(player.yaw), 0.1f, -std::cos(player.yaw)});
        Projectile p;
        p.position = player.position + direction * 1.5f + Vec3{0.0f, 0.5f, 0.0f};
        p.velocity = direction * 22.0f;
        p.damage = 32.0f;
        p.fromPlayer = true;
        world.projectiles.push_back(p);
    } else {
        player.mana = std::min(player.mana + 6.0f * dt, 80.0f);
    }
}

void updateEnemies(GameWorld& world, Player& player, float dt) {
    std::vector<Projectile> newProjectiles;

    for (auto& enemy : world.enemies) {
        enemy.stateTimer -= dt;
        if (enemy.attackCooldown > 0.0f) enemy.attackCooldown -= dt;

        Vec3 toPlayer = player.position - enemy.position;
        float distance = length(toPlayer);
        Vec3 direction = horizontalDirection(enemy.position, player.position);
        enemy.facing = direction;

        switch (enemy.state) {
        case Enemy::BehaviorState::Dormant:
            if (distance < enemy.aggressionRadius * 0.6f) {
                enemy.state = Enemy::BehaviorState::Chase;
            }
            break;
        case Enemy::BehaviorState::Patrol: {
            if (distance < enemy.aggressionRadius) {
                enemy.state = Enemy::BehaviorState::Chase;
                break;
            }
            if (!enemy.patrolPoints.empty()) {
                Vec3 target = enemy.patrolPoints[enemy.patrolIndex];
                Vec3 moveDir = horizontalDirection(enemy.position, target);
                enemy.velocity = moveDir * enemy.moveSpeed * 0.6f;
                enemy.position += enemy.velocity * dt;
                if (length(target - enemy.position) < 1.0f) {
                    enemy.patrolIndex = (enemy.patrolIndex + 1) % enemy.patrolPoints.size();
                }
            }
            break;
        }
        case Enemy::BehaviorState::Chase: {
            Vec3 chaseVel = direction * enemy.moveSpeed;
            enemy.velocity = lerpVec(enemy.velocity, chaseVel, 0.12f);
            enemy.position += enemy.velocity * dt;
            if (distance < enemy.attackRange + 1.0f) {
                enemy.state = Enemy::BehaviorState::Attack;
            } else if (distance > enemy.aggressionRadius * 1.4f) {
                enemy.state = Enemy::BehaviorState::Patrol;
            }
            break;
        }
        case Enemy::BehaviorState::Attack: {
            if (distance > enemy.rangedRange) {
                enemy.state = Enemy::BehaviorState::Chase;
                break;
            }
            if (enemy.attackCooldown <= 0.0f) {
                if (distance < enemy.attackRange) {
                    enemy.attackCooldown = enemy.isBoss ? 1.4f : 1.0f;
                    player.health -= enemy.isBoss ? 22.0f : 12.0f;
                    player.velocity += direction * -7.0f;
                } else {
                    enemy.attackCooldown = enemy.isBoss ? 1.6f : 1.3f;
                    Projectile p;
                    p.fromPlayer = false;
                    p.damage = enemy.isBoss ? 16.0f : 10.0f;
                    Vec3 launch = normalize(toPlayer + Vec3{0.0f, 0.6f, 0.0f});
                    p.velocity = launch * enemy.projectileSpeed;
                    p.position = enemy.position + Vec3{0.0f, 0.8f, 0.0f};
                    newProjectiles.push_back(p);
                }
            }
            if (enemy.isBoss && enemy.health / enemy.maxHealth < enemy.enragedThreshold) {
                enemy.state = Enemy::BehaviorState::Enraged;
                enemy.stateTimer = 2.0f;
            }
            break;
        }
        case Enemy::BehaviorState::Recover: {
            enemy.velocity *= 0.6f;
            enemy.position += enemy.velocity * dt;
            if (enemy.stateTimer <= 0.0f) {
                enemy.state = Enemy::BehaviorState::Chase;
            }
            break;
        }
        case Enemy::BehaviorState::Enraged: {
            float surgeSpeed = enemy.moveSpeed * 1.4f;
            enemy.velocity = direction * surgeSpeed;
            enemy.position += enemy.velocity * dt;
            enemy.attackCooldown -= dt;
            if (enemy.attackCooldown <= 0.0f && distance < enemy.rangedRange) {
                enemy.attackCooldown = 0.8f;
                Projectile burst;
                burst.fromPlayer = false;
                burst.damage = 18.0f;
                burst.position = enemy.position + Vec3{0.0f, 1.0f, 0.0f};
                for (int i = -2; i <= 2; ++i) {
                    Projectile shard = burst;
                    float angleOffset = i * 0.22f;
                    float baseYaw = std::atan2(direction.x, -direction.z);
                    float yaw = baseYaw + angleOffset;
                    Vec3 dir{std::sin(yaw), 0.05f, -std::cos(yaw)};
                    shard.velocity = normalize(dir) * (enemy.projectileSpeed + 6.0f);
                    newProjectiles.push_back(shard);
                }
            }
            if (enemy.health / enemy.maxHealth > enemy.enragedThreshold + 0.1f) {
                enemy.state = Enemy::BehaviorState::Chase;
            }
            break;
        }
        }

        enemy.position = findClosestGroundPoint(enemy.position);
    }

    for (auto& projectile : newProjectiles) {
        world.projectiles.push_back(projectile);
    }
}

void updateProjectiles(GameWorld& world, Player& player, float dt) {
    std::vector<Projectile> alive;
    alive.reserve(world.projectiles.size());
    for (auto& projectile : world.projectiles) {
        projectile.ttl -= dt;
        projectile.position += projectile.velocity * dt;
        projectile.position = findClosestGroundPoint(projectile.position);
        if (projectile.ttl <= 0.0f) continue;

        if (projectile.fromPlayer) {
            for (auto& enemy : world.enemies) {
                if (enemy.health <= 0.0f) continue;
                float dist = length(enemy.position - projectile.position);
                if (dist < 1.4f) {
                    applyDamage(enemy, projectile.damage);
                    projectile.ttl = 0.0f;
                    break;
                }
            }
        } else {
            float dist = length(player.position - projectile.position);
            if (dist < 1.6f) {
                player.health -= projectile.damage;
                player.velocity += horizontalDirection(projectile.position, player.position) * 4.0f;
                projectile.ttl = 0.0f;
            }
        }

        if (projectile.ttl > 0.0f) {
            alive.push_back(projectile);
        }
    }
    world.projectiles.swap(alive);
}

void removeDefeatedEnemies(GameWorld& world) {
    std::vector<Enemy> alive;
    alive.reserve(world.enemies.size());
    for (auto& enemy : world.enemies) {
        if (enemy.health > 0.0f) {
            alive.push_back(enemy);
        }
    }
    world.enemies.swap(alive);
}

bool allBossesDefeated(const GameWorld& world) {
    for (const auto& enemy : world.enemies) {
        if (enemy.isBoss) return false;
    }
    return true;
}

void renderProjectiles(const GameWorld& world) {
    for (const auto& projectile : world.projectiles) {
        glPushMatrix();
        glTranslatef(projectile.position.x, projectile.position.y, projectile.position.z);
        glScalef(0.4f, 0.4f, 0.4f);
        glColor3f(projectile.fromPlayer ? 0.6f : 0.9f, projectile.fromPlayer ? 0.8f : 0.3f, 0.2f);
        drawCube(1.0f);
        glPopMatrix();
    }
}

void renderEnemies(const GameWorld& world) {
    for (const auto& enemy : world.enemies) {
        glPushMatrix();
        glTranslatef(enemy.position.x, enemy.position.y, enemy.position.z);
        float yaw = std::atan2(enemy.facing.x, -enemy.facing.z) * 180.0f / kPi;
        glRotatef(yaw, 0.0f, 1.0f, 0.0f);
        if (enemy.isBoss) {
            glScalef(1.6f, 2.8f, 1.6f);
            glColor3f(0.4f, 0.05f, 0.3f);
            drawCube(1.4f);
            glTranslatef(0.0f, 0.8f, 0.0f);
            glScalef(0.7f, 0.7f, 0.7f);
            glColor3f(0.9f, 0.3f, 0.5f);
            drawPrism(0.9f, 1.4f);
        } else {
            glScalef(1.0f, 1.8f, 1.0f);
            glColor3f(0.2f, 0.4f, 0.7f);
            drawCube(1.1f);
            glTranslatef(0.0f, 0.7f, 0.0f);
            glScalef(0.8f, 0.8f, 0.8f);
            glColor3f(0.9f, 0.9f, 0.3f);
            drawPrism(0.6f, 1.0f);
        }
        glPopMatrix();
    }
}

void renderPlayerAvatar(const Player& player) {
    glPushMatrix();
    glTranslatef(player.position.x, player.position.y, player.position.z);
    glRotatef(player.yaw * 180.0f / kPi, 0.0f, 1.0f, 0.0f);
    glScalef(1.1f, 1.9f, 1.1f);
    glColor3f(0.9f, 0.8f, 0.6f);
    drawCube(1.0f);
    glTranslatef(0.0f, 0.8f, 0.0f);
    glScalef(0.7f, 0.7f, 0.7f);
    glColor3f(0.2f, 0.2f, 0.4f);
    drawPrism(0.6f, 1.1f);
    glPopMatrix();
}

void renderGround(const GameWorld& world) {
    for (const auto& tile : world.tiles) {
        drawGroundTile(tile);
    }
}

void renderMenuOverlay(int width, int height, const TitleMenu& menu) {
    glDisable(GL_DEPTH_TEST);
    begin2D(width, height);
    glColor3f(0.1f, 0.05f, 0.12f);
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(static_cast<float>(width), 0.0f);
    glVertex2f(static_cast<float>(width), static_cast<float>(height));
    glVertex2f(0.0f, static_cast<float>(height));
    glEnd();

    glColor3f(0.8f, 0.7f, 0.6f);
    drawString2D(width * 0.25f, height * 0.2f, 6.0f, "ARCANE VANGUARD");

    for (std::size_t i = 0; i < menu.options.size(); ++i) {
        float y = height * 0.45f + static_cast<float>(i) * 70.0f;
        if (static_cast<int>(i) == menu.selection) {
            glColor3f(0.9f, 0.6f + 0.3f * std::sin(menu.pulse), 0.3f);
        } else {
            glColor3f(0.6f, 0.6f, 0.6f);
        }
        drawString2D(width * 0.3f, y, 5.0f, menu.options[i]);
    }

    end2D();
    glEnable(GL_DEPTH_TEST);
}

void renderHud(int width, int height, const Player& player) {
    glDisable(GL_DEPTH_TEST);
    begin2D(width, height);
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(40.0f, height - 160.0f);
    glVertex2f(340.0f, height - 160.0f);
    glVertex2f(340.0f, height - 40.0f);
    glVertex2f(40.0f, height - 40.0f);
    glEnd();

    glColor3f(0.9f, 0.2f, 0.2f);
    drawString2D(60.0f, height - 130.0f, 4.5f, "HEALTH");
    drawString2D(60.0f, height - 110.0f, 4.5f, std::to_string(static_cast<int>(player.health)));

    glColor3f(0.2f, 0.6f, 0.9f);
    drawString2D(60.0f, height - 80.0f, 4.5f, "MANA " + std::to_string(static_cast<int>(player.mana)));

    glColor3f(0.9f, 0.8f, 0.2f);
    drawString2D(60.0f, height - 50.0f, 4.5f, "STAMINA " + std::to_string(static_cast<int>(player.stamina)));

    end2D();
    glEnable(GL_DEPTH_TEST);
}

void resetGame(Player& player, GameWorld& world) {
    player = Player{};
    world.projectiles.clear();
    world.dayNightTimer = 0.0f;
    buildArena(world);
    populateEnemies(world);
}

} // namespace

int main() {
    glfwSetErrorCallback(errorCallback);

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1600, 900, "Arcane Vanguard", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glEnable(GL_DEPTH_TEST);

    AudioEngine audioEngine;
    bool audioReady = audioEngine.initialize();
    if (audioReady) {
        audioEngine.start();
    }

    Player player;
    GameWorld world;
    buildArena(world);
    populateEnemies(world);

    GameState state = GameState::Title;
    TitleMenu menu;

#ifndef SIMPLE3D_HEADLESS
    auto previousTime = std::chrono::steady_clock::now();
#else
    int headlessFrameCounter = 0;
#endif
    float elapsedSeconds = 0.0f;

    while (!glfwWindowShouldClose(window)) {
#ifndef SIMPLE3D_HEADLESS
        auto currentTime = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(currentTime - previousTime).count();
        previousTime = currentTime;
        dt = std::clamp(dt, 0.0f, 0.033f);
#else
        float dt = 1.0f / 60.0f;
        ++headlessFrameCounter;
#endif
        elapsedSeconds += dt;

        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            if (state == GameState::Playing) {
                state = GameState::Title;
                resetGame(player, world);
            } else {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
        }

        switch (state) {
        case GameState::Title: {
            menu.pulse += dt * 2.5f;
            if (menu.inputCooldown > 0.0f) menu.inputCooldown -= dt;
            if (menu.confirmCooldown > 0.0f) menu.confirmCooldown -= dt;

            if (menu.inputCooldown <= 0.0f && glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
                menu.selection = (menu.selection + 1) % static_cast<int>(menu.options.size());
                menu.inputCooldown = 0.2f;
            }
            if (menu.inputCooldown <= 0.0f && glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
                menu.selection = (menu.selection - 1 + static_cast<int>(menu.options.size())) % static_cast<int>(menu.options.size());
                menu.inputCooldown = 0.2f;
            }
            if (menu.confirmCooldown <= 0.0f && glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
                if (menu.selection == 0) {
                    resetGame(player, world);
                    elapsedSeconds = 0.0f;
                    state = GameState::Playing;
                } else if (menu.selection == 1) {
                    resetGame(player, world);
                    world.enemies.resize(6);
                    elapsedSeconds = 0.0f;
                    state = GameState::Playing;
                } else if (menu.selection == 2) {
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }
                menu.confirmCooldown = 0.3f;
            }
            break;
        }
        case GameState::Playing: {
            updatePlayer(player, world, window, dt);
            updateEnemies(world, player, dt);
            updateProjectiles(world, player, dt);
            removeDefeatedEnemies(world);

            if (player.health <= 0.0f) {
                state = GameState::Defeat;
            } else if (world.enemies.empty() || allBossesDefeated(world)) {
                state = GameState::Victory;
            }
            break;
        }
        case GameState::Victory:
        case GameState::Defeat: {
            if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
                state = GameState::Title;
                resetGame(player, world);
            }
            break;
        }
        }

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspect = static_cast<float>(width) / static_cast<float>(height == 0 ? 1 : height);
        Mat4 projection = perspective(60.0f * kPi / 180.0f, aspect, 0.1f, 180.0f);

        Vec3 cameraOffset{std::sin(player.cameraYaw) * -10.0f, 16.0f, std::cos(player.cameraYaw) * -10.0f};
        Vec3 cameraPosition = player.position + cameraOffset;
        Vec3 lookTarget = player.position + Vec3{0.0f, 1.2f, 0.0f};
        Mat4 view = lookAt(cameraPosition, lookTarget, Vec3{0.0f, 1.0f, 0.0f});

        glViewport(0, 0, width, height);
        float dayNight = 0.5f + 0.5f * std::sin(elapsedSeconds * 0.1f);
        glClearColor(0.1f + 0.2f * dayNight, 0.15f + 0.1f * dayNight, 0.25f + 0.2f * dayNight, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(projection.data());
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(view.data());

        renderGround(world);
        renderPlayerAvatar(player);
        renderEnemies(world);
        renderProjectiles(world);

        if (state == GameState::Title) {
            renderMenuOverlay(width, height, menu);
        } else {
            renderHud(width, height, player);
            if (state == GameState::Victory) {
                glDisable(GL_DEPTH_TEST);
                begin2D(width, height);
                glColor3f(0.8f, 0.8f, 0.2f);
                drawString2D(width * 0.35f, height * 0.2f, 6.0f, "VICTORY! PRESS ENTER");
                end2D();
                glEnable(GL_DEPTH_TEST);
            } else if (state == GameState::Defeat) {
                glDisable(GL_DEPTH_TEST);
                begin2D(width, height);
                glColor3f(0.9f, 0.2f, 0.2f);
                drawString2D(width * 0.3f, height * 0.2f, 6.0f, "DEFEATED... PRESS ENTER");
                end2D();
                glEnable(GL_DEPTH_TEST);
            }
        }

        if (state == GameState::Playing) {
            updateWindowTitle(window, player, world, elapsedSeconds);
        }

        glfwSwapBuffers(window);
    }

    audioEngine.stop();
    audioEngine.shutdown();

    glfwTerminate();
#ifdef SIMPLE3D_HEADLESS
    std::cout << "Headless simulation finished after " << headlessFrameCounter << " frames." << std::endl;
#endif
    return 0;
}

