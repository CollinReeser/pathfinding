#include <chrono>
#include <iostream>
#include <ranges>
#include <sstream>

#include "Draw.h"
#include "Map.h"
#include "Util.h"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

#include <SDL2pp/SDL2pp.hh>

#include <SDL_gpu.h>

#include <NFont.h>

#include <entt/entt.hpp>

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

// const uint32_t SCREEN_WIDTH {640};
// const uint32_t SCREEN_HEIGHT {480};
const uint32_t SCREEN_WIDTH {640 * 3};
const uint32_t SCREEN_HEIGHT {480 * 2};

void pathfind_gfx(
    entt::registry &registry,
    GPU_Target* screen,
    SDL2pp::Window &window,
    SDL2pp::Mixer &mixer,
    ImGuiIO &io,
    SDL2pp::Font &sdl_font,
    NFont &font,
    ImFont* imgui_font
) {
    const uint32_t sprite_width {4};
    const uint32_t sprite_height {4};

    const uint32_t map_width {SCREEN_WIDTH / sprite_width};
    const uint32_t map_height {SCREEN_HEIGHT / sprite_height};

    Map map {Map::gen_rand_map(map_width, map_height)};

    for (uint32_t i = 0; i < 100; ++i) {
        const auto [x, y] = map.get_rand_open_xy();

        const auto entity = registry.create();
        registry.emplace<Pos>(entity, (float)x + 0.5f, (float)y + 0.5f);
        registry.emplace<Pather>(entity);
    }

    Frame frame(map, registry);

    frame.draw_frame();

    const SDL_PixelFormat* const pixel_format = SDL_AllocFormat(
        SDL_PIXELFORMAT_ARGB8888
    );

    unsigned char pix_obstacle[
        sprite_width * sprite_height * pixel_format->BytesPerPixel
    ];
    unsigned char pix_open[
        sprite_width * sprite_height * pixel_format->BytesPerPixel
    ];
    unsigned char pix_path[
        sprite_width * sprite_height * pixel_format->BytesPerPixel
    ];

    for (uint32_t y = 0; y < sprite_height; y++) {
        for (uint32_t x = 0; x < sprite_width; x++) {
            reinterpret_cast<uint32_t*>(pix_obstacle)[
                get_node_index(x, y, sprite_width)
            ] = SDL_MapRGBA(
                pixel_format,
                255 /*= red*/,
                0 /*= green*/,
                0 /*= blue*/,
                255 /*= alpha */
            );
        }
    }
    for (uint32_t y = 0; y < sprite_height; y++) {
        for (uint32_t x = 0; x < sprite_width; x++) {
            reinterpret_cast<uint32_t*>(pix_open)[
                get_node_index(x, y, sprite_width)
            ] = SDL_MapRGBA(
                pixel_format,
                200 /*= red*/,
                200 /*= green*/,
                200 /*= blue*/,
                255 /*= alpha */
            );
        }
    }
    for (uint32_t y = 0; y < sprite_height; y++) {
        for (uint32_t x = 0; x < sprite_width; x++) {
            reinterpret_cast<uint32_t*>(pix_path)[
                get_node_index(x, y, sprite_width)
            ] = SDL_MapRGBA(
                pixel_format,
                0 /*= red*/,
                255 /*= green*/,
                0 /*= blue*/,
                255 /*= alpha */
            );
        }
    }

    SDL2pp::Surface sprite_obstacle(
        pix_obstacle,
        sprite_width,
        sprite_height,
        pixel_format->BitsPerPixel,
        pixel_format->BytesPerPixel * sprite_width,
        0,
        0,
        0,
        0
    );

    SDL2pp::Surface sprite_open(
        pix_open,
        sprite_width,
        sprite_height,
        pixel_format->BitsPerPixel,
        pixel_format->BytesPerPixel * sprite_width,
        0,
        0,
        0,
        0
    );

    SDL2pp::Surface sprite_path(
        pix_path,
        sprite_width,
        sprite_height,
        pixel_format->BitsPerPixel,
        pixel_format->BytesPerPixel * sprite_width,
        0,
        0,
        0,
        0
    );

    GPU_Image* texture_obstacle = GPU_CopyImageFromSurface(sprite_obstacle.Get());
    GPU_Image* texture_open = GPU_CopyImageFromSurface(sprite_open.Get());
    GPU_Image* texture_path = GPU_CopyImageFromSurface(sprite_path.Get());

    auto free_guard = Guard(
        [=]() {
            GPU_FreeImage(texture_obstacle);
            GPU_FreeImage(texture_open);
            GPU_FreeImage(texture_path);
        }
    );

    std::vector<std::pair<uint32_t, uint32_t>> open_spaces;

    for (const auto &node : map.get_nodes()) {
        if (!node.blocking) {
            open_spaces.emplace_back(node.x_coord, node.y_coord);
        }
    }

    uint32_t x_click {0};
    uint32_t y_click {0};
    uint8_t clicks {0};

    uint32_t x_mouse {0};
    uint32_t y_mouse {0};

    uint8_t mouse_state {SDL_RELEASED};

    uint64_t frames {0};

    auto start_frame {std::chrono::steady_clock::now()};
    auto end_frame {std::chrono::steady_clock::now()};

    std::chrono::microseconds frame_dur {1};

    auto start_flip = std::chrono::steady_clock::now();
    auto end_flip = std::chrono::steady_clock::now();

    std::chrono::microseconds dur_flip {1};

    auto start_clear = std::chrono::steady_clock::now();
    auto end_clear = std::chrono::steady_clock::now();

    std::chrono::microseconds dur_clear {1};

    auto start_font = std::chrono::steady_clock::now();
    auto end_font = std::chrono::steady_clock::now();

    std::chrono::microseconds dur_font {1};

    auto start_pathfinding = std::chrono::steady_clock::now();
    auto end_pathfinding = std::chrono::steady_clock::now();

    std::chrono::microseconds dur_pathfinding {1};

    const uint32_t num_pathfinds {100};

    // Does not cull the clipped area from calculations, but does make it
    // black.

    // GPU_SetClip(
    //     screen,
    //     2, 2, SCREEN_WIDTH - 32, SCREEN_HEIGHT - 32
    // );

    GPU_EnableCamera(screen, GPU_TRUE);

    {
        GPU_Camera cam = GPU_GetCamera(screen);
        std::cout
            << "Cam values:" << std::endl
            << "  x: " << cam.x << std::endl
            << "  y: " << cam.y << std::endl
            << "  z: " << cam.z << std::endl
            << "  angle: " << cam.angle << std::endl
            << "  zoom_x: " << cam.zoom_x << std::endl
            << "  zoom_y: " << cam.zoom_y << std::endl
            << "  z_near: " << cam.z_near << std::endl
            << "  z_far: " << cam.z_far << std::endl
            ;
    }

    // Will be used to cache the map texture since it changes infrequently.
    GPU_Image* map_image = nullptr;
    auto free_map_image = Guard(
        [=]() {
            if (map_image != nullptr) {
                GPU_FreeImage(map_image);
            }
        }
    );

    bool done = false;
    while (!done) {
        uint32_t drawn_sprites {0};

        start_clear = std::chrono::steady_clock::now();

        GPU_Clear(screen);

        end_clear = std::chrono::steady_clock::now();

        dur_clear = std::chrono::duration_cast<std::chrono::microseconds>(
            end_clear - start_clear
        );

        // This allows controlling of the "camera", literally able to
        // pan/zoom/etc against the texture of the target.
        GPU_Camera cam = GPU_GetCamera(screen);
        // cam.x -= 1;
        // cam.y -= 1;
        // cam.angle += 0.01;
        // cam.zoom_x += 0.01;
        // cam.zoom_y += 0.01;
        // cam.z_near += 0.1;
        // cam.zoom_y += 0.01;
        GPU_SetCamera(screen, &cam);

        // This will fit the GPU_Target into the target rectangle within the
        // window, while the target itself can still be treated as though it is
        // mapped to fill the whole window itself.

        // GPU_SetViewport(screen, GPU_Rect(0, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2));

        // Poll and handle events (inputs, window resize, etc.)
        //
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard
        // flags to tell if dear imgui wants to use your inputs.
        //
        // - When io.WantCaptureMouse is true, do not dispatch mouse input
        // data to your main application.
        //
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard
        // input data to your main application.
        //
        // Generally you may always pass all inputs to dear imgui, and hide
        // them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_MOUSEMOTION:
                x_mouse = event.motion.x;
                y_mouse = event.motion.y;

                break;

            case SDL_MOUSEBUTTONDOWN:
                x_click = event.button.x;
                y_click = event.button.y;
                mouse_state = event.button.state;
                clicks = event.button.clicks;

                if (clicks > 2) {
                    clicks = ((clicks & 0b1) == 1) ? 1 : 2;
                }

                break;

            case SDL_MOUSEBUTTONUP:
                mouse_state = event.button.state;

                break;

            case SDL_QUIT:
                done = true;
                break;

            case SDL_WINDOWEVENT:
                if (
                    event.window.event == SDL_WINDOWEVENT_CLOSE &&
                    event.window.windowID == SDL_GetWindowID(window.Get())
                ) {
                    done = true;
                }

                break;

            default:
                break;
            }
        }

        // This block is drawing what is essentially the "map". Since it's
        // static, or changes rarely, we can cache this blitting as an image and
        // draw _that_ on subsequent frames, at least until anything changes.
        // This can provide a 50x+ speedup for large maps.
        if (map_image == nullptr) {
            // Only rendering one texture at a time substantially improves
            // performance. Switching between two textures back and forth is 3x+
            // slower, worse on Windows than in VM.
            for (const auto &node : map.get_nodes()) {
                const uint32_t x_node {node.x_coord};
                const uint32_t y_node {node.y_coord};

                const uint32_t x_frame {x_node * sprite_width};
                const uint32_t y_frame {y_node * sprite_height};

                if (node.blocking) {
                    GPU_Blit(
                        texture_obstacle,
                        nullptr,
                        screen,
                        x_frame,
                        y_frame
                    );

                    ++drawn_sprites;
                }
            }

            for (const auto &node : map.get_nodes()) {
                const uint32_t x_node {node.x_coord};
                const uint32_t y_node {node.y_coord};

                const uint32_t x_frame {x_node * sprite_width};
                const uint32_t y_frame {y_node * sprite_height};

                if (!node.blocking) {
                    GPU_Blit(
                        texture_open,
                        nullptr,
                        screen,
                        x_frame,
                        y_frame
                    );

                    ++drawn_sprites;
                }
            }

            map_image = GPU_CopyImageFromTarget(screen);
        }
        else {
            GPU_Rect map_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

            GPU_BlitRect(
                map_image,
                nullptr,
                screen,
                &map_rect
            );
        }

        // Get the position on the map that the click corresponds to, dropping
        // any sub-sprite positional information.
        //
        // NOTE: This could also be performed with a shift if the sprite
        // width/height is a power of 2.
        const uint32_t x_click_map = x_click / sprite_width;
        const uint32_t y_click_map = y_click / sprite_height;

        const uint32_t x_click_normal = x_click_map * sprite_width;
        const uint32_t y_click_normal = y_click_map * sprite_height;

        if (
            mouse_state == SDL_PRESSED &&
            !map.is_blocking(x_click_map, y_click_map)
        ) {
            GPU_Blit(
                texture_path,
                nullptr,
                screen,
                x_click_normal,
                y_click_normal
            );

            ++drawn_sprites;

            const uint32_t x_mouse_map = x_mouse / sprite_width;
            const uint32_t y_mouse_map = y_mouse / sprite_height;

            const uint32_t x_mouse_normal = x_mouse_map * sprite_width;
            const uint32_t y_mouse_normal = y_mouse_map * sprite_height;

            if (!map.is_blocking(x_mouse_map, y_mouse_map)) {
                GPU_Blit(
                    texture_path,
                    nullptr,
                    screen,
                    x_mouse_normal,
                    y_mouse_normal
                );

                ++drawn_sprites;

                const auto block_lamb = [](const MapNode &node) -> bool {
                    return !node.blocking;
                };

                {
                    Pathfind< Map, MapNode, decltype(block_lamb)> pathfinder(
                        map,
                        x_click_map, y_click_map,
                        x_mouse_map, y_mouse_map,
                        block_lamb
                    );

                    const auto path {pathfinder.get_path()};

                    for (const auto &[x_path_map, y_path_map] : path) {
                        const uint32_t x_path = x_path_map * sprite_width;
                        const uint32_t y_path = y_path_map * sprite_height;

                        GPU_Blit(
                            texture_path,
                            nullptr,
                            screen,
                            x_path,
                            y_path
                        );

                        ++drawn_sprites;
                    }
                }

                start_pathfinding = std::chrono::steady_clock::now();

                uint32_t loops {0};
                for (const auto &[x_start, y_start] : open_spaces) {
                    std::ranges::reverse_view rv_open_spaces {open_spaces};

                    for (const auto &[x_end, y_end] : rv_open_spaces) {
                        Pathfind<Map, MapNode, decltype(block_lamb)> pathfinder(
                            map,
                            x_start, y_start,
                            x_end, y_end,
                            block_lamb
                        );

                        const auto path {pathfinder.get_path()};

                        for (const auto &[x_path_map, y_path_map] : path) {
                            const uint32_t x_path = x_path_map * sprite_width;
                            const uint32_t y_path = y_path_map * sprite_height;

                            GPU_Blit(
                                texture_path,
                                nullptr,
                                screen,
                                x_path,
                                y_path
                            );

                            ++drawn_sprites;
                        }

                        ++loops;

                        if (loops > num_pathfinds) {
                            goto DONE;
                        }
                    }
                }

                DONE:
                ;

                end_pathfinding = std::chrono::steady_clock::now();

                dur_pathfinding =
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        end_pathfinding - start_pathfinding
                    );
            }
        }

        if (frames % 60 == 0) {
            start_font = std::chrono::steady_clock::now();
        }

        font.draw(
            screen, 0, 0, SDL_Color{0, 0, 0, 255},
            "Per-frame time (ms): %d", static_cast<uint32_t>(
                frame_dur.count() / 1000
            )
        );

        font.draw(
            screen, 0, font.getHeight(), SDL_Color{0, 0, 0, 255},
            "Flip time (us): %d", static_cast<uint32_t>(dur_flip.count())
        );

        font.draw(
            screen, 0, font.getHeight() * 2, SDL_Color{0, 0, 0, 255},
            "Clear time (us): %d", static_cast<uint32_t>(dur_clear.count())
        );

        font.draw(
            screen, 0, font.getHeight() * 3, SDL_Color{0, 0, 0, 255},
            "FPS: %d", static_cast<uint32_t>(
                (1000.0 / (frame_dur.count() / 1000.0))
            )
        );

        font.draw(
            screen, 0, font.getHeight() * 4, SDL_Color{0, 0, 0, 255},
            "Pathfinding per (us): %d", static_cast<uint32_t>(
                dur_pathfinding.count() / num_pathfinds
            )
        );

        font.draw(
            screen, 0, font.getHeight() * 5, SDL_Color{0, 0, 0, 255},
            "Pathfinding total (ms): %d", static_cast<uint32_t>(
                dur_pathfinding.count() / 1000
            )
        );

        font.draw(
            screen, 0, font.getHeight() * 6, SDL_Color{0, 0, 0, 255},
            "Font time (us): %d", static_cast<uint32_t>(dur_font.count())
        );

        font.draw(
            screen, 0, font.getHeight() * 7, SDL_Color{0, 0, 0, 255},
            "Drawn sprites: %d", drawn_sprites
        );

        font.drawBox(
            screen, SDL_Rect(0, font.getHeight() * 10, 400, 100),
            "The quick brown fox jumps over the lazy dog, the quick brown fox jumps over the lazy dog, the quick brown fox jumps over the lazy dog, the quick brown fox jumps over the lazy dog, the quick brown fox jumps over the lazy dog..."
        );

        font.drawColumn(
            screen, 600, font.getHeight() * 10, 400,
            "The quick brown fox jumps over the lazy dog, the quick brown fox jumps over the lazy dog, the quick brown fox jumps over the lazy dog, the quick brown fox jumps over the lazy dog, the quick brown fox jumps over the lazy dog..."
        );

        if (frames % 60 == 0) {
            end_font = std::chrono::steady_clock::now();

            dur_font = std::chrono::duration_cast<std::chrono::microseconds>(
                end_font - start_font
            );
        }

        start_flip = std::chrono::steady_clock::now();

        GPU_Flip(screen);

        end_flip = std::chrono::steady_clock::now();

        dur_flip = std::chrono::duration_cast<std::chrono::microseconds>(
            end_flip - start_flip
        );

        ++frames;

        end_frame = std::chrono::steady_clock::now();

        frame_dur = std::chrono::duration_cast<std::chrono::microseconds>(
            end_frame - start_frame
        );

        start_frame = std::chrono::steady_clock::now();
    }

    return;
}

void demo_gfx(
    SDL2pp::Window &window,
    SDL2pp::SDLTTF &sdl_ttf,
    GPU_Target* screen,
    SDL2pp::Mixer &mixer,
    ImGuiIO &io,
    SDL2pp::Font &sdl_font,
    ImFont* imgui_font
) {
    uint32_t sprite_1_width = SCREEN_WIDTH / 8;
    uint32_t sprite_1_height = SCREEN_HEIGHT / 8;

    GPU_Image* sprite2 = GPU_LoadImage("assets/test.png");
    auto free_sprite2 = Guard(
        [=]() {
            GPU_FreeImage(sprite2);
        }
    );

    const SDL_PixelFormat* const pixel_format = SDL_AllocFormat(
        SDL_PIXELFORMAT_ARGB8888
    );

    unsigned char pixels[
        sprite_1_width * sprite_1_height * pixel_format->BytesPerPixel
    ];

    unsigned char red {0};
    unsigned char green {0};
    unsigned char blue {0};
    unsigned char alpha {128};
    for (uint32_t i = 0; i < sprite_1_width * sprite_1_height; i++) {
        if (i > 255 * 12 * 4) {
            ((uint32_t*)pixels)[i] = SDL_MapRGBA(
                pixel_format, 128, 0, 128, alpha
            );
        }
        else {
            switch (i % 4) {
            case 0:
                if (red < 255) {
                    red++;
                }

                break;

            case 1:
                if (red == 255 && green < 255) {
                    green++;
                }

                break;

            case 2:
                if (green == 255 && blue < 255) {
                    blue++;
                }

                break;

            case 3:
                if (blue == 255) {
                    red = 0;
                    green = 0;
                    blue = 0;
                    alpha = 0;
                }

                break;

            default:
                assert(false);
            }

            ((uint32_t*)pixels)[i] = SDL_MapRGBA(
                pixel_format, red, green, blue, alpha
            );
        }
    }

    SDL2pp::Surface sprite1_surface(
        pixels,
        sprite_1_width,
        sprite_1_height,
        pixel_format->BitsPerPixel,
        pixel_format->BytesPerPixel * sprite_1_width,
        0,
        0,
        0,
        0
    );

    GPU_Image* sprite1 = GPU_CopyImageFromSurface(sprite1_surface.Get());
    auto free_sprite1 = Guard(
        [=]() {
            GPU_FreeImage(sprite1);
        }
    );

    GPU_BlitRect(sprite1, nullptr, screen, nullptr);

    {
        GPU_Rect target_rect(
            SCREEN_WIDTH / 4,
            SCREEN_HEIGHT / 4,
            SCREEN_WIDTH / 2,
            SCREEN_HEIGHT / 2
        );

        GPU_BlitRect(
            sprite1,
            nullptr,
            screen,
            &target_rect
        );
    }

    uint32_t pixels_2_width {255};
    uint32_t pixels_2_height {255};
    uint32_t pixels_2[pixels_2_width * pixels_2_height];

    red = 128;
    green = 0;
    blue = 128;
    alpha = 128;
    for (uint32_t y = 0; y < pixels_2_height; y++) {
        for (uint32_t x = 0; x < pixels_2_width; x++) {
            pixels_2[y * pixels_2_width + x] = SDL_MapRGBA(
                pixel_format, red, green, blue, 255
            );
        }

        red++;
    }

    SDL2pp::Surface sprite2_surface(
        pixels_2,
        pixels_2_width,
        pixels_2_height,
        pixel_format->BitsPerPixel,
        pixel_format->BytesPerPixel * pixels_2_width,
        0,
        0,
        0,
        0
    );

    GPU_Image* sprite2_surface_texture = GPU_CopyImageFromSurface(
        sprite2_surface.Get()
    );
    auto free_sprite2_surface_texture = Guard(
        [=]() {
            GPU_FreeImage(sprite2_surface_texture);
        }
    );

    GPU_SetBlending(sprite2_surface_texture, 1);
    GPU_SetBlendMode(sprite2_surface_texture, GPU_BLEND_NORMAL);
    GPU_SetRGBA(sprite2_surface_texture, 255, 255, 255, 192);

    {
        GPU_Rect target_rect(
            SCREEN_WIDTH / 4 * 3,
            SCREEN_HEIGHT / 8 * 3,
            pixels_2_width,
            pixels_2_height
        );

        GPU_BlitRect(
            sprite2_surface_texture,
            nullptr,
            screen,
            &target_rect
        );
    }

    // OGG sound file
    SDL2pp::Chunk sound("assets/test.ogg");

    auto text_surface = sdl_font.RenderText_Solid(
        "Hello, world!",
        SDL_Color{255, 255, 255, 255}
    );
    GPU_Image* text = GPU_CopyImageFromSurface(text_surface.Get());
    auto free_text = Guard(
        [=]() {
            GPU_FreeImage(text);
        }
    );

    {
        GPU_Rect target_rect(
            0, 0, text->base_w, text->base_h
        );

        GPU_BlitRect(
            text,
            nullptr,
            screen,
            &target_rect
        );
    }

    GPU_BlitRectX(
        sprite2,
        nullptr,
        screen,
        nullptr,
        45,
        text->base_w / 4,
        text->base_h,
        GPU_FLIP_NONE
    );

    GPU_Flip(screen);

    // Play our sound one time on a first available mixer channel
    mixer.PlayChannel(-1, sound);

    SDL_Event event;
    bool wait_for_next_stage {true};
    while (wait_for_next_stage)
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_KEYDOWN:
            wait_for_next_stage = false;
            break;

        default:
            break;
        }

        SDL_Delay(1);
    }

    bool show_demo_window = false;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    int32_t click_x {-1};
    int32_t click_y {-1};
    uint8_t clicks {0};
    uint8_t mouse_state {SDL_RELEASED};

    bool done = false;
    while (!done) {
        // Poll and handle events (inputs, window resize, etc.)
        //
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard
        // flags to tell if dear imgui wants to use your inputs.
        //
        // - When io.WantCaptureMouse is true, do not dispatch mouse input
        // data to your main application.
        //
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard
        // input data to your main application.
        //
        // Generally you may always pass all inputs to dear imgui, and hide
        // them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            switch (event.type) {
            case SDL_QUIT:
                done = true;
                break;

            case SDL_WINDOWEVENT:
                if (
                    event.window.event == SDL_WINDOWEVENT_CLOSE &&
                    event.window.windowID == SDL_GetWindowID(window.Get())
                ) {
                    done = true;
                }

                break;

            case SDL_MOUSEBUTTONDOWN:
                click_x = event.button.x;
                click_y = event.button.y;
                mouse_state = event.button.state;
                clicks = event.button.clicks;

                if (clicks > 2) {
                    clicks = ((clicks & 0b1) == 1) ? 1 : 2;
                }

                break;

            default:
                break;
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in
        // ImGui::ShowDemoWindow()! You can browse its code to learn more
        // about Dear ImGui!).
        if (show_demo_window) {
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        // 2. Show a simple window that we create ourselves. We use a
        // Begin/End pair to created a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            // Create a window called "Hello, world!" and append into it.
            ImGui::Begin("Hello, world!");

            // Display some text (you can use a format strings too)
            ImGui::Text("This is some useful text.");
            // Edit bools storing our window open/close state
            ImGui::Checkbox("Demo Window", &show_demo_window);
            ImGui::Checkbox("Another Window", &show_another_window);

            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            // Edit 3 floats representing a color
            ImGui::ColorEdit3("clear color", (float*)&clear_color);

            // Buttons return true when clicked (most widgets return true
            // when edited/activated)
            if (ImGui::Button("Button")) {
                counter++;
            }
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            if (ImGui::Button("Next Stage")) {
                done = true;
            }

            ImGui::Text("Mouse X = %d", click_x);
            ImGui::Text("Mouse Y = %d", click_y);

            switch (mouse_state) {
                case SDL_PRESSED:
                    ImGui::Text("Mouse PRESSED");
                    break;

                case SDL_RELEASED:
                    ImGui::Text("Mouse RELEASED");
                    break;

                default:
                    break;
            }

            ImGui::Text("Mouse Clicks = %d", clicks);

            ImGui::Text(
                "Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate
            );
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window) {
            // Pass a pointer to our bool variable (the window will have a
            // closing button that will clear the bool when clicked)
            ImGui::Begin("Another Window", &show_another_window);
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me")) {
                show_another_window = false;
            }

            GPU_Renderer* renderer = GPU_GetCurrentRenderer();
            GPU_RendererID id = renderer->id;

            ImGui::Text("Using renderer: %s (%d.%d)\n", id.name, id.major_version, id.minor_version);
            ImGui::Text("  Shader versions supported: %d to %d\n\n", renderer->min_shader_version, renderer->max_shader_version);

            ImGui::End();
        }

        // Rendering
        ImGui::Render();

        GPU_ClearColor(
            screen,
            SDL_Color(
                (uint8_t)(clear_color.x * 255),
                (uint8_t)(clear_color.y * 255),
                (uint8_t)(clear_color.z * 255),
                (uint8_t)(clear_color.w * 255)
            )
        );

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        GPU_Flip(screen);
    }

    return;
}

int main(int argc, char** argv) {
    entt::registry registry;

    std::cout << "Hello, world!" << std::endl;

    try {
        GPU_SetDebugLevel(GPU_DEBUG_LEVEL_MAX);

        // Unlock framerate.
        GPU_SetPreInitFlags(GPU_INIT_DISABLE_VSYNC);

        GPU_Target* screen = GPU_Init(
            SCREEN_WIDTH, SCREEN_HEIGHT,
            GPU_DEFAULT_INIT_FLAGS
        );

        // SDL_GL_SetSwapInterval(0);

        if (screen == nullptr) {
            std::cerr << "Failed to initialize sdl-gpu." << std::endl;

            exit(1);
        }

        auto guard = Guard(
            []() {
                GPU_Quit();
            }
        );

        GPU_SetDefaultAnchor(0, 0);

        // Likewise, init SDL_ttf library
        SDL2pp::SDLTTF sdl_ttf;

        SDL_GLContext& gl_context = screen->context->context;
        SDL_Window* window_ptr = SDL_GetWindowFromID(screen->context->windowID);
        SDL2pp::Window window(window_ptr);

        // Initialize audio mixer
        SDL2pp::Mixer mixer(
            MIX_DEFAULT_FREQUENCY,
            MIX_DEFAULT_FORMAT,
            MIX_DEFAULT_CHANNELS,
            4096
        );

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        ImGui_ImplSDL2_InitForOpenGL(window.Get(), gl_context);

        const char* glsl_version = "#version 120";
        ImGui_ImplOpenGL3_Init(glsl_version);

        // SDL_ttf font
        SDL2pp::Font sdl_font("assets/Vera.ttf", 20);
        NFont font("assets/Vera.ttf", 20);

        ImFont* imgui_font = io.Fonts->AddFontFromFileTTF("assets/Vera.ttf", 16.0f);
        IM_ASSERT(imgui_font != NULL);

        // demo_gfx(
        //     window,
        //     sdl_ttf,
        //     screen,
        //     mixer,
        //     io,
        //     sdl_font,
        //     imgui_font
        // );

        pathfind_gfx(
            registry,
            screen,
            window,
            mixer,
            io,
            sdl_font,
            font,
            imgui_font
        );

    // All SDL objects are released at this point or if an error occurs
    } catch (SDL2pp::Exception& e) {
        // Exception stores SDL_GetError() result and name of function which
        // failed
        std::cerr << "Error in: " << e.GetSDLFunction() << std::endl;
        std::cerr << "  Reason: " << e.GetSDLError() << std::endl;
    } catch (std::exception& e) {
        // This also works (e.g. "SDL_Init failed: No available video device")
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
