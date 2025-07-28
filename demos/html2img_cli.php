<?php
// CLI entry for html_file_to_image()
if ($argc < 4) {
    fwrite(STDERR, "Usage: php html2img_cli.php <input.html> <output> <format>\n");
    exit(1);
}
$ext = getenv('HTML2IMG_EXT') ?: __DIR__.'/../html2img.so';
if (!extension_loaded('html2img')) dl($ext);
$input  = $argv[1];
$output = $argv[2];
$fmt    = $argv[3];
$data = html_file_to_image($input, $output, $fmt);
file_put_contents($output, $data);
?>
