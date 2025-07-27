# html2img PHP Extension

## Running the supplied sample

After building the extension, you can render `demos/output_html_test.html` using a simple PHP script:

```php
<?php
$html = file_get_contents(__DIR__ . '/demos/output_html_test.html');
$path = html_css_to_image($html, 'png');
echo "Image saved to: $path\n";
```

Ensure the extension is loaded:

```bash
php -dextension=html2img.so run_sample.php
```

## Building

Use `bootstrap.sh` to fetch litehtml at the pinned commit and compile the extension:

```bash
./bootstrap.sh
```

The script applies `patches/litehtml-static.patch` so litehtml builds as a static library.
For Windows cross-builds run `build-windows-mingw.sh` or build with Visual Studio using `build-windows-vs17.bat`.
You can also open `html2img.sln` in Visual Studio 2022 and build the `Release|x64` configuration.
