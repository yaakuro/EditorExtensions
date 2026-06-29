
#pragma once

#include <Clients/EditorExtensionsSystemComponent.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/API/EntityPropertyEditorNotificationBus.h>

namespace EditorExtensions
{
    class ViewportScreenshotActionHandler;

    /// System component for EditorExtensions editor
    class EditorExtensionsEditorSystemComponent
        : public EditorExtensionsSystemComponent
        , protected AzToolsFramework::EditorEvents::Bus::Handler
        , protected AzToolsFramework::EntityPropertyEditorNotificationBus::Handler
    {
        using BaseSystemComponent = EditorExtensionsSystemComponent;
    public:
        AZ_COMPONENT_DECL(EditorExtensionsEditorSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        EditorExtensionsEditorSystemComponent();
        ~EditorExtensionsEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AzToolsFramework::EditorEventsBus overrides ...
        void NotifyRegisterViews() override;

        void SavePresetForComponent(const AZ::EntityComponentIdPair& pair);
        void LoadPresetForComponent(const AZ::EntityComponentIdPair& pair);


        // EntityPropertyEditorNotificationBus
        void OnComponentSelectionChanged(
            AzToolsFramework::EntityPropertyEditor* entityPropertyEditor,
            const AZStd::unordered_set<AZ::EntityComponentIdPair>& selectedEntityComponentIds) override;

        AzToolsFramework::EntityPropertyEditor* m_propertyEditor = nullptr;
        bool m_actionsAdded = false;

        // Owns the viewport screenshot action/menu/toolbar registration and in-flight captures.
        AZStd::unique_ptr<ViewportScreenshotActionHandler> m_viewportScreenshotActionHandler;
    };
} // namespace EditorExtensions
