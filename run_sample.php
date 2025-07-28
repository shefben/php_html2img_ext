<?php
$data = html_file_to_image('demos/output_html_test.html', 'out.png', 'png');
file_put_contents('out.png', $data);
print "Image saved to out.png\n";

