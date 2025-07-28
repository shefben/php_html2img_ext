#ifndef PHP_HTML2IMG_ARGINFO_H
#define PHP_HTML2IMG_ARGINFO_H

#ifdef ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_html_file_to_image, 0, 3, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, html_path, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, output_path, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, format, IS_STRING, 0)
ZEND_END_ARG_INFO()
#else
#   error "Requires PHP 8 arginfo macros"
#endif

#endif
