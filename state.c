#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

#include "protocols/content-type.h"
#include "protocols/xdg-shell.h"
#include "raycast.c"
#include "shm.c"

#define WIDTH 1920
#define HEIGHT 1080

typedef struct {
    struct wl_display* display;
    struct wl_registry* registry;
    struct wl_compositor* compositor;
    struct xdg_wm_base* xdg_wm_base;
    struct wp_content_type_manager_v1* content_type_manager;

    struct {
        struct wl_seat* seat;
        struct wl_pointer* pointer;
        struct wl_keyboard* keyboard;
        // xkb is needed to handle multiple layouts yada yada.
        struct xkb_state* xkb_state;
        struct xkb_context* xkb_context;
        struct xkb_keymap* xkb_keymap;
    } input;

    struct {
        struct wl_shm* interface;
        struct wl_shm_pool* pool;
    } shm;

    struct {
        struct wl_surface* wl_surface;
        struct xdg_surface* xdg_surface;
        struct xdg_toplevel* xdg_toplevel;
        struct wl_buffer* buffer;
        uint32_t last_time;
        uint32_t* data;  // mapped data (XRGB8888) 0xFFRRGGBB
        size_t width, height, stride;
    } surface;

    Raycaster raycaster;
} State;

static void wl_buffer_release(void* data, struct wl_buffer* wl_buffer) {
    // Sent by the compositor when it's no longer using this buffer
    wl_buffer_destroy(wl_buffer);
}

static const struct wl_buffer_listener wl_buffer_listener = {
    .release = wl_buffer_release,
};

static void xdg_surface_configure(void* data, struct xdg_surface* xdg_surface, uint32_t serial) {
    fprintf(stderr, "INFO: Configure serial=%d\n", serial);
    State* self = data;
    // FIXME: Get width/height

    // NOTE: Since we only create a single surface we don't have to check which
    // xdg_surface it is
    xdg_surface_ack_configure(xdg_surface, serial);

    // We can only attach a buffer *after* the initial XDG configure has been acked.
    // otherwise we will trigger a protocol error
    wl_surface_attach(self->surface.wl_surface, self->surface.buffer, 0, 0);
    wl_surface_damage_buffer(
        self->surface.wl_surface,
        0,
        0,
        self->surface.width,
        self->surface.height
    );
    wl_surface_commit(self->surface.wl_surface);
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

static void xdg_wm_base_ping(void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial) {
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

static const struct wl_callback_listener frame_callback_listener;

void on_frame(void* data, struct wl_callback* wl_callback, uint32_t time) {
    wl_callback_destroy(wl_callback);

    State* self = data;
    wl_callback = wl_surface_frame(self->surface.wl_surface);
    wl_callback_add_listener(wl_callback, &frame_callback_listener, self);

    const size_t width = self->surface.width, height = self->surface.height;
    for (size_t x = 0; x < width; x++) {
        for (size_t y = 0; y < height; y++) {
            self->surface.data[x + (y * width)] = 0xFF181818;
        }
    }

    double dt = 0;
    if (self->surface.last_time != 0) {
        dt = (double)(time - self->surface.last_time) / 1000.;
    }

    raycaster_update(&self->raycaster, dt);
    raycaster_draw(&self->raycaster, width, height, self->surface.data);

    wl_surface_attach(self->surface.wl_surface, self->surface.buffer, 0, 0);
    wl_surface_damage_buffer(self->surface.wl_surface, 0, 0, width, height);
    wl_surface_commit(self->surface.wl_surface);

    self->surface.last_time = time;
}

static const struct wl_callback_listener frame_callback_listener = {
    .done = on_frame,
};

static void wl_keyboard_keymap(
    void* data,
    struct wl_keyboard* wl_keyboard,
    uint32_t format,
    int32_t fd,
    uint32_t size
) {
    fprintf(stderr, "INFO: wl_keyboard_keymap: fd=%d, size=%d\n", fd, size);

    State* state = data;
    assert(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);

    char* map_shm = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    assert(map_shm != MAP_FAILED);

    struct xkb_keymap* xkb_keymap = xkb_keymap_new_from_string(
        state->input.xkb_context,
        map_shm,
        XKB_KEYMAP_FORMAT_TEXT_V1,
        XKB_KEYMAP_COMPILE_NO_FLAGS
    );
    munmap(map_shm, size);
    close(fd);

    struct xkb_state* xkb_state = xkb_state_new(xkb_keymap);
    xkb_keymap_unref(state->input.xkb_keymap);
    xkb_state_unref(state->input.xkb_state);
    state->input.xkb_keymap = xkb_keymap;
    state->input.xkb_state = xkb_state;
}

static void wl_keyboard_enter(
    void* data,
    struct wl_keyboard* wl_keyboard,
    uint32_t serial,
    struct wl_surface* surface,
    struct wl_array* keys
) {
    fprintf(stderr, "INFO: wl_keyboard_enter: serial=%d\n", serial);
}

void wl_keyboard_key(
    void* data,
    struct wl_keyboard* wl_keyboard,
    uint32_t serial,
    uint32_t time,
    uint32_t key,
    uint32_t key_state
) {
    State* state = data;

    char buf[128];
    uint32_t keycode = key + 8;
    xkb_keysym_t sym = xkb_state_key_get_one_sym(state->input.xkb_state, keycode);
    xkb_keysym_get_name(sym, buf, sizeof(buf));
    xkb_state_key_get_utf8(state->input.xkb_state, keycode, buf, sizeof(buf));

    bool pressed = key_state == WL_KEYBOARD_KEY_STATE_PRESSED;
    raycaster_input(&state->raycaster, sym, pressed);
}

static void wl_keyboard_leave(
    void* data,
    struct wl_keyboard* wl_keyboard,
    uint32_t serial,
    struct wl_surface* surface
) {
    fprintf(stderr, "keyboard leave\n");
}

static void wl_keyboard_modifiers(
    void* data,
    struct wl_keyboard* wl_keyboard,
    uint32_t serial,
    uint32_t mods_depressed,
    uint32_t mods_latched,
    uint32_t mods_locked,
    uint32_t group
) {
    State* state = data;
    xkb_state_update_mask(
        state->input.xkb_state,
        mods_depressed,
        mods_latched,
        mods_locked,
        0,
        0,
        group
    );
}

static void wl_keyboard_repeat_info(
    void* data,
    struct wl_keyboard* wl_keyboard,
    int32_t rate,
    int32_t delay
) {
    /* Left as an exercise for the reader */
}

static struct wl_keyboard_listener wl_keyboard_listener = {
    .keymap = wl_keyboard_keymap,
    .enter = wl_keyboard_enter,
    .leave = wl_keyboard_leave,
    .key = wl_keyboard_key,
    .modifiers = wl_keyboard_modifiers,
    .repeat_info = wl_keyboard_repeat_info,
};

void on_registry_global(
    void* data,
    struct wl_registry* wl_registry,
    uint32_t name,
    const char* interface,
    uint32_t version
) {
    State* self = data;
    bool bound = false;

    if ((bound = strcmp(interface, wl_compositor_interface.name) == 0)) {
        self->compositor = wl_registry_bind(wl_registry, name, &wl_compositor_interface, version);
    } else if ((bound = strcmp(interface, wl_shm_interface.name) == 0)) {
        self->shm.interface = wl_registry_bind(wl_registry, name, &wl_shm_interface, version);
    } else if ((bound = strcmp(interface, xdg_wm_base_interface.name) == 0)) {
        self->xdg_wm_base = wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, version);
        xdg_wm_base_add_listener(self->xdg_wm_base, &xdg_wm_base_listener, data);
    } else if ((bound = strcmp(interface, wl_seat_interface.name) == 0)) {
        self->input.seat = wl_registry_bind(wl_registry, name, &wl_seat_interface, version);
        self->input.keyboard = wl_seat_get_keyboard(self->input.seat);
        wl_keyboard_add_listener(self->input.keyboard, &wl_keyboard_listener, self);
        self->input.pointer = wl_seat_get_pointer(self->input.seat);
    } else if ((bound = strcmp(interface, wp_content_type_manager_v1_interface.name) == 0)) {
        self->content_type_manager
            = wl_registry_bind(wl_registry, name, &wp_content_type_manager_v1_interface, version);
    }

    if (bound) fprintf(stderr, "INFO: Bound '%s' v%d\n", interface, version);
}

static struct wl_registry_listener registry_listener = {
    .global = on_registry_global,
};

static void init_surface(State* self) {
    self->surface.wl_surface = wl_compositor_create_surface(self->compositor);

    // xdg_wm is used to make the toplevel (IE the window that gets managed by the compositor)
    self->surface.xdg_surface
        = xdg_wm_base_get_xdg_surface(self->xdg_wm_base, self->surface.wl_surface);
    xdg_surface_add_listener(self->surface.xdg_surface, &xdg_surface_listener, self);
    self->surface.xdg_toplevel = xdg_surface_get_toplevel(self->surface.xdg_surface);
    xdg_toplevel_set_title(self->surface.xdg_toplevel, "3D Renderer");
    xdg_toplevel_set_app_id(self->surface.xdg_toplevel, "fht.test.3d");

    // content type since it lets fht-compositor auto-float the window (anything that has a
    // content-type will be automatically floated, I still didn't add a window rule yet)
    struct wp_content_type_v1* content_type = wp_content_type_manager_v1_get_surface_content_type(
        self->content_type_manager,
        self->surface.wl_surface
    );
    wp_content_type_v1_set_content_type(content_type, WP_CONTENT_TYPE_V1_TYPE_GAME);

    // commit everything
    wl_surface_commit(self->surface.wl_surface);

    struct wl_callback* frame_callback = wl_surface_frame(self->surface.wl_surface);
    wl_callback_add_listener(frame_callback, &frame_callback_listener, self);
}

void state_init(State* self) {
    // initialize global stuff. other globals will be initialized using the first
    // wl_display_roundtrip when the compositor will start advertising and binding globals.
    self->display = wl_display_connect(NULL);
    self->registry = wl_display_get_registry(self->display);
    self->input.xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

    wl_registry_add_listener(self->registry, &registry_listener, self);
    // pump events in order to bind globals
    wl_display_roundtrip(self->display);

    init_surface(self);

    self->surface.width = WIDTH;
    self->surface.height = HEIGHT;
    self->surface.stride = self->surface.width * 4;
    const int shm_pool_size = self->surface.height * self->surface.stride * 2;

    int fd = allocate_shm_file(shm_pool_size);
    self->surface.data = mmap(NULL, shm_pool_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    self->shm.pool = wl_shm_create_pool(self->shm.interface, fd, shm_pool_size);
    self->surface.buffer = wl_shm_pool_create_buffer(
        self->shm.pool,
        0,
        self->surface.width,
        self->surface.height,
        self->surface.stride,
        WL_SHM_FORMAT_ARGB8888
    );

    raycaster_init(&self->raycaster);
}

int state_run(State* self) {
    while (wl_display_dispatch(self->display) != -1) {
        // FIXME: Add raycaster update
    };
    wl_display_disconnect(self->display);
    return 0;
}
