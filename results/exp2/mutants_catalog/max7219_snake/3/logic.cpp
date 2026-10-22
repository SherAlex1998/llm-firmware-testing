#include "logic.h"

static byte led_buffer[RENDER_PANELS][2];
static byte frame[RENDER_PANELS][FIELD_HEIGHT];   // what each panel row shows (the matrices cannot be read back)

static int game_loop_filter = 0;

static Game game;
static UserInput user_input;

static void render_led_send_command(byte opcode, byte data);
static void render_led_send_buffer();

static void start(void) {
    game_init(&game);
    game_prepare_level(&game);

    user_input.next_move = DIRECTION_RIGHT;
    game_loop_filter = 0;

    snake_render_init();
    snake_render_field();
}

void snake_init() {
    randomSeed(analogRead(A0));
    start();
}

void snake_init_seeded(unsigned long seed) {
    randomSeed(seed);
    start();
}

void snake_loop_pass() {
    snake_input_read();

    game_loop_filter++;
    if (game_loop_filter >= GAME_LOOP_DELAY / LOOP_DELAY) {
        game_loop_filter = 0;
        snake_step();
    }
}

STEP_RESULT snake_step() {
    STEP_RESULT result = game_handle_next_step(&game, &user_input);
    if (result == STEP_RESULT_SUCCESS) {
        snake_render_field();
    } else {
        game_prepare_level(&game);
        user_input.next_move = DIRECTION_RIGHT;
        snake_render_field();
    }
    return result;
}

void snake_input_read() {
    snake_input_apply(analogRead(BTN_PIN_DY), analogRead(BTN_PIN_DX));
}

void snake_input_apply(int raw_dy, int raw_dx) {
    int dy = raw_dy - BTN_SIGNAL_MID_LEVEL;
    int dx = raw_dx - BTN_SIGNAL_MID_LEVEL;

    int abs_dy = dy < 0 ? -dy : dy;
    int abs_dx = dx < 0 ? -dx : dx;

    if (abs_dy > abs_dx) {
        if (abs_dy > BTN_SIGNAL_THRESHOLD) {
            user_input.next_move = dy < 0 ? DIRECTION_UP : DIRECTION_DOWN;
        }
    } else {
        if (abs_dx > BTN_SIGNAL_THRESHOLD) {
            user_input.next_move = dx < 0 ? DIRECTION_LEFT : DIRECTION_RIGHT;
        }
    }
}

void snake_render_init() {
    pinMode(LED_PIN_DATA, OUTPUT);
    pinMode(LED_PIN_CLOCK, OUTPUT);
    pinMode(LED_PIN_CS, OUTPUT);
    digitalWrite(LED_PIN_CS, HIGH);

    render_led_send_command(LED_OP_DISPLAYTEST, 0);
    render_led_send_command(LED_OP_SCANLIMIT, 7);
    render_led_send_command(LED_OP_DECODEMODE, 1);
    render_led_send_command(LED_OP_SHUTDOWN, 1);
    render_led_send_command(LED_OP_INTENSITY, 8);
}

void snake_render_field() {
    for (int y = 0; y < FIELD_HEIGHT; y++) {
        for (int x = 0; x < FIELD_WIDTH;) {
            byte data = 0, mask = 1 << (FIELD_HEIGHT - 1), d = RENDER_PANELS - x / FIELD_HEIGHT - 1;
            for (; mask > 0; x++) {
                if (!GAME_CELL_IS_TYPE(game.field.cells[x][y], CELL_TYPE_EMPTY))
                    data |= mask;
                mask >>= 1;
            }

            led_buffer[d][0] = FIELD_HEIGHT - y;
            led_buffer[d][1] = data;
            frame[d][y] = data;
        }
        render_led_send_buffer();
    }
}

static void render_led_send_command(byte opcode, byte data) {
    for (int i = 0; i < RENDER_PANELS; i++) {
        led_buffer[i][0] = opcode;
        led_buffer[i][1] = data;
    }
    render_led_send_buffer();
}

static void render_led_send_buffer() {
    digitalWrite(LED_PIN_CS, LOW);

    for (int i = RENDER_PANELS - 1; i >= 0; i--) {
        shiftOut(LED_PIN_DATA, LED_PIN_CLOCK, MSBFIRST, led_buffer[i][0]);
        shiftOut(LED_PIN_DATA, LED_PIN_CLOCK, MSBFIRST, led_buffer[i][1]);
    }

    digitalWrite(LED_PIN_CS, HIGH);
}

DIRECTION snake_next_move() {
    return user_input.next_move;
}

Game* snake_game() {
    return &game;
}

int snake_loop_filter() {
    return game_loop_filter;
}

uint8_t snake_led_row(uint8_t panel, uint8_t row) {
    return frame[panel % RENDER_PANELS][row % FIELD_HEIGHT];
}
