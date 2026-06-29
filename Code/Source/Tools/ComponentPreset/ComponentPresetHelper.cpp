/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

#include <AzQtComponents/Components/Widgets/FileDialog.h>

#include <QFileDialog>

#include "ComponentPresetHelper.h"

namespace EditorExtensions
{
    bool ComponentPresetHelper::SavePreset(AZ::Component* component)
    {
        if (!component)
        {
            return false;
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        if (!serializeContext)
        {
            return false;
        }

        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
        {
            return false;
        }

        AZ::IO::FixedMaxPath projectRoot;
        {
            auto resolved = fileIO->ResolvePath("@projectroot@");
            if (!resolved)
            {
                return false;
            }
            projectRoot = *resolved;
        }

        AZ::IO::FixedMaxPath defaultDir = projectRoot / "Presets";

        QString selectedPath = AzQtComponents::FileDialog::GetSaveFileName_(
            AzToolsFramework::GetActiveWindow(),
            QObject::tr("Save Component Config Preset"),
            QString::fromUtf8(defaultDir.c_str(), aznumeric_cast<int>(defaultDir.Native().size())),
            QObject::tr("Component Preset (*.componentpreset)"));

        if (selectedPath.isEmpty())
        {
            return false;
        }

        AZStd::string absolutePath = selectedPath.toUtf8().data();

        AZ::IO::FileIOStream fileStream(
            absolutePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath);
        if (!fileStream.IsOpen())
        {
            AZ_Warning("ComponentPreset", false, "Failed to open file for writing: %s", absolutePath.c_str());
            return false;
        }

        bool success = AZ::Utils::SaveObjectToStream(
            fileStream, AZ::DataStream::ST_JSON, component, component->RTTI_GetType(), serializeContext);
        if (!success)
        {
            AZ_Warning("ComponentPreset", false, "Failed to serialize component to: %s", absolutePath.c_str());
        }

        return success;
    }

    bool ComponentPresetHelper::LoadPreset(AZ::Component* component)
    {
        if (!component)
        {
            return false;
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        if (!serializeContext)
        {
            return false;
        }

        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
        {
            return false;
        }

        AZ::IO::FixedMaxPath projectRoot;
        {
            auto resolved = fileIO->ResolvePath("@projectroot@");
            if (!resolved)
            {
                return false;
            }
            projectRoot = *resolved;
        }

        AZ::IO::FixedMaxPath defaultDir = projectRoot / "Presets";

        QString selectedPath = QFileDialog::getOpenFileName(
            AzToolsFramework::GetActiveWindow(),
            QObject::tr("Load Component Config Preset"),
            QString::fromUtf8(defaultDir.c_str(), aznumeric_cast<int>(defaultDir.Native().size())),
            QObject::tr("Component Preset (*.componentpreset)"));

        if (selectedPath.isEmpty())
        {
            return false;
        }

        AZStd::string absolutePath = selectedPath.toUtf8().data();

        // Only load a preset saved for the same component type.
        AZ::Uuid presetTypeId = AZ::Uuid::CreateNull();
        const bool inspected = AZ::Utils::InspectSerializedFile(
            absolutePath.c_str(), serializeContext,
            [&presetTypeId](void*, const AZ::Uuid& classId, AZ::SerializeContext*)
            {
                if (presetTypeId.IsNull())
                {
                    presetTypeId = classId;
                }
            });
        if (inspected && !presetTypeId.IsNull() && presetTypeId != component->RTTI_GetType())
        {
            AZ_Warning("ComponentPreset", false,
                       "Preset '%s' is for component type %s, but the selected component is %s - "
                       "not loading it.",
                       absolutePath.c_str(), presetTypeId.ToString<AZStd::string>().c_str(),
                       component->RTTI_GetType().ToString<AZStd::string>().c_str());
            return false;
        }

        AZ::IO::FileIOStream fileStream(absolutePath.c_str(), AZ::IO::OpenMode::ModeRead);
        if (!fileStream.IsOpen())
        {
            AZ_Warning("ComponentPreset", false, "Failed to open file: %s", absolutePath.c_str());
            return false;
        }

        // Save the component ID before load so we can restore it after
        // (the serialized "Id" field from the preset would overwrite it)
        AZ::ComponentId savedId = component->GetId();

        AZ::Utils::FilterDescriptor filter(nullptr, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES);

        bool success = AZ::Utils::LoadObjectFromStreamInPlace(
            fileStream, serializeContext, component->RTTI_GetType(), component, filter);

        if (!success)
        {
            AZ_Warning("ComponentPreset", false,
                       "Failed to load preset - incompatible component type or corrupted file: %s",
                       absolutePath.c_str());
            return false;
        }

        // Restore the original component ID since the preset should not change which component this is
        component->SetId(savedId);

        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay,
            AzToolsFramework::Refresh_AttributesAndValues);

        return true;
    }
} // namespace PCG
