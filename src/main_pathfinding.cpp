#include <iostream>

#include "Draw.h"
#include "Map.h"
#include "Util.h"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"

#include <SDL2pp/SDL2pp.hh>

#include <entt/entt.hpp>

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

const uint32_t SCREEN_WIDTH {640 * 2};
const uint32_t SCREEN_HEIGHT {480 * 2};

void demo_gfx(
    SDL2pp::SDL &sdl,
    SDL2pp::Window &window,
    SDL2pp::SDLTTF &sdl_ttf,
    SDL2pp::Renderer &renderer,
    SDL2pp::Mixer &mixer,
    ImGuiIO &io,
    SDL2pp::Font &sdl_font,
    ImFont* imgui_font
) {
    uint32_t sprite_1_width = SCREEN_WIDTH / 8;
    uint32_t sprite_1_height = SCREEN_HEIGHT / 8;

    // SDL_image support
    SDL2pp::Texture sprite2(renderer, "assets/test.png");

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

    SDL2pp::Texture sprite1(renderer, sprite1_surface);

    // Also note a safe way to specify null rects and points
    renderer.Copy(sprite1, SDL2pp::NullOpt, SDL2pp::NullOpt);
    renderer.Copy(
        sprite1,
        SDL2pp::NullOpt,
        SDL2pp::Rect(
            SCREEN_WIDTH / 4,
            SCREEN_HEIGHT / 4,
            SCREEN_WIDTH / 2,
            SCREEN_HEIGHT / 2
        )
    );

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

    SDL2pp::Texture sprite2_surface_texture(renderer, sprite2_surface);
    sprite2_surface_texture.SetBlendMode(SDL_BLENDMODE_BLEND);
    sprite2_surface_texture.SetAlphaMod(192);

    renderer.Copy(
        sprite2_surface_texture,
        SDL2pp::NullOpt,
        SDL2pp::Rect(
            SCREEN_WIDTH / 4 * 3,
            SCREEN_HEIGHT / 8 * 3,
            pixels_2_width,
            pixels_2_height
        )
    );

    // OGG sound file
    SDL2pp::Chunk sound("assets/test.ogg");

    // Create texture from surface containing text rendered by SDL_ttf
    SDL2pp::Texture text(
        renderer,
        sdl_font.RenderText_Solid("Hello, world!",
        SDL_Color{255, 255, 255, 255})
    );

    // There are multiple convenient ways to construct e.g. a Rect;
    // Objects provide extensive set of getters
    renderer.Copy(
        text,
        SDL2pp::NullOpt,
        SDL2pp::Rect(SDL2pp::Point(0, 0), text.GetSize())
    );

    renderer.Copy(
        text,
        SDL2pp::NullOpt,
        SDL2pp::Rect(32, 32, 512, 256)
    );

    // Copy() is overloaded, providing access to both SDL_RenderCopy and
    // SDL_RenderCopyEx
    renderer.Copy(sprite2, SDL2pp::NullOpt, SDL2pp::NullOpt, 45.0);

    renderer.Present();

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
    while (!done)
    {
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
        ImGui_ImplSDLRenderer_NewFrame();
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
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        SDL_SetRenderDrawColor(
            renderer.Get(),
            (Uint8)(clear_color.x * 255),
            (Uint8)(clear_color.y * 255),
            (Uint8)(clear_color.z * 255),
            (Uint8)(clear_color.w * 255)
        );
        SDL_RenderClear(renderer.Get());
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer.Get());
    }

    return;
}

int main(int argc, char** argv) {
    entt::registry registry;

    std::cout << "Hello, world!" << std::endl;

    Pathfind<Map, MapNode> pathfind {};

    Map map {Map::gen_rand_map()};

    for (uint32_t i = 0; i < 100; ++i) {
        const auto [x, y] = map.get_rand_open_xy();

        const auto entity = registry.create();
        registry.emplace<Pos>(entity, (float)x + 0.5f, (float)y + 0.5f);
        registry.emplace<Pather>(entity);
    }

    Frame frame(map, registry);

    frame.draw_frame();

    try {
        // Init SDL; will be automatically deinitialized when the object is
        // destroyed
        SDL2pp::SDL sdl(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

        // Likewise, init SDL_ttf library
        SDL2pp::SDLTTF sdl_ttf;

        // Straightforward wrappers around corresponding SDL2 objects
        // These take full care of proper object destruction and error checking
        SDL2pp::Window window(
            "libSDL2pp demo",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            SCREEN_WIDTH,
            SCREEN_HEIGHT,
            SDL_WINDOW_RESIZABLE
        );

        SDL2pp::Renderer renderer(window, -1, SDL_RENDERER_ACCELERATED);

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
        ImGui_ImplSDL2_InitForSDLRenderer(window.Get(), renderer.Get());
        ImGui_ImplSDLRenderer_Init(renderer.Get());

        // SDL_ttf font
        SDL2pp::Font sdl_font("assets/Vera.ttf", 20);

        ImFont* imgui_font = io.Fonts->AddFontFromFileTTF("assets/Vera.ttf", 16.0f);
        IM_ASSERT(imgui_font != NULL);

        demo_gfx(
            sdl,
            window,
            sdl_ttf,
            renderer,
            mixer,
            io,
            sdl_font,
            imgui_font
        );

        SDL_RenderClear(renderer.Get());
        SDL_RenderPresent(renderer.Get());

        SDL_Delay(1000);

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
