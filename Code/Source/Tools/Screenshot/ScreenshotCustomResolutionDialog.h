/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QDialog>

class QSpinBox;

namespace EditorExtensions
{
    //! Minimal width/height picker for screenshot captures, replacing the Editor-internal
    //! CCustomResolutionDlg so this gem has no dependency on EditorLib headers.
    class ScreenshotCustomResolutionDialog
        : public QDialog
    {
        Q_OBJECT
    public:
        ScreenshotCustomResolutionDialog(int width, int height, QWidget* parent = nullptr);

        int GetWidth() const;
        int GetHeight() const;

    private:
        QSpinBox* m_width = nullptr;
        QSpinBox* m_height = nullptr;
    };
} // namespace EditorExtensions
