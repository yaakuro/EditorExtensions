/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <AzQtComponents/Components/Widgets/BrowseEdit.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>

namespace EditorExtensions

{
    namespace Panel
    {
        class AttachmentPreviewPanel final : public QWidget
        {
            Q_OBJECT

        public:
            explicit AttachmentPreviewPanel(QWidget* parent = nullptr);
            ~AttachmentPreviewPanel() override;

        private slots:
            void OnActorChanged(int index);
            void OnAnimationChanged(int index);
            void OnPlayClicked();
            void OnPauseClicked();
            void OnStopClicked();
            void OnBrowsePrefabClicked();
            void OnAddAttachmentClicked();
            void OnRemoveAttachmentClicked(QListWidgetItem* item);

        private:
            void CreateLayout();
            QWidget* CreateActorGroup();
            QGroupBox* CreateAnimationGroup();
            QGroupBox* CreateAttachmentGroup();
            void RefreshActorList();
            void RefreshAnimationList();
            void RefreshBoneList();
            void ClearAnimations();

            // Actor selection group
            QComboBox* m_actorDropdown = nullptr;

            // Animation playback group
            QComboBox* m_animationDropdown = nullptr;
            QPushButton* m_playButton = nullptr;
            QPushButton* m_pauseButton = nullptr;
            QPushButton* m_stopButton = nullptr;

            // Attachment group
            AzQtComponents::BrowseEdit* m_prefabPathEdit = nullptr;
            QComboBox* m_boneDropdown = nullptr;
            QLineEdit* m_offsetXEdit = nullptr;
            QLineEdit* m_offsetYEdit = nullptr;
            QLineEdit* m_offsetZEdit = nullptr;
            QPushButton* m_addButton = nullptr;

            // Attachment list
            QListWidget* m_attachmentList = nullptr;

            QGroupBox* m_animationGroup = nullptr;
            QGroupBox* m_attachmentGroup = nullptr;

            AZ::EntityId m_currentActorEntityId;
        };
    } // namespace Panel
} // namespace AttachmentPreview
