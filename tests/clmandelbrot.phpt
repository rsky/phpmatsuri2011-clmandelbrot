--TEST--
clmandelbrot() function
--FILE--
<?php
$im = clmandelbrot(100, 100);
if (!is_resource($im)) {
    echo 'didn\'t return resource';
} else {
    printf('%dx%d', imagesx($im), imagesy($im));
}
?>
--EXPECT--
100x100
