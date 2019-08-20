/* vim:fileencoding=utf-8
 * 
 * Copyright (C) 2019 Kovid Goyal <kovid at kovidgoyal.net>
 *
 * Distributed under terms of the GPLv3 license
 */

(function() {
    "use strict";

    function fix_google_markup() {
        var cc = document.getElementById('center_col');
        cc.style.marginLeft = '0';
        cc = document.getElementById('cnt');
        if (cc) cc.style.paddingTop = '0';
        ['sfcnt', 'top_nav', 'before-appbar', 'appbar', 'searchform', 'easter-egg'].forEach(function(id) {
            var elem = document.getElementById(id);
            if (elem) elem.style.display = 'none';
        });
    }

    if (window.location.hostname === 'www.google.com') fix_google_markup();
})();
