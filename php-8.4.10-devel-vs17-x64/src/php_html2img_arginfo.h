/* ---------------------------------------------------------------------------
 *  php_html2img_arginfo.h  –  generated for PHP 8.4 / html2img extension
 * ---------------------------------------------------------------------------
 *
 *  Function:   string html_css_to_image(string $html, ?string $format = 'png');
 *  Notes:
 *    • Returns an absolute file path (string) to the cached / newly‑rendered
 *      image.  You can tighten the return type later if you change it.
 *    • `$format` is optional; default handled inside C code.
 * ------------------------------------------------------------------------- */

#ifndef PHP_HTML2IMG_ARGINFO_H
#define PHP_HTML2IMG_ARGINFO_H

#ifdef ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX   /* PHP 8.0+ */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_html_css_to_image, /*pass_rest_by_ref*/0,
                                        /*required_num_args*/1,    /*return_type*/IS_STRING,
                                        /*return_nullable*/0)
    /* string $html   (required) */
    ZEND_ARG_TYPE_INFO(0 /*by‑val*/, html, IS_STRING, 0 /*not nullable*/)

    /* ?string $format = 'png'   (optional) */
    ZEND_ARG_TYPE_INFO(0 /*by‑val*/, format, IS_STRING, 0 /*not nullable*/)
ZEND_END_ARG_INFO()

#else   /* fall back for very old headers – unlikely for PHP 8.4 */
#   error "This arginfo header requires PHP 8’s arginfo macros."
#endif

#endif /* PHP_HTML2IMG_ARGINFO_H */
