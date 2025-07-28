# html2img PHP Extension

## Running the supplied sample

After building the extension, you can render `demos/output_html_test.html` using a simple PHP script:

```php
<?php
$data = html_file_to_image('demos/output_html_test.html', 'out.png', 'png');
file_put_contents('out.png', $data);
echo "Image saved to out.png\n";
```

Ensure the extension is loaded and run the sample script:

```bash
php -dextension=html2img.so run_sample.php
```

You can also convert files directly from the command line using `html2img_cli.php`:

```bash
php -dextension=html2img.so demos/html2img_cli.php input.html output.png png
```

## Building

Use `bootstrap.sh` to fetch RmlUi at the pinned commit and compile the extension:

```bash
./bootstrap.sh
```

The script applies `patches/rmlui-static.patch` so RmlUi builds as a static library.
For Windows cross-builds run `build-windows-mingw.sh` or build with Visual Studio using `build-windows-vs17.bat`.
You can also open `html2img.sln` in Visual Studio 2022 and build the `Release|x64` configuration.


## Asset path resolution

Relative URLs for fonts and images are resolved relative to the PHP script calling
`html_file_to_image()`. The helper in `src/path_utils.*` normalises `file://` URIs
into regular filesystem paths and then joins them with the directory of the
executing script. This ensures the same HTML works from the CLI and from Apache.

If fonts still fail to render, confirm that `html2img.font_path` points to a
directory containing TrueType or OpenType fonts. The extension searches this
directory in addition to any absolute `@font-face` URLs embedded in the HTML.
