#include <graphx.h>
#include <keypadc.h>
#include <cmath>
#include <time.h>
#include <sys/util.h>

unsigned long long millis = 0;
float deltaTime = 0;

template <typename T> int sign (T val) {
    return (T(0) < val) - (val < T(0));
}

inline void FillRect (float x, float y, float w, float h) {
    gfx_FillRectangle(x - w / 2, y - h / 2, w, h);
}

struct Vector {
    float x;
    float y;
    float magnitude;
    float angle;

    Vector () {};
    Vector (float p, float q) : x(p), y(q) { SetMagnitude(); SetAngle(); };

    Vector Set (float p, float q) { x = p; y = q; SetMagnitude(); return *this; };
    Vector SetMagnitude () { magnitude = powf(powf(x, 2) + powf(y, 2), 0.5); return *this; };
    Vector SetMagnitude (float _magnitude) { float scale = _magnitude / magnitude; x *= scale; y *= scale; magnitude = _magnitude; return *this; };
    Vector SetAngle () { angle = atan(y / x); return *this; };
    Vector SetAngle (float _angle) { x = magnitude * cos(_angle); y = magnitude * sin(_angle); angle = _angle; return *this; };

    Vector operator+ (Vector v) { Vector result; result.x = x + v.x; result.y = y + v.y; return result; };
    Vector operator- (Vector v) { Vector result; result.x = x - v.x; result.y = y - v.y; return result; };
    Vector operator* (float n) { Vector result; result.x = x * n; result.y = y * n; return result; };
    Vector operator* (Vector v) { Vector result; result.x = x * v.x; result.y = y * v.y; return result; };
    Vector operator+= (Vector v) { x += v.x; y += v.y; return *this; };
    Vector operator-= (Vector v) { x -= v.x; y -= v.y; return *this; };
    Vector operator*= (Vector v) { x *= v.x; y *= v.y; return *this; };
    Vector operator*= (float v) { x *= v; y *= v; return *this; };
    bool operator== (Vector v) { return x == v.x && y == v.y; };
};

struct Puck {
    Vector position;
    Vector velocity;
    static const int playSpeed = 450;
    static const int startSpeed = 150;
    static const uint8_t radius = 5;
    static const uint8_t color = 255;

    void Show ();
    void Update ();
    void Score ();
    void Bounce ();
    void Collide ();
    bool BetweenPaddle (Vector);
    void Reset (int);
    Puck ();
};

Puck puck;

struct Paddle {
    Vector position;
    kb_lkey_t up, down;
    static const int width = 10;
    static const int height = 50;
    static const int border = width / 2 + 10;
    static const uint8_t color = 255;
    int speed = 300;
    static const int autoSpeed = 200;
    int score;
    bool isAuto;

    void Show ();
    void Update ();
    Paddle (float, kb_lkey_t, kb_lkey_t);
};

Paddle::Paddle (float x, kb_lkey_t up, kb_lkey_t down) {
    position.Set(x, GFX_LCD_HEIGHT / 2);
    this->up = up;
    this->down = down;
    score = 0;
}

void Paddle::Show () {
    gfx_SetColor(color);

    FillRect(position.x, position.y, width, height);
}

void Paddle::Update () {
    kb_Scan();
    if (!isAuto) position.y += speed * deltaTime * (kb_IsDown(up) ? -1 : kb_IsDown(down) ? 1 : 0);
    else { position.y += speed * deltaTime * (sign(position.x - puck.position.x) == sign(puck.velocity.x) ? puck.position.y - position.y < -speed * deltaTime ? -1 : puck.position.y - position.y > speed * deltaTime ? 1 : 0 : 0); speed = autoSpeed; };
    position.y = position.y < height / 2 ? height / 2 : position.y > GFX_LCD_HEIGHT - height / 2 ? GFX_LCD_HEIGHT - height / 2 : position.y;
}

Paddle left { Paddle::border, kb_Key2nd, kb_KeyAlpha };
Paddle right { GFX_LCD_WIDTH - Paddle::border, kb_KeyUp, kb_KeyDown };

Puck::Puck () {
    Reset(0);
}

void Puck::Show () {
    gfx_SetColor(color);

    gfx_FillCircle(position.x, position.y, radius);
}

void Puck::Update () {
    position += velocity * deltaTime;

    Collide();
    Bounce();
    Score();
}

inline void Puck::Score () {
    bool leftScores = position.x > GFX_LCD_WIDTH + radius;
    bool rightScores = position.x < -radius;
    if (leftScores || rightScores) Reset(leftScores ? -1 : rightScores ? 1 : 0);
}

inline void Puck::Bounce () {
    if (position.y < radius || position.y > GFX_LCD_HEIGHT - radius) {
        velocity.y *= -1;
        position.y += velocity.y * 2 * deltaTime;
    }
}

inline void Puck::Collide () {
    bool onLeft = position.x - radius < left.position.x + Paddle::width / 2;
    bool onRight = position.x + radius > right.position.x - Paddle::width / 2;

    if (onLeft && BetweenPaddle(left.position) && !kb_IsDown(kb_KeyRight)) {
        velocity.SetAngle((position.y - (left.position.y - Paddle::height / 2)) / Paddle::height * M_PI / 2 - M_PI / 4);
        velocity.SetMagnitude(playSpeed);
    }

    if (onRight && BetweenPaddle(right.position) && !kb_IsDown(kb_KeyMode)) {
        velocity.SetAngle((position.y - (right.position.y - Paddle::height / 2)) / Paddle::height * M_PI / 2 - M_PI / 4);
        velocity.x *= -1;
        velocity.SetMagnitude(playSpeed);
    }
}

inline bool Puck::BetweenPaddle (Vector paddlePosition) {
    return position.y + radius + 10 > paddlePosition.y - Paddle::height / 2 && position.y - radius - 10 < paddlePosition.y + Paddle::height / 2;
}

void Puck::Reset (int state) {
    if (state > 0) right.score++;
    if (state < 0) left.score++;

    position.Set(GFX_LCD_WIDTH / 2, GFX_LCD_HEIGHT / 2);
    uint32_t rn = random() - 0x80000000;
    velocity.Set((float(random() % 300) / 100 + 1) * rn * sign(random() - 0x80000000), rn);
    velocity.SetMagnitude(startSpeed);
}

int main() {
    gfx_Begin();

    srandom((unsigned)time(NULL));

    gfx_ZeroScreen();
    gfx_SetTextTransparentColor(1);
    gfx_SetTextBGColor(0);

    gfx_SetTextScale(2, 2);
    gfx_SetTextFGColor(255);
    gfx_PrintStringXY("Pong84", GFX_LCD_WIDTH / 2 - gfx_GetStringWidth("Pong84") / 2, GFX_LCD_HEIGHT / 2 - 60);

    gfx_SetTextScale(1, 1);
    gfx_SetTextFGColor(224);
    gfx_PrintStringXY("Left for Single Player", GFX_LCD_WIDTH / 4, GFX_LCD_HEIGHT / 2 - 24);
    gfx_SetTextFGColor(31);
    gfx_PrintStringXY("Right for Multiplayer", GFX_LCD_WIDTH / 4, GFX_LCD_HEIGHT / 2 - 12);
    gfx_SetTextFGColor(15);
    gfx_PrintStringXY("2nd, Up Arrow = Move Up", GFX_LCD_WIDTH / 4, GFX_LCD_HEIGHT / 2);
    gfx_PrintStringXY("Alpha, Down Arrow = Move Down", GFX_LCD_WIDTH / 4, GFX_LCD_HEIGHT / 2 + 12);
    gfx_PrintStringXY("Clear = Quit", GFX_LCD_WIDTH / 4, GFX_LCD_HEIGHT / 2 + 24);
    gfx_SetTextFGColor(255);
    gfx_PrintStringXY("Created by Warren James", GFX_LCD_WIDTH / 2 - gfx_GetStringWidth("Created by Warren James") / 2, GFX_LCD_HEIGHT / 2 + 48);
    gfx_PrintStringXY("Published 2025-06-01", GFX_LCD_WIDTH / 2 - gfx_GetStringWidth("Published 2025-06-01") / 2, GFX_LCD_HEIGHT / 2 + 60);
    gfx_PrintStringXY("Version 1.0.0", GFX_LCD_WIDTH / 2 - gfx_GetStringWidth("Version 1.0.0") / 2, GFX_LCD_HEIGHT / 2 + 72);

    gfx_SetTextScale(2, 2);

    while (!kb_IsDown(kb_KeyLeft) && !kb_IsDown(kb_KeyRight) && !kb_IsDown(kb_KeyClear)) kb_Scan();

    if (kb_IsDown(kb_KeyLeft)) left.isAuto = true;

    clock_t t = clock();

    while (!kb_IsDown(kb_KeyClear)) {
        unsigned long long newMillis = (unsigned long long) (1000 * float(clock() - t) / CLOCKS_PER_SEC);
        deltaTime = float(newMillis - millis) / 1000;
        millis = newMillis;

        gfx_ZeroScreen();
        gfx_SetDrawBuffer();

        left.Update();
        left.Show();
        right.Update();
        right.Show();

        if (kb_IsDown(kb_KeyGraph) && !kb_IsDown(kb_KeyYequ)) puck.velocity.Set(-200, 1000);
        else if (!kb_IsDown(kb_KeyGraph) && kb_IsDown(kb_KeyYequ)) puck.velocity.Set(200, 1000);

        puck.Update();
        puck.Show();

        for (float y = 5.0 / GFX_LCD_HEIGHT; y < 1 - 2.5 / GFX_LCD_HEIGHT; y += 0.06) {
            gfx_SetColor(255);
            FillRect(GFX_LCD_WIDTH / 2, y * GFX_LCD_HEIGHT, 5, 5);
        }

        gfx_SetTextFGColor(255);
        gfx_SetTextXY(GFX_LCD_WIDTH / 2 - 37 - 16 * int(log10(left.score + 1)), 10);
        gfx_PrintInt(left.score, 1);

        gfx_SetTextFGColor(255);
        gfx_SetTextXY(GFX_LCD_WIDTH / 2 + 21, 10);
        gfx_PrintInt(right.score, 1);

        gfx_BlitBuffer();

        kb_Scan();
    }

    gfx_End();
}