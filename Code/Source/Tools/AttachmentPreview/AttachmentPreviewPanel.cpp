/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AttachmentPreviewPanel.h"

#include "AttachmentPreviewManager.h"
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionSystem.h>
#include <EMotionFX/Source/Motion.h>
#include <LmbrCentral/Animation/SkeletalHierarchyRequestBus.h>

#include <QFileDialog>

namespace EditorExtensions
{
    namespace Panel
    {
        AttachmentPreviewPanel::AttachmentPreviewPanel(QWidget* parent)
            : QWidget(parent)
        {
            CreateLayout();
        }

        AttachmentPreviewPanel::~AttachmentPreviewPanel() = default;

        void AttachmentPreviewPanel::CreateLayout()
        {
            auto* mainLayout = new QVBoxLayout(this);
            mainLayout->setContentsMargins(4, 4, 4, 4);
            mainLayout->setSpacing(6);

            auto* actorGroup = CreateActorGroup();
            mainLayout->addWidget(actorGroup);

            m_animationGroup = CreateAnimationGroup();
            mainLayout->addWidget(m_animationGroup);

            m_attachmentGroup = CreateAttachmentGroup();
            mainLayout->addWidget(m_attachmentGroup);

            auto* refreshButton = new QPushButton("Refresh Lists", this);
            connect(refreshButton, &QPushButton::clicked, this, [this]() {
                RefreshActorList();
                RefreshAnimationList();
                RefreshBoneList();
            });
            mainLayout->addWidget(refreshButton);

            mainLayout->addStretch();
        }

        QWidget* AttachmentPreviewPanel::CreateActorGroup()
        {
            auto* group = new QGroupBox("Select Actor", this);
            auto* layout = new QVBoxLayout(group);
            layout->setContentsMargins(4, 4, 4, 4);

            auto* label = new QLabel("Actor:", this);
            layout->addWidget(label);

            m_actorDropdown = new QComboBox(this);
            connect(m_actorDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged), 
                    this, &AttachmentPreviewPanel::OnActorChanged);
            layout->addWidget(m_actorDropdown);

            RefreshActorList();

            return group;
        }

        QGroupBox* AttachmentPreviewPanel::CreateAnimationGroup()
        {
            auto* group = new QGroupBox("Animation Playback", this);
            auto* layout = new QVBoxLayout(group);
            layout->setContentsMargins(4, 4, 4, 4);

            m_animationDropdown = new QComboBox(this);
            connect(m_animationDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged), 
                    this, &AttachmentPreviewPanel::OnAnimationChanged);
            layout->addWidget(m_animationDropdown);

            auto* buttonRow = new QHBoxLayout();
            m_playButton = new QPushButton("Play", this);
            connect(m_playButton, &QPushButton::clicked, this, &AttachmentPreviewPanel::OnPlayClicked);
            buttonRow->addWidget(m_playButton);

            m_pauseButton = new QPushButton("Pause", this);
            connect(m_pauseButton, &QPushButton::clicked, this, &AttachmentPreviewPanel::OnPauseClicked);
            buttonRow->addWidget(m_pauseButton);

            m_stopButton = new QPushButton("Stop", this);
            connect(m_stopButton, &QPushButton::clicked, this, &AttachmentPreviewPanel::OnStopClicked);
            buttonRow->addWidget(m_stopButton);

            layout->addLayout(buttonRow);

            auto* spacer = new QWidget(this);
            spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            layout->addWidget(spacer);

            return group;
        }

        QGroupBox* AttachmentPreviewPanel::CreateAttachmentGroup()
        {
            auto* group = new QGroupBox("Attachments", this);
            auto* layout = new QVBoxLayout(group);
            layout->setContentsMargins(4, 4, 4, 4);

            auto* filePathRow = new QHBoxLayout();
            filePathRow->addWidget(new QLabel("Prefab:", this));
            m_prefabPathEdit = new AzQtComponents::BrowseEdit(this);
            connect(m_prefabPathEdit, &AzQtComponents::BrowseEdit::attachedButtonTriggered, 
                    this, &AttachmentPreviewPanel::OnBrowsePrefabClicked);
            filePathRow->addWidget(m_prefabPathEdit, 1);
            layout->addLayout(filePathRow);

            auto* boneRow = new QHBoxLayout();
            boneRow->addWidget(new QLabel("Bone:", this));
            m_boneDropdown = new QComboBox(this);
            boneRow->addWidget(m_boneDropdown, 1);
            layout->addLayout(boneRow);

            auto* offsetRow = new QHBoxLayout();
            offsetRow->addWidget(new QLabel("Offset (X,Y,Z):", this));
            m_offsetXEdit = new QLineEdit("0", this);
            m_offsetYEdit = new QLineEdit("0", this);
            m_offsetZEdit = new QLineEdit("0", this);
            offsetRow->addWidget(m_offsetXEdit);
            offsetRow->addWidget(m_offsetYEdit);
            offsetRow->addWidget(m_offsetZEdit);
            layout->addLayout(offsetRow);

            m_addButton = new QPushButton("Add Attachment", this);
            connect(m_addButton, &QPushButton::clicked, this, &AttachmentPreviewPanel::OnAddAttachmentClicked);
            layout->addWidget(m_addButton);

            m_attachmentList = new QListWidget(this);
            connect(m_attachmentList, &QListWidget::itemDoubleClicked, this, &AttachmentPreviewPanel::OnRemoveAttachmentClicked);
            layout->addWidget(m_attachmentList);

            return group;
        }

        void AttachmentPreviewPanel::RefreshActorList()
        {
            m_actorDropdown->blockSignals(true);
            m_actorDropdown->clear();

            EMotionFX::ActorManager& actorManager = EMotionFX::GetActorManager();
            for (size_t i = 0; i < actorManager.GetNumRootActorInstances(); ++i)
            {
                EMotionFX::ActorInstance* instance = actorManager.GetRootActorInstance(i);
                AZ::EntityId entityId = instance->GetEntityId();
                if (entityId.IsValid())
                {
                    const char* name = nullptr;
                    if (instance->GetActor())
                    {
                        name = instance->GetActor()->GetName();
                    }
                    
                    if (!name || name[0] == '\0')
                    {
                        AZ::Entity* entity = nullptr;
                        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
                        if (entity)
                        {
                            name = entity->GetName().c_str();
                        }
                    }

                    m_actorDropdown->addItem(QString(name ? name : "Unnamed"), QVariant::fromValue(entityId));
                }
            }

            m_actorDropdown->blockSignals(false);
        }

        void AttachmentPreviewPanel::RefreshAnimationList()
        {
            m_animationDropdown->blockSignals(true);
            m_animationDropdown->clear();

            if (!m_currentActorEntityId.IsValid())
            {
                m_animationDropdown->blockSignals(false);
                return;
            }

            EMotionFX::MotionManager& motionManager = EMotionFX::GetMotionManager();
            for (size_t i = 0; i < motionManager.GetNumMotions(); ++i)
            {
                EMotionFX::Motion* motion = motionManager.GetMotion(i);
                m_animationDropdown->addItem(motion->GetFileName(), QVariant::fromValue(AZStd::string(motion->GetFileName())));
            }

            m_animationDropdown->blockSignals(false);
        }

        void AttachmentPreviewPanel::RefreshBoneList()
        {
            m_boneDropdown->blockSignals(true);
            m_boneDropdown->clear();

            if (!m_currentActorEntityId.IsValid())
            {
                m_boneDropdown->blockSignals(false);
                return;
            }

            uint32_t jointCount = 0;
            LmbrCentral::SkeletalHierarchyRequestBus::EventResult(
                jointCount, m_currentActorEntityId, 
                &LmbrCentral::SkeletalHierarchyRequests::GetJointCount);

            for (uint32_t i = 0; i < jointCount; ++i)
            {
                const char* jointName = nullptr;
                LmbrCentral::SkeletalHierarchyRequestBus::EventResult(
                    jointName, m_currentActorEntityId, 
                    &LmbrCentral::SkeletalHierarchyRequests::GetJointNameByIndex, i);
                if (jointName)
                {
                    m_boneDropdown->addItem(jointName);
                }
            }

            m_boneDropdown->blockSignals(false);
        }

        void AttachmentPreviewPanel::ClearAnimations()
        {
            m_animationDropdown->clear();
        }

        void AttachmentPreviewPanel::OnActorChanged(int index)
        {
            if (index < 0)
            {
                return;
            }

            AZ::EntityId newEntityId = m_actorDropdown->itemData(index).value<AZ::EntityId>();
            if (newEntityId == m_currentActorEntityId)
            {
                return;
            }

            m_currentActorEntityId = newEntityId;

            Manager::AttachmentPreviewManager::GetInstance()->SetCurrentActorEntityId(m_currentActorEntityId);

            RefreshAnimationList();
            RefreshBoneList();
        }

        void AttachmentPreviewPanel::OnAnimationChanged(int /*index*/)
        {
        }

        void AttachmentPreviewPanel::OnPlayClicked()
        {
            int index = m_animationDropdown->currentIndex();
            if (index >= 0)
            {
                AZStd::string motionPath = m_animationDropdown->itemData(index).value<AZStd::string>();
                Manager::AttachmentPreviewManager::GetInstance()->PlayAnimation(motionPath.c_str(), true);
            }
        }

        void AttachmentPreviewPanel::OnPauseClicked()
        {
            Manager::AttachmentPreviewManager::GetInstance()->PauseAnimation();
        }

        void AttachmentPreviewPanel::OnStopClicked()
        {
            Manager::AttachmentPreviewManager::GetInstance()->StopAnimation();
        }

        void AttachmentPreviewPanel::OnBrowsePrefabClicked()
        {
            QString selectedFile = QFileDialog::getOpenFileName(
                this,
                "Select Prefab File",
                "",
                "Prefab Files (*.prefab);;All Files (*)");

            if (!selectedFile.isEmpty())
            {
                m_prefabPathEdit->setText(selectedFile);
            }
        }

        void AttachmentPreviewPanel::OnAddAttachmentClicked()
        {
            QString prefabPath = m_prefabPathEdit->text();
            QString boneName = m_boneDropdown->currentText();
            float offsetX = m_offsetXEdit->text().toFloat();
            float offsetY = m_offsetYEdit->text().toFloat();
            float offsetZ = m_offsetZEdit->text().toFloat();

            if (prefabPath.isEmpty() || !m_currentActorEntityId.IsValid())
            {
                return;
            }

            AZ::EntityId attachedId = Manager::AttachmentPreviewManager::GetInstance()->SpawnPrefabAttachment(
                prefabPath.toUtf8().constData(),
                m_currentActorEntityId,
                boneName.toUtf8().constData(),
                offsetX, offsetY, offsetZ);

            if (attachedId.IsValid())
            {
                auto* item = new QListWidgetItem(QString("%1 -> %2").arg(prefabPath).arg(boneName), m_attachmentList);
                item->setData(Qt::UserRole, QVariant::fromValue(attachedId));
            }
        }

        void AttachmentPreviewPanel::OnRemoveAttachmentClicked(QListWidgetItem* item)
        {
            if (!item) return;

            AZ::EntityId attachedId = item->data(Qt::UserRole).value<AZ::EntityId>();
            Manager::AttachmentPreviewManager::GetInstance()->RemoveAttachment(attachedId);
            delete item;
        }
    } // namespace Panel
} // namespace AttachmentPreview
