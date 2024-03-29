This file contains documentation for editing and maintaining the Audacity web
site.  Audacity is free software for sound editing and recording.  The web site
is at <http://audacity.sourceforge.net/>.

CONTENTS:

  1. Installation
  2. License and Copyright
  3. Localization
  4. Updating Content

---------

1. INSTALLATION

   The Audacity web site requires PHP 4, and the PHP PEAR "HTTP" package
   (for content negotiation).

   "MultiViews" are required, so that "copyright.php" can be accessed as
   simply "copyright", for example.  On Apache servers, MultiViews will be
   turned on by the .htaccess file.  (This requires "AllowOverride Options" to
   be enabled in the server configuration.)

   Apache's mod_rewrite is required for automatic redirection of URLs from
   previous versions of the site.

---------

2. LICENSE AND COPYRIGHT

  Except where otherwise noted, all material on this site is licensed under the
  Creative Commons Attribution License, version 2.0.  You can find a copy of
  this license in the file LICENSE.txt, or at:

    http://creativecommons.org/licenses/by/2.0/

  If you contribute original material to this web site, please add your name to
  the comments at the start of each file you add or modify.  For example:

    /*
     * Copyright 2003 Dominic Mazzoni
     * Copyright 2004 Matt Brubeck
     * This file is licensed under a Creative Commons license:
     * http://creativecommons.org/licenses/by/2.0/
     */

   If you contribute material that you can't (or don't want to) place under
   this Creative Commons license, please make sure it is clearly marked with
   copyright or license notices visible to readers of the web site.

   Our use of Google Free web search is governed by Google's Terms of Use:

     http://www.google.com/services/terms_free.html

---------

3. LOCALIZATION

   Internationalization and localization are handled by PHP's gettext
   functions.  All PHP files use the UTF-8 encoding.  When editing these files,
   please configure your editor to use UTF-8.

   Localized pages are served automatically based on the Accept-Language header
   sent by the user's web browser.  Localized versions can also be accessed
   through links in the page footers.  (This method also sets a cookie so that
   future pages will be served in the same language.)

   To add a new translation:

   - Copy the translated po file to locale/X.po where "X" is the locale code.
   - Run "make" to compile the PO file.
   - Add a new item to the available_locales array in include/lang.inc.php.

   Note:  The "full locale name" must match one of the available locales on the
   web server.  Run "localedef --list-archive" for a list of locales.  On a
   Debian system, use /etc/locale.gen and "locale-gen" to add locales.

---------

4. UPDATING CONTENT

   When adding new content, be sure to pass all translatable strings through
   the _("gettext") function.  For example:
   
     <p><?=_("This sentence will be translated.")?></p>

   After adding or changing any translatable string, run "make" to update the
   message catalogs.  For significant changes, you may want to send the updated
   POT file to the translators before the change is published, so that they
   have a chance to update the PO files.
