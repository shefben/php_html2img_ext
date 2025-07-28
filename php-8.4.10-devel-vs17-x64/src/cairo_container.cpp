#include "cairo_container.hpp"
#include <cstring>
#include <cctype>
#include <cairo.h>
#include <iostream>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <regex>
#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#endif

// For FreeType font loading
#include <cairo-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H

// For image loading - you may need to install these libraries
// For now, we'll implement basic GIF loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace std::string_literals;

/* Global FreeType library instance */
static FT_Library ft_library = nullptr;
static std::unordered_map<std::string, cairo_font_face_t*> font_cache;
static std::unordered_map<std::string, cairo_surface_t*> image_cache;


/* -------------------------------------------------------------- */

CairoContainer::CairoContainer(CairoCanvas& c,
                               const std::filesystem::path& base,
                               const std::filesystem::path& fonts,
                               bool allow_remote)
  : canvas_(c), base_path_(base),
    font_dir_(fonts), allow_remote_(allow_remote) {
    
    // Initialize FreeType if not already done
    if (!ft_library) {
        FT_Error error = FT_Init_FreeType(&ft_library);
        if (error) {
            std::cout << "Failed to initialize FreeType library" << std::endl;
        } else {
            std::cout << "FreeType library initialized successfully" << std::endl;
        }
    }
    
    std::cout << "CairoContainer created with font_dir: " << fonts << std::endl;
    std::cout << "Base path: " << base << std::endl;
}

/* helper – search for font files with various extensions */
static std::filesystem::path find_font_file(const std::filesystem::path& base_dir,
                                           const std::string& family)
{
    static const char* exts[] = { ".otf", ".ttf", ".ttc" };
    
    // First try exact match
    for(const char* ext: exts){
        std::filesystem::path p = base_dir / (family + ext);
        if(std::filesystem::exists(p)) {
            std::cout << "Found font file: " << p << std::endl;
            return p;
        }
    }
    
    // Try case-insensitive search
    std::string lower_family = family;
    std::transform(lower_family.begin(), lower_family.end(), lower_family.begin(), ::tolower);
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(base_dir)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                std::string lower_filename = filename;
                std::transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(), ::tolower);
                
                if (lower_filename.find(lower_family) != std::string::npos) {
                    for(const char* ext: exts) {
                        size_t ext_len = strlen(ext);
                        if (lower_filename.length() >= ext_len && 
                            lower_filename.substr(lower_filename.length() - ext_len) == ext) {
                            std::cout << "Found font file (case-insensitive): " << entry.path() << std::endl;
                            return entry.path();
                        }
                    }
                }
            }
        }
    } catch (...) {
        // Directory iteration failed, continue with fallbacks
    }
    
    return {};
}

cairo_font_face_t* load_font_from_file(const std::filesystem::path& font_path) {
    if (!ft_library) return nullptr;
    
    std::string path_str = font_path.string();
    
    // Check cache first
    auto it = font_cache.find(path_str);
    if (it != font_cache.end()) {
        std::cout << "Using cached font: " << path_str << std::endl;
        return it->second;
    }
    
    FT_Face ft_face;
    FT_Error error = FT_New_Face(ft_library, path_str.c_str(), 0, &ft_face);
    
    if (error) {
        std::cout << "Failed to load font from: " << path_str << " (error: " << error << ")" << std::endl;
        return nullptr;
    }
    
    cairo_font_face_t* font_face = cairo_ft_font_face_create_for_ft_face(ft_face, 0);
    
    if (cairo_font_face_status(font_face) != CAIRO_STATUS_SUCCESS) {
        std::cout << "Failed to create Cairo font face from: " << path_str << std::endl;
        cairo_font_face_destroy(font_face);
        FT_Done_Face(ft_face);
        return nullptr;
    }
    
    // Cache the font face
    font_cache[path_str] = font_face;
    std::cout << "Successfully loaded and cached font: " << path_str << std::endl;
    
    return font_face;
}

/* Parse CSS color value (supports rgb(), rgba(), hex, and named colors) */
static void parse_css_color(const std::string& color_str, double& r, double& g, double& b, double& a) {
    r = g = b = 0.0;
    a = 1.0;
    
    std::string color = color_str;
    // Remove whitespace
    color.erase(std::remove_if(color.begin(), color.end(), ::isspace), color.end());
    
    if (color.empty()) return;
    
    // Handle rgba() format
    if (color.substr(0, 5) == "rgba(") {
        std::regex rgba_regex(R"(rgba\((\d+),(\d+),(\d+),([0-9.]+)\))");
        std::smatch matches;
        if (std::regex_match(color, matches, rgba_regex)) {
            r = std::stoi(matches[1].str()) / 255.0;
            g = std::stoi(matches[2].str()) / 255.0;
            b = std::stoi(matches[3].str()) / 255.0;
            a = std::stod(matches[4].str());
            return;
        }
    }
    
    // Handle rgb() format
    if (color.substr(0, 4) == "rgb(") {
        std::regex rgb_regex(R"(rgb\((\d+),(\d+),(\d+)\))");
        std::smatch matches;
        if (std::regex_match(color, matches, rgb_regex)) {
            r = std::stoi(matches[1].str()) / 255.0;
            g = std::stoi(matches[2].str()) / 255.0;
            b = std::stoi(matches[3].str()) / 255.0;
            a = 1.0;
            return;
        }
    }
    
    // Handle hex format
    if (color[0] == '#') {
        std::string hex = color.substr(1);
        if (hex.length() == 6) {
            r = std::stoi(hex.substr(0, 2), nullptr, 16) / 255.0;
            g = std::stoi(hex.substr(2, 2), nullptr, 16) / 255.0;
            b = std::stoi(hex.substr(4, 2), nullptr, 16) / 255.0;
            a = 1.0;
            return;
        }
    }
    
    // Handle named colors (basic set)
    if (color == "black") { r = 0; g = 0; b = 0; a = 1; }
    else if (color == "white") { r = 1; g = 1; b = 1; a = 1; }
    else if (color == "red") { r = 1; g = 0; b = 0; a = 1; }
    else if (color == "green") { r = 0; g = 1; b = 0; a = 1; }
    else if (color == "blue") { r = 0; g = 0; b = 1; a = 1; }
    else if (color == "transparent") { r = 0; g = 0; b = 0; a = 0; }
}

/* Parse text-shadow CSS property */
static std::vector<TextShadow> parse_text_shadow(const std::string& shadow_str) {
    std::vector<TextShadow> shadows;
    
    if (shadow_str.empty() || shadow_str == "none") {
        return shadows;
    }
    
    std::cout << "Parsing text-shadow: '" << shadow_str << "'" << std::endl;
    
    // Split by comma for multiple shadows
    std::stringstream ss(shadow_str);
    std::string shadow_def;
    
    while (std::getline(ss, shadow_def, ',')) {
        TextShadow shadow;
        
        // Trim whitespace
        shadow_def.erase(0, shadow_def.find_first_not_of(" \t"));
        shadow_def.erase(shadow_def.find_last_not_of(" \t") + 1);
        
        std::cout << "Processing shadow definition: '" << shadow_def << "'" << std::endl;
        
        // Parse shadow definition: [offset-x] [offset-y] [blur-radius] [color]
        // Example: "1px 1px 0 rgba(0,0,0,0.8)" or "2px 2px 4px #000000"
        
        std::istringstream iss(shadow_def);
        std::string token;
        std::vector<std::string> tokens;
        
        while (iss >> token) {
            tokens.push_back(token);
        }
        
        if (tokens.size() >= 2) {
            // Parse offset-x (remove 'px' suffix)
            std::string x_str = tokens[0];
            if (x_str.length() > 2 && x_str.substr(x_str.length() - 2) == "px") {
                x_str = x_str.substr(0, x_str.length() - 2);
            }
            shadow.offset_x = std::stod(x_str);
            
            // Parse offset-y (remove 'px' suffix)
            std::string y_str = tokens[1];
            if (y_str.length() > 2 && y_str.substr(y_str.length() - 2) == "px") {
                y_str = y_str.substr(0, y_str.length() - 2);
            }
            shadow.offset_y = std::stod(y_str);
            
            // Parse blur-radius if present
            if (tokens.size() >= 3) {
                std::string blur_str = tokens[2];
                // Check if this token is a color (starts with # or is a named color or contains '(')
                if (blur_str[0] != '#' && blur_str.find('(') == std::string::npos && 
                    blur_str != "black" && blur_str != "white" && blur_str != "red" && 
                    blur_str != "green" && blur_str != "blue" && blur_str != "transparent") {
                    // It's likely a blur radius, not a color
                    if (blur_str.length() > 2 && blur_str.substr(blur_str.length() - 2) == "px") {
                        blur_str = blur_str.substr(0, blur_str.length() - 2);
                    }
                    shadow.blur_radius = std::stod(blur_str);
                    
                    // Parse color if present
                    if (tokens.size() >= 4) {
                        std::string color_str;
                        // Handle multi-token colors like "rgba(0, 0, 0, 0.8)"
                        for (size_t i = 3; i < tokens.size(); ++i) {
                            if (i > 3) color_str += " ";
                            color_str += tokens[i];
                        }
                        parse_css_color(color_str, shadow.red, shadow.green, shadow.blue, shadow.alpha);
                    } else {
                        // Default to black
                        shadow.red = shadow.green = shadow.blue = 0.0;
                        shadow.alpha = 1.0;
                    }
                } else {
                    // Token 2 is actually a color, no blur radius
                    shadow.blur_radius = 0.0;
                    std::string color_str;
                    for (size_t i = 2; i < tokens.size(); ++i) {
                        if (i > 2) color_str += " ";
                        color_str += tokens[i];
                    }
                    parse_css_color(color_str, shadow.red, shadow.green, shadow.blue, shadow.alpha);
                }
            } else {
                // No blur or color specified, default to black with no blur
                shadow.blur_radius = 0.0;
                shadow.red = shadow.green = shadow.blue = 0.0;
                shadow.alpha = 1.0;
            }
            
            std::cout << "Parsed shadow: offset(" << shadow.offset_x << ", " << shadow.offset_y 
                     << ") blur(" << shadow.blur_radius << ") color(" << shadow.red << ", " 
                     << shadow.green << ", " << shadow.blue << ", " << shadow.alpha << ")" << std::endl;
            
            shadows.push_back(shadow);
        }
    }
    
    std::cout << "Total shadows parsed: " << shadows.size() << std::endl;
    return shadows;
}

litehtml::uint_ptr CairoContainer::create_font(const litehtml::font_description& d,
                                               const litehtml::document*,
                                               litehtml::font_metrics* fm)
{
    std::cout << "create_font called - family: '" << d.family << "', size: " << d.size << ", weight: " << d.weight << std::endl;
    
    ToyFont* F = new ToyFont;
    F->size_px = d.size ? d.size : 16;
    F->face = nullptr;
    
    std::string font_family = d.family;
    if(font_family.empty()) {
        font_family = "Arial";
    }
    
    std::cout << "Looking for font: " << font_family << " at " << F->size_px << "px" << std::endl;
    
    // Try to load custom font from file first
    std::filesystem::path font_path = find_font_file(base_path_, font_family);
    if (font_path.empty()) {
        font_path = find_font_file(font_dir_, font_family);
    }
    
    if (!font_path.empty()) {
        F->face = load_font_from_file(font_path);
    }
    
    // If custom font loading failed, fall back to system fonts
    if (!F->face) {
        std::cout << "Custom font not found, trying system fonts" << std::endl;
        
        cairo_font_weight_t weight = CAIRO_FONT_WEIGHT_NORMAL;
        if(d.weight >= 700) {
            weight = CAIRO_FONT_WEIGHT_BOLD;
        }

        F->face = cairo_toy_font_face_create(
            font_family.c_str(),
            CAIRO_FONT_SLANT_NORMAL,
            weight);

        if(cairo_font_face_status(F->face) != CAIRO_STATUS_SUCCESS){
            std::cout << "System font failed, trying fallbacks" << std::endl;
            cairo_font_face_destroy(F->face);
            
            // Try common fallbacks
            const char* fallbacks[] = {"Arial", "Verdana", "sans-serif", "serif"};
            for (const char* fallback : fallbacks) {
                F->face = cairo_toy_font_face_create(fallback, CAIRO_FONT_SLANT_NORMAL, weight);
                if (cairo_font_face_status(F->face) == CAIRO_STATUS_SUCCESS) {
                    std::cout << "Using fallback font: " << fallback << std::endl;
                    break;
                }
                cairo_font_face_destroy(F->face);
                F->face = nullptr;
            }
        }
    }
    
    if (!F->face) {
        std::cout << "Font creation completely failed!" << std::endl;
        delete F;
        return 0;
    }

    // Calculate font metrics
    if(fm){
        // Create a temporary Cairo context to measure the font
        cairo_surface_t* temp_surface = cairo_image_surface_create(CAIRO_FORMAT_A8, 1, 1);
        cairo_t* temp_cr = cairo_create(temp_surface);
        cairo_set_font_face(temp_cr, F->face);
        cairo_set_font_size(temp_cr, F->size_px);
        
        cairo_font_extents_t font_extents;
        cairo_font_extents(temp_cr, &font_extents);
        
        fm->height = static_cast<int>(font_extents.height);
        fm->ascent = static_cast<int>(font_extents.ascent);
        fm->descent = static_cast<int>(font_extents.descent);
        fm->x_height = static_cast<int>(font_extents.height * 0.5);
        
        cairo_destroy(temp_cr);
        cairo_surface_destroy(temp_surface);
        
        std::cout << "Font metrics - height: " << fm->height << ", ascent: " << fm->ascent << ", descent: " << fm->descent << std::endl;
    }
    
    pool_.push_back(F);
    std::cout << "Font created successfully, ptr: " << reinterpret_cast<void*>(F) << std::endl;
    return reinterpret_cast<litehtml::uint_ptr>(F);
}

void CairoContainer::delete_font(litehtml::uint_ptr f)
{
    std::cout << "delete_font called for ptr: " << reinterpret_cast<void*>(f) << std::endl;
    auto* F = reinterpret_cast<ToyFont*>(f);
    if(F) {
        // Don't destroy cached font faces, they're managed globally
        delete F;
    }
}

int CairoContainer::text_width(const char* txt, litehtml::uint_ptr f)
{
    if(!txt || !f) {
        std::cout << "text_width called with null parameters" << std::endl;
        return 0;
    }
    
    std::cout << "text_width called for text: '" << txt << "'" << std::endl;
    
    ToyFont* F = reinterpret_cast<ToyFont*>(f);
    cairo_surface_t* dummy = cairo_image_surface_create(CAIRO_FORMAT_A8, 1, 1);
    cairo_t* cr = cairo_create(dummy);
    cairo_set_font_face(cr, F->face);
    cairo_set_font_size(cr, F->size_px);

    cairo_text_extents_t ext;
    cairo_text_extents(cr, txt, &ext);

    cairo_destroy(cr);
    cairo_surface_destroy(dummy);
    
    int width = static_cast<int>(ext.x_advance + 0.5);
    std::cout << "text_width result: " << width << std::endl;
    return width;
}

// You can call this in your PHP extension when processing the HTML:
// In php_html2img.cpp, modify the HTML processing to extract shadows:

// Add this to your php_html2img.cpp before rendering:
std::string extract_and_store_text_shadows(const std::string& html_str) {
    // Extract all text-shadow definitions from CSS
    std::string shadows;
    
    // Look for text-shadow in style tags
    std::regex style_block_regex(R"(<style[^>]*>(.*?)</style>)", std::regex_constants::icase | std::regex_constants::ECMAScript);
    std::smatch style_matches;
    
    auto search_start = html_str.cbegin();
    while (std::regex_search(search_start, html_str.cend(), style_matches, style_block_regex)) {
        std::string css_content = style_matches[1].str();
        std::cout << "Found CSS block: " << css_content << std::endl;
        
        // Look for text-shadow properties
        std::regex text_shadow_regex(R"(text-shadow\s*:\s*([^;}]+)[;}])");
        std::smatch shadow_matches;
        
        auto css_search_start = css_content.cbegin();
        while (std::regex_search(css_search_start, css_content.cend(), shadow_matches, text_shadow_regex)) {
            std::string shadow_def = shadow_matches[1].str();
            std::cout << "Found text-shadow definition: " << shadow_def << std::endl;
            
            if (!shadows.empty()) shadows += " | ";
            shadows += shadow_def;
            
            css_search_start = shadow_matches.suffix().first;
        }
        
        search_start = style_matches.suffix().first;
    }
    
    return shadows;
}

// ─────────────────────────────────────────────────────────────────────────────
//  tiny box‑blur helper  (only include once in this .cpp file)
// ─────────────────────────────────────────────────────────────────────────────
static void blur_surface(cairo_surface_t* s, int radius)
{
    if (radius <= 0) return;
    const int w = cairo_image_surface_get_width(s);
    const int h = cairo_image_surface_get_height(s);
    const int stride = cairo_image_surface_get_stride(s);
    uint32_t* data = reinterpret_cast<uint32_t*>(cairo_image_surface_get_data(s));
    std::vector<uint32_t> tmp((stride/4) * h);
    const int win = radius * 2 + 1;

    /* horizontal pass */
    for (int y = 0; y < h; ++y) {
        uint32_t* src = data + y*(stride/4);
        uint32_t* dst = tmp .data() + y*(stride/4);
        uint64_t A=0,R=0,G=0,B=0;
        for (int x=-radius; x<=radius; ++x){
            uint32_t p = src[ std::clamp(x,0,w-1) ];
            A +=  p>>24;  R += (p>>16)&0xFF;  G += (p>>8)&0xFF;  B += p&0xFF;
        }
        for (int x=0; x<w; ++x){
            dst[x] = ((A/win)<<24)|((R/win)<<16)|((G/win)<<8)|(B/win);
            int L=x-radius, Rg=x+radius+1;
            uint32_t pL=src[ std::clamp(L ,0,w-1) ],
                     pR=src[ std::clamp(Rg,0,w-1) ];
            A += (pR>>24)-(pL>>24);
            R += ((pR>>16)&0xFF)-((pL>>16)&0xFF);
            G += ((pR>>8 )&0xFF)-((pL>>8 )&0xFF);
            B += (pR&0xFF)-(pL&0xFF);
        }
    }
    /* vertical pass */
    for (int x=0; x<w; ++x){
        uint64_t A=0,R=0,G=0,B=0;
        for (int y=-radius; y<=radius; ++y){
            uint32_t p=tmp[ std::clamp(y,0,h-1)*(stride/4)+x ];
            A +=  p>>24;  R += (p>>16)&0xFF;  G += (p>>8)&0xFF;  B += p&0xFF;
        }
        for (int y=0; y<h; ++y){
            data[y*(stride/4)+x]=((A/win)<<24)|((R/win)<<16)|((G/win)<<8)|(B/win);
            int T=y-radius, Bm=y+radius+1;
            uint32_t pT=tmp[ std::clamp(T ,0,h-1)*(stride/4)+x ],
                     pB=tmp[ std::clamp(Bm,0,h-1)*(stride/4)+x ];
            A += (pB>>24)-(pT>>24);
            R += ((pB>>16)&0xFF)-((pT>>16)&0xFF);
            G += ((pB>>8 )&0xFF)-((pT>>8 )&0xFF);
            B += (pB&0xFF)-(pT&0xFF);
        }
    }
    cairo_surface_mark_dirty(s);
}

// ─────────────────────────────────────────────────────────────────────────────
//  final draw_text implementation
// ─────────────────────────────────────────────────────────────────────────────
void CairoContainer::draw_text(litehtml::uint_ptr ctx,
                               const char*        txt,
                               litehtml::uint_ptr fnt,
                               const litehtml::web_color col,
                               const litehtml::position& pos)
{
    if (!txt || !*txt || !fnt) return;

    auto* F  = reinterpret_cast<ToyFont*>(fnt);
    auto* el = reinterpret_cast<litehtml::element*>(ctx);  // ctx IS the element

    cairo_t* cr = cairo_create(canvas_.surface());
    cairo_set_font_face(cr, F->face);
    cairo_set_font_size(cr, F->size_px);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

    cairo_font_extents_t fm;
    cairo_font_extents(cr, &fm);
    const double base_x = static_cast<double>(pos.x);
    const double base_y = static_cast<double>(pos.y) + fm.ascent;

    std::vector<litehtml::text_shadow> shadows;
    if (el) {
        shadows = el->css().get_text_shadow_list();
        shadows.erase(
            std::remove_if(shadows.begin(), shadows.end(),
                [](const litehtml::text_shadow& s) {
                    return s.color.alpha == 0;
                }),
            shadows.end()
        );
    }

    std::u32string utf32 = litehtml::utf8_to_utf32(txt);
    double cursor_x = 0.0;

    for (char32_t ch : utf32) {
        std::string utf8_char = litehtml::utf32_to_utf8(std::u32string(1, ch));


        cairo_text_extents_t te;
        cairo_text_extents(cr, utf8_char.c_str(), &te);

        for (const auto& sh : shadows) {
            // CSS blur radius (0,2,4,6, etc)
            const int blur_r = static_cast<int>(std::round(sh.blur.val()));
            // do 3 passes of box‑blur to approximate CSS Gaussian
            const int blur_passes = 3;
            // pad must cover blur_r * passes
            const int pad    = blur_r * blur_passes + 2;

            int texW = static_cast<int>(te.x_advance + pad * 2);
            int texH = static_cast<int>(fm.height   + pad * 2);

            cairo_surface_t* tmp = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, texW, texH);
            cairo_t* tc = cairo_create(tmp);
            cairo_set_antialias(tc, CAIRO_ANTIALIAS_BEST);

            cairo_set_source_rgba(tc,
                sh.color.red   / 255.0,
                sh.color.green / 255.0,
                sh.color.blue  / 255.0,
                sh.color.alpha / 255.0);
            cairo_set_font_face(tc, F->face);
            cairo_set_font_size(tc, F->size_px);

            cairo_move_to(tc, pad - te.x_bearing, pad + fm.ascent);
            cairo_show_text(tc, utf8_char.c_str());
            cairo_destroy(tc);

            // triple‑pass box blur ≈ Gaussian
            for (int i = 0; i < blur_passes; ++i) {
                blur_surface(tmp, blur_r);
            }

            cairo_set_source_surface(cr, tmp,
                base_x + cursor_x + sh.offset_x.val() - pad + te.x_bearing,
                base_y + sh.offset_y.val() - pad - fm.ascent);
            cairo_paint(cr);
            cairo_surface_destroy(tmp);
        }

        // draw the glyph itself
        cairo_set_source_rgba(cr,
            col.red   / 255.0,
            col.green / 255.0,
            col.blue  / 255.0,
            col.alpha / 255.0);
        cairo_move_to(cr, base_x + cursor_x, base_y);
        cairo_show_text(cr, utf8_char.c_str());

        cursor_x += te.x_advance;
    }

    cairo_destroy(cr);
}



void CairoContainer::draw_solid_fill(litehtml::uint_ptr,
                                     const litehtml::background_layer& lay,
                                     const litehtml::web_color& c)
{
    std::cout << "draw_solid_fill called" << std::endl;
    cairo_t* cr = cairo_create(canvas_.surface());
    cairo_set_source_rgba(cr,
         c.red/255.0, c.green/255.0, c.blue/255.0, c.alpha/255.0);

    const auto& b = lay.clip_box;

    double x = static_cast<double>(b.left());   
    double y = static_cast<double>(b.top());    
    double w = static_cast<double>(b.width);    
    double h = static_cast<double>(b.height);   

    std::cout << "Fill rect: " << x << ", " << y << ", " << w << ", " << h << std::endl;

    cairo_rectangle(cr, x, y, w, h);
    cairo_fill(cr);
    cairo_destroy(cr);
}

/* ---- misc helpers ----------------------------------------- */
int CairoContainer::pt_to_px(int pt) const { 
    return static_cast<int>((pt * 96.0 / 72.0) + 0.5);
}

void CairoContainer::transform_text(litehtml::string& t, litehtml::text_transform tt)
{
    std::cout << "transform_text called on: '" << t << "'" << std::endl;
    switch(tt) {
        case litehtml::text_transform_uppercase:
            for(char& ch : t) ch = static_cast<char>(std::toupper(ch));
            break;
        case litehtml::text_transform_lowercase:
            for(char& ch : t) ch = static_cast<char>(std::tolower(ch));
            break;
        case litehtml::text_transform_capitalize:
            {
                bool capitalize_next = true;
                for(char& ch : t) {
                    if(std::isspace(ch)) {
                        capitalize_next = true;
                    } else if(capitalize_next) {
                        ch = static_cast<char>(std::toupper(ch));
                        capitalize_next = false;
                    }
                }
            }
            break;
        default:
            break;
    }
    std::cout << "transform_text result: '" << t << "'" << std::endl;
}

void CairoContainer::get_viewport(litehtml::position& v) const
{
    auto s = canvas_.surface();
    v.x = 0; 
    v.y = 0;
    v.width = cairo_image_surface_get_width(s);
    v.height = cairo_image_surface_get_height(s);
    std::cout << "get_viewport: " << v.width << "x" << v.height << std::endl;
}

void CairoContainer::get_media_features(litehtml::media_features& m) const
{
    auto s = canvas_.surface();
    m.type = litehtml::media_type_screen;
    m.width = cairo_image_surface_get_width(s);
    m.height = cairo_image_surface_get_width(s);
    m.color = 8;
    m.monochrome = 0;
    m.color_index = 0;
    m.resolution = 96;
    std::cout << "get_media_features called" << std::endl;
}

void CairoContainer::draw_image(litehtml::uint_ptr, const litehtml::background_layer& layer,
                               const std::string& url, const std::string& base_url)
{
    std::cout << "draw_image called for: " << url << std::endl;
    
    // Try both the original URL and the resolved path
    auto it = image_cache.find(url);
    if (it == image_cache.end()) {
        // Try with base path resolution
        std::string resolved_path = url;
        if (url.find("://") == std::string::npos && url[0] != '/') {
            resolved_path = (base_path_ / url).string();
        }
        it = image_cache.find(resolved_path);
    }
    
    if (it == image_cache.end()) {
        std::cout << "Image not found in cache: " << url << std::endl;
        // Try to load it on demand
        load_image(url.c_str(), base_url.c_str(), false);
        it = image_cache.find(url);
        if (it == image_cache.end()) {
            return;
        }
    }
    
    cairo_surface_t* image_surface = it->second;
    cairo_t* cr = cairo_create(canvas_.surface());
    
    const auto& pos = layer.clip_box;
    
    std::cout << "Drawing image at: " << pos.left() << ", " << pos.top() 
              << " size: " << pos.width << "x" << pos.height << std::endl;
    
    // Set up transformation for positioning and scaling
    cairo_save(cr);
    cairo_translate(cr, pos.left(), pos.top());
    
    // Scale image to fit the designated area if needed
    int img_width = cairo_image_surface_get_width(image_surface);
    int img_height = cairo_image_surface_get_height(image_surface);
    
    if (pos.width != img_width || pos.height != img_height) {
        double scale_x = static_cast<double>(pos.width) / img_width;
        double scale_y = static_cast<double>(pos.height) / img_height;
        cairo_scale(cr, scale_x, scale_y);
    }
    
    // Draw the image
    cairo_set_source_surface(cr, image_surface, 0, 0);
    cairo_paint(cr);
    
    cairo_restore(cr);
    cairo_destroy(cr);
    
    std::cout << "Image drawn successfully" << std::endl;
}

void CairoContainer::load_image(const char* src, const char* baseurl, bool redraw_on_ready)
{
    if (!src) return;
    
    std::cout << "load_image called for: " << src << std::endl;
    
    std::string image_path = src;
    
    // If it's a relative path, make it relative to base_path_
    if (image_path.find("://") == std::string::npos && image_path[0] != '/') {
        image_path = (base_path_ / src).string();
    }
    
    std::cout << "Looking for image at: " << image_path << std::endl;
    
    // Check if image is already cached (check both original src and resolved path)
    if (image_cache.find(src) != image_cache.end() || 
        image_cache.find(image_path) != image_cache.end()) {
        std::cout << "Image already cached: " << src << std::endl;
        return;
    }
    
    // Load image using stb_image
    int width, height, channels;
    unsigned char* data = stbi_load(image_path.c_str(), &width, &height, &channels, 4); // Force RGBA
    
    if (!data) {
        std::cout << "Failed to load image: " << image_path << " - " << stbi_failure_reason() << std::endl;
        return;
    }
    
    std::cout << "Loaded image: " << width << "x" << height << " channels: " << channels << std::endl;
    
    // Convert RGBA to BGRA for Cairo (Cairo uses BGRA format)
    for (int i = 0; i < width * height; i++) {
        unsigned char* pixel = &data[i * 4];
        unsigned char temp = pixel[0]; // R
        pixel[0] = pixel[2]; // R = B
        pixel[2] = temp;     // B = R
        // G and A stay the same
    }
    
    // Create Cairo surface from image data
    cairo_surface_t* surface = cairo_image_surface_create_for_data(
        data, CAIRO_FORMAT_ARGB32, width, height, width * 4);
    
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        std::cout << "Failed to create Cairo surface for image" << std::endl;
        stbi_image_free(data);
        return;
    }
    
    // Cache the surface with both keys for lookup flexibility
    image_cache[src] = surface; // Use original src as key for lookups
    if (image_path != src) {
        image_cache[image_path] = surface; // Also cache with resolved path
    }
    
    std::cout << "Image cached successfully: " << src << std::endl;
}

void CairoContainer::get_image_size(const char* src, const char* baseurl, litehtml::size& sz)
{
    if (!src) return;
    
    std::cout << "get_image_size called for: " << src << std::endl;
    
    // Check cache first
    auto it = image_cache.find(src);
    if (it != image_cache.end()) {
        cairo_surface_t* surface = it->second;
        sz.width = cairo_image_surface_get_width(surface);
        sz.height = cairo_image_surface_get_height(surface);
        std::cout << "Image size from cache: " << sz.width << "x" << sz.height << std::endl;
        return;
    }
    
    // Load image to get size
    std::string image_path = src;
    if (image_path.find("://") == std::string::npos && image_path[0] != '/') {
        image_path = (base_path_ / src).string();
    }
    
    int width, height, channels;
    if (stbi_info(image_path.c_str(), &width, &height, &channels)) {
        sz.width = width;
        sz.height = height;
        std::cout << "Image size from file: " << sz.width << "x" << sz.height << std::endl;
    } else {
        std::cout << "Failed to get image info for: " << image_path << std::endl;
        sz.width = sz.height = 0;
    }
}