<?php
if ($argc < 4) {
    echo "Usage: php html2img_cli.php <input.html> <output.file> <format>\n";
    echo "Format must be png, jpg, or gif.\n";
    exit(1);
}

$ext = getenv('HTML2IMG_EXT') ?: __DIR__.'/../html2img.so';
if (!extension_loaded('html2img')) {
    dl($ext);
}

list(, $input, $output, $format) = $argv;
$binary = html_file_to_image($input, $output, $format);
file_put_contents($output, $binary);
printf("Saved to %s\n", $output);

