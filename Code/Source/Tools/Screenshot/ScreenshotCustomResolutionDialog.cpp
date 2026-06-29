/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Tools/Screenshot/ScreenshotCustomResolutionDialog.h>

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

namespace
{
    // Matches the Editor's CustomResolutionDlg bounds.
    constexpr int MinResolution = 64;
    constexpr int MaxResolution = 8192;
} // namespace

namespace EditorExtensions
{
    ScreenshotCustomResolutionDialog::ScreenshotCustomResolutionDialog(int width, int height, QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle(tr("Custom Screenshot Resolution"));
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

        m_width = new QSpinBox(this);
        m_width->setRange(MinResolution, MaxResolution);
        m_width->setValue(width);

        m_height = new QSpinBox(this);
        m_height->setRange(MinResolution, MaxResolution);
        m_height->setValue(height);

        auto* formLayout = new QFormLayout;
        formLayout->addRow(tr("Width:"), m_width);
        formLayout->addRow(tr("Height:"), m_height);

        auto* infoLabel = new QLabel(
            tr("Maximum Dimension: %1\n\nNote: Dimensions over 8K may be\nunstable depending on hardware.").arg(MaxResolution),
            this);

        auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->addLayout(formLayout);
        mainLayout->addWidget(infoLabel);
        mainLayout->addWidget(buttonBox);
    }

    int ScreenshotCustomResolutionDialog::GetWidth() const
    {
        return m_width->value();
    }

    int ScreenshotCustomResolutionDialog::GetHeight() const
    {
        return m_height->value();
    }
} // namespace EditorExtensions
