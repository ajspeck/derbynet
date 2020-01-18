<?php
require_once('inc/data.inc');


function video_directory() {
  return read_raceinfo('video-directory');
}


// Constructs a file URL for the photo whose base name is $videoname.
function render_video_url($videofile) {
    if ($videofile != '') {
        $videofile = basename($videofile);
        $breaker = @filemtime(video_file_path($videofile));
        // If best_available_file_path can't find any suitable version of the file
        // at all, $breaker will be FALSE from filemtime; use a 0 instead.
        if (!$breaker) {
        $breaker = 0;
        }
        return 'video.php/'.urlencode($videofile).'/'.$breaker;
    } else {
        return '';
    }
}

function video_file_path($videoname) {
    $directory = video_directory();
    if (substr($directory, -1) != DIRECTORY_SEPARATOR) {
        $directory .= DIRECTORY_SEPARATOR;
    }
    return $directory.$videoname;
}


function pseudo_mime_content_type($filename) {
  $mime_types = array(
            'txt' => 'text/plain',
            'htm' => 'text/html',
            'html' => 'text/html',
            'php' => 'text/html',
            'css' => 'text/css',
            'js' => 'application/javascript',
            'json' => 'application/json',
            'xml' => 'application/xml',
            'swf' => 'application/x-shockwave-flash',
            'flv' => 'video/x-flv',

            // images
            'png' => 'image/png',
            'jpe' => 'image/jpeg',
            'jpeg' => 'image/jpeg',
            'jpg' => 'image/jpeg',
            'gif' => 'image/gif',
            'bmp' => 'image/bmp',
            'ico' => 'image/vnd.microsoft.icon',
            'tiff' => 'image/tiff',
            'tif' => 'image/tiff',
            'svg' => 'image/svg+xml',
            'svgz' => 'image/svg+xml',

            // archives
            'zip' => 'application/zip',
            'rar' => 'application/x-rar-compressed',
            'exe' => 'application/x-msdownload',
            'msi' => 'application/x-msdownload',
            'cab' => 'application/vnd.ms-cab-compressed',

            // audio/video
            'mp3' => 'audio/mpeg',
            'qt' => 'video/quicktime',
            'mov' => 'video/quicktime',
            'mp4' => 'video/mp4',
            'mkv' => 'video/x-matroska',

            // adobe
            'pdf' => 'application/pdf',
            'psd' => 'image/vnd.adobe.photoshop',
            'ai' => 'application/postscript',
            'eps' => 'application/postscript',
            'ps' => 'application/postscript',

            // ms office
            'doc' => 'application/msword',
            'rtf' => 'application/rtf',
            'xls' => 'application/vnd.ms-excel',
            'ppt' => 'application/vnd.ms-powerpoint',

            // open office
            'odt' => 'application/vnd.oasis.opendocument.text',
            'ods' => 'application/vnd.oasis.opendocument.spreadsheet',
        );

  $filename_exploded = explode('.', $filename);
  $ext = strtolower(array_pop($filename_exploded));
  if (array_key_exists($ext, $mime_types)) {
	return $mime_types[$ext];
  }
  elseif (function_exists('finfo_open')) {
	$finfo = finfo_open(FILEINFO_MIME);
	$mimetype = finfo_file($finfo, $filename);
	finfo_close($finfo);
	return $mimetype;
  }
  else {
	return 'application/octet-stream';
  }
}


?>
