# ────────────────────────────────────────────────────────────────────────────────

# AGENTS.md – HTML➜Image PHP 8.2 Extension Builder

# ────────────────────────────────────────────────────────────────────────────────

agents:

- name: html2img-builder
- persona: A meticulous C/C++ build‑engineer and PHP internals developer.
  - Think in small, verifiable steps.
  - Prioritise portable, dependency‑light solutions.
  - Output compilable code, build scripts, and README snippets – no theory only.
- goals:
  - Produce a PHP 8.2 extension named **html2img** that exposes `string html_to_image(string $html, string $format = 'png')`
- where:
  - `png` and `gif` preserve full ARGB transparency
  - jpg` returns an opaque image (background auto‑filled)
  - the cache key incorporates the chosen `$format`
  - Behaviour of `html_to_image`:
    1. Hash the input HTML + CSS + format to create a cache key.
    2. If `<cache>/<key>.<format>` exists, return its path.
    3. Otherwise:
       - Parse/layout the HTML/CSS (HTML 4.01 + CSS 2.1 subset).
       - Render to a 32‑bit ARGB bitmap via GD + FreeType.
       - Save as the requested format and return the path.
  - Zero headless‑browser usage; rely on litehtml (or equivalent tiny engine).
  - Cross‑compile for **Linux‑x64 (GCC/Clang)** and **Windows‑x64 (MinGW‑w64 or MSVC)**.
  - Deliver test PHP scripts that recreate the sample banners/buttons supplied by the user.
- constraints:
  - C++17 only (for `std::filesystem`, etc.).
  - Static‑link third‑party libs where feasible to simplify Windows builds.
  - Total external source footprint < 3 MiB (gzip).
  - Produce both **config.m4** (Unix) and **config.w32** (Windows).
- tools:
  - git (for fetching litehtml)
  - cmake + make / ninja
  - phpize / buildconf
  - mingw‑w64 cross toolchain
  - msbuild / nmake (optional)
  - bash & PowerShell
- success_criteria:
  - `php -dextension=html2img.so -r 'echo html_to_image($html);'` returns a valid image path.
  - Running the demo HTML/PHP in `./demos/` through the extension re‑creates the exact imagery.
  - `make test` passes on both Linux and Windows GitHub Actions runners.

# ────────────────────────────────────────────────────────────────────────────────

# Phased Prompts

# ────────────────────────────────────────────────────────────────────────────────

- prompts:
    
### id: 01_requirements_analysis
      - content: **Phase 1 – Requirements decomposition & dependency decision**
        1. Parse the _Goals_ and _Constraints_ above.
        2. Confirm `litehtml` + system GD satisfy them; fall back to Netsurf’s `libcss`/`libdom` only if litehtml fails.
        3. Emit a decision matrix (engine, compile flags, binary size).
        4. List exact versions/commit SHAs to pin.
        5. Sketch directory layout (`ext/html2img/{src,3rdparty,tests}`).
    
### id: 02_scaffold_repository
        - content: **Phase 2 – Repository scaffold**
        1. Generate `bootstrap.sh` that:  
           • Clones litehtml at the pinned commit.  
           • Initialises submodules.  
           • Patches `CMakeLists.txt` to `add_library(litehtml STATIC …)`.
        2. Create:  
           • `config.m4` (Unix)  
           • `config.w32` (Windows)  
           • Top‑level `CMakeLists.txt` detecting PHP includes/libs, GD, FreeType.
        3. Stub `php_html2img.cpp` exposing MINIT, MINFO, `PHP_FUNCTION(html_to_image)`.
        4. Provide example commit messages.
    
### id: 03_gd_render_backend
        - content: **Phase 3 – Implement GD render backend**
          1. In `src/`, add `gd_container.hpp/.cpp` implementing `litehtml::document_container`.
          2. Map litehtml ops to GD:  
             • `draw_text` → `gdImageStringFT()` with RGBA.  
             • `draw_rect`, `draw_borders`, `draw_bg` similarly.
          3. Provide `gd_canvas` helper:  
             • 32‑bit ARGB surface.  
             • `export(format)` switches:  
              – `gdImagePng()` for `'png'`  
              – `gdImageGif()` for `'gif'` (set transparency)  
              – `gdImageJpeg()` for `'jpg'` (quality 90)  
              Preserve alpha for PNG/GIF; flatten to white for JPEG.
          4. Unit test: render `<div style="background:#000;width:10px;height:10px"></div>` and assert PNG = 10×10.

### id: 04_cache_hash_and_io
    - content: **Phase 4 – Cache & I/O layer**
      1. Implement `cache_key(html, format)` = SHA‑1 of `(html + format + version_tag)`.
      2. Use `std::filesystem` to create `$TMPDIR/php-html2img-cache/`.
      3. Cross‑OS file locking to avoid stampede.
      4. INI entries:  
         • `html2img.cache_dir` (path)  
         • `html2img.ttl` (seconds; 0 = infinite)  
         • `html2img.default_format` (`png|gif|jpg`, default `png`)
      5. Regression test: two parallel calls with identical HTML/format yield identical bytes; one render only.

### id: 05_configure_and_build_scripts
    - content: **Phase 5 – Build scripts & cross‑compile**
      1. Fill in `config.m4`: call `PHP_REQUIRE_CXX`, add litehtml sources, link `-lgd -lfreetype`.
      2. Fill in `config.w32` using `ADD_SOURCES`, `ADD_INCLUDE`, `ADD_FLAG`, `PHP_ZEND_ADD_INCLUDE`.
      3. Write `build-linux.sh` and `build-windows-mingw.sh`:  
         • `phpize && ./configure --enable-html2img && make -j$(nproc)` (Linux).  
         • Cross‑compile via mingw (`x86_64-w64-mingw32-g++`) into `html2img.dll`.
      4. Document env vars (`PHP_PREFIX`, `GD_LIB`, `FREETYPE_LIB`) for CI.

### id: 06_validation_demos
    - content: **Phase 6 – Validation & demos**
      1. Port user sample to `tests/demo.php`, using `html_to_image()`.
      2. PHPUnit tests compare PNG/GIF/JPG MD5s to golden samples.
      3. README updates: installation, usage, limitations.
      4. CI: GitHub Actions matrix (`ubuntu‑latest`, `windows‑latest`) uploading built modules.

### id: 07_release_packaging
    - content: **Phase 7 – Release packaging**
      1. Auto‑generate `package.xml` for PECL.
      2. Tag `v0.1.0`, add changelog.
      3. Draft GitHub release with binaries (`html2img.so`, `php_html2img.dll`).
      4. Size report: total .so/.dll < 1.5 MiB compressed.

# ────────────────────────────────────────────────────────────────────────────────

# End of AGENTS.md

# ────────────────────────────────────────────────────────────────────────────────
