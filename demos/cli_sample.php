<?php
// Simple command line demo for html_css_to_image()
// This mirrors the example provided in the documentation

$ext = getenv('HTML2IMG_EXT') ?: __DIR__.'/../html2img.so';
if (!extension_loaded('html2img')) dl($ext);

$dir = __DIR__;
$dinMid = realpath("$dir/DINMittelschriftStd.otf");
$dinEng = realpath("$dir/DINEngschriftStd.otf");
$arrow  = realpath("$dir/title_arrow.gif");
foreach (['DINMittelschriftStd.otf' => $dinMid,
          'DINEngschriftStd.otf'   => $dinEng,
          'title_arrow.gif'        => $arrow] as $name => $p) {
    if ($p === false) die("Missing $name\n");
}
$uriMid = 'file:///' . str_replace('\\', '/', $dinMid);
$uriEng = 'file:///' . str_replace('\\', '/', $dinEng);
$uriArr = 'file:///' . str_replace('\\', '/', $arrow);

$html = <<<HTML
<div style="background:#7B7F8D;width:332px;height:95px;color:#FFFFFF;
            font-family:Verdana,Arial,sans-serif;text-align:center;
            position:relative;margin:4px auto;">
    <span style="position:absolute;top:12px;left:0;width:100%;margin:0;
                 padding:0;line-height:1;letter-spacing:.25px;
                 font:11px Verdana,Arial,sans-serif;">
        Game Player License Agreement
    </span>
    <span style="position:absolute;top:29px;left:0;width:100%;margin:0;
                 padding:0;line-height:1;letter-spacing:.25px;
                 font:18px Verdana,Arial,sans-serif;">
        VALVE, L.L.C.
    </span>
    <span style="position:absolute;top:56px;left:0;width:100%;margin:0;
                 padding:0;line-height:1;letter-spacing:.25px;
                 font:18px Verdana,Arial,sans-serif;">
        STEAM GAME PLAYER AGREEMENT
    </span>
</div>

<style>
@font-face{font-family:"DINMittelschriftStd";src:url("{$uriMid}");}
</style>
<div style="width:531px;height:36px;background:#626d5c;
            font:16px/36px 'DINMittelschriftStd',sans-serif;color:#f0f0f0;
            letter-spacing:0;padding-left:7px;position:relative;
            white-space:nowrap;overflow:hidden;">
    <span>GET ANSWERS </span>
    <span style="color:#a6aca1;">TO YOUR QUESTIONS</span>
    <div style="position:absolute;left:7px;bottom:4px;width:34px;height:4px;
                background:#000;"></div>
</div>

<style>
@font-face{font-family:"DINEngschriftStd";src:url("{$uriEng}");}
</style>
<div style="width:531px;height:38px;
            font:20px/36px 'DINEngschriftStd';
            letter-spacing:1px;padding-left:7px;position:relative;
            color:#f0f0f0;white-space:nowrap;overflow:hidden;">
    <span style="display:inline-flex;align-items:center;gap:5px;">
        <img src="{$uriArr}" alt=">">
        Support
    </span>
</div>
HTML;

$img = html_css_to_image($html, 'png');
copy($img, __DIR__.'/banner.png');
echo "Saved to banner.png\n";
?>
