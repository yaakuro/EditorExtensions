/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AttachmentPreviewManager.h"

#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/PlayBackInfo.h>
#include <EMotionFX/Source/MotionSystem.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Actor.h>
#include <Integration/SimpleMotionComponentBus.h>
#include <Integration/Assets/MotionAsset.h>

#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/IO/Path/Path.h>

#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Entity/EntityContextBus.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Prefab/PrefabPublicRequestBus.h>

#include <LmbrCentral/Animation/AttachmentComponentBus.h>

#include <QFile>

namespace EditorExtensions
{
    namespace Manager
    {
        AttachmentPreviewManager* AttachmentPreviewManager::GetInstance()
        {
            static AttachmentPreviewManager instance;
            return &instance;
        }

        AzFramework::EntityContextId AttachmentPreviewManager::GetEditorEntityContextId() const
        {
            AzFramework::EntityContextId contextId{};
            AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                contextId, &AzToolsFramework::EditorEntityContextRequests::GetEditorEntityContextId);
            return contextId;
        }

        void AttachmentPreviewManager::PlayAnimation(const char* motionFileName, bool loop)
        {
            if (!m_currentActorEntityId.IsValid())
            {
                return;
            }

            EMotionFX::ActorInstance* actorInstance = nullptr;
            EMotionFX::ActorManager& actorManager = EMotionFX::GetActorManager();
            for (size_t i = 0; i < actorManager.GetNumRootActorInstances(); ++i)
            {
                EMotionFX::ActorInstance* instance = actorManager.GetRootActorInstance(i);
                if (instance->GetEntityId() == m_currentActorEntityId)
                {
                    actorInstance = instance;
                    break;
                }
            }

            if (!actorInstance)
            {
                return;
            }

            EMotionFX::MotionSystem* motionSystem = actorInstance->GetMotionSystem();
            if (!motionSystem)
            {
                return;
            }

            if (motionFileName && motionFileName[0] != '\0')
            {
                AZ::IO::Path path(motionFileName);
                AZStd::string normalized = path.LexicallyNormal().String();

                AZ::Data::AssetId motionAssetId;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                    motionAssetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, 
                    normalized.c_str(), azrtti_typeid<EMotionFX::Integration::MotionAsset>(), false);

                if (motionAssetId.IsValid())
                {
                    auto motionAsset = AZ::Data::AssetManager::Instance().GetAsset<EMotionFX::Integration::MotionAsset>(motionAssetId, AZ::Data::AssetLoadBehavior::QueueLoad);
                    if (motionAsset)
                    {
                        EMotionFX::PlayBackInfo info;
                        info.m_numLoops = loop ? EMFX_LOOPFOREVER : 1;
                        info.m_playMode = EMotionFX::PLAYMODE_FORWARD;
                        info.m_playNow = true;

                        motionSystem->PlayMotion(motionAsset->m_emfxMotion.get(), &info);
                    }
                }
            }
        }

        void AttachmentPreviewManager::PauseAnimation()
        {
            if (!m_currentActorEntityId.IsValid())
            {
                return;
            }

            EMotionFX::ActorManager& actorManager = EMotionFX::GetActorManager();
            for (size_t i = 0; i < actorManager.GetNumRootActorInstances(); ++i)
            {
                EMotionFX::ActorInstance* instance = actorManager.GetRootActorInstance(i);
                if (instance->GetEntityId() == m_currentActorEntityId)
                {
                    EMotionFX::MotionSystem* motionSystem = instance->GetMotionSystem();
                    if (motionSystem)
                    {
                        motionSystem->StopAllMotions();
                    }
                    break;
                }
            }
        }

        void AttachmentPreviewManager::StopAnimation()
        {
            PauseAnimation();
        }

        AZ::EntityId AttachmentPreviewManager::SpawnPrefabAttachment(
            const char* prefabFilePath,
            AZ::EntityId targetActorEntity,
            const char* boneName,
            float offsetX, float offsetY, float offsetZ)
        {
            if (!prefabFilePath || !targetActorEntity.IsValid())
            {
                return AZ::EntityId{};
            }

            AZ::IO::Path path(prefabFilePath);
            AZStd::string normalizedPath = path.LexicallyNormal().String();

            AzToolsFramework::Prefab::InstantiatePrefabResult instantiateResult;
            AzToolsFramework::Prefab::PrefabPublicRequestBus::BroadcastResult(
                instantiateResult, &AzToolsFramework::Prefab::PrefabPublicRequests::InstantiatePrefab, 
                normalizedPath, AZ::EntityId{}, AZ::Vector3::CreateZero());

            if (!instantiateResult.IsSuccess())
            {
                return AZ::EntityId{};
            }

            AZ::EntityId containerEntityId = instantiateResult.GetValue();
            AZ::Entity* attachedEntity = nullptr;
            
            AZStd::vector<AZ::EntityId> children;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(children, containerEntityId, &AzToolsFramework::EditorEntityInfoRequests::GetChildren);
            if (!children.empty())
            {
                AZ::ComponentApplicationBus::BroadcastResult(attachedEntity, &AZ::ComponentApplicationRequests::FindEntity, children[0]);
            }
            
            if (!attachedEntity)
            {
                AZ::ComponentApplicationBus::BroadcastResult(attachedEntity, &AZ::ComponentApplicationRequests::FindEntity, containerEntityId);
            }

            if (!attachedEntity)
            {
                return AZ::EntityId{};
            }

            AZ::Transform offsetTransform = AZ::Transform::CreateTranslation(AZ::Vector3(offsetX, offsetY, offsetZ));

            LmbrCentral::AttachmentComponentRequestBus::Event(
                attachedEntity->GetId(),
                &LmbrCentral::AttachmentComponentRequests::Attach,
                targetActorEntity,
                boneName ? boneName : "",
                offsetTransform);

            AttachmentInfo info;
            info.attachedEntityId = attachedEntity->GetId();
            info.targetActorEntityId = targetActorEntity;
            info.boneName = boneName ? boneName : "";
            info.offset = AZ::Vector3(offsetX, offsetY, offsetZ);

            m_attachments.push_back(AZStd::move(info));

            return attachedEntity->GetId();
        }

        void AttachmentPreviewManager::RemoveAttachment(AZ::EntityId attachedEntityId)
        {
            if (!attachedEntityId.IsValid())
            {
                return;
            }

            LmbrCentral::AttachmentComponentRequestBus::Event(
                attachedEntityId,
                &LmbrCentral::AttachmentComponentRequests::Detach);

            auto it = AZStd::find_if(m_attachments.begin(), m_attachments.end(), [attachedEntityId](const AttachmentInfo& info) { 
                return info.attachedEntityId == attachedEntityId; 
            });
            
            if (it != m_attachments.end())
            {
                m_attachments.erase(it);
            }

            AzFramework::EntityContextRequestBus::Event(
                GetEditorEntityContextId(),
                &AzFramework::EntityContextRequests::DestroyEntityById, 
                attachedEntityId);
        }

        void AttachmentPreviewManager::DetachAll()
        {
            for (const auto& attachment : m_attachments)
            {
                LmbrCentral::AttachmentComponentRequestBus::Event(
                    attachment.attachedEntityId,
                    &LmbrCentral::AttachmentComponentRequests::Detach);

                AzFramework::EntityContextRequestBus::Event(
                    GetEditorEntityContextId(),
                    &AzFramework::EntityContextRequests::DestroyEntityById, 
                    attachment.attachedEntityId);
            }

            m_attachments.clear();
        }
    } // namespace Manager
} // namespace AttachmentPreview
