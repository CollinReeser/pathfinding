#include <iostream>
#include <random>
#include <vector>

#include "Util.h"

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"

#include <SDL2pp/SDL2pp.hh>

#include <entt/entt.hpp>

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

// node_t represents a single node in the pathfinding graph. These are acquired
// from interactions with map_t.
//
// Type map_t must implement member methods with signatures:
//
// std::vector<node_t> next_nodes(const node_t &cur)
template <typename map_t, typename node_t>
class Pathfind {
private:
public:
    Pathfind() {}
};

class Map;

class MapNode {
private:
    const uint32_t x_coord;
    const uint32_t y_coord;

    bool blocking;

public:
    MapNode(
        const uint32_t x_coord,
        const uint32_t y_coord,
        const bool blocking
    ):
        x_coord(x_coord),
        y_coord(y_coord),
        blocking(blocking)
    {}

    friend Map;
};

class Map {
private:
    static inline std::random_device rd;
    static inline std::mt19937 gen {rd()};

    std::vector<MapNode> nodes;

public:
    // The x-coordinate range.
    const uint32_t width {64};
    // The y-coordinate range.
    const uint32_t height {32};

    Map(const uint32_t width = 64, const uint32_t height = 32):
        width(width),
        height(height)
    {}

    Map(Map&& o) noexcept:
        nodes(std::move(o.nodes)),
        width(o.width),
        height(o.height)
    {}

    static Map gen_rand_map() {
        std::uniform_int_distribution<> rng(0, 5);

        Map map {};

        map.nodes.reserve(map.width * map.height);

        for (uint32_t i = 0; i < map.nodes.capacity(); ++i) {
            const auto [x, y] = get_node_xy(i, map.width);

            map.nodes.emplace_back(x, y, rng(Map::gen) > 4);
        }

        return map;
    }

    void draw_map_to_frame(std::vector<char> &tiles) const {
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                const uint32_t i {get_node_index(x, y, width)};
                if (nodes[i].blocking) {
                    tiles[i] = 'X';
                }
                else {
                    tiles[i] = '.';
                }
            }
        }
    }

    std::pair<uint32_t, uint32_t> get_rand_open_xy() {
        std::uniform_int_distribution<> rng_w(0, width - 1);
        std::uniform_int_distribution<> rng_h(0, height - 1);

        uint32_t x;
        uint32_t y;
        uint32_t i;

        do {
            x = rng_w(Map::gen);
            y = rng_h(Map::gen);

            i = get_node_index(x, y, width);
        } while (!nodes[i].blocking);

        return {x, y};
    }
};

struct Pos {
    float x;
    float y;
};

struct Pather {};

struct Frame {
private:
    const uint32_t width;
    const uint32_t height;

    std::vector<char> tiles;

public:
    Frame(
        const Map &map, const entt::registry &registry,
        const uint32_t view_width = 128, const uint32_t view_height = 128
    ):
        width(map.width),
        height(map.height)
    {
        tiles.reserve(width * height);

        map.draw_map_to_frame(tiles);

        const auto view {registry.view<const Pos>()};

        for (const auto &[entity, pos]: view.each()) {
            const uint32_t i = get_node_index(
                (uint32_t)pos.x, (uint32_t)pos.y, width
            );

            tiles[i] = 'O';
        }
    }

    void draw_frame() {
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                std::cout << tiles[get_node_index(x, y, width)];
            }

            std::cout << std::endl;
        }
    }
};

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

    const uint32_t SCREEN_WIDTH {640 * 2};
    const uint32_t SCREEN_HEIGHT {480 * 2};

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

        SDL2pp::Texture sprite1(
            renderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STATIC,
            16, 16
        );

        // SDL_image support
        SDL2pp::Texture sprite2(renderer, "assets/test.png");

        // Initialize audio mixer
        SDL2pp::Mixer mixer(
            MIX_DEFAULT_FREQUENCY,
            MIX_DEFAULT_FORMAT,
            MIX_DEFAULT_CHANNELS,
            4096
        );

        // OGG sound file
        SDL2pp::Chunk sound("assets/test.ogg");

        // Create texture from surface containing text rendered by SDL_ttf
        SDL2pp::Texture text(
            renderer,
            sdl_font.RenderText_Solid("Hello, world!",
            SDL_Color{255, 255, 255, 255})
        );

        unsigned char pixels[16 * 16 * 4];

        // Note proper constructor for Rect
        sprite1.Update(SDL2pp::Rect(0, 0, 16, 16), pixels, 16 * 4);

        // Most setter methods are chainable
        renderer.SetLogicalSize(
            SCREEN_WIDTH, SCREEN_HEIGHT
        ).SetDrawColor(0, 16, 32).Clear();

        // Also note a safe way to specify null rects and points
        renderer.Copy(sprite1, SDL2pp::NullOpt, SDL2pp::NullOpt);

        // There are multiple convenient ways to construct e.g. a Rect;
        // Objects provide extensive set of getters
        renderer.Copy(
            text,
            SDL2pp::NullOpt,
            SDL2pp::Rect(SDL2pp::Point(0, 0), text.GetSize())
        );

        // Copy() is overloaded, providing access to both SDL_RenderCopy and
        // SDL_RenderCopyEx
        renderer.Copy(sprite2, SDL2pp::NullOpt, SDL2pp::NullOpt, 45.0);

        renderer.Present();

        // Play our sound one time on a first available mixer channel
        mixer.PlayChannel(-1, sound);

        // You can still access wrapped C SDL types
        // SDL_Renderer* sdl_renderer = renderer.Get();

        // Of course, C SDL2 API is still perfectly valid
        SDL_Delay(1000);

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
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
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
