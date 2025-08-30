#include <iostream>
#include <raylib.h>

constexpr float BallSpeed = 420.0f;
constexpr float PaddleSpeed = 360.0f;

constexpr uint8_t BallRadius = 10;
constexpr uint8_t PaddleHeight = 120;
constexpr uint8_t PaddleWidth = 25;

constexpr int32_t ScreenWidth = 1280;
constexpr int32_t ScreenHeight = 800;

int32_t player_score = 0, cpu_score = 0;

auto Green         = Color {38, 185, 154, 255};
auto DarkGreen     = Color {20, 160, 133, 255};
auto LightGreen    = Color {129, 204, 184, 255};
auto Yellow        = Color {243, 213, 91, 255};

class Ball final {
public:
    explicit Ball() = default;
    explicit Ball(const Vector2 position, const uint8_t radius = BallRadius, const Color color = WHITE) :
        m_position(position), m_radius(radius), m_color(color) {}

    ~Ball() = default;

    void draw() const { DrawCircleV(m_position, m_radius, m_color); }

    void update() {
        m_position.x += m_speed.x * GetFrameTime();
        m_position.y += m_speed.y * GetFrameTime();

        if (m_position.x - static_cast<float>(m_radius) <= 0.0f) {
            player_score++;
            reset();
        }
        else if (m_position.x  + static_cast<float>(m_radius) >= ScreenWidth ) {
            cpu_score++;
            reset();
        }

        if (m_position.y - static_cast<float>(m_radius) <= 0.0f ||
            m_position.y + static_cast<float>(m_radius) >= ScreenHeight) {
            m_speed.y *= -1.0f;
        }

        draw();
    }

    void reset() {
        m_position = { static_cast<float>(ScreenWidth) / 2.0f, static_cast<float>(ScreenHeight) / 2.0f };
        const int8_t speed_choices[2] = { -1, 1 };
        m_speed.x *= speed_choices[GetRandomValue(0, 1)];
        m_speed.y *= speed_choices[GetRandomValue(0, 1)];
    }

    [[nodiscard]] auto position() const { return m_position; }
    [[nodiscard]] auto speed() const { return m_speed; }
    [[nodiscard]] auto radius() const { return m_radius; }
    [[nodiscard]] auto color() const { return m_color; }

    void position(const Vector2 position) { m_position = position; }
    void speed(const Vector2 speed) { m_speed = speed; }
    void radius(const uint8_t radius) { m_radius = radius; }
    void color(const Color color) { m_color = color; }

private:
    Vector2 m_position  {0.0f, 0.0f};
    Vector2 m_speed     {BallSpeed, BallSpeed};
    uint8_t m_radius    {BallRadius};
    Color m_color       {WHITE};
};

class Paddle {
public:
    explicit Paddle() = default;
    explicit Paddle(const Vector2 position, const uint8_t width, const uint8_t height, const Color color = WHITE) :
        m_position(position), m_width(width), m_height(height), m_color(color) {}
    virtual ~Paddle() = default;

    virtual void draw() const {
        DrawRectangleRounded(
            Rectangle { m_position.x - static_cast<float>(m_width) / 2.0f,
              m_position.y - static_cast<float>(m_height) / 2.0f,
              static_cast<float>(m_width),
              static_cast<float>(m_height)
            },
            0.8, 0,
            m_color);
    }

    virtual void update() {
        if (IsKeyDown(KEY_UP)) { m_position.y -= m_speed.y * GetFrameTime(); }
        if (IsKeyDown(KEY_DOWN)) { m_position.y += m_speed.y * GetFrameTime(); }

        bounds_check();
        draw();
    }

    virtual void handle_ball_collision(Ball &ball) {
        if (CheckCollisionCircleRec(ball.position(), ball.radius(),
            Rectangle(m_position.x - static_cast<float>(m_width) / 2.0f,
                      m_position.y - static_cast<float>(m_height) / 2.0f,
                      static_cast<float>(m_width),
                      static_cast<float>(m_height)))) {
           ball.speed({ ball.speed().x * -1, ball.speed().y });
        }
    }

    [[nodiscard]] auto position() const { return m_position; }
    [[nodiscard]] auto speed() const { return m_speed; }
    [[nodiscard]] auto width() const { return m_width; }
    [[nodiscard]] auto height() const { return m_height; }
    [[nodiscard]] auto color() const { return m_color; }

    void position(const Vector2 position) { m_position = position; }
    void speed(const Vector2 speed) { m_speed = speed; }
    void width(const uint8_t width) { m_width = width; }
    void height(const uint8_t height) { m_height = height; }
    void color(const Color color) { m_color = color; }

protected:
    Vector2 m_position  {0.0f, 0.0f};
    Vector2 m_speed     {PaddleSpeed, PaddleSpeed};
    uint8_t m_width     {PaddleWidth};
    uint8_t m_height    {PaddleHeight};
    Color m_color       {WHITE};

    virtual void bounds_check() {
        if (m_position.y - static_cast<float>(m_height) / 2.0f <= 0.0f) {
            m_position.y = static_cast<float>(m_height) / 2.0f;
        }
        else if (m_position.y + static_cast<float>(m_height) / 2.0f >= ScreenHeight) {
            m_position.y = ScreenHeight - static_cast<float>(m_height) / 2.0f;
        }
    }
};

class CpuPaddle final : public Paddle {
public:
    explicit CpuPaddle(const Vector2 position, const uint8_t width, const uint8_t height, const Color color = WHITE) {
        Paddle::position(position);
        Paddle::width(width);
        Paddle::height(height);
        Paddle::color(color);
        m_last_reaction_time = static_cast<float>(GetTime());
    }

    ~CpuPaddle() override = default;

    void update(Ball &ball) {
        const auto current_time = static_cast<float>(GetTime());

        // Check if ball is coming toward this paddle (assuming CPU is on the left)

        if (bool ball_coming_toward_paddle = ball.speed().x < 0) {
            // React with some delay (simulates human reaction time)
            if (current_time - m_last_reaction_time > m_reaction_delay) {
                // Predict where ball will be with some error
                const float time_to_paddle = abs(ball.position().x - m_position.x) / abs(ball.speed().x);
                const float predicted_y = ball.position().y + (ball.speed().y * time_to_paddle);

                // Add some prediction error to make it beatable
                float prediction_error = GetRandomValue(-m_prediction_error, m_prediction_error);
                m_target_y = predicted_y + prediction_error;

                m_last_reaction_time = current_time;
            }

            // Move toward target with full speed
            moveTowardTarget(m_speed.y);
        } else {
            // Ball going away - slowly return to center position
            m_target_y = static_cast<float>(ScreenHeight) / 2.0f;
            moveTowardTarget(m_speed.y * m_return_to_center_speed_multiplier);
        }

        bounds_check();
        draw();
    }

private:
    float m_target_y = static_cast<float>(ScreenHeight) / 2.0f;
    float m_last_reaction_time = 0.0f;

    // AI parameters - tuned to be challenging but beatable
    float m_reaction_delay = 0.05f;           // 150ms reaction time
    float m_prediction_error = 20.0f;         // Up to 25 pixels prediction error
    float m_dead_zone = 8.0f;                 // Don't move if within 8 pixels
    float m_return_to_center_speed_multiplier = 1; // Slower return to center

    void moveTowardTarget(float speed) {
        float difference = m_target_y - m_position.y;

        // Only move if outside dead zone
        if (abs(difference) > m_dead_zone) {
            if (difference > 0) {
                m_position.y += speed * GetFrameTime();
            } else {
                m_position.y -= speed * GetFrameTime();
            }
        }
    }

    void bounds_check() {
        float half_height = static_cast<float>(height()) / 2.0f;
        if (m_position.y - half_height < 0) {
            m_position.y = half_height;
        } else if (m_position.y + half_height > ScreenHeight) {
            m_position.y = ScreenHeight - half_height;
        }
    }
};

int32_t
main() {
    InitWindow(ScreenWidth, ScreenHeight, "PONG!");

    Ball ball({ static_cast<float>(ScreenWidth) / 2.0f, static_cast<float>(ScreenHeight) / 2.0f },
        BallRadius, Yellow);
    Paddle player_paddle({ ScreenWidth - PaddleWidth / 2 - 10, static_cast<float>(ScreenHeight) / 2.0f },
        PaddleWidth, PaddleHeight);
    CpuPaddle cpu_paddle({ PaddleWidth / 2 + 10, static_cast<float>(ScreenHeight) / 2.0f },
        PaddleWidth, PaddleHeight);

    ball.reset();
    while (!WindowShouldClose()) {
        BeginDrawing();

        // Draw court
        ClearBackground(DarkGreen);
        DrawRectangleV({ ScreenWidth / 2.0f, 0 }, { ScreenWidth / 2.0f, ScreenHeight }, Green);
        DrawCircleV({ ScreenWidth / 2.0f, ScreenHeight / 2.0f }, 150, LightGreen);
        DrawLineV({ ScreenWidth / 2.0f, 0.0f }, { ScreenWidth / 2.0f, ScreenHeight }, WHITE);

        // Check collisions
        player_paddle.handle_ball_collision(ball);
        cpu_paddle.handle_ball_collision(ball);

        // Update ball and paddles
        ball.update();
        player_paddle.update();
        cpu_paddle.update(ball);

        // Score
        DrawText(TextFormat("%i", cpu_score), ScreenWidth / 4, 20, 80, RAYWHITE);
        DrawText(TextFormat("%i", player_score), 3 * ScreenWidth / 4, 20, 80, RAYWHITE);

        DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 20, GREEN);

        EndDrawing();
    }

    CloseWindow();
}