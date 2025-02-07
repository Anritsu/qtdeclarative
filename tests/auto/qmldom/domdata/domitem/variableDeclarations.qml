// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick

Item {
    // for now only numeric literal and string literal is supported
    function f() {
        let sum = 0, helloWorld = "hello"
        const a = 3;
        const b = "patron";
        var aa = helloWorld, bb = aa;

        const bool1 = true;
        let bool2 = false;
        var nullVar = null;
        return sum;
    }
}
