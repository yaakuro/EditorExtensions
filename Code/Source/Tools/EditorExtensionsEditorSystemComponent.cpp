
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/EntityPropertyEditorRequestsBus.h>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include "EditorExtensionsWidget.h"
#include "EditorExtensionsEditorSystemComponent.h"

#include <EditorExtensions/EditorExtensionsTypeIds.h>
#include <Tools/CppGenerator/CppGeneratorWidget.h>

#include <QAction>
#include <QMenu>

#include <Tools/ComponentPreset/ComponentPresetHelper.h>
#include <Tools/Screenshot/ViewportScreenshotActionHandler.h>

namespace EditorExtensions
{
    AZ_COMPONENT_IMPL(EditorExtensionsEditorSystemComponent, "EditorExtensionsEditorSystemComponent",
        EditorExtensionsEditorSystemComponentTypeId, BaseSystemComponent);

    void EditorExtensionsEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorExtensionsEditorSystemComponent, EditorExtensionsSystemComponent>()
                ->Version(0);
        }
    }

    EditorExtensionsEditorSystemComponent::EditorExtensionsEditorSystemComponent() = default;

    EditorExtensionsEditorSystemComponent::~EditorExtensionsEditorSystemComponent() = default;

    void EditorExtensionsEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("EditorExtensionsEditorService"));
    }

    void EditorExtensionsEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("EditorExtensionsEditorService"));
    }

    void EditorExtensionsEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void EditorExtensionsEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void EditorExtensionsEditorSystemComponent::Activate()
    {
        AzToolsFramework::EntityPropertyEditorNotificationBus::Handler::BusConnect();
        EditorExtensionsSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();

        m_viewportScreenshotActionHandler = AZStd::make_unique<ViewportScreenshotActionHandler>();
        m_viewportScreenshotActionHandler->Activate();
    }

    void EditorExtensionsEditorSystemComponent::Deactivate()
    {
        if (m_viewportScreenshotActionHandler)
        {
            m_viewportScreenshotActionHandler->Deactivate();
            m_viewportScreenshotActionHandler.reset();
        }

        AzToolsFramework::EntityPropertyEditorNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        EditorExtensionsSystemComponent::Deactivate();
    }

    void EditorExtensionsEditorSystemComponent::NotifyRegisterViews()
    {
        AzToolsFramework::ViewPaneOptions options;
        options.paneRect = QRect(100, 100, 500, 400);
        options.showOnToolsToolbar = true;
        options.toolbarIcon = ":/EditorExtensions/toolbar_icon.svg";

        // Register our custom widget as a dockable tool with the Editor under an Examples sub-menu
        AzToolsFramework::RegisterViewPane<EditorExtensionsWidget>("Editor Extensions", "Examples", options);


        AzToolsFramework::ViewPaneOptions cppOptions;
        cppOptions.paneRect = QRect(100, 100, 500, 400);
        cppOptions.showOnToolsToolbar = true;
        cppOptions.toolbarIcon = ":/CppGenerator/cpp_generator.svg";

        // Register our custom widget as a dockable tool with the Editor under an Examples sub-menu
        AzToolsFramework::RegisterViewPane<CppGeneratorWidget>("Cpp Generator", "Examples", cppOptions);
    }

    void EditorExtensionsEditorSystemComponent::OnComponentSelectionChanged(
        AzToolsFramework::EntityPropertyEditor* entityPropertyEditor,
        [[maybe_unused]] const AZStd::unordered_set<AZ::EntityComponentIdPair>& selectedEntityComponentIds)
    {
        if (!entityPropertyEditor)
        {
            return;
        }

        m_propertyEditor = entityPropertyEditor;
        QAction* seperator = new QAction();
        seperator->setSeparator(true);
        m_propertyEditor->addAction(seperator);

        if (m_actionsAdded)
        {
            return;
        }
        m_actionsAdded = true;

        auto saveAction = new QAction(QObject::tr("Save Config Preset..."), m_propertyEditor);
        QObject::connect(saveAction, &QAction::triggered, m_propertyEditor, [this]() {
            if (!m_propertyEditor)
            {
                return;
            }
            AZStd::unordered_set<AZ::EntityComponentIdPair> selected;
            AzToolsFramework::EntityPropertyEditorRequestBus::Broadcast(
                &AzToolsFramework::EntityPropertyEditorRequestBus::Events::GetSelectedComponents,
                selected);
            if (selected.size() != 1)
            {
                return;
            }
            SavePresetForComponent(*selected.begin());
        });
        m_propertyEditor->addAction(saveAction);

        auto loadAction = new QAction(QObject::tr("Load Config Preset..."), m_propertyEditor);
        QObject::connect(loadAction, &QAction::triggered, m_propertyEditor, [this]() {
            if (!m_propertyEditor)
            {
                return;
            }
            AZStd::unordered_set<AZ::EntityComponentIdPair> selected;
            AzToolsFramework::EntityPropertyEditorRequestBus::Broadcast(
                &AzToolsFramework::EntityPropertyEditorRequestBus::Events::GetSelectedComponents,
                selected);
            if (selected.size() != 1)
            {
                return;
            }
            LoadPresetForComponent(*selected.begin());

            // Resolve component pointer and re-activate to sync internal state.
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(
                entity, &AZ::ComponentApplicationBus::Events::FindEntity, (*selected.begin()).GetEntityId());
            if (entity)
            {
                if (auto* component = entity->FindComponent((*selected.begin()).GetComponentId()))
                {
                    AZStd::vector<AZ::Component*> components{ component };
                    AzToolsFramework::EntityCompositionRequestBus::Broadcast(
                        &AzToolsFramework::EntityCompositionRequests::DisableComponents, components);
                    AzToolsFramework::EntityCompositionRequestBus::Broadcast(
                        &AzToolsFramework::EntityCompositionRequests::EnableComponents, components);
                }
            }
        });
        m_propertyEditor->addAction(loadAction);
    }

    void EditorExtensionsEditorSystemComponent::SavePresetForComponent(const AZ::EntityComponentIdPair& pair)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            entity, &AZ::ComponentApplicationBus::Events::FindEntity, pair.GetEntityId());
        if (!entity)
        {
            return;
        }

        AZ::Component* component = entity->FindComponent(pair.GetComponentId());
        if (!component)
        {
            return;
        }

        ComponentPresetHelper::SavePreset(component);
    }

    void EditorExtensionsEditorSystemComponent::LoadPresetForComponent(const AZ::EntityComponentIdPair& pair)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            entity, &AZ::ComponentApplicationBus::Events::FindEntity, pair.GetEntityId());
        if (!entity)
        {
            return;
        }

        AZ::Component* component = entity->FindComponent(pair.GetComponentId());
        if (!component)
        {
            return;
        }

        ComponentPresetHelper::LoadPreset(component);
    }
} // namespace EditorExtensions
