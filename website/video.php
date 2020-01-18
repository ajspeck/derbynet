<?php
session_start(); 

require_once('inc/video-config.inc');

function parse_video_url($url_path_info) {
  $exploded = explode('/', $url_path_info);
  if (count($exploded) >= 2) {
    return array('basename' => urldecode($exploded[1]));
  } else {
    return false;
  }
}

if (isset($_SERVER['PATH_INFO'])) {
  $path_info = $_SERVER['PATH_INFO'];
} else if (isset($_SERVER['REQUEST_URI']) && isset($_SERVER['SCRIPT_NAME']) &&
           substr($_SERVER['REQUEST_URI'], 0, strlen($_SERVER['SCRIPT_NAME'])) == $_SERVER['SCRIPT_NAME']) {
  $path_info = substr($_SERVER['REQUEST_URI'], strlen($_SERVER['SCRIPT_NAME']));
} else if (isset($_SERVER['ORIG_PATH_INFO'])) {
  // Rewrite rules in Apache 2.2 may leave ORIG_PATH_INFO instead of PATH_INFO
  $path_info = 'video.php'.$_SERVER['ORIG_PATH_INFO'];
} else {
  // Debugging only:
  var_export($_SERVER);
  exit(0);
}

$parsed = parse_video_url($path_info);
if (!$parsed) {  // Malformed URL
  http_response_code(404);
  echo "Malformed URL\n";
  var_dump($path_info);
  exit(1);
}

$file_path = video_file_path($parsed['basename']);

if (!$file_path) {  // No such racer/no such photo
  http_response_code(404);
  echo "No file path\n";
  var_dump($parsed);
  exit(1);
}

header('Pragma: public');
header('Cache-Control: max-age=86400, public');
header('Expires: '.gmdate('D, d M Y H:i:s', time() + 86400).' GMT');

header('Content-type: '.pseudo_mime_content_type($file_path));

readfile($file_path);
?>