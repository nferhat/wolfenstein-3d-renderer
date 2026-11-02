#include "state.c"

int main(int argc, char** argv) {
    State state = { 0 };
    state_init(&state);
    return state_run(&state);
}
