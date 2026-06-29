/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/EBus/EBus.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Prefab/PrefabPublicRequestBus.h>

#include <QString>

namespace EditorExtensions
{
    namespace Manager
    {
        class AttachmentPreviewManager final
            : public AZ::EBusTraits
        {
        public:
            AZ_TYPE_INFO(AttachmentPreviewManager, "{B230E0F1-57D4-4E5C-B230-E0F157D44E5C}");

            static AttachmentPreviewManager* GetInstance();

            // Actor + animation control
            void SetCurrentActorEntityId(AZ::EntityId entityId) { m_currentActorEntityId = entityId; }
            AZ::EntityId GetCurrentActorEntityId() const { return m_currentActorEntityId; }
            void PlayAnimation(const char* motionFileName, bool loop = true);
            void PauseAnimation();
            void StopAnimation();

            // Prefab attachment management
            AZ::EntityId SpawnPrefabAttachment(
                const char* prefabFilePath,
                AZ::EntityId targetActorEntity,
                const char* boneName,
                float offsetX, float offsetY, float offsetZ);

            void RemoveAttachment(AZ::EntityId attachedEntityId);
            void DetachAll();

        private:
            AzFramework::EntityContextId GetEditorEntityContextId() const;

            struct AttachmentInfo
            {
                AZ::EntityId attachedEntityId;
                AZ::EntityId targetActorEntityId;
                QString boneName;
                AZ::Vector3 offset;
            };

            AZ::EntityId m_currentActorEntityId;
            AZStd::vector<AttachmentInfo> m_attachments;
        };
    } // namespace Manager
} // namespace AttachmentPreview
