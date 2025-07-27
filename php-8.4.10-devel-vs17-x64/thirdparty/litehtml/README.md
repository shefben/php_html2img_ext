# litehtml: Lightweight HTML/CSS Rendering Engine

## What is litehtml?

**litehtml** is a lightweight and fast HTML rendering engine with good support for CSS2 and many CSS3 features. It is written in C++ and designed for ease of integration into various applications.

A key characteristic of **litehtml** is that it does not perform any drawing operations (text, images, graphics) on its own. Instead, it focuses on parsing HTML/CSS, resolving styles, and calculating the layout of HTML elements. To actually render the content, you need to implement a simple callback interface called `litehtml::document_container`. This design makes litehtml highly flexible, allowing you to use any graphics library or platform-specific drawing routines you prefer.

## Who is litehtml for?

**litehtml** is primarily for C++ developers who need to:

* Display HTML-formatted text or UI elements within their applications.
* Create simple custom browsers or HTML viewers.
* Add HTML rendering capabilities without the overhead and complexity of embedding a full web browser engine like WebKit or Blink.

If your needs are to display rich internet applications or cutting-edge HTML5 features, a full browser engine might be more suitable. However, for tasks like HTML tooltips, formatted log displays, simple documentation viewers, or UI elements styled with HTML/CSS, litehtml offers a much more lightweight and manageable solution.

## Why is litehtml useful?

*   **Lightweight:** It has a small footprint in terms of code size and resource consumption compared to full browser engines.
*   **Portable:** Written in standard C++, litehtml can be compiled on any platform with a C++17 compliant compiler. It's designed to be platform-agnostic.
*   **Flexible Drawing:** By requiring the user to implement the `document_container` interface, litehtml allows complete control over how content is drawn, using any graphics library (e.g., Cairo, Skia, GDI+) or native OS drawing APIs.
*   **Ease of Integration:** Integrating litehtml into an existing C++ project is generally straightforward.
*   **UTF-8 Support:** Handles UTF-8 encoded HTML and CSS.

## HTML Parser

**litehtml** uses the [gumbo-parser](https://github.com/google/gumbo-parser) (an actively maintained fork is at [https://codeberg.org/gumbo-parser/gumbo-parser](https://codeberg.org/gumbo-parser/gumbo-parser)) to parse HTML. Gumbo is an implementation of the HTML5 parsing algorithm written in pure C99, designed to be a building block for other tools.

## Compatibility and Standards Support

*   **C++ Standard:** Requires C++17.
*   **Platforms:** Compatible with any platform supporting C++17 and the STL.
*   **HTML/CSS:** litehtml aims to support common HTML tags and CSS2/CSS3 properties. While not a fully standards-compliant browser engine (and that's not its goal), it supports a significant subset of features sufficient for many use cases, including rendering pages built with frameworks like Bootstrap. A list of supported CSS properties can be found [here](https://docs.google.com/spreadsheet/ccc?key=0AvHXl5n24PuhdHdELUdhaUl4OGlncXhDcDJuM1JpMnc&usp=sharing) (though this list might not be exhaustive or fully up-to-date).

## Installation (Integrating the Library)

litehtml is a library, not a standalone application. To "install" it, you integrate it into your C++ project. The typical way to build litehtml is using CMake:

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/litehtml/litehtml.git
    cd litehtml
    ```
2.  **Build with CMake:**
    ```bash
    mkdir build && cd build
    cmake ..
    make # or your chosen build system's command, e.g., msbuild on Windows
    ```
    This will build the litehtml static or shared library (depending on CMake configuration).
3.  **Link with your project:**
    *   Include the `litehtml/include` directory in your project's include paths.
    *   Link against the compiled litehtml library.
    *   If you are not using the bundled Gumbo parser (i.e., `EXTERNAL_GUMBO` is ON), ensure you also link against Gumbo.

The `CMakeLists.txt` file provides options like `LITEHTML_BUILD_TESTING` (OFF by default) and `EXTERNAL_GUMBO` (OFF by default, meaning it uses its bundled Gumbo).

## How to Use litehtml Programmatically

Using litehtml in your C++ application involves these main steps:

1.  **Implement `litehtml::document_container`:** This is the most crucial step. You must create a C++ class that inherits from `litehtml::document_container` and implements its pure virtual functions. These functions are callbacks that litehtml will invoke to:
    *   Create and manage fonts (`create_font`, `delete_font`, `text_width`, `draw_text`).
    *   Draw backgrounds, borders, and list markers (`draw_solid_fill`, `draw_image`, `draw_borders`, etc.).
    *   Load images (`load_image`, `get_image_size`).
    *   Handle links and cursors (`on_anchor_click`, `set_cursor`).
    *   Provide information about the rendering environment (viewport size, default font, media features).
    *   For a detailed guide on this interface, see [doc/document_container.md](doc/document_container.md).

2.  **Create a `litehtml::document` instance:** Once you have your custom `document_container`, you can parse an HTML string using `litehtml::document::createFromString()`:
    ```cpp
    #include "litehtml.h"
    #include "your_document_container_implementation.h" // Your custom container

    // ...

    your_document_container_implementation my_container;
    std::string html_text = "<html><body>Hello, <b>litehtml</b>!</body></html>";
    litehtml::document::ptr doc = litehtml::document::createFromString(
        html_text.c_str(), // Note: litehtml uses UTF-8 strings
        &my_container,
        litehtml::master_css, // Default master stylesheet
        ""                    // Optional user stylesheet
    );
    ```
    Refer to [doc/document_createFromString.md](doc/document_createFromString.md) for details on encoding detection and options.

3.  **Render the document:** Call `document::render()` to calculate the layout of the HTML elements. This function takes the maximum width available for rendering and returns the optimal width.
    ```cpp
    int max_width = 800; // e.g., width of your window or drawing area
    int optimal_width = doc->render(max_width);
    // Optionally, re-render if optimal_width is significantly different and you want to fit content
    // if (optimal_width < max_width) {
    //     doc->render(optimal_width);
    // }
    ```

4.  **Draw the document:** Call `document::draw()` to actually paint the content. You provide a drawing context handle (`uint_ptr hdc`), coordinates, and a clipping rectangle. This will trigger the callbacks in your `document_container`.
    ```cpp
    // hdc would be something like a Cairo context, Windows HDC, etc.
    // This is specific to your application and drawing backend.
    uint_ptr your_drawing_context = get_your_drawing_context(); // Example function
    litehtml::position clip_rect(0, 0, doc->width(), doc->height());
    doc->draw(your_drawing_context, 0, 0, &clip_rect);
    ```

5.  **Handle events (optional):** litehtml provides functions like `document::on_mouse_over()`, `document::on_lbutton_down()`, etc., to forward mouse events to the HTML document, enabling hover effects and link clicks.

For more detailed usage examples and explanations, please refer to:
*   [doc/using.md](doc/using.md): General guide on using litehtml.
*   The various container implementations in the `containers/` directory (e.g., `containers/cairo`, `containers/windows`) provide practical examples of `document_container` implementations.

## Testing litehtml

You can test the litehtml rendering engine using `litebrowser`, a simple browser application built with litehtml. Source codes for different platforms are available:
*   [For Windows](https://github.com/litehtml/litebrowser)
*   [For Linux](https://github.com/litehtml/litebrowser-linux)
*   [For Haiku](https://github.com/adamfowleruk/litebrowser-haiku)

## License

**litehtml** is distributed under the [New BSD License (3-Clause)](https://opensource.org/licenses/BSD-3-Clause).
The bundled **gumbo-parser** is distributed under the [Apache License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0).

## Links

*   **Source Code:** [https://github.com/litehtml/litehtml](https://github.com/litehtml/litehtml)
*   **Official Website:** [http://www.litehtml.com/](http://www.litehtml.com/) (may contain older information)

---

## Technical Deep Dive

This section provides a more detailed look into how litehtml works and guidelines for contributing.

### How litehtml Works

#### 1. HTML Parsing
*   **Gumbo Parser:** litehtml delegates HTML parsing to the Gumbo parser. Gumbo is a robust HTML5 parsing library that produces a parse tree (DOM tree) from an HTML string. litehtml then traverses this Gumbo tree to build its own internal representation of the document.
*   **DOM Representation:** litehtml creates its own tree of `litehtml::element` objects. Each `element` corresponds to an HTML tag and stores its attributes. Specialized element classes exist for certain tags (e.g., `el_image`, `el_table`, `el_style`).

#### 2. CSS Parsing and Styling
*   **CSS Tokenizer & Parser:** litehtml includes its own CSS tokenizer (`css_tokenizer`) and parser (`css_parser`) to process stylesheets. It handles inline styles (in `style` attributes), styles from `<style>` tags, and external stylesheets (loaded via the `document_container::import_css` callback).
*   **Master Stylesheet:** A default "master" stylesheet (`master_css.h`) provides base styling for HTML elements, mimicking common browser defaults. This can be overridden or supplemented.
*   **Cascading and Specificity:** CSS rules are applied based on standard cascading rules (origin and importance, specificity of selectors, order of appearance). Selectors are parsed and matched against the element tree. Supported selectors include type, class, ID, attribute selectors, and some pseudo-classes.
*   **Style Resolution:** For each element, litehtml computes its final style properties (e.g., color, font-size, margin, padding, display type) by resolving the applied CSS rules. These computed values are stored within the `litehtml::element` or associated style objects.

#### 3. Rendering Process
*   **Render Tree Construction:** Based on the display type (block, inline, flex, etc.) and other CSS properties, a tree of render objects (e.g., `render_block`, `render_inline`, `render_flex`) is implicitly formed. These objects are responsible for layout.
*   **Layout Engine:**
    *   **Block Layout:** Elements with `display: block` (like `<div>`, `<p>`) are laid out vertically, respecting margins, paddings, and widths.
    *   **Inline Layout:** Inline elements (like `<span>`, `<a>`, text nodes) are arranged into line boxes within their containing block. Word wrapping and text alignment are handled here.
    *   **Flex Layout:** Elements with `display: flex` use a flexbox layout model to arrange their children.
    *   **Table Layout:** HTML tables are laid out according to table formatting rules.
    *   **Positioning:** Absolute, relative, and fixed positioning are supported.
*   **`document::render(max_width)`:** This core function initiates the layout process. It performs a recursive traversal of the element tree, calculating the size and position of each element based on its style and content, constrained by the `max_width`. It returns the "optimal" width the content would ideally occupy.
*   **`document::draw(hdc, x, y, clip)`:** After `render()` has determined the layout, `draw()` traverses the elements again. For each visible element, it invokes the appropriate methods on the `document_container` implementation (provided by the user) to perform the actual drawing operations (e.g., `draw_text`, `draw_background`, `draw_borders`). The `hdc` is an abstract handle to the drawing surface/context, `x` and `y` define the viewport offset (for scrolling), and `clip` defines the clipping rectangle.

#### 4. Core Classes
*   **`litehtml::document_container`:** (User-implemented) The bridge between litehtml and the host application's drawing and environment specifics. It's an abstract class whose methods *must* be implemented by the user to handle font creation, text drawing, image loading/drawing, and other platform-dependent tasks.
*   **`litehtml::document`:** Represents the entire HTML document. It holds the root of the element tree, manages stylesheets, and provides the main API for parsing (`createFromString`), layout (`render`), and drawing (`draw`).
*   **`litehtml::element`:** The base class for all HTML elements in the document tree. It stores attributes, computed style information, and parent/child relationships. Various derived classes (e.g., `litehtml::el_image`, `litehtml::el_table`, `litehtml::el_text`) implement behavior specific to certain HTML tags.
*   **`litehtml::render_item` and its derivatives:** These classes (like `render_block`, `render_inline`, `render_flex`, `render_table`) are responsible for the layout and rendering logic of different types of elements. They are typically owned by `element` objects.

#### 5. Platform Abstraction (Containers)
The `containers/` directory showcases examples of `document_container` implementations for different platforms and graphics libraries:
*   `containers/cairo`: Uses the Cairo graphics library for drawing.
*   `containers/windows`: Contains implementations for Windows using GDI+ and direct Win32 API for drawing.
*   `containers/test`: A simple container used for internal testing, often rendering to a bitmap.
These serve as excellent starting points for integrating litehtml into new applications.

### Coding and Contribution Guidelines

#### Coding Style
*   **ClangFormat:** The project uses `.clang-format` to define and enforce coding style. The style is based on `Microsoft` with specific overrides (e.g., C++17 standard, tabs for indentation).
*   **Before committing, please ensure your code is formatted according to this style.** Many IDEs can be configured to use `clang-format` automatically. You can also run `clang-format -i <source_file>` to format a file in place.
*   **General Principles:**
    *   Write clear, maintainable C++.
    *   Use descriptive variable and function names.
    *   Comment complex or non-obvious code sections.
    *   Prefer modern C++ features (C++17) where appropriate.

#### Contribution Process
1.  **Fork the Repository:** Create your own fork of `litehtml/litehtml` on GitHub.
2.  **Create a Branch:** For new features or bug fixes, create a new branch in your fork, typically from the `master` branch.
    ```bash
    git checkout -b my-new-feature
    ```
3.  **Make Changes:** Implement your feature or fix the bug.
    *   Adhere to the coding style (use `clang-format`).
    *   Write or update tests if applicable (note: main tests are in a separate repository: `litehtml/litehtml-tests`). For significant changes, consider how they might be tested.
4.  **Commit Changes:** Make clear, atomic commits.
    ```bash
    git add .
    git commit -m "feat: Implement X feature"
    ```
    Or for a bug fix:
    ```bash
    git commit -m "fix: Correct Y issue in Z component"
    ```
5.  **Push to Your Fork:**
    ```bash
    git push origin my-new-feature
    ```
6.  **Create a Pull Request (PR):** Open a PR from your branch in your fork to the `master` branch of `litehtml/litehtml`.
    *   Provide a clear description of the changes in the PR.
    *   Reference any relevant issues.
7.  **Code Review:** Project maintainers will review your PR. Address any feedback or requested changes.
8.  **Merge:** Once approved, your PR will be merged.

#### Testing
*   The primary test suite for litehtml resides in a separate repository: [litehtml/litehtml-tests](https://github.com/litehtml/litehtml-tests).
*   When building litehtml with the `LITEHTML_BUILD_TESTING` CMake option enabled (`ON`), it will attempt to download and build these tests.
*   If you are making changes that could affect rendering or parsing, it's highly recommended to run these tests. You might need to clone the `litehtml-tests` repository separately and configure it to use your local litehtml build.

#### Reporting Bugs
*   Use the GitHub Issues tracker for the `litehtml/litehtml` repository.
*   Provide a clear and concise description of the bug.
*   Include steps to reproduce the issue, including a minimal HTML/CSS example if possible.
*   Mention your platform and how you are using litehtml (e.g., which container or if it's a custom implementation).
