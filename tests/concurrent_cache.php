<?php
$ext = getenv('HTML2IMG_EXT') ?: __DIR__.'/../html2img.so';
if (!extension_loaded('html2img')) {
    dl($ext);
}
$html = '<div style="width:4px;height:4px;background:#000"></div>';
$format = 'png';
$paths = [];
$func = function() use ($html, $format, &$paths) {
    $paths[] = html_css_to_image($html, $format);
};
$pid = pcntl_fork();
if ($pid == 0) {
    $func();
    exit(0);
} else {
    $func();
    pcntl_wait($status);
}
if ($paths[0] !== $paths[1]) { echo "FAIL path mismatch\n"; exit(1); }
$mtime1 = filemtime($paths[0]);
sleep(1);
$mtime2 = filemtime($paths[1]);
if ($mtime1 != $mtime2) { echo "FAIL multiple renders\n"; exit(1); }
echo "OK\n";
?>
