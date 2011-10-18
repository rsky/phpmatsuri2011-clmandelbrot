--TEST--
cl_get_devices() function
--FILE--
<?php
$devives = cl_get_devices();
if (is_array($devives) && count($devives) > 0) {
    echo 'OK';
} else {
    echo 'NG';
}
?>
--EXPECT--
OK
