<?php
$devices = cl_get_devices();
foreach ($devices as $id => $info) {
    $start = microtime(true);
    for ($i = 0; $i < 100; $i++) {
        $im = clmandelbrot(1024, 1024, 1/512, $id);
    }
    $end = microtime(true);
    printf("device #%d (%s): %0.6fsec.\n", $id, $info['name'], $end - $start);
    imagepng($im, "clmandelbrot{$id}.png");
}
