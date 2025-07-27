<?php
$ext = getenv('HTML2IMG_EXT') ?: __DIR__.'/../html2img.so';
if (!extension_loaded('html2img')) dl($ext);
$fontPath = 'php-8.2.29-src/ext/gd/tests/Rochester-Regular.otf';
$html = "<style>@font-face{font-family:'Custom';src:url('{$fontPath}');}</style><div style='font:20px Custom;'>A</div>";
$path = html_css_to_image($html, 'png');
$md5 = md5_file($path);
file_put_contents(__DIR__.'/relative_font.md5', $md5);
echo "$md5\n";
?>
