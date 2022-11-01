// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

QtObject {
    property var supportedStates: [
        [],
        ["disabled"],
        ["pressed"],
        ["checkable", "checked"],
        ["checkable", "checked", "disabled"],
        ["checkable", "checked"],
        ["highlighted"],
        ["highlighted", "disabled"],
        ["highlighted", "pressed"],
        ["highlighted", "checkable"],
        ["highlighted", "checkable", "pressed"],
        ["highlighted", "checkable", "checked"],
        ["flat"],
        ["flat", "disabled"],
        ["flat", "pressed"],
        ["flat", "checkable"],
        ["flat", "checkable", "checked"],
        ["flat", "checkable", "pressed"],
        ["flat", "checkable", "checked", "pressed"],
        ["flat", "checkable", "highlighted"],
        ["flat", "checkable", "highlighted", "pressed"],
        ["flat", "checkable", "highlighted", "checked"]
    ]

    property Component component: Button {
        text: "Button"
        enabled: !is("disabled")
        flat: is("flat")
        checkable: is("checkable")
        checked: is("checked")
        // Only set it if it's pressed, or the non-pressed examples will have no press effects
        down: is("pressed") ? true : undefined
        highlighted: is("highlighted")
    }
}
